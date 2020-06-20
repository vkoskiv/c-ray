//
//  converter.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 18/08/2017.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

//OBJ parser declares structures using typedefs, so have to include this here.
#include "../libraries/obj_parser.h"

/**
 Convert a given OBJ loader vector into a c-ray vector
 
 @param vec OBJ loader vector
 @return c-ray vector
 */
struct vector vectorFromObj(obj_vector *vec);

struct coord coordFromObj(obj_vector *vec);

/**
 Convert a given OBJ loader polygon into a c-ray polygon
 
 @param face OBJ loader polygon
 @param firstVertexIndex First vertex index of the new polygon
 @param firstNormalIndex First normal index of the new polygon
 @param firstTextureIndex First texture index of the new polygon
 @param polyIndex polygonArray index offset
 @return c-ray polygon
 */
struct poly polyFromObj(obj_face *face, int firstVertexIndex, int firstNormalIndex, int firstTextureIndex, int meshIndex);

/**
 Convert a given OBJ loader material into a c-ray material
 
 @param mat OBJ loader material
 @return c-ray material
 */
struct material materialFromObj(obj_material *mat);
