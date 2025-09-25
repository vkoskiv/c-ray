//
//  server.c
//  c-ray
//
//  Created by Valtteri Koskivuori on 06/04/2021.
//  Copyright © 2021-2025 Valtteri Koskivuori. All rights reserved.
//

#include <stddef.h>
#include <common/logging.h>
//Windows is annoying, so it's just not going to have networking. Because it is annoying and proprietary.
#ifndef WINDOWS

#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "server.h"
#include "protocol.h"

#include <renderer/renderer.h>
#include <common/texture.h>
#include <common/platform/thread.h>
#include <common/networking.h>
#include <common/textbuffer.h>
#include <common/gitsha1.h>
#include <common/cr_assert.h>
#include <common/fileio.h>
#include <common/platform/terminal.h>
#include <common/platform/signal.h>

void client_drop(struct render_client *client) {
	ASSERT(client->socket != -1);
	shutdown(client->socket, SHUT_RDWR);
	close(client->socket);
	client->socket = -1;
	client->status = Disconnected;
}

static cJSON *make_handshake(void) {
	cJSON *handshake = cJSON_CreateObject();
	cJSON_AddStringToObject(handshake, "action", "handshake");
	cJSON_AddStringToObject(handshake, "version", PROTO_VERSION);
	cJSON_AddStringToObject(handshake, "githash", gitHash());
	return handshake;
}

static struct sockaddr_in parse_address(const char *str) {
	char buf[LINEBUFFER_MAXSIZE];
	lineBuffer line = { .buf = buf };
	fillLineBuffer(&line, str, ':');
	struct sockaddr_in address = {0};
	address.sin_family = AF_INET;
	char *addr_string = firstToken(&line);
	struct hostent *ent = gethostbyname(addr_string);
	memcpy(&address.sin_addr.s_addr, ent->h_addr_list[0], ent->h_length);
	address.sin_port = line.amountOf.tokens > 1 ? htons(atoi(lastToken(&line))) : htons(2222);
	return address;
}

static bool client_try_connect(struct render_client *client) {
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
	errno = 0;
	(void)connect(client->socket, (struct sockaddr *)&client->address, sizeof(client->address));
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

// Fetches list of nodes from node string, verifies that they are reachable, and
// returns them in a nice list.
static struct render_client_arr build_client_list(const char *node_list) {
	ASSERT(node_list);
	struct render_client_arr clients = { 0 };
	if (!node_list) return clients;
	if (strlen(node_list) == 0) return clients;
	// Really barebones parsing for IP addresses and ports in a comma-separated list
	// Expected to break easily. Don't break it.
	char buf[LINEBUFFER_MAXSIZE];
	lineBuffer line = { .buf = buf };
	fillLineBuffer(&line, node_list, ',');
	ASSERT(line.amountOf.tokens > 0);
	char *current = firstToken(&line);
	for (size_t i = 0; i < line.amountOf.tokens; ++i) {
		struct render_client client = { 0 };
		client.address = parse_address(current);
		client.status = client_try_connect(&client) ? Connected : ConnectionFailed;
		if (client.status == Connected) render_client_arr_add(&clients, client);
		current = nextToken(&line);
	}
	
	for (size_t i = 0; i < clients.count; ++i) {
		clients.items[i].id = (int)i;
	}
	
	return clients;
}

static cJSON *handle_get_work(struct worker *state, const cJSON *json) {
	(void)state;
	(void)json;
	struct render_tile *tile = tile_next(state->tiles);
	if (!tile) return newAction("renderComplete");
	tile->network_renderer = true;
	cJSON *response = newAction("newWork");
	cJSON_AddItemToObject(response, "tile", encodeTile(tile));
	return response;
}

static cJSON *handle_submit_work(struct worker *state, const cJSON *json) {
	cJSON *result = cJSON_GetObjectItem(json, "result");
	struct texture *texture = deserialize_texture(result);
	cJSON *tile_json = cJSON_GetObjectItem(json, "tile");
	struct render_tile tile = decodeTile(tile_json);
	state->tiles->tiles.items[tile.index] = tile;
	state->tiles->tiles.items[tile.index].state = finished; // FIXME: Remove
	for (int y = tile.end.y - 1; y > tile.begin.y - 1; --y) {
		for (int x = tile.begin.x; x < tile.end.x; ++x) {
			struct color value = tex_get_px(texture, x - tile.begin.x, y - tile.begin.y, false);
			tex_set_px(*state->buf, value, x, y);
		}
	}
	tex_destroy(texture);
	return newAction("ok");
}

struct command serverCommands[] = {
	{"getWork", 0},
	{"submitWork", 1},
	{"goodbye", 2},
};

static cJSON *handle_client_request(struct worker *state, const cJSON *json) {
	if (!json) {
		return errorResponse("Couldn't parse incoming JSON");
	}
	const cJSON *action = cJSON_GetObjectItem(json, "action");
	if (!cJSON_IsString(action)) {
		return errorResponse("No action provided");
	}
	
	switch (matchCommand(serverCommands, sizeof(serverCommands) / sizeof(struct command), action->valuestring)) {
		case 0:
			return handle_get_work(state, json);
			break;
		case 1:
			return handle_submit_work(state, json);
			break;
		case 2:
			state->thread_complete = true;
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
void *client_connection_thread(void *arg) {
	block_signals();
	struct worker *state = arg;
	struct renderer *r = state->renderer;
	struct render_client *client = state->client;
	if (!client) {
		state->thread_complete = true;
		return 0;
	}
	if (client->status != Synced) {
		logr(debug, "Client %i wasn't synced fully, dropping.\n", client->id);
		state->thread_complete = true;
		return 0;
	}
	
	// Set this worker into render mode
	if (!sendJSON(client->socket, newAction("startRender"), NULL)) {
		logr(warning, "Client disconnected? Stopping for %i\n", client->id);
		state->thread_complete = true;
		return 0;
	}
	
	// And just wait for commands.
	while (r->state.s == r_rendering && !state->thread_complete) {
		cJSON *request = readJSON(client->socket);
		if (containsStats(request)) {
			cJSON *array = cJSON_GetObjectItem(request, "tiles");
			if (cJSON_IsArray(array)) {
				cJSON *tile = NULL;
				cJSON_ArrayForEach(tile, array) {
					struct render_tile t = decodeTile(tile);
					state->tiles->tiles.items[t.index] = t;
					//r->state.renderTiles[t.tileNum].completed_samples = t.completed_samples;
				}
			}
		} else {
			cJSON *response = handle_client_request(state, request);
			if (containsError(response)) {
				char *err = cJSON_PrintUnformatted(response);
				logr(debug, "error, exiting netrender thread: %s\n", err);
				free(err);
				sendJSON(client->socket, response, NULL);
				break;
			}
			sendJSON(client->socket, response, NULL);
		}
		cJSON_Delete(request);
	}
	
	// Let the worker now we're done here
	// TODO (right now we disconnect, and the client implies from that)
	state->thread_complete = true;
	return 0;
}

struct sync_thread {
	struct render_client *client;
	char *serialized_renderer;
	size_t progress;
	bool done;
};

static void *client_sync_thread(void *arg) {
	block_signals();
	struct sync_thread *params = arg;
	struct render_client *client = params->client;
	if (client->status != Connected) {
		logr(warning, "Won't sync with client %i, no connection.\n", client->id);
		params->done = true;
		return NULL;
	}
	client->status = Syncing;
	
	// Handshake with the client
	if (!sendJSON(client->socket, make_handshake(), NULL)) {
		client->status = SyncFailed;
		params->done = true;
		return NULL;
	}
	cJSON *response = readJSON(client->socket);
	if (cJSON_HasObjectItem(response, "error")) {
		cJSON *error = cJSON_GetObjectItem(response, "error");
		logr(warning, "Client handshake error: %s\n", error->valuestring);
		client->status = SyncFailed;
		params->done = true;
		cJSON_Delete(response);
		return NULL;
	}
	cJSON_Delete(response);
	response = NULL;
	
	// Send the scene & assets
	cJSON *scene = cJSON_CreateObject();
	cJSON_AddStringToObject(scene, "action", "loadScene");
	logr(debug, "Syncing state to client %d\n", client->id);
	// FIXME: Would be better to just send the string directly instead of wrapping it in json
	cJSON_AddStringToObject(scene, "data", params->serialized_renderer);
	sendJSON(client->socket, scene, &params->progress);
	response = readJSON(client->socket);
	if (!response) {
		logr(debug, "no response\n");
		client->status = SyncFailed;
		params->done = true;
		return NULL;
	}
	if (cJSON_HasObjectItem(response, "error")) {
		cJSON *error = cJSON_GetObjectItem(response, "error");
		logr(warning, "Client scene sync error: %s\n", error->valuestring);
		client->status = SyncFailed;
		params->done = true;
		cJSON_Delete(error);
		cJSON_Delete(response);
		client_drop(client);
		return NULL;
	}
	cJSON *action = cJSON_GetObjectItem(response, "action");
	if (cJSON_IsString(action)) {
		cJSON *threadCount = cJSON_GetObjectItem(response, "threadCount");
		if (cJSON_IsNumber(threadCount)) {
			client->available_threads = threadCount->valueint;
		}
	}
	logr(debug, "Finished client %i sync. It reports %i threads available for rendering.\n", client->id, client->available_threads);
	cJSON_Delete(response);
	
	// Sync successful, mark it as such
	client->status = Synced;
	params->done = true;
	return NULL;
}

void clients_shutdown(const char *node_list) {
	struct render_client_arr clients = build_client_list(node_list);
	logr(info, "Sending shutdown command to %zu client%s.\n", clients.count, PLURAL(clients.count));
	if (clients.count < 1) {
		logr(warning, "No clients found, exiting\n");
		return;
	}
	for (size_t i = 0; i < clients.count; ++i) {
		sendJSON(clients.items[i].socket, newAction("shutdown"), NULL);
		client_drop(&clients.items[i]);
	}
	logr(info, "Done, exiting.\n");
	render_client_arr_free(&clients);
}

#define BAR_LENGTH 32
static void print_bar(struct sync_thread *param) {
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

static void print_progbars(struct sync_thread *params, size_t clientCount) {
	if (!isTeleType()) return;
	
	for (size_t i = 0; i < clientCount; ++i) {
		print_bar(&params[i]);
	}
}

struct render_client_arr clients_sync(const struct renderer *r) {
	signal(SIGPIPE, SIG_IGN);
	struct render_client_arr clients = build_client_list(r->prefs.node_list);
	if (clients.count < 1) {
		logr(warning, "No clients found, rendering solo.\n");
		return (struct render_client_arr){ 0 };
	}
	
	char *serialized = serialize_renderer(r);
	size_t transfer_bytes = strlen(serialized);
	char buf[64];
	logr(info, "Sending %s to %zu client%s...\n", human_file_size(transfer_bytes, buf), clients.count, PLURAL(clients.count));
	
	struct sync_thread *params = calloc(clients.count, sizeof(*params));
	logr(debug, "Client list:\n");
	for (size_t i = 0; i < clients.count; ++i) {
		logr(debug, "\tclient %zu: %s:%i\n", i, inet_ntoa(clients.items[i].address.sin_addr), htons(clients.items[i].address.sin_port));
		params[i].client = &clients.items[i];
		params[i].serialized_renderer = serialized;
	}
	
	struct cr_thread *sync_threads = calloc(clients.count, sizeof(*sync_threads));
	for (size_t i = 0; i < clients.count; ++i) {
		sync_threads[i] = (struct cr_thread){
			.thread_fn = client_sync_thread,
			.user_data = &params[i]
		};
	}
	
	for (size_t i = 0; i < clients.count; ++i) {
		if (thread_start(&sync_threads[i])) {
			logr(warning, "Something went wrong while starting the sync thread for client %i. May want to look into that.\n", (int)i);
		}
	}
	
	size_t loops = 0;
	while (true) {
		bool all_stopped = true;
		for (size_t i = 0; i < clients.count; ++i) {
			if (!params[i].done) all_stopped = false;
		}
		if (all_stopped) break;
		v_timer_sleep_ms(10);
		if (++loops == 10) {
			loops = 0;
			print_progbars(params, clients.count);
			printf("\033[%zuF", clients.count);
		}
	}
	
	// Block here and verify threads are done before continuing.
	for (size_t i = 0; i < clients.count; ++i) {
		thread_wait(&sync_threads[i]);
	}
	
	for (size_t i = 0; i < clients.count; ++i) printf("\n");
	logr(info, "Client sync finished.\n");

	free(serialized);
	free(sync_threads);
	free(params);
	return clients;
}

#else

void *client_connection_thread(void *arg) {
	return 0;
}

void clients_shutdown() {
	logr(warning, "c-ray doesn't support the proprietary networking stack on Windows yet. Sorry!\n");
}

struct renderClient *clients_sync(const struct renderer *r, size_t *count) {
	if (count) *count = 0;
	logr(warning, "c-ray doesn't support the proprietary networking stack on Windows yet. Sorry!\n");
	return NULL;
}

#endif
