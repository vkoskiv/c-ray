//
//  py_types.c
//  libc-ray CPython wrapper
//
//  Created by Valtteri on 9.2.2025
//  Copyright © 2025 Valtteri Koskivuori. All rights reserved.
//

#include "py_types.h"
#include <structmember.h>

static PyMemberDef py_vector_members[] = {
	{ "x", T_FLOAT, offsetof(py_vector, val.x), 0, "x" },
	{ "y", T_FLOAT, offsetof(py_vector, val.y), 0, "y" },
	{ "z", T_FLOAT, offsetof(py_vector, val.z), 0, "z" },
	{ NULL },
};

/*
	WTF. If I mark these functions as static, Python randomly throws bizarre UnicodeDecodeErrors when trying to import the .so:
	Traceback (most recent call last):
	  File "<python-input-0>", line 1, in <module>
	    from bindings.python.lib import c_ray
	  File "/home/vkoskiv/c-ray/bindings/python/lib/c_ray.py", line 5, in <module>
	    from . import cray_wrap as _lib
	UnicodeDecodeError: 'utf-8' codec can't decode byte 0xb8 in position 0: invalid start byte
*/
Py_ssize_t py_vector_length(PyObject *o) {
	(void)o;
	return 3;
}

PyObject *py_vector_subscript(PyObject *o, PyObject *key) {
	py_vector *self = (py_vector *)o;
	if (PyLong_Check(key)) {
		long index = PyLong_AsLong(key);
		if (index < 0 || index > 2) {
			PyErr_SetString(PyExc_IndexError, "cr_vector index out of range");
			return NULL;
		}
		return PyFloat_FromDouble((index == 0) ? self->val.x : (index == 1) ? self->val.y : self->val.z);
	}
	if (PySlice_Check(key)) {
		Py_ssize_t start, stop, step;
		if (PySlice_Unpack(key, &start, &stop, &step) < 0)
			return NULL;
		Py_ssize_t indices[3] = { 0, 1, 2 };
		PyObject *result = PyList_New(0);
		if (!result)
			return NULL;
		for (Py_ssize_t i = start; i < stop && i >= 0 && i <= 2; i += step) {
			PyObject *py_value = PyFloat_FromDouble((indices[i] == 0) ? self->val.x : (indices[i] == 1) ? self->val.y : self->val.z);
			if (!py_value) {
				Py_DECREF(result);
				return NULL;
			}
			PyList_Append(result, py_value);
			Py_DECREF(py_value);
		}
		return result;
	}
	PyErr_SetString(PyExc_TypeError, "cr_vector indices must be integers or slices");
	return NULL;
}

int py_vector_ass_subscript(PyObject *o, PyObject *key, PyObject *value) {
	py_vector *self = (py_vector *)o;
	if (PyLong_Check(key)) {
		long index = PyLong_AsLong(key);
		if (index < 0 || index > 2) {
			PyErr_SetString(PyExc_IndexError, "cr_vector index out of range");
			return -1;
		}
		if (!PyFloat_Check(value) && !PyLong_Check(value)) {
			PyErr_SetString(PyExc_IndexError, "cr_vector values must be float or int");
			return -1;
		}
		double val = PyFloat_AsDouble(value);
		((float *)&self->val)[index] = val;
		return 0;
	}
	
	PyErr_SetString(PyExc_TypeError, "cr_vector indices must be integers");
	return -1;
}

PyMappingMethods py_vector_mapping_methods = {
	.mp_length = py_vector_length,
	.mp_subscript = py_vector_subscript,
	.mp_ass_subscript = py_vector_ass_subscript,
};

PyTypeObject type_py_vector = {
	.ob_base = PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "c_ray.cr_vector",
	.tp_doc = PyDoc_STR("c-ray native 3D vector type"),
	.tp_basicsize = sizeof(py_vector),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_members = py_vector_members,
	.tp_as_mapping = &py_vector_mapping_methods,
};

PyMemberDef py_coord_members[] = {
	{ "u", T_FLOAT, offsetof(py_coord, val.u), 0, "u" },
	{ "v", T_FLOAT, offsetof(py_coord, val.v), 0, "v" },
	{ NULL },
};

Py_ssize_t py_coord_length(PyObject *o) {
	(void)o;
	return 2;
}

PyObject *py_coord_subscript(PyObject *o, PyObject *key) {
	py_coord *self = (py_coord *)o;
	if (PyLong_Check(key)) {
		long index = PyLong_AsLong(key);
		if (index < 0 || index > 1) {
			PyErr_SetString(PyExc_IndexError, "cr_coord index out of range");
			return NULL;
		}
		return PyFloat_FromDouble((index == 0) ? self->val.u : self->val.v);
	}
	if (PySlice_Check(key)) {
		Py_ssize_t start, stop, step;
		if (PySlice_Unpack(key, &start, &stop, &step) < 0)
			return NULL;
		Py_ssize_t indices[3] = { 0, 1 };
		PyObject *result = PyList_New(0);
		if (!result)
			return NULL;
		for (Py_ssize_t i = start; i < stop && i >= 0 && i <= 1; i += step) {
			PyObject *py_value = PyFloat_FromDouble((indices[i] == 0) ? self->val.u : self->val.v);
			if (!py_value) {
				Py_DECREF(result);
				return NULL;
			}
			PyList_Append(result, py_value);
			Py_DECREF(py_value);
		}
		return result;
	}
	PyErr_SetString(PyExc_TypeError, "cr_coord indices must be integers or slices");
	return NULL;
}

int py_coord_ass_subscript(PyObject *o, PyObject *key, PyObject *value) {
	py_coord *self = (py_coord *)o;
	if (PyLong_Check(key)) {
		long index = PyLong_AsLong(key);
		if (index < 0 || index > 1) {
			PyErr_SetString(PyExc_IndexError, "cr_coord index out of range");
			return -1;
		}
		if (!PyFloat_Check(value) && !PyLong_Check(value)) {
			PyErr_SetString(PyExc_IndexError, "cr_coord values must be float or int");
			return -1;
		}
		double val = PyFloat_AsDouble(value);
		((float *)&self->val)[index] = val;
		return 0;
	}
	
	PyErr_SetString(PyExc_TypeError, "cr_coord indices must be integers");
	return -1;
}

PyMappingMethods py_coord_mapping_methods = {
	.mp_length = py_coord_length,
	.mp_subscript = py_coord_subscript,
	.mp_ass_subscript = py_coord_ass_subscript,
};

PyTypeObject type_py_coord = {
	.ob_base = PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "c_ray.cr_coord",
	.tp_doc = PyDoc_STR("c-ray native 2D vector type"),
	.tp_basicsize = sizeof(py_coord),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_members = py_coord_members,
	.tp_as_mapping = &py_coord_mapping_methods,
};

void py_bitmap_dealloc(py_bitmap *self) {
	Py_TYPE(self)->tp_free((PyObject *)self);
}

#define REF_GETTER(type, field, pytype) \
	static PyObject *type##_get_##field(type *self, void *closure) { \
		(void)closure; \
		if (!self->ref) Py_RETURN_NONE; \
		return Py_BuildValue(pytype, self->ref->field); \
	}

#define REF_SETTER(type, field, pytype, ctype) \
	static int type##_set_##field(type *self, PyObject *value, void *closure) { \
		(void)closure; \
		if (!self->ref || !value) return -1; \
		ctype temp; \
		if (!PyArg_Parse(value, pytype, &temp)) return -1; \
		self->ref->field = temp; \
		return 0; \
	}
#define PY_BITMAP_FIELDS \
	X(py_bitmap, colorspace, "i", int) \
	X(py_bitmap, precision, "i", int) \
	X(py_bitmap, stride, "n", size_t) \
	X(py_bitmap, width, "n", size_t) \
	X(py_bitmap, height, "n", size_t) \

#define X(type, field, pytype, ctype) REF_GETTER(type, field, pytype)
PY_BITMAP_FIELDS
#undef X

#define X(type, field, pytype, ctype) REF_SETTER(type, field, pytype, ctype)
PY_BITMAP_FIELDS
#undef X

static PyObject *py_cr_bitmap_get_data(py_bitmap *self, void *closure) {
	(void)closure;
	if (!self->ref) Py_RETURN_NONE;
	if (self->ref->precision == cr_bm_char) {
		return PyBytes_FromStringAndSize((char *)self->ref->data.byte_ptr, self->ref->width * self->ref->height * self->ref->stride);
	} else if (self->ref->precision == cr_bm_float) {
		// Could we apply tp_as_buffer for this? Not sure how that would look from the python side, though.
		return PyMemoryView_FromMemory((char *)self->ref->data.float_ptr, self->ref->width * self->ref->height * self->ref->stride * sizeof(float), PyBUF_READ);
	}
	Py_RETURN_NONE;
}

#define X(type, field, pytype, ctype) {#field, (getter)type##_get_##field, (setter)type##_set_##field, #field, NULL},
static PyGetSetDef py_bitmap_getters_setters[] = {
	PY_BITMAP_FIELDS
	{ "data", (getter)py_cr_bitmap_get_data, NULL, "data", NULL },
	{ 0 },
};
#undef X

PyTypeObject type_py_bitmap = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "c_ray.bitmap",
	.tp_doc = PyDoc_STR(""),
	.tp_basicsize = sizeof(py_bitmap),
	.tp_dealloc = (destructor)py_bitmap_dealloc,
	.tp_itemsize = 0, // Huh?
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_getset = py_bitmap_getters_setters,
	.tp_alloc = PyType_GenericAlloc,
};

PyObject *py_bitmap_wrap(struct cr_bitmap *ref) {
	if (!ref) return NULL;
	py_bitmap *self = (py_bitmap *)type_py_bitmap.tp_alloc(&type_py_bitmap, 0);
	if (!self) return NULL;
	self->ref = ref;
	return (PyObject *)self;
}

static PyMemberDef py_renderer_cb_info_members[] = {
	{ "tiles_count", T_ULONG, offsetof(py_renderer_cb_info, info.tiles_count), 0, "Amount of tiles" },
	{ "active_threads", T_ULONG, offsetof(py_renderer_cb_info, info.active_threads), 0, "Amount of active threads" },
	{ "avg_per_ray", T_DOUBLE, offsetof(py_renderer_cb_info, info.avg_per_ray_us), 0, "Microseconds per ray, on average" },
	{ "samples_per_sec", T_LONG, offsetof(py_renderer_cb_info, info.samples_per_sec), 0, "Samples per second" },
	{ "eta_ms", T_LONG, offsetof(py_renderer_cb_info, info.eta_ms), 0, "ETA to render finished, in milliseconds" },
	{ "finished_passes", T_ULONG, offsetof(py_renderer_cb_info, info.finished_passes), 0, "Passes finished in interactive mode" },
	{ "completion", T_DOUBLE, offsetof(py_renderer_cb_info, info.completion), 0, "Render completion" },
	{ "paused", T_INT, offsetof(py_renderer_cb_info, info.paused), 0, "Boolean, render paused" },
	{ NULL },
};

PyTypeObject type_py_renderer_cb_info = {
	.ob_base = PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "c_ray.callback_info",
	.tp_doc = PyDoc_STR(""),
	.tp_basicsize = sizeof(py_renderer_cb_info),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = PyType_GenericNew,
	.tp_members = py_renderer_cb_info_members,
};

struct cr_python_type all_types[] = {
	{ &type_py_vector, "cr_vector" },
	{ &type_py_coord, "cr_coord" },
	{ &type_py_bitmap, "bitmap" },
	{ &type_py_renderer_cb_info, "callback_info" },
	{ 0 },
};

