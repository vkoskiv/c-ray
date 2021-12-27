//
//  terminal.h
//  C-ray
//
//  Created by Valtteri on 29.3.2020.
//  Copyright © 2020-2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <stdbool.h>

bool isTeleType(void);

///Prepare terminal. On *nix this disables output buffering, on WIN32 it enables ANSI escape codes.
void initTerminal(void);

/// Restore previous terminal state. On *nix it un-hides the cursor.
void restoreTerminal(void);
