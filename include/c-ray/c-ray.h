//
//  c-ray.h
//  C-ray
//
//  Created by Valtteri on 5.1.2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

// Cray public-facing API
#include <stdbool.h>
#include <stdint.h>

struct renderInfo;
struct texture;

//Utilities
char *cr_get_version(void); //The current semantic version
char *cr_get_git_hash(void); //The current git hash of the build

void cr_parse_args(int argc, char **argv);
int cr_is_option_set(char *key);
char *cr_path_arg(void);
void cr_destroy_options(void);

char *cr_get_file_path(char *full_path);


struct cr_renderer;
struct cr_renderer *cr_new_renderer(void);
void cr_destroy_renderer(struct cr_renderer *r);

struct cr_scene;
struct cr_scene *cr_scene_create(struct cr_renderer *r);
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

int cr_load_scene_from_file(struct cr_renderer *r, char *file_path);
int cr_load_scene_from_buf(struct cr_renderer *r, char *buf);

void cr_load_mesh_from_file(char *filePath);
void cr_load_mesh_from_buf(char *buf);

void cr_write_image(struct cr_renderer *r); //Write out the current image to file

void cr_set_thread_count(struct cr_renderer *r, int thread_count, int is_from_system);
int cr_get_thread_count(struct cr_renderer *r);

int cr_get_sample_count(struct cr_renderer *r);

int cr_get_bounces(struct cr_renderer *r);

void cr_set_tile_width(struct cr_renderer *r, unsigned width);
unsigned cr_get_tile_width(struct cr_renderer *r);

void cr_set_tile_height(struct cr_renderer *r, unsigned height);
unsigned cr_get_tile_height(struct cr_renderer *r);

void cr_set_image_width(struct cr_renderer *r, unsigned width);
unsigned cr_get_image_width(struct cr_renderer *r);

void cr_set_image_height(struct cr_renderer *r, unsigned height);
unsigned cr_get_image_height(struct cr_renderer *r);

void cr_set_output_path(struct cr_renderer *r, char *filePath);
char *cr_get_output_path(struct cr_renderer *r);

void cr_set_file_name(struct cr_renderer *r, char *fileName);
char *cr_get_file_name(struct cr_renderer *r);

void cr_set_asset_path(struct cr_renderer *r);

//Single frame
void cr_start_renderer(struct cr_renderer *r);

//Network render worker
void cr_start_render_worker(void);

