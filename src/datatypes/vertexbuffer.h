//
//  vertexbuffer.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#pragma once

//Main vector arrays
extern vec3 *vertexArray;
extern int vertexCount;

extern vec3 *normalArray;
extern int normalCount;

extern vec2 *textureArray;
extern int textureCount;

void allocVertexBuffer(void);
void freeVertexBuffer(void);
