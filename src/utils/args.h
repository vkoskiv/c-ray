//
//  args.h
//  C-ray
//
//  Created by Valtteri on 6.4.2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

void parseArgs(int argc, char **argv);

bool isSet(const char *key);

int intPref(const char *key);

char *stringPref(const char *key);

char *pathArg(void);

char *specifiedAssetPath(void);

void destroyOptions(void);
