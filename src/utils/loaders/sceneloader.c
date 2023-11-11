//
//  sceneloader.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2023 Valtteri Koskivuori. All rights reserved.
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
#include "../../vendored/cJSON.h"
#ifdef WINDOWS // Sigh...
#include <malloc.h>
#else
#include <alloca.h>
#endif
#include <stdlib.h>
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
#include "../../nodes/valuenode.h"
#include "../../nodes/colornode.h"
#include "../../nodes/shaders/emission.h"
#include "../../nodes/textures/constant.h"
#include "../../nodes/valuenode.h"
#include "meshloader.h"

struct transform parseTransformComposite(const cJSON *transforms);

static struct transform parseTransform(const cJSON *data, char *targetName) {
	cJSON *type = cJSON_GetObjectItem(data, "type");
	if (!cJSON_IsString(type)) {
		logr(warning, "Failed to parse transform! No type found\n");
		logr(warning, "Transform data: %s\n", cJSON_Print(data));
		return tform_new_translate(0.0f, 0.0f, 0.0f);
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
			return tform_new_rot_x(deg_to_rad(degrees->valuedouble));
		} else if (validRadians) {
			return tform_new_rot_x(radians->valuedouble);
		} else {
			logr(warning, "Found rotateX transform for object \"%s\" with no valid degrees or radians value given.\n", targetName);
		}
	} else if (stringEquals(type->valuestring, "rotateY")) {
		if (validDegrees) {
			return tform_new_rot_y(deg_to_rad(degrees->valuedouble));
		} else if (validRadians) {
			return tform_new_rot_y(radians->valuedouble);
		} else {
			logr(warning, "Found rotateY transform for object \"%s\" with no valid degrees or radians value given.\n", targetName);
		}
	} else if (stringEquals(type->valuestring, "rotateZ")) {
		if (validDegrees) {
			return tform_new_rot_z(deg_to_rad(degrees->valuedouble));
		} else if (validRadians) {
			return tform_new_rot_z(radians->valuedouble);
		} else {
			logr(warning, "Found rotateZ transform for object \"%s\" with no valid degrees or radians value given.\n", targetName);
		}
	} else if (stringEquals(type->valuestring, "translate")) {
		if (validCoords > 0) {
			return tform_new_translate(Xval, Yval, Zval);
		} else {
			logr(warning, "Found translate transform for object \"%s\" with less than 1 valid coordinate given.\n", targetName);
		}
	} else if (stringEquals(type->valuestring, "scale")) {
		if (validCoords > 0) {
			return tform_new_scale3(Xval, Yval, Zval);
		} else {
			logr(warning, "Found scale transform for object \"%s\" with less than 1 valid scale value given.\n", targetName);
		}
	} else if (stringEquals(type->valuestring, "scaleUniform")) {
		if (validScale) {
			return tform_new_scale(scale->valuedouble);
		} else {
			logr(warning, "Found scaleUniform transform for object \"%s\" with no valid scale value given.\n", targetName);
		}
	} else {
		logr(warning, "Found an invalid transform \"%s\" for object \"%s\"\n", type->valuestring, targetName);
	}
	
	//Hack. This is essentially just a NOP transform that does nothing.
	return tform_new_translate(0.0f, 0.0f, 0.0f);
}

void parsePrefs(struct prefs *prefs, const cJSON *data) {
	if (!data) return;

	const cJSON *threads = cJSON_GetObjectItem(data, "threads");
	if (threads) {
		if (cJSON_IsNumber(threads)) {
			if (threads->valueint > 0) {
				prefs->threads = threads->valueint;
				prefs->fromSystem = false;
			} else {
				prefs->threads = getSysCores() + 2;
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
		size_t threads = intPref("thread_override");
		if (prefs->threads != threads) {
			logr(info, "Overriding thread count to %zu\n", threads);
			prefs->threads = threads;
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

static void parseDisplay(struct sdl_prefs *win, const cJSON *data) {
	if (!data) return;

	const cJSON *enabled = cJSON_GetObjectItem(data, "enabled");
	if (enabled) {
		if (cJSON_IsBool(enabled)) {
			win->enabled = cJSON_IsTrue(enabled);
		} else {
			logr(warning, "Invalid enabled while parsing display prefs.\n");
		}
	}

	const cJSON *isFullscreen = cJSON_GetObjectItem(data, "isFullscreen");
	if (isFullscreen) {
		if (cJSON_IsBool(isFullscreen)) {
			win->fullscreen = cJSON_IsTrue(isFullscreen);
		} else {
			logr(warning, "Invalid isFullscreen while parsing display prefs.\n");
		}
	}

	const cJSON *isBorderless = cJSON_GetObjectItem(data, "isBorderless");
	if (isBorderless) {
		if (cJSON_IsBool(isBorderless)) {
			win->borderless = cJSON_IsTrue(isBorderless);
		} else {
			logr(warning, "Invalid isBorderless while parsing display prefs.\n");
		}
	}

	const cJSON *windowScale = cJSON_GetObjectItem(data, "windowScale");
	if (windowScale) {
		if (cJSON_IsNumber(windowScale)) {
			if (windowScale->valuedouble >= 0) {
				win->scale = windowScale->valuedouble;
			} else {
				win->scale = 1.0f;
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
		return deg_to_rad(degrees->valuedouble);
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
		const float r = cJSON_IsNumber(cJSON_GetArrayItem(data, 0)) ? (float)cJSON_GetArrayItem(data, 0)->valuedouble : 0.0f;
		const float g = cJSON_IsNumber(cJSON_GetArrayItem(data, 1)) ? (float)cJSON_GetArrayItem(data, 1)->valuedouble : 0.0f;
		const float b = cJSON_IsNumber(cJSON_GetArrayItem(data, 2)) ? (float)cJSON_GetArrayItem(data, 2)->valuedouble : 0.0f;
		const float a = cJSON_IsNumber(cJSON_GetArrayItem(data, 3)) ? (float)cJSON_GetArrayItem(data, 3)->valuedouble : 1.0f;
		return (struct color){ r, g, b, a };
	}
	
	ASSERT(cJSON_IsObject(data));
	
	const cJSON *kelvin = cJSON_GetObjectItem(data, "blackbody");
	if (cJSON_IsNumber(kelvin)) return colorForKelvin(kelvin->valuedouble);

	const cJSON *H = cJSON_GetObjectItem(data, "h");
	const cJSON *S = cJSON_GetObjectItem(data, "s");
	const cJSON *L = cJSON_GetObjectItem(data, "l");

	if (cJSON_IsNumber(H) && cJSON_IsNumber(S) && cJSON_IsNumber(L)) {
		return color_from_hsl(H->valuedouble, S->valuedouble, L->valuedouble);
	}

	const cJSON *R = cJSON_GetObjectItem(data, "r");
	const cJSON *G = cJSON_GetObjectItem(data, "g");
	const cJSON *B = cJSON_GetObjectItem(data, "b");
	const cJSON *A = cJSON_GetObjectItem(data, "a");

	return (struct color){
		cJSON_IsNumber(R) ? (float)R->valuedouble : 0.0f,
		cJSON_IsNumber(G) ? (float)G->valuedouble : 0.0f,
		cJSON_IsNumber(B) ? (float)B->valuedouble : 0.0f,
		cJSON_IsNumber(A) ? (float)A->valuedouble : 1.0f,
	};
}

//FIXME: Convert this to use parseBsdfNode
static void parseAmbientColor(struct renderer *r, const cJSON *data) {
	const cJSON *offset = cJSON_GetObjectItem(data, "offset");
	if (cJSON_IsNumber(offset)) {
		r->scene->backgroundOffset = deg_to_rad(offset->valuedouble) / 4.0f;
	}

	const cJSON *down = cJSON_GetObjectItem(data, "down");
	const cJSON *up = cJSON_GetObjectItem(data, "up");
	const cJSON *hdr = cJSON_GetObjectItem(data, "hdr");

	if (cJSON_IsString(hdr)) {
		char *fullPath = stringConcat(r->prefs.assetPath, hdr->valuestring);
		if (is_valid_file(fullPath, r->state.file_cache)) {
			r->scene->background = newBackground(&r->scene->storage, newImageTexture(&r->scene->storage, load_texture(fullPath, &r->scene->storage.node_pool, r->state.file_cache), 0), NULL);
			free(fullPath);
			return;
		}
		free(fullPath);
	}
	
	if (down && up) {
		r->scene->background = newBackground(&r->scene->storage, newGradientTexture(&r->scene->storage, parseColor(down), parseColor(up)), NULL);
		return;
	}

	r->scene->background = newBackground(&r->scene->storage, NULL, NULL);
}

struct transform parse_composite_transform(const cJSON *transforms) {
	if (!transforms) return tform_new();
	//TODO: Pass mesh/instance name as targetName for logging
	if (!cJSON_IsArray(transforms)) return parseTransform(transforms, "compositeBuilder");
	
	struct transform composite = tform_new();

	const cJSON *transform = NULL;
	cJSON_ArrayForEach(transform, transforms) {
		const cJSON *type = cJSON_GetObjectItem(transform, "type");
		if (!cJSON_IsString(type)) break;
		if (stringStartsWith("translate", type->valuestring)) {
			composite.A = mat_mul(composite.A, parseTransform(transform, "translates").A);
		}
	}
	cJSON_ArrayForEach(transform, transforms) {
		const cJSON *type = cJSON_GetObjectItem(transform, "type");
		if (!cJSON_IsString(type)) break;
		if (stringStartsWith("rotate", type->valuestring)) {
			composite.A = mat_mul(composite.A, parseTransform(transform, "translates").A);
		}
	}
	cJSON_ArrayForEach(transform, transforms) {
		const cJSON *type = cJSON_GetObjectItem(transform, "type");
		if (!cJSON_IsString(type)) break;
		if (stringStartsWith("scale", type->valuestring)) {
			composite.A = mat_mul(composite.A, parseTransform(transform, "translates").A);
		}
	}
	composite.Ainv = mat_invert(composite.A);
	return composite;
}

struct mtl_override {
	char *name;
	const struct bsdfNode *bsdf;
};
typedef struct mtl_override mtl_override;
dyn_array_dec(mtl_override);
dyn_array_def(mtl_override);

void mtl_override_free(struct mtl_override *o) {
	if (o->name) {
		free(o->name);
		o->name = NULL;
	}
}

struct mtl_override_arr parse_override_list(struct renderer *r, const cJSON *list) {
	struct mtl_override_arr overrides = { 0 };
	overrides.elem_free = mtl_override_free;
	if (!list || !cJSON_IsArray(list)) return overrides;
	cJSON *material = NULL;
	cJSON_ArrayForEach(material, list) {
		cJSON *name = cJSON_GetObjectItem(material, "replace");
		if (!cJSON_IsString(name)) continue;
		
		mtl_override_arr_add(&overrides, (struct mtl_override){
			.bsdf = parseBsdfNode(r->prefs.assetPath, r->state.file_cache, &r->scene->storage, material),
			.name = stringCopy(name->valuestring)
		});
	}
	return overrides;
}

static void parse_mesh(struct renderer *r, const cJSON *data, int idx, int mesh_file_count) {
	const cJSON *file_name = cJSON_GetObjectItem(data, "fileName");
	if (!cJSON_IsString(file_name)) return;
	//FIXME: This concat + path fixing should be an utility function
	char *fullPath = stringConcat(r->prefs.assetPath, file_name->valuestring);
	windowsFixPath(fullPath);

	logr(plain, "\r");
	logr(info, "Loading mesh file %i/%i%s", idx + 1, mesh_file_count, (idx + 1) == mesh_file_count ? "\n" : "\r");
	struct timeval timer;
	timer_start(&timer);
	//FIXME: A new asset type that contains meshes, materials, etc separately would make this much more flexible.
	struct mesh_arr meshes = load_meshes_from_file(fullPath, r->state.file_cache);
	long us = timer_get_us(timer);
	free(fullPath);
	long ms = us / 1000;
	logr(debug, "Parsing file %-35s took %zu %s\n", file_name->valuestring, ms > 0 ? ms : us, ms > 0 ? "ms" : "μs");

	if (!meshes.count) return;

	// Per JSON 'meshes' array element, these apply to materials before we assign them to instances
	struct mtl_override_arr global_overrides = parse_override_list(r, cJSON_GetObjectItem(data, "materials"));

	// Precompute guessed bsdfs for these instances
	struct bsdf_buffer *file_bsdfs = bsdf_buf_ref(NULL);
	logr(debug, "Figuring out bsdfs for mtllib materials\n");
	// FIXME: 0 index hack relies on wavefront parser behaviour that may change
	struct material_arr file_mats = meshes.items[0].mbuf->materials;
	for (size_t i = 0; i < file_mats.count; ++i) {
		const struct bsdfNode *match = NULL;
		for (size_t j = 0; j < global_overrides.count; ++j) {
			if (stringEquals(file_mats.items[i].name, global_overrides.items[j].name)) {
				match = global_overrides.items[j].bsdf;
			}
		}
		if (match) {
			bsdf_node_ptr_arr_add(&file_bsdfs->bsdfs, match);
		} else {
			bsdf_node_ptr_arr_add(&file_bsdfs->bsdfs, try_to_guess_bsdf(&r->scene->storage, &file_mats.items[i]));
		}
	}
	// TODO: callback for arrays to destroy each element?
	// This leaks 'name'
	mtl_override_arr_free(&global_overrides);

	// Now apply some slightly overcomplicated logic to choose instances to add to the scene.
	// It boils down to:
	// - If a 'pick_instances' array is found, only add those instances that were specified.
	// - If a 'add_instances' array is found, add one instance for each mesh + additional ones in array
	// - If neither are found, just add one instance for every mesh.
	// - If both are found, emit warning and bail out.

	const cJSON *pick_instances = cJSON_GetObjectItem(data, "pick_instances");
	const cJSON *add_instances = cJSON_GetObjectItem(data, "add_instances");

	if (pick_instances && add_instances) {
		logr(warning, "Can't combine pick_instances and add_instances (%s)\n", file_name->valuestring);
		goto done;
	}

	size_t current_mesh_count = r->scene->meshes.count;
	if (!cJSON_IsArray(pick_instances)) {
		// Generate one instance for every mesh, identity transform.
		for (size_t i = 0; i < meshes.count; ++i) {
			struct instance new = new_mesh_instance(&r->scene->meshes, current_mesh_count + i, NULL, NULL);
			new.bbuf = bsdf_buf_ref(file_bsdfs);
			instance_arr_add(&r->scene->instances, new);
		}
		goto done;
	}

	const cJSON *instances = pick_instances ? pick_instances : add_instances;
	if (!cJSON_IsArray(instances)) goto done;

	const cJSON *instance = NULL;
	cJSON_ArrayForEach(instance, instances) {
		const cJSON *mesh_name = cJSON_GetObjectItem(instance, "for");
		ssize_t target_idx = -1;
		if (!cJSON_IsString(mesh_name)) {
			if (!pick_instances) continue;
			target_idx = 0;
		} else {
			for (size_t i = 0; i < meshes.count; ++i) {
				if (stringEquals(mesh_name->valuestring, meshes.items[i].name)) {
					target_idx = i;
				}
			}
		}
		if (target_idx < 0) continue;
		struct instance new = { 0 };
		const cJSON *density = cJSON_GetObjectItem(instance, "density");
		if (cJSON_IsNumber(density)) {
			new = new_mesh_instance(&r->scene->meshes, current_mesh_count + target_idx, (float *)&density->valuedouble, &r->scene->storage.node_pool);
		} else {
			//FIXME: Make newMesh*() and newSphere*() const
			new = new_mesh_instance(&r->scene->meshes, current_mesh_count + target_idx, NULL, NULL);
		}
		new.bbuf = bsdf_buf_ref(NULL);

		struct mtl_override_arr instance_overrides = parse_override_list(r, cJSON_GetObjectItem(instance, "materials"));
		for (size_t i = 0; i < file_mats.count; ++i) {
			const struct bsdfNode *override_match = NULL;
			for (size_t j = 0; j < instance_overrides.count; ++j) {
				if (stringEquals(file_mats.items[i].name, instance_overrides.items[j].name)) {
					override_match = instance_overrides.items[j].bsdf;
				}
			}
			bsdf_node_ptr_arr_add(&new.bbuf->bsdfs, override_match ? override_match : file_bsdfs->bsdfs.items[i]);
		}
		mtl_override_arr_free(&instance_overrides);

		new.composite = parse_composite_transform(cJSON_GetObjectItem(instance, "transforms"));
		instance_arr_add(&r->scene->instances, new);
	}
done:

	bsdf_buf_unref(file_bsdfs);
	// Store meshes
	logr(debug, "Adding %zu meshes\n", meshes.count);
	for (size_t i = 0; i < meshes.count; ++i) {
		mesh_arr_add(&r->scene->meshes, meshes.items[i]);
	}

	mesh_arr_free(&meshes);
}

static void parse_meshes(struct renderer *r, const cJSON *data) {
	if (!cJSON_IsArray(data)) return;
	int idx = 0;
	int mesh_file_count = cJSON_GetArraySize(data);
	const cJSON *mesh = NULL;
	cJSON_ArrayForEach(mesh, data) {
		parse_mesh(r, mesh, idx++, mesh_file_count);
	}
}

static void parse_sphere(struct renderer *r, const cJSON *data) {
	struct sphere new = { 0 };

	const cJSON *radius = NULL;
	radius = cJSON_GetObjectItem(data, "radius");
	if (radius != NULL && cJSON_IsNumber(radius)) {
		new.radius = radius->valuedouble;
	} else {
		new.radius = 1.0f;
		logr(warning, "No radius specified for sphere, setting to %.0f\n", (double)new.radius);
	}
	const size_t new_idx = sphere_arr_add(&r->scene->spheres, new);
	
	// Apply this to all instances that don't have their own "materials" object
	const cJSON *sphere_global_materials = cJSON_GetObjectItem(data, "material");
	
	const cJSON *instances = cJSON_GetObjectItem(data, "instances");
	const cJSON *instance = NULL;
	if (cJSON_IsArray(instances)) {
		cJSON_ArrayForEach(instance, instances) {
			const cJSON *density = cJSON_GetObjectItem(data, "density");

			struct instance new_instance = { 0 };
			if (cJSON_IsNumber(density)) {
				new_instance = new_sphere_instance(&r->scene->spheres, new_idx, (float *)&density->valuedouble, &r->scene->storage.node_pool);
			} else {
				new_instance = new_sphere_instance(&r->scene->spheres, new_idx, NULL, NULL);
			}
			new_instance.bbuf = bsdf_buf_ref(NULL);

			const cJSON *instance_materials = cJSON_GetObjectItem(instance, "materials");
			const cJSON *materials = instance_materials ? instance_materials : sphere_global_materials;

			if (materials) {
				const cJSON *material = NULL;
				if (cJSON_IsArray(materials)) {
					material = cJSON_GetArrayItem(materials, 0);
				} else {
					material = materials;
				}
				bsdf_node_ptr_arr_add(&new_instance.bbuf->bsdfs, parseBsdfNode(r->prefs.assetPath, r->state.file_cache, &r->scene->storage, material));
				const cJSON *type_string = cJSON_GetObjectItem(material, "type");
				if (type_string && stringEquals(type_string->valuestring, "emissive")) new_instance.emits_light = true;
			}

			if (!new_instance.bbuf->bsdfs.count) bsdf_node_ptr_arr_add(&new_instance.bbuf->bsdfs, warningBsdf(&r->scene->storage));
			new_instance.composite = parse_composite_transform(cJSON_GetObjectItem(instance, "transforms"));
			instance_arr_add(&r->scene->instances, new_instance);
		}
	}
}

static void parse_primitive(struct renderer *r, const cJSON *data, int idx) {
	const cJSON *type = NULL;
	type = cJSON_GetObjectItem(data, "type");
	if (stringEquals(type->valuestring, "sphere")) {
		parse_sphere(r, data);
	} else {
		logr(warning, "Unknown primitive type \"%s\" at index %i\n", type->valuestring, idx);
	}
}

static void parse_primitives(struct renderer *r, const cJSON *data) {
	if (!cJSON_IsArray(data)) return;
	if (data != NULL && cJSON_IsArray(data)) {
		int i = 0;
		const cJSON *primitive = NULL;
		cJSON_ArrayForEach(primitive, data) {
			parse_primitive(r, primitive, i++);
		}
	}
}

static void parseScene(struct renderer *r, const cJSON *data) {
	parseAmbientColor(r, cJSON_GetObjectItem(data, "ambientColor"));
	parse_primitives(r, cJSON_GetObjectItem(data, "primitives"));
	parse_meshes(r, cJSON_GetObjectItem(data, "meshes"));
}

int parseJSON(struct renderer *r, const cJSON *json) {

	parsePrefs(&r->prefs, cJSON_GetObjectItem(json, "renderer"));

	if (isSet("output_path")) {
		char *path = stringPref("output_path");
		logr(info, "Overriding output path to %s\n", path);
		free(r->prefs.imgFileName);
		free(r->prefs.imgFilePath);
		r->prefs.imgFilePath = get_file_path(path);
		r->prefs.imgFileName = get_file_name(path);
	}

	parseDisplay(&r->prefs.window, cJSON_GetObjectItem(json, "display"));
	parseCameras(&r->scene->cameras, &r->scene->camera_count, cJSON_GetObjectItem(json, "camera"));

	if (!r->scene->cameras) {
		logr(warning, "No cameras specified, nothing to render.\n");
		return -1;
	}

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
	
	return 0;
}
