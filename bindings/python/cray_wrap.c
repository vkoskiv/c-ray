//
//  cray.c
//  libc-ray CPython wrapper
//
//  Created by Valtteri on 5.12.2023.
//  Copyright © 2023 Valtteri Koskivuori. All rights reserved.
//

/*
	Note: We define _POSIX_C_SOURCE on the command line because our includes
	are a huge mess, for the time being. Python wants 200809L instead of our
	default of '200112L', so we make an exception here to suppress warnings.
*/
#ifdef _POSIX_C_SOURCE
	#undef _POSIX_C_SOURCE
#endif
#include <Python.h>
#include <structmember.h>
#include <c-ray/c-ray.h>
#include "py_types.h"

static PyObject *py_cr_get_version(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	return PyUnicode_FromString(cr_get_version());
}

static PyObject *py_cr_get_git_hash(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	return PyUnicode_FromString(cr_get_git_hash());
}

static PyObject *py_cr_new_renderer(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	struct cr_renderer *new = cr_new_renderer();
	if (!new) {
		PyErr_SetString(PyExc_MemoryError, "Failed to allocate renderer");
		return NULL;
	}
	return PyCapsule_New(new, "cray.cr_renderer", NULL);
}

static PyObject *py_cr_destroy_renderer(PyObject *self, PyObject *args) {
	(void)self;
	PyObject *r_ext;
	if (!PyArg_ParseTuple(args, "O", &r_ext)) {
		return NULL;
	}
	struct cr_renderer *r = PyCapsule_GetPointer(r_ext, "cray.cr_renderer");
	cr_destroy_renderer(r);
	Py_RETURN_NONE;
}

static PyObject *py_cr_renderer_set_num_pref(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *r_ext;
	enum cr_renderer_param p;
	uint64_t num;

	if (!PyArg_ParseTuple(args, "OIk", &r_ext, &p, &num)) {
		return NULL;
	}
	struct cr_renderer *r = PyCapsule_GetPointer(r_ext, "cray.cr_renderer");
	bool ret = cr_renderer_set_num_pref(r, p, num);
	return PyBool_FromLong(ret);
}

static PyObject *py_cr_renderer_set_str_pref(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *r_ext;
	enum cr_renderer_param p;
	char *str;

	if (!PyArg_ParseTuple(args, "OIs", &r_ext, &p, &str)) {
		return NULL;
	}
	struct cr_renderer *r = PyCapsule_GetPointer(r_ext, "cray.cr_renderer");
	bool ret = cr_renderer_set_str_pref(r, p, str);
	return PyBool_FromLong(ret);
}

void py_callable_wrapper(struct cr_renderer_cb_info *info, void *arg) {
	PyObject *py_callback_fn = NULL;
	PyObject *py_user_data = NULL;
	PyGILState_STATE state = PyGILState_Ensure();
	if (!PyArg_ParseTuple(arg, "OO", &py_callback_fn, &py_user_data)) {
		printf("Failed to parse args in py_callable_wrapper\n");
		return;
	}
	PyObject *args = Py_BuildValue("()");
	py_renderer_cb_info *cb = (py_renderer_cb_info *)type_py_renderer_cb_info.tp_new(&type_py_renderer_cb_info, args, NULL);
	Py_DECREF(args);
	cb->info = *info;
	PyObject *py_args = Py_BuildValue("(OO)", cb, py_user_data);
	if (!py_args) {
		printf("In py_callable_wrapper: ");
		PyErr_Print();
		return;
	}
	Py_DECREF(cb);
	Py_INCREF(py_args);
	PyObject *result = PyObject_Call(py_callback_fn, py_args, NULL);
	if (result) Py_DECREF(result);
	Py_DECREF(py_args);
	PyGILState_Release(state);
}

static PyObject *py_cr_renderer_set_callback(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *r_ext;
	enum cr_renderer_callback callback_type;
	PyObject *py_callback_fn;
	PyObject *py_user_data = NULL;

	if (!PyArg_ParseTuple(args, "OIO|O", &r_ext, &callback_type, &py_callback_fn, &py_user_data)) {
		return NULL;
	}
	if (callback_type > cr_cb_on_interactive_pass_finished) {
		PyErr_SetString(PyExc_ValueError, "Unknown callback type");
		return NULL;
	}
	if (!PyCallable_Check(py_callback_fn)) {
		PyErr_SetString(PyExc_ValueError, "callback must be callable");
		return NULL;
	}

	struct cr_renderer *r = PyCapsule_GetPointer(r_ext, "cray.cr_renderer");
	PyObject *py_arg = Py_BuildValue("(OO)", py_callback_fn, py_user_data);
	if (!py_arg) {
		printf("In py_cr_renderer_set_callback: ");
		PyErr_Print();
		return NULL;
	}
	Py_INCREF(py_arg);
	bool ret = cr_renderer_set_callback(r, callback_type, py_callable_wrapper, py_arg);
	return PyBool_FromLong(ret);
}

static PyObject *py_cr_renderer_stop(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *r_ext;

	if (!PyArg_ParseTuple(args, "O", &r_ext)) {
		return NULL;
	}
	struct cr_renderer *r = PyCapsule_GetPointer(r_ext, "cray.cr_renderer");
	Py_BEGIN_ALLOW_THREADS
	cr_renderer_stop(r);
	Py_END_ALLOW_THREADS
	Py_RETURN_NONE;
}

static PyObject *py_cr_renderer_restart(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *r_ext;

	if (!PyArg_ParseTuple(args, "O", &r_ext)) {
		return NULL;
	}
	struct cr_renderer *r = PyCapsule_GetPointer(r_ext, "cray.cr_renderer");
	// Unsure why, but we get a deadlock if we're restarting the renderer while it's calling
	// the Python callback to report a pass was finished and we don't wrap this call with these
	// begin/allow macros. Adding them fixed the deadlock, so I'm happy.
	Py_BEGIN_ALLOW_THREADS;
	cr_renderer_restart_interactive(r);
	Py_END_ALLOW_THREADS;
	Py_RETURN_NONE;
}

static PyObject *py_cr_renderer_toggle_pause(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *r_ext;

	if (!PyArg_ParseTuple(args, "O", &r_ext)) {
		return NULL;
	}
	struct cr_renderer *r = PyCapsule_GetPointer(r_ext, "cray.cr_renderer");
	cr_renderer_toggle_pause(r);
	Py_RETURN_NONE;
}

static PyObject *py_cr_renderer_get_str_pref(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *r_ext;
	enum cr_renderer_param p;
	if (!PyArg_ParseTuple(args, "OI", &r_ext, &p)) {
		return NULL;
	}
	struct cr_renderer *r = PyCapsule_GetPointer(r_ext, "cray.cr_renderer");
	const char *ret = cr_renderer_get_str_pref(r, p);
	if (!ret) {
		PyErr_SetString(PyExc_ValueError, "cr_renderer_param not a string type");
		return NULL;
	}
	return PyUnicode_FromString(ret);
}

static PyObject *py_cr_renderer_get_num_pref(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *r_ext;
	enum cr_renderer_param p;
	if (!PyArg_ParseTuple(args, "OI", &r_ext, &p)) {
		return NULL;
	}
	if (p > cr_renderer_is_iterative) {
		PyErr_SetString(PyExc_ValueError, "cr_renderer_param not a number type");
		return NULL;
	}
	struct cr_renderer *r = PyCapsule_GetPointer(r_ext, "cray.cr_renderer");
	uint64_t ret = cr_renderer_get_num_pref(r, p);
	return PyLong_FromUnsignedLong(ret);
}

static PyObject *py_cr_renderer_get_result(PyObject *self, PyObject *args) {
	(void)self;
	PyObject *r_ext;
	if (!PyArg_ParseTuple(args, "O", &r_ext)) {
		return NULL;
	}
	struct cr_renderer *r = PyCapsule_GetPointer(r_ext, "cray.cr_renderer");
	struct cr_bitmap *bm = cr_renderer_get_result(r);
	if (!bm) Py_RETURN_NONE;
	return py_bitmap_wrap(bm);
}

static PyObject *py_cr_renderer_render(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *r_ext;
	if (!PyArg_ParseTuple(args, "O", &r_ext)) {
		return NULL;
	}
	struct cr_renderer *r = PyCapsule_GetPointer(r_ext, "cray.cr_renderer");
	Py_BEGIN_ALLOW_THREADS
	cr_renderer_render(r);
	Py_END_ALLOW_THREADS
	Py_RETURN_NONE;
}

static PyObject *py_cr_renderer_start_interactive(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *r_ext;
	if (!PyArg_ParseTuple(args, "O", &r_ext)) {
		return NULL;
	}
	struct cr_renderer *r = PyCapsule_GetPointer(r_ext, "cray.cr_renderer");
	cr_renderer_start_interactive(r);
	Py_RETURN_NONE;
}

// TODO: Same here, not sure if we want an explicit teardown
// static PyObject *py_cr_bitmap_free(PyObject *self, PyObject *args) {
// 	Py_RETURN_NOTIMPLEMENTED;
// }

static PyObject *py_cr_renderer_scene_get(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *r_ext;
	if (!PyArg_ParseTuple(args, "O", &r_ext)) {
		return NULL;
	}
	struct cr_renderer *r = PyCapsule_GetPointer(r_ext, "cray.cr_renderer");
	struct cr_scene *s = cr_renderer_scene_get(r);
	return PyCapsule_New(s, "cray.cr_scene", NULL);
}

static PyObject *py_cr_scene_totals(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *s_ext;
	if (!PyArg_ParseTuple(args, "O", &s_ext)) {
		return NULL;
	}
	struct cr_scene *s = PyCapsule_GetPointer(s_ext, "cray.cr_scene");
	struct cr_scene_totals totals = cr_scene_totals(s);
	return Py_BuildValue(
		"{s:i, s:i, s:i, s:i}",
		"meshes", totals.meshes,
		"spheres", totals.spheres,
		"instances", totals.instances,
		"cameras", totals.cameras);
}

static PyObject *py_cr_scene_add_sphere(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *s_ext;
	float radius;
	if (!PyArg_ParseTuple(args, "Of", &s_ext, &radius)) {
		return NULL;
	}
	struct cr_scene *s = PyCapsule_GetPointer(s_ext, "cray.cr_scene");
	cr_sphere sphere = cr_scene_add_sphere(s, radius);
	return PyLong_FromLong(sphere);
}

static PyObject *py_cr_mesh_bind_vertex_buf(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *s_ext;
	cr_mesh mesh;
	PyObject *vec_buff;
	size_t vec_count;
	PyObject *nor_buff;
	size_t nor_count;
	PyObject *tex_buff;
	size_t tex_count;
	if (!PyArg_ParseTuple(args, "OlOnOnOn", &s_ext, &mesh, &vec_buff, &vec_count, &nor_buff, &nor_count, &tex_buff, &tex_count)) {
		return NULL;
	}
	Py_buffer vec_view;
	if (PyObject_GetBuffer(vec_buff, &vec_view, PyBUF_C_CONTIGUOUS | PyBUF_FORMAT) == -1) {
		PyErr_SetString(PyExc_MemoryError, "Failed to parse vec_view");
		return NULL;
	}
	if (vec_count && (vec_view.len / sizeof(struct cr_vector)) != vec_count) {
		printf("vec_count: %zu\n", vec_count);
		printf("vec_view.len: %zu\n", vec_view.len);
		PyErr_SetString(PyExc_MemoryError, "vec_view / sizeof(struct cr_vector) != vec_count");
		return NULL;
	}

	Py_buffer nor_view;
	if (PyObject_GetBuffer(nor_buff, &nor_view, PyBUF_C_CONTIGUOUS | PyBUF_FORMAT) == -1) {
		PyErr_SetString(PyExc_MemoryError, "Failed to parse nor_view");
		return NULL;
	}
	if (nor_count && (nor_view.len / sizeof(struct cr_vector)) != nor_count) {
		printf("nor_count: %zu\n", nor_count);
		printf("nor_view.len: %zu\n", nor_view.len);
		PyErr_SetString(PyExc_MemoryError, "nor_view / sizeof(struct cr_vector) != nor_count");
		return NULL;
	}

	Py_buffer tex_view;
	if (PyObject_GetBuffer(tex_buff, &tex_view, PyBUF_C_CONTIGUOUS | PyBUF_FORMAT) == -1) {
		PyErr_SetString(PyExc_MemoryError, "Failed to parse tex_view");
		return NULL;
	}
	if (tex_count && (tex_view.len / sizeof(struct cr_coord)) != tex_count) {
		printf("tex_count: %zu\n", tex_count);
		printf("tex_view.len: %zu\n", tex_view.len);
		PyErr_SetString(PyExc_MemoryError, "tex_view / sizeof(struct cr_coord) != tex_count");
		return NULL;
	}

	// TODO: add checks
	struct cr_vector *vertices = calloc(vec_count, sizeof(*vertices));
	struct cr_vector *normals = calloc(nor_count, sizeof(*normals));
	struct cr_coord *texcoords = calloc(tex_count, sizeof(*texcoords));
	memcpy(vertices, vec_view.buf, vec_count * sizeof(*vertices));
	memcpy(normals, nor_view.buf, nor_count * sizeof(*normals));
	memcpy(texcoords, tex_view.buf, tex_count * sizeof(*texcoords));

	struct cr_scene *s = PyCapsule_GetPointer(s_ext, "cray.cr_scene");
	cr_mesh_bind_vertex_buf(s, mesh, (struct cr_vertex_buf_param){
		.vertices = vertices,
		.vertex_count = vec_count,
		.normals = normals,
		.normal_count = nor_count,
		.tex_coords = texcoords,
		.tex_coord_count = tex_count
	});

	free(vertices);
	free(normals);
	free(texcoords);
	Py_RETURN_NONE;
}

static PyObject *py_cr_mesh_bind_faces(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *s_ext;
	cr_mesh mesh;
	PyObject *face_buff;
	Py_buffer face_view;
	size_t face_count;
	if (!PyArg_ParseTuple(args, "OlOn", &s_ext, &mesh, &face_buff, &face_count)) {
		return NULL;
	}
	if (PyObject_GetBuffer(face_buff, &face_view, PyBUF_C_CONTIGUOUS | PyBUF_FORMAT) == -1) {
		return NULL;
	}

	if ((face_view.len / sizeof(struct cr_face)) != face_count) {
		printf("face_count: %zu\n", face_count);
		PyErr_SetString(PyExc_MemoryError, "face_view / sizeof(struct cr_face) != face_count");
		return NULL;
	}

	struct cr_face *faces = calloc(face_count, sizeof(*faces));
	memcpy(faces, face_view.buf, face_count * sizeof(*faces));

	struct cr_scene *s = PyCapsule_GetPointer(s_ext, "cray.cr_scene");
	cr_mesh_bind_faces(s, mesh, faces, face_count);
	free(faces);
	Py_RETURN_NONE;
}

static PyObject *py_cr_scene_mesh_new(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *s_ext;
	char *name;
	if (!PyArg_ParseTuple(args, "Os", &s_ext, &name)) {
		return NULL;
	}
	if (!name) {
		PyErr_SetString(PyExc_ValueError, "Name can't be empty");
		return NULL;
	}
	struct cr_scene *s = PyCapsule_GetPointer(s_ext, "cray.cr_scene");
	cr_mesh mesh = cr_scene_mesh_new(s, name);
	return PyLong_FromLong(mesh);
}

static PyObject *py_cr_scene_get_mesh(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *s_ext;
	char *name;
	if (!PyArg_ParseTuple(args, "Os", &s_ext, &name)) {
		return NULL;
	}
	if (!name) {
		PyErr_SetString(PyExc_ValueError, "Name can't be empty");
		return NULL;
	}
	struct cr_scene *s = PyCapsule_GetPointer(s_ext, "cray.cr_scene");
	cr_mesh mesh = cr_scene_get_mesh(s, name);
	return PyLong_FromLong(mesh);
}

static PyObject *py_cr_camera_new(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *s_ext;
	if (!PyArg_ParseTuple(args, "O", &s_ext)) {
		return NULL;
	}
	struct cr_scene *s = PyCapsule_GetPointer(s_ext, "cray.cr_scene");
	cr_camera new = cr_camera_new(s);
	return PyLong_FromLong(new);
}

static PyObject *py_cr_camera_set_num_pref(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *s_ext;
	cr_camera cam;
	enum cr_camera_param param;
	double num;
	if (!PyArg_ParseTuple(args, "Olid", &s_ext, &cam, &param, &num)) {
		return NULL;
	}
	struct cr_scene *s = PyCapsule_GetPointer(s_ext, "cray.cr_scene");
	bool ret = cr_camera_set_num_pref(s, cam, param, num);
	return PyBool_FromLong(ret);
}

static PyObject *py_cr_camera_get_num_pref(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *s_ext;
	cr_camera cam;
	enum cr_camera_param param;
	if (!PyArg_ParseTuple(args, "Oli", &s_ext, &cam, &param)) {
		return NULL;
	}
	struct cr_scene *s = PyCapsule_GetPointer(s_ext, "cray.cr_scene");
	double value = cr_camera_get_num_pref(s, cam, param);
	return PyLong_FromDouble(value);
}

static PyObject *py_cr_camera_update(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *s_ext;
	cr_camera cam;
	if (!PyArg_ParseTuple(args, "Ol", &s_ext, &cam)) {
		return NULL;
	}
	struct cr_scene *s = PyCapsule_GetPointer(s_ext, "cray.cr_scene");
	bool ret = cr_camera_update(s, cam);
	return PyBool_FromLong(ret);
}

static PyObject *py_cr_scene_new_material_set(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *s_ext;
	if (!PyArg_ParseTuple(args, "O", &s_ext)) {
		return NULL;
	}
	struct cr_scene *s = PyCapsule_GetPointer(s_ext, "cray.cr_scene");
	cr_material_set new = cr_scene_new_material_set(s);
	return PyLong_FromLong(new);
}

static PyObject *py_cr_material_set_add(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *s_ext;
	cr_material_set set;
	PyObject *node_desc;
	if (!PyArg_ParseTuple(args, "OlO", &s_ext, &set, &node_desc)) {
		return NULL;
	}
	struct cr_scene *s = PyCapsule_GetPointer(s_ext, "cray.cr_scene");
	if (!PyCapsule_IsValid(node_desc, "cray.shader_node")) {
		cr_material new = cr_material_set_add(s, set, NULL);
		return PyLong_FromLong(new);
	}
	struct cr_shader_node *desc = PyCapsule_GetPointer(node_desc, "cray.shader_node");
	cr_material new = cr_material_set_add(s, set, desc);
	return PyLong_FromLong(new);
}

static PyObject *py_cr_material_update(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *s_ext;
	cr_material_set set;
	cr_material mat;
	PyObject *node_desc;
	if (!PyArg_ParseTuple(args, "OllO", &s_ext, &set, &mat, &node_desc)) {
		return NULL;
	}
	struct cr_scene *s = PyCapsule_GetPointer(s_ext, "cray.cr_scene");
	if (!PyCapsule_IsValid(node_desc, "cray.shader_node")) {
		Py_RETURN_NONE;
	}
	struct cr_shader_node *desc = PyCapsule_GetPointer(node_desc, "cray.shader_node");
	cr_material_update(s, set, mat, desc);
	Py_RETURN_NONE;
}

static PyObject *py_cr_instance_new(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *s_ext;
	cr_object object;
	enum cr_object_type type;
	if (!PyArg_ParseTuple(args, "OlI", &s_ext, &object, &type)) {
		return NULL;
	}
	if (type != cr_object_mesh && type != cr_object_sphere) {
		PyErr_SetString(PyExc_ValueError, "Unknown cr_object_type");
		return NULL;
	}
	struct cr_scene *s = PyCapsule_GetPointer(s_ext, "cray.cr_scene");
	cr_instance new = cr_instance_new(s, object, type);
	return PyLong_FromLong(new);
}

static PyObject *py_cr_instance_set_transform(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *s_ext;
	cr_instance instance;
	PyObject *mtx_buff;
	Py_buffer mtx_view;
	if (!PyArg_ParseTuple(args, "OlO", &s_ext, &instance, &mtx_buff)) {
		return NULL;
	}
	if (PyObject_GetBuffer(mtx_buff, &mtx_view, PyBUF_C_CONTIGUOUS | PyBUF_FORMAT) == -1) {
		return NULL;
	}
	if ((mtx_view.len / sizeof(float)) != 4 * 4) {
		PyErr_SetString(PyExc_MemoryError, "mtx_view / sizeof(float) != 4*4");
		return NULL;
	}
	float mtx[4][4];
	memcpy(mtx, mtx_view.buf, 4 * 4 * sizeof(float));
	struct cr_scene *s = PyCapsule_GetPointer(s_ext, "cray.cr_scene");
	cr_instance_set_transform(s, instance, mtx);
	Py_RETURN_NONE;
}

static PyObject *py_cr_instance_transform(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *s_ext;
	cr_instance instance;
	PyObject *mtx_buff;
	Py_buffer mtx_view;
	if (!PyArg_ParseTuple(args, "OlO", &s_ext, &instance, &mtx_buff)) {
		return NULL;
	}
	if (PyObject_GetBuffer(mtx_buff, &mtx_view, PyBUF_C_CONTIGUOUS | PyBUF_FORMAT) == -1) {
		return NULL;
	}
	if ((mtx_view.len / sizeof(float)) != 4 * 4) {
		PyErr_SetString(PyExc_MemoryError, "mtx_view / sizeof(float) != 4*4");
		return NULL;
	}
	float mtx[4][4];
	memcpy(mtx, mtx_view.buf, 4 * 4 * sizeof(float));
	struct cr_scene *s = PyCapsule_GetPointer(s_ext, "cray.cr_scene");
	cr_instance_transform(s, instance, mtx);
	Py_RETURN_NONE;
}

static PyObject *py_cr_instance_bind_material_set(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *s_ext;
	cr_instance instance;
	cr_material_set set;
	if (!PyArg_ParseTuple(args, "Oll", &s_ext, &instance, &set)) {
		return NULL;
	}
	struct cr_scene *s = PyCapsule_GetPointer(s_ext, "cray.cr_scene");
	bool ret = cr_instance_bind_material_set(s, instance, set);
	return PyBool_FromLong(ret);
}

static PyObject *py_cr_scene_set_background(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	PyObject *s_ext;
	PyObject *node_desc;
	if (!PyArg_ParseTuple(args, "OO", &s_ext, &node_desc)) {
		return NULL;
	}
	struct cr_scene *s = PyCapsule_GetPointer(s_ext, "cray.cr_scene");
	if (!PyCapsule_IsValid(node_desc, "cray.shader_node")) {
		cr_scene_set_background(s, NULL);
		Py_RETURN_NONE;
	}
	struct cr_shader_node *desc = PyCapsule_GetPointer(node_desc, "cray.shader_node");
	bool ret = cr_scene_set_background(s, desc);
	return PyBool_FromLong(ret);
}

static PyObject *py_cr_start_render_worker(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	int port = 2222;
	size_t thread_limit = 0;
	if (!PyArg_ParseTuple(args, "|in", &port, &thread_limit)) {
		return NULL;
	}
	Py_BEGIN_ALLOW_THREADS
	cr_start_render_worker(port, thread_limit);
	Py_END_ALLOW_THREADS
	Py_RETURN_NONE;
}

static PyObject *py_cr_send_shutdown_to_workers(PyObject *self, PyObject *args) {
	(void)self; (void)args;
	char *str = NULL;
	if (!PyArg_ParseTuple(args, "s", &str)) {
		return NULL;
	}
	Py_BEGIN_ALLOW_THREADS
	cr_send_shutdown_to_workers(str);
	Py_END_ALLOW_THREADS
	Py_RETURN_NONE;
}

static PyObject *py_cr_load_json(PyObject *self, PyObject *args) {
	(void)self;
	PyObject *r_ext;
	char *path = NULL;
	if (!PyArg_ParseTuple(args, "Os", &r_ext, &path)) {
		return NULL;
	}
	struct cr_renderer *r = PyCapsule_GetPointer(r_ext, "cray.cr_renderer");
	bool ret = cr_load_json(r, path);
	return PyBool_FromLong(ret);
}

static PyObject *py_log_level_set(PyObject *self, PyObject *args) {
	(void)self;
	enum cr_log_level level;
	if (!PyArg_ParseTuple(args, "I", &level)) {
		return NULL;
	}
	cr_log_level_set(level);
	Py_RETURN_NONE;
}

static PyObject *py_log_level_get(PyObject *self, PyObject *args) {
	(void)self;
	(void)args;
	return PyLong_FromLong(cr_log_level_get());
}

static PyObject *py_debug_dump_state(PyObject *self, PyObject *args) {
	(void)self;
	PyObject *r_ext;
	if (!PyArg_ParseTuple(args, "O", &r_ext)) {
		return NULL;
	}
	struct cr_renderer *r = PyCapsule_GetPointer(r_ext, "cray.cr_renderer");
	cr_debug_dump_state(r);
	Py_RETURN_NONE;
}
static PyMethodDef cray_methods[] = {
	{ "get_version", py_cr_get_version, METH_NOARGS, "" },
	{ "get_git_hash", py_cr_get_git_hash, METH_NOARGS, "" },
	{ "renderer_destroy", py_cr_destroy_renderer, METH_VARARGS, "" },
	{ "new_renderer", py_cr_new_renderer, METH_NOARGS, "" },
	// { "destroy_renderer", py_cr_destroy_renderer, METH_VARARGS, "" },
	{ "renderer_set_num_pref", py_cr_renderer_set_num_pref, METH_VARARGS, "" },
	{ "renderer_set_str_pref", py_cr_renderer_set_str_pref, METH_VARARGS, "" },
	{ "renderer_set_callback", py_cr_renderer_set_callback, METH_VARARGS, "" },
	{ "renderer_stop", py_cr_renderer_stop, METH_VARARGS, "" },
	{ "renderer_restart", py_cr_renderer_restart, METH_VARARGS, "" },
	{ "renderer_toggle_pause", py_cr_renderer_toggle_pause, METH_VARARGS, "" },
	{ "renderer_get_str_pref", py_cr_renderer_get_str_pref, METH_VARARGS, "" },
	{ "renderer_get_num_pref", py_cr_renderer_get_num_pref, METH_VARARGS, "" },
	{ "renderer_get_result", py_cr_renderer_get_result, METH_VARARGS, "" },
	{ "renderer_render", py_cr_renderer_render, METH_VARARGS, "" },
	{ "renderer_start_interactive", py_cr_renderer_start_interactive, METH_VARARGS, "" },
	// { "bitmap_free", py_cr_bitmap_free, METH_VARARGS, "" },
	{ "renderer_scene_get", py_cr_renderer_scene_get, METH_VARARGS, "" },
	{ "scene_totals", py_cr_scene_totals, METH_VARARGS, "" },
	{ "scene_add_sphere", py_cr_scene_add_sphere, METH_VARARGS, "" },
	{ "mesh_bind_vertex_buf", py_cr_mesh_bind_vertex_buf, METH_VARARGS, "" },
	{ "mesh_bind_faces", py_cr_mesh_bind_faces, METH_VARARGS, "" },
	{ "scene_mesh_new", py_cr_scene_mesh_new, METH_VARARGS, "" },
	{ "scene_get_mesh", py_cr_scene_get_mesh, METH_VARARGS, "" },
	{ "camera_new", py_cr_camera_new, METH_VARARGS, "" },
	{ "camera_set_num_pref", py_cr_camera_set_num_pref, METH_VARARGS, "" },
	{ "camera_get_num_pref", py_cr_camera_get_num_pref, METH_VARARGS, "" },
	{ "camera_update", py_cr_camera_update, METH_VARARGS, "" },
	{ "scene_new_material_set", py_cr_scene_new_material_set, METH_VARARGS, "" },
	{ "material_set_add", py_cr_material_set_add, METH_VARARGS, "" },
	{ "material_update", py_cr_material_update, METH_VARARGS, "" },
	{ "instance_new", py_cr_instance_new, METH_VARARGS, "" },
	{ "instance_set_transform", py_cr_instance_set_transform, METH_VARARGS, "" },
	{ "instance_transform", py_cr_instance_transform, METH_VARARGS, "" },
	{ "instance_bind_material_set", py_cr_instance_bind_material_set, METH_VARARGS, "" },
	{ "scene_set_background", py_cr_scene_set_background, METH_VARARGS, "" },
	{ "start_render_worker", py_cr_start_render_worker, METH_VARARGS, "" },
	{ "send_shutdown_to_workers", py_cr_send_shutdown_to_workers, METH_VARARGS, "" },
	{ "load_json", py_cr_load_json, METH_VARARGS, "" },
	{ "log_level_set", py_log_level_set, METH_VARARGS, "" },
	{ "log_level_get", py_log_level_get, METH_NOARGS, "" },
	{ "debug_dump_state", py_debug_dump_state, METH_VARARGS, "" },
	{ NULL, NULL, 0, NULL }
};

static struct PyModuleDef cray_wrap = {
	PyModuleDef_HEAD_INIT,
	"cray",
	NULL,
	-1,
	cray_methods
};

PyMODINIT_FUNC PyInit_cray_wrap(void) {
	PyObject *m = NULL;

	for (int i = 0; all_types[i].py_object; ++i) {
		if (PyType_Ready(all_types[i].py_object) < 0)
			return NULL;
	}

	m = PyModule_Create(&cray_wrap);
	if (!m) return NULL;

	for (int i = 0; all_types[i].py_object; ++i) {
		Py_INCREF(all_types[i].py_object);
		if (PyModule_AddObject(m, all_types[i].py_name, (PyObject *)all_types[i].py_object) < 0) {
			Py_DECREF(all_types[i].py_object);
			Py_DECREF(m);
			return NULL;
		}
	}
	return m;
}
