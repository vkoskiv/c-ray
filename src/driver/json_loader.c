//
//  json_loader.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 02/04/2019.
//  Copyright © 2019-2023 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"

#include "json_loader.h"
#include "../vendored/cJSON.h"
#include "node_parse.h"
#include "loaders/meshloader.h"
#include "../utils/loaders/textureloader.h"

#include <c-ray/c-ray.h>

#include "../datatypes/transforms.h"
#include "../datatypes/quaternion.h"
#include "../datatypes/spline.h"
#include "../datatypes/vector.h"
#include "../utils/string.h"
#include "../utils/platform/capabilities.h"
#include "../utils/logging.h"
#include "../utils/fileio.h"
#include "../utils/timer.h"

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

struct cr_shader_node *global_desc(struct material_arr file_mats, const cJSON *global_overrides, size_t idx) {
	if (idx >= file_mats.count) return NULL;
	struct cr_shader_node *match = NULL;
	const cJSON *override = NULL;
	cJSON_ArrayForEach(override, global_overrides) {
		const cJSON *name = cJSON_GetObjectItem(override, "replace");
		if (cJSON_IsString(name) && stringEquals(name->valuestring, file_mats.items[idx].name)) {
			match = cr_shader_node_build(override);
		}
	}
	return match ? match : try_to_guess_bsdf(&file_mats.items[idx]);
}

static void parse_mesh(struct cr_renderer *r, const cJSON *data, int idx, int mesh_file_count) {
	const cJSON *file_name = cJSON_GetObjectItem(data, "fileName");
	if (!cJSON_IsString(file_name)) return;

	struct cr_scene *scene = cr_renderer_scene_get(r);

	//FIXME: This concat + path fixing should be an utility function
	const char *asset_path = cr_renderer_get_str_pref(r, cr_renderer_asset_path);
	char *full_path = stringConcat(asset_path, file_name->valuestring);
	windowsFixPath(full_path);

	logr(plain, "\r");
	logr(info, "Loading mesh file %i/%i%s", idx + 1, mesh_file_count, (idx + 1) == mesh_file_count ? "\n" : "\r");
	struct timeval timer;
	timer_start(&timer);
	struct mesh_parse_result result = load_meshes_from_file(full_path);
	long us = timer_get_us(timer);
	free(full_path);
	long ms = us / 1000;
	logr(debug, "Parsing file %-35s took %zu %s\n", file_name->valuestring, ms > 0 ? ms : us, ms > 0 ? "ms" : "μs");

	if (!result.meshes.count) return;

	cr_vertex_buf vbuf = cr_scene_vertex_buf_new(scene, (struct cr_vertex_buf_param){
		.vertices = (struct cr_vector *)result.geometry.vertices.items,
		.vertex_count = result.geometry.vertices.count,
		.normals = (struct cr_vector *)result.geometry.normals.items,
		.normal_count = result.geometry.normals.count,
		.tex_coords = (struct cr_coord *)result.geometry.texture_coords.items,
		.tex_coord_count = result.geometry.texture_coords.count,
	});

	// Per JSON 'meshes' array element, these apply to materials before we assign them to instances
	const struct cJSON *global_overrides = cJSON_GetObjectItem(data, "materials");

	// FIXME: Textures get loaded multiple times, put them in a hash table
	// Precompute guessed bsdfs for these instances
	cr_material_set file_set = cr_scene_new_material_set(scene);
	logr(debug, "Figuring out bsdfs for mtllib materials\n");
	for (size_t i = 0; i < result.materials.count; ++i) {
		struct cr_shader_node *desc = global_desc(result.materials, global_overrides, i);
		cr_material_set_add(r, scene, file_set, desc);
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
		logr(warning, "Can't combine pick_instances and add_instances (%s)\n", file_name->valuestring);
		goto done;
	}

	const cJSON *instances = pick_instances ? pick_instances : add_instances;
	if (!cJSON_IsArray(pick_instances)) {
		// Generate one instance for every mesh, identity transform.
		for (size_t i = 0; i < result.meshes.count; ++i) {
			cr_mesh mesh = cr_scene_mesh_new(scene, result.meshes.items[i].name);
			cr_mesh_bind_vertex_buf(scene, mesh, vbuf);
			cr_mesh_bind_faces(scene, mesh, (struct cr_face *)result.meshes.items[i].polygons.items, result.meshes.items[i].polygons.count);
			cr_instance m_instance = cr_instance_new(scene, mesh, cr_object_mesh);
			cr_instance_bind_material_set(r, m_instance, file_set);
		}
		goto done;
	}

	for (size_t m_idx = 0; m_idx < result.meshes.count; ++m_idx) {
		const cJSON *instance = NULL;
		cJSON_ArrayForEach(instance, instances) {
			const cJSON *mesh_name = cJSON_GetObjectItem(instance, "for");
			cr_mesh mesh = -1;
			if (stringEquals(mesh_name->valuestring, result.meshes.items[m_idx].name)) {
				mesh = cr_scene_mesh_new(scene, result.meshes.items[m_idx].name);
				cr_mesh_bind_vertex_buf(scene, mesh, vbuf);
				cr_mesh_bind_faces(scene, mesh, (struct cr_face *)result.meshes.items[m_idx].polygons.items, result.meshes.items[m_idx].polygons.count);
			}
			if (mesh < 0) continue;
			cr_instance new = cr_instance_new(scene, mesh, cr_object_mesh);
			cr_material_set instance_set = cr_scene_new_material_set(scene);
			const cJSON *instance_overrides = cJSON_GetObjectItem(instance, "materials");
			for (size_t i = 0; i < result.materials.count; ++i) {
				struct cr_shader_node *override_match = NULL;
				const cJSON *override = NULL;
				cJSON_ArrayForEach(override, instance_overrides) {
					const cJSON *name = cJSON_GetObjectItem(override, "replace");
					if (cJSON_IsString(name) && stringEquals(name->valuestring, result.materials.items[i].name)) {
						override_match = cr_shader_node_build(override);
						break;
					}
				}
				if (override_match) {
					cr_material_set_add(r, scene, instance_set, override_match);
				} else {
					struct cr_shader_node *global = global_desc(result.materials, global_overrides, i);
					cr_material_set_add(r, scene, instance_set, global);
				}
			}

			cr_instance_set_transform(scene, new, parse_composite_transform(cJSON_GetObjectItem(instance, "transforms")).A.mtx);
			cr_instance_bind_material_set(r, new, instance_set);
		}
	}
done:

	for (size_t i = 0; i < result.meshes.count; ++i) {
		poly_arr_free(&result.meshes.items[i].polygons);
		destroyMesh(&result.meshes.items[i]);
	}
	for (size_t i = 0; i < result.materials.count; ++i) {
		destroyMaterial(&result.materials.items[i]);
	}
	mesh_arr_free(&result.meshes);
	material_arr_free(&result.materials);
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
	const cJSON *instance = NULL;
	if (cJSON_IsArray(instances)) {
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
				cr_material_set_add(r, scene, instance_set, desc);

				// FIXME
				// const cJSON *type_string = cJSON_GetObjectItem(material, "type");
				// if (type_string && stringEquals(type_string->valuestring, "emissive")) new_instance.emits_light = true;
			} else {
				// TODO
				// cr_material_set_add(r, instance_set, warning_material_desc);
			}

			cr_instance_set_transform(scene, new_instance, parse_composite_transform(cJSON_GetObjectItem(instance, "transforms")).A.mtx);
			cr_instance_bind_material_set(r, new_instance, instance_set);
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

	struct cr_shader_node *background = cr_shader_node_build(cJSON_GetObjectItem(data, "ambientColor"));
	cr_scene_set_background(r, scene, background);

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
