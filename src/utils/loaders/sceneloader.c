//
//  sceneloader.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2015-2019 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "sceneloader.h"

#include "../../libraries/cJSON.h"
#include "../../libraries/obj_parser.h"
#include "../../datatypes/scene.h"
#include "../../datatypes/vertexbuffer.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/camera.h"
#include "../../datatypes/texture.h"
#include "../../utils/logging.h"
#include "../../utils/ui.h"
#include "../../utils/filehandler.h"
#include "../../utils/multiplatform.h"
#include "../../renderer/renderer.h"
#include "../../acceleration/kdtree.h"
#include "../converter.h"
#include "textureloader.h"
#include "objloader.h"

struct color parseColor(const cJSON *data);

void addMaterialToMesh(struct mesh *mesh, struct material newMaterial);

struct mesh *lastMesh(struct renderer *r) {
	return &r->scene->meshes[r->scene->meshCount - 1];
}

struct sphere *lastSphere(struct renderer *r) {
	return &r->scene->spheres[r->scene->sphereCount - 1];
}

void loadMeshTextures(struct mesh *mesh) {
	for (int i = 0; i < mesh->materialCount; i++) {
		//FIXME: do this check in materialFromOBJ and just check against hasTexture here
		if (mesh->materials[i].textureFilePath) {
			if (strcmp(mesh->materials[i].textureFilePath, "")) {
				//TODO: Set the shader for this obj to an obnoxious checker pattern if the texture wasn't found
				mesh->materials[i].texture = loadTexture(mesh->materials[i].textureFilePath);
				if (mesh->materials[i].texture) {
					mesh->materials[i].hasTexture = true;
				}
			} else {
				mesh->materials[i].hasTexture = false;
			}
		} else {
			mesh->materials[i].hasTexture = false;
		}
		
	}
}

bool loadMeshNew(struct renderer *r, char *inputFilePath) {
	logr(info, "Loading mesh %s\n", inputFilePath);
	
	r->scene->meshes = realloc(r->scene->meshes, (r->scene->meshCount + 1) * sizeof(struct mesh));
	
	bool valid = false;
	
	struct mesh *newMesh = parseOBJFile(inputFilePath);
	if (newMesh != NULL) {
		r->scene->meshes[r->scene->meshCount] = *newMesh;
		free(newMesh);
		valid = true;
		loadMeshTextures(&r->scene->meshes[r->scene->meshCount]);
	}
	
	r->scene->meshCount++;
	return valid;
}

bool loadMesh(struct renderer *r, char *inputFilePath, int idx, int meshCount) {
	logr(info, "Loading mesh %i/%i\r", idx, meshCount);
	
	obj_scene_data data;
	if (parse_obj_scene(&data, inputFilePath) == 0) {
		printf("\n");
		logr(warning, "Mesh %s not found!\n", getFileName(inputFilePath));
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
	newMesh->firstPolyIndex = polyCount;
	newMesh->polyCount = data.face_count;
	//Transforms init
	newMesh->transformCount = 0;
	
	newMesh->materialCount = 0;
	//Set name
	copyString(getFileName(inputFilePath), &newMesh->name);
	
	//Update vector and poly counts
	vertexCount += data.vertex_count;
	normalCount += data.vertex_normal_count;
	textureCount += data.vertex_texture_count;
	polyCount += data.face_count;
	
	//Data loaded, now convert everything
	//Convert vectors
	vertexArray = realloc(vertexArray, vertexCount * sizeof(struct vector));
	for (int i = 0; i < data.vertex_count; i++) {
		vertexArray[newMesh->firstVectorIndex + i] = vectorFromObj(data.vertex_list[i]);
	}
	
	//Convert normals
	normalArray = realloc(normalArray, normalCount * sizeof(struct vector));
	for (int i = 0; i < data.vertex_normal_count; i++) {
		normalArray[newMesh->firstNormalIndex + i] = vectorFromObj(data.vertex_normal_list[i]);
	}
	//Convert texture vectors
	textureArray = realloc(textureArray, textureCount * sizeof(struct coord));
	for (int i = 0; i < data.vertex_texture_count; i++) {
		textureArray[newMesh->firstTextureIndex + i] = coordFromObj(data.vertex_texture_list[i]);
	}
	//Convert polygons
	polygonArray = realloc(polygonArray, polyCount * sizeof(struct poly));
	for (int i = 0; i < data.face_count; i++) {
		polygonArray[newMesh->firstPolyIndex + i] = polyFromObj(data.face_list[i],
																newMesh->firstVectorIndex,
																newMesh->firstNormalIndex,
																newMesh->firstTextureIndex,
																newMesh->firstPolyIndex + i);
	}
	
	newMesh->materials = calloc(1, sizeof(struct material));
	//Parse materials
	if (data.material_count == 0) {
		//No material, set to something obscene to make it noticeable
		newMesh->materials = calloc(1, sizeof(struct material));
		newMesh->materials[0] = warningMaterial();
		assignBSDF(&newMesh->materials[0]);
		newMesh->materialCount++;
	} else {
		//Loop to add materials to mesh (We already set the material indices in polyFromObj)
		for (int i = 0; i < data.material_count; i++) {
			addMaterialToMesh(newMesh, materialFromObj(data.material_list[i]));
		}
	}
	
	//Load textures for meshes
	loadMeshTextures(newMesh);
	
	//Delete OBJ data
	delete_obj_data(&data);
	
	//Mesh added, update count
	r->scene->meshCount++;
	return true;
}

void addMaterialToMesh(struct mesh *mesh, struct material newMaterial) {
	mesh->materials = realloc(mesh->materials, (mesh->materialCount + 1) * sizeof(struct material));
	mesh->materials[mesh->materialCount++] = newMaterial;
}

//FIXME: change + 1 to ++scene->someCount and just pass the count to array access
//In the future, maybe just pass a list and size and copy at once to save time (large counts)
void addSphere(struct world *scene, struct sphere newSphere) {
	scene->spheres = realloc(scene->spheres, (scene->sphereCount + 1) * sizeof(struct sphere));
	scene->spheres[scene->sphereCount++] = newSphere;
}

void addCamTransform(struct camera *cam, struct transform transform) {
	if (cam->transformCount == 0) {
		cam->transforms = calloc(1, sizeof(struct transform));
	} else {
		cam->transforms = realloc(cam->transforms, (cam->transformCount + 1) * sizeof(struct transform));
	}
	
	cam->transforms[cam->transformCount] = transform;
	cam->transformCount++;
}

void addCamTransforms(struct camera *cam, struct transform *transforms, int count) {
	for (int i = 0; i < count; i++) {
		addCamTransform(cam, transforms[i]);
	}
}

struct material *parseMaterial(const cJSON *data) {
	cJSON *bsdf = NULL;
	cJSON *IOR = NULL;
	//cJSON *roughness = NULL;
	cJSON *color = NULL;
	cJSON *intensity = NULL;
	bool validIOR = false;
	//bool validRoughness = false;
	bool validIntensity = false;
	
	float IORValue = 1.0;
	//float roughnessValue;
	struct color colorValue = blackColor;
	float intensityValue = 1.0;
	
	struct material *mat = calloc(1, sizeof(struct material));
	
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
			intensityValue = 1.0;
		}
	}
	
	IOR = cJSON_GetObjectItem(data, "IOR");
	if (cJSON_IsNumber(IOR)) {
		validIOR = true;
		if (IOR->valuedouble >= 0) {
			IORValue = IOR->valuedouble;
		} else {
			IORValue = 1.0;
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

struct transform parseTransform(const cJSON *data, char *targetName) {
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
	float def = 0.0;
	if (strcmp(type->valuestring, "scale") == 0) {
		def = 1.0;
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
			return newTransformRotateX(degrees->valuedouble);
		} else if (validRadians) {
			return newTransformRotateX(fromRadians(radians->valuedouble));
		} else {
			logr(warning, "Found rotateX transform for object \"%s\" with no valid degrees or radians value given.\n", targetName);
		}
	} else if (strcmp(type->valuestring, "rotateY") == 0) {
		if (validDegrees) {
			return newTransformRotateY(degrees->valuedouble);
		} else if (validRadians) {
			return newTransformRotateY(fromRadians(radians->valuedouble));
		} else {
			logr(warning, "Found rotateY transform for object \"%s\" with no valid degrees or radians value given.\n", targetName);
		}
	} else if (strcmp(type->valuestring, "rotateZ") == 0) {
		if (validDegrees) {
			return newTransformRotateZ(degrees->valuedouble);
		} else if (validRadians) {
			return newTransformRotateZ(fromRadians(radians->valuedouble));
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
	return newTransformTranslate(0.0, 0.0, 0.0);
}

//Parse JSON array of transforms, and return a pointer to an array of corresponding transforms
struct transform *parseTransforms(const cJSON *data) {
	
	int transformCount = cJSON_GetArraySize(data);
	struct transform *transforms = calloc(transformCount, sizeof(struct transform));
	
	cJSON *transform = NULL;
	
	for (int i = 0; i < transformCount; i++) {
		transform = cJSON_GetArrayItem(data, i);
		transforms[i] = parseTransform(transform, "camera");
	}
	return transforms;
}

static struct prefs defaultPrefs = (struct prefs){
	.fileMode = saveModeNormal,
	.tileOrder = renderOrderFromMiddle,
	.threadCount = 1, //We run getSysCores() for this
	.sampleCount = 25,
	.bounces = 20,
	.tileWidth = 32,
	.tileHeight = 32,
	.antialiasing = true
};

int parsePrefs(struct prefs *p, const cJSON *data) {
	if (!data) *p = defaultPrefs;
	
	const cJSON *threadCount = NULL;
	const cJSON *sampleCount = NULL;
	const cJSON *antialiasing = NULL;
	const cJSON *tileWidth = NULL;
	const cJSON *tileHeight = NULL;
	const cJSON *tileOrder = NULL;
	const cJSON *bounces = NULL;
	
	threadCount = cJSON_GetObjectItem(data, "threadCount");
	if (threadCount) {
		if (cJSON_IsNumber(threadCount)) {
			if (threadCount->valueint > 0) {
				p->threadCount = threadCount->valueint;
				p->fromSystem = false;
			} else {
				p->threadCount = getSysCores() + 2;
				p->fromSystem = true;
			}
		} else {
			logr(warning, "Invalid threadCount while parsing renderer\n");
			return -1;
		}
	} else {
		p->threadCount = getSysCores() + 2;
		p->fromSystem = true;
	}
	
	sampleCount = cJSON_GetObjectItem(data, "sampleCount");
	if (sampleCount) {
		if (cJSON_IsNumber(sampleCount)) {
			if (sampleCount->valueint >= 1) {
				p->sampleCount = sampleCount->valueint;
			} else {
				p->sampleCount = 1;
			}
		} else {
			logr(warning, "Invalid sampleCount while parsing renderer\n");
			return -1;
		}
	} else {
		p->sampleCount = defaultPrefs.sampleCount;
	}
	
	
	bounces = cJSON_GetObjectItem(data, "bounces");
	if (bounces) {
		if (cJSON_IsNumber(bounces)) {
			if (bounces->valueint >= 0) {
				p->bounces = bounces->valueint;
			} else {
				p->bounces = 1;
			}
		} else {
			logr(warning, "Invalid bounces while parsing renderer\n");
			return -1;
		}
	} else {
		p->bounces = defaultPrefs.bounces;
	}
	
	antialiasing = cJSON_GetObjectItem(data, "antialiasing");
	if (antialiasing) {
		if (cJSON_IsBool(antialiasing)) {
			p->antialiasing = cJSON_IsTrue(antialiasing);
		} else {
			logr(warning, "Invalid antialiasing bool while parsing renderer\n");
			return -1;
		}
	} else {
		p->antialiasing = defaultPrefs.antialiasing;
	}
	
	tileWidth = cJSON_GetObjectItem(data, "tileWidth");
	if (tileWidth) {
		if (cJSON_IsNumber(tileWidth)) {
			if (tileWidth->valueint >= 1) {
				p->tileWidth = tileWidth->valueint;
			} else {
				p->tileWidth = 1;
			}
		} else {
			logr(warning, "Invalid tileWidth while parsing renderer\n");
			return -1;
		}
	} else {
		p->tileWidth = defaultPrefs.tileWidth;
	}
	
	tileHeight = cJSON_GetObjectItem(data, "tileHeight");
	if (tileHeight) {
		if (cJSON_IsNumber(tileHeight)) {
			if (tileHeight->valueint >= 1) {
				p->tileHeight = tileHeight->valueint;
			} else {
				p->tileHeight = 1;
			}
		} else {
			logr(warning, "Invalid tileHeight while parsing renderer\n");
			return -1;
		}
	} else {
		p->tileHeight = defaultPrefs.tileHeight;
	}
	
	tileOrder = cJSON_GetObjectItem(data, "tileOrder");
	if (tileOrder) {
		if (cJSON_IsString(tileOrder)) {
			if (strcmp(tileOrder->valuestring, "normal") == 0) {
				p->tileOrder = renderOrderNormal;
			} else if (strcmp(tileOrder->valuestring, "random") == 0) {
				p->tileOrder = renderOrderRandom;
			} else if (strcmp(tileOrder->valuestring, "topToBottom") == 0) {
				p->tileOrder = renderOrderTopToBottom;
			} else if (strcmp(tileOrder->valuestring, "fromMiddle") == 0) {
				p->tileOrder = renderOrderFromMiddle;
			} else if (strcmp(tileOrder->valuestring, "toMiddle") == 0) {
				p->tileOrder = renderOrderToMiddle;
			} else {
				p->tileOrder = renderOrderNormal;
			}
		} else {
			logr(warning, "Invalid tileOrder while parsing renderer\n");
			return -1;
		}
	} else {
		p->tileOrder = defaultPrefs.tileOrder;
	}
	
	return 0;
}

static struct display defaultDisplay = (struct display){
	.enabled = true,
	.isBorderless = false,
	.isFullScreen = false,
	.windowScale = 1.0f
};

int parseDisplay(struct display *d, const cJSON *data) {
	if (!data) *d = defaultDisplay;
	const cJSON *enabled = NULL;
	const cJSON *isFullscreen = NULL;
	const cJSON *isBorderless = NULL;
	const cJSON *windowScale = NULL;
	
	enabled = cJSON_GetObjectItem(data, "enabled");
	if (enabled) {
		if (cJSON_IsBool(enabled)) {
			d->enabled = cJSON_IsTrue(enabled);
		} else {
			logr(warning, "Invalid enabled while parsing display prefs.\n");
			return -1;
		}
	} else {
		d->enabled = defaultDisplay.enabled;
	}
	
	isFullscreen = cJSON_GetObjectItem(data, "isFullscreen");
	if (isFullscreen) {
		if (cJSON_IsBool(isFullscreen)) {
			d->isFullScreen = cJSON_IsTrue(isFullscreen);
		} else {
			logr(warning, "Invalid isFullscreen while parsing display prefs.\n");
			return -1;
		}
	} else {
		d->isFullScreen = defaultDisplay.isFullScreen;
	}
	
	isBorderless = cJSON_GetObjectItem(data, "isBorderless");
	if (isBorderless) {
		if (cJSON_IsBool(isBorderless)) {
			d->isBorderless = cJSON_IsTrue(isBorderless);
		} else {
			logr(warning, "Invalid isBorderless while parsing display prefs.\n");
			return -1;
		}
	} else {
		d->isBorderless = defaultDisplay.isBorderless;
	}
	
	windowScale = cJSON_GetObjectItem(data, "windowScale");
	if (windowScale) {
		if (cJSON_IsNumber(windowScale)) {
			if (windowScale->valuedouble >= 0) {
				d->windowScale = windowScale->valuedouble;
			} else {
				d->windowScale = 1.0f;
			}
		} else {
			logr(warning, "Invalid isBorderless while parsing display prefs.\n");
			return -1;
		}
	} else {
		d->windowScale = defaultDisplay.windowScale;
	}
	
	return 0;
}

static struct camera defaultCamera = (struct camera){
	.FOV = 80.0f,
	.aperture = 0.0f
};

int parseCamera(struct camera *c, const cJSON *data) {
	if (!data) *c = defaultCamera;
	const cJSON *FOV = NULL;
	const cJSON *aperture = NULL;
	const cJSON *transforms = NULL;
	
	FOV = cJSON_GetObjectItem(data, "FOV");
	if (FOV) {
		if (cJSON_IsNumber(FOV)) {
			if (FOV->valuedouble >= 0.0) {
				if (FOV->valuedouble > 180.0) {
					c->FOV = 180.0;
				} else {
					c->FOV = FOV->valuedouble;
				}
			} else {
				c->FOV = 80.0;
			}
		} else {
			logr(warning, "Invalid FOV value while parsing camera.\n");
			return -1;
		}
	} else {
		c->FOV = defaultCamera.FOV;
	}
	
	aperture = cJSON_GetObjectItem(data, "aperture");
	if (aperture) {
		if (cJSON_IsNumber(aperture)) {
			if (aperture->valuedouble >= 0.0) {
				c->aperture = aperture->valuedouble;
			} else {
				c->aperture = 0.0;
			}
		} else {
			logr(warning, "Invalid aperture while parsing camera.\n");
			return -1;
		}
	} else {
		c->aperture = defaultCamera.aperture;
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

struct color parseColor(const cJSON *data) {
	
	const cJSON *R = NULL;
	const cJSON *G = NULL;
	const cJSON *B = NULL;
	const cJSON *A = NULL;
	
	struct color newColor;
	
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
		newColor.alpha = 0.0;
	}
	
	return newColor;
}

//FIXME:
int parseAmbientColor(struct renderer *r, const cJSON *data) {
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
		r->scene->hdr = loadTexture(hdr->valuestring);
	}
	
	offset = cJSON_GetObjectItem(data, "offset");
	if (cJSON_IsNumber(offset)) {
		if (r->scene->hdr) {
			r->scene->hdr->offset = toRadians(offset->valuedouble)/4;
		}
	}
	
	return 0;
}

//FIXME: Only parse everything else if the mesh is found and is valid
void parseMesh(struct renderer *r, const cJSON *data, int idx, int meshCount) {
	const cJSON *fileName = cJSON_GetObjectItem(data, "fileName");
	
	const cJSON *bsdf = cJSON_GetObjectItem(data, "bsdf");
	enum bsdfType type = lambertian;
	
	if (cJSON_IsString(bsdf)) {
		if (strcmp(bsdf->valuestring, "lambertian") == 0) {
			type = lambertian;
		} else if (strcmp(bsdf->valuestring, "metal") == 0) {
			type = metal;
		} else if (strcmp(bsdf->valuestring, "glass") == 0) {
			type = glass;
		} else {
			type = lambertian;
		}
	} else {
		logr(warning, "Invalid bsdf while parsing mesh\n");
	}
	
	bool meshValid = false;
	if (fileName != NULL && cJSON_IsString(fileName)) {
		if (loadMesh(r, fileName->valuestring, idx, meshCount)) {
			meshValid = true;
		} else {
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
		for (int i = 0; i < lastMesh(r)->materialCount; i++) {
			lastMesh(r)->materials[i].type = type;
			assignBSDF(&lastMesh(r)->materials[i]);
		}
	}
}

//FIXME:
void parseMeshes(struct renderer *r, const cJSON *data) {
	const cJSON *mesh = NULL;
	int idx = 1;
	int meshCount = cJSON_GetArraySize(data);
	if (data != NULL && cJSON_IsArray(data)) {
		cJSON_ArrayForEach(mesh, data) {
			parseMesh(r, mesh, idx, meshCount);
			idx++;
		}
	}
	printf("\n");
}

struct vector parseCoordinate(const cJSON *data) {
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
	return (struct vector){0.0,0.0,0.0};
}

void parseSphere(struct renderer *r, const cJSON *data) {
	const cJSON *pos = NULL;
	const cJSON *color = NULL;
	const cJSON *roughness = NULL;
	const cJSON *IOR = NULL;
	const cJSON *radius = NULL;
	const cJSON *intensity = NULL;
	
	struct sphere newSphere = defaultSphere();
	
	const cJSON *bsdf = cJSON_GetObjectItem(data, "bsdf");
	
	if (cJSON_IsString(bsdf)) {
		if (strcmp(bsdf->valuestring, "lambertian") == 0) {
			newSphere.material.type = lambertian;
		} else if (strcmp(bsdf->valuestring, "metal") == 0) {
			newSphere.material.type = metal;
		} else if (strcmp(bsdf->valuestring, "glass") == 0) {
			newSphere.material.type = glass;
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
		newSphere.material.roughness = 0.0;
	}
	
	IOR = cJSON_GetObjectItem(data, "IOR");
	if (IOR != NULL && cJSON_IsNumber(IOR)) {
		newSphere.material.IOR = IOR->valuedouble;
	} else {
		newSphere.material.IOR = 1.0;
	}
	
	radius = cJSON_GetObjectItem(data, "radius");
	if (radius != NULL && cJSON_IsNumber(radius)) {
		newSphere.radius = radius->valuedouble;
	} else {
		newSphere.radius = 10;
		logr(warning, "No radius specified for sphere, setting to %.0f\n", newSphere.radius);
	}
	
	//FIXME: Proper materials for spheres
	addSphere(r->scene, newSphere);
	assignBSDF(&lastSphere(r)->material);
}

void parsePrimitive(struct renderer *r, const cJSON *data, int idx) {
	const cJSON *type = NULL;
	type = cJSON_GetObjectItem(data, "type");
	if (strcmp(type->valuestring, "sphere") == 0) {
		parseSphere(r, data);
	} else {
		logr(warning, "Unknown primitive type \"%s\" at index %i\n", type->valuestring, idx);
	}
}

void parsePrimitives(struct renderer *r, const cJSON *data) {
	const cJSON *primitive = NULL;
	if (data != NULL && cJSON_IsArray(data)) {
		int i = 0;
		cJSON_ArrayForEach(primitive, data) {
			parsePrimitive(r, primitive, i);
			i++;
		}
	}
}

static char *defaultFilePath = "./";
static char *defaultFileName = "rendered";
static int defaultCount = 0;
static int defaultWidth = 1280;
static int defaultHeight = 800;
static enum fileType defaultFileType = png;

int parseScene(struct renderer *r, const cJSON *data) {
	
	const cJSON *filePath = NULL;
	const cJSON *fileName = NULL;
	const cJSON *count = NULL;
	const cJSON *width = NULL;
	const cJSON *height = NULL;
	const cJSON *fileType = NULL;
	const cJSON *ambientColor = NULL;
	const cJSON *primitives = NULL;
	const cJSON *meshes = NULL;
	
	filePath = cJSON_GetObjectItem(data, "outputFilePath");
	if (filePath) {
		if (cJSON_IsString(filePath)) {
			copyString(filePath->valuestring, &r->state.image->filePath);
		} else {
			logr(warning, "Invalid filePath while parsing scene.\n");
			return -1;
		}
	} else {
		copyString(defaultFilePath, &r->state.image->filePath);
	}
	
	fileName = cJSON_GetObjectItem(data, "outputFileName");
	if (fileName) {
		if (cJSON_IsString(fileName)) {
			copyString(fileName->valuestring, &r->state.image->fileName);
		} else {
			logr(warning, "Invalid fileName while parsing scene.\n");
			return -1;
		}
	} else {
		copyString(defaultFileName, &r->state.image->fileName);
	}
	
	count = cJSON_GetObjectItem(data, "count");
	if (count) {
		if (cJSON_IsNumber(count)) {
			if (count->valueint >= 0) {
				r->state.image->count = count->valueint;
			} else {
				r->state.image->count = 0;
			}
		} else {
			logr(warning, "Invalid count while parsing scene.\n");
			return -1;
		}
	} else {
		r->state.image->count = defaultCount;
	}
	
	//FIXME: This is super ugly
	width = cJSON_GetObjectItem(data, "width");
	if (width) {
		if (cJSON_IsNumber(width)) {
			if (width->valueint >= 0) {
				r->state.image->width = width->valueint;
				r->mainDisplay->width = width->valueint;
			} else {
				r->state.image->width = 640;
				r->mainDisplay->width = 640;
			}
		} else {
			logr(warning, "Invalid width while parsing scene.\n");
			return -1;
		}
	} else {
		r->state.image->width = defaultWidth;
		r->mainDisplay->width = defaultWidth;
	}
	
	height = cJSON_GetObjectItem(data, "height");
	if (height) {
		if (cJSON_IsNumber(height)) {
			if (height->valueint >= 0) {
				r->state.image->height = height->valueint;
				r->mainDisplay->height = height->valueint;
			} else {
				r->state.image->height = 400;
				r->mainDisplay->height = 400;
			}
		} else {
			logr(warning, "Invalid height while parsing scene.\n");
			return -1;
		}
	} else {
		r->state.image->height = defaultHeight;
		r->mainDisplay->height = defaultHeight;
	}
	
	fileType = cJSON_GetObjectItem(data, "fileType");
	if (fileType) {
		if (cJSON_IsString(fileType)) {
			if (strcmp(fileType->valuestring, "png") == 0) {
				r->state.image->fileType = png;
			} else if (strcmp(fileType->valuestring, "bmp") == 0) {
				r->state.image->fileType = bmp;
			} else {
				r->state.image->fileType = png;
			}
		} else {
			logr(warning, "Invalid fileType while parsing scene.\n");
			return -1;
		}
	} else {
		r->state.image->fileType = defaultFileType;
	}
	
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
		};;
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

//input is either a file path to load if fromStdin = false, or a data buffer if stdin = true
int parseJSON(struct renderer *r, char *input, bool fromStdin) {
	
	/*
	 Note: Since we are freeing the JSON data (and its' pointers) after parsing,
	 we need to *copy* all dynamically allocated strings with the copyString() function.
	 */
	
	char *buf = NULL;
	
	if (fromStdin) {
		buf = input;
	} else {
		size_t bytes = 0;
		buf = loadFile(input, &bytes);
		if (!buf) {
			return -1;
		}
		logr(info, "%zi bytes of input JSON loaded from file, parsing.\n", bytes);
	}
	
	cJSON *json = cJSON_Parse(buf);
	free(buf);
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
	if (parsePrefs(&r->prefs, renderer) == -1) {
		logr(warning, "Renderer parse failed!\n");
		return -2;
	}
	
	display = cJSON_GetObjectItem(json, "display");
	if (parseDisplay(r->mainDisplay, display) == -1) {
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
