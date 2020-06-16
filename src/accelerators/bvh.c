//
//  bvh.c
//  C-ray
//
//  Created by Arsène Pérard-Gayot on 07/06/2020.
//  Copyright © 2020 Arsène Pérard-Gayot (@madmann91). All rights reserved.
//

#include "../includes.h"
#include "bvh.h"

#include "../renderer/pathtrace.h"

#include "../datatypes/vertexbuffer.h"
#include "../datatypes/poly.h"
#include "../datatypes/vector.h"
#include "../datatypes/bbox.h"
#include "../datatypes/lightRay.h"
#include "../datatypes/mesh.h"

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
	struct bvhNode* nodes;
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

static inline void makeLeaf(struct bvhNode* node, unsigned begin, unsigned primCount) {
	node->isLeaf = true;
	node->firstChildOrPrim = begin;
	node->primCount = primCount;
}

static inline unsigned computeBinIndex(int axis, const vector *center, float min, float max) {
	float centerToBin = BIN_COUNT / (max - min);
	float coord = axis == 0 ? center->x : (axis == 1 ? center->y : center->z);
	float floatIndex = (coord - min) * centerToBin;
	unsigned binIndex = floatIndex < 0 ? 0 : floatIndex;
	return binIndex >= BIN_COUNT ? BIN_COUNT - 1 : binIndex;
}

static inline unsigned partitionPrimitiveIndices(
	const struct bvhNode *node,
	const struct bvh *bvh,
	const vector *centers,
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
	const vector *centers,
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
	void* userData,
	void (*getBBoxAndCenter)(void *, unsigned, struct boundingBox *, vector *),
	unsigned count)
{
	vector *centers = malloc(sizeof(vector) * count);
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

static void getPolyBBoxAndCenter(void* userData, unsigned i, struct boundingBox *bbox, vector *center) {
	int* polys = userData;
	vector v0 = vertexArray[polygonArray[polys[i]].vertexIndex[0]];
	vector v1 = vertexArray[polygonArray[polys[i]].vertexIndex[1]];
	vector v2 = vertexArray[polygonArray[polys[i]].vertexIndex[2]];
	*center = getMidPoint(v0, v1, v2);
	bbox->min = vecMin(v0, vecMin(v1, v2));
	bbox->max = vecMax(v0, vecMax(v1, v2));
}

struct bvh *buildBottomLevelBvh(int *polys, unsigned count) {
	struct bvh *bvh = buildBvhGeneric(polys, getPolyBBoxAndCenter, count);
	for (unsigned i = 0; i < count; ++i)
		bvh->primIndices[i] = polys[bvh->primIndices[i]];
	return bvh;
}

static void getMeshBBoxAndCenter(void* userData, unsigned i, struct boundingBox *bbox, vector *center) {
	struct mesh *meshes = userData;
	loadBBoxFromNode(bbox, &meshes[i].bvh->nodes[0]);
	*center = bboxCenter(bbox);
}

struct bvh *buildTopLevelBvh(struct mesh *meshes, unsigned meshCount) {
	return buildBvhGeneric(meshes, getMeshBBoxAndCenter, meshCount);
}

static inline float fastMultiplyAdd(float a, float b, float c) {
#ifdef FP_FAST_FMAF
	return fmaf(a, b, c);
#else
	return a * b + c;
#endif
}

static inline bool rayIntersectsWithBvhNode(
	const struct bvhNode *node,
	const vector *invDir,
	const vector *scaledStart,
	const int* octant,
	float maxDist,
	float* tEntry)
{
	float tMinX = fastMultiplyAdd(node->bounds[0 * 2 +     octant[0]], invDir->x, scaledStart->x);
	float tMaxX = fastMultiplyAdd(node->bounds[0 * 2 + 1 - octant[0]], invDir->x, scaledStart->x);
	float tMinY = fastMultiplyAdd(node->bounds[1 * 2 +     octant[1]], invDir->y, scaledStart->y);
	float tMaxY = fastMultiplyAdd(node->bounds[1 * 2 + 1 - octant[1]], invDir->y, scaledStart->y);
	float tMinZ = fastMultiplyAdd(node->bounds[2 * 2 +     octant[2]], invDir->z, scaledStart->z);
	float tMaxZ = fastMultiplyAdd(node->bounds[2 * 2 + 1 - octant[2]], invDir->z, scaledStart->z);
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

bool rayIntersectsWithGenericBvh(const struct bvh *bvh,
								 bool (*traverseLeaf)(const struct bvh*, const struct bvhNode*, const struct lightRay*, struct hitRecord*),
								 const struct lightRay *ray,
								 struct hitRecord *isect) {
	const struct bvhNode *stack[MAX_BVH_DEPTH + 1];
	int stackSize = 0;

	// Precompute ray octant and inverse direction
	int octant[] = {
		ray->direction.x < 0 ? 1 : 0,
		ray->direction.y < 0 ? 1 : 0,
		ray->direction.z < 0 ? 1 : 0
	};
	vector invDir = { 1.0f / ray->direction.x, 1.0f / ray->direction.y, 1.0f / ray->direction.z };
	vector scaledStart = vecScale(vecMul(ray->start, invDir), -1.0f);
	float maxDist = isect->didIntersect ? isect->distance : FLT_MAX;

	// Special case when the BVH is just a single leaf
	if (bvh->nodeCount == 1) {
		float tEntry;
		if (rayIntersectsWithBvhNode(bvh->nodes, &invDir, &scaledStart, octant, maxDist, &tEntry))
			return traverseLeaf(bvh, bvh->nodes, ray, isect);
		return false;
	}

	const struct bvhNode *node = bvh->nodes;
	bool hasHit = false;
	while (true) {
		unsigned firstChild = node->firstChildOrPrim;
		const struct bvhNode *leftNode  = &bvh->nodes[firstChild];
		const struct bvhNode *rightNode = &bvh->nodes[firstChild + 1];

		float tEntryLeft, tEntryRight;
		bool hitLeft = rayIntersectsWithBvhNode(leftNode, &invDir, &scaledStart, octant, maxDist, &tEntryLeft);
		bool hitRight = rayIntersectsWithBvhNode(rightNode, &invDir, &scaledStart, octant, maxDist, &tEntryRight);

		if (hitLeft) {
			if (leftNode->isLeaf) {
				if (traverseLeaf(bvh, leftNode, ray, isect)) {
					maxDist = isect->distance;
					hasHit = true;
				}
				leftNode = NULL;
			}
		} else
			leftNode = NULL;

		if (hitRight) {
			if (rightNode->isLeaf) {
				if (traverseLeaf(bvh, rightNode, ray, isect)) {
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

static inline bool rayIntersectsWithBvhLeaf(const struct bvh *bvh, const struct bvhNode *leaf, const struct lightRay *ray, struct hitRecord *isect) {
	bool found = false;
	for (int i = 0; i < leaf->primCount; ++i) {
		struct poly p = polygonArray[bvh->primIndices[leaf->firstChildOrPrim + i]];
		if (rayIntersectsWithPolygon(ray, &p, &isect->distance, &isect->surfaceNormal, &isect->uv)) {
			isect->didIntersect = true;
			isect->type = hitTypePolygon;
			isect->polyIndex = p.polyIndex;
			found = true;
		}
	}
	return found;
}

static inline bool rayIntersectsWithBvh(const struct bvh *bvh, const struct bvhNode *leaf, const struct lightRay *ray, struct hitRecord *isect) {
	return rayIntersectsWithGenericBvh(bvh, rayIntersectsWithBvhLeaf, ray, isect);
}

bool rayIntersectsWithTopLevelBvh(const struct bvh *bvh, const struct lightRay *ray, struct hitRecord *isect) {
	return rayIntersectsWithGenericBvh(bvh, rayIntersectsWithBvh, ray, isect);
}

bool rayIntersectsWithBottomLevelBvh(const struct bvh *bvh, const struct lightRay *ray, struct hitRecord *isect) {
	return rayIntersectsWithGenericBvh(bvh, rayIntersectsWithBvhLeaf, ray, isect);
}

void destroyBvh(struct bvh *bvh) {
	if (bvh) {
		free(bvh->nodes);
		free(bvh->primIndices);
		free(bvh);
	}
}
