//
//  scene.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 28/02/15.
//  Copyright (c) 2015-2018 Valtteri Koskivuori. All rights reserved.
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
#include "cJSON.h"

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

struct crayOBJ *lastObj(struct renderer *r) {
	return &r->scene->objs[r->scene->objCount - 1];
}

bool loadOBJ(struct renderer *r, char *inputFileName) {
	printf("Loading OBJ %s%s\n", r->inputFilePath, inputFileName);
	
	obj_scene_data data;
	if (parse_obj_scene(&data, inputFileName, r->inputFilePath) == 0) {
		printf("OBJ %s file not found!\n", getFileName(inputFileName));
		return false;
	}
	printf("OBJ loaded, converting...\n");
	
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
	vertexArray = (struct vector*)realloc(vertexArray, vertexCount * sizeof(struct vector));
	for (int i = 0; i < data.vertex_count; i++) {
		vertexArray[r->scene->objs[r->scene->objCount].firstVectorIndex + i] = vectorFromObj(data.vertex_list[i]);
	}
	
	//Convert normals
	normalArray = (struct vector*)realloc(normalArray, normalCount * sizeof(struct vector));
	for (int i = 0; i < data.vertex_normal_count; i++) {
		normalArray[r->scene->objs[r->scene->objCount].firstNormalIndex + i] = vectorFromObj(data.vertex_normal_list[i]);
	}
	//Convert texture vectors
	textureArray = (struct vector*)realloc(textureArray, textureCount * sizeof(struct vector));
	for (int i = 0; i < data.vertex_texture_count; i++) {
		textureArray[r->scene->objs[r->scene->objCount].firstTextureIndex + i] = vectorFromObj(data.vertex_texture_list[i]);
	}
	//Convert polygons
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
	
	
	printf("Converted OBJ! Translated %i faces, %i vectors and %i materials\n\n", data.face_count, data.vertex_count, data.material_count);
	
	//Delete OBJ data
	delete_obj_data(&data);
	
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

void addCamTransform(struct camera *cam, struct matrixTransform transform) {
	if (cam->transformCount == 0) {
		cam->transforms = (struct matrixTransform*)calloc(1, sizeof(struct matrixTransform));
	} else {
		cam->transforms = (struct matrixTransform*)realloc(cam->transforms, (cam->transformCount + 1) * sizeof(struct matrixTransform));
	}
	
	cam->transforms[cam->transformCount] = transform;
	cam->transformCount++;
}

void addCamTransforms(struct camera *cam, struct matrixTransform *transforms, int count) {
	printf("Adding %i transforms to camera\n", count);
	for (int i = 0; i < count; i++) {
		addCamTransform(cam, transforms[i]);
	}
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

char *loadFile(char *inputFileName) {
	FILE *f = fopen(inputFileName, "rb");
	if (!f) {
		printf("No file found at %s", inputFileName);
		return NULL;
	}
	char *buf = NULL;
	size_t len;
	ssize_t bytesRead = getdelim(&buf, &len, '\0', f);
	if (bytesRead != -1) {
		printf("%zi bytes of input JSON loaded, parsing...\n", bytesRead);
	} else {
		printf("Failed to read input JSON from %s", inputFileName);
		return NULL;
	}
	return buf;
}

struct matrixTransform parseTransform(const cJSON *data) {
	
	cJSON *type = cJSON_GetObjectItem(data, "type");
	if (!cJSON_IsString(type)) {
		printf("Failed to parse transform! No type found\n");
		printf("Transform data: %s\n", cJSON_Print(data));
	}
	
	cJSON *degrees = NULL;
	cJSON *scale = NULL;
	cJSON *X = NULL;
	cJSON *Y = NULL;
	cJSON *Z = NULL;
	
	bool validDegrees = false;
	bool validScale = false;
	bool validCoords = false;
	
	degrees = cJSON_GetObjectItem(data, "degrees");
	scale = cJSON_GetObjectItem(data, "scale");
	X = cJSON_GetObjectItem(data, "X");
	Y = cJSON_GetObjectItem(data, "Y");
	Z = cJSON_GetObjectItem(data, "Z");
	
	if (degrees != NULL && cJSON_IsNumber(degrees)) {
		validDegrees = true;
	}
	if (scale != NULL && cJSON_IsNumber(scale)) {
		validScale = true;
	}
	if (X != NULL && Y != NULL && Z != NULL) {
		if (cJSON_IsNumber(X) && cJSON_IsNumber(Y) && cJSON_IsNumber(Z)) {
			validCoords = true;
		}
	}
	
	if (strcmp(type->valuestring, "rotateX") == 0) {
		if (validDegrees) {
			return newTransformRotateX(degrees->valuedouble);
		} else {
			printf("Found rotateX transform with no valid degrees value given.\n");
		}
	} else if (strcmp(type->valuestring, "rotateY") == 0) {
		if (validDegrees) {
			return newTransformRotateY(degrees->valuedouble);
		} else {
			printf("Found rotateY transform with no valid degrees value given.\n");
		}
	} else if (strcmp(type->valuestring, "rotateZ") == 0) {
		if (validDegrees) {
			return newTransformRotateZ(degrees->valuedouble);
		} else {
			printf("Found rotateZ transform with no valid degrees value given.\n");
		}
	} else if (strcmp(type->valuestring, "translate") == 0) {
		if (validCoords) {
			return newTransformTranslate(X->valuedouble, Y->valuedouble, Z->valuedouble);
		} else {
			printf("Found translate transform with no valid coords given.\n");
		}
	} else if (strcmp(type->valuestring, "scale") == 0) {
		if (validCoords) {
			return newTransformScale(X->valuedouble, Y->valuedouble, Z->valuedouble);
		} else {
			printf("Found scale transform with no valid scale value given.\n");
		}
	} else if (strcmp(type->valuestring, "scaleUniform") == 0) {
		if (validScale) {
			return newTransformScaleUniform(scale->valuedouble);
		} else {
			printf("Found scaleUniform transform with no valid scale value given.\n");
		}
	} else {
		printf("Found an invalid transform\n");
	}
	
	return (struct matrixTransform){};
}

//Parse JSON array of transforms, and return a pointer to an array of corresponding matrixTransforms
struct matrixTransform *parseTransforms(const cJSON *data) {
	
	int transformCount = cJSON_GetArraySize(data);
	struct matrixTransform *transforms = (struct matrixTransform*)calloc(transformCount, sizeof(struct matrixTransform));
	
	cJSON *transform = NULL;
	
	for (int i = 0; i < transformCount; i++) {
		transform = cJSON_GetArrayItem(data, i);
		transforms[i] = parseTransform(transform);
	}
	return transforms;
}

int parseRenderer(struct renderer *r, const cJSON *data) {
	const cJSON *threadCount = NULL;
	const cJSON *sampleCount = NULL;
	const cJSON *antialiasing = NULL;
	const cJSON *newRenderer = NULL;
	const cJSON *tileWidth = NULL;
	const cJSON *tileHeight = NULL;
	const cJSON *tileOrder = NULL;
	
	threadCount = cJSON_GetObjectItem(data, "threadCount");
	if (cJSON_IsNumber(threadCount)) {
		if (threadCount->valueint >= 0) {
			r->threadCount = threadCount->valueint;
		} else {
			r->threadCount = 0;
		}
	} else {
		printf("Invalid threadCount while parsing renderer\n");
		return -1;
	}
	
	sampleCount = cJSON_GetObjectItem(data, "sampleCount");
	if (cJSON_IsNumber(sampleCount)) {
		if (sampleCount->valueint >= 1) {
			r->sampleCount = sampleCount->valueint;
		} else {
			r->sampleCount = 1;
		}
	} else {
		printf("Invalid sampleCount while parsing renderer\n");
		return -1;
	}
	
	antialiasing = cJSON_GetObjectItem(data, "antialiasing");
	if (cJSON_IsBool(antialiasing)) {
		r->antialiasing = cJSON_IsTrue(antialiasing);
	} else {
		printf("Invalid antialiasing bool while parsing renderer\n");
		return -1;
	}
	
	newRenderer = cJSON_GetObjectItem(data, "newRenderer");
	if (cJSON_IsBool(newRenderer)) {
		r->newRenderer = cJSON_IsTrue(newRenderer);
	} else {
		printf("Invalid newRenderer bool while parsing renderer\n");
		return -1;
	}
	
	tileWidth = cJSON_GetObjectItem(data, "tileWidth");
	if (cJSON_IsNumber(tileWidth)) {
		if (tileWidth->valueint >= 1) {
			r->tileWidth = tileWidth->valueint;
		} else {
			r->tileWidth = 1;
		}
	} else {
		printf("Invalid tileWidth while parsing renderer\n");
		return -1;
	}
	
	tileHeight = cJSON_GetObjectItem(data, "tileHeight");
	if (cJSON_IsNumber(tileHeight)) {
		if (tileHeight->valueint >= 1) {
			r->tileHeight = tileHeight->valueint;
		} else {
			r->tileHeight = 1;
		}
	} else {
		printf("Invalid tileHeight while parsing renderer\n");
		return -1;
	}
	
	tileOrder = cJSON_GetObjectItem(data, "tileOrder");
	if (cJSON_IsString(tileOrder)) {
		if (strcmp(tileOrder->valuestring, "normal") == 0) {
			r->tileOrder = renderOrderNormal;
		} else if (strcmp(tileOrder->valuestring, "random") == 0) {
			r->tileOrder = renderOrderRandom;
		} else if (strcmp(tileOrder->valuestring, "topToBottom") == 0) {
			r->tileOrder = renderOrderTopToBottom;
		} else if (strcmp(tileOrder->valuestring, "fromMiddle") == 0) {
			r->tileOrder = renderOrderFromMiddle;
		} else if (strcmp(tileOrder->valuestring, "toMiddle") == 0) {
			r->tileOrder = renderOrderToMiddle;
		} else {
			r->tileOrder = renderOrderNormal;
		}
	} else {
		printf("Invalid tileOrder while parsing renderer\n");
		return -1;
	}
	
	return 0;
}

int parseCamera(struct renderer *r, const cJSON *data) {
	
	const cJSON *isFullscreen = NULL;
	const cJSON *isBorderless = NULL;
	const cJSON *windowScale = NULL;
	const cJSON *FOV = NULL;
	const cJSON *aperture = NULL;
	const cJSON *contrast = NULL;
	const cJSON *bounces = NULL;
	const cJSON *areaLights = NULL;
	const cJSON *transforms = NULL;
	
	isFullscreen = cJSON_GetObjectItem(data, "isFullscreen");
	if (cJSON_IsBool(isFullscreen)) {
		r->scene->camera->isFullScreen = cJSON_IsTrue(isFullscreen);
	}
	
	isBorderless = cJSON_GetObjectItem(data, "isBorderless");
	if (cJSON_IsBool(isBorderless)) {
		r->scene->camera->isBorderless = cJSON_IsTrue(isBorderless);
	}
	
	windowScale = cJSON_GetObjectItem(data, "windowScale");
	if (cJSON_IsNumber(windowScale)) {
		if (windowScale->valuedouble >= 0) {
			r->scene->camera->windowScale = windowScale->valuedouble;
		} else {
			r->scene->camera->windowScale = 0.5;
		}
	}
	
	FOV = cJSON_GetObjectItem(data, "FOV");
	if (cJSON_IsNumber(FOV)) {
		if (FOV->valuedouble >= 0.0) {
			r->scene->camera->FOV = FOV->valuedouble;
		} else {
			r->scene->camera->FOV = 80.0;
		}
	}
	
	aperture = cJSON_GetObjectItem(data, "aperture");
	if (cJSON_IsNumber(aperture)) {
		if (aperture->valuedouble >= 0.0) {
			r->scene->camera->aperture = aperture->valuedouble;
		} else {
			r->scene->camera->aperture = 0.0;
		}
	}
	
	contrast = cJSON_GetObjectItem(data, "contrast");
	if (cJSON_IsNumber(contrast)) {
		if (contrast->valuedouble >= 0.0) {
			r->scene->camera->contrast = contrast->valuedouble;
		} else {
			r->scene->camera->contrast = 0.5;
		}
	}
	
	bounces = cJSON_GetObjectItem(data, "bounces");
	if (cJSON_IsNumber(bounces)) {
		if (bounces->valueint >= 1) {
			r->scene->camera->bounces = bounces->valueint;
		} else {
			r->scene->camera->bounces = 1;
		}
	}
	
	areaLights = cJSON_GetObjectItem(data, "areaLights");
	if (cJSON_IsBool(areaLights)) {
		r->scene->camera->areaLights = cJSON_IsTrue(areaLights);
	}
	
	transforms = cJSON_GetObjectItem(data, "transforms");
	if (cJSON_IsArray(transforms)) {
		int tformCount = cJSON_GetArraySize(transforms);
		addCamTransforms(r->scene->camera, parseTransforms(transforms), tformCount);
	}
	
	return 0;
}

struct color *parseColor(const cJSON *data) {
	
	const cJSON *R = NULL;
	const cJSON *G = NULL;
	const cJSON *B = NULL;
	const cJSON *A = NULL;
	
	struct color *newColor = (struct color*)calloc(1, sizeof(struct color));
	
	R = cJSON_GetObjectItem(data, "r");
	if (R != NULL && cJSON_IsNumber(R)) {
		newColor->red = R->valuedouble;
	} else {
		return NULL;
	}
	G = cJSON_GetObjectItem(data, "g");
	if (R != NULL && cJSON_IsNumber(G)) {
		newColor->green = G->valuedouble;
	} else {
		return NULL;
	}
	B = cJSON_GetObjectItem(data, "b");
	if (R != NULL && cJSON_IsNumber(B)) {
		newColor->blue = B->valuedouble;
	} else {
		return NULL;
	}
	A = cJSON_GetObjectItem(data, "a");
	if (R != NULL && cJSON_IsNumber(A)) {
		newColor->alpha = A->valuedouble;
	} else {
		newColor->alpha = 0.0;
	}
	
	return newColor;
}

int parseAmbientColor(struct renderer *r, const cJSON *data) {
	struct color *ambientColor = parseColor(data);
	if (ambientColor != NULL) {
		r->scene->ambientColor = ambientColor;
	} else {
		return -1;
	}
	return 0;
}

void parseOBJ(struct renderer *r, const cJSON *data) {
	const cJSON *fileName = cJSON_GetObjectItem(data, "fileName");
	bool objValid = false;
	if (fileName != NULL && cJSON_IsString(fileName)) {
		if (loadOBJ(r, fileName->valuestring)) {
			objValid = true;
		} else {
			printf("Failed to find OBJ named %s\n", fileName->valuestring);
			return;
		}
	}
	if (objValid) {
		const cJSON *transforms = cJSON_GetObjectItem(data, "transforms");
		const cJSON *transform = NULL;
		if (transforms != NULL && cJSON_IsArray(transforms)) {
			cJSON_ArrayForEach(transform, transforms) {
				addTransform(lastObj(r), parseTransform(transform));
			}
		}
	}
}

void parseOBJs(struct renderer *r, const cJSON *data) {
	const cJSON *OBJ = NULL;
	if (data != NULL && cJSON_IsArray(data)) {
		cJSON_ArrayForEach(OBJ, data) {
			parseOBJ(r, OBJ);
		}
	}
}

int parseLights(struct renderer *r, const cJSON *data) {
	//TODO
	return -1;
}

int parseScene(struct renderer *r, const cJSON *data) {
	
	const cJSON *filePath = NULL;
	const cJSON *inputFilePath = NULL;
	const cJSON *count = NULL;
	const cJSON *width = NULL;
	const cJSON *height = NULL;
	const cJSON *fileType = NULL;
	const cJSON *ambientColor = NULL;
	const cJSON *lights = NULL;
	const cJSON *spheres = NULL;
	const cJSON *OBJs = NULL;
	
	filePath = cJSON_GetObjectItem(data, "filePath");
	if (cJSON_IsString(filePath)) {
		r->image->filePath = filePath->valuestring;
	}
	
	inputFilePath = cJSON_GetObjectItem(data, "inputFilePath");
	if (cJSON_IsString(inputFilePath)) {
		r->inputFilePath = inputFilePath->valuestring;
	}
	
	count = cJSON_GetObjectItem(data, "count");
	if (cJSON_IsNumber(count)) {
		if (count->valueint >= 0) {
			r->image->count = count->valueint;
		} else {
			r->image->count = 0;
		}
	}
	
	width = cJSON_GetObjectItem(data, "width");
	if (cJSON_IsNumber(width)) {
		if (width->valueint >= 0) {
			r->image->size.width = width->valueint;
		} else {
			r->image->size.width = 640;
		}
	}
	
	height = cJSON_GetObjectItem(data, "height");
	if (cJSON_IsNumber(height)) {
		if (height->valueint >= 0) {
			r->image->size.height = height->valueint;
		} else {
			r->image->size.height = 640;
		}
	}
	
	fileType = cJSON_GetObjectItem(data, "fileType");
	if (cJSON_IsString(fileType)) {
		if (strcmp(fileType->valuestring, "png") == 0) {
			r->image->fileType = png;
		} else if (strcmp(fileType->valuestring, "bmp") == 0) {
			r->image->fileType = bmp;
		} else {
			r->image->fileType = png;
		}
	}
	
	ambientColor = cJSON_GetObjectItem(data, "ambientColor");
	if (cJSON_IsObject(ambientColor)) {
		if (parseAmbientColor(r, ambientColor) == -1) {
			return -1;
		}
	}
	
	/*lights = cJSON_GetObjectItem(data, "lights");
	if (cJSON_IsArray(lights)) {
		if (parseLights(r, lights) == -1) {
			return -1;
		}
	}*/
	
	spheres = cJSON_GetObjectItem(data, "spheres");
	if (cJSON_IsArray(spheres)) {
		//TODO
	}
	
	OBJs = cJSON_GetObjectItem(data, "OBJs");
	if (cJSON_IsArray(OBJs)) {
		parseOBJs(r, OBJs);
	}
	
	return 0;
}

int parseJSON(struct renderer *r, char *inputFileName) {
	
	/*
	 TODO:
	 	LIGHTS, SPHERES
	 	Check and fix camera transforms order
	 */
	
	//Allocate dynamic props
	r->image = (struct outputImage*)calloc(1, sizeof(struct outputImage));
	r->scene->camera = (struct camera*)calloc(1, sizeof(struct camera));
	r->scene->ambientColor = (struct color*)calloc(1, sizeof(struct color));
	
	char *buf = loadFile(inputFileName);
	
	cJSON *json = cJSON_Parse(buf);
	if (json == NULL) {
		printf("Failed to parse JSON\n");
		const char *errptr = cJSON_GetErrorPtr();
		if (errptr != NULL) {
			printf("Error before: %s\n", errptr);
		}
		return -1;
	}
	
	const cJSON *renderer = NULL;
	const cJSON *camera = NULL;
	const cJSON *scene = NULL;
	
	renderer = cJSON_GetObjectItemCaseSensitive(json, "renderer");
	if (renderer != NULL) {
		printf("Parsing renderer prefs...\n");
		if (parseRenderer(r, renderer) == -1) {
			printf("Renderer parse failed!\n");
			return -2;
		}
	}
	
	camera = cJSON_GetObjectItem(json, "camera");
	if (camera != NULL) {
		printf("Parsing camera prefs...\n");
		if (parseCamera(r, camera) == -1) {
			printf("Camera parse failed!\n");
			return -2;
		}
	}
	
	scene = cJSON_GetObjectItem(json, "scene");
	if (scene != NULL) {
		printf("Parsing scene...\n");
		if (parseScene(r, scene) == -1) {
			printf("Scene parse failed!\n");
			return -2;
		}
	}
	
	cJSON_Delete(json);
	
	transformMeshes(r->scene);
	computeKDTrees(r->scene);
	
	printSceneStats(r->scene);
	
	return -2;
}

int testBuild(struct renderer *r, char *inputFileName) {
	printf("Starting SceneBuilder V0.5\n\n");
	
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
		addTransform(lastObj(r), newTransformScaleUniform(40));
		addTransform(lastObj(r), newTransformRotateX(90));
		//addTransform(lastObj(r), newTransformRotateY(-45));
		addTransform(lastObj(r), newTransformTranslate(870, 350, 800));
	}
	
	if (loadOBJ(r, "torus.obj")) {
		addTransform(lastObj(r), newTransformScaleUniform(40));
		addTransform(lastObj(r), newTransformTranslate(1070, 320, 820));
	}
	
	//R G B is 0 1 2
	if (loadOBJ(r, "teapot_test.obj")) {
		addTransform(lastObj(r), newTransformScaleUniform(80));
		addTransform(lastObj(r), newTransformRotateY(45));
		addTransform(lastObj(r), newTransformTranslate(740, 300, 900));
	}
	
	if (loadOBJ(r, "teapot_test.obj")) {
		addTransform(lastObj(r), newTransformScaleUniform(80));
		addTransform(lastObj(r), newTransformRotateY(45));
		addTransform(lastObj(r), newTransformTranslate(740, 300, 1050));
	}
	
	if (loadOBJ(r, "teapot_test.obj")) {
		addTransform(lastObj(r), newTransformScaleUniform(80));
		addTransform(lastObj(r), newTransformRotateY(45));
		addTransform(lastObj(r), newTransformTranslate(740, 300, 1200));
	}
	
	//White reflective 'ceramic' teapot
	if (loadOBJ(r, "teapot_white.obj")) {
		addTransform(lastObj(r), newTransformScaleUniform(80));
		addTransform(lastObj(r), newTransformRotateY(45));
		addTransform(lastObj(r), newTransformTranslate(855, 300, 1125));
	}
	
	if (loadOBJ(r, "teapot_green.obj")) {
		addTransform(lastObj(r), newTransformScaleUniform(80));
		addTransform(lastObj(r), newTransformRotateY(20));
		addTransform(lastObj(r), newTransformTranslate(970, 300, 900));
	}
	
	if (loadOBJ(r, "teapot_green.obj")) {
		addTransform(lastObj(r), newTransformScaleUniform(80));
		addTransform(lastObj(r), newTransformRotateY(20));
		addTransform(lastObj(r), newTransformTranslate(970, 300, 1050));
	}
	
	if (loadOBJ(r, "teapot_green.obj")) {
		addTransform(lastObj(r), newTransformScaleUniform(80));
		addTransform(lastObj(r), newTransformRotateY(20));
		addTransform(lastObj(r), newTransformTranslate(970, 300, 1200));
	}
	
	if (loadOBJ(r, "teapot_blue.obj")) {
		addTransform(lastObj(r), newTransformScaleUniform(80));
		addTransform(lastObj(r), newTransformRotateY(155));
		addTransform(lastObj(r), newTransformTranslate(1210, 300, 900));
	}
	
	if (loadOBJ(r, "teapot_blue.obj")) {
		addTransform(lastObj(r), newTransformScaleUniform(80));
		addTransform(lastObj(r), newTransformRotateY(155));
		addTransform(lastObj(r), newTransformTranslate(1210, 300,1050));
	}
	
	if (loadOBJ(r, "teapot_blue.obj")) {
		addTransform(lastObj(r), newTransformScaleUniform(80));
		addTransform(lastObj(r), newTransformRotateY(155));
		addTransform(lastObj(r), newTransformTranslate(1210, 300,1200));
	}
	
	//Transform all meshes
	transformMeshes(r->scene);
	//And compute the k-d trees for each mesh
	computeKDTrees(r->scene);
	
	//LIGHTS
	
	addLight(r->scene, newLight(vectorWithPos(970, 450, 500), 50, colorWithValues(2, 2, 4, 0)));
	addLight(r->scene, newLight(vectorWithPos(1210, 450,1050), 100, colorWithValues(5, 0, 0, 0)));
	
	addSphere(r->scene, newSphere(vectorWithPos(650, 450, 1650), 150, newMaterial(colorWithValues(0.3, 0.3, 0.3, 0.0), 1.0)));
	addSphere(r->scene, newSphere(vectorWithPos(950, 350, 1500), 50, newMaterial(colorWithValues(0.3, 0.3, 0.3, 0.0), 1.0)));
	addSphere(r->scene, newSphere(vectorWithPos(1100, 350, 1500), 50, newMaterial(colorWithValues(0.0, 0.0, 0.3, 0.0), 0.0)));
	
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
