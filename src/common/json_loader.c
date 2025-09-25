//
//  json_loader.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2025 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"

#include <v.h>
#include <c-ray/c-ray.h>

#include "json_loader.h"
#include "vendored/cJSON.h"
#include "loaders/meshloader.h"
#include "node_parse.h"
#include "loaders/textureloader.h"
#include "quaternion.h"
#include "transforms.h"
#include "vector.h"
#include "cr_string.h"
#include "logging.h"
#include "fileio.h"

static struct transform parse_tform(const cJSON *data) {
	const cJSON *type = cJSON_GetObjectItem(data, "type");
	if (!cJSON_IsString(type)) {
		logr(warning, "No type found for transform: %s\n", cJSON_Print(data));
		return tform_new();
	}

	//For translate, we want the default to be 0. For scaling, def should be 1
	const float fallback = stringEquals(type->valuestring, "scale") ? 1.0f : 0.0f;
	
	const cJSON *x_in = cJSON_GetObjectItem(data, "x");
	const float x = cJSON_IsNumber(x_in) ? x_in->valuedouble : fallback;
	
	const cJSON *y_in = cJSON_GetObjectItem(data, "y");
	const float y = cJSON_IsNumber(y_in) ? y_in->valuedouble : fallback;

	const cJSON *z_in = cJSON_GetObjectItem(data, "z");
	const float z = cJSON_IsNumber(z_in) ? z_in->valuedouble : fallback;

	const cJSON *scale_in = cJSON_GetObjectItem(data, "scale");
	const float scale = cJSON_IsNumber(scale_in) ? scale_in->valuedouble : fallback;

	const cJSON *degs_in = cJSON_GetObjectItem(data, "degrees");
	const cJSON *rads_in = cJSON_GetObjectItem(data, "radians");
	const float rads = cJSON_IsNumber(degs_in) ?
						deg_to_rad(degs_in->valuedouble) :
						cJSON_IsNumber(rads_in) ? rads_in->valuedouble : 0.0f;
	if (stringEquals(type->valuestring, "rotateX")) {
		return tform_new_rot_x(rads);
	} else if (stringEquals(type->valuestring, "rotateY")) {
		return tform_new_rot_y(rads);
	} else if (stringEquals(type->valuestring, "rotateZ")) {
		return tform_new_rot_z(rads);
	} else if (stringEquals(type->valuestring, "translate")) {
		return tform_new_translate(x, y, z);
	} else if (stringEquals(type->valuestring, "scale")) {
		return tform_new_scale3(x, y, z);
	} else if (stringEquals(type->valuestring, "scaleUniform")) {
		return tform_new_scale(scale);
	} else {
		logr(warning, "Found an invalid transform \"%s\"\n", type->valuestring);
	}

	return tform_new();
}

void parse_prefs(struct cr_renderer *ext, const cJSON *data) {
	if (!data || !ext) return;

	const cJSON *threads = cJSON_GetObjectItem(data, "threads");
	if (cJSON_IsNumber(threads) && threads->valueint > 0) {
		if (threads->valueint > 0) {
			cr_renderer_set_num_pref(ext, cr_renderer_threads, threads->valueint);
			// prefs->fromSystem = false;
		} else {
			cr_renderer_set_num_pref(ext, cr_renderer_threads, v_sys_get_cores() + 2);
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

	const cJSON *width = cJSON_GetObjectItem(data, "width");
	if (cJSON_IsNumber(width)) {
		cr_renderer_set_num_pref(ext, cr_renderer_override_width, width->valueint);
	}

	const cJSON *height = cJSON_GetObjectItem(data, "height");
	if (cJSON_IsNumber(height)) {
		cr_renderer_set_num_pref(ext, cr_renderer_override_height, height->valueint);
	}
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
		const char *type = cJSON_GetStringValue(cJSON_GetObjectItem(transform, "type"));
		if (!type) return angles;
		if (stringEquals(type, "rotateX")) {
			angles.roll = getRadians(transform);
		}
		if (stringEquals(type, "rotateY")) {
			angles.pitch = getRadians(transform);
		}
		if (stringEquals(type, "rotateZ")) {
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
		if (type && stringEquals(type->valuestring, "translate")) {
			const cJSON *x = cJSON_GetObjectItem(transform, "x");
			const cJSON *y = cJSON_GetObjectItem(transform, "y");
			const cJSON *z = cJSON_GetObjectItem(transform, "z");
			
			return (struct vector){
				x ? x->valuedouble : 0.0,
				y ? y->valuedouble : 0.0,
				z ? z->valuedouble : 0.0,
			};
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

struct transform parse_composite_transform(const cJSON *transforms) {
	if (!transforms) return tform_new();
	//TODO: Pass mesh/instance name as targetName for logging
	if (!cJSON_IsArray(transforms)) return parse_tform(transforms);
	
	struct transform composite = tform_new();

	const cJSON *transform = NULL;
	cJSON_ArrayForEach(transform, transforms) {
		const cJSON *type = cJSON_GetObjectItem(transform, "type");
		if (!cJSON_IsString(type)) break;
		if (stringStartsWith("translate", type->valuestring)) {
			composite.A = mat_mul(composite.A, parse_tform(transform).A);
		}
	}
	cJSON_ArrayForEach(transform, transforms) {
		const cJSON *type = cJSON_GetObjectItem(transform, "type");
		if (!cJSON_IsString(type)) break;
		if (stringStartsWith("rotate", type->valuestring)) {
			composite.A = mat_mul(composite.A, parse_tform(transform).A);
		}
	}
	cJSON_ArrayForEach(transform, transforms) {
		const cJSON *type = cJSON_GetObjectItem(transform, "type");
		if (!cJSON_IsString(type)) break;
		if (stringStartsWith("scale", type->valuestring)) {
			composite.A = mat_mul(composite.A, parse_tform(transform).A);
		}
	}
	return composite;
}

struct cr_shader_node *check_overrides(struct mesh_material_arr file_mats, size_t mat_idx, const cJSON *overrides) {
	if (mat_idx >= file_mats.count) return NULL;
	const cJSON *override = NULL;
	if (!cJSON_IsArray(overrides)) return NULL;
	cJSON_ArrayForEach(override, overrides) {
		const cJSON *name = cJSON_GetObjectItem(override, "replace");
		if (cJSON_IsString(name) && stringEquals(name->valuestring, file_mats.items[mat_idx].name)) {
			return cr_shader_node_build(override);
		}
	}
	return NULL;
}

void mesh_material_free(struct mesh_material *m) {
	if (m->name) free(m->name);
	if (m->mat) cr_shader_node_free(m->mat);
}

static void parse_mesh(struct cr_renderer *r, const cJSON *data, int idx, int mesh_file_count) {
	const char *file_name = cJSON_GetStringValue(cJSON_GetObjectItem(data, "fileName"));
	if (!file_name) return;

	struct cr_scene *scene = cr_renderer_scene_get(r);

	//FIXME: This concat + path fixing should be an utility function
	const char *asset_path = cr_renderer_get_str_pref(r, cr_renderer_asset_path);
	char *full_path = stringConcat(asset_path, file_name);
	windowsFixPath(full_path);

	logr(plain, "\r");
	logr(info, "Loading mesh file %i/%i%s", idx + 1, mesh_file_count, (idx + 1) == mesh_file_count ? "\n" : "\r");
	v_timer timer = { 0 };
	v_timer_start(&timer);
	struct mesh_parse_result result = load_meshes_from_file(full_path);
	long us = v_timer_get_us(timer);
	free(full_path);
	long ms = us / 1000;
	logr(debug, "Parsing file %-35s took %li %s\n", file_name, ms > 0 ? ms : us, ms > 0 ? "ms" : "μs");

	if (!result.meshes.count) return;

	struct cr_vertex_buf_param vbuf = {
		.vertices = (struct cr_vector *)result.geometry.vertices.items,
		.vertex_count = result.geometry.vertices.count,
		.normals = (struct cr_vector *)result.geometry.normals.items,
		.normal_count = result.geometry.normals.count,
		.tex_coords = (struct cr_coord *)result.geometry.texture_coords.items,
		.tex_coord_count = result.geometry.texture_coords.count,
	};
	// Per JSON 'meshes' array element, these apply to materials before we assign them to instances
	const struct cJSON *global_overrides = cJSON_GetObjectItem(data, "materials");

	// Copy mesh materials to set
	cr_material_set file_set = cr_scene_new_material_set(scene);
	for (size_t i = 0; i < result.materials.count; ++i) {
		struct cr_shader_node *maybe_override = check_overrides(result.materials, i, global_overrides);
		cr_material_set_add(scene, file_set, maybe_override ? maybe_override : result.materials.items[i].mat);
		if (maybe_override) cr_shader_node_free(maybe_override);
	}

	// Now apply some slightly overcomplicated logic to choose instances to add to the scene.
	// It boils down to:
	// - If a 'pick_instances' array is found, only add those instances that were specified.
	// - If a 'add_instances' array is found, add one instance for each mesh + additional ones in array
	// - If neither are found, just add one instance for every mesh.
	// - If both are found, emit warning and bail out.

	const cJSON *pick_instances = cJSON_GetObjectItem(data, "pick_instances");
	const cJSON *add_instances = cJSON_GetObjectItem(data, "add_instances");

	if (pick_instances && add_instances) {
		logr(warning, "Can't combine pick_instances and add_instances (%s)\n", file_name);
		goto done;
	}

	const cJSON *instances = pick_instances ? pick_instances : add_instances;
	if (!cJSON_IsArray(pick_instances)) {
		// Generate one instance for every mesh, identity transform.
		for (size_t i = 0; i < result.meshes.count; ++i) {
			cr_mesh mesh = cr_scene_mesh_new(scene, result.meshes.items[i].name);
			cr_mesh_bind_vertex_buf(scene, mesh, vbuf);

			cr_mesh_bind_faces(scene, mesh, result.meshes.items[i].faces.items, result.meshes.items[i].faces.count);
			cr_instance m_instance = cr_instance_new(scene, mesh, cr_object_mesh);
			cr_instance_bind_material_set(scene, m_instance, file_set);
			cr_instance_set_transform(scene, m_instance, parse_composite_transform(cJSON_GetObjectItem(data, "transforms")).A.mtx);
			cr_mesh_finalize(scene, mesh);
		}
		goto done;
	}

	const cJSON *instance = NULL;
	cJSON_ArrayForEach(instance, instances) {
		char *mesh_name = cJSON_GetStringValue(cJSON_GetObjectItem(instance, "for"));
		if (!mesh_name) continue;
		// Find this mesh in parse result, and add it to the scene if it isn't there yet.
		cr_mesh mesh = -1;
		for (size_t i = 0; i < result.meshes.count; ++i) {
			if (stringEquals(result.meshes.items[i].name, mesh_name)) {
				mesh = cr_scene_get_mesh(scene, result.meshes.items[i].name);
				if (mesh < 0) {
					mesh = cr_scene_mesh_new(scene, result.meshes.items[i].name);
					cr_mesh_bind_vertex_buf(scene, mesh, vbuf);
					cr_mesh_bind_faces(scene, mesh, result.meshes.items[i].faces.items, result.meshes.items[i].faces.count);
					cr_mesh_finalize(scene, mesh);
				}
			}
		}
		if (mesh < 0) continue;
		// And now create the instance
		cr_instance new = cr_instance_new(scene, mesh, cr_object_mesh);
		cr_material_set instance_set = cr_scene_new_material_set(scene);
		// For the instance materials, we iterate the mesh materials, check if a "replace" exists with that name,
		// if one does, use that, otherwise grab the mesh material.
		const cJSON *instance_overrides = cJSON_GetObjectItem(instance, "materials");
		for (size_t i = 0; i < result.materials.count; ++i) {
			struct cr_shader_node *material = NULL;
			// Find the material we want to use. Check if instance overrides it, otherwise use mesh global one
			material = check_overrides(result.materials, i, instance_overrides);
			// If material is NULL here, it gets set to an obnoxious material internally.
			cr_material_set_add(scene, instance_set, material ? material : result.materials.items[i].mat);
			cr_shader_node_free(material);
		}
		cr_instance_set_transform(scene, new, parse_composite_transform(cJSON_GetObjectItem(instance, "transforms")).A.mtx);
		cr_instance_bind_material_set(scene, new, instance_set);
	}
	
done:

	result.meshes.elem_free = ext_mesh_free;
	ext_mesh_arr_free(&result.meshes);
	result.materials.elem_free = mesh_material_free;
	mesh_material_arr_free(&result.materials);
	vector_arr_free(&result.geometry.vertices);
	vector_arr_free(&result.geometry.normals);
	coord_arr_free(&result.geometry.texture_coords);
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
	struct cr_scene *scene = cr_renderer_scene_get(r);

	const cJSON *radius = NULL;
	radius = cJSON_GetObjectItem(data, "radius");
	float rad = 0.0f;
	if (radius != NULL && cJSON_IsNumber(radius)) {
		rad = radius->valuedouble;
	} else {
		rad = 1.0f;
		logr(warning, "No radius specified for sphere, setting to %.0f\n", (double)rad);
	}

	cr_sphere new_sphere = cr_scene_add_sphere(scene, rad);
	
	// Apply this to all instances that don't have their own "materials" object
	const cJSON *sphere_global_materials = cJSON_GetObjectItem(data, "material");
	
	const cJSON *instances = cJSON_GetObjectItem(data, "instances");
	if (!cJSON_IsArray(instances)) return;
	const cJSON *instance = NULL;
	cJSON_ArrayForEach(instance, instances) {

		cr_instance new_instance = cr_instance_new(scene, new_sphere, cr_object_sphere);

		cr_material_set instance_set = cr_scene_new_material_set(scene);

		const cJSON *instance_materials = cJSON_GetObjectItem(instance, "materials");
		const cJSON *materials = instance_materials ? instance_materials : sphere_global_materials;

		if (materials) {
			const cJSON *material = NULL;
			if (cJSON_IsArray(materials)) {
				material = cJSON_GetArrayItem(materials, 0);
			} else {
				material = materials;
			}
			struct cr_shader_node *desc = cr_shader_node_build(material);
			cr_material_set_add(scene, instance_set, desc);
			cr_shader_node_free(desc);

			// FIXME
			// const cJSON *type_string = cJSON_GetObjectItem(material, "type");
			// if (type_string && stringEquals(type_string->valuestring, "emissive")) new_instance.emits_light = true;
		} else {
			cr_material_set_add(scene, instance_set, NULL);
		}

		cr_instance_set_transform(scene, new_instance, parse_composite_transform(cJSON_GetObjectItem(instance, "transforms")).A.mtx);
		cr_instance_bind_material_set(scene, new_instance, instance_set);
	}
}

static void parse_primitive(struct cr_renderer *r, const cJSON *data, int idx) {
	const char *type = cJSON_GetStringValue(cJSON_GetObjectItem(data, "type"));
	if (type && stringEquals(type, "sphere")) {
		parse_sphere(r, data);
	} else if (type) {
		logr(warning, "Unknown primitive type \"%s\" at index %i\n", type, idx);
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

	struct cr_shader_node *background = cr_shader_node_build(cJSON_GetObjectItem(data, "ambientColor"));
	cr_scene_set_background(scene, background);
	cr_shader_node_free(background);

	parse_primitives(r, cJSON_GetObjectItem(data, "primitives"));
	parse_meshes(r, cJSON_GetObjectItem(data, "meshes"));
}

int parse_json(struct cr_renderer *r, struct cJSON *json) {
	struct cr_scene *scene = cr_renderer_scene_get(r);
	parse_prefs(r, cJSON_GetObjectItem(json, "renderer"));
	parse_cameras(scene, cJSON_GetObjectItem(json, "camera"));
	if (!cr_scene_totals(scene).cameras) {
		logr(warning, "No cameras specified, nothing to render.\n");
		return -1;
	}

	const cJSON *renderer = cJSON_GetObjectItem(json, "renderer");
	const cJSON *selected_camera = cJSON_GetObjectItem(renderer, "selected_camera");
	if (cJSON_IsNumber(selected_camera)) {
		cr_renderer_set_num_pref(r, cr_renderer_override_cam, selected_camera->valueint);
	}
	parseScene(r, cJSON_GetObjectItem(json, "scene"));

	return 0;
}
