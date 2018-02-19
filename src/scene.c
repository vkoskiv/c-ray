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
#include "filehandler.h"
#include "converter.h"
#include "renderer.h"

#define TOKEN_DEBUG_ENABLED false

char *trimSpaces(char *inputLine);

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

bool loadOBJ(struct renderer *r, char *inputFileName) {
	printf("Loading OBJ %s%s\n", r->inputFilePath, inputFileName);
	
	obj_scene_data data;
	if (parse_obj_scene(&data, inputFileName, r->inputFilePath) == 0) {
		printf("OBJ %s file not found!\n", getFileName(inputFileName));
		return false;
	}
	printf("OBJ loaded, converting\n");
	
	//Create crayOBJ to keep track of objs
	r->scene->objs = (struct crayOBJ*)realloc(r->scene->objs, (r->scene->objCount + 1) * sizeof(struct crayOBJ));
	//Vertex data
	r->scene->objs[r->scene->objCount].firstVectorIndex = vertexCount;
	r->scene->objs[r->scene->objCount].vertexCount = data.vertex_count;
	//Normal data
	r->scene->objs[r->scene->objCount].firstNormalIndex = normalCount;
	r->scene->objs[r->scene->objCount].normalCount = data.vertex_normal_count;
	//Texture vector data
	r->scene->objs[r->scene->objCount].firstTextureIndex = textureCount;
	r->scene->objs[r->scene->objCount].textureCount = data.vertex_texture_count;
	//Poly data
	r->scene->objs[r->scene->objCount].firstPolyIndex = polyCount;
	r->scene->objs[r->scene->objCount].polyCount = data.face_count;
	//Transforms init
	r->scene->objs[r->scene->objCount].transformCount = 0;
	r->scene->objs[r->scene->objCount].transforms = (struct matrixTransform*)malloc(sizeof(struct matrixTransform));
	
	r->scene->objs[r->scene->objCount].materialCount = 0;
	//Set name
	r->scene->objs[r->scene->objCount].objName = getFileName(inputFileName);
	
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
		vertexArray[r->scene->objs[r->scene->objCount].firstVectorIndex + i] = vectorFromObj(data.vertex_list[i]);
	}
	
	//Convert normals
	printf("Converting normals\n");
	normalArray = (struct vector*)realloc(normalArray, normalCount * sizeof(struct vector));
	for (int i = 0; i < data.vertex_normal_count; i++) {
		normalArray[r->scene->objs[r->scene->objCount].firstNormalIndex + i] = vectorFromObj(data.vertex_normal_list[i]);
	}
	//Convert texture vectors
	printf("Converting texture coordinates\n");
	textureArray = (struct vector*)realloc(textureArray, textureCount * sizeof(struct vector));
	for (int i = 0; i < data.vertex_texture_count; i++) {
		textureArray[r->scene->objs[r->scene->objCount].firstTextureIndex + i] = vectorFromObj(data.vertex_texture_list[i]);
	}
	//Convert polygons
	printf("Converting polygons\n");
	polygonArray = (struct poly*)realloc(polygonArray, polyCount * sizeof(struct poly));
	for (int i = 0; i < data.face_count; i++) {
		polygonArray[r->scene->objs[r->scene->objCount].firstPolyIndex + i] = polyFromObj(data.face_list[i],
																							r->scene->objs[r->scene->objCount].firstVectorIndex,
																							r->scene->objs[r->scene->objCount].firstNormalIndex,
																							r->scene->objs[r->scene->objCount].firstTextureIndex,
																							r->scene->objs[r->scene->objCount].firstPolyIndex + i);
	}
	
	r->scene->objs[r->scene->objCount].materials = (struct material*)calloc(1, sizeof(struct material));
	//Parse materials
	if (data.material_count == 0) {
		//No material, set to something obscene to make it noticeable
		r->scene->objs[r->scene->objCount].materials = (struct material*)calloc(1, sizeof(struct material));
		*r->scene->objs[r->scene->objCount].materials = newMaterial(colorWithValues(255.0/255.0, 20.0/255.0, 147.0/255.0, 0), 0);
	} else {
		//Loop to add materials to obj (We already set the material indices in polyFromObj)
		for (int i = 0; i < data.material_count; i++) {
			addMaterialOBJ(&r->scene->objs[r->scene->objCount], materialFromObj(data.material_list[i]));
		}
	}
	
	//Delete OBJ data
	delete_obj_data(&data);
	printf("Loaded OBJ! Translated %i faces and %i vectors\n\n", data.face_count, data.vertex_count);
	
	//Obj added, update count
	r->scene->objCount++;
	return true;
}

//FIXME: change + 1 to ++scene->someCount and just pass the count to array access
//In the future, maybe just pass a list and size and copy at once to save time (large counts)
void addSphere(struct world *scene, struct sphere newSphere) {
	scene->spheres = (struct sphere*)realloc(scene->spheres, (scene->sphereCount + 1) * sizeof(struct sphere));
	scene->spheres[scene->sphereCount++] = newSphere;
}

void addMaterial(struct world *scene, struct material newMaterial) {
	scene->materials = (struct material*)realloc(scene->materials, (scene->materialCount + 1) * sizeof(struct material));
	scene->materials[scene->materialCount++] = newMaterial;
}

void addMaterialOBJ(struct crayOBJ *obj, struct material newMaterial) {
	obj->materials = (struct material*)realloc(obj->materials, (obj->materialCount + 1) * sizeof(struct material));
	obj->materials[obj->materialCount++] = newMaterial;
}

void addLight(struct world *scene, struct light newLight) {
	scene->lights = (struct light*)realloc(scene->lights, (scene->lightCount + 1) * sizeof(struct light));
	scene->lights[scene->lightCount++] = newLight;
}

void transformMeshes(struct world *scene) {
	printf("Running transforms...\n");
	for (int i = 0; i < scene->objCount; ++i) {
		printf("Transforming %s...", scene->objs[i].objName);
		transformMesh(&scene->objs[i]);
		printf("Transformed %s!\n", scene->objs[i].objName);
	}
	printf("Transforms done!\n");
}

void computeKDTrees(struct world *scene) {
	printf("Computing KD-trees...\n");
	for (int i = 0; i < scene->objCount; ++i) {
		printf("Computing tree for %s...", scene->objs[i].objName);
		scene->objs[i].tree = buildTree(&polygonArray[scene->objs[i].firstPolyIndex],
										scene->objs[i].polyCount,
										scene->objs[i].firstPolyIndex, 0);
		printf(" Done!\n");
	}
}

//FIXME: Move this to transforms.c
void addCamTransform(struct camera *cam, struct matrixTransform transform) {
	if (cam->transformCount == 0) {
		cam->transforms = (struct matrixTransform*)calloc(1, sizeof(struct matrixTransform));
	} else {
		cam->transforms = (struct matrixTransform*)realloc(cam->transforms, (cam->transformCount + 1) * sizeof(struct matrixTransform));
	}
	
	cam->transforms[cam->transformCount] = transform;
	cam->transformCount++;
}

void printSceneStats(struct world *scene) {
	printf("SceneBuilder done!\n");
	printf("Totals: %i vectors, %i normals, %i polygons, %i spheres and %i lights\n\n",
		   vertexCount,
		   normalCount,
		   polyCount,
		   scene->sphereCount,
		   scene->lightCount);
}

/*int buildScene(struct renderer *r, char *inputFile) {
	printf("Starting C-Ray Scene Tokenizer\n\n");
	
	FILE *file = fopen(inputFile, "r");
	if (!inputFile) {
		return -1;
	}
	
	//Costants
	const char *equals = "=", *comma = ",", *closeBlock = "}";
	char *err = NULL;
	char *token;
	char *savePointer;
	int materialIndex = 0, sphereIndex = 0, objIndex = 0, lightIndex = 0;
	
	char line[255];
	
	while (fgets(line, sizeof(line), file) != NULL) {
		//Trim comments
		if (trimSpaces(line)[0] == '#') {
			//Ignore
		}
		
		//output image params
		if (strcmp(trimSpaces(line), "output(){\n") == 0) {
 
			 r->image->filePath = "output/";
			 r->image->fileName = "rendered";
			 r->image->count = 0;
			 r->image->size.width = 1280;
			 r->image->size.height = 800;
			 r->image->fileType = png;
 
			
			while (trimSpaces(line)[0] != *closeBlock) {
				err = fgets(trimSpaces(line), sizeof(line), file);
				if (!err) {
					printf("Failed to parse image params\n");
				}
				if (strncmp(trimSpaces(line), "outputFilePath", 14) == 0) {
					token = strtok_r(trimSpaces(line), equals, &savePointer);
					
				}
			}
		}
		
		//renderer params
		if (strcmp(trimSpaces(line), "renderer(){\n") == 0) {
 
			 r->threadCount = 0; //Override, 0 defaults to physical core count
			 r->sampleCount = 25;
			 r->antialiasing = true;
			 r->newRenderer = false; //New, recursive rayTracing algorighm (buggy!)
			 r->tileWidth = 128;
			 r->tileHeight = 128;
			 r->tileOrder = renderOrderFromMiddle;
 
		}
		
		//camera params
		if (strcmp(trimSpaces(line), "camera(){\n") == 0) {
 
			 r->scene->camera->isFullScreen = false;
			 r->scene->camera->isBorderless = false;
			 r->scene->camera-> windowScale = 1.0;
			 
			 r->scene->camera->         FOV = 80.0;
			 r->scene->camera->    aperture = 0.0;
			 r->scene->camera->    contrast = 0.5;
			 r->scene->camera->bounces = 3;
			 r->scene->camera->areaLights = true;
			 And pos + rotations
 
		}
		
		//Scene, OBJs + transforms
		if (strcmp(trimSpaces(line), "scene(){\n") == 0) {
			<#statements#>
		}
	}
	
	return 0;
}*/

int testBuild(struct renderer *r, char *inputFileName) {
	printf("Starting SceneBuilder V0.5\n\n");
	
	//MATERIALS
	addMaterial(r->scene, newMaterialFull(colorWithValues(0.1, 0.05, 0.05, 0.0),
										colorWithValues(0.6, 0.1, 0.1, 0.0),
										colorWithValues(1, 0.2, 0.2, 0.0), .2, .1, 0, 0, 0, 20.)); //Matte red

	addMaterial(r->scene, newMaterial(colorWithValues(0.1, 0.1, 0.1, 0.0), 0.0)); //Matte green
	addMaterial(r->scene, newMaterial(colorWithValues(0.1, 0.1, 0.2, 0.0), 0.0)); //Matte blue
	addMaterial(r->scene, newMaterial(colorWithValues(0.8, 0.8, 0.8, 0.0), 0.0));
	addMaterial(r->scene, newMaterial(colorWithValues(0.0, 0.5, 1.0, 0.0), 1.0)); //0.517647
	addMaterial(r->scene, newMaterial(colorWithValues(0.3, 0.3, 0.3, 0.0), 1.0));
	addMaterial(r->scene, newMaterial(colorWithValues(0.3, 0.0, 0.0, 0.0), 1.0));
	addMaterial(r->scene, newMaterial(colorWithValues(0.0, 0.3, 0.0, 0.0), 1.0));
	addMaterial(r->scene, newMaterial(colorWithValues(0.0, 0.0, 0.3, 0.0), 0.0));
	addMaterial(r->scene, newMaterial(colorWithValues(0.9, 0.9, 0.9, 0.0), 0.0));
	addMaterial(r->scene, newMaterial(colorWithValues(1.0, 0.0, 0.0, 0.0), 0.0));
	
	//Output image prefs
	r->image = (struct outputImage*)calloc(1, sizeof(struct outputImage));
	r->image->filePath = "output/";
	r->image->fileName = "rendered";
	r->image->count = 0;
	r->image->size.width = 1280;
	r->image->size.height = 800;
	r->image->fileType = png;
	
	//Renderer prefs
	r->threadCount = 0; //Override, 0 defaults to physical core count
	r->sampleCount = 25;
	r->antialiasing = true;
	r->newRenderer = true; //New, recursive rayTracing algorighm (buggy!)
	r->tileWidth = 128;
	r->tileHeight = 128;
	r->tileOrder = renderOrderFromMiddle;
	
	//Camera prefs
	//TODO: Move camera to renderer
	r->scene->camera = (struct camera*)calloc(1, sizeof(struct camera));
	initCamera(r->scene->camera);
	r->scene->camera->isFullScreen = false;
	r->scene->camera->isBorderless = false;
	r->scene->camera-> windowScale = 1.0;
	
	r->scene->camera->         FOV = 80.0;
	r->scene->camera->    aperture = 0.0;
	r->scene->camera->    contrast = 0.5;
	r->scene->camera->bounces = 3;
	r->scene->camera->areaLights = true;
	
	//comment this block, and uncomment the next block below to toggle the detailed view of the lighting bug
	addCamTransform(r->scene->camera, newTransformTranslate(970, 480, 600)); //Set pos here
	addCamTransform(r->scene->camera, newTransformRotateX(21));//And add as many rotations as you want!
	addCamTransform(r->scene->camera, newTransformRotateZ(9)); //Don't scale or translate!
	transformCameraIntoView(r->scene->camera);
	
	//Comment above block, and uncomment this to toggle the detailed view
	/*addCamTransform(r->scene->camera, newTransformTranslate(750, 550, 1500)); //Set pos here
	addCamTransform(r->scene->camera, newTransformRotateX(21));//And add as many rotations as you want!
	addCamTransform(r->scene->camera, newTransformRotateY(90));*/
	
	r->scene->ambientColor = (struct color*)calloc(1, sizeof(struct color));
	r->scene->ambientColor->  red = 0.4;
	r->scene->ambientColor->green = 0.6;
	r->scene->ambientColor-> blue = 0.6;
	
	r->inputFilePath = "input/";
	
	//NOTE: Translates have to come last!
	if (loadOBJ(r, "newScene.obj")) {
		//Add transforms here
	}
	
	if (loadOBJ(r, "teht1.obj")) {
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformScaleUniform(40));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformRotateX(90));
		//addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformRotateY(-45));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformTranslate(870, 350, 800));
	}
	
	if (loadOBJ(r, "torus.obj")) {
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformScaleUniform(40));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformTranslate(1070, 320, 820));
	}
	
	//R G B is 0 1 2
	if (loadOBJ(r, "teapot_test.obj")) {
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformScaleUniform(80));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformRotateY(45));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformTranslate(740, 300, 900));
	}
	
	if (loadOBJ(r, "teapot_test.obj")) {
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformScaleUniform(80));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformRotateY(45));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformTranslate(740, 300, 1050));
	}
	
	if (loadOBJ(r, "teapot_test.obj")) {
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformScaleUniform(80));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformRotateY(45));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformTranslate(740, 300, 1200));
	}
	
	//White reflective 'ceramic' teapot
	if (loadOBJ(r, "teapot_white.obj")) {
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformScaleUniform(80));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformRotateY(45));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformTranslate(855, 300, 1125));
	}
	
	if (loadOBJ(r, "teapot_green.obj")) {
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformScaleUniform(80));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformRotateY(20));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformTranslate(970, 300, 900));
	}
	
	if (loadOBJ(r, "teapot_green.obj")) {
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformScaleUniform(80));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformRotateY(20));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformTranslate(970, 300, 1050));
	}
	
	if (loadOBJ(r, "teapot_green.obj")) {
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformScaleUniform(80));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformRotateY(20));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformTranslate(970, 300, 1200));
	}
	
	if (loadOBJ(r, "teapot_blue.obj")) {
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformScaleUniform(80));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformRotateY(155));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformTranslate(1210, 300,900));
	}
	
	if (loadOBJ(r, "teapot_blue.obj")) {
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformScaleUniform(80));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformRotateY(155));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformTranslate(1210, 300,1050));
	}
	
	if (loadOBJ(r, "teapot_blue.obj")) {
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformScaleUniform(80));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformRotateY(155));
		addTransform(&r->scene->objs[r->scene->objCount - 1], newTransformTranslate(1210, 300,1200));
	}
	
	//Transform all meshes
	transformMeshes(r->scene);
	//And compute the k-d trees for each mesh
	computeKDTrees(r->scene);
	
	//LIGHTS
	
	addLight(r->scene, newLight(vectorWithPos(970, 450, 500), 50, colorWithValues(2, 2, 4, 0)));
	addLight(r->scene, newLight(vectorWithPos(1210, 450,1050), 100, colorWithValues(5, 0, 0, 0)));

	/*addLight(r->scene, newLight(vectorWithPos(1160, 400, 0),    13, colorWithValues(0.2, 0.2, 0.2, 0.0)));
	addLight(r->scene, newLight(vectorWithPos(760 , 500, 0),    42, colorWithValues(0.2, 0.2, 0.2, 0.0)));
	addLight(r->scene, newLight(vectorWithPos(640 , 350, 600), 200, colorWithValues(6.0, 0.0, 0.0, 0.0)));
	addLight(r->scene, newLight(vectorWithPos(940 , 350, 600), 200, colorWithValues(0.0, 6.0, 0.0, 0.0)));
	addLight(r->scene, newLight(vectorWithPos(1240, 350, 600), 200, colorWithValues(0.0, 0.0, 6.0, 0.0)));*/
	
	addSphere(r->scene, newSphere(vectorWithPos(650, 450, 1650), 150, 5));
	addSphere(r->scene, newSphere(vectorWithPos(950, 350, 1500), 50, 6));
	addSphere(r->scene, newSphere(vectorWithPos(1100, 350, 1500), 50, 8));
	
	printSceneStats(r->scene);
	
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
