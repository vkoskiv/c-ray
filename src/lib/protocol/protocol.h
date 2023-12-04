//
//  protocol.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 21/03/2021.
//  Copyright © 2021-2022 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include "../../common/vendored/cJSON.h"
#include <stdbool.h>

#define PROTO_VERSION "0.1"

struct render_tile;
struct render_client;
struct texture;
struct renderer;

struct command {
	char *name;
	int id;
};

int matchCommand(const struct command *cmdlist, size_t commandCount, const char *cmd);

// Consumes given json, no need to free it after.
bool sendJSON(int socket, cJSON *json, size_t *progress);

cJSON *readJSON(int socket);

cJSON *errorResponse(const char *error);

cJSON *goodbye(void);

cJSON *newAction(const char *action);

cJSON *encodeTile(const struct render_tile *tile);

struct render_tile decodeTile(const cJSON *json);

cJSON *serialize_texture(const struct texture *t);

struct texture *deserialize_texture(const cJSON *json);

bool containsError(const cJSON *json);

bool containsGoodbye(const cJSON *json);

bool containsStats(const cJSON *json);

char *serialize_renderer(const struct renderer *r);
struct renderer *deserialize_renderer(const char *data);

void dump_renderer_state(const struct renderer *r);
