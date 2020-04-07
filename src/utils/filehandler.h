//
//  filehandler.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

/// Returns a string containing `bytes` converted into a more human readable format.
/// @param bytes How many bytes you have
/// @return A human readable file size string.
char *humanFileSize(unsigned long bytes);

//Prints the file size of a given file to the console in a user-readable format
void printFileSize(char *fileName);

char *loadFile(char *inputFileName, size_t *bytes);

/// Returns true if the file at the given path exists and is readable.
/// @param path Path to check
bool isValidFile(char *path);

/**
 Extract the filename from a given file path

 @param input File path to be processed
 @return Filename string, including file type extension
 */
char *getFileName(char *input);

/// Extract the path from a given full path, excluding the filename
/// @param input Full path
char *getFilePath(char *input);

/// Await for input on stdin for up to 2 seconds. If nothing shows up, return NULL
/// @param bytes Bytes read, if successful.
char *readStdin(size_t *bytes);

//FIXME: Move these to a better place
/// Check if two strings are equal
/// @param s1 Left string
/// @param s2 Right string
bool stringEquals(const char *s1, const char *s2);

/// Check of string contains another string
/// @param haystack String to be searched
/// @param needle String to search for
bool stringContains(const char *haystack, const char *needle);

/// Copy strings
/// @param source Source string
/// @param destination Destination string
void copyString(const char *source, char **destination);

/// Concatenate given strings
/// @param str1 Original string
/// @param str2 Concatenated string
char *concatString(const char *str1, const char *str2);

/// Check the size of a given file in bytes.
/// @param fileName File to check
size_t getFileSize(char *fileName);
