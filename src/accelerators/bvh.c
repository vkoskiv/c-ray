//
//  bvh.c
//  C-ray
//
//  Created by Arsène Pérard-Gayot on 07/06/2020.
//  Copyright © 2020-2022 Arsène Pérard-Gayot (@madmann91), Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "bvh.h"

#include "../renderer/pathtrace.h"

#include "../datatypes/bbox.h"
#include "../datatypes/mesh.h"
#include "../renderer/instance.h"

/*
 * This BVH builder is based on "On fast Construction of SAH-based Bounding Volume Hierarchies",
 * by I. Wald. The general idea is to approximate the SAH by subdividing each axis in several
 * bins. Primitives are then placed into those bins according to their centers, and then the
 * split is found by sweeping the bin array to find the partition with the lowest SAH.
 * The algorithm needs only a bounding box and a center per primitive, which allows to build
 * BVHs out of any primitive type, including other BVHs (necessary for instancing).
 */

#define MAX_BVH_DEPTH  64   // This should be enough for most scenes
#define MAX_LEAF_SIZE  16   // Maximum number of primitives per leaf (used to avoid cases where the SAH gets "stuck")
#define TRAVERSAL_COST 1.5f // Ratio (cost of traversing a node / cost of intersecting a primitive)
#define BIN_COUNT      32   // Number of bins to use to approximate the SAH

struct bvhNode {
	float bounds[6]; // Node bounds (min x, max x, min y, max y, ...)
	unsigned firstChildOrPrim; // Index to the first child or primitive (if the node is a leaf)
	unsigned primCount : 30;
	bool isLeaf : 1;
};

struct bvh {
	struct bvhNode *nodes;
	int *primIndices;
	unsigned nodeCount;
};

// Bin used to approximate the SAH.
typedef struct Bin {
	struct boundingBox bbox;
	unsigned count;
	float cost;
} Bin;

static inline void storeBBoxInNode(struct bvhNode *node, const struct boundingBox *bbox) {
	node->bounds[0] = bbox->min.x;
	node->bounds[1] = bbox->max.x;
	node->bounds[2] = bbox->min.y;
	node->bounds[3] = bbox->max.y;
	node->bounds[4] = bbox->min.z;
	node->bounds[5] = bbox->max.z;
}

static inline void loadBBoxFromNode(struct boundingBox *bbox, const struct bvhNode *node) {
	bbox->min.x = node->bounds[0];
	bbox->max.x = node->bounds[1];
	bbox->min.y = node->bounds[2];
	bbox->max.y = node->bounds[3];
	bbox->min.z = node->bounds[4];
	bbox->max.z = node->bounds[5];
}

static inline float nodeArea(const struct bvhNode *node) {
	struct boundingBox bbox;
	loadBBoxFromNode(&bbox, node);
	return bboxHalfArea(&bbox);
}

static inline void makeLeaf(struct bvhNode *node, unsigned begin, unsigned primCount) {
	node->isLeaf = true;
	node->firstChildOrPrim = begin;
	node->primCount = primCount;
}

static inline unsigned computeBinIndex(int axis, const struct vector *center, float min, float max) {
	float centerToBin = BIN_COUNT / (max - min);
	float coord = axis == 0 ? center->x : (axis == 1 ? center->y : center->z);
	float floatIndex = (coord - min) * centerToBin;
	unsigned binIndex = floatIndex < 0 ? 0 : floatIndex;
	return binIndex >= BIN_COUNT ? BIN_COUNT - 1 : binIndex;
}

static inline unsigned partitionPrimitiveIndices(
	const struct bvhNode *node,
	const struct bvh *bvh,
	const struct vector *centers,
	unsigned axis, unsigned bin,
	unsigned begin, unsigned end)
{
	// Perform the split by partitioning primitive indices in-place
	unsigned i = begin, j = end;
	while (i < j) {
		while (i < j) {
			unsigned binIndex = computeBinIndex(axis, &centers[bvh->primIndices[i]], node->bounds[axis * 2], node->bounds[axis * 2 + 1]);
			if (binIndex >= bin)
				break;
			i++;
		}

		while (i < j) {
			unsigned binIndex = computeBinIndex(axis, &centers[bvh->primIndices[j - 1]], node->bounds[axis * 2], node->bounds[axis * 2 + 1]);
			if (binIndex < bin)
				break;
			j--;
		}

		if (i >= j)
			break;

		int tmp = bvh->primIndices[j - 1];
		bvh->primIndices[j - 1] = bvh->primIndices[i];
		bvh->primIndices[i] = tmp;

		j--;
		i++;
	}
	return i;
}

static void buildBvhRecursive(
	unsigned nodeId,
	struct bvh *bvh,
	const struct boundingBox *bboxes,
	const struct vector *centers,
	unsigned begin, unsigned end,
	unsigned depth)
{
	unsigned primCount = end - begin;
	struct bvhNode *node = &bvh->nodes[nodeId];

	if (depth >= MAX_BVH_DEPTH || primCount < 2) {
		makeLeaf(node, begin, primCount);
		return;
	}

	Bin bins[3][BIN_COUNT];
	float minCost[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
	unsigned minBin[3] = { 1, 1, 1 };
	for (int axis = 0; axis < 3; ++axis) {
		// Initialize bins
		for (int i = 0; i < BIN_COUNT; ++i) {
			bins[axis][i].bbox = emptyBBox;
			bins[axis][i].count = 0;
		}

		// Fill bins with primitives
		for (unsigned i = begin; i < end; ++i) {
			int primIndex = bvh->primIndices[i];
			unsigned binIndex = computeBinIndex(axis, &centers[primIndex], node->bounds[axis * 2], node->bounds[axis * 2 + 1]);
			Bin *bin = &bins[axis][binIndex];
			extendBBox(&bin->bbox, &bboxes[primIndex]);
			bin->count++;
		}

		// Sweep from the right to the left to compute the partial SAH cost.
		// Recall that the SAH is the sum of two parts: SA(left) * N(left) + SA(right) * N(right).
		// This loop computes SA(right) * N(right) alone.
		struct boundingBox curBBox = emptyBBox;
		unsigned curCount = 0;
		for (unsigned i = BIN_COUNT; i > 1; --i) {
			Bin *bin = &bins[axis][i - 1];
			curCount += bin->count;
			extendBBox(&curBBox, &bin->bbox);
			bin->cost = curCount * bboxHalfArea(&curBBox);
		}

		// Sweep from the left to the right to compute the full cost and find the minimum.
		curBBox = emptyBBox;
		curCount = 0;
		for (unsigned i = 0; i < BIN_COUNT - 1; i++) {
			Bin *bin = &bins[axis][i];
			curCount += bin->count;
			extendBBox(&curBBox, &bin->bbox);
			float cost = curCount * bboxHalfArea(&curBBox) + bins[axis][i + 1].cost;
			if (cost < minCost[axis]) {
				minBin[axis] = i + 1;
				minCost[axis] = cost;
			}
		}
	}

	// Find the minimum cost for all three axes
	unsigned minAxis = 0;
	if (minCost[1] < minCost[0]) minAxis = 1;
	if (minCost[2] < minCost[minAxis]) minAxis = 2;

	// Determine if splitting is beneficial or not
	float leafCost = nodeArea(node) * (primCount - TRAVERSAL_COST);
	if (minCost[minAxis] > leafCost) {
		if (primCount > MAX_LEAF_SIZE) {
			// Fallback strategy to avoid large leaves: Approximate median split
			for (unsigned i = 0, accumCount = 0, bestApprox = primCount; i < BIN_COUNT - 1; ++i) {
				accumCount += bins[minAxis][i].count;
				unsigned approx = abs((int)primCount/2 - (int)accumCount);
				if (approx < bestApprox) {
					bestApprox = approx;
					minBin[minAxis] = i + 1;
				}
			}
		} else {
			makeLeaf(node, begin, primCount);
			return;
		}
	}

	// Perform the split by partitioning primitive indices in-place
	unsigned beginRight = partitionPrimitiveIndices(node, bvh, centers, minAxis, minBin[minAxis], begin, end);
	if (beginRight > begin) {
		unsigned leftIndex = bvh->nodeCount;
		unsigned rightIndex = leftIndex + 1;
		bvh->nodeCount += 2;

		// Compute the bounding box of the children
		struct boundingBox leftBBox = emptyBBox;
		struct boundingBox rightBBox = emptyBBox;
		for (unsigned i = 0; i < minBin[minAxis]; ++i)
			extendBBox(&leftBBox, &bins[minAxis][i].bbox);
		for (unsigned i = minBin[minAxis]; i < BIN_COUNT; ++i)
			extendBBox(&rightBBox, &bins[minAxis][i].bbox);
		storeBBoxInNode(&bvh->nodes[leftIndex], &leftBBox);
		storeBBoxInNode(&bvh->nodes[rightIndex], &rightBBox);
		node->firstChildOrPrim = leftIndex;
		node->isLeaf = false;

		buildBvhRecursive(leftIndex, bvh, bboxes, centers, begin, beginRight, depth + 1);
		buildBvhRecursive(rightIndex, bvh, bboxes, centers, beginRight, end, depth + 1);
	} else {
		makeLeaf(node, begin, primCount);
	}
}

// Builds a BVH using the provided callback to obtain bounding boxes and centers for each primitive
static inline struct bvh *buildBvhGeneric(
	const void *userData,
	void (*getBBoxAndCenter)(const void *, unsigned, struct boundingBox *, struct vector *),
	unsigned count)
{
	if (count < 1) {
		struct bvh *bvh = malloc(sizeof(struct bvh));
		bvh->nodeCount = 0;
		bvh->nodes = NULL;
		bvh->primIndices = NULL;
		return bvh;
	}
	struct vector *centers = malloc(sizeof(struct vector) * count);
	struct boundingBox *bboxes = malloc(sizeof(struct boundingBox) * count);
	int *primIndices = malloc(sizeof(int) * count);

	struct boundingBox rootBBox = emptyBBox;

	// Precompute bboxes and centers
	for (unsigned i = 0; i < count; ++i) {
		getBBoxAndCenter(userData, i, &bboxes[i], &centers[i]);
		primIndices[i] = i;
		rootBBox.min = vecMin(rootBBox.min, bboxes[i].min);
		rootBBox.max = vecMax(rootBBox.max, bboxes[i].max);
	}

	// Binary tree property: total number of nodes (inner + leaves) = 2 * number of leaves - 1
	unsigned maxNodes = 2 * count - 1;

	struct bvh *bvh = malloc(sizeof(struct bvh));
	bvh->nodeCount = 1;
	bvh->nodes = malloc(sizeof(struct bvhNode) * maxNodes);
	bvh->primIndices = primIndices;
	storeBBoxInNode(&bvh->nodes[0], &rootBBox);

	buildBvhRecursive(0, bvh, bboxes, centers, 0, count, 0);

	// Shrink array of nodes (since some leaves may contain more than 1 primitive)
	bvh->nodes = realloc(bvh->nodes, sizeof(struct bvhNode) * bvh->nodeCount);
	free(centers);
	free(bboxes);
	return bvh;
}

static void getPolyBBoxAndCenter(const void *userData, unsigned i, struct boundingBox *bbox, struct vector *center) {
	const struct mesh *mesh = userData;
	struct vector v0 = mesh->vertices[mesh->polygons[i].vertexIndex[0]];
	struct vector v1 = mesh->vertices[mesh->polygons[i].vertexIndex[1]];
	struct vector v2 = mesh->vertices[mesh->polygons[i].vertexIndex[2]];
	*center = getMidPoint(v0, v1, v2);
	bbox->min = vecMin(v0, vecMin(v1, v2));
	bbox->max = vecMax(v0, vecMax(v1, v2));
}

struct boundingBox getRootBoundingBox(const struct bvh *bvh) {
	struct boundingBox box;
	loadBBoxFromNode(&box, &bvh->nodes[0]);
	return box;
}

struct bvh *build_mesh_bvh(const struct mesh *mesh) {
	return buildBvhGeneric(mesh, getPolyBBoxAndCenter, mesh->poly_count);
}

static void getInstanceBBoxAndCenter(const void *userData, unsigned i, struct boundingBox *bbox, struct vector *center) {
	const struct instance *instances = userData;
	instances[i].getBBoxAndCenterFn(&instances[i], bbox, center);
}

struct bvh *buildTopLevelBvh(const struct instance *instances, unsigned instanceCount) {
	return buildBvhGeneric(instances, getInstanceBBoxAndCenter, instanceCount);
}

static inline float fastMultiplyAdd(float a, float b, float c) {
#ifdef FP_FAST_FMAF
	return fmaf(a, b, c);
#else
	return a * b + c;
#endif
}

static inline bool intersectNode(
	const struct bvhNode *node,
	const struct vector *invDir,
	const struct vector *scaledStart,
	const int *octant,
	float maxDist,
	float *tEntry)
{
	float tMinX = fastMultiplyAdd(node->bounds[0 +     octant[0]], invDir->x, scaledStart->x);
	float tMaxX = fastMultiplyAdd(node->bounds[0 + 1 - octant[0]], invDir->x, scaledStart->x);
	float tMinY = fastMultiplyAdd(node->bounds[2 +     octant[1]], invDir->y, scaledStart->y);
	float tMaxY = fastMultiplyAdd(node->bounds[2 + 1 - octant[1]], invDir->y, scaledStart->y);
	float tMinZ = fastMultiplyAdd(node->bounds[4 +     octant[2]], invDir->z, scaledStart->z);
	float tMaxZ = fastMultiplyAdd(node->bounds[4 + 1 - octant[2]], invDir->z, scaledStart->z);
	// Note the order here is important.
	// Because the comparisons are of the form x < y ? x : y, they
	// are guaranteed not to produce NaNs if the right hand side is not a NaN.
	float tMin = tMinX > tMinY ? tMinX : tMinY;
	float tMax = tMaxX < tMaxY ? tMaxX : tMaxY;
	tMin = tMin > tMinZ ? tMin : tMinZ;
	tMax = tMax < tMaxZ ? tMax : tMaxZ;
	// TODO: Add [tmin, tmax] to the lightRay structure for more efficient culling.
	tMin = tMin > 0 ? tMin : 0;
	tMax = tMax < maxDist ? tMax : maxDist;
	*tEntry = tMin;
	return tMin <= tMax;
}

static inline bool traverseBvhGeneric(
	const void *userData,
	const struct bvh *bvh,
	bool (*intersectLeaf)(const void *, const struct bvh *, const struct bvhNode *, const struct lightRay *, struct hitRecord *, sampler *),
	const struct lightRay *ray,
	struct hitRecord *isect,
	sampler *sampler)
{
	if (bvh->nodeCount < 1) {
		isect->instIndex = -1;
		return false;
	}
	const struct bvhNode *stack[MAX_BVH_DEPTH + 1];
	int stackSize = 0;

	// Precompute ray octant and inverse direction
	int octant[] = {
		signbit(ray->direction.x) ? 1 : 0,
		signbit(ray->direction.y) ? 1 : 0,
		signbit(ray->direction.z) ? 1 : 0
	};
	struct vector invDir = { 1.0f / ray->direction.x, 1.0f / ray->direction.y, 1.0f / ray->direction.z };
	struct vector scaledStart = vecScale(vecMul(ray->start, invDir), -1.0f);
	float maxDist = isect->distance;
	
	if (bvh->nodeCount < 1) return false;

	// Special case when the BVH is just a single leaf
	if (bvh->nodeCount == 1) {
		float tEntry;
		if (intersectNode(bvh->nodes, &invDir, &scaledStart, octant, maxDist, &tEntry))
			return intersectLeaf(userData, bvh, bvh->nodes, ray, isect, sampler);
		return false;
	}

	const struct bvhNode *node = bvh->nodes;
	bool hasHit = false;
	while (true) {
		unsigned firstChild = node->firstChildOrPrim;
		const struct bvhNode *leftNode  = &bvh->nodes[firstChild];
		const struct bvhNode *rightNode = &bvh->nodes[firstChild + 1];

		float tEntryLeft, tEntryRight;
		bool hitLeft = intersectNode(leftNode, &invDir, &scaledStart, octant, maxDist, &tEntryLeft);
		bool hitRight = intersectNode(rightNode, &invDir, &scaledStart, octant, maxDist, &tEntryRight);

		if (hitLeft) {
			if (unlikely(leftNode->isLeaf)) {
				if (intersectLeaf(userData, bvh, leftNode, ray, isect, sampler)) {
					maxDist = isect->distance;
					hasHit = true;
				}
				leftNode = NULL;
			}
		} else
			leftNode = NULL;

		if (hitRight) {
			if (unlikely(rightNode->isLeaf)) {
				if (intersectLeaf(userData, bvh, rightNode, ray, isect, sampler)) {
					maxDist = isect->distance;
					hasHit = true;
				}
				rightNode = NULL;
			}
		} else
			rightNode = NULL;

		if ((rightNode != NULL) & (leftNode != NULL)) {
			// Choose the child that is the closest, and push the other on the stack.
			if (tEntryLeft > tEntryRight) {
				node = leftNode;
				leftNode = rightNode;
				rightNode = node;
			}
			node = leftNode;
			stack[stackSize++] = rightNode;
		} else if ((rightNode != NULL) ^ (leftNode != NULL)) {
			// Only one child needs traversal
			node = rightNode != NULL ? rightNode : leftNode;
		} else {
			if (stackSize == 0)
				break;
			node = stack[--stackSize];
		}
	}
	return hasHit;
}

static inline bool intersectBottomLevelLeaf(
	const void *userData,
	const struct bvh *bvh,
	const struct bvhNode *leaf,
	const struct lightRay *ray,
	struct hitRecord *isect,
	sampler *sampler)
{
	(void)sampler;
	const struct mesh *mesh = userData;
	bool found = false;
	for (int i = 0; i < leaf->primCount; ++i) {
		struct poly *p = &mesh->polygons[bvh->primIndices[leaf->firstChildOrPrim + i]];
		if (rayIntersectsWithPolygon(mesh, ray, p, isect)) {
			isect->polygon = p;
			found = true;
		}
	}
	return found;
}

bool traverseBottomLevelBvh(const struct mesh *mesh, const struct lightRay *ray, struct hitRecord *isect, sampler *sampler) {
	return traverseBvhGeneric(mesh, mesh->bvh, intersectBottomLevelLeaf, ray, isect, sampler);
}

static inline bool intersectTopLevelLeaf(
	const void *userData,
	const struct bvh *bvh,
	const struct bvhNode *leaf,
	const struct lightRay *ray,
	struct hitRecord *isect,
	sampler *sampler)
{
	const struct instance *instances = userData;
	bool found = false;
	for (int i = 0; i < leaf->primCount; ++i) {
		int currIndex = bvh->primIndices[leaf->firstChildOrPrim + i];
		if (instances[currIndex].intersectFn(&instances[currIndex], ray, isect, sampler)) {
			isect->instIndex = currIndex;
			found = true;
		}
	}
	return found;
}

bool traverseTopLevelBvh(
	const struct instance *instances,
	const struct bvh *bvh,
	const struct lightRay *ray,
	struct hitRecord *isect,
	sampler *sampler)
{
	return traverseBvhGeneric((void *)instances, bvh, intersectTopLevelLeaf, ray, isect, sampler);
}

void destroyBvh(struct bvh *bvh) {
	if (bvh) {
		if (bvh->nodes) free(bvh->nodes);
		if (bvh->primIndices) free(bvh->primIndices);
		free(bvh);
	}
}
