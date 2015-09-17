//
//  modeler.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 04/03/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "modeler.h"

polyMesh *buildPlane(vector *corner1, vector *corner2, vector *corner3, vector *corner4) {
    polyMesh *mesh;
    mesh->verticeCount = 4;
    mesh->vertexArray = (vector*)malloc(sizeof(vector) * mesh->verticeCount);
    mesh->vertexArray[0] = *corner1;
    mesh->vertexArray[1] = *corner2;
    mesh->vertexArray[2] = *corner3;
    mesh->vertexArray[3] = *corner4;
    return mesh;
}

int *translateMesh(polyMesh *mesh) {
    return 0;
}

polyMesh *buildCube(vector *corner1, vector *corner2) {
    return NULL;
}