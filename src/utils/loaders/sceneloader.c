//
//  sceneloader.c
//  C-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include "sceneloader.h"

//FIXME: We should only need to include c-ray.h here!

#include "../../datatypes/scene.h"
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
#include <string.h>
#include "../platform/capabilities.h"
#include "../../datatypes/image/imagefile.h"
#include "../../renderer/renderer.h"
#include "textureloader.h"
#include "../../renderer/instance.h"
#include "../../utils/args.h"
#include "../../utils/timer.h"
#include "../../utils/string.h"
#include "../../nodes/bsdfnode.h"
#include "meshloader.h"

struct transform parseTransformComposite(const cJSON *transforms);

static struct instance *lastInstance(struct renderer *r) {
	return &r->scene->instances[r->scene->instanceCount - 1];
}

static struct sphere *lastSphere(struct renderer *r) {
	return &r->scene->spheres[r->scene->sphereCount - 1];
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

	bool width_set = false;
	bool height_set = false;
	const cJSON *width = cJSON_GetObjectItem(data, "width");
	if (width) {
		if (cJSON_IsNumber(width)) {
			if (width->valueint >= 0) {
				prefs->override_width = width->valueint;
			} else {
				prefs->override_width = 640;
			}
			width_set = true;
		} else {
			logr(warning, "Invalid width while parsing scene.\n");
		}
	}

	const cJSON *height = cJSON_GetObjectItem(data, "height");
	if (height) {
		if (cJSON_IsNumber(height)) {
			if (height->valueint >= 0) {
				prefs->override_height = height->valueint;
			} else {
				prefs->override_height = 400;
			}
			height_set = true;
		} else {
			logr(warning, "Invalid height while parsing scene.\n");
		}
	}

	if (width_set && height_set) prefs->override_dimensions = true;

	const cJSON *fileType = cJSON_GetObjectItem(data, "fileType");
	if (fileType) {
		if (cJSON_IsString(fileType)) {
			if (stringEquals(fileType->valuestring, "bmp")) {
				prefs->imgType = bmp;
			} else if (stringEquals(fileType->valuestring, "png")) {
				prefs->imgType = png;
			} else if (stringEquals(fileType->valuestring, "qoi")) {
				prefs->imgType = qoi;
			} else {
				prefs->imgType = unknown;
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
			prefs->override_width = width;
			prefs->override_height = height;
			prefs->override_dimensions = true;
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
static struct euler_angles *parseRotations(const cJSON *transforms) {
	if (!transforms) return NULL;
	
	struct euler_angles rotations = { 0 };
	const cJSON *transform = NULL;
	cJSON_ArrayForEach(transform, transforms) {
		cJSON *type = cJSON_GetObjectItem(transform, "type");
		if (stringEquals(type->valuestring, "rotateX")) {
			rotations.roll = getRadians(transform);
		}
		if (stringEquals(type->valuestring, "rotateY")) {
			rotations.pitch = getRadians(transform);
		}
		if (stringEquals(type->valuestring, "rotateZ")) {
			rotations.yaw = getRadians(transform);
		}
	}
	
	struct euler_angles *angles = calloc(1, sizeof(*angles));
	*angles = rotations;
	
	return angles;
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
				cam.focus_distance = focalDistance->valuedouble;
			} else {
				cam.focus_distance = 0.0f;
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

	const cJSON *width = cJSON_GetObjectItem(data, "width");
	if (width) {
		if (cJSON_IsNumber(width)) {
			cam.width = width->valueint > 0 ? width->valueint : 640;
		} else {
			logr(warning, "Invalid sensor width while parsing camera.\n");
		}
	}

	const cJSON *height = cJSON_GetObjectItem(data, "height");
	if (height) {
		if (cJSON_IsNumber(height)) {
			cam.height = height->valueint > 0 ? height->valueint : 400;
		} else {
			logr(warning, "Invalid sensor height while parsing camera.\n");
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
	struct euler_angles *rotations = NULL;
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
	cam_recompute_optics(&cam);
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

struct color parseColor(const cJSON *data) {
	if (cJSON_IsArray(data)) {
		const float r = cJSON_IsNumber(cJSON_GetArrayItem(data, 0)) ? cJSON_GetArrayItem(data, 0)->valuedouble : 0.0f;
		const float g = cJSON_IsNumber(cJSON_GetArrayItem(data, 1)) ? cJSON_GetArrayItem(data, 1)->valuedouble : 0.0f;
		const float b = cJSON_IsNumber(cJSON_GetArrayItem(data, 2)) ? cJSON_GetArrayItem(data, 2)->valuedouble : 0.0f;
		const float a = cJSON_IsNumber(cJSON_GetArrayItem(data, 3)) ? cJSON_GetArrayItem(data, 3)->valuedouble : 1.0f;
		return (struct color){ r, g, b, a };
	}
	
	ASSERT(cJSON_IsObject(data));
	
	const cJSON *R = NULL;
	const cJSON *G = NULL;
	const cJSON *B = NULL;
	const cJSON *A = NULL;
	const cJSON *H = NULL;
	const cJSON *S = NULL;
	const cJSON *L = NULL;
	const cJSON *kelvin = NULL;
	
	kelvin = cJSON_GetObjectItem(data, "blackbody");
	if (cJSON_IsNumber(kelvin)) return colorForKelvin(kelvin->valuedouble);

	H = cJSON_GetObjectItem(data, "h");
	S = cJSON_GetObjectItem(data, "s");
	L = cJSON_GetObjectItem(data, "l");

	if (cJSON_IsNumber(H) && cJSON_IsNumber(S) && cJSON_IsNumber(L)) {
		return color_from_hsl(H->valuedouble, S->valuedouble, L->valuedouble);
	}

	R = cJSON_GetObjectItem(data, "r");
	G = cJSON_GetObjectItem(data, "g");
	B = cJSON_GetObjectItem(data, "b");
	A = cJSON_GetObjectItem(data, "a");

	return (struct color){
		cJSON_IsNumber(R) ? R->valuedouble : 0.0f,
		cJSON_IsNumber(G) ? G->valuedouble : 0.0f,
		cJSON_IsNumber(B) ? B->valuedouble : 0.0f,
		cJSON_IsNumber(A) ? A->valuedouble : 1.0f,
	};
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
		if (isValidFile(fullPath, r->state.file_cache)) {
			r->scene->background = newBackground(&r->scene->storage, newImageTexture(&r->scene->storage, load_texture(fullPath, &r->scene->storage.node_pool, r->state.file_cache), 0), NULL);
			free(fullPath);
			return;
		}
	}
	
	if (down && up) {
		r->scene->background = newBackground(&r->scene->storage, newGradientTexture(&r->scene->storage, parseColor(down), parseColor(up)), NULL);
		return;
	}

	r->scene->background = newBackground(&r->scene->storage, NULL, NULL);
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
			composite.A = multiplyMatrices(composite.A, tforms[i].A);
		}
	}
	
	// Rotates
	for (size_t i = 0; i < count; ++i) {
		if (isRotation(&tforms[i])) {
			composite.A = multiplyMatrices(composite.A, tforms[i].A);
		}
	}
	
	// Scales
	for (size_t i = 0; i < count; ++i) {
		if (isScale(&tforms[i])) {
			composite.A = multiplyMatrices(composite.A, tforms[i].A);
		}
	}
	
	composite.Ainv = inverseMatrix(composite.A);
	composite.type = transformTypeComposite;
	free(tforms);
	return composite;
}

static struct transform parseInstanceTransform(const cJSON *instance) {
	const cJSON *transforms = cJSON_GetObjectItem(instance, "transforms");
	return parseTransformComposite(transforms);
}

void apply_materials_to_instance(struct renderer *r, struct instance *instance, const cJSON *materials) {
	for (size_t i = 0; i < (size_t)instance->material_count; ++i) {
		try_to_guess_bsdf(&r->scene->storage, &instance->materials[i]);
	}
	if (!materials) return;
	struct cJSON *material = NULL;
	if (cJSON_IsArray(materials)) {
		// Array of graphs, so map them to mesh materials.
		ASSERT((size_t)cJSON_GetArraySize(materials) <= instance->material_count);
		size_t i = 0;
		cJSON_ArrayForEach(material, materials) {
			size_t old_i = i;
			if (cJSON_HasObjectItem(material, "replace")) {
				// Replace specific material
				const cJSON *name = cJSON_GetObjectItem(material, "replace");
				struct instance *m = instance;
				bool found = false;
				for (size_t j = 0; j < m->material_count && !found; ++j) {
					if (stringEquals(m->materials[j].name, name->valuestring)) {
						i = j;
						found = true;
					}
				}
				if (!found) goto skip;
			}
			instance->materials[i].bsdf = parseBsdfNode(r->prefs.assetPath, r->state.file_cache, &r->scene->storage, material);
			//FIXME: Hack
			cJSON *type_string = cJSON_GetObjectItem(material, "type");
			if (type_string && stringEquals(type_string->valuestring, "emissive")) {
				cJSON *color = cJSON_GetObjectItem(material, "color");
				cJSON *strength = cJSON_GetObjectItem(material, "strength");
				instance->materials[i].emission = colorCoef(strength->valuedouble, parseColor(color));
			}
			ASSERT(instance->materials[i].bsdf);
			skip:
			i = old_i;
			i++;
		}
	} else {
		// Single graph, map it to every material in a mesh.
		const struct bsdfNode *node = parseBsdfNode(r->prefs.assetPath, r->state.file_cache, &r->scene->storage, materials);
		for (size_t i = 0; i < instance->material_count; ++i) {
			instance->materials[i].bsdf = node;
		}
	}
}

static void parse_mesh_instances(struct renderer *r, const cJSON *data, struct mesh *meshes, size_t mesh_count) {
	const cJSON *file_name = cJSON_GetObjectItem(data, "fileName");
	const cJSON *instances = cJSON_GetObjectItem(data, "instances");
	if (cJSON_GetArraySize(instances) < 1) {
		logr(warning, "Mesh file %s has no instances. It will not appear.\n", file_name->valuestring);
	}

	const cJSON *bsdf_fallback = cJSON_GetObjectItem(data, "bsdf");
	if (bsdf_fallback) {
		logr(warning, "Old mesh-global bsdf type provided for %s.\n", file_name->valuestring);
		logr(warning, "This is no longer supported. Use global or instance local material graphs instead.\n");
	}

	// Mesh file global material override list. Apply if instance local one is not provided.
	const cJSON *mesh_global_materials = cJSON_GetObjectItem(data, "materials");

	for (size_t i = 0; i < mesh_count; ++i) {
		const cJSON *instance = NULL;
		cJSON_ArrayForEach(instance, instances) {
			const cJSON *density = cJSON_GetObjectItem(instance, "density");
			const cJSON *mesh_name = cJSON_GetObjectItem(instance, "for");
			if (cJSON_IsString(mesh_name) && !stringEquals(mesh_name->valuestring, meshes[i].name)) break;
			struct instance new;
			if (cJSON_IsNumber(density)) {
				new = newMeshVolume(&meshes[i], density->valuedouble, &r->scene->storage.node_pool);
			} else {
				//FIXME: Make newMesh*() and newSphere*() const
				new = newMeshSolid(&meshes[i]);
			}

			const cJSON *instance_materials = cJSON_GetObjectItem(instance, "materials");
			const cJSON *materials = instance_materials ? instance_materials : mesh_global_materials;

			new.material_count = meshes[i].materialCount;
			new.materials = calloc(new.material_count, sizeof(*new.materials));//malloc(new.material_count * sizeof(*new.materials));
			memcpy(new.materials, meshes[i].materials, new.material_count * sizeof(*new.materials));

			apply_materials_to_instance(r, &new, materials);
			new.composite = parseInstanceTransform(instance);
			addInstanceToScene(r->scene, new);
		}
	}
}

static size_t parse_mesh(struct renderer *r, const cJSON *data, int idx, int mesh_file_count) {
	const cJSON *file_name = cJSON_GetObjectItem(data, "fileName");

	if (!cJSON_IsString(file_name)) return 0;
	//FIXME: This concat + path fixing should be an utility function
	char *fullPath = stringConcat(r->prefs.assetPath, file_name->valuestring);
	windowsFixPath(fullPath);

	logr(plain, "\r");
	logr(info, "Loading mesh file %i/%i%s", idx, mesh_file_count, idx == mesh_file_count ? "\n" : "\r");
	size_t valid_mesh_count = 0;
	struct timeval timer;
	timer_start(&timer);
	struct mesh *meshes = load_meshes_from_file(fullPath, &valid_mesh_count, r->state.file_cache);
	long us = timer_get_us(timer);
	free(fullPath);
	if (!meshes) return 0;

	long ms = us / 1000;
	logr(debug, "Parsing file %-35s took %zu %s\n", file_name->valuestring, ms > 0 ? ms : us, ms > 0 ? "ms" : "μs");

	//TODO: Implement a more ergonomic array implementation
	//Also let's not use an array in this particular case, instead use a hashtable.
	r->scene->meshes = realloc(r->scene->meshes, (r->scene->meshCount + valid_mesh_count) * sizeof(*r->scene->meshes));
	memcpy(r->scene->meshes + r->scene->meshCount, meshes, valid_mesh_count * sizeof(*meshes));
	free(meshes);
	r->scene->meshCount += valid_mesh_count;

	return valid_mesh_count;
}

static void parseMeshes(struct renderer *r, const cJSON *data) {
	if (!cJSON_IsArray(data)) return;
	int idx = 1;
	int mesh_file_count = cJSON_GetArraySize(data);
	size_t *amounts = calloc(mesh_file_count, sizeof(*amounts));
	if (!cJSON_IsArray(data)) return;
	const cJSON *mesh = NULL;
	int i = 0;
	cJSON_ArrayForEach(mesh, data) {
		amounts[i++] = parse_mesh(r, mesh, idx, mesh_file_count);
		idx++;
	}
	i = 0;
	size_t current_offset = 0;
	cJSON_ArrayForEach(mesh, data) {
		parse_mesh_instances(r, mesh, r->scene->meshes + current_offset, amounts[i]);
		current_offset += amounts[i++];
	}
	free(amounts);
}

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
		newSphere.radius = 1.0f;
		logr(warning, "No radius specified for sphere, setting to %.0f\n", newSphere.radius);
	}
	
	//FIXME: Proper materials for spheres
	addSphere(r->scene, newSphere);
	
	const cJSON *density = cJSON_GetObjectItem(data, "density");
	
	const cJSON *instances = cJSON_GetObjectItem(data, "instances");
	const cJSON *instance = NULL;
	if (cJSON_IsArray(instances)) {
		cJSON_ArrayForEach(instance, instances) {
			addInstanceToScene(r->scene, density ? newSphereVolume(lastSphere(r), density->valuedouble, &r->scene->storage.node_pool) : newSphereSolid(lastSphere(r)));
			lastInstance(r)->composite = parseInstanceTransform(instance);
		}
	}

	const cJSON *materials = cJSON_GetObjectItem(data, "material");
	if (materials) {
		// Single graph, map it to every material in a mesh.
		lastSphere(r)->material.bsdf = parseBsdfNode(r->prefs.assetPath, r->state.file_cache, &r->scene->storage, materials);
	} else {
		try_to_guess_bsdf(&r->scene->storage, &lastSphere(r)->material);
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

int parseJSON(struct renderer *r, const char *input) {
	cJSON *json = cJSON_Parse(input);
	if (!json) {
		const char *errptr = cJSON_GetErrorPtr();
		if (errptr) {
			logr(warning, "Failed to parse JSON\n");
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

	if (r->prefs.override_dimensions) {
		for (size_t i = 0; i < r->scene->camera_count; ++i) {
			r->scene->cameras[i].width = r->prefs.override_width;
			r->scene->cameras[i].height = r->prefs.override_height;
			cam_recompute_optics(&r->scene->cameras[i]);
		}
	}

	const cJSON *selected_camera = cJSON_GetObjectItem(json, "selected_camera");
	if (cJSON_IsNumber(selected_camera)) {
		r->prefs.selected_camera = (size_t)selected_camera->valueint;
	}

	r->prefs.selected_camera = r->prefs.selected_camera < r->scene->camera_count ? r->prefs.selected_camera : r->scene->camera_count - 1;
	if (r->prefs.selected_camera != 0) logr(info, "Selecting camera %li\n", r->prefs.selected_camera);

	parseScene(r, cJSON_GetObjectItem(json, "scene"));
	cJSON_Delete(json);
	
	return 0;
}
