//
//  multiplatform.c
//  C-Ray
//
//  Created by Valtteri on 19/03/2019.
//  Copyright © 2019 Valtteri Koskivuori. All rights reserved.
//


#include "../includes.h"

void initTerminal() {
	//Disable output buffering
	setbuf(stdout, NULL);
	
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
