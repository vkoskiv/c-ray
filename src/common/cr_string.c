//
//  cr_string.c
//  c-ray
//
//  Created by Valtteri on 12.4.2020.
//  Copyright © 2020-2025 Valtteri Koskivuori. All rights reserved.
//

#include <stdbool.h>
#include "cr_string.h"
#include <string.h>
#include <stdlib.h>
#include "cr_assert.h"

bool stringEquals(const char *s1, const char *s2) {
	if (!s1 || !s2) return false;
	return strcmp(s1, s2) == 0;
}

bool stringContains(const char *haystack, const char *needle) {
	if (!haystack || !needle) return false;
	return strstr(haystack, needle) ? true : false;
}

bool stringStartsWith(const char *prefix, const char *string) {
	if (!prefix || !string) return false;
	size_t prefix_len = strlen(prefix);
	size_t string_len = strlen(string);
	return string_len < prefix_len ? false : memcmp(prefix, string, prefix_len) == 0;
}

bool stringEndsWith(const char *postfix, const char *string) {
	if (!postfix || !string) return false;
	size_t postfix_len = strlen(postfix);
	size_t string_len = strlen(string);
	return string_len < postfix_len ? false : memcmp(postfix, string + string_len - postfix_len, postfix_len) == 0;
}

//Copies source over to the destination pointer.
char *stringCopy(const char *source) {
	if (!source) return NULL;
	char *copy = malloc(strlen(source) + 1);
	strcpy(copy, source);
	return copy;
}

char *stringConcat(const char *str1, const char *str2) {
	ASSERT(str1); ASSERT(str2);
	char *new = malloc(strlen(str1) + strlen(str2) + 1);
	strcpy(new, str1);
	strcat(new, str2);
	return new;
}

#ifdef WINDOWS
// Windows uses \ for path separation, so we have to flip those, if present.
static void windowsFlipSlashes(char *path) {
	size_t len = strlen(path);
	for (size_t i = 0; i < len; ++i) {
		if (path[i] == '/') path[i] = '\\';
	}
}

// On windows, passing a path to fopen() with CRLF causes it to fail with 'invalid argument'. Fun!
static void windowsStripCRLF(char *path) {
	size_t length = strlen(path);
	for (size_t i = 0; i < length; ++i) {
		if (path[i] == '\r') path[i] = '\0';
	}
}
#endif

void windowsFixPath(char *path) {
#ifdef WINDOWS
	windowsFlipSlashes(path);
	windowsStripCRLF(path);
#else
	(void)path;
#endif
}

char *stringToLower(const char *orig) {
	if (!orig) return NULL;
	char *str = stringCopy(orig);
	size_t len = strlen(str);
	for (size_t i = 0; i < len; ++i) {
		if (str[i] > 64 && str[i] < 91) { // A-Z ASCII
			str[i] += 32; // Offset to lowercase
		}
	}
	return str;
}
