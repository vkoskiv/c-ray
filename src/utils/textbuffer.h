//
//  textbuffer.h
//  C-ray
//
//  Created by Valtteri on 12.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stddef.h>

struct _textBuffer {
	char *buf;
	size_t buflen;
	union {
		size_t lines;
		size_t tokens;
	} amountOf;
	
	union {
		size_t line;
		size_t token;
	} current;
	size_t currentByteOffset;
	char *delimiters;
	size_t delimCount;
};

typedef struct _textBuffer textBuffer;
typedef struct _textBuffer lineBuffer;

textBuffer *newTextView(textBuffer *original, const size_t start, const size_t lines);

textBuffer *newTextBuffer(char *contents);

void dumpBuffer(textBuffer *buffer);

char *goToLine(textBuffer *file, size_t line);

char *nextLine(textBuffer *file);

char *firstLine(textBuffer *file);

char *currentLine(textBuffer *file);

char *lastLine(textBuffer *file);

void freeTextBuffer(textBuffer *file);


void fillLineBuffer(lineBuffer *buffer, char *contents, char *delimiters);

char *goToToken(lineBuffer *line, size_t token);

char *nextToken(lineBuffer *line);

char *firstToken(lineBuffer *line);

char *lastToken(lineBuffer *line);

void freeLineBuffer(lineBuffer *line);

int testTextView(void);
