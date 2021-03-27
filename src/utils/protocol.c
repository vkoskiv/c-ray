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
#include "../utils/assert.h"
#include "../libraries/cJSON.h"
#include "../utils/platform/thread.h"
#include "../utils/args.h"
#include "../utils/textbuffer.h"
#include "../utils/string.h"
#include "../utils/gitsha1.h"
#include "../datatypes/image/imagefile.h"
#include "../renderer/renderer.h"
#include <errno.h>
#include "networking.h"
#include "../datatypes/vector.h"
#include "../datatypes/vertexbuffer.h"
#include "../utils/base64.h"
#include "../datatypes/scene.h"

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

//FIXME: Non-portable
cJSON *encodeScene(struct world *sceneIn) {
	cJSON *scene = cJSON_CreateObject();
	char *data = b64encode(sceneIn, sizeof(struct world));
	cJSON_AddStringToObject(scene, "data", data);
	free(data);
	return scene;
}

//FIXME: Non-portable
cJSON *encodeState(struct state stateIn) {
	cJSON *state = cJSON_CreateObject();
	char *data = b64encode(&stateIn, sizeof(struct state));
	cJSON_AddStringToObject(state, "data", data);
	free(data);
	return state;
}

//FIXME: Non-portable
cJSON *encodePrefs(struct prefs prefsIn) {
	cJSON *prefs = cJSON_CreateObject();
	char *data = b64encode(&prefsIn, sizeof(struct prefs));
	cJSON_AddStringToObject(prefs, "data", data);
	free(data);
	return prefs;
}

cJSON *encodeRenderer(const struct renderer *r) {
	cJSON *renderer = cJSON_CreateObject();
	
	cJSON_AddStringToObject(renderer, "action", "syncRenderer");
	cJSON_AddItemToObject(renderer, "scene", encodeScene(r->scene));
	cJSON_AddItemToObject(renderer, "state", encodeState(r->state));
	cJSON_AddItemToObject(renderer, "prefs", encodePrefs(r->prefs));
	
	return renderer;
}

struct renderer *decodeRenderer(const cJSON *json) {
	struct renderer *r = calloc(1, sizeof(*r));
	
	cJSON *scene = cJSON_GetObjectItem(json, "scene");
	cJSON *state = cJSON_GetObjectItem(json, "state");
	cJSON *prefs = cJSON_GetObjectItem(json, "prefs");
	
	cJSON *sceneData = cJSON_GetObjectItem(scene, "data");
	cJSON *stateData = cJSON_GetObjectItem(state, "data");
	cJSON *prefsData = cJSON_GetObjectItem(prefs, "data");
	
	r->scene = (struct world *)b64decode(sceneData->valuestring, strlen(sceneData->valuestring));
	r->state = *(struct state *)b64decode(stateData->valuestring, strlen(stateData->valuestring));
	r->prefs = *(struct prefs *)b64decode(prefsData->valuestring, strlen(prefsData->valuestring));
	
	r->prefs.isWorker = true;
	
	return r;
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
	{"syncVertices", 1},
	{"syncRenderer", 2},
};

int matchWorkerCommand(char *cmd) {
	size_t commandCount = sizeof(workerCommands) / sizeof(struct {char *name; int id;});
	ASSERT(commandCount == 3);
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

cJSON *receiveVertices(const cJSON *json) {
	cJSON *vertices = cJSON_GetObjectItem(json, "vertices");
	if (!vertices) return errorResponse("No vertices");
	cJSON *vcount = cJSON_GetObjectItem(json, "vertices_count");
	if (!vcount) return errorResponse("No vertices_count");
	
	cJSON *normals = cJSON_GetObjectItem(json, "normals");
	if (!normals) return errorResponse("No normals");
	cJSON *ncount = cJSON_GetObjectItem(json, "normals_count");
	if (!ncount) return errorResponse("No normals_count");
	
	cJSON *textureCoords = cJSON_GetObjectItem(json, "tex");
	if (!textureCoords) return errorResponse("No tex");
	cJSON *tcount = cJSON_GetObjectItem(json, "tex_count");
	if (!tcount) return errorResponse("No tex_count");

	g_vertices = (struct vector *)b64decode(vertices->valuestring, strlen(vertices->valuestring));
	vertexCount = vcount->valueint;
	g_normals = (struct vector *)b64decode(normals->valuestring, strlen(normals->valuestring));
	normalCount = ncount->valueint;
	g_textureCoords = (struct coord *)b64decode(textureCoords->valuestring, strlen(textureCoords->valuestring));
	
	return newAction("ok");
}

cJSON *receiveRenderer(const cJSON *json) {
	g_worker_renderer = decodeRenderer(json);
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
			return receiveVertices(json);
			break;
		case 2:
			return receiveRenderer(json);
			break;
			
		default:
			return errorResponse("Unknown command");
			break;
	}
	
	return goodbye();
	ASSERT_NOT_REACHED();
	return NULL;
}

// Worker response handler
//TODO: Delete
cJSON *processWorkerResponse(struct renderClient *client, const cJSON *json) {
	(void)client;
	if (!json) {
		return errorResponse("Couldn't parse incoming JSON");
	}
	const cJSON *action = cJSON_GetObjectItem(json, "action");
	if (!cJSON_IsString(action)) {
		return errorResponse("No action provided");
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

void *clientHandler(void *arg) {
	struct renderClient *client = (struct renderClient *)threadUserData(arg);
	
	client->socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client->socket == -1) {
		logr(warning, "Failed to bind to socket on client %i\n", client->id);
		return NULL;
	}
	if (connect(client->socket, (struct sockaddr *)&client->address, sizeof(client->address)) != 0) {
		logr(warning, "Connection failed on client %i\n", client->id);
		return NULL;
	}
	
	char *receiveBuffer = NULL;
	
	cJSON *handshake = makeHandshake();
	char *content = cJSON_PrintUnformatted(handshake);
	if (chunkedSend(client->socket, content)) {
		logr(error, "chunkedSend() failed, error: %s\n", strerror(errno));
	}
	free(content);
	cJSON_Delete(handshake);
	
	for (;;) {
		//ssize_t ret = recv(sockfd, receiveBuffer, MAXRCVLEN, 0);
		ssize_t ret = chunkedReceive(client->socket, &receiveBuffer);
		if (!ret) {
			logr(info, "Client %i received nothing.\n", client->id);
			break;
		}
		cJSON *clientResponse = cJSON_Parse(receiveBuffer);
		cJSON *serverResponse = processWorkerResponse(client, clientResponse);
		char *responseText = cJSON_PrintUnformatted(serverResponse);
		//write(sockfd, responseText, strlen(responseText));
		if (chunkedSend(client->socket, responseText)) {
			logr(error, "chunkedSend() failed, error: %s\n", strerror(errno));
		}
		free(responseText);
		free(receiveBuffer);
		cJSON_Delete(serverResponse);
		cJSON_Delete(clientResponse);
		receiveBuffer = NULL;
		if (containsGoodbye(serverResponse)) {
			logr(debug, "Client %i said goodbye, exiting handler thread.\n", client->id);
			break;
		}
	}
	
	close(client->socket);
	client->socket = 0;
	
	return NULL;
}

// Start off with just a single node
int startMasterServer() {
	
	logr(info, "Attempting to connect clients...\n");
	size_t clientCount = 0;
	struct renderClient *clients = buildClientList(&clientCount);
	if (clientCount < 1) {
		logr(warning, "No clients found, rendering solo.\n");
		return 0;
	}
	logr(debug, "Client list:\n");
	for (size_t i = 0; i < clientCount; ++i) {
		logr(debug, "\tclient %zu: %s:%i\n", i, inet_ntoa(clients[i].address.sin_addr), htons(clients[i].address.sin_port));
	}
	
	struct crThread *clientThreads = calloc(clientCount, sizeof(*clientThreads));
	for (size_t i = 0; i < clientCount; ++i) {
		clientThreads[i] = (struct crThread){
			.threadFunc = clientHandler,
			.userData = &clients[i]
		};
	}
	
	for (size_t i = 0; i < clientCount; ++i) {
		if (threadStart(&clientThreads[i])) {
			logr(warning, "Something went wrong while starting the connection thread for client %i. May want to look into that.\n", (int)i);
		}
	}
	
	// Block here and wait for these threads to finish doing their thing before continuing.
	for (size_t i = 0; i < clientCount; ++i) {
		threadWait(&clientThreads[i]);
	}
	logr(info, "All clients are finished.\n");
	free(clientThreads);
	free(clients);
	return 0;
}

void workerCleanup() {
	//ASSERT_NOT_REACHED();
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
		logr(debug, "Listening for connections on port %i\n", C_RAY_PORT);
		connectionSocket = accept(receivingSocket, (struct sockaddr *)&masterAddress, &len);
		if (connectionSocket < 0) {
			logr(error, "Failed to accept\n");
		}
		logr(debug, "Got connection from %s\n", inet_ntoa(masterAddress.sin_addr));
		
		for (;;) {
			//ssize_t read = recv(connectionSocket, buf, MAXRCVLEN, 0);
			ssize_t read = chunkedReceive(connectionSocket, &buf);
			if (read < 0) {
				logr(warning, "Something went wrong. Error: %s\n", strerror(errno));
				close(connectionSocket);
				break;
			}
			
			logr(debug, "Got from master: %s\n", buf);
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
			free(buf);
			if (containsGoodbye(myResponse)) {
				close(connectionSocket);
				free(buf);
				break;
			}
			cJSON_Delete(myResponse);
			cJSON_Delete(message);
			buf = NULL;
		}
		workerCleanup(); // Prepare for next render
	}
	free(buf);
	close(receivingSocket);
	return 0;
}

//FIXME: Non-portable.
cJSON *encodeVertexBuffers() {
	cJSON *payload = cJSON_CreateObject();
	cJSON_AddStringToObject(payload, "action", "syncVertices");
	char *vertices = b64encode(g_vertices, vertexCount * sizeof(struct vector));
	logr(debug, "vertices: %s\n", vertices);
	char *normals = b64encode(g_normals, normalCount * sizeof(struct vector));
	logr(debug, "normals: %s\n", normals);
	char *textureCoords = b64encode(g_textureCoords, textureCount * sizeof(struct coord));
	logr(debug, "textureCoords: %s\n", textureCoords);
	cJSON_AddStringToObject(payload, "vertices", vertices);
	cJSON_AddNumberToObject(payload, "vertices_count", vertexCount);
	cJSON_AddStringToObject(payload, "normals", normals);
	cJSON_AddNumberToObject(payload, "normals_count", normalCount);
	cJSON_AddStringToObject(payload, "tex", textureCoords);
	cJSON_AddNumberToObject(payload, "tex_count", textureCount);
	free(vertices);
	free(normals);
	free(textureCoords);
	char *test = cJSON_PrintUnformatted(payload);
	logr(debug, "Encoded: %s\n", test);
	free(test);
	return payload;
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
		cJSON_Delete(error);
		cJSON_Delete(response);
		return NULL;
	}
	cJSON_Delete(response);
	response = NULL;
	logr(debug, "Sending vertex buffers\n");
	sendJSON(client, encodeVertexBuffers());
	response = readJSON(client);
	char *responseText = cJSON_PrintUnformatted(response);
	logr(debug, "Response: %s\n", responseText);
	free(responseText);
	responseText = NULL;
	if (cJSON_HasObjectItem(response, "error")) {
		cJSON *error = cJSON_GetObjectItem(response, "error");
		logr(warning, "Client vertex sync error: %s\n", error->valuestring);
		client->state = SyncFailed;
		cJSON_Delete(error);
		cJSON_Delete(response);
		return NULL;
	}
	cJSON_Delete(response);
	
	logr(debug, "Sending renderer state\n");
	sendJSON(client, encodeRenderer(params->renderer));
	response = readJSON(client);
	responseText = cJSON_PrintUnformatted(response);
	logr(debug, "Response: %s\n", responseText);
	if (cJSON_HasObjectItem(response, "error")) {
		cJSON *error = cJSON_GetObjectItem(response, "error");
		logr(warning, "Client state sync error: %s\n", error->valuestring);
		client->state = SyncFailed;
		cJSON_Delete(error);
		cJSON_Delete(response);
		return NULL;
	}
	free(responseText);
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
