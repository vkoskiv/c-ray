//
//  server.h
//  c-ray
//
//  Created by Valtteri Koskivuori on 06/04/2021.
//  Copyright © 2021-2025 Valtteri Koskivuori. All rights reserved.
//

#pragma once

#include <common/dyn_array.h>

#ifndef WINDOWS
#include <arpa/inet.h>
#endif

struct renderer;

enum client_status {
	Disconnected,
	Connected,
	ConnectionFailed,
	Syncing,
	SyncFailed,
	Synced,
	Rendering,
	Finished
};

struct render_client {
#ifndef WINDOWS
	struct sockaddr_in address;
#endif
	enum client_status status;
	int available_threads;
	int socket;
	int id;
};

typedef struct render_client render_client;
dyn_array_def(render_client)

void clients_shutdown(const char *node_list);

// Synchronise renderer state with clients, and return a list of clients
// ready to do some rendering
struct render_client_arr clients_sync(const struct renderer *r);

void *client_connection_thread(void *arg);
