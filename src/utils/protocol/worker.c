//
//  worker.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 06/04/2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#include "../logging.h"
//Windows is annoying, so it's just not going to have networking. Because it is annoying and proprietary.
#ifndef WINDOWS
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "worker.h"
#include "protocol.h"

#include "../../datatypes/tile.h"
#include "../../renderer/renderer.h"
#include "../../datatypes/image/texture.h"
#include "../../datatypes/scene.h"
#include "../platform/mutex.h"
#include "../platform/thread.h"
#include "../networking.h"
#include "../string.h"
#include "../filecache.h"
#include "../gitsha1.h"
#include "../timer.h"
#include "../args.h"

struct renderer *g_worker_renderer = NULL;
struct crMutex *g_worker_socket_mutex = NULL;

struct command workerCommands[] = {
	{"handshake", 0},
	{"loadScene", 1},
	{"loadAssets", 2},
	{"startRender", 3},
};

struct workerThreadState {
	int thread_num;
	int connectionSocket;
	struct crMutex *socketMutex;
	struct renderer *renderer;
	bool threadComplete;
};

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
	logr(info, "Received scene description\n");
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
	return resp;
}

static cJSON *receiveAssets(const cJSON *json) {
	cJSON *files = cJSON_GetObjectItem(json, "files");
	char *data = cJSON_PrintUnformatted(files);
	logr(info, "Received scene assets\n");
	decodeFileCache(data);
	free(data);
	return newAction("ok");
}

// Tilenum of -1 communicates that it failed to get work, signaling the work thread to exit
static struct renderTile getWork(int connectionSocket) {
	if (!sendJSON(connectionSocket, newAction("getWork"))) {
		struct renderTile tile = {0};
		tile.tileNum = -1;
		return tile;
	}
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

static bool submitWork(int sock, struct texture *work, struct renderTile forTile) {
	cJSON *result = encodeTexture(work);
	cJSON *tile = encodeTile(forTile);
	cJSON *package = newAction("submitWork");
	cJSON_AddItemToObject(package, "result", result);
	cJSON_AddItemToObject(package, "tile", tile);
	return sendJSON(sock, package);
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
		if (!submitWork(sock, thing, tile)) {
			releaseMutex(sockMutex);
			break;
		}
		cJSON *resp = readJSON(sock);
		if (!resp || !stringEquals(cJSON_GetObjectItem(resp, "action")->valuestring, "ok")) {
			releaseMutex(sockMutex);
			break;
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

static cJSON *startRender(int connectionSocket) {
	g_worker_renderer->state.isRendering = true;
	g_worker_renderer->state.renderAborted = false;
	g_worker_renderer->state.saveImage = false;
	logr(info, "Starting network render job\n");
	
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
	
	//TODO: Send out stats here
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

// Worker command handler
static cJSON *processCommand(int connectionSocket, const cJSON *json) {
	if (!json) {
		return errorResponse("Couldn't parse incoming JSON");
	}
	const cJSON *action = cJSON_GetObjectItem(json, "action");
	if (!cJSON_IsString(action)) {
		return errorResponse("No action provided");
	}
	
	switch (matchCommand(workerCommands, sizeof(workerCommands) / sizeof(struct command),action->valuestring)) {
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
			return startRender(connectionSocket);
			break;
		default:
			return errorResponse("Unknown command");
			break;
	}
	
	ASSERT_NOT_REACHED();
}

static void workerCleanup() {
	destroyRenderer(g_worker_renderer);
	destroyFileCache();
}

bool isShutdown(cJSON *json) {
	cJSON *action = cJSON_GetObjectItem(json, "action");
	if (cJSON_IsString(action)) {
		if (stringEquals(action->valuestring, "shutdown")) {
			return true;
		}
	}
	return false;
}

int startWorkerServer() {
	signal(SIGPIPE, SIG_IGN);
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
	
	bool running = true;
	
	while (running) {
		logr(info, "Listening for connections on port %i\n", port);
		connectionSocket = accept(receivingSocket, (struct sockaddr *)&masterAddress, &len);
		if (connectionSocket < 0) {
			logr(error, "Failed to accept\n");
		}
		logr(info, "Got connection from %s\n", inet_ntoa(masterAddress.sin_addr));
		
		for (;;) {
			buf = NULL;
			ssize_t read = chunkedReceive(connectionSocket, &buf);
			if (read == 0) break;
			if (read < 0) {
				logr(warning, "Something went wrong. Error: %s\n", strerror(errno));
				break;
			}
			cJSON *message = cJSON_Parse(buf);
			if (isShutdown(message)) {
				running = false;
				cJSON_Delete(message);
				break;
			}
			cJSON *myResponse = processCommand(connectionSocket, message);
			char *responseText = cJSON_PrintUnformatted(myResponse);
			if (!chunkedSend(connectionSocket, responseText)) {
				logr(debug, "chunkedSend() failed, error %s\n", strerror(errno));
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
		if (running) {
			logr(info, "Cleaning up for next render\n");
		} else {
			logr(info, "Received shutdown command, exiting\n");
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
#else
int startWorkerServer() {
	logr(error, "c-ray doesn't support the proprietary networking stack on Windows yet. Sorry!\n");
}
#endif
