//
//  converter.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 18/08/2017.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "converter.h"

#include "../libraries/obj_parser.h"

/**
 Convert a given OBJ loader vector into a c-ray vector
 
 @param vec OBJ loader vector
 @return c-ray vector
 */
vec3 vec3FromObj(obj_vector *vec) {
	return (vec3){vec->e[0], vec->e[1], vec->e[2]};
}

vec2 vec2FromObj(obj_vector *vec) {
	return (vec2){vec->e[0], vec->e[1]};
}

/**
 Convert a given OBJ loader polygon into a c-ray polygon
 
 @param face OBJ loader polygon
 @param firstVertexIndex First vertex index of the new polygon
 @param firstNormalIndex First normal index of the new polygon
 @param firstTextureIndex First texture index of the new polygon
 @param polyIndex polygonArray index offset
 @return c-ray polygon
 */
struct poly polyFromObj(obj_face *face, int firstVertexIndex, int firstNormalIndex, int firstTextureIndex, int polyIndex) {
	struct poly polygon;
	
	if (face->normal_index[0] == -1) {
		polygon.hasNormals = false;
	} else {
		polygon.hasNormals = true;
	}
	
	polygon.vertexCount = face->vertex_count;
	//If no materials are found (missing .mtl), we will just patch in a bright pink material to show that
	polygon.materialIndex = face->material_index == -1 ? 0 : face->material_index;
	polygon.polyIndex = polyIndex;
	for (int i = 0; i < polygon.vertexCount; i++) {
		polygon.vertexIndex[i] = firstVertexIndex + face->vertex_index[i];
	}
	for (int i = 0; i < polygon.vertexCount; i++) {
		polygon.normalIndex[i] = firstNormalIndex + face->normal_index[i];
	}
	for (int i = 0; i < polygon.vertexCount; i++) {
		polygon.textureIndex[i] = firstTextureIndex + face->texture_index[i];
	}
	
	return polygon;
}


/**
 Convert a given OBJ loader material into a c-ray material
 
 @param mat OBJ loader material
 @return c-ray material
 */
struct material *materialFromObj(obj_material *obj_mat) {
	struct material *mat = newMaterial(MATERIAL_TYPE_DEFAULT);

	/*newMat.name = calloc(256, sizeof(char));
	newMat.textureFilePath = calloc(500, sizeof(char));
	
	newMat.hasTexture = false;
	
	for (int i = 0; i < 255; i++) {
		newMat.name[i] = mat->name[i];
		newMat.name[255] = '\0';
	}
	
	for (int i = 0; i < 500; i++) {
		newMat.textureFilePath[i] = mat->texture_filename[i];
		newMat.textureFilePath[499] = '\0';
	}*/

	setMaterialVec3(mat, "albedo", (vec3) { obj_mat->diff[0], obj_mat->diff[1], obj_mat->diff[2] });
	setMaterialFloat(mat, "ior", obj_mat->refract_index);
	setMaterialFloat(mat, "roughness", 1.0f - obj_mat->glossy);

	return mat;
}
