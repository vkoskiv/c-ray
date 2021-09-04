//
//  args.h
//  C-ray
//
//  Created by Valtteri on 6.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

void parseArgs(int argc, char **argv);

bool isSet(char *key);

int intPref(char *key);

char *stringPref(char *key);

char *pathArg(void);

char *specifiedAssetPath(void);

void destroyOptions(void);
