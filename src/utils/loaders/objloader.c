//
//  objloader.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "objloader.h"

#include "../../datatypes/mesh.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/poly.h"
#include "../../datatypes/material.h"
#include "../../utils/logging.h"
#include "../../utils/string.h"
#include "../../datatypes/vertexbuffer.h"
#include "mtlloader.h"
#include "../filehandler.h"
#include "../../utils/assert.h"
#include "../../utils/textbuffer.h"

//TODO: REMOVE
#include "../../utils/platform/terminal.h"

#define ws " \t\n\r"

static struct vector parseVertex() {
	return (struct vector){atof(strtok(NULL, ws)), atof(strtok(NULL, ws)), atof(strtok(NULL, ws))};
}

static struct coord parseCoord() {
	return (struct coord){atof(strtok(NULL, ws)), atof(strtok(NULL, ws))};
}

static int parseIndices(int *vertexIndex, int *normalIndex, int *textureIndex) {
	
	char *tempstr;
	char *token;
	int vertexCount = 0;
	
	while ((token = strtok(NULL, ws)) != NULL) {
		
		//texture and normal indices are optional
		if (textureIndex != NULL) {
			textureIndex[vertexCount] = 0;
		}
		if (normalIndex != NULL) {
			normalIndex[vertexCount] = 0;
		}
		
		vertexIndex[vertexCount] = atoi(token);
		
		if (stringContains(token, "//")) {
			//Only normal and vertex
			tempstr = strchr(token, '/');
			tempstr++;
			normalIndex[vertexCount] = atoi(++tempstr);
		} else if (stringContains(token, "/")) {
			tempstr = strchr(token, '/');
			textureIndex[vertexCount] = atoi(++tempstr);
			if (stringContains(tempstr, "/")) {
				tempstr = strchr(tempstr, '/');
				normalIndex[vertexCount] = atoi(++tempstr);
			}
		}
		
		vertexCount++;
	}
	
	return vertexCount;
}

static int convert(int amount, int index) {
	if (index == 0)
		return -1;
	if (index < 0)
		return amount + index;
	
	return index - 1;
}

static void convertIndices(int amount, int *indices) {
	for (int i = 0; i < 3; ++i) {
		indices[i] = convert(amount, indices[i]);
	}
}

static struct poly parsePoly(struct mesh mesh) {
	struct poly p;
	p.vertexCount = parseIndices(p.vertexIndex, p.normalIndex, p.textureIndex);
	
	/*convertIndices(vertexCount, p.vertexIndex);
	convertIndices(normalCount, p.normalIndex);
	convertIndices(textureCount, p.textureIndex);*/
	
	for (int i = 0; i < p.vertexCount; ++i) {
		if (p.normalIndex[i] == 0) {
			p.hasNormals = false;
		}
		
		p.vertexIndex[i] = mesh.firstVectorIndex + (p.vertexIndex[i] - 1);
		p.normalIndex[i] = mesh.firstNormalIndex + (p.normalIndex[i] - 1);
		p.textureIndex[i] = mesh.firstTextureIndex + (p.textureIndex[i] - 1);
	}
	
	return p;
}

static int findMaterialIndex(struct mesh *mesh, char *mtlName) {
	for (int i = 0; i < mesh->materialCount; ++i) {
		if (stringEquals(mesh->materials[i].name, mtlName)) {
			return i;
		}
	}
	return 0;
}

static struct mesh parseOBJMesh(textBuffer *segment) {
	(void)segment;
	return (struct mesh){0};
}

static size_t countMeshes(textBuffer *buffer) {
	size_t meshCount = 0;
	char *head = firstLine(buffer);
	lineBuffer line = {0};
	while (head) {
		fillLineBuffer(&line, head, " ");
		char *first = firstToken(&line);
		if (first[0] == 'o') meshCount++;
		head = nextLine(buffer);
	}
	logr(debug, "File contains %zu meshes\n", meshCount);
	head = firstLine(buffer);
	return meshCount;
}

struct mesh *parseOBJFile(char *filePath, size_t *meshCountOut) {
	logr(debug, "Loading OBJ file\n");
	char *rawText = loadFile(filePath, NULL);
	textBuffer *file = newTextBuffer(rawText);
	
	//Figure out how many meshes this file contains
	size_t meshCount = countMeshes(file);

	//Get the offsets
	char *head = firstLine(file);
	lineBuffer line = {0};
	size_t *meshOffsets = malloc(meshCount * sizeof(*meshOffsets));
	int i = 0;
	while (head) {
		fillLineBuffer(&line, head, " ");
		char *first = firstToken(&line);
		if (first[0] == 'o') meshOffsets[i++] = file->current.line;
		head = nextLine(file);
	}
	logr(debug, "They start on lines:\n");
	
	for (size_t l = 0; l < meshCount; ++l) {
		logr(debug, "%zu\n", meshOffsets[l]);
	}
	head = firstLine(file);
	
	struct mesh *meshes = calloc(meshCount, sizeof(*meshes));
	
	for (size_t m = 0; m < meshCount; ++m) {
		textBuffer *segment = newTextView(file, meshOffsets[m], meshOffsets[m + 1] - meshOffsets[m]);
		meshes[m] = parseOBJMesh(segment);
		freeTextBuffer(segment);
	}
	
	freeLineBuffer(&line);
	freeTextBuffer(file);
	restoreTerminal();
	if (meshCountOut) *meshCountOut = meshCount;
	exit(0);
	return NULL;
}

//Parse a Wavefront OBJ file and return a generic mesh.
//Note: This will also alter the global vertex arrays
//We will add vertices to them as the OBJ is loaded.
struct mesh *parseOBJFilea(char *filePath) {
	struct mesh *newMesh = calloc(1, sizeof(*newMesh));
	
	int linenum = 0;
	char *token;
	char currLine[500];
	FILE *fileStream;
	
	fileStream = fopen(filePath, "r");
	if (fileStream == 0) {
		logr(warning, "OBJ not found at %s\n", filePath);
		free(newMesh);
		return NULL;
	}
	
	//First, find vertex counts
	int vCount = 0;
	int nCount = 0;
	int tCount = 0;
	int pCount = 0;
	
	while (fgets(currLine, 500, fileStream)) {
		token = strtok(currLine, ws);
		//skip comments
		if(token == NULL || stringEquals(token, "#")) {
			continue;
		} else if (stringEquals(token, "v")) {
			vCount++;
		} else if (stringEquals(token, "vn")) {
			nCount++;
		} else if (stringEquals(token, "vt")) {
			tCount++;
		} else if (stringEquals(token, "f")) {
			pCount++;
		}
	}
	fseek(fileStream, 0, SEEK_SET);
	
	newMesh->firstVectorIndex = vertexCount;
	newMesh->firstNormalIndex = normalCount;
	newMesh->firstTextureIndex = textureCount;
	
	newMesh->vertexCount = vCount;
	newMesh->normalCount = nCount;
	newMesh->textureCount = tCount;
	newMesh->polyCount = pCount;
	
	vertexCount += vCount;
	vertexArray = realloc(vertexArray, vertexCount * sizeof(struct vector));
	normalCount += nCount;
	normalArray = realloc(normalArray, normalCount * sizeof(struct vector));
	textureCount += tCount;
	textureArray = realloc(textureArray, textureCount * sizeof(struct coord));
	
	newMesh->polygons = malloc(pCount * sizeof(struct poly));
	
	//newMesh->name;
	//newMesh->materials;
	//newMesh->materialCount;
	
	int currMatIdx = 0;
	int currVecIdx = 0;
	int currNorIdx = 0;
	int currTexIdx = 0;
	int currPolIdx = 0;
	
	while (fgets(currLine, 500, fileStream)) {
		token = strtok(currLine, ws);
		linenum++;
		
		if (token == NULL || stringEquals(token, "#")) {
			//skip comments
			continue;
		} else if (stringEquals(token, "v")) {
			//Vertex
			vertexArray[newMesh->firstVectorIndex + currVecIdx] = parseVertex();
			currVecIdx++;
		} else if (stringEquals(token, "vn")) {
			//Normal
			normalArray[newMesh->firstNormalIndex + currNorIdx] = parseVertex();
			currNorIdx++;
		} else if (stringEquals(token, "vt")) {
			//Texture coord
			textureArray[newMesh->firstTextureIndex + currTexIdx] = parseCoord();
			currTexIdx++;
		} else if (stringEquals(token, "f")) {
			//Polygon
			struct poly p = parsePoly(*newMesh);
			p.materialIndex = currMatIdx;
			newMesh->polygons[currPolIdx] = p;
			currPolIdx++;
		} else if (stringEquals(token, "usemtl")) {
			//current material index
			currMatIdx = findMaterialIndex(newMesh, strtok(NULL, ws));
		} else if (stringEquals(token, "mtllib")) {
			//new material
			int mtlCount = 0;
			//I am so very sorry for this. I didn't mean to. Just wanted to get it working.
			char *fullPath = (char*)calloc(1024, sizeof(char));
			sprintf(fullPath, "%s", filePath);
			char key = '\0';
			int i = (int)strcspn(filePath, &key);
			fullPath[i-3] = 'm';
			fullPath[i-2] = 't';
			fullPath[i-1] = 'l';
			
			struct material *newMats = parseMTLFile(fullPath, &mtlCount);
			
			if (newMats != NULL) {
				newMesh->materialCount = mtlCount;
				newMesh->materials = newMats;
			} else {
				newMesh->materials = calloc(1, sizeof(*newMesh->materials));
				newMesh->materials[0] = warningMaterial();
				newMesh->materialCount = 1;
			}
			free(fullPath);
		} else if (stringEquals(token, "o")) {
			//object name
			newMesh->name = copyString(strtok(NULL, ws));
		} else if (stringEquals(token, "s")) {
			//smoothShading
			//TODO
		} else {
			logr(warning, "Unrecognized command '%s' in OBJ file %s on line %i\n", token, filePath, linenum);
		}
	}
	
	fclose(fileStream);
	
	return newMesh;
}
