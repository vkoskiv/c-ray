//
//  scene.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015 Valtteri Koskivuori. All rights reserved.
//

#include "scene.h"

#define TOKEN_DEBUG_ENABLED false

//FIXME: Spheres and lights don't use vertex indices yet
//FIXME: lightSphere->pointLight, sphereAmount->sphereCount etc

//Prototypes
//Trims spaces and tabs from a char array
char *trimSpaces(char *inputLine);
//Parses a scene file and allocates memory accordingly
int allocMemory(scene *scene, char *inputFileName);

vector vectorFromObj(obj_vector *vec) {
	vector vector;
	vector.x = vec->e[0];
	vector.y = vec->e[1];
	vector.z = vec->e[2];
	vector.isTransformed = false;
	return vector;
}

poly polyFromObj(obj_face *face, int firstVertexIndex, int firstNormalIndex, int firstTextureIndex) {
	poly polygon;
	polygon.vertexCount = face->vertex_count;
	polygon.materialIndex = face->material_index;
	for (int i = 0; i < polygon.vertexCount; i++)
		polygon.vertexIndex[i] = firstVertexIndex + face->vertex_index[i];
	for (int i = 0; i < polygon.vertexCount; i++)
		polygon.normalIndex[i] = firstNormalIndex + face->normal_index[i];
	for (int i = 0; i < polygon.vertexCount; i++)
		polygon.textureIndex[i] = firstTextureIndex + face->texture_index[i];
	return polygon;
}

//Compute the bounding volume for a given OBJ and save it to that OBJ.
//This is used to optimize rendering, where we only loop thru all polygons
//in an OBJ if we know the ray has entered its' bounding volume, a sphere in this case
void computeBoundingVolume(crayOBJ *object) {
	vector minPoint = vertexArray[object->firstVectorIndex];
	vector maxPoint = vertexArray[object->firstVectorIndex];
	for (int i = object->firstVectorIndex + 1; i < (object->firstVectorIndex + object->vertexCount); i++) {
		minPoint = minVector(&minPoint, &vertexArray[i]);
		maxPoint = maxVector(&maxPoint, &vertexArray[i]);
	}
	vector center = vectorWithPos(0.5 * (minPoint.x + maxPoint.x), 0.5 * (minPoint.y + maxPoint.y), 0.5 * (minPoint.z + maxPoint.z));
	
	float maxDistance = 0.0;
	
	for (int i = object->firstVectorIndex + 1; i < (object->firstVectorIndex + object->vertexCount); i++) {
		vector fromCenter = subtractVectors(&vertexArray[i], &center);
		maxDistance = max(maxDistance, pow(vectorLength(&fromCenter), 2));
	}
	float sphereRadius = sqrtf(maxDistance);
	object->boundingVolume.pos = center;
	object->boundingVolume.pos.isTransformed = false;
	object->boundingVolume.radius = sphereRadius;
	
	printf("%s boundingVolume radius %f\n", object->objName, object->boundingVolume.radius);
}

char *getFileName(char *input) {
	char *fn;
	
	/* handle trailing '/' e.g.
	 input == "/home/me/myprogram/" */
	if (input[(strlen(input) - 1)] == '/')
		input[(strlen(input) - 1)] = '\0';
	
	(fn = strrchr(input, '/')) ? ++fn : (fn = input);
	
	return fn;
}

void addOBJ(scene *scene, int materialIndex, char *inputFileName) {
	printf("Loading OBJ %s\n", inputFileName);
	obj_scene_data data;
	parse_obj_scene(&data, inputFileName);
	printf("OBJ loaded, converting\n");
	
	//Create crayOBJ to keep track of objs
	scene->objs = (crayOBJ*)realloc(scene->objs, (scene->objCount + 1) * sizeof(crayOBJ));
	//Vertex data
	scene->objs[scene->objCount].firstVectorIndex = vertexCount;
	scene->objs[scene->objCount].vertexCount = data.vertex_count;
	//Normal data
	scene->objs[scene->objCount].firstNormalIndex = normalCount;
	scene->objs[scene->objCount].normalCount = data.vertex_normal_count;
	//Texture vector data
	scene->objs[scene->objCount].firstTextureIndex = textureCount;
	scene->objs[scene->objCount].textureCount = data.vertex_texture_count;
	//Poly data
	scene->objs[scene->objCount].firstPolyIndex = polyCount;
	scene->objs[scene->objCount].polyCount = data.face_count;
	//BoundingVolume init
	scene->objs[scene->objCount].boundingVolume.pos = vectorWithPos(0, 0, 0);
	scene->objs[scene->objCount].boundingVolume.radius = 0;
	//Transforms init
	scene->objs[scene->objCount].transformCount = 0;
	scene->objs[scene->objCount].transforms = (matrixTransform*)malloc(sizeof(matrixTransform));
	
	//Set name
	scene->objs[scene->objCount].objName = getFileName(inputFileName);
	
	//Update vector and poly counts
	vertexCount += data.vertex_count;
	normalCount += data.vertex_normal_count;
	textureCount += data.vertex_texture_count;
	polyCount += data.face_count;
	
	//Data loaded, now convert everything
	//Convert vectors
	printf("Converting vectors\n");
	vertexArray = (vector*)realloc(vertexArray, vertexCount * sizeof(vector));
	for (int i = 0; i < data.vertex_count; i++) {
		vertexArray[scene->objs[scene->objCount].firstVectorIndex + i] = vectorFromObj(data.vertex_list[i]);
	}
	
	//Convert normals
	printf("Converting normals\n");
	normalArray = (vector*)realloc(normalArray, normalCount * sizeof(vector));
	for (int i = 0; i < data.vertex_normal_count; i++) {
		normalArray[scene->objs[scene->objCount].firstNormalIndex + i] = vectorFromObj(data.vertex_normal_list[i]);
	}
	//Convert texture vectors
	printf("Converting texture coordinates\n");
	textureArray = (vector*)realloc(textureArray, textureCount * sizeof(vector));
	for (int i = 0; i < data.vertex_texture_count; i++) {
		textureArray[scene->objs[scene->objCount].firstTextureIndex + i] = vectorFromObj(data.vertex_texture_list[i]);
	}
	//Convert polygons
	printf("Converting polygons\n");
	polygonArray = (poly*)realloc(polygonArray, polyCount * sizeof(poly));
	for (int i = 0; i < data.face_count; i++) {
		polygonArray[scene->objs[scene->objCount].firstPolyIndex + i] = polyFromObj(data.face_list[i],
                                                                                    scene->objs[scene->objCount].firstVectorIndex,
                                                                                    scene->objs[scene->objCount].firstNormalIndex,
                                                                                    scene->objs[scene->objCount].firstTextureIndex);
		polygonArray[scene->objs[scene->objCount].firstPolyIndex + i].materialIndex = materialIndex;
	}
	
	//Delete OBJ data
	delete_obj_data(&data);
	printf("Loaded OBJ! Translated %i faces and %i vectors\n\n", data.face_count, data.vertex_count);
	
	//Obj added, update count
	scene->objCount++;
}

//In the future, maybe just pass a list and size and copy at once to save time (large counts)
void addSphere(scene *scene, sphere newSphere) {
    scene->spheres = (sphere*)realloc(scene->spheres, (scene->sphereCount + 1) * sizeof(sphere));
    scene->spheres[scene->sphereCount++] = newSphere;
}

void addMaterial(scene *scene, material newMaterial) {
    scene->materials = (material*)realloc(scene->materials, (scene->materialCount + 1) * sizeof(material));
    scene->materials[scene->materialCount++] = newMaterial;
}

void addLight(scene *scene, light newLight) {
    scene->lights = (light*)realloc(scene->lights, (scene->lightCount + 1) * sizeof(light));
    scene->lights[scene->lightCount++] = newLight;
}

void addCamera(scene *scene, camera newCamera) {
    scene->camera = (camera*)realloc(scene->camera, (scene->cameraCount + 1) * sizeof(camera));
    scene->camera[scene->cameraCount++] = newCamera;
}

void transformMeshes(scene *scene) {
	printf("Running transforms...\n");
	for (int i = 0; i < scene->objCount; ++i) {
		printf("Transforming %s...\n", scene->objs[i].objName);
		transformMesh(&scene->objs[i]);
		printf("Transformed %s!\n", scene->objs[i].objName);
	}
	printf("Transforms done!\n");
}

void computeBoundingVolumes(scene *scene) {
	printf("\nComputing bounding volumes...\n");
	for (int i = 0; i < scene->objCount; ++i) {
		computeBoundingVolume(&scene->objs[i]);
	}
	printf("\n");
}

scene *newScene() {
    scene *newScene;
    newScene = (scene*)calloc(1, sizeof(scene));
    
    return newScene;
}

int testBuild(scene *scene, char *inputFileName) {
	printf("Starting SceneBuilder V0.5\n\n");
	
	/*scene->  sphereAmount = 3;
	scene-> polygonAmount = 13;
	scene->materialAmount = 11;
	scene->   lightAmount = 6;
	scene-> objCount = 0;
	scene->customVertexCount = 23;*/
	
	scene->camera = (camera*)calloc(1, sizeof(camera));
	//Override renderer thread count, 0 defaults to physical core count
	scene->camera-> threadCount = 8;
	//General scene params
	scene->camera->       width = 1280;
	scene->camera->      height = 800;
	scene->camera->isFullScreen = false;
	scene->camera->isBorderless = false;
	scene->camera->         FOV = 80.0;
	scene->camera-> focalLength = 0;
	scene->camera-> sampleCount = 1;
	scene->camera->  frameCount = 1;
	scene->camera->     bounces = 3;
	scene->camera->    contrast = 0.7;
	scene->camera-> windowScale = 1.0;
	scene->camera->    fileType = png;
	scene->camera->  areaLights = true;
	scene->camera-> aprxShadows = false; //Approximate mesh shadows, true is faster but results in inaccurate shadows
	scene->camera->         pos = vectorWithPos(940, 480, 0);
	scene->camera->  tileWidth  = 64;
	scene->camera->  tileHeight = 64;
	scene->camera->   tileOrder = renderOrderFromMiddle;
	
	scene->ambientColor = (color*)calloc(1, sizeof(color));
	scene->ambientColor->  red = 0.4;
	scene->ambientColor->green = 0.6;
	scene->ambientColor-> blue = 0.6;
	
	//NOTE: Translates have to come last!
	addOBJ(scene, 4, "../output/monkeyHD.obj");
	addTransform(&scene->objs[scene->objCount - 1], newTransformScale(10, 10, 10));
	addTransform(&scene->objs[scene->objCount - 1], newTransformRotateY(200));
	addTransform(&scene->objs[scene->objCount - 1], newTransformRotateZ(45));
	addTransform(&scene->objs[scene->objCount - 1], newTransformTranslate(1400, 415, 1000));
	
	addOBJ(scene, 6, "../output/torus.obj");
	addTransform(&scene->objs[scene->objCount - 1], newTransformScale(90, 90, 90));
	addTransform(&scene->objs[scene->objCount - 1], newTransformRotateX(45));
	addTransform(&scene->objs[scene->objCount - 1], newTransformRotateY(105));
	addTransform(&scene->objs[scene->objCount - 1], newTransformTranslate(640, 460, 800));
	
	
	//R G B is 0 1 2
	addOBJ(scene, 0, "../output/wt_teapot.obj");
	addTransform(&scene->objs[scene->objCount - 1], newTransformScale(80, 80, 80));
	addTransform(&scene->objs[scene->objCount - 1], newTransformRotateY(45));
	addTransform(&scene->objs[scene->objCount - 1], newTransformTranslate(740, 300, 900));
	
	addOBJ(scene, 1, "../output/wt_teapot.obj");
	addTransform(&scene->objs[scene->objCount - 1], newTransformScale(80, 80, 80));
	addTransform(&scene->objs[scene->objCount - 1], newTransformRotateY(90));
	addTransform(&scene->objs[scene->objCount - 1], newTransformTranslate(970, 300, 900));
	
	addOBJ(scene, 2, "../output/wt_teapot.obj");
	addTransform(&scene->objs[scene->objCount - 1], newTransformScale(80, 80, 80));
	addTransform(&scene->objs[scene->objCount - 1], newTransformRotateY(155));
	addTransform(&scene->objs[scene->objCount - 1], newTransformTranslate(1210, 300,900));
	
	transformMeshes(scene);
	computeBoundingVolumes(scene);
	
    
    //FIXME: TEMPORARY
	vertexArray = (vector*)realloc(vertexArray, ((vertexCount+15) * sizeof(vector)));
	//Hard coded vertices for this test
	//Vertices
	//FLOOR
	vertexArray[vertexCount + 0] = vectorWithPos(200,300,0);
	vertexArray[vertexCount + 1] = vectorWithPos(1720,300,0);
	vertexArray[vertexCount + 2] = vectorWithPos(200,300,2000);
	vertexArray[vertexCount + 3] = vectorWithPos(1720,300,2000);
	//CEILING
	vertexArray[vertexCount + 4] = vectorWithPos(200,840,0);
	vertexArray[vertexCount + 5] = vectorWithPos(1720,840,0);
	vertexArray[vertexCount + 6] = vectorWithPos(200,840,2000);
	vertexArray[vertexCount + 7] = vectorWithPos(1720,840,2000);
	
	//MIRROR PLANE
	vertexArray[vertexCount + 8] = vectorWithPos(1420,750,1800);
	vertexArray[vertexCount + 9] = vectorWithPos(1420,300,1800);
	vertexArray[vertexCount + 10] = vectorWithPos(1620,750,1700);
	vertexArray[vertexCount + 11] = vectorWithPos(1620,300,1700);
	//SHADOW TEST
	vertexArray[vertexCount + 12] = vectorWithPos(1000,450,1100);
	vertexArray[vertexCount + 13] = vectorWithPos(1300,700,1100);
	vertexArray[vertexCount + 14] = vectorWithPos(1000,700,1300);
    
	//FIXME: TEMPORARY polygons
    scene->customPolyCount = 13;
    scene->customPolys = (poly*)calloc(scene->customPolyCount, sizeof(poly));
    
	//FLOOR
	scene->customPolys[0].vertexCount = MAX_VERTEX_COUNT;
	scene->customPolys[0].vertexIndex[0] = vertexCount + 0;
	scene->customPolys[0].vertexIndex[1] = vertexCount + 1;
	scene->customPolys[0].vertexIndex[2] = vertexCount + 2;
	scene->customPolys[0].materialIndex = 3;
	scene->customPolys[1].vertexCount = MAX_VERTEX_COUNT;
	scene->customPolys[1].vertexIndex[0] = vertexCount + 1;
	scene->customPolys[1].vertexIndex[1] = vertexCount + 2;
	scene->customPolys[1].vertexIndex[2] = vertexCount + 3;
	scene->customPolys[1].materialIndex = 3;
	
	//CEILING
	scene->customPolys[2].vertexCount = MAX_VERTEX_COUNT;
	scene->customPolys[2].vertexIndex[0] = vertexCount + 4;
	scene->customPolys[2].vertexIndex[1] = vertexCount + 5;
	scene->customPolys[2].vertexIndex[2] = vertexCount + 6;
	scene->customPolys[2].materialIndex = 3;
	scene->customPolys[3].vertexCount = MAX_VERTEX_COUNT;
	scene->customPolys[3].vertexIndex[0] = vertexCount + 5;
	scene->customPolys[3].vertexIndex[1] = vertexCount + 6;
	scene->customPolys[3].vertexIndex[2] = vertexCount + 7;
	scene->customPolys[3].materialIndex = 3;
	
	//MIRROR PLANE
	scene->customPolys[4].vertexCount = MAX_VERTEX_COUNT;
	scene->customPolys[4].vertexIndex[0] = vertexCount + 8;
	scene->customPolys[4].vertexIndex[1] = vertexCount + 9;
	scene->customPolys[4].vertexIndex[2] = vertexCount + 10;
	scene->customPolys[4].materialIndex = 5;
	scene->customPolys[5].vertexCount = MAX_VERTEX_COUNT;
	scene->customPolys[5].vertexIndex[0] = vertexCount + 9;
	scene->customPolys[5].vertexIndex[1] = vertexCount + 10;
	scene->customPolys[5].vertexIndex[2] = vertexCount + 11;
	scene->customPolys[5].materialIndex = 5;
	
	//SHADOW TEST POLY
	scene->customPolys[6].vertexCount = MAX_VERTEX_COUNT;
	scene->customPolys[6].vertexIndex[0] = vertexCount + 12;
	scene->customPolys[6].vertexIndex[1] = vertexCount + 13;
	scene->customPolys[6].vertexIndex[2] = vertexCount + 14;
	scene->customPolys[6].materialIndex = 4;
	
	//REAR WALL
	scene->customPolys[7].vertexCount = MAX_VERTEX_COUNT;
	scene->customPolys[7].vertexIndex[0] = vertexCount + 2;
	scene->customPolys[7].vertexIndex[1] = vertexCount + 3;
	scene->customPolys[7].vertexIndex[2] = vertexCount + 6;
	scene->customPolys[7].materialIndex = 1;
	scene->customPolys[8].vertexCount = MAX_VERTEX_COUNT;
	scene->customPolys[8].vertexIndex[0] = vertexCount + 3;
	scene->customPolys[8].vertexIndex[1] = vertexCount + 6;
	scene->customPolys[8].vertexIndex[2] = vertexCount + 7;
	scene->customPolys[8].materialIndex = 1;
	
	//LEFT WALL
	scene->customPolys[9].vertexCount = MAX_VERTEX_COUNT;
	scene->customPolys[9].vertexIndex[0] = vertexCount + 0;
	scene->customPolys[9].vertexIndex[1] = vertexCount + 2;
	scene->customPolys[9].vertexIndex[2] = vertexCount + 4;
	scene->customPolys[9].materialIndex = 0;
	scene->customPolys[10].vertexCount = MAX_VERTEX_COUNT;
	scene->customPolys[10].vertexIndex[0] = vertexCount + 2;
	scene->customPolys[10].vertexIndex[1] = vertexCount + 4;
	scene->customPolys[10].vertexIndex[2] = vertexCount + 6;
	scene->customPolys[10].materialIndex = 0;
	
	//RIGHT WALL
	scene->customPolys[11].vertexCount = MAX_VERTEX_COUNT;
	scene->customPolys[11].vertexIndex[0] = vertexCount + 1;
	scene->customPolys[11].vertexIndex[1] = vertexCount + 3;
	scene->customPolys[11].vertexIndex[2] = vertexCount + 5;
	scene->customPolys[11].materialIndex = 2;
	scene->customPolys[12].vertexCount = MAX_VERTEX_COUNT;
	scene->customPolys[12].vertexIndex[0] = vertexCount + 3;
	scene->customPolys[12].vertexIndex[1] = vertexCount + 5;
	scene->customPolys[12].vertexIndex[2] = vertexCount + 7;
	scene->customPolys[12].materialIndex = 2;
	
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
    
	//LIGHTS
    addLight(scene, newLight(vectorWithPos(1160, 400, 0),    13, colorWithValues(0.2, 0.2, 0.2, 0.0)));
    addLight(scene, newLight(vectorWithPos(760 , 500, 0),    42, colorWithValues(0.2, 0.2, 0.2, 0.0)));
    addLight(scene, newLight(vectorWithPos(640 , 350, 600), 200, colorWithValues(6.0, 0.0, 0.0, 0.0)));
    addLight(scene, newLight(vectorWithPos(940 , 350, 600), 200, colorWithValues(0.0, 6.0, 0.0, 0.0)));
    addLight(scene, newLight(vectorWithPos(1240, 350, 600), 200, colorWithValues(0.0, 0.0, 6.0, 0.0)));
	
    addSphere(scene, newSphere(vectorWithPos(650, 450, 1650), 150, 5));
    addSphere(scene, newSphere(vectorWithPos(950, 350, 1500), 50, 6));
    addSphere(scene, newSphere(vectorWithPos(1100, 350, 1500), 50, 8));
	
	if (TOKEN_DEBUG_ENABLED) {
		return 4; //Debug mode - Won't render anything
	} else {
		return 0;
	}
}

/*int allocMemory(scene *scene, char *inputFileName) {
	int materialCount = 0, lightCount = 0, polyCount = 0, sphereCount = 0, objCount = 0;
	FILE *inputFile = fopen(inputFileName, "r");
	if (!inputFile)
		return -1;
	char line[255];
	while (fgets(line, sizeof(line), inputFile) != NULL) {
		if (strcmp(trimSpaces(line), "material(){\n") == 0) {
			materialCount++;
		}
		if (strcmp(trimSpaces(line), "light(){\n") == 0) {
			lightCount++;
		}
		if (strcmp(trimSpaces(line), "sphere(){\n") == 0) {
			sphereCount++;
		}
		if (strcmp(trimSpaces(line), "poly(){\n") == 0) {
			polyCount++;
		}
		if (strcmp(trimSpaces(line), "OBJ(){\n") == 0) {
			objCount++;
		}
	}
	fclose(inputFile);
	scene->materials = (material *)calloc(materialCount, sizeof(material));
	scene->lights = (light *)calloc(lightCount, sizeof(light));
	scene->spheres = (sphere*)calloc(sphereCount, sizeof(sphere));
	//scene->polys = (poly*)calloc(polyCount, sizeof(poly));
	
	scene->materialAmount = materialCount;
	scene->lightAmount = lightCount;
	scene->sphereAmount = sphereCount;
	scene->polygonAmount = polyCount;
	scene->objCount = objCount;
	return 0;
}*/

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
