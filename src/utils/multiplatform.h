//
//  multiplatform.h
//  C-Ray
//
//  Created by Valtteri on 19/03/2019.
//  Copyright © 2019 Valtteri Koskivuori. All rights reserved.
//

//Multi-platform stubs for thread-handling and other operations that differ on *nix and WIN32


//Prepare terminal. On *nix this disables output buffering, on WIN32 it enables ANSI escape codes.
void initTerminal(void);

//Thread handling

