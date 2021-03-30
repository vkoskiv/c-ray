//
//  protocol.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 21/03/2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "protocol.h"

//Windows is annoying, so it's just not going to have networking. Because it is annoying and proprietary.
#include "../utils/logging.h"
#ifndef WINDOWS

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <string.h>
#include "assert.h"
#include "../libraries/cJSON.h"
#include "platform/thread.h"
#include "args.h"
#include "textbuffer.h"
#include "string.h"
#include "gitsha1.h"
#include "../datatypes/image/imagefile.h"
#include "../renderer/renderer.h"
#include <errno.h>
#include "networking.h"
#include "../datatypes/vector.h"
#include "../datatypes/vertexbuffer.h"
#include "base64.h"
#include "../datatypes/scene.h"
#include "filecache.h"

#define C_RAY_HEADERSIZE 8
#define C_RAY_CHUNKSIZE 1024
#define C_RAY_PORT 2222
#define PROTO_VERSION "0.1"

struct renderer *g_worker_renderer = NULL;

enum clientState {
	Disconnected,
	Connected,
	ConnectionFailed,
	Syncing,
	SyncFailed,
	Synced,
	Rendering,
	Finished
};

struct renderClient {
	struct sockaddr_in address;
	enum clientState state;
	int socket;
	int id;
};

// Consumes given json, no need to free it after.
void sendJSON(struct renderClient *client, cJSON *json) {
	ASSERT(client);
	ASSERT(json);
	char *jsonText = cJSON_PrintUnformatted(json);
	cJSON_Delete(json);
	chunkedSend(client->socket, jsonText);
	free(jsonText);
}

cJSON *readJSON(struct renderClient *client) {
	ASSERT(client);
	char *recvBuf = NULL;
	chunkedReceive(client->socket, &recvBuf);
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

struct { char *name; int id; } workerCommands[] = {
	{"handshake", 0},
	{"loadScene", 1},
	{"loadAssets", 2}
};

int matchWorkerCommand(char *cmd) {
	size_t commandCount = sizeof(workerCommands) / sizeof(struct {char *name; int id;});
	for (size_t i = 0; i < commandCount; ++i) {
		if (stringEquals(workerCommands[i].name, cmd)) return workerCommands[i].id;
	}
	return -1;
}

cJSON *newAction(char *action) {
	if (!action) return NULL;
	cJSON *actionJson = cJSON_CreateObject();
	cJSON_AddStringToObject(actionJson, "action", action);
	return actionJson;
}

cJSON *validateHandshake(const cJSON *in) {
	const cJSON *version = cJSON_GetObjectItem(in, "version");
	const cJSON *githash = cJSON_GetObjectItem(in, "githash");
	if (!stringEquals(version->valuestring, PROTO_VERSION)) return errorResponse("Protocol version mismatch");
	if (!stringEquals(githash->valuestring, gitHash())) return errorResponse("Git hash mismatch");
	return newAction("startSync");
}

cJSON *receiveScene(const cJSON *json) {
	cJSON *scene = cJSON_GetObjectItem(json, "data");
	char *sceneText = cJSON_PrintUnformatted(scene);
	g_worker_renderer = newRenderer();
	//FIXME: HACK
	g_worker_renderer->prefs.assetPath = stringCopy("input/");
	if (loadScene(g_worker_renderer, sceneText)) {
		return errorResponse("Scene parsing error");
	}
	free(sceneText);
	return newAction("ok");
}

cJSON *receiveAssets(const cJSON *json) {
	cJSON *files = cJSON_GetObjectItem(json, "files");
	char *data = cJSON_PrintUnformatted(files);
	decodeFileCache(data);
	free(data);
	return newAction("ok");
}

// Worker command handler
cJSON *processCommand(struct renderClient *client, const cJSON *json) {
	(void)client;
	if (!json) {
		return errorResponse("Couldn't parse incoming JSON");
	}
	const cJSON *action = cJSON_GetObjectItem(json, "action");
	if (!cJSON_IsString(action)) {
		return errorResponse("No action provided");
	}
	
	switch (matchWorkerCommand(action->valuestring)) {
		case 0:
			return validateHandshake(json);
			break;
		case 1:
			return receiveScene(json);
			break;
		case 2:
			return receiveAssets(json);
			break;
		default:
			return errorResponse("Unknown command");
			break;
	}
	
	return goodbye();
	ASSERT_NOT_REACHED();
	return NULL;
}

cJSON *makeHandshake() {
	cJSON *handshake = cJSON_CreateObject();
	cJSON_AddStringToObject(handshake, "action", "handshake");
	cJSON_AddStringToObject(handshake, "version", PROTO_VERSION);
	cJSON_AddStringToObject(handshake, "githash", gitHash());
	return handshake;
}

struct sockaddr_in parseAddress(const char *str) {
	lineBuffer *line = newLineBuffer();
	fillLineBuffer(line, str, ':');
	struct sockaddr_in address = {0};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(firstToken(line));
	address.sin_port = line->amountOf.tokens > 1 ? htons(atoi(lastToken(line))) : htons(2222);
	destroyLineBuffer(line);
	return address;
}

bool checkConnectivity(struct renderClient client) {
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		logr(error, "Failed to bind to socket while testing connectivity\n");
	}
	bool success = false;
	fcntl(sockfd, F_SETFL, O_NONBLOCK);
	fd_set fdset;
	struct timeval tv;
	connect(sockfd, (struct sockaddr *)&client.address, sizeof(client.address));
	FD_ZERO(&fdset);
	FD_SET(sockfd, &fdset);
	tv.tv_sec = 1; // 1 second timeout.
	tv.tv_usec = 0;
	
	if (select(sockfd + 1, NULL, &fdset, NULL, &tv) == 1) {
		int so_error ;
		socklen_t len = sizeof(so_error);
		getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
		if (so_error == 0) {
			logr(debug, "Connected to %s:%i\n", inet_ntoa(client.address.sin_addr), htons(client.address.sin_port));
			success = true;
		} else {
			logr(debug, "%s on %s:%i, dropping.\n", strerror(so_error), inet_ntoa(client.address.sin_addr), htons(client.address.sin_port));
		}
	}
	shutdown(sockfd, SHUT_RDWR);
	close(sockfd);
	return success;
}

// Fetches list of nodes from arguments, verifies that they are reachable, and
// returns them in a nice list. Also got the size there in the amount param, if you need it.
struct renderClient *buildClientList(size_t *amount) {
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
		//clients[i].state = checkConnectivity(clients[i]) ? Connected : ConnectionFailed;
		clients[i].state = Connected;
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
		clients[i].socket = -1;
		clients[i].id = (int)i;
		clients[i].state = Disconnected;
	}
	
	if (amount) *amount = validClients;
	destroyLineBuffer(line);
	return clients;
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

bool connectToClient(struct renderClient *client) {
	ASSERT(client->socket == -1);
	client->socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client->socket == -1) {
		logr(warning, "Failed to bind to socket on client %i\n", client->id);
		client->state = ConnectionFailed;
		return false;
	}
	
	if (connect(client->socket, (struct sockaddr *)&client->address, sizeof(client->address)) != 0) {
		logr(warning, "Failed to connect to %i\n", client->id);
		client->state = ConnectionFailed;
		return false;
	}
	client->state = Connected;
	return true;
}

void workerCleanup() {
	//ASSERT_NOT_REACHED();
	destroyRenderer(g_worker_renderer);
	destroyFileCache();
}

int startWorkerServer() {
	int receivingSocket, connectionSocket;
	struct sockaddr_in ownAddress;
	receivingSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (receivingSocket == -1) {
		logr(error, "Socket creation failed.\n");
	}
	
	bzero(&ownAddress, sizeof(ownAddress));
	ownAddress.sin_family = AF_INET;
	ownAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	ownAddress.sin_port = htons(C_RAY_PORT);
	
	int opt_val = 1;
	setsockopt(receivingSocket, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));
	
	if ((bind(receivingSocket, (struct sockaddr *)&ownAddress, sizeof(ownAddress))) != 0) {
		logr(error, "Failed to bind to socket\n");
	}
	
	if (listen(receivingSocket, 1) != 0) {
		logr(error, "It wouldn't listen\n");
	}
	
	struct sockaddr_in masterAddress;
	socklen_t len = sizeof(masterAddress);
	char *buf = NULL;
	
	// TODO: Should put this in a loop too with a cleanup,
	// so we can just leave render nodes on all the time, waiting for render tasks.
	while (1) {
		logr(info, "Listening for connections on port %i\n", C_RAY_PORT);
		connectionSocket = accept(receivingSocket, (struct sockaddr *)&masterAddress, &len);
		if (connectionSocket < 0) {
			logr(error, "Failed to accept\n");
		}
		logr(debug, "Got connection from %s\n", inet_ntoa(masterAddress.sin_addr));
		
		for (;;) {
			buf = NULL;
			ssize_t read = chunkedReceive(connectionSocket, &buf);
			if (read == 0) break;
			if (read < 0) {
				logr(warning, "Something went wrong. Error: %s\n", strerror(errno));
				close(connectionSocket);
				break;
			}
			cJSON *message = cJSON_Parse(buf);
			cJSON *myResponse = processCommand(NULL, message);
			char *responseText = cJSON_PrintUnformatted(myResponse);
			logr(debug, "Responding     : %s\n", responseText);
			if (chunkedSend(connectionSocket, responseText)) {
				logr(warning, "chunkedSend() failed, error %s\n", strerror(errno));
				close(connectionSocket);
				break;
			};
			free(responseText);
			if (buf) free(buf);
			if (containsGoodbye(myResponse) || containsError(myResponse)) {
				close(connectionSocket);
				break;
			}
			cJSON_Delete(myResponse);
			cJSON_Delete(message);
			buf = NULL;
		}
		close(connectionSocket);
		workerCleanup(); // Prepare for next render
	}
	free(buf);
	close(receivingSocket);
	return 0;
}

struct syncThreadParams {
	struct renderClient *client;
	const struct renderer *renderer;
};

void *handleClientSync(void *arg) {
	struct syncThreadParams *params = (struct syncThreadParams *)threadUserData(arg);
	struct renderClient *client = params->client;
	connectToClient(client);
	if (client->state != Connected) {
		logr(warning, "Won't sync with client %i, no connection.\n", client->id);
		return NULL;
	}
	client->state = Syncing;
	
	// Handshake with the client
	sendJSON(client, makeHandshake());
	cJSON *response = readJSON(client);
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
	sendJSON(client, assets);
	response = readJSON(client);
	char *responseText = cJSON_PrintUnformatted(response);
	logr(debug, "Response: %s\n", responseText);
	free(responseText);
	responseText = NULL;
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
	sendJSON(client, scene);
	response = readJSON(client);
	responseText = cJSON_PrintUnformatted(response);
	logr(debug, "Response: %s\n", responseText);
	free(responseText);
	responseText = NULL;
	if (cJSON_HasObjectItem(response, "error")) {
		cJSON *error = cJSON_GetObjectItem(response, "error");
		logr(warning, "Client scene sync error: %s\n", error->valuestring);
		client->state = SyncFailed;
		cJSON_Delete(error);
		cJSON_Delete(response);
		return NULL;
	}
	cJSON_Delete(response);
	
	// Sync successful, mark it as such
	client->state = Synced;
	return NULL;
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
int startMasterServer() {
	logr(error, "c-ray doesn't support the proprietary networking stack on Windows. Sorry!\n");
}
int startWorkerServer() {
	logr(error, "c-ray doesn't support the proprietary networking stack on Windows. Sorry!\n");
}
#endif
