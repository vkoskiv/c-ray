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
void printFileSize(const char *fileName);

char *loadFile(const char *fileName, size_t *bytes);

// This is a more robust file writing function, that will seek alternate directories
// if the specified one wasn't writeable.
void writeFile(const unsigned char *buf, size_t bufsize, const char *filePath);

/// Returns true if the file at the given path exists and is readable.
/// @param path Path to check
bool isValidFile(char *path);

/**
 Extract the filename from a given file path

 @param input File path to be processed
 @return Filename string, including file type extension
 */
char *getFileName(const char *input);

/// Extract the path from a given full path, excluding the filename
/// @param input Full path
char *getFilePath(const char *input);

/// Await for input on stdin for up to 2 seconds. If nothing shows up, return NULL
/// @param bytes Bytes read, if successful.
char *readStdin(size_t *bytes);

/// Check the size of a given file in bytes.
/// @param fileName File to check
size_t getFileSize(const char *fileName);
