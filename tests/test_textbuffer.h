//
//  test_textbuffer.h
//  C-ray
//
//  Created by Valtteri on 24.6.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../src/utils/textbuffer.h"
#include "../src/utils/string.h"

//TODO: Tests for all of these.
bool textbuffer_textview(void) {
	bool pass = true;
	char *string = NULL;
	string = copyString("This is a\nMultiline\nstring!\n");
	
	textBuffer *original = newTextBuffer(string);
	//dumpBuffer(original);
	
	test_assert(original->amountOf.lines == 4);
	
	textBuffer *view = newTextView(original, 0, 1);
	test_assert(stringEquals(currentLine(view), "This is a"));
	freeTextBuffer(view);
	test_assert(view == NULL);
	
	view = newTextView(original, 1, 1);
	test_assert(stringEquals(currentLine(view), "Multiline"));
	freeTextBuffer(view);
	test_assert(view == NULL);
	
	view = newTextView(original, 2, 1);
	test_assert(stringEquals(currentLine(view), "string!"));
	freeTextBuffer(view);
	test_assert(view == NULL);
	
	freeTextBuffer(original);
	test_assert(view == NULL);
	free(string);
	
	return pass;
}

bool textbuffer_tokenizer(void) {
	bool pass = true;
	char *rawText = copyString("This is a\nMultiline\nstring!\n");
	textBuffer *file = newTextBuffer(rawText);
	
	char *currentLine = firstLine(file);
	lineBuffer line = {0};
	
	fillLineBuffer(&line, currentLine, " ");
	char *currentToken = firstToken(&line);
	test_assert(stringEquals(currentToken, "This"));
	currentToken = nextToken(&line);
	test_assert(stringEquals(currentToken, "is"));
	currentToken = nextToken(&line);
	test_assert(stringEquals(currentToken, "a"));
	currentToken = firstToken(&line);
	test_assert(stringEquals(currentToken, "This"));
	
	fillLineBuffer(&line, nextLine(file), " ");
	currentToken = firstToken(&line);
	test_assert(stringEquals(currentToken, "Multiline"));
	currentToken = lastToken(&line);
	test_assert(stringEquals(currentToken, "string!"));
	
	freeLineBuffer(&line);
	freeTextBuffer(file);
	free(rawText);
	
	return pass;
}
