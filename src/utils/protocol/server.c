//
//  server.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 06/04/2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#include <stddef.h>
#include "../logging.h"
//Windows is annoying, so it's just not going to have networking. Because it is annoying and proprietary.
#ifndef WINDOWS

#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "server.h"
#include "protocol.h"

#include "../../renderer/renderer.h"
#include "../../datatypes/image/texture.h"
#include "../platform/thread.h"
#include "../networking.h"
#include "../textbuffer.h"
#include "../gitsha1.h"
#include "../args.h"
#include "../assert.h"
#include "../filecache.h"

void disconnectFromClient(struct renderClient *client) {
	ASSERT(client->socket != -1);
	shutdown(client->socket, SHUT_RDWR);
	close(client->socket);
	client->socket = -1;
	client->state = Disconnected;
}

static cJSON *makeHandshake() {
	cJSON *handshake = cJSON_CreateObject();
	cJSON_AddStringToObject(handshake, "action", "handshake");
	cJSON_AddStringToObject(handshake, "version", PROTO_VERSION);
	cJSON_AddStringToObject(handshake, "githash", gitHash());
	return handshake;
}

static struct sockaddr_in parseAddress(const char *str) {
	lineBuffer *line = newLineBuffer();
	fillLineBuffer(line, str, ':');
	struct sockaddr_in address = {0};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(firstToken(line));
	address.sin_port = line->amountOf.tokens > 1 ? htons(atoi(lastToken(line))) : htons(2222);
	destroyLineBuffer(line);
	return address;
}

static bool connectToClient(struct renderClient *client) {
	client->socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client->socket == -1) {
		logr(warning, "Failed to bind to socket on client %i\n", client->id);
		client->state = ConnectionFailed;
		return false;
	}
	logr(debug, "Attempting connection to %s...\n", inet_ntoa(client->address.sin_addr));
	bool success = false;
	fcntl(client->socket, F_SETFL, O_NONBLOCK);
	fd_set fdset;
	struct timeval tv;
	connect(client->socket, (struct sockaddr *)&client->address, sizeof(client->address));
	FD_ZERO(&fdset);
	FD_SET(client->socket, &fdset);
	tv.tv_sec = 2; // 2 second timeout.
	tv.tv_usec = 0;
	
	if (select(client->socket + 1, NULL, &fdset, NULL, &tv) == 1) {
		int so_error ;
		socklen_t len = sizeof(so_error);
		getsockopt(client->socket, SOL_SOCKET, SO_ERROR, &so_error, &len);
		if (so_error == 0) {
			logr(debug, "Connected to %s:%i\n", inet_ntoa(client->address.sin_addr), htons(client->address.sin_port));
			success = true;
			client->state = Connected;
			// Unset non-blocking
			int oldFlags = fcntl(client->socket, F_GETFL);
			fcntl(client->socket, F_SETFL, oldFlags & ~O_NONBLOCK);
		} else {
			logr(debug, "%s on %s:%i, dropping.\n", strerror(so_error), inet_ntoa(client->address.sin_addr), htons(client->address.sin_port));
			close(client->socket);
			client->state = ConnectionFailed;
		}
	}
	return success;
}

// Fetches list of nodes from arguments, verifies that they are reachable, and
// returns them in a nice list. Also got the size there in the amount param, if you need it.
static struct renderClient *buildClientList(size_t *amount) {
	ASSERT(isSet("use_clustering"));
	ASSERT(isSet("nodes_list"));
	char *nodesString = stringPref("nodes_list");
	// Really barebones parsing for IP addresses and ports in a comma-separated list
	// Expected to break easily. Don't break it.
	lineBuffer *line = newLineBuffer();
	fillLineBuffer(line, nodesString, ',');
	ASSERT(line->amountOf.tokens > 0);
	size_t clientCount = line->amountOf.tokens;
	struct renderClient *clients = calloc(clientCount, sizeof(*clients));
	char *current = firstToken(line);
	for (size_t i = 0; i < clientCount; ++i) {
		clients[i].address = parseAddress(current);
		clients[i].state = connectToClient(&clients[i]) ? Connected : ConnectionFailed;
		current = nextToken(line);
	}
	size_t validClients = 0;
	for (size_t i = 0; i < clientCount; ++i) {
		validClients += clients[i].state == ConnectionFailed ? 0 : 1;
	}
	if (validClients < clientCount) {
		// Prune unavailable clients
		struct renderClient *confirmedClients = calloc(validClients, sizeof(*confirmedClients));
		size_t j = 0;
		for (size_t i = 0; i < clientCount; ++i) {
			if (clients[i].state != ConnectionFailed) {
				confirmedClients[j++] = clients[i];
			}
		}
		free(clients);
		clients = confirmedClients;
	}
	
	for (size_t i = 0; i < validClients; ++i) {
		clients[i].id = (int)i;
	}
	
	if (amount) *amount = validClients;
	destroyLineBuffer(line);
	return clients;
}

static cJSON *processGetWork(struct renderThreadState *state, const cJSON *json) {
	(void)state;
	(void)json;
	struct renderTile tile = nextTile(state->renderer);
	state->renderer->state.renderTiles[tile.tileNum].networkRenderer = true;
	if (tile.tileNum == -1) return errorResponse("renderComplete");
	cJSON *response = newAction("newWork");
	cJSON_AddItemToObject(response, "tile", encodeTile(tile));
	return response;
}

static cJSON *processSubmitWork(struct renderThreadState *state, const cJSON *json) {
	cJSON *resultJson = cJSON_GetObjectItem(json, "result");
	struct texture *tileImage = decodeTexture(resultJson);
	cJSON *tileJson = cJSON_GetObjectItem(json, "tile");
	struct renderTile tile = decodeTile(tileJson);
	state->renderer->state.renderTiles[tile.tileNum].isRendering = false;
	state->renderer->state.renderTiles[tile.tileNum].renderComplete = true;
	for (int y = tile.end.y - 1; y > tile.begin.y - 1; --y) {
		for (int x = tile.begin.x; x < tile.end.x; ++x) {
			struct color value = textureGetPixel(tileImage, x - tile.begin.x, y - tile.begin.x, false);
			setPixel(state->output, value, x, y);
		}
	}
	destroyTexture(tileImage);
	return newAction("ok");
}

struct command serverCommands[] = {
	{"getWork", 0},
	{"submitWork", 1},
};

static cJSON *processClientRequest(struct renderThreadState *state, const cJSON *json) {
	if (!json) {
		return errorResponse("Couldn't parse incoming JSON");
	}
	const cJSON *action = cJSON_GetObjectItem(json, "action");
	if (!cJSON_IsString(action)) {
		return errorResponse("No action provided");
	}
	
	switch (matchCommand(serverCommands, sizeof(serverCommands) / sizeof(struct command), action->valuestring)) {
		case 0:
			return processGetWork(state, json);
			break;
		case 1:
			return processSubmitWork(state, json);
			break;
		default:
			return errorResponse("Unknown command");
			break;
	}
	
	return goodbye();
	ASSERT_NOT_REACHED();
	return NULL;
}

// Master side
void *networkRenderThread(void *arg) {
	struct renderThreadState *state = (struct renderThreadState *)threadUserData(arg);
	struct renderer *r = state->renderer;
	//struct texture *image = state->output;
	struct renderClient *client = state->client;
	if (!client) {
		state->threadComplete = true;
		return 0;
	}
	if (client->state != Synced) {
		logr(debug, "Client %i wasn't synced fully, dropping.\n", client->id);
		state->threadComplete = true;
		return 0;
	}
	
	// Set this worker into render mode
	if (!sendJSON(client->socket, newAction("startRender"))) {
		logr(warning, "Client disconnected? Stopping for %i\n", client->id);
		state->threadComplete = true;
		return 0;
	}
	
	// And just wait for commands.
	while (r->state.isRendering) {
		cJSON *request = readJSON(client->socket);
		if (!request) break;
		cJSON *response = processClientRequest(state, request);
		if (containsError(response)) {
			sendJSON(client->socket, response);
			break;
		}
		sendJSON(client->socket, response);
		cJSON_Delete(request);
	}
	
	// Let the worker now we're done here
	// TODO (right now we disconnect, and the client implies from that)
	state->threadComplete = true;
	return 0;
}

struct syncThreadParams {
	struct renderClient *client;
	const struct renderer *renderer;
};

//TODO: Rename to clientSyncThread
static void *handleClientSync(void *arg) {
	struct syncThreadParams *params = (struct syncThreadParams *)threadUserData(arg);
	struct renderClient *client = params->client;
	if (client->state != Connected) {
		logr(warning, "Won't sync with client %i, no connection.\n", client->id);
		return NULL;
	}
	client->state = Syncing;
	
	// Handshake with the client
	if (!sendJSON(client->socket, makeHandshake())) {
		client->state = SyncFailed;
		return NULL;
	}
	cJSON *response = readJSON(client->socket);
	if (cJSON_HasObjectItem(response, "error")) {
		cJSON *error = cJSON_GetObjectItem(response, "error");
		logr(warning, "Client handshake error: %s\n", error->valuestring);
		client->state = SyncFailed;
		cJSON_Delete(response);
		return NULL;
	}
	cJSON_Delete(response);
	response = NULL;
	
	// Send assets
	logr(debug, "Sending assets...\n");
	cJSON *assets = cJSON_CreateObject();
	cJSON_AddStringToObject(assets, "action", "loadAssets");
	cJSON_AddItemToObject(assets, "files", cJSON_Parse(encodeFileCache()));
	sendJSON(client->socket, assets);
	response = readJSON(client->socket);
	if (!response) {
		client->state = SyncFailed;
		return NULL;
	}
	if (cJSON_HasObjectItem(response, "error")) {
		cJSON *error = cJSON_GetObjectItem(response, "error");
		logr(warning, "Client asset sync error: %s\n", error->valuestring);
		client->state = SyncFailed;
		cJSON_Delete(response);
		return NULL;
	}
	cJSON_Delete(response);
	
	// Send the scene
	logr(debug, "Sending scene data\n");
	cJSON *scene = cJSON_CreateObject();
	cJSON_AddStringToObject(scene, "action", "loadScene");
	cJSON *data = cJSON_Parse(params->renderer->sceneCache);
	cJSON_AddItemToObject(scene, "data", data);
	cJSON_AddStringToObject(scene, "assetPath", params->renderer->prefs.assetPath);
	sendJSON(client->socket, scene);
	response = readJSON(client->socket);
	if (!response) {
		client->state = SyncFailed;
		return NULL;
	}
	if (cJSON_HasObjectItem(response, "error")) {
		cJSON *error = cJSON_GetObjectItem(response, "error");
		logr(warning, "Client scene sync error: %s\n", error->valuestring);
		client->state = SyncFailed;
		cJSON_Delete(error);
		cJSON_Delete(response);
		disconnectFromClient(client);
		return NULL;
	}
	cJSON *action = cJSON_GetObjectItem(response, "action");
	if (cJSON_IsString(action)) {
		cJSON *threadCount = cJSON_GetObjectItem(response, "threadCount");
		if (cJSON_IsNumber(threadCount)) {
			client->availableThreads = threadCount->valueint;
		}
	}
	logr(debug, "Finished client %i sync. It reports %i threads available for rendering.\n", client->id, client->availableThreads);
	cJSON_Delete(response);
	
	// Sync successful, mark it as such
	client->state = Synced;
	return NULL;
}

void shutdownClients() {
	size_t clientCount = 0;
	struct renderClient *clients = buildClientList(&clientCount);
	logr(info, "Sending shutdown command to %zu clients.\n", clientCount);
	if (clientCount < 1) {
		logr(warning, "No clients found, exiting\n");
		return;
	}
	for (size_t i = 0; i < clientCount; ++i) {
		sendJSON(clients[i].socket, newAction("shutdown"));
		disconnectFromClient(&clients[i]);
	}
	logr(info, "Done, exiting.\n");
	free(clients);
}

struct renderClient *syncWithClients(const struct renderer *r, size_t *count) {
	logr(info, "Attempting to connect clients...\n");
	size_t clientCount = 0;
	struct renderClient *clients = buildClientList(&clientCount);
	if (clientCount < 1) {
		logr(warning, "No clients found, rendering solo.\n");
		return 0;
	}
	
	struct syncThreadParams *params = calloc(clientCount, sizeof(*params));
	logr(debug, "Client list:\n");
	for (size_t i = 0; i < clientCount; ++i) {
		logr(debug, "\tclient %zu: %s:%i\n", i, inet_ntoa(clients[i].address.sin_addr), htons(clients[i].address.sin_port));
		params[i].client = &clients[i];
		params[i].renderer = r;
	}
	
	struct crThread *syncThreads = calloc(clientCount, sizeof(*syncThreads));
	for (size_t i = 0; i < clientCount; ++i) {
		syncThreads[i] = (struct crThread){
			.threadFunc = handleClientSync,
			.userData = &params[i]
		};
	}
	
	for (size_t i = 0; i < clientCount; ++i) {
		if (threadStart(&syncThreads[i])) {
			logr(warning, "Something went wrong while starting the sync thread for client %i. May want to look into that.\n", (int)i);
		}
	}
	
	// Block here and wait for these threads to finish doing their thing before continuing.
	for (size_t i = 0; i < clientCount; ++i) {
		threadWait(&syncThreads[i]);
	}
	logr(info, "Client sync finished.\n");
	//FIXME: We should prune clients that dropped out during sync here
	if (count) *count = clientCount;
	free(syncThreads);
	return clients;
}

#else

void *networkRenderThread(void *arg) {
	return 0;
}

void shutdownClients() {
	logr(warning, "c-ray doesn't support the proprietary networking stack on Windows yet. Sorry!\n");
}

struct renderClient *syncWithClients(const struct renderer *r, size_t *count) {
	if (count) *count = 0;
	logr(warning, "c-ray doesn't support the proprietary networking stack on Windows yet. Sorry!\n");
	return NULL;
}

#endif
