import ctypes as ct

lib = ct.CDLL("lib/libc-ray.so")
lib.cr_get_version.restype = ct.c_char_p
lib.cr_get_git_hash.restype = ct.c_char_p

class version:
	def semantic():
		return lib.cr_get_version().decode()
	def hash():
		return lib.cr_get_git_hash().decode()
