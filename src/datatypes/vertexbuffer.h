//
//  vertexbuffer.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

//Main vector arrays
extern struct vector *vertexArray;
extern int vertexCount;

extern struct vector *normalArray;
extern int normalCount;

extern struct coord *textureArray;
extern int textureCount;

void allocVertexBuffer(void);
void freeVertexBuffer(void);
