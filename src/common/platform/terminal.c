//
//  terminal.c
//  c-ray
//
//  Created by Valtteri on 29.3.2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#include "terminal.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef WINDOWS
#include <Windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include "signal.h"
#include "../logging.h"

bool isTeleType(void) {
#ifdef WINDOWS
	return _isatty(_fileno(stdout));
#else
	return isatty(fileno(stdout));
#endif
}

static void show_cursor(bool show) {
	(void)show;
#ifndef NO_COLOR
	if (isTeleType()) show ? fputs("\033[?25h", stdout) : fputs("\033[?25l", stdout);
#endif
}

//FIXME: This doesn't perform cleanup
static void handler(int sig) {
	if (sig == 2) { //SIGINT
		printf("\n");
		logr(info, "Aborting initialization.\n");
		term_restore();
		exit(0);
	}
}

void term_init(void) {
	if (registerHandler(sigint, handler)) {
		logr(warning, "Unable to catch SIGINT\n");
	}
	//If we're on a reasonable (non-windows) terminal, hide the cursor.
#ifndef WINDOWS
	//Disable output buffering
	setbuf(stdout, NULL);
	// show_cursor(false);
#endif
	
	//Configure Windows terminals to handle color escape codes
#ifdef WINDOWS
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut != INVALID_HANDLE_VALUE) {
		DWORD dwMode = 0;
		if (GetConsoleMode(hOut, &dwMode)) {
			dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			SetConsoleMode(hOut, dwMode);
		}
	}
#endif
}

void term_restore(void) {
#ifndef WINDOWS
	show_cursor(true);
#endif
}
