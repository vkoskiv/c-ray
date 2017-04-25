#pragma once

#define OBJ_LINE_SIZE 500
#define MAX_VERTEX_COUNT 4 //can only handle quads or triangles

struct obj_face {
	int vertex_index[MAX_VERTEX_COUNT];
	int normal_index[MAX_VERTEX_COUNT];
	int texture_index[MAX_VERTEX_COUNT];
	int vertex_count;
	int material_index;
};

struct obj_sphere {
	int pos_index;
	int up_normal_index;
	int equator_normal_index;
	int texture_index[MAX_VERTEX_COUNT];
	int material_index;
};

struct obj_plane {
	int pos_index;
	int normal_index;
	int rotation_normal_index;
	int texture_index[MAX_VERTEX_COUNT];
	int material_index;
};

struct obj_vector {
	double e[3];
};

struct obj_material {
	char name[MATERIAL_NAME_SIZE];
	char texture_filename[OBJ_FILENAME_LENGTH];
	double amb[3];
	double diff[3];
	double spec[3];
	double reflect;
	double refract;
	double trans;
	double shiny;
	double glossy;
	double refract_index;
};

struct obj_camera {
	int camera_pos_index;
	int camera_look_point_index;
	int camera_up_norm_index;
};

struct obj_light_point {
	int pos_index;
	int material_index;
};

struct obj_light_disc {
	int pos_index;
	int normal_index;
	int material_index;
};

struct obj_light_quad {
	int vertex_index[MAX_VERTEX_COUNT];
	int material_index;
};

struct obj_growable_scene_data {
//	vector extreme_dimensions[2];
	char scene_filename[OBJ_FILENAME_LENGTH];
	char material_filename[OBJ_FILENAME_LENGTH];
	
	list vertex_list;
	list vertex_normal_list;
	list vertex_texture_list;
	
	list face_list;
	list sphere_list;
	list plane_list;
	
	list light_point_list;
	list light_quad_list;
	list light_disc_list;
	
	list material_list;
	
	struct obj_camera *camera;
};

struct obj_scene_data {
	struct obj_vector **vertex_list;
    struct obj_vector **vertex_normal_list;
    struct obj_vector **vertex_texture_list;
	
    struct obj_face **face_list;
    struct obj_sphere **sphere_list;
    struct obj_plane **plane_list;
	
    struct obj_light_point **light_point_list;
    struct obj_light_quad **light_quad_list;
    struct obj_light_disc **light_disc_list;
	
    struct obj_material **material_list;
	
	int vertex_count;
	int vertex_normal_count;
	int vertex_texture_count;

	int face_count;
	int sphere_count;
	int plane_count;

	int light_point_count;
	int light_quad_count;
	int light_disc_count;

	int material_count;

    struct obj_camera *camera;
};

int parse_obj_scene(struct obj_scene_data *data_out, char *filename);
void delete_obj_data(struct obj_scene_data *data_out);
