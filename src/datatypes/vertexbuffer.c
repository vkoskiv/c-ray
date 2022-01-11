//
//  vertexbuffer.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2022 Valtteri Koskivuori. All rights reserved.
//

//Main vertex arrays

/*
 Note:
 C-Ray stores all vertex data in shared arrays, and uses data structures
 to keep track of them.
 */

#include "../includes.h"

#include "vector.h"

struct vector *g_vertices;
int vertexCount;
struct vector *g_normals;
int normalCount;
struct coord *g_textureCoords;
int textureCount;

void allocVertexBuffers() {
	ASSERT(!g_vertices);
	g_vertices = calloc(1, sizeof(*g_vertices));
	g_normals = calloc(1, sizeof(*g_normals));
	g_textureCoords = calloc(1, sizeof(*g_textureCoords));
	
	vertexCount = 0;
	normalCount = 0;
	textureCount = 0;
}

void destroyVertexBuffers() {
	if (g_vertices) {
		free(g_vertices);
		g_vertices = NULL;
	}
	if (g_normals) {
		free(g_normals);
		g_normals = NULL;
	}
	if (g_textureCoords) {
		free(g_textureCoords);
		g_textureCoords = NULL;
	}
}
