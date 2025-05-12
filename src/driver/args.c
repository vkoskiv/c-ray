//
//  args.c
//  c-ray
//
//  Created by Valtteri on 6.4.2020.
//  Copyright © 2020-2023 Valtteri Koskivuori. All rights reserved.
//

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <c-ray/c-ray.h>
#include "args.h"

#include <common/platform/terminal.h>
#include <common/platform/capabilities.h>
#include <common/hashtable.h>
#include <common/logging.h>
#include <common/fileio.h>
#include <common/cr_assert.h>
#include <common/textbuffer.h>
#include <common/cr_string.h>

// FIXME: Replace this whole thing with getopt

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
	printf("    [-vv]            -> Enable very verbose mode\n");
	printf("    [--iterative]    -> Start in iterative mode (Experimental)\n");
	printf("    [--worker]       -> Start up as a network render worker (Experimental)\n");
	printf("    [--nodes <list>] -> Use worker nodes in comma-separated ip:port list for a faster render (Experimental)\n");
	printf("    [--shutdown]     -> Use in conjunction with a node list to send a shutdown command to a list of clients\n");
	printf("    [--asset-path]   -> Specify an asset path to load assets from, useful in scripts\n");
	printf("    [--no-sdl]       -> Disable render preview window\n");
	// printf("    [--test]         -> Run the test suite\n"); // FIXME
	term_restore();
	exit(0);
}

bool parseDims(const char *dimStr, int *widthOut, int *heightOut) {
	if (!dimStr) return false;
	lineBuffer buf;
	char container[LINEBUFFER_MAXSIZE];
	buf.buf = container;
	fillLineBuffer(&buf, dimStr, 'x');
	char *widthStr = firstToken(&buf);
	char *heightStr = nextToken(&buf);
	if (!widthStr && !heightStr) {
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
	return true;
}

struct driver_args *args_parse(int argc, char **argv) {
	struct driver_args *args = newConstantsDatabase();
	static bool inputFileSet = false;
	char *alternatePath = NULL;
	//Always omit the first argument.
	for (int i = 1; i < argc; ++i) {
		//FIXME: I had to move this up here to fix a bizarre bug where
		// the isValidFile() calls below returned true for "--asset-path" but
		// *only* in release builds. WTF.
		if (stringEquals(argv[i], "--asset-path")) {
			if (argv[i + 1]) {
				setDatabaseString(args, "asset_path", argv[i + 1]);
			}
			continue;
		}
		
		if (alternatePath) {
			free(alternatePath);
			alternatePath = NULL;
		}
		alternatePath = stringConcat(argv[i], ".json");
		
		if (is_valid_file(argv[i]) && !inputFileSet) {
			setDatabaseString(args, "inputFile", argv[i]);
			inputFileSet = true;
		} else if (is_valid_file(alternatePath) && !inputFileSet) {
			setDatabaseString(args, "inputFile", alternatePath);
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
				n = n > sys_get_cores() * 2 ? sys_get_cores() * 2 : n;
				setDatabaseInt(args, "thread_override", n);
			} else {
				logr(warning, "Invalid -j parameter given!\n");
			}
		}
		
		if (stringEquals(argv[i], "-s")) {
			char *sampleStr = argv[i + 1];
			if (sampleStr) {
				int n = atoi(sampleStr);
				n = n < 1 ? 1 : n;
				setDatabaseInt(args, "samples_override", n);
			} else {
				logr(warning, "Invalid -s parameter given!\n");
			}
		}
		
		if (stringEquals(argv[i], "-d")) {
			char *dimstr = argv[i + 1];
			int width = 0;
			int height = 0;
			if (parseDims(dimstr, &width, &height)) {
				setDatabaseTag(args, "dims_override");
				setDatabaseInt(args, "dims_width", width);
				setDatabaseInt(args, "dims_height", height);
			} else {
				logr(warning, "Invalid -d parameter given!\n");
			}
		}
		
		if (stringEquals(argv[i], "-t")) {
			char *dimstr = argv[i + 1];
			int width = 0;
			int height = 0;
			if (parseDims(dimstr, &width, &height)) {
				setDatabaseTag(args, "tiledims_override");
				setDatabaseInt(args, "tile_width", width);
				setDatabaseInt(args, "tile_height", height);
			} else {
				logr(warning, "Invalid -t parameter given!\n");
			}
		}

		if (stringEquals(argv[i], "-o")) {
			char *pathstr = argv[i + 1];
			setDatabaseString(args, "output_path", pathstr);
		}

		if (stringEquals(argv[i], "-c")) {
			char *str = argv[i + 1];
			if (str) {
				int n = atoi(str);
				n = n < 0 ? 0 : n;
				setDatabaseInt(args, "cam_index", n);
			} else {
				logr(warning, "Invalid -c parameter given!\n");
			}
		}

		if (stringEquals(argv[i], "--suite")) {
			if (argv[i + 1]) {
				setDatabaseString(args, "test_suite", argv[i + 1]);
			}
		}
		
		if (stringEquals(argv[i], "--test")) {
			setDatabaseTag(args, "runTests");
			char *testIdxStr = argv[i + 1];
			if (testIdxStr) {
				int n = atoi(testIdxStr);
				n = n < 0 ? 0 : n;
				setDatabaseInt(args, "test_idx", n);
			}
		}
		
		if (stringEquals(argv[i], "--test-perf")) {
			setDatabaseTag(args, "runPerfTests");
			char *testIdxStr = argv[i + 1];
			if (testIdxStr) {
				int n = atoi(testIdxStr);
				n = n < 0 ? 0 : n;
				setDatabaseInt(args, "test_idx", n);
			}
		}
		
		if (stringEquals(argv[i], "--tcount")) {
			setDatabaseTag(args, "runTests");
			setDatabaseInt(args, "test_idx", -2);
		}
		
		if (stringEquals(argv[i], "--ptcount")) {
			setDatabaseTag(args, "runTests");
			setDatabaseInt(args, "test_idx", -3);
		}
		
		if (stringEquals(argv[i], "--iterative")) {
			setDatabaseTag(args, "interactive");
		}
		
		if (stringEquals(argv[i], "--shutdown")) {
			setDatabaseTag(args, "shutdown");
		}
		
		if (stringEquals(argv[i], "--nodes")) {
			ASSERT(i + 1 <= argc);
			char *nodes = argv[i + 1];
			if (nodes) setDatabaseString(args, "nodes_list", nodes);
		}
		
		if (stringEquals(argv[i], "--worker")) {
			setDatabaseTag(args, "is_worker");
			char *portStr = argv[i + 1];
			if (portStr && portStr[0] != '-') {
				int port = atoi(portStr);
				// Verify it's in the valid port range
				port = port < 1024 ? 1024 : port;
				port = port > 65535 ? 65535 : port;
				setDatabaseInt(args, "worker_port", port);
			}
		}

		if (stringEquals(argv[i], "--no-sdl")) {
			setDatabaseTag(args, "no_sdl");
		}
		
		if (strncmp(argv[i], "-", 1) == 0) {
			int vees = 0;
			char c = 0;
			char *head = argv[i];
			while ((c = *head++))
				if (c == 'v') vees++;
			if (vees == 1) {
				setDatabaseString(args, "log_level", "debug");
			} else if (vees >= 2) {
				setDatabaseString(args, "log_level", "spam");
			}
		}
	}
	
	if (args_is_set(args, "shutdown") && args_is_set(args, "nodes_list")) {
		cr_send_shutdown_to_workers(args_string(args, "nodes_list"));
		term_restore();
		exit(0);
	}
	
	if (alternatePath) {
		free(alternatePath);
		alternatePath = NULL;
	}
	
	return args;
}

bool args_is_set(struct driver_args *args, const char *key) {
	if (!args) return false;
	return existsInDatabase(args, key);
}

int args_int(struct driver_args *args, const char *key) {
	ASSERT(existsInDatabase(args, key));
	return getDatabaseInt(args, key);
}

char *args_string(struct driver_args *args, const char *key) {
	return getDatabaseString(args, key);
}

char *args_path(struct driver_args *args) {
	ASSERT(existsInDatabase(args, "inputFile"));
	return getDatabaseString(args, "inputFile");
}

char *args_asset_path(struct driver_args *args) {
	ASSERT(existsInDatabase(args, "asset_path"));
	return getDatabaseString(args, "asset_path");
}

void args_destroy(struct driver_args *args) {
	freeConstantsDatabase(args);
}
