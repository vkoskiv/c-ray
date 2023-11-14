//
//  c-ray.h
//  c-ray
//
//  Created by Valtteri on 5.1.2020.
//  Copyright © 2020-2023 Valtteri Koskivuori. All rights reserved.
//

#pragma once

// Cray public-facing API
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define C_RAY_PROTO_DEFAULT_PORT 2222

struct renderInfo;
struct texture;

//Utilities
char *cr_get_version(void); //The current semantic version
char *cr_get_git_hash(void); //The current git hash of the build

char *cr_get_file_path(char *full_path);

struct cr_renderer;
struct cr_renderer *cr_new_renderer(void);

enum cr_renderer_param {
	cr_renderer_threads = 0,
	cr_renderer_samples,
	cr_renderer_bounces,
	cr_renderer_tile_width,
	cr_renderer_tile_height,

	cr_renderer_tile_order,
	//TODO: Renderer shouldn't know these
	cr_renderer_output_path,
	cr_renderer_asset_path,
	cr_renderer_output_name,
	cr_renderer_output_filetype,
	cr_renderer_output_num,
	cr_renderer_override_width,
	cr_renderer_override_height,
	cr_renderer_should_save, //FIXME: Remove
	cr_renderer_override_cam,
	cr_renderer_node_list,
	cr_renderer_scene_cache, // FIXME: Remove
	cr_renderer_is_iterative,
};

enum cr_tile_state {
	cr_tile_ready_to_render = 0,
	cr_tile_rendering,
	cr_tile_finished
};

struct cr_tile {
	int w;
	int h;
	int start_x;
	int start_y;
	int end_x;
	int end_y;
	enum cr_tile_state state;
	bool network_renderer;
	int index;
	size_t total_samples;
	size_t completed_samples;
};

struct cr_renderer_cb_info {
	void *user_data;
	const struct texture *fb;
	const struct cr_tile *tiles;
	size_t tiles_count;

	size_t active_threads;
	double avg_per_ray_us;
	int64_t samples_per_sec;
	int64_t eta_ms;
	double completion;
	bool paused;
};

struct cr_renderer_callbacks {
	void (*cr_renderer_on_start)(struct cr_renderer_cb_info *info);
	void (*cr_renderer_on_stop)(struct cr_renderer_cb_info *info);
	void (*cr_renderer_status)(struct cr_renderer_cb_info *info);
	void (*cr_renderer_on_state_changed)(struct cr_renderer_cb_info *info);
	void *user_data;
};

bool cr_renderer_set_num_pref(struct cr_renderer *ext, enum cr_renderer_param p, uint64_t num);
bool cr_renderer_set_str_pref(struct cr_renderer *ext, enum cr_renderer_param p, const char *str);
bool cr_renderer_set_callbacks(struct cr_renderer *ext, struct cr_renderer_callbacks cb);
void cr_renderer_stop(struct cr_renderer *ext, bool should_save);
void cr_renderer_toggle_pause(struct cr_renderer *ext);
const char *cr_renderer_get_str_pref(struct cr_renderer *ext, enum cr_renderer_param p);
uint64_t cr_renderer_get_num_pref(struct cr_renderer *ext, enum cr_renderer_param p);
void cr_destroy_renderer(struct cr_renderer *r);

struct cr_scene;
struct cr_scene *cr_scene_create(struct cr_renderer *r);

//FIXME: This should only have to take cr_scene
bool cr_scene_set_background_hdr(struct cr_renderer *r_ext, struct cr_scene *s_ext, const char *hdr_filename);
struct cr_color {
	float r;
	float g;
	float b;
	float a;
};
bool cr_scene_set_background(struct cr_scene *s_ext, struct cr_color *down, struct cr_color *up);
void cr_scene_destroy(struct cr_scene *s);

enum cr_camera_param {
	cr_camera_fov,
	cr_camera_focus_distance,
	cr_camera_fstops,

	cr_camera_pose_x,
	cr_camera_pose_y,
	cr_camera_pose_z,
	cr_camera_pose_roll,
	cr_camera_pose_pitch,
	cr_camera_pose_yaw,

	cr_camera_time,

	cr_camera_res_x,
	cr_camera_res_y,
};

typedef int32_t cr_camera;
cr_camera cr_camera_new(struct cr_scene *ext);
bool cr_camera_set_num_pref(struct cr_scene *ext, cr_camera c, enum cr_camera_param p, double num);
bool cr_camera_update(struct cr_scene *ext, cr_camera c);

void cr_load_mesh_from_file(char *filePath);
void cr_load_mesh_from_buf(char *buf);

//Single frame
struct texture *cr_renderer_render(struct cr_renderer *r);

//Network render worker
void cr_start_render_worker(int port, size_t thread_limit);

