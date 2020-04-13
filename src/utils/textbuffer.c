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

textBuffer *newTextBuffer(char *contents) {
	char *buf = contents;
	if (!buf) return NULL;
	textBuffer *new = calloc(1, sizeof(textBuffer));
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

char *goToLine(textBuffer *file, size_t line) {
	if (line < file->amountOf.lines) {
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

void fillLineBuffer(lineBuffer *line, char *contents, char *delimiters) {
	char *buf = contents;
	if (!buf) return;
	if (line->buf) free(line->buf);
	copyString(buf, &line->buf);
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
