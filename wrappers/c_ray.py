import ctypes as ct
from contextlib import contextmanager
from enum import IntEnum

c_ray = ct.CDLL("lib/libc-ray.so")

c_ray.cr_get_version.restype = ct.c_char_p
def cr_get_version():
    return c_ray.cr_get_version()

c_ray.cr_get_git_hash.restype = ct.c_char_p
def cr_get_git_hash():
    return c_ray.cr_get_git_hash()

class cr_renderer(ct.Structure):
    pass

c_ray.cr_new_renderer.restype = ct.POINTER(cr_renderer)
c_ray.cr_destroy_renderer.argtypes = [ct.POINTER(cr_renderer)]

class cr_rparam(IntEnum):
	threads = 0
	samples = 1
	bounces = 2
	tile_width = 3
	tile_height = 4
	tile_order = 5
	output_path = 6
	asset_path = 7
	output_name = 8
	output_filetype = 9
	output_num = 10
	override_width = 11
	override_height = 12
	should_save = 13
	override_cam = 14
	node_list = 15
	is_iterative = 16

c_ray.cr_renderer_set_num_pref.restype = ct.c_bool
c_ray.cr_renderer_set_num_pref.argtypes = [ct.POINTER(cr_renderer), ct.c_int, ct.c_uint64]

c_ray.cr_renderer_get_num_pref.restype = ct.c_uint64
c_ray.cr_renderer_get_num_pref.argtypes = [ct.POINTER(cr_renderer), ct.c_int]

class Renderer:
    def __init__(self):
        self.obj_ptr = c_ray.cr_new_renderer()

    def close(self):
        c_ray.cr_destroy_renderer(self.obj_ptr)

    def get_threads(self):
        return c_ray.cr_renderer_get_num_pref(self.obj_ptr, cr_rparam.threads)
    def set_threads(self, value):
        c_ray.cr_renderer_set_num_pref(self.obj_ptr, cr_rparam.threads, value)

    threads = property(get_threads, set_threads, None, "Local thread count, defaults to nproc + 2")

    @classmethod
    def from_param(cls, param):
        if not isinstance(param, cls):
            raise TypeError("Expected an instance of Renderer")
        return param.obj_ptr

    @contextmanager
    def __call__(self):
        yield self
        self.close()
    def __enter__(self):
        return self
    def __exit__(self, exc_type, exc_value, traceback):
        self.close()

if __name__ == "__main__":
    print("Hello, c-ray!");
    print("libc-ray version {} ({})".format(cr_get_version().decode(), cr_get_git_hash().decode()))
    with Renderer() as r:
        print("Initial value {}".format(r.threads))
        r.threads = 42
        print("Got value {}".format(r.threads))
        r.threads = 69
        print("Got value {}".format(r.threads))
        r.threads = 1234
        print("Got value {}".format(r.threads))
