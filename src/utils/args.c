//
//  args.c
//  C-ray
//
//  Created by Valtteri on 6.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "args.h"
#include "hashtable.h"
#include <string.h>
#include "logging.h"
#include "fileio.h"
#include "assert.h"
#include "platform/terminal.h"
#include "platform/capabilities.h"
#include <stdlib.h>
#include "textbuffer.h"
#include "testrunner.h"
#include "string.h"

static struct hashtable *g_options;

static void printUsage(const char *progname) {
	printf("Usage: %s [-hjsdtv] [input_json...]\n", progname);
	printf("  Available options are:\n");
	printf("    [-h]            -> Show this message\n");
	printf("    [-j <n>]        -> Override thread count to n\n");
	printf("    [-s <n>]        -> Override sample count to n\n");
	printf("    [-d <w>x<h>]    -> Override image dimensions to <w>x<h>\n");
	printf("    [-t <w>x<h>]    -> Override tile  dimensions to <w>x<h>\n");
	printf("    [-v]            -> Enable verbose mode\n");
	printf("    [--interactive] -> Start in interactive mode (Experimental)\n");
	printf("    [--test]        -> Run the test suite\n");
	restoreTerminal();
	exit(0);
}

bool parseDims(const char *dimStr, int *widthOut, int *heightOut) {
	if (!dimStr) return false;
	lineBuffer *buf = newLineBuffer();
	fillLineBuffer(buf, dimStr, 'x');
	char *widthStr = firstToken(buf);
	char *heightStr = nextToken(buf);
	if (!widthStr && !heightStr) {
		destroyLineBuffer(buf);
		return false;
	}
	int width = atoi(widthStr);
	int height = atoi(heightStr);
	width = width > 65536 ? 65536 : width;
	height = height > 65536 ? 65536 : height;
	width = width < 1 ? 1 : width;
	height = height < 1 ? 1 : height;
	
	if (widthOut) *widthOut = width;
	if (heightOut) *heightOut = height;
	destroyLineBuffer(buf);
	return true;
}

void parseArgs(int argc, char **argv) {
	g_options = newTable();
	static bool inputFileSet = false;
	int testIdx = -1;
	char *alternatePath = NULL;
	//Always omit the first argument.
	for (int i = 1; i < argc; ++i) {
		
		if (alternatePath) {
			free(alternatePath);
			alternatePath = NULL;
		}
		alternatePath = stringConcat(argv[i], ".json");
		
		if (isValidFile(argv[i]) && !inputFileSet) {
			setString(g_options, "inputFile", argv[i]);
			inputFileSet = true;
		} else if (isValidFile(alternatePath) && !inputFileSet) {
			setString(g_options, "inputFile", alternatePath);
			inputFileSet = true;
		}else if (stringEquals(argv[i], "-h")) {
			printUsage(argv[0]);
		} else if (stringEquals(argv[i], "-j")) {
			char *threadstr = argv[i + 1];
			if (threadstr) {
				int n = atoi(threadstr);
				n = n < 1 ? 1 : n;
				n = n > getSysCores() * 2 ? getSysCores() * 2 : n;
				setInt(g_options, "thread_override", n);
			} else {
				logr(warning, "Invalid -j parameter given!\n");
			}
		} else if (stringEquals(argv[i], "-s")) {
			char *sampleStr = argv[i + 1];
			if (sampleStr) {
				int n = atoi(sampleStr);
				n = n < 1 ? 1 : n;
				setInt(g_options, "samples_override", n);
			} else {
				logr(warning, "Invalid -s parameter given!\n");
			}
		} else if (stringEquals(argv[i], "-d")) {
			char *dimstr = argv[i + 1];
			int width = 0;
			int height = 0;
			if (parseDims(dimstr, &width, &height)) {
				setTag(g_options, "dims_override");
				setInt(g_options, "dims_width", width);
				setInt(g_options, "dims_height", height);
			} else {
				logr(warning, "Invalid -d parameter given!\n");
			}
		} else if (stringEquals(argv[i], "-t")) {
			char *dimstr = argv[i + 1];
			int width = 0;
			int height = 0;
			if (parseDims(dimstr, &width, &height)) {
				setTag(g_options, "tiledims_override");
				setInt(g_options, "tile_width", width);
				setInt(g_options, "tile_height", height);
			} else {
				logr(warning, "Invalid -t parameter given!\n");
			}
		} else if (stringEquals(argv[i], "--test")) {
			setTag(g_options, "runTests");
			char *testIdxStr = argv[i + 1];
			if (testIdxStr) {
				int n = atoi(testIdxStr);
				n = n < 0 ? 0 : n;
				testIdx = n;
			}
		} else if (stringEquals(argv[i], "--test-perf")) {
			setTag(g_options, "runPerfTests");
			char *testIdxStr = argv[i + 1];
			if (testIdxStr) {
				int n = atoi(testIdxStr);
				n = n < 0 ? 0 : n;
				testIdx = n;
			}
		} else if (stringEquals(argv[i], "--tcount")) {
			setTag(g_options, "runTests");
			testIdx = -2;
		} else if (stringEquals(argv[i], "--ptcount")) {
			setTag(g_options, "runTests");
			testIdx = -3;
		} else if (stringEquals(argv[i], "--interactive")) {
			setTag(g_options, "interactive");
		} else if (strncmp(argv[i], "-", 1) == 0) {
			setTag(g_options, ++argv[i]);
		}
	}
	logr(debug, "Verbose mode enabled\n");
	
	if (alternatePath) {
		free(alternatePath);
		alternatePath = NULL;
	}
	
	if (isSet("runTests") || isSet("runPerfTests")) {
#ifdef CRAY_TESTING
		switch (testIdx) {
			case -3:
				printf("%i", getPerfTestCount());
				exit(0);
				break;
			case -2:
				printf("%i", getTestCount());
				exit(0);
				break;
			case -1:
				exit(isSet("runPerfTests") ? runPerfTests() : runTests());
				break;
			default:
				exit(isSet("runPerfTests") ? runPerfTest(testIdx) : runTest(testIdx));
				break;
		}
#else
		logr(warning, "You need to compile with tests enabled.\n");
		logr(warning, "Run: `cmake . -DTESTING=True` and then `make`\n");
		exit(-1);
#endif
	}
}

bool isSet(char *key) {
	return exists(g_options, key);
}

int intPref(char *key) {
	ASSERT(exists(g_options, key));
	return getInt(g_options, key);
}

char *pathArg() {
	ASSERT(exists(g_options, "inputFile"));
	return getString(g_options, "inputFile");
}

void destroyOptions() {
	freeTable(g_options);
}
