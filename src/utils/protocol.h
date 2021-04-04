//
//  protocol.h
//  C-Ray
//
//  Created by Valtteri Koskivuori on 21/03/2021.
//  Copyright © 2021 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct renderClient;
struct renderTile;
struct texture;
struct renderer;

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

#include <arpa/inet.h>

struct renderClient {
	struct sockaddr_in address;
	enum clientState state;
	int availableThreads;
	int socket;
	int id;
};

// Synchronise renderer state with clients, and return a list of clients
// ready to do some rendering
struct renderClient *syncWithClients(const struct renderer *r, size_t *count);

struct tileResponse {
	struct texture *result;
	//TODO: Additional perf metrics here?
	enum {
		Success,
		Failed,
		ConnectionLost,
	} status;
};

struct tileResponse *requestTile(struct renderClient *, struct renderTile *);

//int startMasterServer(void);
int startWorkerServer(void);

void *networkRenderThread(void *arg);
