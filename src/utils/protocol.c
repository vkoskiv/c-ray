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
#include "../datatypes/tile.h"
#include "../datatypes/image/texture.h"
#include "../datatypes/color.h"
#include "../utils/platform/mutex.h"
#include "../utils/timer.h"

#define C_RAY_HEADERSIZE 8
#define C_RAY_CHUNKSIZE 1024
#define C_RAY_DEFAULT_PORT 2222
#define PROTO_VERSION "0.1"

struct renderer *g_worker_renderer = NULL;
struct crMutex *g_worker_socket_mutex = NULL;

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
	int availableThreads;
	int socket;
	int id;
};

// Consumes given json, no need to free it after.
static void sendJSON(int socket, cJSON *json) {
	ASSERT(json);
	char *jsonText = cJSON_PrintUnformatted(json);
	cJSON_Delete(json);
	chunkedSend(socket, jsonText);
	free(jsonText);
}

static cJSON *readJSON(int socket) {
	char *recvBuf = NULL;
	chunkedReceive(socket, &recvBuf);
	cJSON *received = cJSON_Parse(recvBuf);
	free(recvBuf);
	return received;
}

static cJSON *errorResponse(char *error) {
	cJSON *errorMsg = cJSON_CreateObject();
	cJSON_AddStringToObject(errorMsg, "error", error);
	return errorMsg;
}

static cJSON *goodbye() {
	cJSON *goodbye = cJSON_CreateObject();
	cJSON_AddStringToObject(goodbye, "action", "goodbye");
	return goodbye;
}

static cJSON *newAction(char *action) {
	if (!action) return NULL;
	cJSON *actionJson = cJSON_CreateObject();
	cJSON_AddStringToObject(actionJson, "action", action);
	return actionJson;
}

static cJSON *validateHandshake(const cJSON *in) {
	const cJSON *version = cJSON_GetObjectItem(in, "version");
	const cJSON *githash = cJSON_GetObjectItem(in, "githash");
	if (!stringEquals(version->valuestring, PROTO_VERSION)) return errorResponse("Protocol version mismatch");
	if (!stringEquals(githash->valuestring, gitHash())) return errorResponse("Git hash mismatch");
	return newAction("startSync");
}

static cJSON *receiveScene(const cJSON *json) {
	cJSON *scene = cJSON_GetObjectItem(json, "data");
	char *sceneText = cJSON_PrintUnformatted(scene);
	g_worker_renderer = newRenderer();
	g_worker_socket_mutex = createMutex();
	cJSON *assetPathJson = cJSON_GetObjectItem(json, "assetPath");
	g_worker_renderer->prefs.assetPath = stringCopy(assetPathJson->valuestring);
	if (loadScene(g_worker_renderer, sceneText)) {
		return errorResponse("Scene parsing error");
	}
	free(sceneText);
	cJSON *resp = newAction("ready");
	
	// Stash in our capabilities here
	//TODO: Maybe some performance value in here, so the master knows how much work to assign?
	// For now just report back how many threads we've got available.
	cJSON_AddNumberToObject(resp, "threadCount", g_worker_renderer->prefs.threadCount);
	logr(info, "Starting network render job\n");
	return resp;
}

static cJSON *receiveAssets(const cJSON *json) {
	cJSON *files = cJSON_GetObjectItem(json, "files");
	char *data = cJSON_PrintUnformatted(files);
	decodeFileCache(data);
	free(data);
	return newAction("ok");
}

struct command {
	char *name;
	int id;
};

static cJSON *encodeTile(struct renderTile tile) {
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

static struct renderTile decodeTile(const cJSON *json) {
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

static cJSON *encodeTexture(const struct texture *t) {
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

static struct texture *decodeTexture(const cJSON *json) {
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

struct workerThreadState {
	int thread_num;
	int connectionSocket;
	struct crMutex *socketMutex;
	struct renderer *renderer;
	bool threadComplete;
};

static struct renderTile getWork(int connectionSocket) {
	sendJSON(connectionSocket, newAction("getWork"));
	cJSON *response = readJSON(connectionSocket);
	cJSON *error = cJSON_GetObjectItem(response, "error");
	if (cJSON_IsString(error)) {
		if (stringEquals(error->valuestring, "renderComplete")) {
			struct renderTile tile = {0};
			tile.tileNum = -1;
			return tile;
		}
	}
	if (!response) {
		struct renderTile tile = {0};
		tile.tileNum = -1;
		return tile;
	}
	cJSON *tileJson = cJSON_GetObjectItem(response, "tile");
	struct renderTile tile = decodeTile(tileJson);
	cJSON_Delete(response);
	return tile;
}

static void submitWork(int sock, struct texture *work, struct renderTile forTile) {
	cJSON *result = encodeTexture(work);
	cJSON *tile = encodeTile(forTile);
	cJSON *package = newAction("submitWork");
	cJSON_AddItemToObject(package, "result", result);
	cJSON_AddItemToObject(package, "tile", tile);
	sendJSON(sock, package);
}

static void *workerThread(void *arg) {
	struct workerThreadState *threadState = (struct workerThreadState *)threadUserData(arg);
	struct renderer *r = threadState->renderer;
	int sock = threadState->connectionSocket;
	struct crMutex *sockMutex = threadState->socketMutex;
	
	lockMutex(sockMutex);
	struct renderTile tile = getWork(sock);
	releaseMutex(sockMutex);
	
	while (tile.tileNum != -1 && r->state.isRendering) {
		struct texture *thing = renderSingleTile(r, tile);
		
		lockMutex(sockMutex);
		submitWork(sock, thing, tile);
		cJSON *resp = readJSON(sock);
		if (!stringEquals(cJSON_GetObjectItem(resp, "action")->valuestring, "ok")) {
			threadState->threadComplete = true;
			return 0;
		}
		releaseMutex(sockMutex);
		
		lockMutex(sockMutex);
		tile = getWork(sock);
		releaseMutex(sockMutex);
	}
	
	threadState->threadComplete = true;
	return 0;
}

#define active_msec  16

static cJSON *startRender(int connectionSocket, const cJSON *json) {
	g_worker_renderer->state.isRendering = true;
	g_worker_renderer->state.renderAborted = false;
	g_worker_renderer->state.saveImage = false;
	logr(info, "Starting network render server\n");
	
	int threadCount = g_worker_renderer->prefs.threadCount;
	// Map of threads that have finished, so we don't check them again.
	bool *checkedThreads = calloc(threadCount, sizeof(*checkedThreads));
	struct crThread *workerThreads = calloc(threadCount, sizeof(*workerThreads));
	struct workerThreadState *workerThreadStates = calloc(threadCount, sizeof(*workerThreadStates));
	
	//Create render threads (Nonblocking)
	for (int t = 0; t < threadCount; ++t) {
		workerThreadStates[t] = (struct workerThreadState){.thread_num = t, .connectionSocket = connectionSocket, .socketMutex = g_worker_socket_mutex, .renderer = g_worker_renderer};
		workerThreads[t] = (struct crThread){.threadFunc = workerThread, .userData = &workerThreadStates[t]};
		if (threadStart(&workerThreads[t])) {
			logr(error, "Failed to create a crThread.\n");
		} else {
			g_worker_renderer->state.activeThreads++;
		}
	}
	
	while (g_worker_renderer->state.isRendering) {
		//Wait for render threads to finish (Render finished)
		for (int t = 0; t < threadCount; ++t) {
			if (workerThreadStates[t].threadComplete && !checkedThreads[t]) {
				--g_worker_renderer->state.activeThreads;
				checkedThreads[t] = true; //Mark as checked
			}
			if (!g_worker_renderer->state.activeThreads || g_worker_renderer->state.renderAborted) {
				g_worker_renderer->state.isRendering = false;
			}
		}
		sleepMSec(active_msec);
	}
	
	return goodbye();
}

struct command workerCommands[] = {
	{"handshake", 0},
	{"loadScene", 1},
	{"loadAssets", 2},
	{"startRender", 3},
};

static int matchCommand(struct command *cmdlist, char *cmd) {
	size_t commandCount = sizeof(workerCommands) / sizeof(struct {char *name; int id;});
	for (size_t i = 0; i < commandCount; ++i) {
		if (stringEquals(cmdlist[i].name, cmd)) return cmdlist[i].id;
	}
	return -1;
}

// Worker command handler
static cJSON *processCommand(int connectionSocket, const cJSON *json) {
	if (!json) {
		return errorResponse("Couldn't parse incoming JSON");
	}
	const cJSON *action = cJSON_GetObjectItem(json, "action");
	if (!cJSON_IsString(action)) {
		return errorResponse("No action provided");
	}
	
	switch (matchCommand(workerCommands, action->valuestring)) {
		case 0:
			return validateHandshake(json);
			break;
		case 1:
			return receiveScene(json);
			break;
		case 2:
			return receiveAssets(json);
			break;
		case 3:
			// startRender contains worker event loop and blocks until render completion.
			return startRender(connectionSocket, json);
			break;
		default:
			return errorResponse("Unknown command");
			break;
	}
	
	ASSERT_NOT_REACHED();
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

static bool checkConnectivity(struct renderClient client) {
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
		clients[i].state = checkConnectivity(clients[i]) ? Connected : ConnectionFailed;
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

static bool containsError(const cJSON *json) {
	const cJSON *error = cJSON_GetObjectItem(json, "error");
	if (cJSON_IsString(error)) {
		return true;
	}
	return false;
}

static bool containsGoodbye(const cJSON *json) {
	const cJSON *action = cJSON_GetObjectItem(json, "action");
	if (cJSON_IsString(action)) {
		if (stringEquals(action->valuestring, "goodbye")) {
			return true;
		}
	}
	return false;
}

static bool connectToClient(struct renderClient *client) {
	ASSERT(client->socket == -1);
	client->socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client->socket == -1) {
		logr(warning, "Failed to bind to socket on client %i\n", client->id);
		client->state = ConnectionFailed;
		return false;
	}
	
	//FIXME: Timeout needed here
	logr(debug, "Attempting connection to %s (this might get stuck here, no timeout yet)\n", inet_ntoa(client->address.sin_addr));
	if (connect(client->socket, (struct sockaddr *)&client->address, sizeof(client->address)) != 0) {
		logr(warning, "Failed to connect to %i\n", client->id);
		client->state = ConnectionFailed;
		return false;
	}
	client->state = Connected;
	return true;
}

static void disconnectFromClient(struct renderClient *client) {
	ASSERT(client->socket != -1);
	shutdown(client->socket, SHUT_RDWR);
	close(client->socket);
	client->socket = -1;
	client->state = Disconnected;
}

static void workerCleanup() {
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
	
	int port = isSet("worker_port") ? intPref("worker_port") : C_RAY_DEFAULT_PORT;
	
	bzero(&ownAddress, sizeof(ownAddress));
	ownAddress.sin_family = AF_INET;
	ownAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	ownAddress.sin_port = htons(port);
	
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
		logr(info, "Listening for connections on port %i\n", port);
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
				shutdown(connectionSocket, SHUT_RDWR);
				close(connectionSocket);
				break;
			}
			cJSON *message = cJSON_Parse(buf);
			cJSON *myResponse = processCommand(connectionSocket, message);
			char *responseText = cJSON_PrintUnformatted(myResponse);
			if (chunkedSend(connectionSocket, responseText)) {
				logr(warning, "chunkedSend() failed, error %s\n", strerror(errno));
				shutdown(connectionSocket, SHUT_RDWR);
				close(connectionSocket);
				break;
			};
			free(responseText);
			if (buf) free(buf);
			if (containsGoodbye(myResponse) || containsError(myResponse)) {
				cJSON_Delete(myResponse);
				cJSON_Delete(message);
				break;
			}
			cJSON_Delete(myResponse);
			cJSON_Delete(message);
			buf = NULL;
		}
		shutdown(connectionSocket, SHUT_RDWR);
		close(connectionSocket);
		workerCleanup(); // Prepare for next render
	}
	free(buf);
	shutdown(receivingSocket, SHUT_RDWR);
	close(receivingSocket);
	return 0;
}

static cJSON *processGetWork(struct renderThreadState *state, const cJSON *json) {
	(void)state;
	(void)json;
	struct renderTile tile = nextTile(state->renderer);
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
	
	switch (matchCommand(serverCommands, action->valuestring)) {
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
		return NULL;
	}
	
	// Set this worker into render mode
	sendJSON(client->socket, newAction("startRender"));
	
	// And just wait for commands.
	while (r->state.isRendering) {
		cJSON *request = readJSON(client->socket);
		if (!request) break;
		sendJSON(client->socket, processClientRequest(state, request));
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

static void *handleClientSync(void *arg) {
	struct syncThreadParams *params = (struct syncThreadParams *)threadUserData(arg);
	struct renderClient *client = params->client;
	connectToClient(client);
	if (client->state != Connected) {
		logr(warning, "Won't sync with client %i, no connection.\n", client->id);
		return NULL;
	}
	client->state = Syncing;
	
	// Handshake with the client
	sendJSON(client->socket, makeHandshake());
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
	cJSON_AddStringToObject(scene, "assetPath", params->renderer->prefs.assetPath);
	sendJSON(client->socket, scene);
	response = readJSON(client->socket);
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
