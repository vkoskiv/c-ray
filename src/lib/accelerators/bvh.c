//
//  bvh.c
//  c-ray
//
//  Created by Arsène Pérard-Gayot on 07/06/2020.
//  Copyright © 2020-2025 Arsène Pérard-Gayot (@madmann91), Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "bvh.h"

#include <renderer/pathtrace.h>

#include <datatypes/bbox.h>
#include <datatypes/mesh.h>
#include <datatypes/poly.h>
#include <renderer/instance.h>
#include <common/vector.h>
#include <common/platform/thread.h>
#include <common/platform/thread_pool.h>
#include <common/platform/capabilities.h>
#include <common/platform/signal.h>
#include <common/timer.h>

#include <limits.h>
#include <assert.h>
#include <float.h>
#include <stdlib.h>
#include <math.h>

/*
 * This BVH builder is based on "On fast Construction of SAH-based Bounding Volume Hierarchies",
 * by I. Wald. The general idea is to approximate the SAH by subdividing each axis in several
 * bins. Primitives are then placed into those bins according to their centers, and then the
 * split is found by sweeping the bin array to find the partition with the lowest SAH.
 * The algorithm needs only a bounding box and a center per primitive, which allows to build
 * BVHs out of any primitive type, including other BVHs (necessary for instancing).
 */

#define PRIM_COUNT_BITS  4    // Number of bits for the primitive count in a leaf
#define MAX_BVH_DEPTH    64   // This should be enough for most scenes
#define TRAVERSAL_COST   1.5f // Ratio (cost of traversing a node / cost of intersecting a primitive)
#define BIN_COUNT        32   // Number of bins to use to approximate the SAH
#define ROBUST_TRAVERSAL 0    // Set to 1 in order to use a fully robust algo. (from T. Ize's "Robust BVH Ray Traversal")
#define MAX_LEAF_SIZE    ((1 << PRIM_COUNT_BITS) - 1)

typedef size_t index_t;
typedef bool (*intersect_leaf_fn_t)(
	const void *,
	const struct bvh *,
	const struct lightRay *,
	size_t, size_t,
	struct hitRecord *);

// This structure has the same size as `index_type`
struct bvh_index {
	index_t first_child_or_prim : sizeof(index_t) * CHAR_BIT - PRIM_COUNT_BITS;
	index_t prim_count : PRIM_COUNT_BITS;
};

struct bvh_node {
	float bounds[6];        // Node bounds (min x, max x, min y, max y, ...)
	struct bvh_index index; // Indices pointing to primitives and children (if any)
};

struct bvh {
	struct bvh_node *nodes;
	size_t *prim_indices;
	size_t node_count;
};

// Bin used to approximate the SAH.
struct bin {
	struct boundingBox bbox;
	size_t count;
};

struct split {
	unsigned axis;
	float cost;
	size_t pos;
};

struct top_level_data {
	const struct instance *instances;
	sampler *sampler;
};

// A small wrapper that generates an FMA when the target arch. supports it
static inline float fast_mul_add(float a, float b, float c) {
#ifdef FP_FAST_FMAF
	return fmaf(a, b, c);
#else
#ifdef __clang__
#pragma STDC FP_CONTRACT ON
#endif
	return a * b + c;
#endif
}

// Because the comparisons are of the form x < y ? x : y, they
// are guaranteed not to produce NaNs if the right hand side is not a NaN.
// See T. Ize's "Robust BVH Ray Traversal" article.
static inline float robust_min(float a, float b) { return a < b ? a : b; }
static inline float robust_max(float a, float b) { return a > b ? a : b; }

#if !ROBUST_TRAVERSAL
static inline float safe_inverse(float x) { return fabsf(x) <= FLT_EPSILON ? 1.f / FLT_EPSILON : 1.f / x; }
#endif

static inline unsigned find_largest_axis(const struct vector *v) {
	if (v->y >= v->z && v->y >= v->x) return 1;
	if (v->z >= v->y && v->z >= v->x) return 2;
	return 0;
}

static inline void store_bbox_to_node(struct bvh_node *node, const struct boundingBox *bbox) {
	node->bounds[0] = bbox->min.x;
	node->bounds[1] = bbox->max.x;
	node->bounds[2] = bbox->min.y;
	node->bounds[3] = bbox->max.y;
	node->bounds[4] = bbox->min.z;
	node->bounds[5] = bbox->max.z;
}

static inline struct boundingBox load_bbox_from_node(const struct bvh_node *node) {
	return (struct boundingBox) {
		.min = { node->bounds[0], node->bounds[2], node->bounds[4] },
		.max = { node->bounds[1], node->bounds[3], node->bounds[5] },
	};
}

static inline float compute_half_node_area(const struct bvh_node *node) {
	struct boundingBox bbox = load_bbox_from_node(node);
	return bboxHalfArea(&bbox);
}

static inline struct bvh_index make_leaf_index(size_t begin, size_t prim_count) {
	assert(prim_count <= MAX_LEAF_SIZE);
	return (struct bvh_index) {
		.first_child_or_prim = begin,
		.prim_count = prim_count
	};
}

static inline struct bvh_index make_inner_index(size_t first_child) {
	return (struct bvh_index) {
		.first_child_or_prim = first_child,
		.prim_count = 0
	};
}

static inline struct split make_invalid_split() {
	return (struct split) {
		.axis = -1,
		.pos = 0,
		.cost = FLT_MAX
	};
}

static inline bool is_valid_split(const struct split *split) {
	return split->axis <= 2;
}

static inline struct bin make_empty_bin() {
	return (struct bin) {
		.bbox = emptyBBox,
		.count = 0
	};
}

static inline void extend_bin(struct bin *bin, const struct boundingBox *bbox) {
	extendBBox(&bin->bbox, bbox);
	bin->count++;
}

static inline void merge_bin(struct bin *bin, const struct bin *other) {
	extendBBox(&bin->bbox, &other->bbox);
	bin->count += other->count;
}

static inline float compute_partial_cost(const struct bin *bin) {
	return bin->count * bboxHalfArea(&bin->bbox);
}

static inline bool is_on_left_partition(const struct vector *center, unsigned axis, float split_pos) {
	return vec_component(center, axis) < split_pos;
}

static size_t partition_prim_indices(
	unsigned axis,
	float split_pos,
	size_t* prim_indices,
	const struct vector *centers,
	size_t begin, size_t end)
{
	// Perform the split by partitioning primitive indices in-place
	size_t i = begin, j = end;
	while (i < j) {
		while (i < j) {
			if (is_on_left_partition(&centers[prim_indices[i]], axis, split_pos))
				break;
			i++;
		}

		while (i < j) {
			if (!is_on_left_partition(&centers[prim_indices[j - 1]], axis, split_pos))
				break;
			j--;
		}

		if (i >= j)
			break;

		size_t tmp = prim_indices[j - 1];
		prim_indices[j - 1] = prim_indices[i];
		prim_indices[i] = tmp;

		j--;
		i++;
	}
	return i;
}

static inline void setup_bins(struct bin bins[3][BIN_COUNT]) {
	for (unsigned axis = 0; axis < 3; ++axis) {
		for (size_t i = 0; i < BIN_COUNT; ++i)
			bins[axis][i] = make_empty_bin();
	}
}

static inline void fill_bins(
	struct bin bins[3][BIN_COUNT],
	const size_t *prim_indices,
	const struct vector *centers,
	const float *bin_scale,
	const float *bin_offset,
	const struct boundingBox *bboxes,
	size_t begin, size_t end)
{
	for (size_t i = begin; i < end; ++i) {
		size_t prim_index = prim_indices[i];
		for (unsigned axis = 0; axis < 3; ++axis) {
			float bin_pos = robust_max(fast_mul_add(
				vec_component(&centers[prim_index], axis), bin_scale[axis], bin_offset[axis]), 0.f);
			size_t bin_index = bin_pos;
			bin_index = bin_index >= BIN_COUNT ? BIN_COUNT - 1 : bin_index;
			extend_bin(&bins[axis][bin_index], &bboxes[prim_index]);
		}
	}
}

static inline struct split find_best_split(struct bin bins[3][BIN_COUNT]) {
	struct split best_split = make_invalid_split();
	float partial_cost[BIN_COUNT];
	for (unsigned axis = 0; axis < 3; ++axis) {
		// Sweep from the right to the left to compute the partial SAH cost.
		// Recall that the SAH is the sum of two parts: SA(left) * N(left) + SA(right) * N(right).
		// This loop computes SA(right) * N(right) alone.
		struct bin accum = make_empty_bin();
		for (size_t i = BIN_COUNT - 1; i > 0; --i) {
			struct bin *bin = &bins[axis][i];
			merge_bin(&accum, bin);
			partial_cost[i] = compute_partial_cost(&accum);
		}

		// Sweep from the left to the right to compute the full cost and find the minimum.
		accum = make_empty_bin();
		for (size_t i = 0; i < BIN_COUNT - 1; i++) {
			merge_bin(&accum, &bins[axis][i]);
			const float cost = compute_partial_cost(&accum) + partial_cost[i + 1];
			if (cost < best_split.cost) {
				best_split.axis = axis;
				best_split.pos = i + 1;
				best_split.cost = cost;
			}
		}
	}
	return best_split;
}

static inline struct boundingBox compute_bbox(
	const struct boundingBox *bboxes,
	const size_t *prim_indices,
	size_t begin, size_t end)
{
	struct boundingBox bbox = emptyBBox;
	for (size_t i = begin; i < end; ++i)
		extendBBox(&bbox, &bboxes[prim_indices[i]]);
	return bbox;
}

static inline bool compare_prim_indices(const struct vector *centers, unsigned axis, size_t i, size_t j) {
	return vec_component(&centers[i], axis) < vec_component(&centers[j], axis);
}

// This is a variant of shell sort, preferred over qsort because it needs access to the centers.
static inline void sort_prim_indices(size_t *prim_indices, const struct vector *centers, unsigned axis, size_t count) {
	static const size_t gaps[] = { 701, 301, 132, 57, 23, 10, 4, 1 };
	for (size_t k = 0; k < sizeof(gaps) / sizeof(gaps[0]); ++k) {
		size_t gap = gaps[k];
		for (size_t i = gap; i < count; ++i) {
			size_t elem = prim_indices[i];
			size_t j = i;
			for (; j >= gap && compare_prim_indices(centers, axis, elem, prim_indices[j - gap]); j -= gap)
				prim_indices[j] = prim_indices[j - gap];
			prim_indices[j] = elem;
		}
	}
}

static inline size_t fallback_split(
	size_t *prim_indices,
	const struct vector *node_extents,
	const struct vector *centers,
	size_t begin, size_t end)
{
	const unsigned axis = find_largest_axis(node_extents);
	sort_prim_indices(prim_indices + begin, centers, axis, end - begin);
	return (begin + end) / 2;
}

static void build_bvh_recursive(
	size_t node_id,
	struct bvh *bvh,
	const struct boundingBox *bboxes,
	const struct vector *centers,
	size_t begin, size_t end,
	size_t depth)
{
	const size_t prim_count = end - begin;
	struct bvh_node *node = &bvh->nodes[node_id];

	if (depth >= MAX_BVH_DEPTH || prim_count < 2)
		goto make_leaf;

	struct bin bins[3][BIN_COUNT];
	const struct boundingBox node_bbox = load_bbox_from_node(node);
	const struct vector node_extents = vec_sub(node_bbox.max, node_bbox.min);

	const float bin_scale[] = {
		BIN_COUNT / node_extents.x,
		BIN_COUNT / node_extents.y,
		BIN_COUNT / node_extents.z
	};
	const float bin_offset[] = {
		-node_bbox.min.x * bin_scale[0],
		-node_bbox.min.y * bin_scale[1],
		-node_bbox.min.z * bin_scale[2]
	};
	setup_bins(bins);
	fill_bins(bins, bvh->prim_indices, centers, bin_scale, bin_offset, bboxes, begin, end);
	const struct split split = find_best_split(bins);

	const float leaf_cost = compute_half_node_area(node) * (prim_count - TRAVERSAL_COST);
	size_t right_begin;
	if (!is_valid_split(&split) || split.cost > leaf_cost) {
		if (prim_count <= MAX_LEAF_SIZE)
			goto make_leaf;
		right_begin = fallback_split(bvh->prim_indices, &node_extents, centers, begin, end);
	} else {
		const float split_pos = vec_component(&node_bbox.min, split.axis) +
			(float)split.pos / bin_scale[split.axis]; // 1 / bin_scale is the bin size
		right_begin = partition_prim_indices(split.axis, split_pos, bvh->prim_indices, centers, begin, end);
		if (right_begin == begin || right_begin == end)
			right_begin = fallback_split(bvh->prim_indices, &node_extents, centers, begin, end);
	}

	const size_t first_child = bvh->node_count;
	bvh->node_count += 2;

	// Compute the bounding box of the children
	const struct boundingBox left_bbox  = compute_bbox(bboxes, bvh->prim_indices, begin, right_begin);
	const struct boundingBox right_bbox = compute_bbox(bboxes, bvh->prim_indices, right_begin, end);
	store_bbox_to_node(&bvh->nodes[first_child + 0], &left_bbox);
	store_bbox_to_node(&bvh->nodes[first_child + 1], &right_bbox);
	node->index = make_inner_index(first_child);

	build_bvh_recursive(first_child + 0, bvh, bboxes, centers, begin, right_begin, depth + 1);
	build_bvh_recursive(first_child + 1, bvh, bboxes, centers, right_begin, end, depth + 1);
	return;

make_leaf:
	node->index = make_leaf_index(begin, prim_count);
}

// Builds a BVH using the provided callback to obtain bounding boxes and centers for each primitive
static inline struct bvh *build_bvh_generic(
	const void *user_data,
	void (*get_bbox_and_center)(const void *, unsigned, struct boundingBox *, struct vector *),
	size_t count)
{
	if (count < 1) {
		logr(debug, "bvh count < 1\n");
		return NULL;
	}

	struct vector *centers = malloc(sizeof(struct vector) * count);
	struct boundingBox *bboxes = malloc(sizeof(struct boundingBox) * count);
	size_t *prim_indices = malloc(sizeof(size_t) * count);

	for (unsigned i = 0; i < count; ++i) {
		get_bbox_and_center(user_data, i, &bboxes[i], &centers[i]);
		prim_indices[i] = i;
	}

	// Binary tree property: total number of nodes (inner + leaves) = 2 * number of leaves - 1
	const size_t max_nodes = 2 * count - 1;
	const struct boundingBox root_bbox = compute_bbox(bboxes, prim_indices, 0, count);

	struct bvh *bvh = malloc(sizeof(struct bvh));
	bvh->node_count = 1; // For the root
	bvh->nodes = malloc(sizeof(struct bvh_node) * max_nodes);
	bvh->prim_indices = prim_indices;
	store_bbox_to_node(&bvh->nodes[0], &root_bbox);

	build_bvh_recursive(0, bvh, bboxes, centers, 0, count, 0);

	// Shrink array of nodes (since some leaves may contain more than 1 primitive)
	bvh->nodes = realloc(bvh->nodes, sizeof(struct bvh_node) * bvh->node_count);
	free(centers);
	free(bboxes);
	return bvh;
}

// TODO: Add [tmin, tmax] to the lightRay structure for more efficient culling.
#if ROBUST_TRAVERSAL
static inline bool intersect_node(
	const struct bvh_node *node,
	const struct vector *inv_dir,
	const struct vector *start,
	const int *octant,
	float max_dist,
	float *t_entry)
{
	float tmin_x = (node->bounds[0 +     octant[0]] - start->x) * inv_dir->x;
	float tmax_x = (node->bounds[0 + 1 - octant[0]] - start->x) * inv_dir->x;
	float tmin_y = (node->bounds[2 +     octant[1]] - start->y) * inv_dir->y;
	float tmax_y = (node->bounds[2 + 1 - octant[1]] - start->y) * inv_dir->y;
	float tmin_z = (node->bounds[4 +     octant[2]] - start->z) * inv_dir->z;
	float tmax_z = (node->bounds[4 + 1 - octant[2]] - start->z) * inv_dir->z;
	float tmin = robust_max(tmin_x, robust_max(tmin_y, robust_max(tmin_z, 0.f)));
	float tmax = robust_min(tmax_x, robust_min(tmax_y, robust_min(tmax_z, max_dist)));
	tmax *= 1.00000024f; // See T. Ize's "Robust BVH Ray Traversal" article.
	*t_entry = tmin;
	return tmin <= tmax;
}
#else
static inline bool intersect_node(
	const struct bvh_node *node,
	const struct vector *inv_dir,
	const struct vector *scaled_start,
	const int *octant,
	float max_dist,
	float *t_entry)
{
	float tmin_x = fast_mul_add(node->bounds[0 +     octant[0]], inv_dir->x, scaled_start->x);
	float tmax_x = fast_mul_add(node->bounds[0 + 1 - octant[0]], inv_dir->x, scaled_start->x);
	float tmin_y = fast_mul_add(node->bounds[2 +     octant[1]], inv_dir->y, scaled_start->y);
	float tmax_y = fast_mul_add(node->bounds[2 + 1 - octant[1]], inv_dir->y, scaled_start->y);
	float tmin_z = fast_mul_add(node->bounds[4 +     octant[2]], inv_dir->z, scaled_start->z);
	float tmax_z = fast_mul_add(node->bounds[4 + 1 - octant[2]], inv_dir->z, scaled_start->z);
	float tmin = robust_max(tmin_x, robust_max(tmin_y, robust_max(tmin_z, 0.f)));
	float tmax = robust_min(tmax_x, robust_min(tmax_y, robust_min(tmax_z, max_dist)));
	*t_entry = tmin;
	return tmin <= tmax;
}
#endif

static inline bool traverse_bvh_generic(
	const void *user_data,
	const struct bvh *bvh,
	intersect_leaf_fn_t intersect_leaf,
	const struct lightRay *ray,
	struct hitRecord *isect)
{
	if (bvh->node_count < 1) {
		isect->instIndex = -1;
		return false;
	}

	struct bvh_index stack[MAX_BVH_DEPTH + 1];
	struct bvh_index top = bvh->nodes[0].index;
	size_t stack_size = 0;

	// Precompute ray octant and inverse direction
	int octant[] = {
		signbit(ray->direction.x) ? 1 : 0,
		signbit(ray->direction.y) ? 1 : 0,
		signbit(ray->direction.z) ? 1 : 0
	};

#if ROBUST_TRAVERSAL
	struct vector inv_dir = {
		1.f / ray->direction.x,
		1.f / ray->direction.y,
		1.f / ray->direction.z
	};
	struct vector start = ray->start;
#else
	struct vector inv_dir = {
		safe_inverse(ray->direction.x),
		safe_inverse(ray->direction.y),
		safe_inverse(ray->direction.z)
	};
	struct vector start = vec_negate(vec_mul(ray->start, inv_dir));
#endif
	float max_dist = isect->distance;
	bool was_hit = false;

	while (true) {
		while (likely(top.prim_count == 0)) {
			index_t first_child = top.first_child_or_prim;
			const struct bvh_node *left_node  = &bvh->nodes[first_child + 0];
			const struct bvh_node *right_node = &bvh->nodes[first_child + 1];

			float t_left, t_right;
			bool hit_left  = intersect_node(left_node, &inv_dir, &start, octant, max_dist, &t_left);
			bool hit_right = intersect_node(right_node, &inv_dir, &start, octant, max_dist, &t_right);

			if (hit_left) {
				struct bvh_index first = left_node->index;
				if (hit_right) {
					// Choose the child that is the closest, and push the other on the stack.
					struct bvh_index second = right_node->index;
					if (t_left > t_right) {
						struct bvh_index tmp = first;
						first = second;
						second = tmp;
					}
					stack[stack_size++] = second;
				}
				top = first;
			} else if (likely(hit_right))
				top = right_node->index;
			else
				goto pop;
		}

		if (intersect_leaf(
			user_data, bvh, ray,
			top.first_child_or_prim,
			top.first_child_or_prim + top.prim_count,
			isect))
		{
			max_dist = isect->distance;
			was_hit = true;
		}

pop:
		if (unlikely(stack_size == 0))
			break;
		top = stack[--stack_size];
	}
	return was_hit;
}

static void get_poly_bbox_and_center(const void *userData, unsigned i, struct boundingBox *bbox, struct vector *center) {
	const struct mesh *mesh = userData;
	struct vector v0 = mesh->vbuf.vertices.items[mesh->polygons.items[i].vertexIndex[0]];
	struct vector v1 = mesh->vbuf.vertices.items[mesh->polygons.items[i].vertexIndex[1]];
	struct vector v2 = mesh->vbuf.vertices.items[mesh->polygons.items[i].vertexIndex[2]];
	*center = vec_get_midpoint(v0, v1, v2);
	bbox->min = vec_min(v0, vec_min(v1, v2));
	bbox->max = vec_max(v0, vec_max(v1, v2));
}

static void get_instance_bbox_and_center(const void *userData, unsigned i, struct boundingBox *bbox, struct vector *center) {
	const struct instance *instances = userData;
	instances[i].getBBoxAndCenterFn(&instances[i], bbox, center);
}

static inline bool intersect_bottom_level_leaf(
	const void *user_data,
	const struct bvh *bvh,
	const struct lightRay *ray,
	size_t begin, size_t end,
	struct hitRecord *isect)
{
	const struct mesh *mesh = user_data;
	bool found = false;
	for (size_t i = begin; i < end; ++i) {
		struct poly *p = &mesh->polygons.items[bvh->prim_indices[i]];
		if (rayIntersectsWithPolygon(mesh, ray, p, isect)) {
			isect->polygon = p;
			found = true;
		}
	}
	return found;
}

static inline bool intersect_top_level_leaf(
	const void *user_data,
	const struct bvh *bvh,
	const struct lightRay *ray,
	size_t begin, size_t end,
	struct hitRecord *isect)
{
	const struct top_level_data *top_level_data = user_data;
	const struct instance *instances = top_level_data->instances;
	struct sampler *sampler = top_level_data->sampler;
	bool found = false;
	for (size_t i = begin; i < end; ++i) {
		size_t prim_index = bvh->prim_indices[i];
		if (instances[prim_index].intersectFn(&instances[prim_index], ray, isect, sampler)) {
			isect->instIndex = prim_index;
			found = true;
		}
	}
	return found;
}

struct boundingBox get_root_bbox(const struct bvh *bvh) {
	return load_bbox_from_node(&bvh->nodes[0]);
}

struct boundingBox get_transformed_root_bbox(const struct bvh *bvh, const struct matrix4x4 *mat) {
	// This algorithm recursively traverses the BVH in order to get a better approximation of the
	// bounding box of the transformed BVH. An area threshold, expressed as a fraction of the area
	// of the root node, is used to stop recursion early: For nodes that have a smaller area, the
	// bounding box of the node is used as a proxy for the bounding box of the elements contained
	// in the subtree. Thus, the smaller the threshold, the more precise the algorithm is, at the
	// cost of more iterations.
	const float area_threshold = 0.1f * bboxHalfArea((struct boundingBox[]) { get_root_bbox(bvh) });

	index_t stack[MAX_BVH_DEPTH + 1];
	index_t top = 0;
	size_t stack_size = 0;
	struct boundingBox bbox = emptyBBox;

	while (true) {
		struct bvh_node* node = &bvh->nodes[top];
		struct boundingBox node_bbox = load_bbox_from_node(node);

		// Stop recursing at a fixed depth, when we hit a leaf, or when the surface area of the bounding
		// box is lower than the given threshold.
		if (stack_size >= MAX_BVH_DEPTH + 1 ||
			node->index.prim_count > 0 ||
			bboxHalfArea(&node_bbox) < area_threshold)
		{
			tform_bbox(&node_bbox, *mat);
			extendBBox(&bbox, &node_bbox);
			if (stack_size <= 0)
				break;
			top = stack[--stack_size];
		} else {
			stack[stack_size++] = node->index.first_child_or_prim + 0;
			top = node->index.first_child_or_prim + 1;
		}
	}

	return bbox;
}

struct bvh *build_mesh_bvh(const struct mesh *mesh) {
	return build_bvh_generic(mesh, get_poly_bbox_and_center, mesh->polygons.count);
}

struct bvh *build_top_level_bvh(const struct instance_arr instances) {
	return build_bvh_generic(instances.items, get_instance_bbox_and_center, instances.count);
}

bool traverse_bottom_level_bvh(
	const struct mesh *mesh,
	const struct lightRay *ray,
	struct hitRecord *isect,
	sampler *sampler)
{
	(void)sampler;
	return traverse_bvh_generic(mesh, mesh->bvh, intersect_bottom_level_leaf, ray, isect);
}

bool traverse_top_level_bvh(
	const struct instance *instances,
	const struct bvh *bvh,
	const struct lightRay *ray,
	struct hitRecord *isect,
	sampler *sampler)
{
	return traverse_bvh_generic(
		&(struct top_level_data) { instances, sampler },
		bvh, intersect_top_level_leaf, ray, isect);
}

void destroy_bvh(struct bvh *bvh) {
	if (!bvh) return;
	if (bvh->nodes) free(bvh->nodes);
	if (bvh->prim_indices) free(bvh->prim_indices);
	free(bvh);
}

void bvh_build_task(void *arg) {
	block_signals();
	struct mesh *mesh = (struct mesh *)arg;
	struct timeval timer = { 0 };
	timer_start(&timer);
	mesh->bvh = build_mesh_bvh(mesh);
	if (mesh->bvh) {
		logr(debug, "Built BVH for %s, took %lums\n", mesh->name, timer_get_ms(timer));
	} else {
		logr(debug, "BVH build FAILED for %s\n", mesh->name);
	}
}

// FIXME: Add pthread_cancel() support
void compute_accels(struct mesh_arr meshes) {
	struct cr_thread_pool *pool = thread_pool_create(sys_get_cores());
	logr(info, "Updating %zu BVHs: ", meshes.count);
	struct timeval timer = { 0 };
	timer_start(&timer);
	for (size_t i = 0; i < meshes.count; ++i) {
		if (!meshes.items[i].bvh) thread_pool_enqueue(pool, bvh_build_task, &meshes.items[i]);
	}
	thread_pool_wait(pool);

	printSmartTime(timer_get_ms(timer));
	logr(plain, "\n");
	thread_pool_destroy(pool);
}

