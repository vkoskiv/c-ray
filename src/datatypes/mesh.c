//
//  mesh.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "mesh.h"

#include "../acceleration/kdtree.h"
#include "../datatypes/vertexbuffer.h"

void addTransform(struct mesh *mesh, struct transform transform) {
	if (mesh->transformCount == 0) {
		mesh->transforms = calloc(1, sizeof(struct transform));
	} else {
		mesh->transforms = realloc(mesh->transforms, (mesh->transformCount + 1) * sizeof(struct transform));
	}
	mesh->transforms[mesh->transformCount] = transform;
	mesh->transformCount++;
}

void transformMesh(struct mesh *mesh) {
	//Bit of a hack here, using way more memory than needed. Should also work on 32-bit now
	bool *tformed = (bool *)calloc(mesh->vertexCount, sizeof(bool));
	bool *ntformed = (bool *)calloc(mesh->normalCount, sizeof(bool));
	for (int tf = 0; tf < mesh->transformCount; tf++) {
		//Perform transforms
		for (int p = mesh->firstPolyIndex; p < (mesh->firstPolyIndex + mesh->polyCount); p++) {
			for (int v = 0; v < polygonArray[p].vertexCount; v++) {
				//vec
				if (!tformed[polygonArray[p].vertexIndex[v] - mesh->firstVectorIndex]) {
					transformVector(&vertexArray[polygonArray[p].vertexIndex[v]], mesh->transforms[tf].A);
					tformed[polygonArray[p].vertexIndex[v] - mesh->firstVectorIndex] = true;
				}
			}
		}
		for (int n = mesh->firstNormalIndex; n < (mesh->firstNormalIndex + mesh->normalCount); n++) {
			//normal, skip translates and do inverse transpose
			if (!ntformed[n - mesh->firstNormalIndex]) {
				if (mesh->transforms[tf].type != transformTypeTranslate) {
					transformVector(&normalArray[n], transpose(mesh->transforms[tf].Ainv));
					ntformed[n - mesh->firstNormalIndex] = true;
				}
			}
		}
		//Clear isTransformed flags
		memset(tformed, 0, mesh->vertexCount * sizeof(bool));
		memset(ntformed, 0, mesh->normalCount * sizeof(bool));
	}
	free(tformed);
	free(ntformed);
}

void freeMesh(struct mesh *mesh) {
	if (mesh->name) {
		free(mesh->name);
	}
	if (mesh->transforms) {
		free(mesh->transforms);
	}
	if (mesh->tree) {
		freeTree(mesh->tree);
		free(mesh->tree);
	}
	freeMaterial(mesh->material);
}
