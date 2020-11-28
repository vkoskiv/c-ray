//
//  textbuffer.h
//  C-ray
//
//  Created by Valtteri on 12.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stddef.h>

#define LINEBUFFER_MAXSIZE 2048

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
};

typedef struct _textBuffer textBuffer;
typedef struct _textBuffer lineBuffer;

textBuffer *newTextBuffer(const char *contents);

lineBuffer *newLineBuffer(void);

// A subset of a textBuffer
textBuffer *newTextView(textBuffer *original, const size_t start, const size_t lines);

void dumpBuffer(textBuffer *buffer);

char *goToLine(textBuffer *file, size_t line);

char *peekLine(textBuffer *file, size_t line);

char *nextLine(textBuffer *file);

char *previousLine(textBuffer *file);

char *peekNextLine(textBuffer *file);

char *firstLine(textBuffer *file);

char *currentLine(textBuffer *file);

char *lastLine(textBuffer *file);

void freeTextBuffer(textBuffer *file);


void fillLineBuffer(lineBuffer *buffer, const char *contents, char delimiter);

char *goToToken(lineBuffer *line, size_t token);

char *peekToken(lineBuffer *line, size_t token);

char *nextToken(lineBuffer *line);

char *previousToken(lineBuffer *line);

char *peekNextToken(lineBuffer *line);

char *firstToken(lineBuffer *line);

char *currentToken(lineBuffer *line);

char *lastToken(lineBuffer *line);

void destroyLineBuffer(lineBuffer *line);
