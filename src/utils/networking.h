//
//  networking.h
//  C-ray
//
//  Created by Valtteri on 5.1.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#ifndef WINDOWS

#include <unistd.h>

bool chunkedSend(int socket, const char *data);

ssize_t chunkedReceive(int socket, char **data);

#endif
