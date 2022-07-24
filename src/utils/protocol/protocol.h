//
//  protocol.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 21/03/2021.
//  Copyright © 2021-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../../vendored/cJSON.h"
#include <stdbool.h>

#define C_RAY_DEFAULT_PORT 2222
#define PROTO_VERSION "0.1"

struct renderTile;
struct renderClient;
struct texture;
struct renderer;

struct command {
	char *name;
	int id;
};

int matchCommand(const struct command *cmdlist, size_t commandCount, const char *cmd);

// Consumes given json, no need to free it after.
bool sendJSON(int socket, cJSON *json);

// Same as above, but progress gets updated with a 0-100 value of where we're at.
bool sendJSONWithProgress(int socket, cJSON *json, size_t *progress);

cJSON *readJSON(int socket);

cJSON *errorResponse(const char *error);

cJSON *goodbye(void);

cJSON *newAction(const char *action);

cJSON *encodeTile(const struct renderTile *tile);

struct renderTile decodeTile(const cJSON *json);

cJSON *encodeTexture(const struct texture *t);

struct texture *decodeTexture(const cJSON *json);

bool containsError(const cJSON *json);

bool containsGoodbye(const cJSON *json);

bool containsStats(const cJSON *json);

void disconnectFromClient(struct renderClient *client);
