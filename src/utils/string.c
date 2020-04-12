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
	if (strcmp(s1, s2) == 0) {
		return true;
	} else {
		return false;
	}
}

bool stringContains(const char *haystack, const char *needle) {
	if (strstr(haystack, needle) == NULL) {
		return false;
	} else {
		return true;
	}
}

//Copies source over to the destination pointer.
void copyString(const char *source, char **destination) {
	*destination = malloc(strlen(source) + 1);
	strcpy(*destination, source);
}

char *concatString(const char *str1, const char *str2) {
	ASSERT(str1); ASSERT(str2);
	char *new = malloc(strlen(str1) + strlen(str2) + 1);
	strcpy(new, str1);
	strcat(new, str2);
	return new;
}
