//
//  mesh.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "mesh.h"

#include "../acceleration/bbox.h"
#include "../acceleration/kdtree.h"

//Parse given .obj and .mtl and return a mesh
struct mesh *parseOBJFile(char *fileName) {
	struct mesh *mesh = calloc(1, sizeof(struct mesh));
	
	//TODO
	
	return mesh;
}

void addTransform(struct mesh *mesh, struct matrixTransform transform) {
	if (mesh->transformCount == 0) {
		mesh->transforms = calloc(1, sizeof(struct matrixTransform));
	} else {
		mesh->transforms = realloc(mesh->transforms, (mesh->transformCount + 1) * sizeof(struct matrixTransform));
	}
	mesh->transforms[mesh->transformCount] = transform;
	mesh->transformCount++;
}

void transformMesh(struct mesh *mesh) {
	//Bit of a hack here, using way more memory than needed. Should also work on 32-bit now
	bool *tformed = (bool *)calloc(mesh->polyCount*3, 8);
	for (int tf = 0; tf < mesh->transformCount; tf++) {
		//Perform transforms
		for (int p = mesh->firstPolyIndex; p < (mesh->firstPolyIndex + mesh->polyCount); p++) {
			for (int v = 0; v < polygonArray[p].vertexCount; v++) {
				transformVector(&vertexArray[polygonArray[p].vertexIndex[v]], &mesh->transforms[tf], tformed[polygonArray[p].vertexIndex[v]]);
				tformed[polygonArray[p].vertexIndex[v]] = true;
			}
		}
		//Clear isTransformed flags
		for (int p = mesh->firstPolyIndex; p < (mesh->firstPolyIndex + mesh->polyCount); p++) {
			for (int v = 0; v < polygonArray[p].vertexCount; v++) {
				tformed[polygonArray[p].vertexIndex[v]] = false;
			}
		}
	}
	free(tformed);
}

void freeMesh(struct mesh *mesh) {
	if (mesh->meshName) {
		free(mesh->meshName);
	}
	for (int i = 0; i < mesh->materialCount; i++) {
		freeMaterial(&mesh->materials[i]);
	}
	free(mesh->materials);
	
	if (mesh->tree) {
		freeTree(mesh->tree);
	}
	free(mesh->tree);
	
	free(mesh->transforms);
}
