//
//  textbuffer.c
//  C-ray
//
//  Created by Valtteri on 12.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include <stddef.h>
#include "textbuffer.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "filehandler.h"
#include "logging.h"
#include "string.h"
#include <stdio.h>
#include "assert.h"

static size_t strlen_newline(const char *str) {
	size_t len = 0;
	while (*(str++) != '\n')
		++len;
	return len;
}

textBuffer *newTextView(textBuffer *original, const size_t start, const size_t lines) {
	ASSERT(original);
	ASSERT(lines > 0);
	ASSERT(start + lines <= original->amountOf.lines);
	
	char *head = goToLine(original, start);
	size_t start_offset = original->currentByteOffset;
	head = goToLine(original, start + (lines - 1));
	size_t len = strlen(head) + 1;
	size_t end_offset = original->currentByteOffset + len;
	head = goToLine(original, start);
	size_t bytes = end_offset - start_offset;

	char *buf = malloc(bytes * sizeof(*buf));
	memcpy(buf, head, bytes);
	
	textBuffer *new = calloc(1, sizeof(*new));
	new->buf = buf;
	new->buflen = bytes;
	new->amountOf.lines = lines;
	new->current.line = 0;
	logr(debug, "Created new textView handle of size %zu, that has %zu lines\n", new->buflen, new->amountOf.lines);
	return new;
}

textBuffer *newTextBuffer(char *contents) {
	char *buf = contents;
	if (!buf) return NULL;
	textBuffer *new = calloc(1, sizeof(*new));
	new->buf = buf;
	new->buflen = strlen(contents);
	
	//Figure out the line count and convert newlines
	size_t lines = 0;
	for (size_t i = 0; i < new->buflen; ++i) {
		if (buf[i] == '\n') {
			buf[i] = '\0';
			lines++;
		}
	}
	
	new->amountOf.lines = lines;
	new->current.line = 0;
	logr(debug, "Created new textBuffer handle of size %zu, that has %zu lines\n", new->buflen, new->amountOf.lines);
	return new;
}

void dumpBuffer(textBuffer *buffer) {
	logr(debug, "Dumping buffer:\n\n\n");
	char *head = firstLine(buffer);
	while (head) {
		printf("%s\n", head);
		head = nextLine(buffer);
	}
	printf("\n\n");
}

char *goToLine(textBuffer *file, size_t line) {
	if (line < file->amountOf.lines) {
		file->currentByteOffset = 0;
		char *head = file->buf;
		for (size_t i = 0; i < line; ++i) {
			size_t offset = strlen(head) + 1;
			head += strlen(head) + 1;
			file->currentByteOffset += offset;
		}
		file->current.line = line;
		return head;
	} else {
		return NULL;
	}
}

char *nextLine(textBuffer *file) {
	char *head = file->buf + file->currentByteOffset;
	if (file->current.line + 1 < file->amountOf.lines) {
		size_t offset = strlen(head) + 1;
		head += offset;
		file->current.line++;
		file->currentByteOffset += offset;
		return head;
	} else {
		return NULL;
	}
}

char *firstLine(textBuffer *file) {
	char *head = file->buf;
	file->current.line = 0;
	file->currentByteOffset = 0;
	return head;
}

char *currentLine(textBuffer *file) {
	return file->buf;
}

char *lastLine(textBuffer *file) {
	char *head = goToLine(file, file->amountOf.lines - 1);
	file->current.line = file->amountOf.lines - 1;
	return head;
}

void freeTextBuffer(textBuffer *file) {
	if (file) {
		if (file->buf) free(file->buf);
		free(file);
	}
}

void fillLineBuffer(lineBuffer *line, const char *contents, char *delimiters) {
	const char *buf = contents;
	if (!buf) return;
	if (line->buf) free(line->buf);
	line->buf = copyString(buf);
	line->buflen = strlen(line->buf);
	
	size_t tokens = 0;
	for (size_t i = 0; i < line->buflen + 1; ++i) {
		for (size_t d = 0; d < strlen(delimiters); ++d) {
			if (line->buf[i] == delimiters[d] || line->buf[i] == '\0') {
				line->buf[i] = '\0';
				tokens++;
			}
		}
	}
	
	line->amountOf.tokens = tokens;
}

char *goToToken(lineBuffer *line, size_t token) {
	return goToLine(line, token);
}

char *nextToken(lineBuffer *line) {
	return nextLine(line);
}

char *firstToken(lineBuffer *line) {
	return firstLine(line);
}

char *lastToken(lineBuffer *line) {
	return lastLine(line);
}

void freeLineBuffer(lineBuffer *line) {
	if (line) {
		if (line->buf) free(line->buf);
	}
}

//TODO: Tests for all of these.
int testTextView(void) {
	logr(debug, "Testing textView\n");
	char *string = NULL;
	string = copyString("This is a\nMultiline\nstring!\n");
	logr(debug, "\n%s\n", string);
	
	textBuffer *original = newTextBuffer(string);
	dumpBuffer(original);
	
	textBuffer *view = newTextView(original, 0, 1);
	ASSERT(stringEquals(currentLine(view), "This is a"));
	freeTextBuffer(view);
	view = newTextView(original, 1, 1);
	ASSERT(stringEquals(currentLine(view), "Multiline"));
	freeTextBuffer(view);
	view = newTextView(original, 2, 1);
	ASSERT(stringEquals(currentLine(view), "string!"));
	freeTextBuffer(view);
	return 0;
}

static void testTokenizer(char *filePath) {
	char *rawText = loadFile(filePath, NULL);
	textBuffer *file = newTextBuffer(rawText);
	
	char *currentLine = firstLine(file);
	lineBuffer line = {0};
	while (currentLine) {
		fillLineBuffer(&line, currentLine, " ");
		char *currentToken = firstToken(&line);
		printf("Line %zu: ", file->current.line);
		while (currentToken) {
			printf("%s ", currentToken);
			currentToken = nextToken(&line);
		}
		printf("\n");
		currentLine = nextLine(file);
	}
	freeLineBuffer(&line);
	freeTextBuffer(file);
}
