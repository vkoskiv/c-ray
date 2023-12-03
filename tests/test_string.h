//
//  test_string.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 07/09/2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../src/common/string.h"

bool string_stringEquals(void) {
	
	test_assert(!stringEquals("TestStringOne", "TestStringTwo"));
	test_assert(stringEquals("12345", "12345"));
	
	test_assert(!stringEquals("test-test-test", "test-test"));
	test_assert(!stringEquals("test-test", "test-test-test"));
	
	test_assert(stringEquals("", ""));
	test_assert(stringEquals(" ", " "));
	
	return true;
}

bool string_stringContains(void) {
	
	test_assert(stringContains("This is a really long string containing a bunch of text.", "long"));
	test_assert(stringContains("This is a really long string containing a bunch of text.", " "));
	
	test_assert(stringContains("Test string", ""));
	
	return true;
}

bool string_copyString(void) {
	
	char *staticString = "Statically allocated string here";
	char *dynamicString = stringCopy(staticString);
	
	test_assert(dynamicString);
	test_assert(strlen(dynamicString) == strlen(staticString));
	test_assert(stringEquals(staticString, dynamicString));
	
	char *secondString = stringCopy(dynamicString);
	
	test_assert(secondString);
	test_assert(strlen(secondString) == strlen(dynamicString));
	test_assert(stringEquals(dynamicString, secondString));
	
	free(dynamicString);
	free(secondString);
	
	return true;
}

bool string_concatString(void) {
	
	char *hello = "Hello, ";
	char *world = "world!";
	
	char *concatenated = stringConcat(hello, world);
	test_assert(concatenated);
	
	test_assert(strlen(hello) + strlen(world) == strlen(concatenated));
	test_assert(stringEquals(concatenated, "Hello, world!"));
	
	free(concatenated);
	
	return true;
}

bool string_lowerCase(void) {
	
	char *spongetext = "ThiS is A STRING cONtaINing REALLy CoNfUSing CasinG.";
	
	char *lowercase = stringToLower(spongetext);
	test_assert(lowercase);
	
	test_assert(stringEquals(lowercase, "this is a string containing really confusing casing."));
	
	free(lowercase);
	
	return true;
}

bool string_startsWith(void) {
	
	char *string = "abcdefghijklmn";
	
	test_assert(stringStartsWith("a", string));
	test_assert(stringStartsWith("ab", string));
	test_assert(stringStartsWith("abc", string));
	test_assert(stringStartsWith("abcd", string));
	test_assert(stringStartsWith("abcde", string));
	test_assert(stringStartsWith("abcdef", string));
	test_assert(stringStartsWith("abcdefg", string));
	test_assert(stringStartsWith("abcdefgh", string));
	test_assert(stringStartsWith("abcdefghi", string));
	test_assert(stringStartsWith("abcdefghij", string));
	test_assert(stringStartsWith("abcdefghijk", string));
	test_assert(stringStartsWith("abcdefghijkl", string));
	test_assert(stringStartsWith("abcdefghijklm", string));
	test_assert(stringStartsWith("abcdefghijklmn", string));
	
	test_assert(!stringStartsWith("bcde", string));
	test_assert(!stringStartsWith("acde", string));
	
	return true;
}

bool string_endsWith(void) {

	char *string = "abcdefghijklmn";

	test_assert(stringEndsWith("n", string));
	test_assert(stringEndsWith("mn", string));
	test_assert(stringEndsWith("lmn", string));
	test_assert(stringEndsWith("klmn", string));
	test_assert(stringEndsWith("jklmn", string));
	test_assert(stringEndsWith("ijklmn", string));
	test_assert(stringEndsWith("hijklmn", string));
	test_assert(stringEndsWith("ghijklmn", string));
	test_assert(stringEndsWith("fghijklmn", string));
	test_assert(stringEndsWith("efghijklmn", string));
	test_assert(stringEndsWith("defghijklmn", string));
	test_assert(stringEndsWith("cdefghijklmn", string));
	test_assert(stringEndsWith("bcdefghijklmn", string));
	test_assert(stringEndsWith("abcdefghijklmn", string));

	test_assert(!stringEndsWith("jklm", string));
	test_assert(!stringEndsWith("jkle", string));

	return true;
}
