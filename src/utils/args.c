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
#include "platform/terminal.h"
#include "platform/capabilities.h"
#include <stdlib.h>
#include "textbuffer.h"

static struct hashtable *options;

void printUsage(const char *progname) {
	logr(info, "Usage: %s [-hjdv] [input_json...]\n", progname);
	logr(info, "	Available options are:\n");
	logr(info, "		[-h]         -> Show this message\n");
	logr(info, "		[-j <n>]     -> Override thread count to n\n");
	logr(info, "		[-d <w>x<h>] -> Override image dimensions to <w>x<h>\n");
	logr(info, "		[-v]         -> Enable verbose mode\n");
	restoreTerminal();
	exit(0);
}

void parseOptions(int argc, char **argv) {
	options = newTable();
	static bool inputFileSet = false;
	//Always omit the first argument.
	for (int i = 1; i < argc; ++i) {
		if (isValidFile(argv[i]) && !inputFileSet) {
			setString(options, "inputFile", argv[i], (int)strlen(argv[i]));
			inputFileSet = true;
		} else if (strncmp(argv[i], "-h", 2) == 0) {
			printUsage(argv[0]);
		} else if (strncmp(argv[i], "-j", 2) == 0) {
			char *threadstr = argv[i + 1];
			if (threadstr) {
				int n = atoi(threadstr);
				n = n < 1 ? 1 : n;
				n = n > getSysCores() * 2 ? getSysCores() * 2 : n;
				setInt(options, "thread_override", n);
			} else {
				logr(warning, "Invalid -j parameter given!\n");
			}
		} else if (strncmp(argv[i], "-d", 2) == 0) {
			char *dimstr = argv[i + 1];
			if (dimstr) {
				lineBuffer buf = {0};
				fillLineBuffer(&buf, dimstr, "x");
				char *widthStr = firstToken(&buf);
				char *heightStr = nextToken(&buf);
				if (widthStr && heightStr) {
					int width = atoi(widthStr);
					int height = atoi(heightStr);
					width = width > 65536 ? 65536 : width;
					height = height > 65536 ? 65536 : height;
					width = width < 1 ? 1 : width;
					height = height < 1 ? 1 : height;
					setTag(options, "dims_override");
					setInt(options, "dims_width", width);
					setInt(options, "dims_height", height);
				} else {
					logr(warning, "Invalid -d parameter given!\n");
				}
			}
		} else if (strncmp(argv[i], "-", 1) == 0) {
			setTag(options, ++argv[i]);
		}
	}
	logr(debug, "Verbose mode enabled\n");
}

bool isSet(char *key) {
	return exists(options, key);
}

int intPref(char *key) {
	ASSERT(exists(options, key));
	return getInt(options, key);
}

char *pathArg() {
	ASSERT(exists(options, "inputFile"));
	return getString(options, "inputFile");
}

void destroyOptions() {
	freeTable(options);
}
