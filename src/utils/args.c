//
//  args.c
//  C-ray
//
//  Created by Valtteri on 6.4.2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
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
#include "protocol/server.h"

static struct constantsDatabase *g_options;

static void printUsage(const char *progname) {
	printf("Usage: %s [-hjsdtocv] [input_json...]\n", progname);
	printf("  Available options are:\n");
	printf("    [-h]             -> Show this message\n");
	printf("    [-j <n>]         -> Override thread count to n\n");
	printf("    [-s <n>]         -> Override sample count to n\n");
	printf("    [-d <w>x<h>]     -> Override image dimensions to <w>x<h>\n");
	printf("    [-t <w>x<h>]     -> Override tile  dimensions to <w>x<h>\n");
	printf("    [-o <path>]      -> Override output file path to <path>\n");
	printf("    [-c <cam_index>] -> Select camera. Defaults to 0\n");
	printf("    [-v]             -> Enable verbose mode\n");
	printf("    [--iterative]    -> Start in iterative mode (Experimental)\n");
	printf("    [--worker]       -> Start up as a network render worker (Experimental)\n");
	printf("    [--nodes <list>] -> Use worker nodes in comma-separated ip:port list for a faster render (Experimental)\n");
	printf("    [--shutdown]     -> Use in conjunction with a node list to send a shutdown command to a list of clients\n");
	printf("    [--asset-path]   -> Specify an asset path to load assets from, useful in scripts\n");
	printf("    [--test]         -> Run the test suite\n");
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
	g_options = newConstantsDatabase();
	static bool inputFileSet = false;
	int testIdx = -1;
	(void)testIdx;
	char *alternatePath = NULL;
	//Always omit the first argument.
	for (int i = 1; i < argc; ++i) {
		
		if (alternatePath) {
			free(alternatePath);
			alternatePath = NULL;
		}
		alternatePath = stringConcat(argv[i], ".json");
		
		if (isValidFile(argv[i], NULL) && !inputFileSet) {
			setDatabaseString(g_options, "inputFile", argv[i]);
			inputFileSet = true;
		} else if (isValidFile(alternatePath, NULL) && !inputFileSet) {
			setDatabaseString(g_options, "inputFile", alternatePath);
			inputFileSet = true;
		}
		
		if (stringEquals(argv[i], "-h")) {
			printUsage(argv[0]);
		}
		
		if (stringEquals(argv[i], "-j")) {
			char *threadstr = argv[i + 1];
			if (threadstr) {
				int n = atoi(threadstr);
				n = n < 0 ? 0 : n;
				n = n > getSysCores() * 2 ? getSysCores() * 2 : n;
				setDatabaseInt(g_options, "thread_override", n);
			} else {
				logr(warning, "Invalid -j parameter given!\n");
			}
		}
		
		if (stringEquals(argv[i], "-s")) {
			char *sampleStr = argv[i + 1];
			if (sampleStr) {
				int n = atoi(sampleStr);
				n = n < 1 ? 1 : n;
				setDatabaseInt(g_options, "samples_override", n);
			} else {
				logr(warning, "Invalid -s parameter given!\n");
			}
		}
		
		if (stringEquals(argv[i], "-d")) {
			char *dimstr = argv[i + 1];
			int width = 0;
			int height = 0;
			if (parseDims(dimstr, &width, &height)) {
				setDatabaseTag(g_options, "dims_override");
				setDatabaseInt(g_options, "dims_width", width);
				setDatabaseInt(g_options, "dims_height", height);
			} else {
				logr(warning, "Invalid -d parameter given!\n");
			}
		}
		
		if (stringEquals(argv[i], "-t")) {
			char *dimstr = argv[i + 1];
			int width = 0;
			int height = 0;
			if (parseDims(dimstr, &width, &height)) {
				setDatabaseTag(g_options, "tiledims_override");
				setDatabaseInt(g_options, "tile_width", width);
				setDatabaseInt(g_options, "tile_height", height);
			} else {
				logr(warning, "Invalid -t parameter given!\n");
			}
		}

		if (stringEquals(argv[i], "-o")) {
			char *pathstr = argv[i + 1];
			setDatabaseString(g_options, "output_path", pathstr);
		}

		if (stringEquals(argv[i], "-c")) {
			char *str = argv[i + 1];
			if (str) {
				int n = atoi(str);
				n = n < 0 ? 0 : n;
				setDatabaseInt(g_options, "cam_index", n);
			} else {
				logr(warning, "Invalid -c parameter given!\n");
			}
		}

		if (stringEquals(argv[i], "--suite")) {
			if (argv[i + 1]) {
				setDatabaseString(g_options, "test_suite", argv[i + 1]);
			}
		}
		
		if (stringEquals(argv[i], "--asset-path")) {
			if (argv[i + 1]) {
				setDatabaseString(g_options, "asset_path", argv[i + 1]);
			}
		}
		
		if (stringEquals(argv[i], "--test")) {
			setDatabaseTag(g_options, "runTests");
			char *testIdxStr = argv[i + 1];
			if (testIdxStr) {
				int n = atoi(testIdxStr);
				n = n < 0 ? 0 : n;
				testIdx = n;
			}
		}
		
		if (stringEquals(argv[i], "--test-perf")) {
			setDatabaseTag(g_options, "runPerfTests");
			char *testIdxStr = argv[i + 1];
			if (testIdxStr) {
				int n = atoi(testIdxStr);
				n = n < 0 ? 0 : n;
				testIdx = n;
			}
		}
		
		if (stringEquals(argv[i], "--tcount")) {
			setDatabaseTag(g_options, "runTests");
			testIdx = -2;
		}
		
		if (stringEquals(argv[i], "--ptcount")) {
			setDatabaseTag(g_options, "runTests");
			testIdx = -3;
		}
		
		if (stringEquals(argv[i], "--iterative")) {
			setDatabaseTag(g_options, "interactive");
		}
		
		if (stringEquals(argv[i], "--shutdown")) {
			setDatabaseTag(g_options, "shutdown");
		}
		
		if (stringEquals(argv[i], "--nodes")) {
			setDatabaseTag(g_options, "use_clustering");
			ASSERT(i + 1 <= argc);
			char *nodes = argv[i + 1];
			if (nodes) setDatabaseString(g_options, "nodes_list", nodes);
		}
		
		if (stringEquals(argv[i], "--worker")) {
			setDatabaseTag(g_options, "is_worker");
			char *portStr = argv[i + 1];
			if (portStr && portStr[0] != '-') {
				int port = atoi(portStr);
				// Verify it's in the valid port range
				port = port < 1024 ? 1024 : port;
				port = port > 65535 ? 65535 : port;
				setDatabaseInt(g_options, "worker_port", port);
			}
		}
		
		if (strncmp(argv[i], "-", 1) == 0) {
			setDatabaseTag(g_options, ++argv[i]);
		}
	}
	logr(debug, "Verbose mode enabled\n");
	
	if (isSet("shutdown") && isSet("nodes_list")) {
		shutdownClients();
		restoreTerminal();
		exit(0);
	}
	
	if (alternatePath) {
		free(alternatePath);
		alternatePath = NULL;
	}
	
	if (isSet("runTests") || isSet("runPerfTests")) {
#ifdef CRAY_TESTING
		char *suite = NULL;
		if (isSet("test_suite")) suite = getDatabaseString(g_options, "test_suite");
		switch (testIdx) {
			case -3:
				printf("%i", getPerfTestCount(suite));
				exit(0);
				break;
			case -2:
				printf("%i", getTestCount(suite));
				exit(0);
				break;
			case -1:
				exit(isSet("runPerfTests") ? runPerfTests(suite) : runTests(suite));
				break;
			default:
				exit(isSet("runPerfTests") ? runPerfTest(testIdx, suite) : runTest(testIdx, suite));
				break;
		}
#else
		logr(warning, "You need to compile with tests enabled.\n");
		logr(warning, "Run: `cmake . -DTESTING=True` and then `make`\n");
		exit(-1);
#endif
	}
}

bool isSet(const char *key) {
	if (!g_options) return false;
	return existsInDatabase(g_options, key);
}

int intPref(const char *key) {
	ASSERT(existsInDatabase(g_options, key));
	return getDatabaseInt(g_options, key);
}

char *stringPref(const char *key) {
	return getDatabaseString(g_options, key);
}

char *pathArg() {
	ASSERT(existsInDatabase(g_options, "inputFile"));
	return getDatabaseString(g_options, "inputFile");
}

char *specifiedAssetPath(void) {
	ASSERT(existsInDatabase(g_options, "asset_path"));
	return getDatabaseString(g_options, "asset_path");
}

void destroyOptions() {
	freeConstantsDatabase(g_options);
}
