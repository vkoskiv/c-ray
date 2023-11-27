//
//  c-ray.c
//  c-ray
//
//  Created by Valtteri on 5.1.2020.
//  Copyright © 2020-2023 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include <c-ray/c-ray.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

#include "../renderer/renderer.h"
#include "../datatypes/scene.h"
#include "../utils/gitsha1.h"
#include "../utils/logging.h"
#include "../utils/fileio.h"
#include "../utils/platform/terminal.h"
#include "../utils/assert.h"
#include "../datatypes/image/texture.h"
#include "../utils/string.h"
#include "../utils/protocol/server.h"
#include "../utils/protocol/worker.h"
#include "../utils/filecache.h"
#include "../utils/hashtable.h"
#include "../datatypes/camera.h"
#include "../utils/loaders/textureloader.h"

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
			logr(info, "Selected camera %lu\n", num);
			return true;
		}
		case cr_renderer_is_iterative: {
			r->prefs.iterative = true;
			return true;
		}
		default: {
			logr(warning, "Renderer param %i not a number\n", p);
		}
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
			if (r->prefs.assetPath) free(r->prefs.assetPath);
			r->prefs.assetPath = stringCopy(str);
			return true;
		}
		case cr_renderer_output_name: {
			if (r->prefs.imgFileName) free(r->prefs.imgFileName);
			r->prefs.imgFileName = stringCopy(str);
			return true;
		}
		case cr_renderer_output_filetype: {
			if (stringEquals(str, "bmp")) {
				r->prefs.imgType = bmp;
			} else if (stringEquals(str, "png")) {
				r->prefs.imgType = png;
			} else if (stringEquals(str, "qoi")) {
				r->prefs.imgType = qoi;
			} else {
				return false;
			}
			return true;
		}
		case cr_renderer_node_list: {
			if (r->prefs.node_list) free(r->prefs.node_list);
			r->prefs.node_list = stringCopy(str);
			if (!r->state.file_cache) r->state.file_cache = cache_create();
			return true;
		}
		case cr_renderer_scene_cache: {
			if (r->sceneCache) free(r->sceneCache);
			r->sceneCache = stringCopy(str);
			return true;
		}
		default: {
			logr(warning, "Renderer param %i not a string\n", p);
		}
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
		case cr_renderer_asset_path: return r->prefs.assetPath;
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
		case cr_renderer_output_filetype: return r->prefs.imgType;
		default: return 0; // TODO
	}
	return 0;
}

bool cr_scene_set_background(struct cr_renderer *r_ext, struct cr_scene *s_ext, struct cr_shader_node *desc) {
	if (!r_ext || !s_ext) return false;
	struct world *s = (struct world *)s_ext;
	s->background = desc ? build_bsdf_node(r_ext, desc) : newBackground(&s->storage, NULL, NULL, NULL);
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

struct cr_vertex_buf *cr_vertex_buf_new(struct cr_vertex_buf in) {
	struct vertex_buffer *buf = vertex_buf_ref(NULL);
	// TODO: T_arr_add_n()
	if (in.vertices && in.vertex_count) {
		for (size_t i = 0; i < in.vertex_count; ++i) {
			vector_arr_add(&buf->vertices, *(struct vector *)&in.vertices[i]);
		}
	}
	if (in.normals && in.normal_count) {
		for (size_t i = 0; i < in.normal_count; ++i) {
			vector_arr_add(&buf->normals, *(struct vector *)&in.normals[i]);
		}
	}
	if (in.tex_coords && in.tex_coord_count) {
		for (size_t i = 0; i < in.tex_coord_count; ++i) {
			coord_arr_add(&buf->texture_coords, *(struct coord *)&in.tex_coords[i]);
		}
	}
	return (struct cr_vertex_buf *)buf;
}

void cr_vertex_buf_del(struct cr_vertex_buf *buf) {
	vertex_buf_unref((struct vertex_buffer *)buf);
}

void cr_mesh_bind_vertex_buf(struct cr_scene *s_ext, cr_mesh mesh, struct cr_vertex_buf *buf) {
	if (!s_ext || !buf) return;
	struct world *scene = (struct world *)s_ext;
	if ((size_t)mesh > scene->meshes.count - 1) return;
	struct mesh *m = &scene->meshes.items[mesh];
	m->vbuf = vertex_buf_ref((struct vertex_buffer *)buf);
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

bool cr_instance_bind_material_set(struct cr_renderer *r_ext, cr_instance instance, cr_material_set *set) {
	if (!r_ext || !set) return false;
	struct renderer *r = (struct renderer *)r_ext;
	struct world *scene = r->scene;
	if ((size_t)instance > scene->instances.count - 1) return false;
	struct instance *i = &scene->instances.items[instance];
	i->bbuf = bsdf_buf_ref((struct bsdf_buffer *)set);
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

cr_material_set *cr_material_set_new(void) {
	return (cr_material_set *)bsdf_buf_ref(NULL);
}

void cr_material_set_add(struct cr_renderer *r_ext, cr_material_set *set, struct cr_shader_node *desc) {
	if (!set || !desc) return;
	struct bsdf_buffer *buf = (struct bsdf_buffer *)set;
	const struct bsdfNode *node = build_bsdf_node(r_ext, desc);
	bsdf_node_ptr_arr_add(&buf->bsdfs, node);
}

void cr_material_set_del(cr_material_set *set) {
	if (!set) return;
	bsdf_buf_unref((struct bsdf_buffer *)set);
}

struct texture *cr_renderer_render(struct cr_renderer *ext) {
	struct renderer *r = (struct renderer *)ext;
	if (r->prefs.node_list) {
		r->state.clients = clients_sync(r);
		free(r->sceneCache);
		r->sceneCache = NULL;
		cache_destroy(r->state.file_cache);
	}
	if (!r->state.clients.count && !r->prefs.threads) {
		logr(warning, "You specified 0 local threads, and no network clients were found. Nothing to do.\n");
		return NULL;
	}
	return renderFrame(r);
}

void cr_start_render_worker(int port, size_t thread_limit) {
	worker_start(port, thread_limit);
}

