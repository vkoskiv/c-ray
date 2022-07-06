//
//  wavefront.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2022 Valtteri Koskivuori. All rights reserved.
//

#include <string.h>
#include "../../../../includes.h"
#include "../../../../datatypes/mesh.h"
#include "../../../../datatypes/vector.h"
#include "../../../../datatypes/poly.h"
#include "../../../../datatypes/material.h"
#include "../../../logging.h"
#include "../../../string.h"
#include "../../../fileio.h"
#include "../../../textbuffer.h"
#include "mtlloader.h"

#include "wavefront.h"

static int findMaterialIndex(struct material *materialSet, int materialCount, char *mtlName) {
	for (int i = 0; i < materialCount; ++i) {
		if (stringEquals(materialSet[i].name, mtlName)) {
			return i;
		}
	}
	return 0;
}

// Count lines starting with thing
static size_t count(textBuffer *buffer, const char *thing) {
	size_t thingCount = 0;
	char *head = firstLine(buffer);
	while (head) {
		if (stringStartsWith(thing, head)) thingCount++;
		head = nextLine(buffer);
	}
	head = firstLine(buffer);
	return thingCount;
}

static size_t countPolygons(textBuffer *buffer) {
	size_t thingCount = 0;
	char *head = firstLine(buffer);
	lineBuffer *line = newLineBuffer();
	while (head) {
		if (head[0] == 'f') {
			fillLineBuffer(line, head, ' ');
			if (line->amountOf.tokens > 4) {
				thingCount += 2;
			} else {
				thingCount++;
			}
		}
		head = nextLine(buffer);
	}
	destroyLineBuffer(line);
	return thingCount;
}

static struct vector parseVertex(lineBuffer *line) {
	ASSERT(line->amountOf.tokens == 4);
	return (struct vector){atof(nextToken(line)), atof(nextToken(line)), atof(nextToken(line))};
}

static struct coord parseCoord(lineBuffer *line) {
	// Some weird OBJ files just have a 0.0 as the third value for 2d coordinates.
	ASSERT(line->amountOf.tokens == 3 || line->amountOf.tokens == 4);
	return (struct coord){atof(nextToken(line)), atof(nextToken(line))};
}

// Wavefront supports different indexing types like
// f v1 v2 v3 [v4]
// f v1/vt1 v2/vt2 v3/vt3
// f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
// f v1//vn1 v2//vn2 v3//vn3
// Or a quad:
// f v1//vn1 v2//vn2 v3//vn3 v4//vn4
size_t parsePolygons(lineBuffer *line, struct poly *buf) {
	lineBuffer *batch = newLineBuffer();
	size_t polycount = line->amountOf.tokens - 3;
	// For now, c-ray will just translate quads to two polygons while parsing
	// Explode in a ball of fire if we encounter an ngon
	bool is_ngon = polycount > 2;
	if (is_ngon) {
		logr(debug, "!! Found an ngon in wavefront file, skipping !!\n");
		polycount = 2;
	}
	bool skipped = false;
	for (size_t i = 0; i < polycount; ++i) {
		firstToken(line);
		struct poly *p = &buf[i];
		p->vertexCount = MAX_CRAY_VERTEX_COUNT;
		for (int j = 0; j < p->vertexCount; ++j) {
			fillLineBuffer(batch, nextToken(line), '/');
			p->vertexIndex[j] = atoi(firstToken(batch));
			p->textureIndex[j] = atoi(nextToken(batch));
			p->normalIndex[j] = atoi(nextToken(batch));
			if (i == 1 && !skipped) {
				nextToken(line);
				skipped = true;
			}
		}
	}
	destroyLineBuffer(batch);
	return polycount;
}

static int fixIndex(size_t max, int oldIndex) {
	if (oldIndex == 0) // Unused
		return -1;
	
	if (oldIndex < 0) // Relative to end of list
		return (int)max + oldIndex;
	
	return oldIndex - 1;// Normal indexing
}

static void fixIndices(struct poly *p, size_t totalVertices, size_t totalTexCoords, size_t totalNormals) {
	for (int i = 0; i < MAX_CRAY_VERTEX_COUNT; ++i) {
		p->vertexIndex[i] = fixIndex(totalVertices, p->vertexIndex[i]);
		p->textureIndex[i] = fixIndex(totalTexCoords, p->textureIndex[i]);
		p->normalIndex[i] = fixIndex(totalNormals, p->normalIndex[i]);
	}
}

struct mesh *parseWavefront(const char *filePath, size_t *finalMeshCount, struct file_cache *cache) {
	size_t bytes = 0;
	char *rawText = loadFile(filePath, &bytes, cache);
	if (!rawText) return NULL;
	logr(debug, "Loading OBJ at %s\n", filePath);
	textBuffer *file = newTextBuffer(rawText);
	char *assetPath = getFilePath(filePath);
	
	//Start processing line-by-line, state machine style.
	size_t meshCount = 1;
	//meshCount += count(file, "o");
	//meshCount += count(file, "g");
	//size_t currentMesh = 0;
	size_t valid_meshes = 0;
	
	// Allocate local buffers (memcpy these to global buffers if parsing succeeds)
	size_t fileVertices = count(file, "v");
	size_t currentVertex = 0;
	struct vector *vertices = malloc(fileVertices * sizeof(*vertices));
	size_t fileTexCoords = count(file, "vt");
	size_t currentTextureCoord = 0;
	struct coord *texCoords = malloc(fileTexCoords * sizeof(*texCoords));
	size_t fileNormals = count(file, "vn");
	size_t currentNormal = 0;
	struct vector *normals = malloc(fileNormals * sizeof(*normals));
	size_t filePolys = countPolygons(file);
	size_t currentPoly = 0;
	struct poly *polygons = malloc(filePolys * sizeof(*polygons));
	
	struct material *materialSet = NULL;
	int materialCount = 0;
	int currentMaterialIndex = 0;
	
	//FIXME: Handle more than one mesh
	struct mesh *meshes = calloc(1, sizeof(*meshes));
	struct mesh *currentMeshPtr = NULL;
	
	currentMeshPtr = meshes;
	valid_meshes = 1;
	
	int currentVertexCount = 0;
	int currentNormalCount = 0;
	int currentTextureCount = 0;
	
	struct poly polybuf[2];

	char *head = firstLine(file);
	lineBuffer *line = newLineBuffer();
	while (head) {
		fillLineBuffer(line, head, ' ');
		char *first = firstToken(line);
		if (first[0] == '#') {
			head = nextLine(file);
			continue;
		} else if (first[0] == '\0') {
			head = nextLine(file);
			continue;
		} else if (first[0] == 'o' || first[0] == 'g') {
			//FIXME: o and g probably have a distinction for a reason?
			//currentMeshPtr = &meshes[currentMesh++];
			currentMeshPtr->name = stringCopy(peekNextToken(line));
			//valid_meshes++;
		} else if (stringEquals(first, "v")) {
			vertices[currentVertex++] = parseVertex(line);
			currentVertexCount++;
		} else if (stringEquals(first, "vt")) {
			texCoords[currentTextureCoord++] = parseCoord(line);
			currentTextureCount++;
		} else if (stringEquals(first, "vn")) {
			normals[currentNormal++] = parseVertex(line);
			currentNormalCount++;
		} else if (stringEquals(first, "s")) {
			// Smoothing groups. We don't care about these, we always smooth.
		} else if (stringEquals(first, "f")) {
			size_t count = parsePolygons(line, polybuf);
			for (size_t i = 0; i < count; ++i) {
				struct poly p = polybuf[i];
				fixIndices(&p, fileVertices, fileTexCoords, fileNormals);
				p.materialIndex = currentMaterialIndex;
				p.hasNormals = p.normalIndex[0] != -1;
				polygons[currentPoly++] = p;
			}
		} else if (stringEquals(first, "usemtl")) {
			currentMaterialIndex = findMaterialIndex(materialSet, materialCount, peekNextToken(line));
		} else if (stringEquals(first, "mtllib")) {
			char *mtlFilePath = stringConcat(assetPath, peekNextToken(line));
			windowsFixPath(mtlFilePath);
			materialSet = parseMTLFile(mtlFilePath, &materialCount, cache);
			free(mtlFilePath);
		} else {
			char *fileName = getFileName(filePath);
			logr(debug, "Unknown statement \"%s\" in OBJ \"%s\" on line %zu\n",
				 first, fileName, file->current.line);
			free(fileName);
		}
		head = nextLine(file);
	}
	destroyLineBuffer(line);
	
	if (finalMeshCount) *finalMeshCount = valid_meshes;
	destroyTextBuffer(file);
	free(rawText);
	free(assetPath);

	if (materialSet) {
		for (size_t i = 0; i < meshCount; ++i) {
			meshes[i].materials = materialSet;
			meshes[i].materialCount = materialCount;
		}
	} else {
		for (size_t i = 0; i < meshCount; ++i) {
			meshes[i].materials = calloc(1, sizeof(struct material));
			meshes[i].materials[0] = warningMaterial();
			meshes[i].materialCount = 1;
		}
	}
	
	currentMeshPtr->polygons = polygons;
	currentMeshPtr->poly_count = (int)filePolys;
	currentMeshPtr->vertices = vertices;
	currentMeshPtr->vertex_count = currentVertexCount;
	currentMeshPtr->normals = normals;
	currentMeshPtr->normal_count = currentNormalCount;
	currentMeshPtr->texture_coords = texCoords;
	currentMeshPtr->tex_coord_count = currentTextureCount;
	
	return meshes;
}
