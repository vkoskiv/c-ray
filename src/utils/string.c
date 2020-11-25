//
//  string.c
//  C-ray
//
//  Created by Valtteri on 12.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include <stdbool.h>
#include "string.h"
#include <string.h>
#include <stdlib.h>
#include "assert.h"

bool stringEquals(const char *s1, const char *s2) {
	ASSERT(s1); ASSERT(s2);
	return strcmp(s1, s2) == 0;
}

bool stringContains(const char *haystack, const char *needle) {
	ASSERT(haystack); ASSERT(needle);
	return strstr(haystack, needle);
}

bool stringStartsWith(const char *prefix, const char *string) {
	ASSERT(prefix); ASSERT(string);
	size_t prefix_len = strlen(prefix);
	size_t string_len = strlen(string);
	return string_len < prefix_len ? false : memcmp(prefix, string, prefix_len) == 0;
}

//Copies source over to the destination pointer.
char *stringCopy(const char *source) {
	ASSERT(source);
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

char *stringToLower(const char *orig) {
	char *str = stringCopy(orig);
	size_t len = strlen(str);
	for (size_t i = 0; i < len; ++i) {
		if (str[i] > 64 && str[i] < 91) { // A-Z ASCII
			str[i] += 32; // Offset to lowercase
		}
	}
	return str;
}
