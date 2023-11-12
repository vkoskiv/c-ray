//
//  args.h
//  C-ray
//
//  Created by Valtteri on 6.4.2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

void args_parse(int argc, char **argv);

bool args_is_set(const char *key);

int args_int(const char *key);

char *args_string(const char *key);

char *args_path(void);

char *args_asset_path(void);

void args_destroy(void);
