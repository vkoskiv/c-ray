//
//  vertexbuffer.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

//Main vertex arrays

/*
 Note:
 C-Ray stores all vectors and polygons in shared arrays, and uses data structures
 to keep track of them.
 */

#include "../includes.h"

vec3 *vertexArray;
int vertexCount;
vec3 *normalArray;
int normalCount;
vec2 *textureArray;
int textureCount;

void freeVertexBuffer() {
	if (vertexArray) {
		free(vertexArray);
	}
	if (normalArray) {
		free(normalArray);
	}
	if (textureArray) {
		free(textureArray);
	}
	if (polygonArray) {
		free(polygonArray);
	}
}

void allocVertexBuffer() {
	vertexArray = calloc(1, sizeof(vec3));
	normalArray = calloc(1, sizeof(vec3));
	textureArray = calloc(1, sizeof(vec2));
	polygonArray = calloc(1, sizeof(struct poly));
	
	vertexCount = 0;
	normalCount = 0;
	textureCount = 0;
	polyCount = 0;
}
