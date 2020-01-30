//
//  multiplatform.h
//  C-Ray
//
//  Created by Valtteri on 19/03/2019.
//  Copyright © 2015-2020 Valtteri Koskivuori. All rights reserved.
//

//Multi-platform stubs for thread-handling and other operations that differ on *nix and WIN32

///Prepare terminal. On *nix this disables output buffering, on WIN32 it enables ANSI escape codes.
void initTerminal(void);

/// Restore previous terminal state. On *nix it un-hides the cursor.
void restoreTerminal(void);

/// Get amount of logical processing cores on the system
/// @remark Is unaware of NUMA nodes on high core count systems
/// @return Amount of logical processing cores
int getSysCores(void);

//Thread handling

