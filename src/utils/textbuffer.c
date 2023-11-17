//
//  textbuffer.c
//  C-ray
//
//  Created by Valtteri on 12.4.2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"

#include <stddef.h>
#include "textbuffer.h"

#include <stdlib.h>
#include <string.h>
#include "logging.h"
#include "string.h"
#include <stdio.h>
#include "assert.h"

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
	return new;
}

//TODO: Optional size
textBuffer *newTextBuffer(const char *contents) {
	if (!contents) return NULL;
	textBuffer *new = calloc(1, sizeof(*new));
	new->buf = stringCopy(contents);
	new->buflen = strlen(contents);
	
	//Figure out the line count and convert newlines
	size_t lines = 0;
	for (size_t i = 0; i < new->buflen; ++i) {
		if (new->buf[i] == '\n') {
			new->buf[i] = '\0';
			lines++;
		}
	}
	
	new->amountOf.lines = lines;
	new->current.line = 0;
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
			head += offset;
			file->currentByteOffset += offset;
		}
		file->current.line = line;
		return head;
	} else {
		return NULL;
	}
}

char *peekLine(const textBuffer *file, size_t line) {
	if (line < file->amountOf.lines) {
		char *head = file->buf;
		for (size_t i = 0; i < line; ++i) {
			size_t offset = strlen(head) + 1;
			head += offset;
		}
		return head;
	} else {
		return NULL;
	}
}

char *nextLine(textBuffer *file) {
	char *head = file->buf + file->currentByteOffset;
	if (file->current.line + 1 < file->amountOf.lines) {
		size_t offset = strlen(head) + 1;
		file->current.line++;
		file->currentByteOffset += offset;
		return head + offset;
	} else {
		return NULL;
	}
}

char *previousLine(textBuffer *file) {
	char *head = goToLine(file, file->current.line - 1);
	return head;
}

char *peekNextLine(const textBuffer *file) {
	char *head = file->buf + file->currentByteOffset;
	if (file->current.line + 1 < file->amountOf.lines) {
		size_t offset = strlen(head) + 1;
		return head + offset;
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

char *currentLine(const textBuffer *file) {
	return file->buf + file->currentByteOffset;
}

char *lastLine(textBuffer *file) {
	char *head = goToLine(file, file->amountOf.lines - 1);
	file->current.line = file->amountOf.lines - 1;
	return head;
}

void destroyTextBuffer(textBuffer *file) {
	if (file) {
		if (file->buf) free(file->buf);
		free(file);
	}
}

void fillLineBuffer(lineBuffer *line, const char *contents, char delimiter) {
	if (!contents) return;
	size_t copyLen = min(strlen(contents), LINEBUFFER_MAXSIZE - 1);
	memcpy(line->buf, contents, copyLen);
	line->buf[copyLen] = '\0';
	line->buflen = copyLen;
	line->amountOf.tokens = 0;
	for (size_t i = 0; i < line->buflen + 1; ++i) {
		char cur = line->buf[i];
		char next = i > line->buflen ? 0 : line->buf[i + 1];
		if ((cur == delimiter && next) || cur == '\0') {
			line->buf[i] = '\0';
			line->amountOf.tokens++;
		}
	}
}

char *goToToken(lineBuffer *line, size_t token) {
	return goToLine(line, token);
}

char *peekToken(const lineBuffer *line, size_t token) {
	return peekLine(line, token);
}

char *nextToken(lineBuffer *line) {
	return nextLine(line);
}

char *previousToken(lineBuffer *line) {
	return previousLine(line);
}

char *peekNextToken(const lineBuffer *line) {
	return peekNextLine(line);
}

char *firstToken(lineBuffer *line) {
	return firstLine(line);
}

char *currentToken(const lineBuffer *line) {
	return currentLine(line);
}

char *lastToken(lineBuffer *line) {
	return lastLine(line);
}

void dumpLine(lineBuffer *line) {
	logr(debug, "Dumping line:\n\n\n");
	char *head = firstToken(line);
	while (head) {
		printf("%s ", head);
		head = nextToken(line);
	}
	printf("\n\n");
}
