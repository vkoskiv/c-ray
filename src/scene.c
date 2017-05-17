//
//  scene.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "includes.h"
#include "scene.h"

#include "camera.h"
#include "light.h"
#include "obj_parser.h"
#include "poly.h"
#include "obj.h"
#include "kdtree.h"

#define TOKEN_DEBUG_ENABLED false

//Prototypes
//Trims spaces and tabs from a char array
char *trimSpaces(char *inputLine);
//Parses a scene file and allocates memory accordingly
int allocMemory(struct scene *scene, char *inputFileName);


/**
 Convert a given OBJ loader vector into a c-ray vector

 @param vec OBJ loader vector
 @return c-ray vector
 */
struct vector vectorFromObj(obj_vector *vec) {
	struct vector vector;
	vector.x = vec->e[0];
	vector.y = vec->e[1];
	vector.z = vec->e[2];
	vector.isTransformed = false;
	return vector;
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
	if (face->normal_index[0] == -1)
		polygon.hasNormals = false;
	else
		polygon.hasNormals = true;
	polygon.vertexCount = face->vertex_count;
	polygon.materialIndex = face->material_index;
	polygon.polyIndex = polyIndex;
	for (int i = 0; i < polygon.vertexCount; i++)
		polygon.vertexIndex[i] = firstVertexIndex + face->vertex_index[i];
	for (int i = 0; i < polygon.vertexCount; i++)
		polygon.normalIndex[i] = firstNormalIndex + face->normal_index[i];
	for (int i = 0; i < polygon.vertexCount; i++)
		polygon.textureIndex[i] = firstTextureIndex + face->texture_index[i];
	return polygon;
}


/**
 Convert a given OBJ loader material into a c-ray material

 @param mat OBJ loader material
 @return c-ray material
 */
struct material materialFromObj(obj_material *mat) {
	struct material newMat;
	newMat.diffuse.red   = mat->diff[0];
	newMat.diffuse.green = mat->diff[1];
	newMat.diffuse.blue  = mat->diff[2];
	newMat.diffuse.alpha = 0;
	newMat.ambient.red   = mat->amb[0];
	newMat.ambient.green = mat->amb[1];
	newMat.ambient.blue  = mat->amb[2];
	newMat.ambient.alpha = 0;
	newMat.specular.red  = mat->spec[0];
	newMat.specular.green= mat->spec[1];
	newMat.specular.blue = mat->spec[2];
	newMat.specular.alpha= 0;
	newMat.reflectivity  = mat->reflect;
	newMat.refractivity  = mat->refract;
	newMat.IOR           = mat->refract_index;
	newMat.glossiness    = mat->glossy;
	newMat.transparency  = mat->trans;
	newMat.sharpness     = 0;
	return newMat;
}

/**
 Extract the filename from a given file path

 @param input File path to be processed
 @return Filename string, including file type extension
 */
char *getFileName(char *input) {
	char *fn;
	
	/* handle trailing '/' e.g.
	 input == "/home/me/myprogram/" */
	if (input[(strlen(input) - 1)] == '/')
		input[(strlen(input) - 1)] = '\0';
	
	(fn = strrchr(input, '/')) ? ++fn : (fn = input);
	
	return fn;
}

void addMaterialOBJ(struct crayOBJ *obj, struct material newMaterial);

bool addOBJ(struct scene *sceneData, char *inputFileName) {
	printf("Loading OBJ %s\n", inputFileName);
	obj_scene_data data;
	if (parse_obj_scene(&data, inputFileName) == 0) {
		printf("OBJ %s file not found!\n", getFileName(inputFileName));
		return false;
	}
	printf("OBJ loaded, converting\n");
	
	//Create crayOBJ to keep track of objs
	sceneData->objs = (struct crayOBJ*)realloc(sceneData->objs, (sceneData->objCount + 1) * sizeof(struct crayOBJ));
	//Vertex data
	sceneData->objs[sceneData->objCount].firstVectorIndex = vertexCount;
	sceneData->objs[sceneData->objCount].vertexCount = data.vertex_count;
	//Normal data
	sceneData->objs[sceneData->objCount].firstNormalIndex = normalCount;
	sceneData->objs[sceneData->objCount].normalCount = data.vertex_normal_count;
	//Texture vector data
	sceneData->objs[sceneData->objCount].firstTextureIndex = textureCount;
	sceneData->objs[sceneData->objCount].textureCount = data.vertex_texture_count;
	//Poly data
	sceneData->objs[sceneData->objCount].firstPolyIndex = polyCount;
	sceneData->objs[sceneData->objCount].polyCount = data.face_count;
	//Transforms init
	sceneData->objs[sceneData->objCount].transformCount = 0;
	sceneData->objs[sceneData->objCount].transforms = (struct matrixTransform*)malloc(sizeof(struct matrixTransform));
	
	sceneData->objs[sceneData->objCount].materialCount = 0;
	//Set name
	sceneData->objs[sceneData->objCount].objName = getFileName(inputFileName);
	
	//Update vector and poly counts
	vertexCount += data.vertex_count;
	normalCount += data.vertex_normal_count;
	textureCount += data.vertex_texture_count;
	polyCount += data.face_count;
	
	//Data loaded, now convert everything
	//Convert vectors
	printf("Converting vectors\n");
	vertexArray = (struct vector*)realloc(vertexArray, vertexCount * sizeof(struct vector));
	for (int i = 0; i < data.vertex_count; i++) {
		vertexArray[sceneData->objs[sceneData->objCount].firstVectorIndex + i] = vectorFromObj(data.vertex_list[i]);
	}
	
	//Convert normals
	printf("Converting normals\n");
	normalArray = (struct vector*)realloc(normalArray, normalCount * sizeof(struct vector));
	for (int i = 0; i < data.vertex_normal_count; i++) {
		normalArray[sceneData->objs[sceneData->objCount].firstNormalIndex + i] = vectorFromObj(data.vertex_normal_list[i]);
	}
	//Convert texture vectors
	printf("Converting texture coordinates\n");
	textureArray = (struct vector*)realloc(textureArray, textureCount * sizeof(struct vector));
	for (int i = 0; i < data.vertex_texture_count; i++) {
		textureArray[sceneData->objs[sceneData->objCount].firstTextureIndex + i] = vectorFromObj(data.vertex_texture_list[i]);
	}
	//Convert polygons
	printf("Converting polygons\n");
	polygonArray = (struct poly*)realloc(polygonArray, polyCount * sizeof(struct poly));
	for (int i = 0; i < data.face_count; i++) {
		polygonArray[sceneData->objs[sceneData->objCount].firstPolyIndex + i] = polyFromObj(data.face_list[i],
																							sceneData->objs[sceneData->objCount].firstVectorIndex,
																							sceneData->objs[sceneData->objCount].firstNormalIndex,
																							sceneData->objs[sceneData->objCount].firstTextureIndex,
																							sceneData->objs[sceneData->objCount].firstPolyIndex + i);
	}
	
	sceneData->objs[sceneData->objCount].materials = (struct material*)calloc(1, sizeof(struct material));
	//Parse materials
	if (data.material_count == 0) {
		//No material, set to something obscene to make it noticeable
		sceneData->objs[sceneData->objCount].materials = (struct material*)calloc(1, sizeof(struct material));
		*sceneData->objs[sceneData->objCount].materials = newMaterial(colorWithValues(255.0/255.0, 20.0/255.0, 147.0/255.0, 0), 0);
	} else {
		//Loop to add materials to obj (We already set the material indices in polyFromObj)
		for (int i = 0; i < data.material_count; i++) {
			addMaterialOBJ(&sceneData->objs[sceneData->objCount], materialFromObj(data.material_list[i]));
		}
	}
	
	//Delete OBJ data
	delete_obj_data(&data);
	printf("Loaded OBJ! Translated %i faces and %i vectors\n\n", data.face_count, data.vertex_count);
	
	//Obj added, update count
	sceneData->objCount++;
	return true;
}

//FIXME: Temporary
void overrideMaterial(struct scene *world, struct crayOBJ *obj, int materialIndex) {
	obj->materials = &world->materials[materialIndex];
}

//FIXME: change + 1 to ++scene->someCount and just pass the count to array access
//In the future, maybe just pass a list and size and copy at once to save time (large counts)
void addSphere(struct scene *scene, struct sphere newSphere) {
	scene->spheres = (struct sphere*)realloc(scene->spheres, (scene->sphereCount + 1) * sizeof(struct sphere));
	scene->spheres[scene->sphereCount++] = newSphere;
}

void addMaterial(struct scene *scene, struct material newMaterial) {
	scene->materials = (struct material*)realloc(scene->materials, (scene->materialCount + 1) * sizeof(struct material));
	scene->materials[scene->materialCount++] = newMaterial;
}

void addMaterialOBJ(struct crayOBJ *obj, struct material newMaterial) {
	obj->materials = (struct material*)realloc(obj->materials, (obj->materialCount + 1) * sizeof(struct material));
	obj->materials[obj->materialCount++] = newMaterial;
}

void addLight(struct scene *scene, struct light newLight) {
	scene->lights = (struct light*)realloc(scene->lights, (scene->lightCount + 1) * sizeof(struct light));
	scene->lights[scene->lightCount++] = newLight;
}

void addCamera(struct scene *scene, struct camera *newCamera) {
	scene->camera = (struct camera*)realloc(scene->camera, (scene->cameraCount + 1) * sizeof(struct camera));
	scene->camera[scene->cameraCount++] = *newCamera;
}

void transformMeshes(struct scene *scene) {
	printf("Running transforms...\n");
	for (int i = 0; i < scene->objCount; ++i) {
		printf("Transforming %s...", scene->objs[i].objName);
		transformMesh(&scene->objs[i]);
		printf("Transformed %s!\n", scene->objs[i].objName);
	}
	printf("Transforms done!\n");
}

void computeKDTrees(struct scene *scene) {
	printf("Computing KD-trees...\n");
	for (int i = 0; i < scene->objCount; ++i) {
		printf("Computing tree for %s...", scene->objs[i].objName);
		scene->objs[i].tree = buildTree(&polygonArray[scene->objs[i].firstPolyIndex],
										scene->objs[i].polyCount,
										scene->objs[i].firstPolyIndex, 0);
		printf(" Done!\n");
	}
}

void computeFocalLength(struct scene *world) {
	//Focal length is calculated based on the camera FOV value
	if (world->camera->FOV > 0.0 && world->camera->FOV < 189.0) {
		world->camera->focalLength = 0.5 * world->camera->width / tanf((double)(PIOVER180) * 0.5 * world->camera->FOV);
	}
}

//FIXME: Move this to transforms.c
void addCamTransform(struct scene *world, struct matrixTransform transform) {
	if (world->camTransformCount == 0) {
		world->camTransforms = (struct matrixTransform*)calloc(1, sizeof(struct matrixTransform));
	} else {
		world->camTransforms = (struct matrixTransform*)realloc(world->camTransforms, (world->camTransformCount + 1) * sizeof(struct matrixTransform));
	}
	
	world->camTransforms[world->camTransformCount] = transform;
	world->camTransformCount++;
}

void printSceneStats(struct scene *scene) {
	printf("SceneBuilder done!\n");
	printf("Totals: %i vectors, %i normals, %i polygons, %i spheres and %i lights\n\n",
		   vertexCount,
		   normalCount,
		   polyCount,
		   scene->sphereCount,
		   scene->lightCount);
}

int testBuild(struct scene *scene, char *inputFileName) {
	printf("Starting SceneBuilder V0.5\n\n");
	
	//MATERIALS
	addMaterial(scene, newMaterial(colorWithValues(0.6, 0.1, 0.1, 0.0), 0.0)); //Matte red
	addMaterial(scene, newMaterial(colorWithValues(0.1, 0.5, 0.1, 0.0), 0.0)); //Matte green
	addMaterial(scene, newMaterial(colorWithValues(0.1, 0.1, 0.5, 0.0), 0.0)); //Matte blue
	addMaterial(scene, newMaterial(colorWithValues(0.8, 0.8, 0.8, 0.0), 0.0));
	addMaterial(scene, newMaterial(colorWithValues(0.0, 0.5, 1.0, 0.0), 1.0)); //0.517647
	addMaterial(scene, newMaterial(colorWithValues(0.3, 0.3, 0.3, 0.0), 1.0));
	addMaterial(scene, newMaterial(colorWithValues(0.3, 0.0, 0.0, 0.0), 1.0));
	addMaterial(scene, newMaterial(colorWithValues(0.0, 0.3, 0.0, 0.0), 1.0));
	addMaterial(scene, newMaterial(colorWithValues(0.0, 0.0, 0.3, 0.0), 0.0));
	addMaterial(scene, newMaterial(colorWithValues(0.9, 0.9, 0.9, 0.0), 0.0));
	addMaterial(scene, newMaterial(colorWithValues(1.0, 0.0, 0.0, 0.0), 0.0));
	
	struct camera *cam = (struct camera*)calloc(1, sizeof(struct camera));
	//Override renderer thread count, 0 defaults to physical core count
	cam-> threadCount = 0;
	cam->filePath     = "../output/";
	cam->       width = 1280;
	cam->      height = 800;
	cam->isFullScreen = false;
	cam->isBorderless = false;
	cam->         FOV = 80.0;
	cam-> focalLength = 0;
	cam-> sampleCount = 100;
	cam->  frameCount = 1;
	cam->     bounces = 3;
	cam->    contrast = 0.5;
	cam-> windowScale = 1.0;
	cam->    fileType = png;
	cam->  areaLights = true;
	cam->antialiasing = true;
	cam->newRenderer  = false; //New, recursive rayTracing algorighm (buggy!)
	cam->  tileWidth  = 64;
	cam->  tileHeight = 64;
	cam->   tileOrder = renderOrderFromMiddle;
	cam->pos = vectorWithPos(0, 0, 0); //Don't change
	
	addCamTransform(scene, newTransformTranslate(970, 480, 600)); //Set pos here
	addCamTransform(scene, newTransformRotateX(21));//And add as many rotations as you want!
	addCamTransform(scene, newTransformRotateZ(9)); //Don't scale or translate!
	
	scene->ambientColor = (struct color*)calloc(1, sizeof(struct color));
	scene->ambientColor->  red = 0.4;
	scene->ambientColor->green = 0.6;
	scene->ambientColor-> blue = 0.6;
	
	addCamera(scene, cam);
	computeFocalLength(scene);
	free(cam);
	
	//NOTE: Translates have to come last!
	if (addOBJ(scene, "../output/newScene.obj")) {
		//Add transforms here
	}
	
	if (addOBJ(scene, "../output/torus.obj")) {
		addTransform(&scene->objs[scene->objCount - 1], newTransformScale(40, 40, 40));
		addTransform(&scene->objs[scene->objCount - 1], newTransformTranslate(1070, 320, 820));
	}
	
	//R G B is 0 1 2
	if (addOBJ(scene, "../output/teapot_test.obj")) {
		addTransform(&scene->objs[scene->objCount - 1], newTransformScale(80, 80, 80));
		addTransform(&scene->objs[scene->objCount - 1], newTransformRotateY(45));
		addTransform(&scene->objs[scene->objCount - 1], newTransformTranslate(740, 300, 900));
	}
	
	if (addOBJ(scene, "../output/teapot_test.obj")) {
		addTransform(&scene->objs[scene->objCount - 1], newTransformScale(80, 80, 80));
		addTransform(&scene->objs[scene->objCount - 1], newTransformRotateY(45));
		addTransform(&scene->objs[scene->objCount - 1], newTransformTranslate(740, 300, 1050));
	}
	
	if (addOBJ(scene, "../output/teapot_test.obj")) {
		addTransform(&scene->objs[scene->objCount - 1], newTransformScale(80, 80, 80));
		addTransform(&scene->objs[scene->objCount - 1], newTransformRotateY(45));
		addTransform(&scene->objs[scene->objCount - 1], newTransformTranslate(740, 300, 1200));
	}
	
	if (addOBJ(scene, "../output/teapot_green.obj")) {
		addTransform(&scene->objs[scene->objCount - 1], newTransformScale(80, 80, 80));
		addTransform(&scene->objs[scene->objCount - 1], newTransformRotateY(20));
		addTransform(&scene->objs[scene->objCount - 1], newTransformTranslate(970, 300, 900));
	}
	
	if (addOBJ(scene, "../output/teapot_green.obj")) {
		addTransform(&scene->objs[scene->objCount - 1], newTransformScale(80, 80, 80));
		addTransform(&scene->objs[scene->objCount - 1], newTransformRotateY(20));
		addTransform(&scene->objs[scene->objCount - 1], newTransformTranslate(970, 300, 1050));
	}
	
	if (addOBJ(scene, "../output/teapot_green.obj")) {
		addTransform(&scene->objs[scene->objCount - 1], newTransformScale(80, 80, 80));
		addTransform(&scene->objs[scene->objCount - 1], newTransformRotateY(20));
		addTransform(&scene->objs[scene->objCount - 1], newTransformTranslate(970, 300, 1200));
	}
	
	if (addOBJ(scene, "../output/teapot_blue.obj")) {
		addTransform(&scene->objs[scene->objCount - 1], newTransformScale(80, 80, 80));
		addTransform(&scene->objs[scene->objCount - 1], newTransformRotateY(155));
		addTransform(&scene->objs[scene->objCount - 1], newTransformTranslate(1210, 300,900));
	}
	
	if (addOBJ(scene, "../output/teapot_blue.obj")) {
		addTransform(&scene->objs[scene->objCount - 1], newTransformScale(80, 80, 80));
		addTransform(&scene->objs[scene->objCount - 1], newTransformRotateY(155));
		addTransform(&scene->objs[scene->objCount - 1], newTransformTranslate(1210, 300,1050));
	}
	
	if (addOBJ(scene, "../output/teapot_blue.obj")) {
		addTransform(&scene->objs[scene->objCount - 1], newTransformScale(80, 80, 80));
		addTransform(&scene->objs[scene->objCount - 1], newTransformRotateY(155));
		addTransform(&scene->objs[scene->objCount - 1], newTransformTranslate(1210, 300,1200));
	}
	
	//Transform all meshes
	transformMeshes(scene);
	//And compute the k-d trees for each mesh
	computeKDTrees(scene);
	
	//LIGHTS
	
	addLight(scene, newLight(vectorWithPos(970, 350, 500), 13, colorWithValues(2, 2, 4, 0)));
	
	/*addLight(scene, newLight(vectorWithPos(1160, 400, 0),    13, colorWithValues(0.2, 0.2, 0.2, 0.0)));
	addLight(scene, newLight(vectorWithPos(760 , 500, 0),    42, colorWithValues(0.2, 0.2, 0.2, 0.0)));
	addLight(scene, newLight(vectorWithPos(640 , 350, 600), 200, colorWithValues(6.0, 0.0, 0.0, 0.0)));
	addLight(scene, newLight(vectorWithPos(940 , 350, 600), 200, colorWithValues(0.0, 6.0, 0.0, 0.0)));
	addLight(scene, newLight(vectorWithPos(1240, 350, 600), 200, colorWithValues(0.0, 0.0, 6.0, 0.0)));*/
	
	addSphere(scene, newSphere(vectorWithPos(650, 450, 1650), 150, 5));
	addSphere(scene, newSphere(vectorWithPos(950, 350, 1500), 50, 6));
	addSphere(scene, newSphere(vectorWithPos(1100, 350, 1500), 50, 8));
	
	printSceneStats(scene);
	
	if (TOKEN_DEBUG_ENABLED) {
		return 4; //Debug mode - Won't render anything
	} else {
		return 0;
	}
}

//Removes tabs and spaces from a char byte array, terminates it and returns it.
char *trimSpaces(char *inputLine) {
	int i, j;
	char *outputLine = inputLine;
	for (i = 0, j = 0; i < strlen(inputLine); i++, j++) {
		if (inputLine[i] == ' ') { //Space
			j--;
		} else if (inputLine[i] == '\t') { //Tab
			j--;
		} else {
			outputLine[j] = inputLine[i];
		}
	}
	//Add null termination byte
	outputLine[j] = '\0';
	return outputLine;
}
