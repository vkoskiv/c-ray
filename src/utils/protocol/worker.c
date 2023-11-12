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
#include "../loaders/sceneloader.h"
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
	size_t completedSamples;
	long avgSampleTime;
	struct render_tile *current;
};

static cJSON *validateHandshake(cJSON *in) {
	const cJSON *version = cJSON_GetObjectItem(in, "version");
	const cJSON *githash = cJSON_GetObjectItem(in, "githash");
	if (!stringEquals(version->valuestring, PROTO_VERSION)) return errorResponse("Protocol version mismatch");
	if (!stringEquals(githash->valuestring, gitHash())) return errorResponse("Git hash mismatch");
	cJSON_Delete(in);
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
	logr(info, "Received scene description\n");
	g_worker_renderer = renderer_new();
	g_worker_renderer->state.file_cache = cache;
	g_worker_socket_mutex = mutex_create();
	cJSON *assetPathJson = cJSON_GetObjectItem(json, "assetPath");
	g_worker_renderer->prefs.assetPath = stringCopy(assetPathJson->valuestring);
	if (parse_json(g_worker_renderer, scene)) {
		return errorResponse("Scene parsing error");
	}
	cache_destroy(cache);
	cJSON *resp = newAction("ready");
	
	// Stash in our capabilities here
	//TODO: Maybe some performance value in here, so the master knows how much work to assign?
	// For now just report back how many threads we've got available.
	cJSON_AddNumberToObject(resp, "threadCount", g_worker_renderer->prefs.threads);
	return resp;
}

// Tilenum of -1 communicates that it failed to get work, signaling the work thread to exit
static struct render_tile *getWork(int connectionSocket) {
	if (!sendJSON(connectionSocket, newAction("getWork"), NULL)) {
		return NULL;
	}
	cJSON *response = readJSON(connectionSocket);
	if (!response) return NULL;
	cJSON *action = cJSON_GetObjectItem(response, "action");
	if (cJSON_IsString(action)) {
		if (stringEquals(action->valuestring, "renderComplete")) {
			logr(debug, "Master reported render is complete\n");
			return NULL;
		}
	}
	// FIXME: Pass the tile index only, and rename it to index too.
	// In fact, this whole tile object thing might be a bit pointless, since
	// we can just keep track of indices, and compute the tile dims
	cJSON *tileJson = cJSON_GetObjectItem(response, "tile");
	struct render_tile tile = decodeTile(tileJson);
	g_worker_renderer->state.tiles.items[tile.index] = tile;
	logr(debug, "Got work   : %i ((%i,%i),(%i,%i))\n", tile.index, tile.begin.x, tile.begin.y, tile.end.x, tile.end.y);
	cJSON_Delete(response);
	return &g_worker_renderer->state.tiles.items[tile.index];
}

static bool submitWork(int sock, struct texture *work, struct render_tile *forTile) {
	cJSON *result = encodeTexture(work);
	cJSON *tile = encodeTile(forTile);
	cJSON *package = newAction("submitWork");
	cJSON_AddItemToObject(package, "result", result);
	cJSON_AddItemToObject(package, "tile", tile);
	logr(debug, "Submit work: %i\n", forTile->index);
	return sendJSON(sock, package, NULL);
}

static void *workerThread(void *arg) {
	block_signals();
	struct workerThreadState *thread = (struct workerThreadState *)thread_user_data(arg);
	struct renderer *r = thread->renderer;
	int sock = thread->connectionSocket;
	struct cr_mutex *sockMutex = thread->socketMutex;
	
	//Fetch initial task
	mutex_lock(sockMutex);
	thread->current = getWork(sock);
	mutex_release(sockMutex);
	struct texture *tileBuffer = newTexture(float_p, thread->current->width, thread->current->height, 3);
	sampler *sampler = newSampler();

	struct camera *cam = thread->cam;
	
	struct timeval timer = { 0 };
	thread->completedSamples = 1;
	
	while (thread->current && r->state.rendering) {
		if (tileBuffer->width != thread->current->width || tileBuffer->height != thread->current->height) {
			destroyTexture(tileBuffer);
			tileBuffer = newTexture(float_p, thread->current->width, thread->current->height, 3);
		}
		long totalUsec = 0;
		long samples = 0;
		
		while (thread->completedSamples < r->prefs.sampleCount+1 && r->state.rendering) {
			timer_start(&timer);
			for (int y = thread->current->end.y - 1; y > thread->current->begin.y - 1; --y) {
				for (int x = thread->current->begin.x; x < thread->current->end.x; ++x) {
					if (r->state.render_aborted || !g_running) goto bail;
					uint32_t pixIdx = (uint32_t)(y * cam->width + x);
					initSampler(sampler, SAMPLING_STRATEGY, thread->completedSamples - 1, r->prefs.sampleCount, pixIdx);
					
					int local_x = x - thread->current->begin.x;
					int local_y = y - thread->current->begin.y;
					struct color output = textureGetPixel(tileBuffer, local_x, local_y, false);
					struct color sample = path_trace(cam_get_ray(cam, x, y, sampler), r->scene, r->prefs.bounces, sampler);

					nan_clamp(&sample, &output);
					
					//And process the running average
					output = colorCoef((float)(thread->completedSamples - 1), output);
					output = colorAdd(output, sample);
					float t = 1.0f / thread->completedSamples;
					output = colorCoef(t, output);
					
					setPixel(tileBuffer, output, local_x, local_y);
				}
			}
			//For performance metrics
			samples++;
			totalUsec += timer_get_us(timer);
			thread->completedSamples++;
			thread->current->completed_samples++;
			thread->avgSampleTime = totalUsec / samples;
		}
		
		thread->current->state = finished;
		mutex_lock(sockMutex);
		if (!submitWork(sock, tileBuffer, thread->current)) {
			mutex_release(sockMutex);
			break;
		}
		cJSON *resp = readJSON(sock);
		if (!resp || !stringEquals(cJSON_GetObjectItem(resp, "action")->valuestring, "ok")) {
			mutex_release(sockMutex);
			break;
		}
		mutex_release(sockMutex);
		thread->completedSamples = 1;
		mutex_lock(sockMutex);
		thread->current = getWork(sock);
		mutex_release(sockMutex);
		tex_clear(tileBuffer);
	}
bail:
	destroySampler(sampler);
	destroyTexture(tileBuffer);
	
	thread->threadComplete = true;
	return 0;
}

#define active_msec  16

static cJSON *startRender(int connectionSocket) {
	g_worker_renderer->state.rendering = true;
	g_worker_renderer->state.render_aborted = false;
	g_worker_renderer->state.saveImage = false;
	logr(info, "Starting network render job\n");
	
	size_t threadCount = g_worker_renderer->prefs.threads;
	struct cr_thread *worker_threads = calloc(threadCount, sizeof(*worker_threads));
	struct workerThreadState *workerThreadStates = calloc(threadCount, sizeof(*workerThreadStates));
	
	//Create render threads (Nonblocking)
	for (size_t t = 0; t < threadCount; ++t) {
		workerThreadStates[t] = (struct workerThreadState){
				.thread_num = t,
				.connectionSocket = connectionSocket,
				.socketMutex = g_worker_socket_mutex,
				.renderer = g_worker_renderer,
				.cam = &g_worker_renderer->scene->cameras.items[g_worker_renderer->prefs.selected_camera]};
		worker_threads[t] = (struct cr_thread){.thread_fn = workerThread, .user_data = &workerThreadStates[t]};
		if (thread_start(&worker_threads[t]))
			logr(error, "Failed to create a crThread.\n");
	}
	
	int pauser = 0;
	while (g_worker_renderer->state.rendering) {
		// Send stats about 4x/s
		if (pauser == 256 / active_msec) {
			cJSON *stats = newAction("stats");
			cJSON *array = cJSON_AddArrayToObject(stats, "tiles");
			logr(debug, "( ");
			for (size_t t = 0; t < threadCount; ++t) {
				struct render_tile *tile = workerThreadStates[t].current;
				if (tile) {
					cJSON_AddItemToArray(array, encodeTile(tile));
					logr(plain, "%i: %5zu%s", tile->index, tile->completed_samples, t < threadCount - 1 ? ", " : " ");
				}
			}
			logr(plain, ")\n");

			mutex_lock(g_worker_socket_mutex);
			if (!sendJSON(connectionSocket, stats, NULL)) {
				logr(debug, "Connection lost, bailing out.\n");
				// Setting this flag also kills the threads.
				g_worker_renderer->state.rendering = false;
			}
			mutex_release(g_worker_socket_mutex);
			pauser = 0;
		}
		pauser++;

		size_t inactive = 0;
		for (size_t t = 0; t < threadCount; ++t) {
			if (workerThreadStates[t].threadComplete) inactive++;
		}
		if (g_worker_renderer->state.render_aborted || inactive == threadCount)
			g_worker_renderer->state.rendering = false;

		timer_sleep_ms(active_msec);
	}

	//Make sure workder threads are terminated before continuing (This blocks)
	for (size_t t = 0; t < threadCount; ++t) {
		thread_wait(&worker_threads[t]);
	}
	return NULL;
}

// Worker command handler
static cJSON *processCommand(int connectionSocket, cJSON *json) {
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
			cJSON_Delete(json);
			return startRender(connectionSocket);
			break;
		default:
			cJSON_Delete(json);
			return errorResponse("Unknown command");
			break;
	}
	
	ASSERT_NOT_REACHED();
}

static void workerCleanup() {
	renderer_destroy(g_worker_renderer);
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
	
	int port = args_is_set("worker_port") ? args_int("worker_port") : C_RAY_DEFAULT_PORT;
	
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
			char *size = human_file_size(length);
			logr(debug, "Received %s, took %lums.\n", size, millisecs);
			free(size);
			cJSON *message = cJSON_Parse(buf);
			if (isShutdown(message)) {
				g_running = false;
				cJSON_Delete(message);
				break;
			}
			cJSON *myResponse = processCommand(connectionSocket, message);
			if (!myResponse) {
				if (buf) free(buf);
				break;
			}
			char *responseText = cJSON_PrintUnformatted(myResponse);
			if (!chunkedSend(connectionSocket, responseText, NULL)) {
				logr(debug, "chunkedSend() failed, error %s\n", strerror(errno));
				break;
			};
			free(responseText);
			if (buf) free(buf);
			buf = NULL;
			if (containsGoodbye(myResponse) || containsError(myResponse)) {
				cJSON_Delete(myResponse);
				break;
			}
			cJSON_Delete(myResponse);
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
