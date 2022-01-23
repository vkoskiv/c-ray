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
	//Vertices
	int vertex_count;
	struct vector *vertices;
	
	//Normals
	int normal_count;
	struct vector *normals;
	
	//Texture coordinates
	int tex_coord_count;
	struct coord *texture_coords;
	
	//Faces
	int poly_count;
	struct poly *polygons;
	
	//Materials
	int materialCount;
	struct material *materials;
	
	struct bvh *bvh;

	float rayOffset;

	char *name;
};

void destroyMesh(struct mesh *mesh);
