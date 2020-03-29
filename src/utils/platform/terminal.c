//
//  terminal.c
//  C-ray
//
//  Created by Valtteri on 29.3.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "terminal.h"
#include <stdbool.h>
#include <stdio.h>

#ifdef WINDOWS
#include <Windows.h>
#endif

void showCursor(bool show) {
	show ? fputs("\e[?25h", stdout) : fputs("\e[?25l", stdout);
}

void initTerminal() {
	//If we're on a reasonable (non-windows) terminal, hide the cursor.
#ifndef WINDOWS
	//Disable output buffering
	setbuf(stdout, NULL);
	showCursor(false);
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

void restoreTerminal() {
#ifndef WINDOWS
	showCursor(true);
#endif
}
