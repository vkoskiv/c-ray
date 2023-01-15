//
//  mesh.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 27/04/2017.
//  Copyright © 2017-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct vector;
struct coord;

struct mesh {
	struct vector *vertices;
	struct vector *normals;
	struct coord *texture_coords;
	struct poly *polygons;
	struct material *materials;
	struct bvh *bvh;
	float surface_area;
	char *name;
	int vertex_count;
	int normal_count;
	int tex_coord_count;
	int poly_count;
	int materialCount;
	float rayOffset;
};

void destroyMesh(struct mesh *mesh);
