#ifdef WINDOWS
#include <windows.h>
#include <libloaderapi.h>
#else
#include <dlfcn.h>
#endif
#include "dyn.h"

void *dyn_load(const char *filename) {
#ifdef WINDOWS
	return (void *)LoadLibraryA(filename);
#else
	return dlopen(filename, RTLD_LAZY);
#endif
}

void *dyn_sym(void *handle, const char *name) {
#ifdef WINDOWS
	return (void *)GetProcAddress((HMODULE)handle, name);
#else
	return dlsym(handle, name);
#endif
}

int dyn_close(void *handle) {
#ifdef WINDOWS
	return (int)FreeLibrary((HMODULE)handle);
#else
	return dlclose(handle);
#endif
}