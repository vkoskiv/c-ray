//
//  protocol.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 21/03/2021.
//  Copyright © 2021-2022 Valtteri Koskivuori. All rights reserved.
//

#include "protocol.h"

#ifndef WINDOWS

#include <string.h>

#include "../../utils/logging.h"
#include "../../datatypes/image/imagefile.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/tile.h"
#include "../../datatypes/image/texture.h"
#include "assert.h"
#include "../string.h"
#include "../gitsha1.h"
#include "../networking.h"
#include "../base64.h"
#include "../timer.h"

// Consumes given json, no need to free it after.
bool sendJSON(int socket, cJSON *json) {
	ASSERT(json);
	char *jsonText = cJSON_PrintUnformatted(json);
	cJSON_Delete(json);
	bool ret = chunkedSend(socket, jsonText, NULL);
	free(jsonText);
	return ret;
}

bool sendJSONWithProgress(int socket, cJSON *json, size_t *progress) {
	ASSERT(json);
	char *jsonText = cJSON_PrintUnformatted(json);
	cJSON_Delete(json);
	bool ret = chunkedSend(socket, jsonText, progress);
	free(jsonText);
	return ret;
}

cJSON *readJSON(int socket) {
	char *recvBuf = NULL;
	size_t length = 0;
	struct timeval timer;
	timer_start(&timer);
	if (chunkedReceive(socket, &recvBuf, &length) == 0) {
		return NULL;
	}
	long millisecs = timer_get_ms(timer);
	char *size = humanFileSize(length);
	logr(debug, "Received %s, took %lums.\n", size, millisecs);
	free(size);
	
	cJSON *received = cJSON_Parse(recvBuf);
	free(recvBuf);
	return received;
}

cJSON *errorResponse(const char *error) {
	cJSON *errorMsg = cJSON_CreateObject();
	cJSON_AddStringToObject(errorMsg, "error", error);
	return errorMsg;
}

cJSON *goodbye() {
	cJSON *goodbye = cJSON_CreateObject();
	cJSON_AddStringToObject(goodbye, "action", "goodbye");
	return goodbye;
}

cJSON *newAction(const char *action) {
	if (!action) return NULL;
	cJSON *actionJson = cJSON_CreateObject();
	cJSON_AddStringToObject(actionJson, "action", action);
	return actionJson;
}

cJSON *encodeTile(const struct renderTile *tile) {
	cJSON *json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json, "width", tile->width);
	cJSON_AddNumberToObject(json, "height", tile->height);
	cJSON_AddNumberToObject(json, "beginX", tile->begin.x);
	cJSON_AddNumberToObject(json, "beginY", tile->begin.y);
	cJSON_AddNumberToObject(json, "endX", tile->end.x);
	cJSON_AddNumberToObject(json, "endY", tile->end.y);
	cJSON_AddNumberToObject(json, "tileNum", tile->tileNum);
	return json;
}

struct renderTile decodeTile(const cJSON *json) {
	struct renderTile tile = {0};
	tile.width = cJSON_GetObjectItem(json, "width")->valueint;
	tile.height = cJSON_GetObjectItem(json, "height")->valueint;
	tile.begin.x = cJSON_GetObjectItem(json, "beginX")->valueint;
	tile.begin.y = cJSON_GetObjectItem(json, "beginY")->valueint;
	tile.end.x = cJSON_GetObjectItem(json, "endX")->valueint;
	tile.end.y = cJSON_GetObjectItem(json, "endY")->valueint;
	tile.tileNum = cJSON_GetObjectItem(json, "tileNum")->valueint;
	return tile;
}

cJSON *encodeTexture(const struct texture *t) {
	cJSON *json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json, "width", t->width);
	cJSON_AddNumberToObject(json, "height", t->height);
	cJSON_AddNumberToObject(json, "channels", t->channels);
	size_t primSize = t->precision == char_p ? sizeof(char) : sizeof(float);
	size_t bytes = t->width * t->height * t->channels * primSize;
	char *encoded = b64encode(t->data.byte_p, bytes);
	cJSON_AddStringToObject(json, "data", encoded);
	cJSON_AddBoolToObject(json, "isFloatPrecision", t->precision == float_p);
	free(encoded);
	return json;
}

struct texture *decodeTexture(const cJSON *json) {
	struct texture *tex = calloc(1, sizeof(*tex));
	tex->hasAlpha = false;
	tex->colorspace = linear;
	char *data = cJSON_GetStringValue(cJSON_GetObjectItem(json, "data"));
	tex->data.byte_p = b64decode(data, strlen(data), NULL);
	tex->width = cJSON_GetNumberValue(cJSON_GetObjectItem(json, "width"));
	tex->height = cJSON_GetNumberValue(cJSON_GetObjectItem(json, "height"));
	tex->channels = cJSON_GetNumberValue(cJSON_GetObjectItem(json, "channels"));
	tex->precision = cJSON_IsTrue(cJSON_GetObjectItem(json, "isFloatPrecision")) ? float_p : char_p;
	return tex;
}

int matchCommand(const struct command *cmdlist, size_t commandCount, const char *cmd) {
	for (size_t i = 0; i < commandCount; ++i) {
		if (stringEquals(cmdlist[i].name, cmd)) return cmdlist[i].id;
	}
	return -1;
}

bool containsError(const cJSON *json) {
	const cJSON *error = cJSON_GetObjectItem(json, "error");
	if (cJSON_IsString(error)) {
		return true;
	}
	return false;
}

bool containsGoodbye(const cJSON *json) {
	const cJSON *action = cJSON_GetObjectItem(json, "action");
	if (cJSON_IsString(action)) {
		if (stringEquals(action->valuestring, "goodbye")) {
			return true;
		}
	}
	return false;
}

bool containsStats(const cJSON *json) {
	const cJSON *action = cJSON_GetObjectItem(json, "action");
	if (cJSON_IsString(action)) {
		if (stringEquals(action->valuestring, "stats")) {
			return true;
		}
	}
	return false;
}

#endif
