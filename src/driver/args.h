//
//  args.h
//  C-ray
//
//  Created by Valtteri on 6.4.2020.
//  Copyright © 2020-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct driver_args *args_parse(int argc, char **argv);

bool args_is_set(struct driver_args *args, const char *key);

int args_int(struct driver_args *args, const char *key);

char *args_string(struct driver_args *args, const char *key);

char *args_path(struct driver_args *args);

char *args_asset_path(struct driver_args *args);

void args_destroy(struct driver_args *args);
