//
//  worker.c
//  C-Ray
//
//  Created by Valtteri Koskivuori on 06/04/2021.
//  Copyright © 2021-2022 Valtteri Koskivuori. All rights reserved.
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
#include "../../renderer/pathtrace.h"
#include "../../datatypes/image/texture.h"
#include "../../datatypes/scene.h"
#include "../../datatypes/camera.h"
#include "../platform/mutex.h"
#include "../platform/thread.h"
#include "../networking.h"
#include "../string.h"
#include "../filecache.h"
#include "../gitsha1.h"
#include "../timer.h"
#include "../args.h"
#include "../../utils/platform/signal.h"
#include <stdio.h>
#include <inttypes.h>

struct renderer *g_worker_renderer = NULL;
struct cr_mutex *g_worker_socket_mutex = NULL;
static bool g_running = false;

struct command workerCommands[] = {
	{"handshake", 0},
	{"loadScene", 1},
	{"startRender", 3},
};

struct workerThreadState {
	int thread_num;
	int connectionSocket;
	struct cr_mutex *socketMutex;
	struct camera *cam;
	struct renderer *renderer;
	bool threadComplete;
	uint64_t totalSamples;
	int completedSamples;
	long avgSampleTime;
};

static cJSON *validateHandshake(const cJSON *in) {
	const cJSON *version = cJSON_GetObjectItem(in, "version");
	const cJSON *githash = cJSON_GetObjectItem(in, "githash");
	if (!stringEquals(version->valuestring, PROTO_VERSION)) return errorResponse("Protocol version mismatch");
	if (!stringEquals(githash->valuestring, gitHash())) return errorResponse("Git hash mismatch");
	return newAction("startSync");
}

static cJSON *receiveScene(const cJSON *json) {
	// Load assets
	cJSON *fileCache = cJSON_GetObjectItem(json, "files");
	if (!fileCache) return errorResponse("No file cache found");
	char *data = cJSON_PrintUnformatted(fileCache);
	//FIXME: This is an awkward API, why not pass cJSON directly?
	struct file_cache *cache = calloc(1, sizeof(*cache));
	cache_decode(cache, data);
	free(data);
	
	// And then the scene
	cJSON *scene = cJSON_GetObjectItem(json, "data");
	char *sceneText = cJSON_PrintUnformatted(scene);
	logr(info, "Received scene description\n");
	g_worker_renderer = newRenderer();
	g_worker_renderer->state.file_cache = cache;
	g_worker_socket_mutex = mutex_create();
	cJSON *assetPathJson = cJSON_GetObjectItem(json, "assetPath");
	g_worker_renderer->prefs.assetPath = stringCopy(assetPathJson->valuestring);
	if (loadScene(g_worker_renderer, sceneText)) {
		return errorResponse("Scene parsing error");
	}
	free(sceneText);
	cache_destroy(cache);
	cJSON *resp = newAction("ready");
	
	// Stash in our capabilities here
	//TODO: Maybe some performance value in here, so the master knows how much work to assign?
	// For now just report back how many threads we've got available.
	cJSON_AddNumberToObject(resp, "threadCount", g_worker_renderer->prefs.threadCount);
	return resp;
}

// Tilenum of -1 communicates that it failed to get work, signaling the work thread to exit
static struct renderTile getWork(int connectionSocket) {
	if (!sendJSON(connectionSocket, newAction("getWork"))) {
		return (struct renderTile){ .tileNum = -1 };
	}
	cJSON *response = readJSON(connectionSocket);
	cJSON *action = cJSON_GetObjectItem(response, "action");
	if (cJSON_IsString(action)) {
		if (stringEquals(action->valuestring, "renderComplete")) {
			logr(debug, "Master reported render is complete\n");
			return (struct renderTile){ .tileNum = -1 };
		}
	}
	if (!response) {
		return (struct renderTile){ .tileNum = -1 };
	}
	cJSON *tileJson = cJSON_GetObjectItem(response, "tile");
	struct renderTile tile = decodeTile(tileJson);
	logr(debug, "Got work   : %i ((%i,%i),(%i,%i))\n", tile.tileNum, tile.begin.x, tile.begin.y, tile.end.x, tile.end.y);
	cJSON_Delete(response);
	return tile;
}

static bool submitWork(int sock, struct texture *work, struct renderTile forTile) {
	cJSON *result = encodeTexture(work);
	cJSON *tile = encodeTile(&forTile);
	cJSON *package = newAction("submitWork");
	cJSON_AddItemToObject(package, "result", result);
	cJSON_AddItemToObject(package, "tile", tile);
	logr(debug, "Submit work: %i\n", forTile.tileNum);
	return sendJSON(sock, package);
}

static void *workerThread(void *arg) {
	struct workerThreadState *threadState = (struct workerThreadState *)threadUserData(arg);
	struct renderer *r = threadState->renderer;
	int sock = threadState->connectionSocket;
	struct cr_mutex *sockMutex = threadState->socketMutex;
	
	//Fetch initial task
	mutex_lock(sockMutex);
	struct renderTile tile = getWork(sock);
	mutex_release(sockMutex);
	struct texture *tileBuffer = newTexture(char_p, tile.width, tile.height, 3);
	sampler *sampler = newSampler();

	struct camera *cam = threadState->cam;
	
	struct timeval timer = { 0 };
	threadState->completedSamples = 1;
	
	while (tile.tileNum != -1 && r->state.isRendering) {
		long totalUsec = 0;
		long samples = 0;
		
		while (threadState->completedSamples < r->prefs.sampleCount+1 && r->state.isRendering) {
			timer_start(&timer);
			for (int y = tile.end.y - 1; y > tile.begin.y - 1; --y) {
				for (int x = tile.begin.x; x < tile.end.x; ++x) {
					if (r->state.renderAborted || !g_running) goto bail;
					uint32_t pixIdx = (uint32_t)(y * cam->width + x);
					initSampler(sampler, SAMPLING_STRATEGY, threadState->completedSamples - 1, r->prefs.sampleCount, pixIdx);
					
					struct color output = textureGetPixel(r->state.renderBuffer, x, y, false);
					struct lightRay incidentRay = cam_get_ray(cam, x, y, sampler);
					struct color sample = pathTrace(&incidentRay, r->scene, r->prefs.bounces, sampler);
					
					//And process the running average
					output = colorCoef((float)(threadState->completedSamples - 1), output);
					output = colorAdd(output, sample);
					float t = 1.0f / threadState->completedSamples;
					output = colorCoef(t, output);
					
					//Store internal render buffer (float precision)
					setPixel(r->state.renderBuffer, output, x, y);
					
					//Gamma correction
					output = colorToSRGB(output);
					
					//And store the image data
					int localX = x - tile.begin.x;
					int localY = y - tile.begin.y;
					setPixel(tileBuffer, output, localX, localY);
				}
			}
			//For performance metrics
			samples++;
			totalUsec += timer_get_us(timer);
			threadState->totalSamples++;
			threadState->completedSamples++;
			threadState->avgSampleTime = totalUsec / samples;
		}
		
		mutex_lock(sockMutex);
		if (!submitWork(sock, tileBuffer, tile)) {
			mutex_release(sockMutex);
			break;
		}
		cJSON *resp = readJSON(sock);
		if (!resp || !stringEquals(cJSON_GetObjectItem(resp, "action")->valuestring, "ok")) {
			mutex_release(sockMutex);
			break;
		}
		mutex_release(sockMutex);
		threadState->completedSamples = 1;
		mutex_lock(sockMutex);
		tile = getWork(sock);
		mutex_release(sockMutex);
		if (tileBuffer->width != tile.width || tileBuffer->height != tile.height) {
			destroyTexture(tileBuffer);
			tileBuffer = newTexture(char_p, tile.width, tile.height, 3);
		}
	}
bail:
	destroySampler(sampler);
	destroyTexture(tileBuffer);
	
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
		workerThreadStates[t] = (struct workerThreadState){
				.thread_num = t,
				.connectionSocket = connectionSocket,
				.socketMutex = g_worker_socket_mutex,
				.renderer = g_worker_renderer,
				.cam = &g_worker_renderer->scene->cameras[g_worker_renderer->prefs.selected_camera]};
		workerThreads[t] = (struct crThread){.threadFunc = workerThread, .userData = &workerThreadStates[t]};
		if (threadStart(&workerThreads[t])) {
			logr(error, "Failed to create a crThread.\n");
		} else {
			g_worker_renderer->state.activeThreads++;
		}
	}
	
	float avgSampleTime = 0.0f;
	float avgTimePerTilePass = 0.0f;
	int ctr = 1;
	int pauser = 0;
	//TODO: Send out stats here
	while (g_worker_renderer->state.isRendering) {
		
		// Gather and send statistics to master node
		for(int t = 0; t < g_worker_renderer->prefs.threadCount; ++t) {
			avgSampleTime += workerThreadStates[t].avgSampleTime;
		}
		avgTimePerTilePass += avgSampleTime / g_worker_renderer->prefs.threadCount;
		avgTimePerTilePass /= ctr++;
		
		// Send stats about 1x/s
		if (pauser == 1024 / active_msec) {
			uint64_t completedSamples = 0;
			for (int t = 0; t < threadCount; ++t) {
				completedSamples += workerThreadStates[t].totalSamples;
			}
			cJSON *stats = newAction("stats");
			cJSON_AddNumberToObject(stats, "completed", completedSamples);
			cJSON_AddNumberToObject(stats, "avgPerPass", avgTimePerTilePass);
			mutex_lock(g_worker_socket_mutex);
			logr(debug, "Sending stats update for: %"PRIu64", %.2f\n", completedSamples, avgTimePerTilePass);
			sendJSON(connectionSocket, stats);
			mutex_release(g_worker_socket_mutex);
			pauser = 0;
		}
		pauser++;
		
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
		timer_sleep_ms(active_msec);
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
	
	switch (matchCommand(workerCommands, sizeof(workerCommands) / sizeof(struct command), action->valuestring)) {
		case 0:
			return validateHandshake(json);
			break;
		case 1:
			return receiveScene(json);
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
	g_worker_renderer = NULL;
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

// Hack. Yoink the receiving socket fd to close it. Don't see another way.
static int recvsock_temp = 0;
void exitHandler(int sig) {
	(void)sig;
	ASSERT(sig == SIGINT);
	printf("\n");
	logr(info, "Received ^C, shutting down worker.\n");
	g_running = false;
	close(recvsock_temp);
}

int startWorkerServer() {
	signal(SIGPIPE, SIG_IGN);
	if (registerHandler(sigint, exitHandler) < 0) {
		logr(error, "registerHandler failed\n");
	}
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
	
	recvsock_temp = receivingSocket;
	if (listen(receivingSocket, 1) != 0) {
		logr(error, "It wouldn't listen\n");
	}
	
	struct sockaddr_in masterAddress;
	socklen_t len = sizeof(masterAddress);
	char *buf = NULL;
	
	g_running = true;
	
	while (g_running) {
		logr(info, "Listening for connections on port %i\n", port);
		connectionSocket = accept(receivingSocket, (struct sockaddr *)&masterAddress, &len);
		if (connectionSocket < 0) {
			logr(debug, "Failed to accept\n");
			goto bail;
		}
		logr(info, "Got connection from %s\n", inet_ntoa(masterAddress.sin_addr));
		
		for (;;) {
			buf = NULL;
			size_t length = 0;
			struct timeval timer;
			timer_start(&timer);
			ssize_t read = chunkedReceive(connectionSocket, &buf, &length);
			if (read == 0) break;
			if (read < 0) {
				logr(warning, "Something went wrong. Error: %s\n", strerror(errno));
				break;
			}
			long millisecs = timer_get_ms(timer);
			char *size = humanFileSize(length);
			logr(debug, "Received %s, took %lums.\n", size, millisecs);
			free(size);
			cJSON *message = cJSON_Parse(buf);
			if (isShutdown(message)) {
				g_running = false;
				cJSON_Delete(message);
				break;
			}
			cJSON *myResponse = processCommand(connectionSocket, message);
			char *responseText = cJSON_PrintUnformatted(myResponse);
			if (!chunkedSend(connectionSocket, responseText, NULL)) {
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
	bail:
		if (g_running) {
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
