//
//  c-ray.c
//  c-ray
//
//  Created by Valtteri on 5.1.2020.
//  Copyright © 2020-2023 Valtteri Koskivuori. All rights reserved.
//

#include "../../includes.h"
#include <c-ray/c-ray.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

#include "../renderer/renderer.h"
#include "../datatypes/scene.h"
#include "../../common/gitsha1.h"
#include "../../common/fileio.h"
#include "../../common/platform/terminal.h"
#include "../../common/assert.h"
#include "../../common/texture.h"
#include "../../common/string.h"
#include "../protocol/server.h"
#include "../protocol/worker.h"
#include "../../common/hashtable.h"
#include "../datatypes/camera.h"
#include "../../common/loaders/textureloader.h"
#include "../protocol/protocol.h"

#ifdef CRAY_DEBUG_ENABLED
#define DEBUG "D"
#else
#define DEBUG ""
#endif

#define VERSION "0.6.3"DEBUG

char *cr_get_version() {
	return VERSION;
}

char *cr_get_git_hash() {
	return gitHash();
}

// -- Renderer --

struct cr_renderer;

struct cr_renderer *cr_new_renderer() {
	return (struct cr_renderer *)renderer_new();
}

bool cr_renderer_set_num_pref(struct cr_renderer *ext, enum cr_renderer_param p, uint64_t num) {
	if (!ext) return false;
	struct renderer *r = (struct renderer *)ext;
	switch (p) {
		case cr_renderer_threads: {
			r->prefs.threads = num;
			return true;
		}
		case cr_renderer_samples: {
			r->prefs.sampleCount = num;
			return true;
		}
		case cr_renderer_bounces: {
			if (num > 512) return false;
			r->prefs.bounces = num;
			return true;
		}
		case cr_renderer_tile_width: {
			r->prefs.tileWidth = num;
			return true;
		}
		case cr_renderer_tile_height: {
			r->prefs.tileHeight = num;
			return true;
		}
		case cr_renderer_output_num: {
			r->prefs.imgCount = num;
			return true;
		}
		case cr_renderer_override_width: {
			r->prefs.override_width = num;
			return true;
		}
		case cr_renderer_override_height: {
			r->prefs.override_height = num;
			return true;
		}
		case cr_renderer_override_cam: {
			if (!r->scene->cameras.count) return false;
			if (num >= r->scene->cameras.count) return false;
			r->prefs.selected_camera = num;
			return true;
		}
		case cr_renderer_is_iterative: {
			r->prefs.iterative = true;
			return true;
		}
		default: return false;
	}
	return false;
}

bool cr_renderer_set_str_pref(struct cr_renderer *ext, enum cr_renderer_param p, const char *str) {
	if (!ext) return false;
	struct renderer *r = (struct renderer *)ext;
	switch (p) {
		case cr_renderer_tile_order: {
			if (stringEquals(str, "random")) {
				r->prefs.tileOrder = ro_random;
			} else if (stringEquals(str, "topToBottom")) {
				r->prefs.tileOrder = ro_top_to_bottom;
			} else if (stringEquals(str, "fromMiddle")) {
				r->prefs.tileOrder = ro_from_middle;
			} else if (stringEquals(str, "toMiddle")) {
				r->prefs.tileOrder = ro_to_middle;
			} else {
				r->prefs.tileOrder = ro_normal;
			}
			return true;
		}
		case cr_renderer_output_path: {
			if (r->prefs.imgFilePath) free(r->prefs.imgFilePath);
			r->prefs.imgFilePath = stringCopy(str);
			return true;
		}
		case cr_renderer_asset_path: {
			// TODO: we shouldn't really be touching anything but prefs in here.
			if (r->scene->asset_path) free(r->scene->asset_path);
			r->scene->asset_path = stringCopy(str);
			return true;
		}
		case cr_renderer_output_name: {
			if (r->prefs.imgFileName) free(r->prefs.imgFileName);
			r->prefs.imgFileName = stringCopy(str);
			return true;
		}
		case cr_renderer_node_list: {
			if (r->prefs.node_list) free(r->prefs.node_list);
			r->prefs.node_list = stringCopy(str);
			return true;
		}
		default: return false;
	}
	return false;
}

bool cr_renderer_set_callbacks(struct cr_renderer *ext, struct cr_renderer_callbacks cb) {
	if (!ext) return false;
	struct renderer *r = (struct renderer *)ext;
	r->state.cb = cb;
	return true;
}

void cr_renderer_stop(struct cr_renderer *ext, bool should_save) {
	if (!ext) return;
	struct renderer *r = (struct renderer *)ext;
	r->state.saveImage = should_save;
	r->state.render_aborted = true;
}

void cr_renderer_toggle_pause(struct cr_renderer *ext) {
	if (!ext) return;
	struct renderer *r = (struct renderer *)ext;
	for (size_t i = 0; i < r->prefs.threads; ++i) {
		// FIXME: Use array for workers
		// FIXME: What about network renderers?
		r->state.workers.items[i].paused = !r->state.workers.items[i].paused;
	}
}

const char *cr_renderer_get_str_pref(struct cr_renderer *ext, enum cr_renderer_param p) {
	if (!ext) return NULL;
	struct renderer *r = (struct renderer *)ext;
	switch (p) {
		case cr_renderer_output_path: return r->prefs.imgFilePath;
		case cr_renderer_output_name: return r->prefs.imgFileName;
		case cr_renderer_asset_path: return r->scene->asset_path;
		default: return NULL;
	}
	return NULL;
}

uint64_t cr_renderer_get_num_pref(struct cr_renderer *ext, enum cr_renderer_param p) {
	if (!ext) return 0;
	struct renderer *r = (struct renderer *)ext;
	switch (p) {
		case cr_renderer_threads: return r->prefs.threads;
		case cr_renderer_samples: return r->prefs.sampleCount;
		case cr_renderer_bounces: return r->prefs.bounces;
		case cr_renderer_tile_width: return r->prefs.tileWidth;
		case cr_renderer_tile_height: return r->prefs.tileHeight;
		case cr_renderer_output_num: return r->prefs.imgCount;
		case cr_renderer_override_width: return r->prefs.override_width;
		case cr_renderer_override_height: return r->prefs.override_height;
		case cr_renderer_should_save: return r->state.saveImage ? 1 : 0;
		default: return 0; // TODO
	}
	return 0;
}

bool cr_scene_set_background(struct cr_scene *s_ext, struct cr_shader_node *desc) {
	if (!s_ext) return false;
	struct world *s = (struct world *)s_ext;
	s->background = desc ? build_bsdf_node(s_ext, desc) : newBackground(&s->storage, NULL, NULL, NULL);
	s->bg_desc = desc ? desc : NULL;
	return true;
}

// -- Scene --

struct cr_scene;

struct cr_scene *cr_renderer_scene_get(struct cr_renderer *ext) {
	if (!ext) return NULL;
	return (struct cr_scene *)((struct renderer *)ext)->scene;
}

struct cr_scene_totals cr_scene_totals(struct cr_scene *s_ext) {
	if (!s_ext) return (struct cr_scene_totals){ 0 };
	struct world *s = (struct world *)s_ext;
	return (struct cr_scene_totals){
		.meshes = s->meshes.count,
		.spheres = s->spheres.count,
		.instances = s->instances.count,
		.cameras = s->cameras.count
	};
}

cr_sphere cr_scene_add_sphere(struct cr_scene *s_ext, float radius) {
	if (!s_ext) return -1;
	struct world *scene = (struct world *)s_ext;
	return sphere_arr_add(&scene->spheres, (struct sphere){ .radius = radius });
}

cr_vertex_buf cr_scene_vertex_buf_new(struct cr_scene *s_ext, struct cr_vertex_buf_param in) {
	if (!s_ext) return -1;
	struct world *scene = (struct world *)s_ext;
	struct vertex_buffer new = { 0 };
	// TODO: T_arr_add_n()
	if (in.vertices && in.vertex_count) {
		for (size_t i = 0; i < in.vertex_count; ++i) {
			vector_arr_add(&new.vertices, *(struct vector *)&in.vertices[i]);
		}
	}
	if (in.normals && in.normal_count) {
		for (size_t i = 0; i < in.normal_count; ++i) {
			vector_arr_add(&new.normals, *(struct vector *)&in.normals[i]);
		}
	}
	if (in.tex_coords && in.tex_coord_count) {
		for (size_t i = 0; i < in.tex_coord_count; ++i) {
			coord_arr_add(&new.texture_coords, *(struct coord *)&in.tex_coords[i]);
		}
	}
	return vertex_buffer_arr_add(&scene->v_buffers, new);
}

void cr_mesh_bind_vertex_buf(struct cr_scene *s_ext, cr_mesh mesh, cr_vertex_buf buf) {
	if (!s_ext) return;
	struct world *scene = (struct world *)s_ext;
	if ((size_t)mesh > scene->meshes.count - 1) return;
	struct mesh *m = &scene->meshes.items[mesh];
	if ((size_t)buf > scene->v_buffers.count - 1) return;
	m->vbuf = &scene->v_buffers.items[buf];
	m->vbuf_idx = buf;
}

void cr_mesh_bind_faces(struct cr_scene *s_ext, cr_mesh mesh, struct cr_face *faces, size_t face_count) {
	if (!s_ext || !faces) return;
	struct world *scene = (struct world *)s_ext;
	if ((size_t)mesh > scene->meshes.count - 1) return;
	struct mesh *m = &scene->meshes.items[mesh];
	// FIXME: memcpy
	for (size_t i = 0; i < face_count; ++i) {
		poly_arr_add(&m->polygons, *(struct poly *)&faces[i]);
	}
}

cr_mesh cr_scene_mesh_new(struct cr_scene *s_ext, const char *name) {
	if (!s_ext) return -1;
	struct world *scene = (struct world *)s_ext;
	struct mesh new = { 0 };
	if (name) new.name = stringCopy(name);
	return mesh_arr_add(&scene->meshes, new);
}

cr_mesh cr_scene_get_mesh(struct cr_scene *s_ext, const char *name) {
	if (!s_ext || !name) return -1;
	struct world *scene = (struct world *)s_ext;
	for (size_t i = 0; i < scene->meshes.count; ++i) {
		if (stringEquals(scene->meshes.items[i].name, name)) {
			return i;
		}
	}
	return -1;
}

cr_instance cr_instance_new(struct cr_scene *s_ext, cr_object object, enum cr_object_type type) {
	if (!s_ext) return -1;
	struct world *scene = (struct world *)s_ext;
	struct instance new;
	switch (type) {
		case cr_object_mesh:
			new = new_mesh_instance(&scene->meshes, object, NULL, NULL);
			break;
		case cr_object_sphere:
			new = new_sphere_instance(&scene->spheres, object, NULL, NULL);
			break;
		default:
			return -1;
	}

	return instance_arr_add(&scene->instances, new);
}

void cr_instance_set_transform(struct cr_scene *s_ext, cr_instance instance, float row_major[4][4]) {
	if (!s_ext) return;
	struct world *scene = (struct world *)s_ext;
	if ((size_t)instance > scene->instances.count - 1) return;
	struct instance *i = &scene->instances.items[instance];
	struct matrix4x4 mtx = {
		.mtx = {
			{ row_major[0][0], row_major[0][1], row_major[0][2], row_major[0][3] },
			{ row_major[1][0], row_major[1][1], row_major[1][2], row_major[1][3] },
			{ row_major[2][0], row_major[2][1], row_major[2][2], row_major[2][3] },
			{ row_major[3][0], row_major[3][1], row_major[3][2], row_major[3][3] },
		}
	};
	i->composite = (struct transform){
		.A = mtx,
		.Ainv = mat_invert(mtx)
	};
}

bool cr_instance_bind_material_set(struct cr_scene *s_ext, cr_instance instance, cr_material_set set) {
	if (!s_ext) return false;
	struct world *scene = (struct world *)s_ext;
	if ((size_t)instance > scene->instances.count - 1) return false;
	if ((size_t)set > scene->shader_buffers.count - 1) return false;
	struct instance *i = &scene->instances.items[instance];
	i->bbuf_idx = set;
	return true;
}

void cr_destroy_renderer(struct cr_renderer *ext) {
	struct renderer *r = (struct renderer *)ext;
	ASSERT(r);
	renderer_destroy(r);
}

// -- Camera --

struct camera default_camera = {
	.FOV = 80.0f,
	.focus_distance = 0.0f,
	.fstops = 0.0f,
	.width = 800,
	.height = 600,
};

cr_camera cr_camera_new(struct cr_scene *ext) {
	if (!ext) return -1;
	struct world *scene = (struct world *)ext;
	return camera_arr_add(&scene->cameras, default_camera);
}

bool cr_camera_set_num_pref(struct cr_scene *ext, cr_camera c, enum cr_camera_param p, double num) {
	if (c < 0 || !ext) return false;
	struct world *scene = (struct world *)ext;
	if ((size_t)c > scene->cameras.count - 1) return false;
	struct camera *cam = &scene->cameras.items[c];
	switch (p) {
		case cr_camera_fov: {
			cam->FOV = num;
			return true;
		}
		case cr_camera_focus_distance: {
			cam->focus_distance = num;
			return true;
		}
		case cr_camera_fstops: {
			cam->fstops = num;
			return true;
		}
		case cr_camera_pose_x: {
			cam->position.x = num;
			return true;
		}
		case cr_camera_pose_y: {
			cam->position.y = num;
			return true;
		}
		case cr_camera_pose_z: {
			cam->position.z = num;
			return true;
		}
		case cr_camera_pose_roll: {
			cam->orientation.roll = num;
			return true;
		}
		case cr_camera_pose_pitch: {
			cam->orientation.pitch = num;
			return true;
		}
		case cr_camera_pose_yaw: {
			cam->orientation.yaw = num;
			return true;
		}
		case cr_camera_time: {
			cam->time = num;
			return true;
		}
		case cr_camera_res_x: {
			cam->width = num;
			return true;
		}
		case cr_camera_res_y: {
			cam->height = num;
			return true;
		}
	}

	cam_update_pose(cam, &cam->orientation, &cam->position);
	return false;
}

bool cr_camera_update(struct cr_scene *ext, cr_camera c) {
	if (c < 0 || !ext) return false;
	struct world *scene = (struct world *)ext;
	if ((size_t)c > scene->cameras.count - 1) return false;
	struct camera *cam = &scene->cameras.items[c];
	cam_update_pose(cam, &cam->orientation, &cam->position);
	cam_recompute_optics(cam);
	return true;
}

bool cr_camera_remove(struct cr_scene *s, cr_camera c) {
	//TODO
	(void)s;
	(void)c;
	return false;
}

// --

cr_material_set cr_scene_new_material_set(struct cr_scene *s_ext) {
	if (!s_ext) return -1;
	struct world *scene = (struct world *)s_ext;
	return bsdf_buffer_arr_add(&scene->shader_buffers, (struct bsdf_buffer){ 0 });
}

struct cr_vector_node *vector_deepcopy(const struct cr_vector_node *in);
struct cr_color_node *color_deepcopy(const struct cr_color_node *in);

struct cr_value_node *value_deepcopy(const struct cr_value_node *in) {
	if (!in) return NULL;
	struct cr_value_node *out = calloc(1, sizeof(*out));
	out->type = in->type;
	switch (in->type) {
		case cr_vn_constant:
			out->arg.constant = in->arg.constant;
			break;
		case cr_vn_fresnel:
			out->arg.fresnel.IOR = value_deepcopy(in->arg.fresnel.IOR);
			out->arg.fresnel.normal = vector_deepcopy(in->arg.fresnel.normal);
			break;
		case cr_vn_map_range:
			out->arg.map_range.from_max = value_deepcopy(in->arg.map_range.from_max);
			out->arg.map_range.from_min = value_deepcopy(in->arg.map_range.from_min);
			out->arg.map_range.to_max = value_deepcopy(in->arg.map_range.to_max);
			out->arg.map_range.to_min = value_deepcopy(in->arg.map_range.to_min);
			break;
		case cr_vn_raylength:
			break;
		case cr_vn_alpha:
			out->arg.alpha.color = color_deepcopy(in->arg.alpha.color);
			break;
		case cr_vn_vec_to_value:
			out->arg.vec_to_value.comp = in->arg.vec_to_value.comp;
			out->arg.vec_to_value.vec = vector_deepcopy(in->arg.vec_to_value.vec);
			break;
		case cr_vn_math:
			out->arg.math.A = value_deepcopy(in->arg.math.A);
			out->arg.math.B = value_deepcopy(in->arg.math.B);
			out->arg.math.op = in->arg.math.op;
			break;
		case cr_vn_grayscale:
			out->arg.grayscale.color = color_deepcopy(in->arg.grayscale.color);
			break;
		default:
			break;
		
	}
	return out;
}

struct cr_color_node *color_deepcopy(const struct cr_color_node *in) {
	if (!in) return NULL;
	struct cr_color_node *out = calloc(1, sizeof(*out));
	out->type = in->type;
	switch (in->type) {
		case cr_cn_constant:
			out->arg.constant = in->arg.constant;
			break;
		case cr_cn_image:
			out->arg.image.full_path = stringCopy(in->arg.image.full_path);
			out->arg.image.options = in->arg.image.options;
			break;
		case cr_cn_checkerboard:
			out->arg.checkerboard.a = color_deepcopy(in->arg.checkerboard.a);
			out->arg.checkerboard.b = color_deepcopy(in->arg.checkerboard.b);
			out->arg.checkerboard.scale = value_deepcopy(in->arg.checkerboard.scale);
			break;
		case cr_cn_blackbody:
			out->arg.blackbody.degrees = value_deepcopy(in->arg.blackbody.degrees);
			break;
		case cr_cn_split:
			out->arg.split.node = value_deepcopy(in->arg.split.node);
			break;
		case cr_cn_rgb:
			out->arg.rgb.red = value_deepcopy(in->arg.rgb.red);
			out->arg.rgb.green = value_deepcopy(in->arg.rgb.green);
			out->arg.rgb.blue = value_deepcopy(in->arg.rgb.blue);
			break;
		case cr_cn_hsl:
			out->arg.hsl.H = value_deepcopy(in->arg.hsl.H);
			out->arg.hsl.S = value_deepcopy(in->arg.hsl.S);
			out->arg.hsl.L = value_deepcopy(in->arg.hsl.L);
			break;
		case cr_cn_vec_to_color:
			out->arg.vec_to_color.vec = vector_deepcopy(in->arg.vec_to_color.vec);
		case cr_cn_gradient:
			out->arg.gradient.a = color_deepcopy(in->arg.gradient.a);
			out->arg.gradient.b = color_deepcopy(in->arg.gradient.b);
		default:
			break;
	}
	return out;
}

struct cr_vector_node *vector_deepcopy(const struct cr_vector_node *in) {
	if (!in) return NULL;
	struct cr_vector_node *out = calloc(1, sizeof(*out));
	out->type = in->type;
	switch (in->type) {
		case cr_vec_constant:
			out->arg.constant = in->arg.constant;
		case cr_vec_normal:
		case cr_vec_uv:
			break;
		case cr_vec_vecmath:
			out->arg.vecmath.A = vector_deepcopy(in->arg.vecmath.A);
			out->arg.vecmath.B = vector_deepcopy(in->arg.vecmath.B);
			out->arg.vecmath.C = vector_deepcopy(in->arg.vecmath.C);
			out->arg.vecmath.f = value_deepcopy(in->arg.vecmath.f);
			out->arg.vecmath.op = in->arg.vecmath.op;
			break;
		case cr_vec_mix:
			out->arg.vec_mix.A = vector_deepcopy(in->arg.vec_mix.A);
			out->arg.vec_mix.B = vector_deepcopy(in->arg.vec_mix.B);
			out->arg.vec_mix.factor = value_deepcopy(in->arg.vec_mix.factor);
			break;
		default:
			break;
	}
	return out;
}

struct cr_shader_node *shader_deepcopy(const struct cr_shader_node *in) {
	if (!in) return NULL;
	struct cr_shader_node *out = calloc(1, sizeof(*out));
	out->type = in->type;
	switch (in->type) {
		case cr_bsdf_diffuse:
			out->arg.diffuse.color = color_deepcopy(in->arg.diffuse.color);
			break;
		case cr_bsdf_metal:
			out->arg.metal.color = color_deepcopy(in->arg.metal.color);
			out->arg.metal.roughness = value_deepcopy(in->arg.metal.roughness);
			break;
		case cr_bsdf_glass:
			out->arg.glass.color = color_deepcopy(in->arg.glass.color);
			out->arg.glass.roughness = value_deepcopy(in->arg.glass.roughness);
			out->arg.glass.IOR = value_deepcopy(in->arg.glass.IOR);
			break;
		case cr_bsdf_plastic:
			out->arg.plastic.color = color_deepcopy(in->arg.plastic.color);
			out->arg.plastic.roughness = value_deepcopy(in->arg.plastic.roughness);
			out->arg.plastic.IOR = value_deepcopy(in->arg.plastic.IOR);
			break;
		case cr_bsdf_mix:
			out->arg.mix.A = shader_deepcopy(in->arg.mix.A);
			out->arg.mix.B = shader_deepcopy(in->arg.mix.B);
			out->arg.mix.factor = value_deepcopy(in->arg.mix.factor);
			break;
		case cr_bsdf_add:
			out->arg.add.A = shader_deepcopy(in->arg.add.A);
			out->arg.add.B = shader_deepcopy(in->arg.add.B);
			break;
		case cr_bsdf_transparent:
			out->arg.transparent.color = color_deepcopy(in->arg.transparent.color);
			break;
		case cr_bsdf_emissive:
			out->arg.emissive.color = color_deepcopy(in->arg.emissive.color);
			out->arg.emissive.strength = value_deepcopy(in->arg.emissive.strength);
			break;
		case cr_bsdf_translucent:
			out->arg.translucent.color = color_deepcopy(in->arg.translucent.color);
			break;
		case cr_bsdf_background:
			out->arg.background.color = color_deepcopy(in->arg.background.color);
			out->arg.background.pose = vector_deepcopy(in->arg.background.pose);
			out->arg.background.strength = value_deepcopy(in->arg.background.strength);
		default:
			break;

	}

	return out;
}

void cr_material_set_add(struct cr_scene *s_ext, cr_material_set set, struct cr_shader_node *desc) {
	if (!s_ext) return;
	struct world *s = (struct world *)s_ext;
	if ((size_t)set > s->shader_buffers.count - 1) return;
	struct bsdf_buffer *buf = &s->shader_buffers.items[set];
	const struct bsdfNode *node = build_bsdf_node(s_ext, desc);
	cr_shader_node_ptr_arr_add(&buf->descriptions, shader_deepcopy(desc));
	bsdf_node_ptr_arr_add(&buf->bsdfs, node);
}

struct texture *cr_renderer_render(struct cr_renderer *ext) {
	struct renderer *r = (struct renderer *)ext;
	if (r->prefs.node_list) {
		r->state.clients = clients_sync(r);
	}
	if (!r->state.clients.count && !r->prefs.threads) {
		return NULL;
	}
	return renderer_render(r);
}

void cr_start_render_worker(int port, size_t thread_limit) {
	worker_start(port, thread_limit);
}

void cr_send_shutdown_to_workers(const char *node_list) {
	clients_shutdown(node_list);
}
