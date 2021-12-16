//
//  sceneloader.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2020 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "sceneloader.h"

//FIXME: We should only need to include c-ray.h here!

#include "../../libraries/cJSON.h"
#include "../../datatypes/scene.h"
#include "../../datatypes/vertexbuffer.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/camera.h"
#include "../../datatypes/mesh.h"
#include "../../datatypes/sphere.h"
#include "../../datatypes/material.h"
#include "../../datatypes/poly.h"
#include "../../datatypes/transforms.h"
#include "../logging.h"
#include "../fileio.h"
#include "../string.h"
#include "../platform/capabilities.h"
#include "../../datatypes/image/imagefile.h"
#include "../../renderer/renderer.h"
#include "textureloader.h"
#include "../../datatypes/instance.h"
#include "../../utils/args.h"
#include "../../utils/timer.h"
#include "../../utils/string.h"
#include "../../nodes/bsdfnode.h"
#include "meshloader.h"

struct transform parseTransformComposite(const cJSON *transforms);

static struct color parseColor(const cJSON *data);

static struct instance *lastInstance(struct renderer *r) {
	return &r->scene->instances[r->scene->instanceCount - 1];
}

static struct mesh *lastMesh(struct renderer *r) {
	return &r->scene->meshes[r->scene->meshCount - 1];
}

static struct sphere *lastSphere(struct renderer *r) {
	return &r->scene->spheres[r->scene->sphereCount - 1];
}

static bool loadMeshNew(struct renderer *r, char *inputFilePath, int idx, int totalMeshes) {
	logr(plain, "\r");
	logr(info, "Loading mesh %i/%i%s", idx, totalMeshes, idx == totalMeshes ? "\n" : "\r");
	bool valid = false;
	size_t meshCount = 0;
	struct mesh *newMeshes = loadMesh(inputFilePath, &meshCount);
	if (meshCount == 0) return false;
	ASSERT(meshCount == 1); //FIXME: Remove this
	if (newMeshes) {
		for (size_t m = 0; m < meshCount; ++m) {
			r->scene->meshes[r->scene->meshCount + m] = newMeshes[m];
			free(&newMeshes[m]);
			valid = true;
		}
	}
	
	r->scene->meshCount += meshCount;
	return valid;
}

static void addSphere(struct world *scene, struct sphere newSphere) {
	scene->spheres[scene->sphereCount++] = newSphere;
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
	if (stringEquals(type->valuestring, "scale")) {
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
	
	if (stringEquals(type->valuestring, "rotateX")) {
		if (validDegrees) {
			return newTransformRotateX(toRadians(degrees->valuedouble));
		} else if (validRadians) {
			return newTransformRotateX(radians->valuedouble);
		} else {
			logr(warning, "Found rotateX transform for object \"%s\" with no valid degrees or radians value given.\n", targetName);
		}
	} else if (stringEquals(type->valuestring, "rotateY")) {
		if (validDegrees) {
			return newTransformRotateY(toRadians(degrees->valuedouble));
		} else if (validRadians) {
			return newTransformRotateY(radians->valuedouble);
		} else {
			logr(warning, "Found rotateY transform for object \"%s\" with no valid degrees or radians value given.\n", targetName);
		}
	} else if (stringEquals(type->valuestring, "rotateZ")) {
		if (validDegrees) {
			return newTransformRotateZ(toRadians(degrees->valuedouble));
		} else if (validRadians) {
			return newTransformRotateZ(radians->valuedouble);
		} else {
			logr(warning, "Found rotateZ transform for object \"%s\" with no valid degrees or radians value given.\n", targetName);
		}
	} else if (stringEquals(type->valuestring, "translate")) {
		if (validCoords > 0) {
			return newTransformTranslate(Xval, Yval, Zval);
		} else {
			logr(warning, "Found translate transform for object \"%s\" with less than 1 valid coordinate given.\n", targetName);
		}
	} else if (stringEquals(type->valuestring, "scale")) {
		if (validCoords > 0) {
			return newTransformScale(Xval, Yval, Zval);
		} else {
			logr(warning, "Found scale transform for object \"%s\" with less than 1 valid scale value given.\n", targetName);
		}
	} else if (stringEquals(type->valuestring, "scaleUniform")) {
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

void parsePrefs(struct prefs *prefs, const cJSON *data) {
	if (!data) return;

	const cJSON *threads = cJSON_GetObjectItem(data, "threads");
	if (threads) {
		if (cJSON_IsNumber(threads)) {
			if (threads->valueint > 0) {
				prefs->threadCount = threads->valueint;
				prefs->fromSystem = false;
			} else {
				prefs->threadCount = getSysCores() + 2;
				prefs->fromSystem = true;
			}
		} else {
			logr(warning, "Invalid threads while parsing renderer\n");
		}
	}

	const cJSON *samples = cJSON_GetObjectItem(data, "samples");
	if (samples) {
		if (cJSON_IsNumber(samples)) {
			if (samples->valueint >= 1) {
				prefs->sampleCount = samples->valueint;
			} else {
				prefs->sampleCount = 1;
			}
		} else {
			logr(warning, "Invalid samples while parsing renderer\n");
		}
	}

	const cJSON *bounces = cJSON_GetObjectItem(data, "bounces");
	if (bounces) {
		if (cJSON_IsNumber(bounces)) {
			if (bounces->valueint >= 0) {
				prefs->bounces = bounces->valueint;
			} else {
				prefs->bounces = 1;
			}
		} else {
			logr(warning, "Invalid bounces while parsing renderer\n");
		}
	}

	const cJSON *antialiasing = cJSON_GetObjectItem(data, "antialiasing");
	if (antialiasing) {
		if (cJSON_IsBool(antialiasing)) {
			prefs->antialiasing = cJSON_IsTrue(antialiasing);
		} else {
			logr(warning, "Invalid antialiasing bool while parsing renderer\n");
		}
	}

	const cJSON *tileWidth = cJSON_GetObjectItem(data, "tileWidth");
	if (tileWidth) {
		if (cJSON_IsNumber(tileWidth)) {
			if (tileWidth->valueint >= 1) {
				prefs->tileWidth = tileWidth->valueint;
			} else {
				prefs->tileWidth = 1;
			}
		} else {
			logr(warning, "Invalid tileWidth while parsing renderer\n");
		}
	}

	const cJSON *tileHeight = cJSON_GetObjectItem(data, "tileHeight");
	if (tileHeight) {
		if (cJSON_IsNumber(tileHeight)) {
			if (tileHeight->valueint >= 1) {
				prefs->tileHeight = tileHeight->valueint;
			} else {
				prefs->tileHeight = 1;
			}
		} else {
			logr(warning, "Invalid tileHeight while parsing renderer\n");
		}
	}

	const cJSON *tileOrder = cJSON_GetObjectItem(data, "tileOrder");
	if (tileOrder) {
		if (cJSON_IsString(tileOrder)) {
			if (stringEquals(tileOrder->valuestring, "random")) {
				prefs->tileOrder = renderOrderRandom;
			} else if (stringEquals(tileOrder->valuestring, "topToBottom")) {
				prefs->tileOrder = renderOrderTopToBottom;
			} else if (stringEquals(tileOrder->valuestring, "fromMiddle")) {
				prefs->tileOrder = renderOrderFromMiddle;
			} else if (stringEquals(tileOrder->valuestring, "toMiddle")) {
				prefs->tileOrder = renderOrderToMiddle;
			} else {
				prefs->tileOrder = renderOrderNormal;
			}
		} else {
			logr(warning, "Invalid tileOrder while parsing renderer\n");
		}
	}

	const cJSON *filePath = cJSON_GetObjectItem(data, "outputFilePath");
	if (filePath) {
		if (cJSON_IsString(filePath)) {
			free(prefs->imgFilePath);
			prefs->imgFilePath = stringCopy(filePath->valuestring);
		} else {
			logr(warning, "Invalid filePath while parsing scene.\n");
		}
	}

	const cJSON *fileName = cJSON_GetObjectItem(data, "outputFileName");
	if (fileName) {
		if (cJSON_IsString(fileName)) {
			free(prefs->imgFileName);
			prefs->imgFileName = stringCopy(fileName->valuestring);
		} else {
			logr(warning, "Invalid fileName while parsing scene.\n");
		}
	}

	const cJSON *count = cJSON_GetObjectItem(data, "count");
	if (count) {
		if (cJSON_IsNumber(count)) {
			if (count->valueint >= 0) {
				prefs->imgCount = count->valueint;
			} else {
				prefs->imgCount = 0;
			}
		} else {
			logr(warning, "Invalid count while parsing scene.\n");
		}
	}

	const cJSON *width = cJSON_GetObjectItem(data, "width");
	if (width) {
		if (cJSON_IsNumber(width)) {
			if (width->valueint >= 0) {
				prefs->imageWidth = width->valueint;
			} else {
				prefs->imageWidth = 640;
			}
		} else {
			logr(warning, "Invalid width while parsing scene.\n");
		}
	}

	const cJSON *height = cJSON_GetObjectItem(data, "height");
	if (height) {
		if (cJSON_IsNumber(height)) {
			if (height->valueint >= 0) {
				prefs->imageHeight = height->valueint;
			} else {
				prefs->imageHeight = 400;
			}
		} else {
			logr(warning, "Invalid height while parsing scene.\n");
		}
	}

	const cJSON *fileType = cJSON_GetObjectItem(data, "fileType");
	if (fileType) {
		if (cJSON_IsString(fileType)) {
			if (stringEquals(fileType->valuestring, "bmp")) {
				prefs->imgType = bmp;
			} else {
				prefs->imgType = png;
			}
		} else {
			logr(warning, "Invalid fileType while parsing scene.\n");
		}
	}

	// Now check and apply potential CLI overrides.
	if (isSet("thread_override")) {
		int threads = intPref("thread_override");
		if (prefs->threadCount != threads) {
			logr(info, "Overriding thread count to %i\n", threads);
			prefs->threadCount = threads;
			prefs->fromSystem = false;
		}
	}
	
	if (isSet("samples_override")) {
		if (isSet("is_worker")) {
			logr(warning, "Can't override samples when in worker mode\n");
		} else {
			int samples = intPref("samples_override");
			logr(info, "Overriding sample count to %i\n", samples);
			prefs->sampleCount = samples;
		}
	}
	
	if (isSet("dims_override")) {
		if (isSet("is_worker")) {
			logr(warning, "Can't override dimensions when in worker mode\n");
		} else {
			int width = intPref("dims_width");
			int height = intPref("dims_height");
			logr(info, "Overriding image dimensions to %ix%i\n", width, height);
			prefs->imageWidth = width;
			prefs->imageHeight = height;
		}
	}
	
	if (isSet("tiledims_override")) {
		if (isSet("is_worker")) {
			logr(warning, "Can't override tile dimensions when in worker mode\n");
		} else {
			int width = intPref("tile_width");
			int height = intPref("tile_height");
			logr(info, "Overriding tile  dimensions to %ix%i\n", width, height);
			prefs->tileWidth = width;
			prefs->tileHeight = height;
		}
	}

	if (isSet("cam_index")) {
		prefs->selected_camera = intPref("cam_index");
		logr(info, "Selecting camera %i\n", prefs->selected_camera);
	}
}

static void parseDisplay(struct prefs *prefs, const cJSON *data) {
	if (!data) return;

	const cJSON *enabled = cJSON_GetObjectItem(data, "enabled");
	if (enabled) {
		if (cJSON_IsBool(enabled)) {
			prefs->enabled = cJSON_IsTrue(enabled);
		} else {
			logr(warning, "Invalid enabled while parsing display prefs.\n");
		}
	}

	const cJSON *isFullscreen = cJSON_GetObjectItem(data, "isFullscreen");
	if (isFullscreen) {
		if (cJSON_IsBool(isFullscreen)) {
			prefs->fullscreen = cJSON_IsTrue(isFullscreen);
		} else {
			logr(warning, "Invalid isFullscreen while parsing display prefs.\n");
		}
	}

	const cJSON *isBorderless = cJSON_GetObjectItem(data, "isBorderless");
	if (isBorderless) {
		if (cJSON_IsBool(isBorderless)) {
			prefs->borderless = cJSON_IsTrue(isBorderless);
		} else {
			logr(warning, "Invalid isBorderless while parsing display prefs.\n");
		}
	}

	const cJSON *windowScale = cJSON_GetObjectItem(data, "windowScale");
	if (windowScale) {
		if (cJSON_IsNumber(windowScale)) {
			if (windowScale->valuedouble >= 0) {
				prefs->scale = windowScale->valuedouble;
			} else {
				prefs->scale = 1.0f;
			}
		} else {
			logr(warning, "Invalid isBorderless while parsing display prefs.\n");
		}
	}
}

struct spline *test() {
	return spline_new((struct vector){-0.1f, 0.0f, -0.7f}, (struct vector){-0.1f, 0.2f, -0.7f}, (struct vector){0.1f, 0.2f, -0.7f}, (struct vector){0.1f, 0.0f, -0.7f});
}

float getRadians(const cJSON *object) {
	cJSON *degrees = cJSON_GetObjectItem(object, "degrees");
	cJSON *radians = cJSON_GetObjectItem(object, "radians");
	if (degrees) {
		return toRadians(degrees->valuedouble);
	}
	if (radians) {
		return radians->valuedouble;
	}
	return 0.0f;
}

//TODO: Delet these two
static struct not_a_quaternion *parseRotations(const cJSON *transforms) {
	if (!transforms) return NULL;
	
	struct not_a_quaternion rotations = { 0 };
	const cJSON *transform = NULL;
	cJSON_ArrayForEach(transform, transforms) {
		cJSON *type = cJSON_GetObjectItem(transform, "type");
		if (stringEquals(type->valuestring, "rotateX")) {
			rotations.rotX = getRadians(transform);
		}
		if (stringEquals(type->valuestring, "rotateY")) {
			rotations.rotY = getRadians(transform);
		}
		if (stringEquals(type->valuestring, "rotateZ")) {
			rotations.rotZ = getRadians(transform);
		}
	}
	
	struct not_a_quaternion *quat = calloc(1, sizeof(*quat));
	*quat = rotations;
	
	return quat;
}

static struct vector *parseLocation(const cJSON *transforms) {
	if (!transforms) return NULL;
	
	struct vector *loc = NULL;
	const cJSON *transform = NULL;
	cJSON_ArrayForEach(transform, transforms) {
		cJSON *type = cJSON_GetObjectItem(transform, "type");
		if (stringEquals(type->valuestring, "translate")) {
			loc = calloc(1, sizeof(*loc));
			const cJSON *x = cJSON_GetObjectItem(transform, "x");
			const cJSON *y = cJSON_GetObjectItem(transform, "y");
			const cJSON *z = cJSON_GetObjectItem(transform, "z");
			
			*loc = (struct vector){x->valuedouble, y->valuedouble, z->valuedouble};
		}
	}
	return loc;
}

static struct camera parseCamera(const cJSON *data) {
	struct camera cam = (struct camera){ 0 };
	if (!cJSON_IsObject(data)) return cam;
	const cJSON *FOV = cJSON_GetObjectItem(data, "FOV");
	if (FOV) {
		if (cJSON_IsNumber(FOV)) {
			if (FOV->valuedouble >= 0.0) {
				if (FOV->valuedouble > 180.0) {
					cam.FOV = 180.0f;
				} else {
					cam.FOV = FOV->valuedouble;
				}
			} else {
				cam.FOV = 80.0f;
			}
		} else {
			logr(warning, "Invalid FOV value while parsing camera.\n");
		}
	}

	const cJSON *focalDistance = cJSON_GetObjectItem(data, "focalDistance");
	if (focalDistance) {
		if (cJSON_IsNumber(focalDistance)) {
			if (focalDistance->valuedouble >= 0.0) {
				cam.focalDistance = focalDistance->valuedouble;
			} else {
				cam.focalDistance = 0.0f;
			}
		} else {
			logr(warning, "Invalid focalDistance while parsing camera.\n");
		}
	}

	const cJSON *aperture = cJSON_GetObjectItem(data, "fstops");
	if (aperture) {
		if (cJSON_IsNumber(aperture)) {
			if (aperture->valuedouble >= 0.0) {
				cam.fstops = aperture->valuedouble;
			} else {
				cam.fstops = 0.0f;
			}
		} else {
			logr(warning, "Invalid aperture while parsing camera.\n");
		}
	}

	const cJSON *time = cJSON_GetObjectItem(data, "time");
	if (time) {
		if (cJSON_IsNumber(time)) {
			if (time->valuedouble >= 0.0) {
				cam.time = time->valuedouble;
			} else {
				cam.time = 0.0f;
			}
		} else {
			logr(warning, "Invalid time while parsing camera.\n");
		}
	}

	// FIXME: Hack - we should really just not use transforms externally for the camera
	// Just can't be bothered to fix up all the scene files by hand right now
	struct not_a_quaternion *rotations = NULL;
	struct vector *location = NULL;
	const cJSON *transforms = cJSON_GetObjectItem(data, "transforms");
	if (transforms) {
		if (cJSON_IsArray(transforms)) {
			rotations = parseRotations(transforms);
			location = parseLocation(transforms);
		} else {
			logr(warning, "Invalid transforms while parsing camera.\n");
		}
	}

#ifdef TEST_BEZIER
	cam.path = test();
#endif

	cam_update_pose(&cam, rotations, location);
	free(rotations);
	free(location);
	return cam;
}

static void parseCameras(struct camera **cam, size_t *cam_count, const cJSON *data) {
	if (!data) return;

	if (cJSON_IsObject(data)) {
		// Single camera
		*cam = calloc(1, sizeof(struct camera));
		(*cam)[0] = parseCamera(data);
		if (cam_count) *cam_count = 1;
		return;
	}

	ASSERT(cJSON_IsArray(data));
	size_t camera_count = cJSON_GetArraySize(data);
	*cam = calloc(camera_count, sizeof(struct camera));

	cJSON *camera = NULL;
	size_t current_camera = 0;
	cJSON_ArrayForEach(camera, data) {
		(*cam)[current_camera++] = parseCamera(camera);
	}

	if (cam_count) *cam_count = current_camera;
}

static struct color parseColor(const cJSON *data) {
	struct color newColor;
	if (cJSON_IsArray(data)) {
		newColor.red =   cJSON_IsNumber(cJSON_GetArrayItem(data, 0)) ? cJSON_GetArrayItem(data, 0)->valuedouble : 0.0f;
		newColor.green = cJSON_IsNumber(cJSON_GetArrayItem(data, 1)) ? cJSON_GetArrayItem(data, 1)->valuedouble : 0.0f;
		newColor.blue =  cJSON_IsNumber(cJSON_GetArrayItem(data, 2)) ? cJSON_GetArrayItem(data, 2)->valuedouble : 0.0f;
		newColor.alpha = cJSON_IsNumber(cJSON_GetArrayItem(data, 3)) ? cJSON_GetArrayItem(data, 3)->valuedouble : 1.0f;
		return newColor;
	}
	
	ASSERT(cJSON_IsObject(data));
	
	const cJSON *R = NULL;
	const cJSON *G = NULL;
	const cJSON *B = NULL;
	const cJSON *A = NULL;
	const cJSON *kelvin = NULL;
	
	kelvin = cJSON_GetObjectItem(data, "blackbody");
	if (kelvin && cJSON_IsNumber(kelvin)) {
		newColor = colorForKelvin(kelvin->valuedouble);
		return newColor;
	}
	
	R = cJSON_GetObjectItem(data, "r");
	if (cJSON_IsNumber(R)) {
		newColor.red = R->valuedouble;
	} else {
		newColor.red = 0.0f;
	}
	G = cJSON_GetObjectItem(data, "g");
	if (cJSON_IsNumber(G)) {
		newColor.green = G->valuedouble;
	} else {
		newColor.green = 0.0f;
	}
	B = cJSON_GetObjectItem(data, "b");
	if (cJSON_IsNumber(B)) {
		newColor.blue = B->valuedouble;
	} else {
		newColor.blue = 0.0f;
	}
	A = cJSON_GetObjectItem(data, "a");
	if (cJSON_IsNumber(A)) {
		newColor.alpha = A->valuedouble;
	} else {
		newColor.alpha = 1.0f;
	}
	
	return newColor;
}

//FIXME: Convert this to use parseBsdfNode
static void parseAmbientColor(struct renderer *r, const cJSON *data) {
	const cJSON *offset = cJSON_GetObjectItem(data, "offset");
	if (cJSON_IsNumber(offset)) {
		r->scene->backgroundOffset = toRadians(offset->valuedouble) / 4.0f;
	}

	const cJSON *down = cJSON_GetObjectItem(data, "down");
	const cJSON *up = cJSON_GetObjectItem(data, "up");
	const cJSON *hdr = cJSON_GetObjectItem(data, "hdr");
	
	if (cJSON_IsString(hdr)) {
		char *fullPath = stringConcat(r->prefs.assetPath, hdr->valuestring);
		if (isValidFile(fullPath)) {
			r->scene->background = newBackground(r->scene, newImageTexture(r->scene, loadTexture(fullPath, &r->scene->nodePool), 0), NULL);
			free(fullPath);
			return;
		}
	}
	
	if (down && up) {
		r->scene->background = newBackground(r->scene, newGradientTexture(r->scene, parseColor(down), parseColor(up)), NULL);
		return;
	}

	r->scene->background = newBackground(r->scene, NULL, NULL);
}

struct transform parseTransformComposite(const cJSON *transforms) {
	// Combine found list of discerete transforms into a single matrix.
	if (!transforms) return newTransform();
	const cJSON *transform = NULL;
	size_t count = cJSON_GetArraySize(transforms);
	struct transform *tforms = calloc(count, sizeof(*tforms));
	size_t idx = 0;
	cJSON_ArrayForEach(transform, transforms) {
		tforms[idx++] = parseTransform(transform, "compositeBuilder");
	}
	
	struct transform composite = newTransform();
	
	// Order is: scales/rotates, then translates
	
	// Translates
	for (size_t i = 0; i < count; ++i) {
		if (isTranslate(&tforms[i])) {
			composite.A = multiplyMatrices(&composite.A, &tforms[i].A);
		}
	}
	
	// Rotates
	for (size_t i = 0; i < count; ++i) {
		if (isRotation(&tforms[i])) {
			composite.A = multiplyMatrices(&composite.A, &tforms[i].A);
		}
	}
	
	// Scales
	for (size_t i = 0; i < count; ++i) {
		if (isScale(&tforms[i])) {
			composite.A = multiplyMatrices(&composite.A, &tforms[i].A);
		}
	}
	
	composite.Ainv = inverseMatrix(&composite.A);
	composite.type = transformTypeComposite;
	free(tforms);
	return composite;
}

static struct transform parseInstanceTransform(const cJSON *instance) {
	const cJSON *transforms = cJSON_GetObjectItem(instance, "transforms");
	return parseTransformComposite(transforms);
}

static const struct colorNode *parseTextureNode(struct world *w, const cJSON *node);

static enum vecOp parseVectorOp(const cJSON *data) {
	if (!cJSON_IsString(data)) {
		logr(warning, "No vector op given, defaulting to add.\n");
		return VecAdd;
	};
	if (stringEquals(data->valuestring, "add")) return VecAdd;
	if (stringEquals(data->valuestring, "subtract")) return VecSubtract;
	if (stringEquals(data->valuestring, "multiply")) return VecMultiply;
	if (stringEquals(data->valuestring, "average")) return VecAverage;
	if (stringEquals(data->valuestring, "dot")) return VecDot;
	if (stringEquals(data->valuestring, "cross")) return VecCross;
	if (stringEquals(data->valuestring, "normalize")) return VecNormalize;
	if (stringEquals(data->valuestring, "reflect")) return VecReflect;
	if (stringEquals(data->valuestring, "length")) return VecLength;
	if (stringEquals(data->valuestring, "abs")) return VecAbs;
	return VecAdd;
}

static const struct vectorNode *parseVectorNode(struct world *w, const struct cJSON *node) {
	if (!node) return NULL;
	const cJSON *type = cJSON_GetObjectItem(node, "type");
	if (!cJSON_IsString(type)) {
		logr(warning, "No type provided for vectorNode.\n");
		return newConstantVector(w, vecZero());
	}

	if (stringEquals(type->valuestring, "vecmath")) {
		const struct vectorNode *a = parseVectorNode(w, cJSON_GetObjectItem(node, "vector1"));
		const struct vectorNode *b = parseVectorNode(w, cJSON_GetObjectItem(node, "vector2"));
		const enum vecOp op = parseVectorOp(cJSON_GetObjectItem(node, "op"));
		return newVecMath(w, a, b, op);
	}
	return NULL;
}

static const struct valueNode *parseValueNode(struct world *w, const cJSON *node) {
	if (!node) return NULL;
	if (cJSON_IsNumber(node)) {
		return newConstantValue(w, node->valuedouble);
	}

	const cJSON *type = cJSON_GetObjectItem(node, "type");
	if (!cJSON_IsString(type)) {
		logr(warning, "No type provided for valueNode.\n");
		return newConstantValue(w, 0.0f);
	}

	const struct valueNode *IOR = parseValueNode(w, cJSON_GetObjectItem(node, "IOR"));
	const struct vectorNode *normal = parseVectorNode(w, cJSON_GetObjectItem(node, "normal"));

	if (stringEquals(type->valuestring, "fresnel")) {
		return newFresnel(w, IOR, normal);
	}
	if (stringEquals(type->valuestring, "map_range")) {
		const struct valueNode *input_value = parseValueNode(w, cJSON_GetObjectItem(node, "input"));
		const struct valueNode *from_min = parseValueNode(w, cJSON_GetObjectItem(node, "from_min"));
		const struct valueNode *from_max = parseValueNode(w, cJSON_GetObjectItem(node, "from_max"));
		const struct valueNode *to_min = parseValueNode(w, cJSON_GetObjectItem(node, "to_min"));
		const struct valueNode *to_max = parseValueNode(w, cJSON_GetObjectItem(node, "to_max"));
		return newMapRange(w, input_value, from_min, from_max, to_min, to_max);
	}

	return newGrayscaleConverter(w, parseTextureNode(w, node));
}

static const struct colorNode *parseTextureNode(struct world *w, const cJSON *node) {
	if (!node) return NULL;

	if (cJSON_IsArray(node)) {
		return newConstantTexture(w, parseColor(node));
	}

	if (cJSON_IsString(node)) {
		// No options provided, go with defaults.
		return newImageTexture(w, loadTexture(node->valuestring, &w->nodePool), 0);
	}

	// Should be an object, then.
	ASSERT(cJSON_IsObject(node));

	// Handle options first
	uint8_t options = 0;
	options |= SRGB_TRANSFORM; // Enabled by default.
	// Do we want to do an srgb transform?
	const cJSON *srgbTransform = cJSON_GetObjectItem(node, "transform");
	if (srgbTransform) {
		if (!cJSON_IsTrue(srgbTransform)) {
			options &= ~SRGB_TRANSFORM;
		}
	}

	// Do we want bilinear interpolation enabled?
	const cJSON *lerp = cJSON_GetObjectItem(node, "lerp");
	if (!cJSON_IsTrue(lerp)) {
		options |= NO_BILINEAR;
	}

	//FIXME: No good way to know if it's a color, so just check if it's got "r" in there.
	const cJSON *red = cJSON_GetObjectItem(node, "r");
	if (red) {
		// This is actually still a color object.
		return newConstantTexture(w, parseColor(node));
	}
	const cJSON *type = cJSON_GetObjectItem(node, "type");
	if (cJSON_IsString(type)) {
		// Oo, what's this?
		if (stringEquals(type->valuestring, "checkerboard")) {
			const struct colorNode *a = parseTextureNode(w, cJSON_GetObjectItem(node, "color1"));
			const struct colorNode *b = parseTextureNode(w, cJSON_GetObjectItem(node, "color2"));
			const struct valueNode *scale = parseValueNode(w, cJSON_GetObjectItem(node, "scale"));
			return newCheckerBoardTexture(w, a, b, scale);
		}
		if (stringEquals(type->valuestring, "blackbody")) {
			const cJSON *degrees = cJSON_GetObjectItem(node, "degrees");
			ASSERT(cJSON_IsNumber(degrees));
			return newBlackbody(w, newConstantValue(w, degrees->valuedouble));
		}
		if (stringEquals(type->valuestring, "split")) {
			return newSplitValue(w, parseValueNode(w, cJSON_GetObjectItem(node, "constant")));
		}
		if (stringEquals(type->valuestring, "combine")) {
			const struct valueNode *r = parseValueNode(w, cJSON_GetObjectItem(node, "r"));
			const struct valueNode *g = parseValueNode(w, cJSON_GetObjectItem(node, "g"));
			const struct valueNode *b = parseValueNode(w, cJSON_GetObjectItem(node, "b"));
			return newCombineRGB(w, r, g, b);
		}
		if (stringEquals(type->valuestring, "to_color")) {
			//return newVecToColor(w, ...)
		}
	}

	const cJSON *path = cJSON_GetObjectItem(node, "path");
	if (cJSON_IsString(path)) {
		return newImageTexture(w, loadTexture(path->valuestring, &w->nodePool), options);
	}

	logr(warning, "Failed to parse textureNode. Here's a dump:\n");
	logr(warning, "\n%s\n", cJSON_Print(node));
	logr(warning, "Setting to an obnoxious pink material.\n");
	return unknownTextureNode(w);
}

static const struct bsdfNode *parseBsdfNode(struct world *w, const cJSON *node) {
	if (!node) return NULL;
	const cJSON *type = cJSON_GetObjectItem(node, "type");
	if (!cJSON_IsString(type)) {
		logr(warning, "No type provided for bsdfNode.\n");
		return warningBsdf(w);
	}

	const struct colorNode *color = parseTextureNode(w, cJSON_GetObjectItem(node, "color"));
	const struct valueNode *roughness = parseValueNode(w, cJSON_GetObjectItem(node, "roughness"));
	const struct valueNode *strength = parseValueNode(w, cJSON_GetObjectItem(node, "strength"));
	const struct valueNode *IOR = parseValueNode(w, cJSON_GetObjectItem(node, "IOR"));
	const struct valueNode *factor = parseValueNode(w, cJSON_GetObjectItem(node, "factor"));
	const struct bsdfNode *A = parseBsdfNode(w, cJSON_GetObjectItem(node, "A"));
	const struct bsdfNode *B = parseBsdfNode(w, cJSON_GetObjectItem(node, "B"));
	
	if (stringEquals(type->valuestring, "diffuse")) {
		return newDiffuse(w, color);
	} else if (stringEquals(type->valuestring, "metal")) {
		return newMetal(w, color, roughness);
	} else if (stringEquals(type->valuestring, "glass")) {
		return newGlass(w, color, roughness, IOR);
	} else if (stringEquals(type->valuestring, "plastic")) {
		return newPlastic(w, color);
	} else if (stringEquals(type->valuestring, "mix")) {
		return newMix(w, A, B, factor);
	} else if (stringEquals(type->valuestring, "add")) {
		return newAdd(w, A, B);
	} else if (stringEquals(type->valuestring, "transparent")) {
		return newTransparent(w, color);
	} else if (stringEquals(type->valuestring, "emissive")) {
		return newEmission(w, color, strength);
	}
	
	logr(warning, "Failed to parse node. Here's a dump:\n");
	logr(warning, "\n%s\n", cJSON_Print(node));
	logr(warning, "Setting to an obnoxious pink material.\n");
	return warningBsdf(w);
}

//FIXME: Only parse everything else if the mesh is found and is valid
static void parseMesh(struct renderer *r, const cJSON *data, int idx, int meshCount) {
	const cJSON *fileName = cJSON_GetObjectItem(data, "fileName");
	
	const cJSON *bsdf = cJSON_GetObjectItem(data, "bsdf");
	const cJSON *intensity = cJSON_GetObjectItem(data, "intensity");
	const cJSON *roughness = cJSON_GetObjectItem(data, "roughness");
	const cJSON *density = cJSON_GetObjectItem(data, "density");
	enum bsdfType type = lambertian;
	
	//TODO: Wrap this into a function
	if (cJSON_IsString(bsdf)) {
		if (stringEquals(bsdf->valuestring, "metal")) {
			type = metal;
		} else if (stringEquals(bsdf->valuestring, "glass")) {
			type = glass;
		} else if (stringEquals(bsdf->valuestring, "plastic")) {
			type = plastic;
		} else if (stringEquals(bsdf->valuestring, "emissive")) {
			type = emission;
		} else {
			type = lambertian;
		}
	} else {
		logr(warning, "Invalid bsdf while parsing mesh\n");
	}
	
	bool meshValid = false;
	if (fileName != NULL && cJSON_IsString(fileName)) {
		char *fullPath = stringConcat(r->prefs.assetPath, fileName->valuestring);
		windowsFixPath(fullPath);
		bool success = false;
		struct timeval timer;
		startTimer(&timer);
		success = loadMeshNew(r, fullPath, idx, meshCount);
		long us = getUs(timer);
		if (success) {
			long ms = us / 1000;
			logr(debug, "Parsing mesh %-35s took %zu %s\n", lastMesh(r)->name, ms > 0 ? ms : us, ms > 0 ? "ms" : "μs");
		}
		if (success) {
			meshValid = true;
			free(fullPath);
		} else {
			free(fullPath);
			return;
		}
	}
	
	if (meshValid) {
		const cJSON *instances = cJSON_GetObjectItem(data, "instances");
		const cJSON *instance = NULL;
		if (instances != NULL && cJSON_IsArray(instances)) {
			cJSON_ArrayForEach(instance, instances) {
				struct instance new = density ? newMeshVolume(lastMesh(r), density->valuedouble) : newMeshSolid(lastMesh(r));
				new.composite = parseInstanceTransform(instance);
				addInstanceToScene(r->scene, new);
			}
		}
		
		const cJSON *materials = cJSON_GetObjectItem(data, "material");
		if (materials) {
			struct cJSON *material = NULL;
			if (cJSON_IsArray(materials)) {
				// Array of graphs, so map them to mesh materials.
				ASSERT(cJSON_GetArraySize(materials) <= lastMesh(r)->materialCount);
				size_t i = 0;
				cJSON_ArrayForEach(material, materials) {
					lastMesh(r)->materials[i++].bsdf = parseBsdfNode(r->scene, material);
				}
			} else {
				// Single graph, map it to every material in a mesh.
				const struct bsdfNode *node = parseBsdfNode(r->scene, materials);
				for (int i = 0; i < lastMesh(r)->materialCount; ++i) {
					lastMesh(r)->materials[i].bsdf = node;
				}
			}
		} else {
			// Fallback, this is the old way of assigning materials.
			//FIXME: Delet this.
			for (int i = 0; i < lastMesh(r)->materialCount; ++i) {
				lastMesh(r)->materials[i].type = type;
				if (type == emission && intensity) {
					lastMesh(r)->materials[i].emission = colorCoef(intensity->valuedouble, lastMesh(r)->materials[i].diffuse);
				}
				if (type == glass) {
					const cJSON *IOR = cJSON_GetObjectItem(data, "IOR");
					if (cJSON_IsNumber(IOR)) {
						lastMesh(r)->materials[i].IOR = IOR->valuedouble;
					}
				} else if (type == plastic) {
					lastMesh(r)->materials[i].IOR = 1.45;
				}
				if (cJSON_IsNumber(roughness)) lastMesh(r)->materials[i].roughness = roughness->valuedouble;
				assignBSDF(r->scene, &lastMesh(r)->materials[i]);
			}
		}
	}
}

static void parseMeshes(struct renderer *r, const cJSON *data) {
	if (!cJSON_IsArray(data)) return;
	const cJSON *mesh = NULL;
	int idx = 1;
	//FIXME: This doesn't account for wavefront files with multiple meshes.
	int meshCount = cJSON_GetArraySize(data);
	r->scene->meshes = calloc(meshCount, sizeof(*r->scene->meshes));
	if (data != NULL && cJSON_IsArray(data)) {
		cJSON_ArrayForEach(mesh, data) {
			parseMesh(r, mesh, idx, meshCount);
			idx++;
		}
	}
}

/*static struct vector parseCoordinate(const cJSON *data) {
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
}*/

static void parseSphere(struct renderer *r, const cJSON *data) {
	const cJSON *color = NULL;
	const cJSON *roughness = NULL;
	const cJSON *IOR = NULL;
	const cJSON *radius = NULL;
	const cJSON *intensity = NULL;
	
	struct sphere newSphere = defaultSphere();
	
	const cJSON *bsdf = cJSON_GetObjectItem(data, "bsdf");
	
	//TODO: Break this out to a function
	if (cJSON_IsString(bsdf)) {
		if (stringEquals(bsdf->valuestring, "lambertian")) {
			newSphere.material.type = lambertian;
		} else if (stringEquals(bsdf->valuestring, "metal")) {
			newSphere.material.type = metal;
		} else if (stringEquals(bsdf->valuestring, "glass")) {
			newSphere.material.type = glass;
		} else if (stringEquals(bsdf->valuestring, "plastic")) {
			newSphere.material.type = plastic;
		} else if (stringEquals(bsdf->valuestring, "emissive")) {
			newSphere.material.type = emission;
		}
	} else {
		logr(warning, "Sphere BSDF not found, defaulting to lambertian.\n");
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
	
	const cJSON *density = cJSON_GetObjectItem(data, "density");
	
	const cJSON *instances = cJSON_GetObjectItem(data, "instances");
	const cJSON *instance = NULL;
	if (cJSON_IsArray(instances)) {
		cJSON_ArrayForEach(instance, instances) {
			addInstanceToScene(r->scene, density ? newSphereVolume(lastSphere(r), density->valuedouble) : newSphereSolid(lastSphere(r)));
			lastInstance(r)->composite = parseInstanceTransform(instance);
		}
	}

	const cJSON *materials = cJSON_GetObjectItem(data, "material");
	if (materials) {
		// Single graph, map it to every material in a mesh.
		lastSphere(r)->material.bsdf = parseBsdfNode(r->scene, materials);
	} else {
		assignBSDF(r->scene, &lastSphere(r)->material);
	}
}

static void parsePrimitive(struct renderer *r, const cJSON *data, int idx) {
	const cJSON *type = NULL;
	type = cJSON_GetObjectItem(data, "type");
	if (stringEquals(type->valuestring, "sphere")) {
		parseSphere(r, data);
	} else {
		logr(warning, "Unknown primitive type \"%s\" at index %i\n", type->valuestring, idx);
	}
}

static void parsePrimitives(struct renderer *r, const cJSON *data) {
	if (!cJSON_IsArray(data)) return;
	const cJSON *primitive = NULL;
	int primCount = cJSON_GetArraySize(data);
	r->scene->spheres = calloc(primCount, sizeof(*r->scene->spheres));
	
	if (data != NULL && cJSON_IsArray(data)) {
		int i = 0;
		cJSON_ArrayForEach(primitive, data) {
			parsePrimitive(r, primitive, i);
			i++;
		}
	}
}

static void parseScene(struct renderer *r, const cJSON *data) {
	parseAmbientColor(r, cJSON_GetObjectItem(data, "ambientColor"));
	parsePrimitives(r, cJSON_GetObjectItem(data, "primitives"));
	parseMeshes(r, cJSON_GetObjectItem(data, "meshes"));
}

int parseJSON(struct renderer *r, char *input) {
	cJSON *json = cJSON_Parse(input);
	if (!json) {
		logr(warning, "Failed to parse JSON\n");
		const char *errptr = cJSON_GetErrorPtr();
		if (errptr != NULL) {
			logr(warning, "Error before: %s\n", errptr);
			return -2;
		}
	}

	parsePrefs(&r->prefs, cJSON_GetObjectItem(json, "renderer"));

	if (isSet("output_path")) {
		char *path = stringPref("output_path");
		logr(info, "Overriding output path to %s\n", path);
		free(r->prefs.imgFileName);
		free(r->prefs.imgFilePath);
		r->prefs.imgFilePath = getFilePath(path);
		r->prefs.imgFileName = getFileName(path);
	}

	parseDisplay(&r->prefs, cJSON_GetObjectItem(json, "display"));
	parseCameras(&r->scene->cameras, &r->scene->camera_count, cJSON_GetObjectItem(json, "camera"));

	for (size_t i = 0; i < r->scene->camera_count; ++i) {
		r->scene->cameras[i].width = r->prefs.imageWidth;
		r->scene->cameras[i].height = r->prefs.imageHeight;
		cam_recompute_optics(&r->scene->cameras[i]);
	}

	const cJSON *selected_camera = cJSON_GetObjectItem(json, "selected_camera");
	if (cJSON_IsNumber(selected_camera)) {
		size_t selection = selected_camera->valueint;
		r->prefs.selected_camera = selection < r->scene->camera_count ? selection : r->scene->camera_count - 1;
	}

	parseScene(r, cJSON_GetObjectItem(json, "scene"));
	cJSON_Delete(json);
	
	return 0;
}
