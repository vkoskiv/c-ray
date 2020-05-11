//
//  vertexbuffer.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

//Main vertex arrays

/*
 Note:
 C-Ray stores all vectors and polygons in shared arrays, and uses data structures
 to keep track of them.
 */

#include "../includes.h"

#include "poly.h"
#include "vector.h"
#include "../utils/assert.h"

struct vector *vertexArray;
int vertexCount;
struct vector *normalArray;
int normalCount;
struct coord *textureArray;
int textureCount;

void allocVertexBuffer() {
	ASSERT(!vertexArray);
	vertexArray = calloc(1, sizeof(*vertexArray));
	normalArray = calloc(1, sizeof(*normalArray));
	textureArray = calloc(1, sizeof(*textureArray));
	polygonArray = calloc(1, sizeof(*polygonArray));
	
	vertexCount = 0;
	normalCount = 0;
	textureCount = 0;
	polyCount = 0;
}

void destroyVertexBuffer() {
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
