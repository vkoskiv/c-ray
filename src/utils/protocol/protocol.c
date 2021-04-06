//
//  protocol.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 21/03/2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#include "protocol.h"

#include <string.h>

#include "../../utils/logging.h"
#include "../../libraries/cJSON.h"
#include "../../datatypes/image/imagefile.h"
#include "../../datatypes/vector.h"
#include "../../datatypes/vertexbuffer.h"
#include "../../datatypes/tile.h"
#include "../../datatypes/image/texture.h"
#include "../../datatypes/color.h"
#include "assert.h"
#include "../string.h"
#include "../gitsha1.h"
#include "../networking.h"
#include "../base64.h"

// Consumes given json, no need to free it after.
bool sendJSON(int socket, cJSON *json) {
	ASSERT(json);
	char *jsonText = cJSON_PrintUnformatted(json);
	cJSON_Delete(json);
	bool ret = chunkedSend(socket, jsonText);
	free(jsonText);
	return ret;
}

cJSON *readJSON(int socket) {
	char *recvBuf = NULL;
	if (chunkedReceive(socket, &recvBuf) == 0) {
		return NULL;
	}
	cJSON *received = cJSON_Parse(recvBuf);
	free(recvBuf);
	return received;
}

cJSON *errorResponse(char *error) {
	cJSON *errorMsg = cJSON_CreateObject();
	cJSON_AddStringToObject(errorMsg, "error", error);
	return errorMsg;
}

cJSON *goodbye() {
	cJSON *goodbye = cJSON_CreateObject();
	cJSON_AddStringToObject(goodbye, "action", "goodbye");
	return goodbye;
}

cJSON *newAction(char *action) {
	if (!action) return NULL;
	cJSON *actionJson = cJSON_CreateObject();
	cJSON_AddStringToObject(actionJson, "action", action);
	return actionJson;
}

cJSON *encodeTile(struct renderTile tile) {
	cJSON *json = cJSON_CreateObject();
	cJSON_AddNumberToObject(json, "width", tile.width);
	cJSON_AddNumberToObject(json, "height", tile.height);
	cJSON_AddNumberToObject(json, "beginX", tile.begin.x);
	cJSON_AddNumberToObject(json, "beginY", tile.begin.y);
	cJSON_AddNumberToObject(json, "endX", tile.end.x);
	cJSON_AddNumberToObject(json, "endY", tile.end.y);
	cJSON_AddNumberToObject(json, "tileNum", tile.tileNum);
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

int matchCommand(struct command *cmdlist, size_t commandCount, char *cmd) {
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
