//
//  sceneloader.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "sceneloader.h"

//FIXME: We should only need to include c-ray.h here!

#include "../../libraries/cJSON.h"
#include "../../libraries/obj_parser.h"
#include "../../datatypes/scene.h"
#include "../../datatypes/vertexbuffer.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/camera.h"
#include "../../datatypes/image/hdr.h"
#include "../../datatypes/mesh.h"
#include "../../datatypes/sphere.h"
#include "../../datatypes/material.h"
#include "../../datatypes/poly.h"
#include "../../datatypes/transforms.h"
#include "../logging.h"
#include "../ui.h"
#include "../filehandler.h"
#include "../string.h"
#include "../platform/capabilities.h"
#include "../../datatypes/image/imagefile.h"
#include "../../renderer/renderer.h"
#include "../converter.h"
#include "textureloader.h"
#include "objloader.h"

static struct color parseColor(const cJSON *data);

static void addMaterialToMesh(struct mesh *mesh, struct material newMaterial);

static struct mesh *lastMesh(struct renderer *r) {
	return &r->scene->meshes[r->scene->meshCount - 1];
}

static struct sphere *lastSphere(struct renderer *r) {
	return &r->scene->spheres[r->scene->sphereCount - 1];
}

//FIXME: Do something about this awful mess.
static void loadMeshTextures(char *assetPath, struct mesh *mesh) {
	for (int i = 0; i < mesh->materialCount; ++i) {
		//FIXME: do this check in materialFromOBJ and just check against hasTexture here
		if (mesh->materials[i].textureFilePath) {
			if (strcmp(mesh->materials[i].textureFilePath, "")) {
				//TODO: Set the shader for this obj to an obnoxious checker pattern if the texture wasn't found
				char *fullPath = concatString(assetPath, mesh->materials[i].textureFilePath);
				mesh->materials[i].texture = loadTexture(fullPath);
				free(fullPath);
				if (mesh->materials[i].texture) {
					mesh->materials[i].hasTexture = true;
				} else {
					mesh->materials[i].hasTexture = false;
				}
			} else {
				mesh->materials[i].hasTexture = false;
			}
		} else {
			mesh->materials[i].hasTexture = false;
		}
		
		if (mesh->materials[i].normalMapPath) {
			if (strcmp(mesh->materials[i].normalMapPath, "")) {
				char *fullPath = concatString(assetPath, mesh->materials[i].normalMapPath);
				mesh->materials[i].normalMap = loadTexture(fullPath);
				free(fullPath);
				if (mesh->materials[i].normalMap) {
					mesh->materials[i].hasNormalMap = true;
				} else {
					mesh->materials[i].hasNormalMap = false;
				}
			} else {
				mesh->materials[i].hasNormalMap = false;
			}
		} else {
			mesh->materials[i].hasNormalMap = false;
		}
		
	}
}

static bool loadMeshNew(struct renderer *r, char *inputFilePath) {
	logr(info, "Loading OBJ %s... ", inputFilePath);
	
	bool valid = false;
	
	size_t meshCount = 0;
	struct mesh *newMeshes = parseOBJFile(inputFilePath, &meshCount);
	r->scene->meshes = realloc(r->scene->meshes, (r->scene->meshCount + meshCount) * sizeof(struct mesh));
	if (newMeshes) {
		for (size_t m = 0; m < meshCount; ++m) {
			r->scene->meshes[r->scene->meshCount + m] = newMeshes[m];
			free(&newMeshes[m]);
			valid = true;
			loadMeshTextures(r->prefs.assetPath, &r->scene->meshes[r->scene->meshCount + m]);
		}
	}
	
	logr(info, "Found %zu meshes\n", meshCount);
	
	r->scene->meshCount += meshCount;
	return valid;
}

static bool loadMesh(struct renderer *r, char *inputFilePath, int idx, int meshCount) {
	printf("\r");
	logr(info, "Loading mesh %i/%i%s", idx, meshCount, idx == meshCount ? "\n" : "\r");
	
	obj_scene_data data;
	if (parse_obj_scene(&data, inputFilePath) == 0) {
		if (idx != meshCount) printf("\n");
		logr(warning, "Mesh \"%s\" not found!\n", getFileName(inputFilePath));
		return false;
	}
	
	//Create mesh to keep track of meshes
	r->scene->meshes = realloc(r->scene->meshes, (r->scene->meshCount + 1) * sizeof(struct mesh));
	struct mesh *newMesh = &r->scene->meshes[r->scene->meshCount];
	//Vertex data
	newMesh->firstVectorIndex = vertexCount;
	newMesh->vertexCount = data.vertex_count;
	//Normal data
	newMesh->firstNormalIndex = normalCount;
	newMesh->normalCount = data.vertex_normal_count;
	//Texture vector data
	newMesh->firstTextureIndex = textureCount;
	newMesh->textureCount = data.vertex_texture_count;
	//Poly data
	newMesh->polyCount = data.face_count;
	//Transforms init
	newMesh->transformCount = 0;
	
	newMesh->materialCount = 0;
	//Set name
	newMesh->name = copyString(getFileName(inputFilePath));
	
	//Update vector and poly counts
	vertexCount += data.vertex_count;
	normalCount += data.vertex_normal_count;
	textureCount += data.vertex_texture_count;
	
	//Data loaded, now convert everything
	//Convert vectors
	vertexArray = realloc(vertexArray, vertexCount * sizeof(struct vector));
	for (int i = 0; i < data.vertex_count; ++i) {
		vertexArray[newMesh->firstVectorIndex + i] = vectorFromObj(data.vertex_list[i]);
	}
	
	//Convert normals
	normalArray = realloc(normalArray, normalCount * sizeof(struct vector));
	for (int i = 0; i < data.vertex_normal_count; ++i) {
		normalArray[newMesh->firstNormalIndex + i] = vectorFromObj(data.vertex_normal_list[i]);
	}
	//Convert texture vectors
	textureArray = realloc(textureArray, textureCount * sizeof(struct coord));
	for (int i = 0; i < data.vertex_texture_count; ++i) {
		textureArray[newMesh->firstTextureIndex + i] = coordFromObj(data.vertex_texture_list[i]);
	}
	//Convert polygons
	newMesh->polygons = malloc(data.face_count * sizeof(struct poly));
	for (int i = 0; i < data.face_count; ++i) {
		newMesh->polygons[i] = polyFromObj(data.face_list[i],
										   newMesh->firstVectorIndex,
										   newMesh->firstNormalIndex,
										   newMesh->firstTextureIndex);
	}
	
	newMesh->materials = calloc(1, sizeof(*newMesh->materials));
	//Parse materials
	if (data.material_count == 0) {
		//No material, set to something obscene to make it noticeable
		newMesh->materials = calloc(1, sizeof(*newMesh->materials));
		newMesh->materials[0] = warningMaterial();
		assignBSDF(&newMesh->materials[0]);
		newMesh->materialCount++;
	} else {
		//Loop to add materials to mesh (We already set the material indices in polyFromObj)
		for (int i = 0; i < data.material_count; ++i) {
			addMaterialToMesh(newMesh, materialFromObj(data.material_list[i]));
		}
	}
	
	//Load textures for meshes
	loadMeshTextures(getFilePath(inputFilePath), newMesh);
	
	//Delete OBJ data
	delete_obj_data(&data);
	
	//Mesh added, update count
	r->scene->meshCount++;
	return true;
}

static void addMaterialToMesh(struct mesh *mesh, struct material newMaterial) {
	mesh->materials = realloc(mesh->materials, (mesh->materialCount + 1) * sizeof(struct material));
	mesh->materials[mesh->materialCount++] = newMaterial;
}

//FIXME: change + 1 to ++scene->someCount and just pass the count to array access
//In the future, maybe just pass a list and size and copy at once to save time (large counts)
static void addSphere(struct world *scene, struct sphere newSphere) {
	scene->spheres = realloc(scene->spheres, (scene->sphereCount + 1) * sizeof(struct sphere));
	scene->spheres[scene->sphereCount++] = newSphere;
}

static void addCamTransform(struct camera *cam, struct transform transform) {
	if (cam->transformCount == 0) {
		cam->transforms = calloc(1, sizeof(*cam->transforms));
	} else {
		cam->transforms = realloc(cam->transforms, (cam->transformCount + 1) * sizeof(struct transform));
	}
	
	cam->transforms[cam->transformCount] = transform;
	cam->transformCount++;
}

static void addCamTransforms(struct camera *cam, struct transform *transforms, int count) {
	for (int i = 0; i < count; ++i) {
		addCamTransform(cam, transforms[i]);
	}
}

static struct material *parseMaterial(const cJSON *data) {
	cJSON *bsdf = NULL;
	cJSON *IOR = NULL;
	//cJSON *roughness = NULL;
	cJSON *color = NULL;
	cJSON *intensity = NULL;
	bool validIOR = false;
	//bool validRoughness = false;
	bool validIntensity = false;
	
	float IORValue = 1.0f;
	//float roughnessValue;
	struct color colorValue = blackColor;
	float intensityValue = 1.0f;
	
	struct material *mat = calloc(1, sizeof(*mat));
	
	color = cJSON_GetObjectItem(data, "color");
	if (cJSON_IsObject(color)) {
		colorValue = parseColor(color);
	} else {
		logr(warning, "No color given\n");
		logr(warning, "Material data: %s\n", cJSON_Print(data));
	}
	
	intensity = cJSON_GetObjectItem(data, "intensity");
	if (cJSON_IsNumber(intensity)) {
		validIntensity = true;
		if (intensity->valuedouble >= 0) {
			intensityValue = intensity->valuedouble;
		} else {
			intensityValue = 1.0f;
		}
	}
	
	IOR = cJSON_GetObjectItem(data, "IOR");
	if (cJSON_IsNumber(IOR)) {
		validIOR = true;
		if (IOR->valuedouble >= 0) {
			IORValue = IOR->valuedouble;
		} else {
			IORValue = 1.0f;
		}
	}
	
	bsdf = cJSON_GetObjectItem(data, "bsdf");
	if (!cJSON_IsString(bsdf)) {
		logr(warning, "No bsdf type given for material.");
		logr(error, "Material data: %s\n", cJSON_Print(data));
	}
	
	if (strcmp(bsdf->valuestring, "lambertian") == 0) {
		mat->type = lambertian;
		mat->diffuse = colorValue;
	} else if (strcmp(bsdf->valuestring, "metal") == 0) {
		mat->type = metal;
		mat->diffuse = colorValue;
	} else if (strcmp(bsdf->valuestring, "glass") == 0) {
		mat->type = glass;
		mat->diffuse = colorValue;
		if (validIOR) {
			mat->IOR = IORValue;
		} else {
			logr(warning, "Glass shader defined, but no IOR given\n");
			logr(error, "Material data: %s\n", cJSON_Print(data));
		}
	} else if (strcmp(bsdf->valuestring, "emission") == 0) {
		mat->type = emission;
		mat->diffuse = colorValue;
		if (validIntensity) {
			mat->emission = colorCoef(intensityValue, mat->emission);
		} else {
			logr(warning, "Emission shader defined, but no intensity given\n");
			logr(error, "Material data: %s\n", cJSON_Print(data));
		}
	}
	assignBSDF(mat);
	return mat;
}

static struct transform parseTransform(const cJSON *data, char *targetName) {
	cJSON *type = cJSON_GetObjectItem(data, "type");
	if (!cJSON_IsString(type)) {
		logr(warning, "Failed to parse transform! No type found\n");
		logr(warning, "Transform data: %s\n", cJSON_Print(data));
	}
	
	cJSON *degrees = NULL;
	cJSON *radians = NULL;
	cJSON *scale = NULL;
	cJSON *X = NULL;
	cJSON *Y = NULL;
	cJSON *Z = NULL;
	
	bool validDegrees = false;
	bool validRadians = false;
	bool validScale = false;
	
	degrees = cJSON_GetObjectItem(data, "degrees");
	radians = cJSON_GetObjectItem(data, "radians");
	scale = cJSON_GetObjectItem(data, "scale");
	X = cJSON_GetObjectItem(data, "X");
	Y = cJSON_GetObjectItem(data, "Y");
	Z = cJSON_GetObjectItem(data, "Z");
	
	if (degrees != NULL && cJSON_IsNumber(degrees)) {
		validDegrees = true;
	}
	if (radians != NULL && cJSON_IsNumber(radians)) {
		validRadians = true;
	}
	if (scale != NULL && cJSON_IsNumber(scale)) {
		validScale = true;
	}
	
	//For translate, we want the default to be 0. For scaling, def should be 1
	float def = 0.0f;
	if (strcmp(type->valuestring, "scale") == 0) {
		def = 1.0f;
	}
	
	int validCoords = 0; //Accept if we have at least one provided
	float Xval, Yval, Zval;
	if (X != NULL && cJSON_IsNumber(X)) {
		Xval = X->valuedouble;
		validCoords++;
	} else {
		Xval = def;
	}
	if (Y != NULL && cJSON_IsNumber(Y)) {
		Yval = Y->valuedouble;
		validCoords++;
	} else {
		Yval = def;
	}
	if (Z != NULL && cJSON_IsNumber(Z)) {
		Zval = Z->valuedouble;
		validCoords++;
	} else {
		Zval = def;
	}
	
	if (strcmp(type->valuestring, "rotateX") == 0) {
		if (validDegrees) {
			return newTransformRotateX(toRadians(degrees->valuedouble));
		} else if (validRadians) {
			return newTransformRotateX(radians->valuedouble);
		} else {
			logr(warning, "Found rotateX transform for object \"%s\" with no valid degrees or radians value given.\n", targetName);
		}
	} else if (strcmp(type->valuestring, "rotateY") == 0) {
		if (validDegrees) {
			return newTransformRotateY(toRadians(degrees->valuedouble));
		} else if (validRadians) {
			return newTransformRotateY(radians->valuedouble);
		} else {
			logr(warning, "Found rotateY transform for object \"%s\" with no valid degrees or radians value given.\n", targetName);
		}
	} else if (strcmp(type->valuestring, "rotateZ") == 0) {
		if (validDegrees) {
			return newTransformRotateZ(toRadians(degrees->valuedouble));
		} else if (validRadians) {
			return newTransformRotateZ(radians->valuedouble);
		} else {
			logr(warning, "Found rotateZ transform for object \"%s\" with no valid degrees or radians value given.\n", targetName);
		}
	} else if (strcmp(type->valuestring, "translate") == 0) {
		if (validCoords > 0) {
			return newTransformTranslate(Xval, Yval, Zval);
		} else {
			logr(warning, "Found translate transform for object \"%s\" with less than 1 valid coordinate given.\n", targetName);
		}
	} else if (strcmp(type->valuestring, "scale") == 0) {
		if (validCoords > 0) {
			return newTransformScale(Xval, Yval, Zval);
		} else {
			logr(warning, "Found scale transform for object \"%s\" with less than 1 valid scale value given.\n", targetName);
		}
	} else if (strcmp(type->valuestring, "scaleUniform") == 0) {
		if (validScale) {
			return newTransformScaleUniform(scale->valuedouble);
		} else {
			logr(warning, "Found scaleUniform transform for object \"%s\" with no valid scale value given.\n", targetName);
		}
	} else {
		logr(warning, "Found an invalid transform \"%s\" for object \"%s\"\n", type->valuestring, targetName);
	}
	
	//Hack. This is essentially just a NOP transform that does nothing.
	return newTransformTranslate(0.0f, 0.0f, 0.0f);
}

//Parse JSON array of transforms, and return a pointer to an array of corresponding transforms
static struct transform *parseTransforms(const cJSON *data) {
	
	int transformCount = cJSON_GetArraySize(data);
	struct transform *transforms = calloc(transformCount, sizeof(*transforms));
	
	cJSON *transform = NULL;
	
	for (int i = 0; i < transformCount; ++i) {
		transform = cJSON_GetArrayItem(data, i);
		transforms[i] = parseTransform(transform, "camera");
	}
	return transforms;
}

static struct prefs defaultPrefs() {
	char* imgFilePath;
	char* imgFileName;
	imgFilePath = copyString("./");
	imgFileName = copyString("rendered");
	return (struct prefs){
		.tileOrder = renderOrderFromMiddle,
		.threadCount = getSysCores(), //We run getSysCores() for this
		.sampleCount = 25,
		.bounces = 20,
		.tileWidth = 32,
		.tileHeight = 32,
		.antialiasing = true,
		.imgFilePath = imgFilePath,
		.imgFileName = imgFileName,
		.imgCount = 0,
		.imageWidth = 1280,
		.imageHeight = 800,
		.imgType = png,
		.fullscreen = false,
		.borderless = false,
		.scale = 1.0f
	};
}

static struct prefs parsePrefs(const cJSON *data) {
	struct prefs p = defaultPrefs();
	
	if (!data) return p;
	
	const cJSON *threads = NULL;
	const cJSON *samples = NULL;
	const cJSON *antialiasing = NULL;
	const cJSON *tileWidth = NULL;
	const cJSON *tileHeight = NULL;
	const cJSON *tileOrder = NULL;
	const cJSON *bounces = NULL;
	const cJSON *filePath = NULL;
	const cJSON *fileName = NULL;
	const cJSON *count = NULL;
	const cJSON *width = NULL;
	const cJSON *height = NULL;
	const cJSON *fileType = NULL;
	
	threads = cJSON_GetObjectItem(data, "threads");
	if (threads) {
		if (cJSON_IsNumber(threads)) {
			if (threads->valueint > 0) {
				p.threadCount = threads->valueint;
				p.fromSystem = false;
			} else {
				p.threadCount = getSysCores() + 2;
				p.fromSystem = true;
			}
		} else {
			logr(warning, "Invalid threads while parsing renderer\n");
		}
	} else {
		p.threadCount = getSysCores() + 2;
		p.fromSystem = true;
	}
	
	samples = cJSON_GetObjectItem(data, "samples");
	if (samples) {
		if (cJSON_IsNumber(samples)) {
			if (samples->valueint >= 1) {
				p.sampleCount = samples->valueint;
			} else {
				p.sampleCount = 1;
			}
		} else {
			logr(warning, "Invalid samples while parsing renderer\n");
		}
	} else {
		p.sampleCount = defaultPrefs().sampleCount;
	}
	
	
	bounces = cJSON_GetObjectItem(data, "bounces");
	if (bounces) {
		if (cJSON_IsNumber(bounces)) {
			if (bounces->valueint >= 0) {
				p.bounces = bounces->valueint;
			} else {
				p.bounces = 1;
			}
		} else {
			logr(warning, "Invalid bounces while parsing renderer\n");
		}
	} else {
		p.bounces = defaultPrefs().bounces;
	}
	
	antialiasing = cJSON_GetObjectItem(data, "antialiasing");
	if (antialiasing) {
		if (cJSON_IsBool(antialiasing)) {
			p.antialiasing = cJSON_IsTrue(antialiasing);
		} else {
			logr(warning, "Invalid antialiasing bool while parsing renderer\n");
		}
	} else {
		p.antialiasing = defaultPrefs().antialiasing;
	}
	
	tileWidth = cJSON_GetObjectItem(data, "tileWidth");
	if (tileWidth) {
		if (cJSON_IsNumber(tileWidth)) {
			if (tileWidth->valueint >= 1) {
				p.tileWidth = tileWidth->valueint;
			} else {
				p.tileWidth = 1;
			}
		} else {
			logr(warning, "Invalid tileWidth while parsing renderer\n");
		}
	} else {
		p.tileWidth = defaultPrefs().tileWidth;
	}
	
	tileHeight = cJSON_GetObjectItem(data, "tileHeight");
	if (tileHeight) {
		if (cJSON_IsNumber(tileHeight)) {
			if (tileHeight->valueint >= 1) {
				p.tileHeight = tileHeight->valueint;
			} else {
				p.tileHeight = 1;
			}
		} else {
			logr(warning, "Invalid tileHeight while parsing renderer\n");
		}
	} else {
		p.tileHeight = defaultPrefs().tileHeight;
	}
	
	tileOrder = cJSON_GetObjectItem(data, "tileOrder");
	if (tileOrder) {
		if (cJSON_IsString(tileOrder)) {
			if (strcmp(tileOrder->valuestring, "random") == 0) {
				p.tileOrder = renderOrderRandom;
			} else if (strcmp(tileOrder->valuestring, "topToBottom") == 0) {
				p.tileOrder = renderOrderTopToBottom;
			} else if (strcmp(tileOrder->valuestring, "fromMiddle") == 0) {
				p.tileOrder = renderOrderFromMiddle;
			} else if (strcmp(tileOrder->valuestring, "toMiddle") == 0) {
				p.tileOrder = renderOrderToMiddle;
			} else {
				p.tileOrder = renderOrderNormal;
			}
		} else {
			logr(warning, "Invalid tileOrder while parsing renderer\n");
		}
	} else {
		p.tileOrder = defaultPrefs().tileOrder;
	}
	
	filePath = cJSON_GetObjectItem(data, "outputFilePath");
	if (filePath) {
		if (cJSON_IsString(filePath)) {
			p.imgFilePath = copyString(filePath->valuestring);
		} else {
			logr(warning, "Invalid filePath while parsing scene.\n");
		}
	} else {
		p.imgFilePath = copyString(defaultPrefs().imgFilePath);
	}
	
	fileName = cJSON_GetObjectItem(data, "outputFileName");
	if (fileName) {
		if (cJSON_IsString(fileName)) {
			p.imgFileName = copyString(fileName->valuestring);
		} else {
			logr(warning, "Invalid fileName while parsing scene.\n");
		}
	} else {
		p.imgFileName = copyString(defaultPrefs().imgFileName);
	}
	
	count = cJSON_GetObjectItem(data, "count");
	if (count) {
		if (cJSON_IsNumber(count)) {
			if (count->valueint >= 0) {
				p.imgCount = count->valueint;
			} else {
				p.imgCount = 0;
			}
		} else {
			logr(warning, "Invalid count while parsing scene.\n");
		}
	} else {
		p.imgCount = defaultPrefs().imgCount;
	}
	
	width = cJSON_GetObjectItem(data, "width");
	if (width) {
		if (cJSON_IsNumber(width)) {
			if (width->valueint >= 0) {
				p.imageWidth = width->valueint;
			} else {
				p.imageWidth = 640;
			}
		} else {
			logr(warning, "Invalid width while parsing scene.\n");
		}
	} else {
		p.imageWidth = defaultPrefs().imageWidth;
	}
	
	height = cJSON_GetObjectItem(data, "height");
	if (height) {
		if (cJSON_IsNumber(height)) {
			if (height->valueint >= 0) {
				p.imageHeight = height->valueint;
			} else {
				p.imageHeight = 400;
			}
		} else {
			logr(warning, "Invalid height while parsing scene.\n");
		}
	} else {
		p.imageHeight = defaultPrefs().imageHeight;
	}
	
	fileType = cJSON_GetObjectItem(data, "fileType");
	if (fileType) {
		if (cJSON_IsString(fileType)) {
			if (strcmp(fileType->valuestring, "bmp") == 0) {
				p.imgType = bmp;
			} else {
				p.imgType = png;
			}
		} else {
			logr(warning, "Invalid fileType while parsing scene.\n");
		}
	} else {
		p.imgType = defaultPrefs().imgType;
	}
	
	return p;
}

static int parseDisplay(struct prefs *prefs, const cJSON *data) {
	if (!data) {
		prefs->enabled = true;
		prefs->fullscreen = false;
		prefs->borderless = false;
		prefs->scale = 1.0f;
	}
	const cJSON *enabled = NULL;
	const cJSON *isFullscreen = NULL;
	const cJSON *isBorderless = NULL;
	const cJSON *windowScale = NULL;
	
	enabled = cJSON_GetObjectItem(data, "enabled");
	if (enabled) {
		if (cJSON_IsBool(enabled)) {
			prefs->enabled = cJSON_IsTrue(enabled);
		} else {
			logr(warning, "Invalid enabled while parsing display prefs.\n");
			return -1;
		}
	} else {
		prefs->enabled = false;
	}
	
	isFullscreen = cJSON_GetObjectItem(data, "isFullscreen");
	if (isFullscreen) {
		if (cJSON_IsBool(isFullscreen)) {
			prefs->fullscreen = cJSON_IsTrue(isFullscreen);
		} else {
			logr(warning, "Invalid isFullscreen while parsing display prefs.\n");
			return -1;
		}
	} else {
		prefs->fullscreen = false;
	}
	
	isBorderless = cJSON_GetObjectItem(data, "isBorderless");
	if (isBorderless) {
		if (cJSON_IsBool(isBorderless)) {
			prefs->borderless = cJSON_IsTrue(isBorderless);
		} else {
			logr(warning, "Invalid isBorderless while parsing display prefs.\n");
			return -1;
		}
	} else {
		prefs->borderless = false;
	}
	
	windowScale = cJSON_GetObjectItem(data, "windowScale");
	if (windowScale) {
		if (cJSON_IsNumber(windowScale)) {
			if (windowScale->valuedouble >= 0) {
				prefs->scale = windowScale->valuedouble;
			} else {
				prefs->scale = 1.0f;
			}
		} else {
			logr(warning, "Invalid isBorderless while parsing display prefs.\n");
			return -1;
		}
	} else {
		prefs->scale = 1.0f;
	}
	
	return 0;
}

static struct camera defaultCamera() {
	return (struct camera){
		.FOV = 80.0f,
		.focalDistance = 10.0f,
		.aperture = 0.0f
	};
}

static int parseCamera(struct camera *c, const cJSON *data) {
	if (!data) *c = defaultCamera();
	const cJSON *FOV = NULL;
	const cJSON *focalDistance = NULL;
	const cJSON *aperture = NULL;
	const cJSON *transforms = NULL;
	
	FOV = cJSON_GetObjectItem(data, "FOV");
	if (FOV) {
		if (cJSON_IsNumber(FOV)) {
			if (FOV->valuedouble >= 0.0) {
				if (FOV->valuedouble > 180.0) {
					c->FOV = 180.0f;
				} else {
					c->FOV = FOV->valuedouble;
				}
			} else {
				c->FOV = 80.0f;
			}
		} else {
			logr(warning, "Invalid FOV value while parsing camera.\n");
			return -1;
		}
	} else {
		c->FOV = defaultCamera().FOV;
	}
	
	focalDistance = cJSON_GetObjectItem(data, "focalDistance");
	if (focalDistance) {
		if (cJSON_IsNumber(focalDistance)) {
			if (focalDistance->valuedouble >= 0.0) {
				c->focalDistance = focalDistance->valuedouble;
			} else {
				c->focalDistance = 0.0f;
			}
		} else {
			logr(warning, "Invalid focalDistance while parsing camera.\n");
			return -1;
		}
	} else {
		c->focalDistance = defaultCamera().focalDistance;
	}
	
	aperture = cJSON_GetObjectItem(data, "fstops");
	if (aperture) {
		if (cJSON_IsNumber(aperture)) {
			if (aperture->valuedouble >= 0.0) {
				c->fstops = aperture->valuedouble;
			} else {
				c->fstops = 0.0f;
			}
		} else {
			logr(warning, "Invalid aperture while parsing camera.\n");
			return -1;
		}
	} else {
		c->aperture = defaultCamera().aperture;
	}
	
	transforms = cJSON_GetObjectItem(data, "transforms");
	if (transforms) {
		if (cJSON_IsArray(transforms)) {
			int tformCount = cJSON_GetArraySize(transforms);
			struct transform *tforms = parseTransforms(transforms);
			addCamTransforms(c, tforms, tformCount);
			free(tforms);
		} else {
			logr(warning, "Invalid transforms while parsing camera.\n");
			return -1;
		}
	} else {
		initCamera(c);
	}
	
	return 0;
}

static struct color parseColor(const cJSON *data) {
	
	const cJSON *R = NULL;
	const cJSON *G = NULL;
	const cJSON *B = NULL;
	const cJSON *A = NULL;
	const cJSON *kelvin = NULL;
	
	struct color newColor;
	
	kelvin = cJSON_GetObjectItem(data, "blackbody");
	if (kelvin && cJSON_IsNumber(kelvin)) {
		newColor = colorForKelvin(kelvin->valuedouble);
		return newColor;
	}
	
	R = cJSON_GetObjectItem(data, "r");
	if (R != NULL && cJSON_IsNumber(R)) {
		newColor.red = R->valuedouble;
	} else {
		newColor.red = 0.0f;
	}
	G = cJSON_GetObjectItem(data, "g");
	if (R != NULL && cJSON_IsNumber(G)) {
		newColor.green = G->valuedouble;
	} else {
		newColor.green = 0.0f;
	}
	B = cJSON_GetObjectItem(data, "b");
	if (R != NULL && cJSON_IsNumber(B)) {
		newColor.blue = B->valuedouble;
	} else {
		newColor.blue = 0.0f;
	}
	A = cJSON_GetObjectItem(data, "a");
	if (R != NULL && cJSON_IsNumber(A)) {
		newColor.alpha = A->valuedouble;
	} else {
		newColor.alpha = 0.0f;
	}
	
	return newColor;
}

//FIXME:
static int parseAmbientColor(struct renderer *r, const cJSON *data) {
	const cJSON *down = NULL;
	const cJSON *up = NULL;
	const cJSON *hdr = NULL;
	const cJSON *offset = NULL;
	
	struct gradient newGradient;
	
	down = cJSON_GetObjectItem(data, "down");
	up = cJSON_GetObjectItem(data, "up");
	
	newGradient.down = parseColor(down);
	newGradient.up = parseColor(up);
	
	r->scene->ambientColor = newGradient;
	
	hdr = cJSON_GetObjectItem(data, "hdr");
	if (cJSON_IsString(hdr)) {
		char *fullPath = concatString(r->prefs.assetPath, hdr->valuestring);
		r->scene->hdr = loadHDRI(fullPath);
		free(fullPath);
	}
	
	offset = cJSON_GetObjectItem(data, "offset");
	if (cJSON_IsNumber(offset)) {
		if (r->scene->hdr) {
			r->scene->hdr->offset = toRadians(offset->valuedouble) / 4.0f;
		}
	}
	
	return 0;
}

//FIXME: Only parse everything else if the mesh is found and is valid
static void parseMesh(struct renderer *r, const cJSON *data, int idx, int meshCount) {
	const cJSON *fileName = cJSON_GetObjectItem(data, "fileName");
	
	const cJSON *bsdf = cJSON_GetObjectItem(data, "bsdf");
	const cJSON *intensity = cJSON_GetObjectItem(data, "intensity");
	const cJSON *roughness = cJSON_GetObjectItem(data, "roughness");
	enum bsdfType type = lambertian;
	
	//TODO: Wrap this into a function
	if (cJSON_IsString(bsdf)) {
		if (strcmp(bsdf->valuestring, "metal") == 0) {
			type = metal;
		} else if (strcmp(bsdf->valuestring, "glass") == 0) {
			type = glass;
		} else if (strcmp(bsdf->valuestring, "plastic") == 0) {
			type = plastic;
		} else if (strcmp(bsdf->valuestring, "emissive") == 0) {
			type = emission;
		} else {
			type = lambertian;
		}
	} else {
		logr(warning, "Invalid bsdf while parsing mesh\n");
	}
	
	bool meshValid = false;
	if (fileName != NULL && cJSON_IsString(fileName)) {
		char *fullPath = concatString(r->prefs.assetPath, fileName->valuestring);
		if (loadMesh(r, fullPath, idx, meshCount)) {
			meshValid = true;
			free(fullPath);
		} else {
			free(fullPath);
			return;
		}
	}
	if (meshValid) {
		const cJSON *transforms = cJSON_GetObjectItem(data, "transforms");
		const cJSON *transform = NULL;
		//TODO: Use parseTransforms for this
		if (transforms != NULL && cJSON_IsArray(transforms)) {
			cJSON_ArrayForEach(transform, transforms) {
				addTransform(lastMesh(r), parseTransform(transform, lastMesh(r)->name));
			}
		}
		
		//FIXME: this isn't right.
		for (int i = 0; i < lastMesh(r)->materialCount; ++i) {
			lastMesh(r)->materials[i].type = type;
			if (type == emission) {
				lastMesh(r)->materials[i].emission = colorCoef(intensity->valuedouble, lastMesh(r)->materials[i].diffuse);
			}
			if (cJSON_IsNumber(roughness)) lastMesh(r)->materials[i].roughness = roughness->valuedouble;
			assignBSDF(&lastMesh(r)->materials[i]);
		}
	}
}

static void parseMeshes(struct renderer *r, const cJSON *data) {
	const cJSON *mesh = NULL;
	int idx = 1;
	int meshCount = cJSON_GetArraySize(data);
	if (data != NULL && cJSON_IsArray(data)) {
		cJSON_ArrayForEach(mesh, data) {
			parseMesh(r, mesh, idx, meshCount);
			idx++;
		}
	}
}

static struct vector parseCoordinate(const cJSON *data) {
	const cJSON *X = NULL;
	const cJSON *Y = NULL;
	const cJSON *Z = NULL;
	X = cJSON_GetObjectItem(data, "X");
	Y = cJSON_GetObjectItem(data, "Y");
	Z = cJSON_GetObjectItem(data, "Z");
	
	if (X != NULL && Y != NULL && Z != NULL) {
		if (cJSON_IsNumber(X) && cJSON_IsNumber(Y) && cJSON_IsNumber(Z)) {
			return vecWithPos(X->valuedouble, Y->valuedouble, Z->valuedouble);
		}
	}
	logr(warning, "Invalid coordinate parsed! Returning 0,0,0\n");
	logr(warning, "Faulty JSON: %s\n", cJSON_Print(data));
	return (struct vector){0.0f,0.0f,0.0f};
}

static void parseSphere(struct renderer *r, const cJSON *data) {
	const cJSON *pos = NULL;
	const cJSON *color = NULL;
	const cJSON *roughness = NULL;
	const cJSON *IOR = NULL;
	const cJSON *radius = NULL;
	const cJSON *intensity = NULL;
	
	struct sphere newSphere = defaultSphere();
	
	const cJSON *bsdf = cJSON_GetObjectItem(data, "bsdf");
	
	//TODO: Break this out to a function
	if (cJSON_IsString(bsdf)) {
		if (strcmp(bsdf->valuestring, "lambertian") == 0) {
			newSphere.material.type = lambertian;
		} else if (strcmp(bsdf->valuestring, "metal") == 0) {
			newSphere.material.type = metal;
		} else if (strcmp(bsdf->valuestring, "glass") == 0) {
			newSphere.material.type = glass;
		} else if (strcmp(bsdf->valuestring, "plastic") == 0) {
			newSphere.material.type = plastic;
		} else if (strcmp(bsdf->valuestring, "emissive") == 0) {
			newSphere.material.type = emission;
		}
	} else {
		logr(warning, "Sphere BSDF not found, defaulting to lambertian.\n");
	}
	
	pos = cJSON_GetObjectItem(data, "pos");
	if (pos != NULL) {
		newSphere.pos = parseCoordinate(pos);
	} else {
		logr(warning, "No position specified for sphere\n");
	}
	
	color = cJSON_GetObjectItem(data, "color");
	if (color != NULL) {
		switch (newSphere.material.type) {
			case emission:
				newSphere.material.emission = parseColor(color);
				break;
				
			default:
				newSphere.material.ambient = parseColor(color);
				newSphere.material.diffuse = parseColor(color);
				break;
		}
	} else {
		logr(warning, "No color specified for sphere\n");
	}
	
	//FIXME: Another hack.
	intensity = cJSON_GetObjectItem(data, "intensity");
	if (intensity != NULL) {
		if (cJSON_IsNumber(intensity) && (newSphere.material.type == emission)) {
			newSphere.material.emission = colorCoef(intensity->valuedouble, newSphere.material.emission);
		}
	}
	
	roughness = cJSON_GetObjectItem(data, "roughness");
	if (roughness != NULL && cJSON_IsNumber(roughness)) {
		newSphere.material.roughness = roughness->valuedouble;
	} else {
		newSphere.material.roughness = 0.0f;
	}
	
	IOR = cJSON_GetObjectItem(data, "IOR");
	if (IOR != NULL && cJSON_IsNumber(IOR)) {
		newSphere.material.IOR = IOR->valuedouble;
	} else {
		newSphere.material.IOR = 1.0f;
	}
	
	radius = cJSON_GetObjectItem(data, "radius");
	if (radius != NULL && cJSON_IsNumber(radius)) {
		newSphere.radius = radius->valuedouble;
	} else {
		newSphere.radius = 10.0f;
		logr(warning, "No radius specified for sphere, setting to %.0f\n", newSphere.radius);
	}
	
	//FIXME: Proper materials for spheres
	addSphere(r->scene, newSphere);
	assignBSDF(&lastSphere(r)->material);
}

static void parsePrimitive(struct renderer *r, const cJSON *data, int idx) {
	const cJSON *type = NULL;
	type = cJSON_GetObjectItem(data, "type");
	if (strcmp(type->valuestring, "sphere") == 0) {
		parseSphere(r, data);
	} else {
		logr(warning, "Unknown primitive type \"%s\" at index %i\n", type->valuestring, idx);
	}
}

static void parsePrimitives(struct renderer *r, const cJSON *data) {
	const cJSON *primitive = NULL;
	if (data != NULL && cJSON_IsArray(data)) {
		int i = 0;
		cJSON_ArrayForEach(primitive, data) {
			parsePrimitive(r, primitive, i);
			i++;
		}
	}
}

static int initializeParser(char *input) {
	cJSON *json = cJSON_Parse(input);
	if (!json) {
		logr(warning, "Failed to parse JSON\n");
		const char *err = cJSON_GetErrorPtr();
		if (err) {
			cJSON_Delete(json);
			logr(warning, "Error before: %s\n", err);
			return -1;
		}
	}
	return 0;
}

static int parseScene(struct renderer *r, const cJSON *data) {
	
	const cJSON *ambientColor = NULL;
	const cJSON *primitives = NULL;
	const cJSON *meshes = NULL;
	
	ambientColor = cJSON_GetObjectItem(data, "ambientColor");
	if (ambientColor) {
		if (cJSON_IsObject(ambientColor)) {
			if (parseAmbientColor(r, ambientColor) == -1) {
				logr(warning, "Invalid ambientColor while parsing scene.\n");
				return -1;
			}
		}
	} else {
		r->scene->ambientColor = (struct gradient){
			.down = (struct color){0.4f, 0.4f, 0.5f, 0.0f},
			.up   = (struct color){0.8f, 0.8f, 1.0f, 0.0f}
		};
	}
	
	primitives = cJSON_GetObjectItem(data, "primitives");
	if (primitives) {
		if (cJSON_IsArray(primitives)) {
			parsePrimitives(r, primitives);
		}
	}
	
	meshes = cJSON_GetObjectItem(data, "meshes");
	if (meshes) {
		if (cJSON_IsArray(meshes)) {
			parseMeshes(r, meshes);
		}
	}
	
	return 0;
}

int parseJSON(struct renderer *r, char *input) {
	
	/*
	 Note: Since we are freeing the JSON data (and its' pointers) after parsing,
	 we need to *copy* all dynamically allocated strings with the copyString() function.
	 */
	cJSON *json = cJSON_Parse(input);
	if (json == NULL) {
		logr(warning, "Failed to parse JSON\n");
		const char *errptr = cJSON_GetErrorPtr();
		if (errptr != NULL) {
			cJSON_Delete(json);
			logr(warning, "Error before: %s\n", errptr);
			return -2;
		}
	}
	
	const cJSON *renderer = NULL;
	const cJSON *display = NULL;
	const cJSON *camera = NULL;
	const cJSON *scene = NULL;
	
	renderer = cJSON_GetObjectItem(json, "renderer");
	
	//FIXME: Hack
	char *assetPath = r->prefs.assetPath;
	r->prefs = parsePrefs(renderer);
	r->prefs.assetPath = assetPath;
	
	//FIXME: Another hack.
	display = cJSON_GetObjectItem(json, "display");
	if (parseDisplay(&r->prefs, display) == -1) {
		logr(warning, "Display parse failed!\n");
		return -2;
	}
	
	camera = cJSON_GetObjectItem(json, "camera");
	if (parseCamera(r->scene->camera, camera) == -1) {
		logr(warning, "Camera parse failed!\n");
		return -2;
	}
	
	scene = cJSON_GetObjectItem(json, "scene");
	if (parseScene(r, scene) == -1) {
		logr(warning, "Scene parse failed!\n");
		return -2;
	}
	
	cJSON_Delete(json);
	
	return 0;
}
