//
//  args.h
//  C-ray
//
//  Created by Valtteri on 6.4.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

void parseOptions(int argc, char **argv);

bool isSet(char *key);

char *pathArg(void);

void destroyOptions(void);
