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
#include <c-ray/c-ray.h>

#include "../../datatypes/scene.h"
#include "../../datatypes/vector.h"
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
#include "../../utils/timer.h"
#include "../../utils/string.h"
#include "../../utils/platform/signal.h"
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

void parse_prefs(struct cr_renderer *ext, const cJSON *data) {
	if (!data || !ext) return;

	const cJSON *threads = cJSON_GetObjectItem(data, "threads");
	if (cJSON_IsNumber(threads) && threads->valueint > 0) {
		if (threads->valueint > 0) {
			cr_renderer_set_num_pref(ext, cr_renderer_threads, threads->valueint);
			// prefs->fromSystem = false;
		} else {
			cr_renderer_set_num_pref(ext, cr_renderer_threads, getSysCores() + 2);
			// prefs->fromSystem = true;
		}
	}

	const cJSON *samples = cJSON_GetObjectItem(data, "samples");
	if (cJSON_IsNumber(samples) && samples->valueint > 0)
		cr_renderer_set_num_pref(ext, cr_renderer_samples, samples->valueint);

	const cJSON *bounces = cJSON_GetObjectItem(data, "bounces");
	if (cJSON_IsNumber(bounces) && bounces->valueint >= 0)
		cr_renderer_set_num_pref(ext, cr_renderer_bounces, bounces->valueint);

	const cJSON *tile_width = cJSON_GetObjectItem(data, "tileWidth");
	if (cJSON_IsNumber(tile_width) && tile_width->valueint > 0)
		cr_renderer_set_num_pref(ext, cr_renderer_tile_width, tile_width->valueint);

	const cJSON *tile_height = cJSON_GetObjectItem(data, "tileHeight");
	if (cJSON_IsNumber(tile_height) && tile_height->valueint > 0)
		cr_renderer_set_num_pref(ext, cr_renderer_tile_height, tile_height->valueint);

	const cJSON *tile_order = cJSON_GetObjectItem(data, "tileOrder");
	if (cJSON_IsString(tile_order)) {
		cr_renderer_set_str_pref(ext, cr_renderer_tile_order, tile_order->valuestring);
	}

	const cJSON *file_path = cJSON_GetObjectItem(data, "outputFilePath");
	if (cJSON_IsString(file_path)) {
		cr_renderer_set_str_pref(ext, cr_renderer_output_path, file_path->valuestring);
	}

	const cJSON *file_name = cJSON_GetObjectItem(data, "outputFileName");
	if (cJSON_IsString(file_name)) {
		cr_renderer_set_str_pref(ext, cr_renderer_output_name, file_name->valuestring);
	}

	const cJSON *count = cJSON_GetObjectItem(data, "count");
	if (cJSON_IsNumber(count)) {
		cr_renderer_set_num_pref(ext, cr_renderer_output_num, count->valueint);
	}

	const cJSON *width = cJSON_GetObjectItem(data, "width");
	if (cJSON_IsNumber(width)) {
		cr_renderer_set_num_pref(ext, cr_renderer_override_width, width->valueint);
	}

	const cJSON *height = cJSON_GetObjectItem(data, "height");
	if (cJSON_IsNumber(height)) {
		cr_renderer_set_num_pref(ext, cr_renderer_override_height, height->valueint);
	}

	const cJSON *fileType = cJSON_GetObjectItem(data, "fileType");
	if (cJSON_IsString(fileType)) {
		cr_renderer_set_str_pref(ext, cr_renderer_output_filetype, fileType->valuestring);
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

static struct euler_angles parseRotations(const cJSON *transforms) {
	if (!transforms) return (struct euler_angles){ 0 };
	
	struct euler_angles angles = { 0 };
	const cJSON *transform = NULL;
	cJSON_ArrayForEach(transform, transforms) {
		cJSON *type = cJSON_GetObjectItem(transform, "type");
		if (stringEquals(type->valuestring, "rotateX")) {
			angles.roll = getRadians(transform);
		}
		if (stringEquals(type->valuestring, "rotateY")) {
			angles.pitch = getRadians(transform);
		}
		if (stringEquals(type->valuestring, "rotateZ")) {
			angles.yaw = getRadians(transform);
		}
	}
	
	return angles;
}

static struct vector parse_location(const cJSON *transforms) {
	if (!transforms) return (struct vector){ 0 };
	
	const cJSON *transform = NULL;
	cJSON_ArrayForEach(transform, transforms) {
		cJSON *type = cJSON_GetObjectItem(transform, "type");
		if (stringEquals(type->valuestring, "translate")) {
			const cJSON *x = cJSON_GetObjectItem(transform, "x");
			const cJSON *y = cJSON_GetObjectItem(transform, "y");
			const cJSON *z = cJSON_GetObjectItem(transform, "z");
			
			return (struct vector){x->valuedouble, y->valuedouble, z->valuedouble};
		}
	}
	return (struct vector){ 0 };
}

static void parse_camera(struct cr_scene *s, const cJSON *data) {
	if (!cJSON_IsObject(data)) return;
	cr_camera cam = cr_camera_new(s);

	const cJSON *FOV = cJSON_GetObjectItem(data, "FOV");
	if (cJSON_IsNumber(FOV) && FOV->valuedouble >= 0.0 && FOV->valuedouble < 180.0)
		cr_camera_set_num_pref(s, cam, cr_camera_fov, FOV->valuedouble);

	const cJSON *focus_dist = cJSON_GetObjectItem(data, "focalDistance"); //FIXME: Rename in json
	if (cJSON_IsNumber(focus_dist) && focus_dist->valuedouble >= 0.0)
		cr_camera_set_num_pref(s, cam, cr_camera_focus_distance, focus_dist->valuedouble);

	const cJSON *fstops = cJSON_GetObjectItem(data, "fstops");
	if (cJSON_IsNumber(fstops) && fstops->valuedouble >= 0.0)
		cr_camera_set_num_pref(s, cam, cr_camera_fstops, fstops->valuedouble);

	const cJSON *width = cJSON_GetObjectItem(data, "width");
	if (cJSON_IsNumber(width) && width->valueint > 0)
		cr_camera_set_num_pref(s, cam, cr_camera_res_x, width->valuedouble);

	const cJSON *height = cJSON_GetObjectItem(data, "height");
	if (cJSON_IsNumber(height) && height->valueint > 0)
		cr_camera_set_num_pref(s, cam, cr_camera_res_y, height->valuedouble);

	const cJSON *time = cJSON_GetObjectItem(data, "time");
	if (cJSON_IsNumber(time) && time->valuedouble >= 0.0)
		cr_camera_set_num_pref(s, cam, cr_camera_time, time->valuedouble);

	const cJSON *transforms = cJSON_GetObjectItem(data, "transforms");

	struct vector location = parse_location(transforms);
	cr_camera_set_num_pref(s, cam, cr_camera_pose_x, (double)location.x);
	cr_camera_set_num_pref(s, cam, cr_camera_pose_y, (double)location.y);
	cr_camera_set_num_pref(s, cam, cr_camera_pose_z, (double)location.z);

	struct euler_angles pose = parseRotations(transforms);
	cr_camera_set_num_pref(s, cam, cr_camera_pose_roll,  (double)pose.roll);
	cr_camera_set_num_pref(s, cam, cr_camera_pose_pitch, (double)pose.pitch);
	cr_camera_set_num_pref(s, cam, cr_camera_pose_yaw,   (double)pose.yaw);

	cr_camera_update(s, cam);
}

static void parse_cameras(struct cr_scene *scene, const cJSON *data) {
	if (!data || !scene) return;

	if (cJSON_IsObject(data)) {
		parse_camera(scene, data);
		return;
	}
	ASSERT(cJSON_IsArray(data));
	cJSON *camera = NULL;
	cJSON_ArrayForEach(camera, data) {
		parse_camera(scene, camera);
	}
}

//FIXME: Convert this to use parseBsdfNode
static void parse_ambient_color(struct cr_renderer *r, struct cr_scene *s, const cJSON *data) {
	struct renderer *todo_remove_r = (struct renderer *)r;
	const cJSON *offset = cJSON_GetObjectItem(data, "offset");
	if (cJSON_IsNumber(offset)) {
		todo_remove_r->scene->backgroundOffset = deg_to_rad(offset->valuedouble) / 4.0f;
	}

	const cJSON *down = cJSON_GetObjectItem(data, "down");
	const cJSON *up = cJSON_GetObjectItem(data, "up");
	const cJSON *hdr = cJSON_GetObjectItem(data, "hdr");

	if (cJSON_IsString(hdr)) {
		if (cr_scene_set_background_hdr(r, s, hdr->valuestring)) return;
	}
	
	if (down && up) {
		struct color d = color_parse(down);
		struct color u = color_parse(up);
		cr_scene_set_background(s, (struct cr_color *)&d, (struct cr_color *)&u);
		return;
	}
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

static void parse_mesh(struct cr_renderer *r, const cJSON *data, int idx, int mesh_file_count) {
	const cJSON *file_name = cJSON_GetObjectItem(data, "fileName");
	if (!cJSON_IsString(file_name)) return;

	struct renderer *todo_remove_r = (struct renderer *)r;

	//FIXME: This concat + path fixing should be an utility function
	char *fullPath = stringConcat(todo_remove_r->prefs.assetPath, file_name->valuestring);
	windowsFixPath(fullPath);

	logr(plain, "\r");
	logr(info, "Loading mesh file %i/%i%s", idx + 1, mesh_file_count, (idx + 1) == mesh_file_count ? "\n" : "\r");
	struct timeval timer;
	timer_start(&timer);
	//FIXME: A new asset type that contains meshes, materials, etc separately would make this much more flexible.
	struct mesh_arr meshes = load_meshes_from_file(fullPath, todo_remove_r->state.file_cache);
	long us = timer_get_us(timer);
	free(fullPath);
	long ms = us / 1000;
	logr(debug, "Parsing file %-35s took %zu %s\n", file_name->valuestring, ms > 0 ? ms : us, ms > 0 ? "ms" : "μs");

	if (!meshes.count) return;

	// Per JSON 'meshes' array element, these apply to materials before we assign them to instances
	struct mtl_override_arr global_overrides = parse_override_list(todo_remove_r, cJSON_GetObjectItem(data, "materials"));

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
			bsdf_node_ptr_arr_add(&file_bsdfs->bsdfs, try_to_guess_bsdf(&todo_remove_r->scene->storage, &file_mats.items[i]));
		}
	}

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

	size_t current_mesh_count = todo_remove_r->scene->meshes.count;
	if (!cJSON_IsArray(pick_instances)) {
		// Generate one instance for every mesh, identity transform.
		for (size_t i = 0; i < meshes.count; ++i) {
			struct instance new = new_mesh_instance(&todo_remove_r->scene->meshes, current_mesh_count + i, NULL, NULL);
			new.bbuf = bsdf_buf_ref(file_bsdfs);
			instance_arr_add(&todo_remove_r->scene->instances, new);
		}
		goto done;
	}

	const cJSON *instances = pick_instances ? pick_instances : add_instances;
	if (!cJSON_IsArray(instances)) goto done;

	const cJSON *instance = NULL;
	cJSON_ArrayForEach(instance, instances) {
		const cJSON *mesh_name = cJSON_GetObjectItem(instance, "for");
		int64_t target_idx = -1;
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
			new = new_mesh_instance(&todo_remove_r->scene->meshes, current_mesh_count + target_idx, (float *)&density->valuedouble, &todo_remove_r->scene->storage.node_pool);
		} else {
			//FIXME: Make newMesh*() and newSphere*() const
			new = new_mesh_instance(&todo_remove_r->scene->meshes, current_mesh_count + target_idx, NULL, NULL);
		}
		new.bbuf = bsdf_buf_ref(NULL);

		struct mtl_override_arr instance_overrides = parse_override_list(todo_remove_r, cJSON_GetObjectItem(instance, "materials"));
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
		instance_arr_add(&todo_remove_r->scene->instances, new);
	}
done:

	bsdf_buf_unref(file_bsdfs);
	// Store meshes
	logr(debug, "Adding %zu meshes\n", meshes.count);
	for (size_t i = 0; i < meshes.count; ++i) {
		mesh_arr_add(&todo_remove_r->scene->meshes, meshes.items[i]);
	}

	mesh_arr_free(&meshes);
}

static void parse_meshes(struct cr_renderer *r, const cJSON *data) {
	if (!cJSON_IsArray(data)) return;
	int idx = 0;
	int mesh_file_count = cJSON_GetArraySize(data);
	const cJSON *mesh = NULL;
	cJSON_ArrayForEach(mesh, data) {
		parse_mesh(r, mesh, idx++, mesh_file_count);
	}
}

static void parse_sphere(struct cr_renderer *r, const cJSON *data) {
	struct sphere new = { 0 };

	struct renderer *todo_remove_r = (struct renderer *)r;

	const cJSON *radius = NULL;
	radius = cJSON_GetObjectItem(data, "radius");
	if (radius != NULL && cJSON_IsNumber(radius)) {
		new.radius = radius->valuedouble;
	} else {
		new.radius = 1.0f;
		logr(warning, "No radius specified for sphere, setting to %.0f\n", (double)new.radius);
	}
	const size_t new_idx = sphere_arr_add(&todo_remove_r->scene->spheres, new);
	
	// Apply this to all instances that don't have their own "materials" object
	const cJSON *sphere_global_materials = cJSON_GetObjectItem(data, "material");
	
	const cJSON *instances = cJSON_GetObjectItem(data, "instances");
	const cJSON *instance = NULL;
	if (cJSON_IsArray(instances)) {
		cJSON_ArrayForEach(instance, instances) {
			const cJSON *density = cJSON_GetObjectItem(data, "density");

			struct instance new_instance = { 0 };
			if (cJSON_IsNumber(density)) {
				new_instance = new_sphere_instance(&todo_remove_r->scene->spheres, new_idx, (float *)&density->valuedouble, &todo_remove_r->scene->storage.node_pool);
			} else {
				new_instance = new_sphere_instance(&todo_remove_r->scene->spheres, new_idx, NULL, NULL);
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
				bsdf_node_ptr_arr_add(&new_instance.bbuf->bsdfs, parseBsdfNode(todo_remove_r->prefs.assetPath, todo_remove_r->state.file_cache, &todo_remove_r->scene->storage, material));
				const cJSON *type_string = cJSON_GetObjectItem(material, "type");
				if (type_string && stringEquals(type_string->valuestring, "emissive")) new_instance.emits_light = true;
			}

			if (!new_instance.bbuf->bsdfs.count) bsdf_node_ptr_arr_add(&new_instance.bbuf->bsdfs, warningBsdf(&todo_remove_r->scene->storage));
			new_instance.composite = parse_composite_transform(cJSON_GetObjectItem(instance, "transforms"));
			instance_arr_add(&todo_remove_r->scene->instances, new_instance);
		}
	}
}

static void parse_primitive(struct cr_renderer *r, const cJSON *data, int idx) {
	const cJSON *type = NULL;
	type = cJSON_GetObjectItem(data, "type");
	if (stringEquals(type->valuestring, "sphere")) {
		parse_sphere(r, data);
	} else {
		logr(warning, "Unknown primitive type \"%s\" at index %i\n", type->valuestring, idx);
	}
}

static void parse_primitives(struct cr_renderer *r, const cJSON *data) {
	if (!cJSON_IsArray(data)) return;
	if (data != NULL && cJSON_IsArray(data)) {
		int i = 0;
		const cJSON *primitive = NULL;
		cJSON_ArrayForEach(primitive, data) {
			parse_primitive(r, primitive, i++);
		}
	}
}

static void parseScene(struct cr_renderer *r, const cJSON *data) {
	struct cr_scene *scene = cr_renderer_scene_get(r);
	parse_ambient_color(r, scene, cJSON_GetObjectItem(data, "ambientColor"));
	parse_primitives(r, cJSON_GetObjectItem(data, "primitives"));
	parse_meshes(r, cJSON_GetObjectItem(data, "meshes"));
}

int parse_json(struct cr_renderer *r, cJSON *json) {
	struct cr_scene *scene = cr_renderer_scene_get(r);
	parse_prefs(r, cJSON_GetObjectItem(json, "renderer"));
	parse_cameras(scene, cJSON_GetObjectItem(json, "camera"));
	if (!cr_scene_totals(scene).cameras) {
		logr(warning, "No cameras specified, nothing to render.\n");
		return -1;
	}

	const cJSON *selected_camera = cJSON_GetObjectItem(json, "selected_camera");
	if (cJSON_IsNumber(selected_camera)) {
		cr_renderer_set_num_pref(r, cr_renderer_override_cam, selected_camera->valueint);
	}
	parseScene(r, cJSON_GetObjectItem(json, "scene"));
	
	return 0;
}
