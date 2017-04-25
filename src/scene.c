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

material *materialFromObj(obj_material *mat) {
    material *newMat = (material*)calloc(1, sizeof(material));
    newMat->diffuse.red   = mat->diff[0];
    newMat->diffuse.green = mat->diff[1];
    newMat->diffuse.blue  = mat->diff[2];
    newMat->reflectivity  = mat->reflect;
    return newMat;
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

void addOBJ(scene *sceneData, char *inputFileName) {
	printf("Loading OBJ %s\n", inputFileName);
	obj_scene_data data;
    if (parse_obj_scene(&data, inputFileName) == 0) {
        printf("OBJ %s file not found!\n", getFileName(inputFileName));
        return;
    }
	printf("OBJ loaded, converting\n");
	
	//Create crayOBJ to keep track of objs
	sceneData->objs = (crayOBJ*)realloc(sceneData->objs, (sceneData->objCount + 1) * sizeof(crayOBJ));
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
	//BoundingVolume init
	sceneData->objs[sceneData->objCount].boundingVolume.pos = vectorWithPos(0, 0, 0);
	sceneData->objs[sceneData->objCount].boundingVolume.radius = 0;
	//Transforms init
	sceneData->objs[sceneData->objCount].transformCount = 0;
	sceneData->objs[sceneData->objCount].transforms = (matrixTransform*)malloc(sizeof(matrixTransform));
	
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
	vertexArray = (vector*)realloc(vertexArray, vertexCount * sizeof(vector));
	for (int i = 0; i < data.vertex_count; i++) {
		vertexArray[sceneData->objs[sceneData->objCount].firstVectorIndex + i] = vectorFromObj(data.vertex_list[i]);
	}
	
	//Convert normals
	printf("Converting normals\n");
	normalArray = (vector*)realloc(normalArray, normalCount * sizeof(vector));
	for (int i = 0; i < data.vertex_normal_count; i++) {
		normalArray[sceneData->objs[sceneData->objCount].firstNormalIndex + i] = vectorFromObj(data.vertex_normal_list[i]);
	}
	//Convert texture vectors
	printf("Converting texture coordinates\n");
	textureArray = (vector*)realloc(textureArray, textureCount * sizeof(vector));
	for (int i = 0; i < data.vertex_texture_count; i++) {
		textureArray[sceneData->objs[sceneData->objCount].firstTextureIndex + i] = vectorFromObj(data.vertex_texture_list[i]);
	}
	//Convert polygons
	printf("Converting polygons\n");
	polygonArray = (poly*)realloc(polygonArray, polyCount * sizeof(poly));
	for (int i = 0; i < data.face_count; i++) {
		polygonArray[sceneData->objs[sceneData->objCount].firstPolyIndex + i] = polyFromObj(data.face_list[i],
                                                                                    sceneData->objs[sceneData->objCount].firstVectorIndex,
                                                                                    sceneData->objs[sceneData->objCount].firstNormalIndex,
                                                                                    sceneData->objs[sceneData->objCount].firstTextureIndex);
		//polygonArray[sceneData->objs[sceneData->objCount].firstPolyIndex + i].materialIndex = materialIndex;
	}
    
    //Parse materials
    if (data.material_count == 0) {
        //No material, set to something obscene to make it noticeable
        sceneData->objs[sceneData->objCount].material = (material*)calloc(1, sizeof(material));
        *sceneData->objs[sceneData->objCount].material = newMaterial(colorWithValues(255.0/255.0, 20.0/255.0, 147.0/255.0, 0), 0);
    } else {
        //Material found, set it
        sceneData->objs[sceneData->objCount].material = (material*)calloc(1, sizeof(material));
        sceneData->objs[sceneData->objCount].material = materialFromObj(data.material_list[0]);
    }
	
	//Delete OBJ data
	delete_obj_data(&data);
	printf("Loaded OBJ! Translated %i faces and %i vectors\n\n", data.face_count, data.vertex_count);
	
	//Obj added, update count
	sceneData->objCount++;
}

//FIXME: Temporary
void overrideMaterial(scene *world, crayOBJ *obj, int materialIndex) {
    obj->material = &world->materials[materialIndex];
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

void addCamera(scene *scene, camera *newCamera) {
    scene->camera = (camera*)realloc(scene->camera, (scene->cameraCount + 1) * sizeof(camera));
    scene->camera[scene->cameraCount++] = *newCamera;
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

//FIXME: Move this to transforms.c
void addCamTransform(scene *world, matrixTransform transform) {
	if (world->camTransformCount == 0) {
		world->camTransforms = (matrixTransform*)calloc(1, sizeof(matrixTransform));
	} else {
		world->camTransforms = (matrixTransform*)realloc(world->camTransforms, (world->camTransformCount + 1) * sizeof(matrixTransform));
	}
	
	world->camTransforms[world->camTransformCount] = transform;
	world->camTransformCount++;
}

int testBuild(scene *scene, char *inputFileName) {
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
	
    camera *cam = (camera*)calloc(1, sizeof(camera));
	//Override renderer thread count, 0 defaults to physical core count
	cam-> threadCount = 8;
	cam->       width = 1280;
	cam->      height = 800;
	cam->isFullScreen = false;
	cam->isBorderless = false;
	cam->         FOV = 80.0;
	cam-> focalLength = 0;
	cam-> sampleCount = 20;
	cam->  frameCount = 1;
	cam->     bounces = 3;
	cam->    contrast = 0.7;
	cam-> windowScale = 1.0;
	cam->    fileType = png;
	cam->  areaLights = true;
	cam-> aprxShadows = false; //Approximate mesh shadows, true is faster but results in inaccurate shadows
	cam->  tileWidth  = 64;
	cam->  tileHeight = 64;
	cam->   tileOrder = renderOrderFromMiddle;
	cam->pos = vectorWithPos(0, 0, 0); //Don't change
	
	addCamTransform(scene, newTransformTranslate(940, 480, 0)); //Set pos here
	addCamTransform(scene, newTransformRotateZ(14));//And add as many rotations as you want!
	addCamTransform(scene, newTransformRotateZ(-14)); //Don't scale or translate!
	
	scene->ambientColor = (color*)calloc(1, sizeof(color));
	scene->ambientColor->  red = 0.4;
	scene->ambientColor->green = 0.6;
	scene->ambientColor-> blue = 0.6;
    
    addCamera(scene, cam);
	
	//NOTE: Translates have to come last!
	addOBJ(scene, "../output/monkeyHD.obj");
	addTransform(&scene->objs[scene->objCount - 1], newTransformScale(10, 10, 10));
	addTransform(&scene->objs[scene->objCount - 1], newTransformRotateY(200));
	addTransform(&scene->objs[scene->objCount - 1], newTransformRotateZ(45));
	addTransform(&scene->objs[scene->objCount - 1], newTransformTranslate(1400, 415, 1000));
	
	addOBJ(scene, "../output/torus.obj");
	addTransform(&scene->objs[scene->objCount - 1], newTransformScale(90, 90, 90));
	addTransform(&scene->objs[scene->objCount - 1], newTransformRotateX(45));
	addTransform(&scene->objs[scene->objCount - 1], newTransformRotateY(105));
	addTransform(&scene->objs[scene->objCount - 1], newTransformTranslate(640, 460, 800));
	
	
	//R G B is 0 1 2
	addOBJ(scene, "../output/wt_teapot.obj");
    overrideMaterial(scene, &scene->objs[2], 0);
	addTransform(&scene->objs[scene->objCount - 1], newTransformScale(80, 80, 80));
	addTransform(&scene->objs[scene->objCount - 1], newTransformRotateY(45));
	addTransform(&scene->objs[scene->objCount - 1], newTransformTranslate(740, 300, 900));
	
	addOBJ(scene, "../output/wt_teapot.obj");
    overrideMaterial(scene, &scene->objs[3], 1);
	addTransform(&scene->objs[scene->objCount - 1], newTransformScale(80, 80, 80));
	addTransform(&scene->objs[scene->objCount - 1], newTransformRotateY(90));
	addTransform(&scene->objs[scene->objCount - 1], newTransformTranslate(970, 300, 900));
	
	addOBJ(scene, "../output/wt_teapot.obj");
    overrideMaterial(scene, &scene->objs[4], 2);
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
