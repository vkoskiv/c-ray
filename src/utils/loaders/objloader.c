//
//  objloader.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "objloader.h"

#include "../../datatypes/mesh.h"
#include "../../utils/logging.h"
#include "../../utils/filehandler.h"
#include "../../datatypes/vertexbuffer.h"
#include "mtlloader.h"

#define ws " \t\n\r"

vec3 parseVertex() {
	return (vec3){atof(strtok(NULL, ws)), atof(strtok(NULL, ws)), atof(strtok(NULL, ws))};
}

vec2 parsevec2() {
	return (vec2){atof(strtok(NULL, ws)), atof(strtok(NULL, ws))};
}

int parseIndices(int *vertexIndex, int *normalIndex, int *textureIndex) {
	
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

int convert(int amount, int index) {
	if (index == 0)
		return -1;
	if (index  < 0)
		return amount + index;
	
	return index - 1;
}

void convertIndices(int amount, int *indices) {
	for (int i = 0; i < 3; i++) {
		indices[i] = convert(amount, indices[i]);
	}
}

struct poly parsePoly() {
	struct poly p;
	p.vertexCount = parseIndices(p.vertexIndex, p.normalIndex, p.textureIndex);
	
	/*convertIndices(vertexCount, p.vertexIndex);
	convertIndices(normalCount, p.normalIndex);
	convertIndices(textureCount, p.textureIndex);*/
	
	for (int i = 0; i < p.vertexCount; i++) {
		if (p.normalIndex[i] == 0) {
			p.hasNormals = false;
		}
		
		p.vertexIndex[i] = (vertexCount+1) + p.vertexIndex[i];
		p.normalIndex[i] = (normalCount+1) + p.normalIndex[i];
		p.textureIndex[i] = (textureCount+1) + p.textureIndex[i];
	}
	
	return p;
}

int findMaterialIndex(struct mesh *mesh, char *mtlName) {
	/*
	for (int i = 0; i < mesh->materialCount; i++) {
		if (stringEquals(mesh->materials[i]->name, mtlName)) {
			return i;
		}
	}*/
	//return mesh->mat;
	return 0;
}

//Parse a Wavefront OBJ file and return a generic mesh.
//Note: This will also alter the global vertex arrays
//We will add vertices to them as the OBJ is loaded.
struct mesh *parseOBJFile(char *filePath) {
	struct mesh *newMesh = calloc(1, sizeof(struct mesh));
	
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
		if(token == NULL || stringEquals(token, "//") || stringEquals(token, "#")) {
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
	newMesh->firstPolyIndex = polyCount;
	
	newMesh->vertexCount = vCount;
	newMesh->normalCount = nCount;
	newMesh->textureCount = tCount;
	newMesh->polyCount = pCount;
	
	vertexCount += vCount;
	vertexArray = realloc(vertexArray, vertexCount * sizeof(vec3));
	normalCount += nCount;
	normalArray = realloc(normalArray, normalCount * sizeof(vec3));
	textureCount += tCount;
	textureArray = realloc(textureArray, textureCount * sizeof(vec3));
	polyCount += pCount;
	polygonArray = realloc(polygonArray, polyCount * sizeof(struct poly));
	
	//newMesh->name;
	//newMesh->materials = malloc(sizeof(IMaterial));
	//newMesh->materialCount = 1;
	
	int currMatIdx = 0;
	int currVecIdx = 0;
	int currNorIdx = 0;
	int currTexIdx = 0;
	int currPolIdx = 0;
	
	while (fgets(currLine, 500, fileStream)) {
		token = strtok(currLine, ws);
		linenum++;
		
		if (token == NULL || stringEquals(token, "//") || stringEquals(token, "#")) {
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
			//Texture vec2
			textureArray[newMesh->firstTextureIndex + currTexIdx] = parsevec2();
			currTexIdx++;
		} else if (stringEquals(token, "f")) {
			//Polygon
			struct poly p = parsePoly();
			p.materialIndex = 0;
			p.polyIndex = currPolIdx;
			polygonArray[newMesh->firstPolyIndex + currPolIdx] = p;
			currPolIdx++;
		} else if (stringEquals(token, "usemtl")) {
			//current material index
			currMatIdx = 0;// findMaterialIndex(newMesh, strtok(NULL, ws));
		} else if (stringEquals(token, "mtllib")) {
			//new material
			/*
			int *mtlCount = malloc(1*sizeof(int));
			char *fullPath = (char*)calloc(1024, sizeof(char));
			sprintf(fullPath, "%s%s", "input/", strtok(NULL, ws));
			IMaterial *newMats = parseMTLFile(fullPath, mtlCount);
			if (newMats != NULL) {
				//newMesh->materialCount = *mtlCount;
				//newMesh->materials = newMats;
			}
			free(fullPath);
			free(mtlCount);
			*/
		} else if (stringEquals(token, "o")) {
			//object name
			copyString(strtok(NULL, ws), &newMesh->name);
		} else if (stringEquals(token, "s")) {
			//smoothShading
			//TODO
		} else {
			logr(warning, "Unrecognized command '%s' in OBJ file %s on line %i\n", token, filePath, linenum);
		}
	}

	//newMesh->mat = NewMaterial(MATERIAL_TYPE_DEFAULT);
	fclose(fileStream);
	
	return newMesh;
}
