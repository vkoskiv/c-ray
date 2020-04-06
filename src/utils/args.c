//
//  args.c
//  C-ray
//
//  Created by Valtteri on 6.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include <stdbool.h>
#include <stdint.h>
#include "args.h"
#include "hashtable.h"
#include <string.h>
#include "logging.h"
#include "filehandler.h"
#include "assert.h"

static struct hashtable *options;

void parseOptions(int argc, char **argv) {
	options = newTable();
	static bool inputFileSet = false;
	//Always omit the first argument.
	for (int i = 1; i < argc; ++i) {
		if (isValidFile(argv[i]) && !inputFileSet) {
			setString(options, "inputFile", argv[i], (int)strlen(argv[i]));
			inputFileSet = true;
		} else if (strncmp(argv[i], "-", 1) == 0) {
			setTag(options, ++argv[i]);
		} else {
		}
	}
	logr(debug, "Verbose mode enabled\n");
}

bool isSet(char *key) {
	return exists(options, key);
}

char *pathArg() {
	ASSERT(exists(options, "inputFile"));
	return getString(options, "inputFile");
}
