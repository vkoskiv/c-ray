//
//  string.h
//  C-ray
//
//  Created by Valtteri on 12.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stdbool.h>

/// Check if two strings are equal
/// @param s1 Left string
/// @param s2 Right string
bool stringEquals(const char *s1, const char *s2);

/// Check of string contains another string
/// @param haystack String to be searched
/// @param needle String to search for
bool stringContains(const char *haystack, const char *needle);

bool stringStartsWith(const char *prefix, const char *string);

/// Copy strings
/// @param source String to be copied
/// @return New heap-allocated string
char *stringCopy(const char *source);

/// Concatenate given strings
/// @param str1 Original string
/// @param str2 Concatenated string
char *stringConcat(const char *str1, const char *str2);

char *stringToLower(const char *str);
