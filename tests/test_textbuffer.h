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
	pass = original->amountOf.lines == 3;
	
	textBuffer *view = newTextView(original, 0, 1);
	pass = stringEquals(currentLine(view), "This is a");
	freeTextBuffer(view);
	pass = view == NULL;
	
	view = newTextView(original, 1, 1);
	pass = stringEquals(currentLine(view), "Multiline");
	freeTextBuffer(view);
	pass = view == NULL;
	
	view = newTextView(original, 2, 1);
	pass = stringEquals(currentLine(view), "string!");
	freeTextBuffer(view);
	pass = view == NULL;
	
	freeTextBuffer(original);
	pass = original == NULL;
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
	pass = stringEquals(currentToken, "This");
	currentToken = nextToken(&line);
	pass = stringEquals(currentToken, "is");
	currentToken = nextToken(&line);
	pass = stringEquals(currentToken, "a");
	currentToken = firstToken(&line);
	pass = stringEquals(currentToken, "This");
	
	fillLineBuffer(&line, nextLine(file), " ");
	currentToken = firstToken(&line);
	pass = stringEquals(currentToken, "Multiline");
	currentToken = lastToken(&line);
	pass = stringEquals(currentToken, "string!");
	
	freeLineBuffer(&line);
	freeTextBuffer(file);
	free(rawText);
	
	return pass;
}
