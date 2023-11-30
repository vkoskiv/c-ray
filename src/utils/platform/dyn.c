#if defined(WINDOWS)
	#include <windows.h>
	#include <libloaderapi.h>
#elif defined(__COSMOPOLITAN__)
	#define _COSMO_SOURCE
	#include "libc/dlopen/dlfcn.h"
#else
	#include <dlfcn.h>
#endif

#include "dyn.h"

void *dyn_load(const char *filename) {
#if defined(WINDOWS)
	return (void *)LoadLibraryA(filename);
#elif defined(__COSMOPOLITAN__)
	return cosmo_dlopen(filename, RTLD_LAZY);
#else
	return dlopen(filename, RTLD_LAZY);
#endif
}

void *dyn_sym(void *handle, const char *name) {
#if defined(WINDOWS)
	return (void *)GetProcAddress((HMODULE)handle, name);
#elif defined(__COSMOPOLITAN__)
	return cosmo_dlsym(handle, name);
#else
	return dlsym(handle, name);
#endif
}

char *dyn_error(void) {
#if defined(WINDOWS)
	return NULL;
#elif defined(__COSMOPOLITAN__)
	return cosmo_dlerror();
#else
	return dlerror();
#endif
}

int dyn_close(void *handle) {
#if defined(WINDOWS)
	return (int)FreeLibrary((HMODULE)handle);
#elif defined(__COSMOPOLITAN__)
	return cosmo_dlclose(handle);
#else
	return dlclose(handle);
#endif
}
