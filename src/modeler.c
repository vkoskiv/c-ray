//
//  modeler.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 04/03/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "modeler.h"
#include "poly.h"
#include "vector.h"

/*polyMesh *buildPlane(vector *corner1, vector *corner2, vector *corner3, vector *corner4) {
    polyMesh *mesh = NULL;
    mesh->polyCount = 2;
    mesh->polyArray = (polygonObject*)malloc(sizeof(polygonObject) * mesh->polyCount);
    mesh->polyArray[0].v1 = *corner1;
    mesh->polyArray[0].v2 = *corner2;
    mesh->polyArray[0].v3 = *corner3;
    
    mesh->polyArray[1].v1 = *corner4;
    mesh->polyArray[1].v2 = *corner2;
    mesh->polyArray[1].v3 = *corner3;
    return mesh;
}

int *translateMesh(polyMesh *mesh) {
    return 0;
}

polyMesh *buildCube(vector *corner1, vector *corner2) {
    return NULL;
}*/