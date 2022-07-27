//
//  server.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 06/04/2021.
//  Copyright © 2021-2022 Valtteri Koskivuori. All rights reserved.
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
#include <stdio.h>

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
#include "../platform/terminal.h"

void disconnectFromClient(struct renderClient *client) {
	ASSERT(client->socket != -1);
	shutdown(client->socket, SHUT_RDWR);
	close(client->socket);
	client->socket = -1;
	client->status = Disconnected;
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
	char *addr_string = firstToken(line);
	struct hostent *ent = gethostbyname(addr_string);
	memcpy(&address.sin_addr.s_addr, ent->h_addr, ent->h_length);
	address.sin_port = line->amountOf.tokens > 1 ? htons(atoi(lastToken(line))) : htons(2222);
	destroyLineBuffer(line);
	return address;
}

static bool connectToClient(struct renderClient *client) {
	client->socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client->socket == -1) {
		logr(warning, "Failed to bind to socket on client %i\n", client->id);
		client->status = ConnectionFailed;
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
			client->status = Connected;
			// Unset non-blocking
			int oldFlags = fcntl(client->socket, F_GETFL);
			fcntl(client->socket, F_SETFL, oldFlags & ~O_NONBLOCK);
		} else {
			logr(debug, "%s on %s:%i, dropping.\n", strerror(so_error), inet_ntoa(client->address.sin_addr), htons(client->address.sin_port));
			close(client->socket);
			client->status = ConnectionFailed;
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
		clients[i].status = connectToClient(&clients[i]) ? Connected : ConnectionFailed;
		current = nextToken(line);
	}
	size_t validClients = 0;
	for (size_t i = 0; i < clientCount; ++i) {
		validClients += clients[i].status == ConnectionFailed ? 0 : 1;
	}
	if (validClients < clientCount) {
		// Prune unavailable clients
		struct renderClient *confirmedClients = calloc(validClients, sizeof(*confirmedClients));
		size_t j = 0;
		for (size_t i = 0; i < clientCount; ++i) {
			if (clients[i].status != ConnectionFailed) {
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
	struct renderTile *tile = nextTile(state->renderer);
	if (!tile) return newAction("renderComplete");
	tile->networkRenderer = true;
	cJSON *response = newAction("newWork");
	cJSON_AddItemToObject(response, "tile", encodeTile(tile));
	return response;
}

static cJSON *processSubmitWork(struct renderThreadState *state, const cJSON *json) {
	cJSON *resultJson = cJSON_GetObjectItem(json, "result");
	struct texture *tileImage = decodeTexture(resultJson);
	cJSON *tileJson = cJSON_GetObjectItem(json, "tile");
	struct renderTile tile = decodeTile(tileJson);
	state->renderer->state.renderTiles[tile.tileNum].state = finished;
	for (int y = tile.end.y - 1; y > tile.begin.y - 1; --y) {
		for (int x = tile.begin.x; x < tile.end.x; ++x) {
			struct color value = textureGetPixel(tileImage, x - tile.begin.x, y - tile.begin.y, false);
			setPixel(state->output, value, x, y);
		}
	}
	destroyTexture(tileImage);
	return newAction("ok");
}

struct command serverCommands[] = {
	{"getWork", 0},
	{"submitWork", 1},
	{"goodbye", 2},
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
		case 2:
			state->threadComplete = true;
			logr(debug, "Client %i said goodbye, disconnecting.\n", state->client->id);
			return goodbye();
			break;
		default:
			logr(debug, "Unknown command: %s\n", cJSON_PrintUnformatted(json));
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
	struct renderClient *client = state->client;
	if (!client) {
		state->threadComplete = true;
		return 0;
	}
	if (client->status != Synced) {
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
	while (r->state.isRendering && !state->threadComplete) {
		cJSON *request = readJSON(client->socket);
		if (containsStats(request)) {
			cJSON *completed = cJSON_GetObjectItem(request, "completed");
			if (cJSON_IsNumber(completed)) state->completedSamples = completed->valueint;
			cJSON *avg = cJSON_GetObjectItem(request, "avgPerPass");
			if (cJSON_IsNumber(avg)) state->avgSampleTime = avg->valuedouble;
		} else {
			cJSON *response = processClientRequest(state, request);
			if (containsError(response)) {
				char *err = cJSON_PrintUnformatted(response);
				logr(debug, "error, exiting thread %i: %s\n", state->thread_num, err);
				free(err);
				sendJSON(client->socket, response);
				break;
			}
			sendJSON(client->socket, response);
		}
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
	const char *assetCache;
	size_t progress;
	bool done;
};

//TODO: Rename to clientSyncThread
static void *handleClientSync(void *arg) {
	struct syncThreadParams *params = (struct syncThreadParams *)threadUserData(arg);
	struct renderClient *client = params->client;
	if (client->status != Connected) {
		logr(warning, "Won't sync with client %i, no connection.\n", client->id);
		return NULL;
	}
	client->status = Syncing;
	
	// Handshake with the client
	if (!sendJSON(client->socket, makeHandshake())) {
		client->status = SyncFailed;
		return NULL;
	}
	cJSON *response = readJSON(client->socket);
	if (cJSON_HasObjectItem(response, "error")) {
		cJSON *error = cJSON_GetObjectItem(response, "error");
		logr(warning, "Client handshake error: %s\n", error->valuestring);
		client->status = SyncFailed;
		cJSON_Delete(response);
		return NULL;
	}
	cJSON_Delete(response);
	response = NULL;
	
	// Send the scene & assets
	cJSON *scene = cJSON_CreateObject();
	cJSON_AddStringToObject(scene, "action", "loadScene");
	logr(debug, "Syncing state: %s\n", params->renderer->sceneCache);
	cJSON *data = cJSON_Parse(params->renderer->sceneCache);
	cJSON_AddItemToObject(scene, "data", data);
	cJSON_AddItemToObject(scene, "files", cJSON_Parse(params->assetCache));
	cJSON_AddStringToObject(scene, "assetPath", params->renderer->prefs.assetPath);
	sendJSONWithProgress(client->socket, scene, &params->progress);
	response = readJSON(client->socket);
	if (!response) {
		logr(debug, "no response\n");
		client->status = SyncFailed;
		return NULL;
	}
	if (cJSON_HasObjectItem(response, "error")) {
		cJSON *error = cJSON_GetObjectItem(response, "error");
		logr(warning, "Client scene sync error: %s\n", error->valuestring);
		client->status = SyncFailed;
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
	client->status = Synced;
	params->done = true;
	return NULL;
}

void shutdownClients() {
	size_t clientCount = 0;
	struct renderClient *clients = buildClientList(&clientCount);
	logr(info, "Sending shutdown command to %zu client%s.\n", clientCount, PLURAL(clientCount));
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

#define BAR_LENGTH 32
void printBar(struct syncThreadParams *param) {
	size_t chars = param->progress * BAR_LENGTH / 100;
	logr(info, "Client %i: [", param->client->id);
	for (size_t i = 0; i < chars; ++i) {
		printf("-");
	}
	for (size_t i = 0; i < BAR_LENGTH - chars; ++i) {
		printf(" ");
	}
	if (param->progress < 100) {
		printf("] (%3zu%%)\n", param->progress);
	} else {
		printf("] (Client finishing up)\n");
	}
}

void printProgressBars(struct syncThreadParams *params, size_t clientCount) {
	if (!isTeleType()) return;
	
	for (size_t i = 0; i < clientCount; ++i) {
		printBar(&params[i]);
	}
}

struct renderClient *syncWithClients(const struct renderer *r, size_t *count) {
	signal(SIGPIPE, SIG_IGN);
	size_t clientCount = 0;
	struct renderClient *clients = buildClientList(&clientCount);
	if (clientCount < 1) {
		logr(warning, "No clients found, rendering solo.\n");
		if (count) *count = 0;
		return NULL;
	}
	
	char *assetCache = cache_encode(r->state.file_cache);
	
	size_t transfer_bytes = strlen(assetCache) + strlen(r->sceneCache);
	char *transfer_size = humanFileSize(transfer_bytes);
	logr(info, "Sending %s to %lu client%s...\n", transfer_size, clientCount, PLURAL(clientCount));
	free(transfer_size);
	
	struct syncThreadParams *params = calloc(clientCount, sizeof(*params));
	logr(debug, "Client list:\n");
	for (size_t i = 0; i < clientCount; ++i) {
		logr(debug, "\tclient %zu: %s:%i\n", i, inet_ntoa(clients[i].address.sin_addr), htons(clients[i].address.sin_port));
		params[i].client = &clients[i];
		params[i].renderer = r;
		params[i].assetCache = assetCache;
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
	
	size_t loops = 0;
	while (true) {
		bool all_stopped = true;
		for (size_t i = 0; i < clientCount; ++i) {
			if (!params[i].done) all_stopped = false;
		}
		if (all_stopped) break;
		timer_sleep_ms(10);
		if (++loops == 10) {
			loops = 0;
			printProgressBars(params, clientCount);
			printf("\033[%zuF", clientCount);
		}
	}
	
	// Block here and verify threads are done before continuing.
	for (size_t i = 0; i < clientCount; ++i) {
		threadWait(&syncThreads[i]);
	}
	
	free(assetCache);
	for (size_t i = 0; i < clientCount; ++i) printf("\n");
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
