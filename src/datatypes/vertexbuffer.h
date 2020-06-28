//
//  vertexbuffer.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

//Main vector arrays
extern struct vector *g_vertices;
extern int vertexCount;

extern struct vector *g_normals;
extern int normalCount;

extern struct coord *g_textureCoords;
extern int textureCount;

void allocVertexBuffers(void);
void destroyVertexBuffers(void);
