#ifndef _DYN_H
#define _DYN_H

void *dyn_load(const char *filename);
void *dyn_sym(void *handle, const char *name);
char *dyn_error(void);
int dyn_close(void *handle);

#endif
