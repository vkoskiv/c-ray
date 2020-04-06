//
//  filehandler.h
//  C-ray
//
//  Created by Valtteri Koskivuori on 28/02/2015.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct texture;
struct renderInfo;


/// Returns a string containing `bytes` converted into a more human readable format.
/// @param bytes How many bytes you have
char *humanFileSize(unsigned long bytes);

//Prints the file size of a given file to the console in a user-readable format
void printFileSize(char *fileName);

//Writes image data to file
void writeImage(struct texture *image, struct renderInfo imginfo);

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

char *readStdin(size_t *bytes);

//FIXME: Move this to a better place
bool stringEquals(const char *s1, const char *s2);
//FIXME: Move this to a better place
bool stringContains(const char *haystack, const char *needle);

void copyString(const char *source, char **destination);

char *concatString(const char *str1, const char *str2);

size_t getFileSize(char *fileName);
