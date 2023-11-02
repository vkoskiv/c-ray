//
//  c-ray.h
//  C-ray
//
//  Created by Valtteri on 5.1.2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

// Cray public-facing API

struct renderInfo;
struct texture;
struct renderer;

//Utilities
char *cr_get_version(void); //The current semantic version

char *cr_get_git_hash(void); //The current git hash of the build

void cr_initialize(void); //Run initial setup of the environment

void cr_parse_args(int argc, char **argv);
int cr_is_option_set(char *key);
char *cr_path_arg(void);
void cr_destroy_options(void);

char *cr_get_file_path(char *full_path);

void cr_write_image(struct renderer *r); //Write out the current image to file

struct renderer *cr_new_renderer(void);
void cr_destroy_renderer(struct renderer *r);

int cr_load_scene_from_file(struct renderer *r, char *file_path);
int cr_load_scene_from_buf(struct renderer *r, char *buf);

void cr_load_mesh_from_file(char *filePath);
void cr_load_mesh_from_buf(char *buf);

//Preferences
void cr_set_render_order(void);
void cr_get_render_order(void);

void cr_set_thread_count(struct renderer *r, int thread_count, int is_from_system);
int cr_get_thread_count(struct renderer *r);

void cr_set_sample_count(struct renderer *r, int sample_count);
int cr_get_sample_count(struct renderer *r);

void cr_set_bounces(struct renderer *r, int bounces);
int cr_get_bounces(struct renderer *r);

void cr_set_tile_width(struct renderer *r, unsigned width);
unsigned cr_get_tile_width(struct renderer *r);

void cr_set_tile_height(struct renderer *r, unsigned height);
unsigned cr_get_tile_height(struct renderer *r);

void cr_set_image_width(struct renderer *r, unsigned width);
unsigned cr_get_image_width(struct renderer *r);

void cr_set_image_height(struct renderer *r, unsigned height);
unsigned cr_get_image_height(struct renderer *r);

void cr_set_output_path(struct renderer *r, char *filePath);
char *cr_get_output_path(struct renderer *r);

void cr_set_file_name(struct renderer *r, char *fileName);
char *cr_get_file_name(struct renderer *r);

void cr_set_asset_path(struct renderer *r);
char *cr_get_asset_path(struct renderer *r);

//Single frame
void cr_start_renderer(struct renderer *r);

//Network render worker
void cr_start_render_worker(void);

//Interactive mode
void cr_start_interactive(void);
void cr_pause_interactive(void); //Toggle paused state
void cr_get_current_image(void); //Just get the current buffer
void cr_restart_interactive(void);

void cr_transform_mesh(void); //Transform, recompute kd-tree, restart

void cr_move_camera(void/*struct dimension delta*/);
void cr_set_hdr(void);

