#ifndef ASPRINTF_H
#define ASPRINTF_H

#if defined(__GNUC__) && ! defined(_GNU_SOURCE)
#define _GNU_SOURCE /* needed for (v)asprintf, affects '#include <stdio.h>' */
#endif
#include <stdio.h>  /* needed for vsnprintf    */
#include <stdlib.h> /* needed for malloc, free */
#include <stdarg.h> /* needed for va_*         */

/*
 * vscprintf:
 * MSVC implements this as _vscprintf, thus we just 'symlink' it here
 * GNU-C-compatible compilers do not implement this, thus we implement it here
 */
#ifdef _MSC_VER
#define vscprintf _vscprintf
#endif

#ifdef __GNUC__
int vscprintf(const char *format, va_list ap) {
	va_list ap_copy;
	va_copy(ap_copy, ap);
	int retval = vsnprintf(NULL, 0, format, ap_copy);
	va_end(ap_copy);
	return retval;
}
#endif

/*
 * asprintf, vasprintf:
 * MSVC does not implement these, thus we implement them here
 * GNU-C-compatible compilers implement these with the same names, thus we
 * don't have to do anything
 */
#ifdef _MSC_VER
int cray_vasprintf(char **strp, const char *format, va_list ap) {
	int len = vscprintf(format, ap);
	if (len == -1)
		return -1;
	char *str = (char*)malloc((size_t) len + 1);
	if (!str)
		return -1;
	int retval = vsnprintf(str, len + 1, format, ap);
	if (retval == -1) {
		free(str);
		return -1;
	}
	*strp = str;
	return retval;
}

int asprintf(char **strp, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	int retval = cray_vasprintf(strp, format, ap);
	va_end(ap);
	return retval;
}
#endif

#endif // ASPRINTF_H
