//
//  networking.h
//  C-ray
//
//  Created by Valtteri on 5.1.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#pragma once

struct renderClient;
struct renderTile;
struct texture;

// Synchronise renderer state with clients, and return a list of clients
// ready to do some rendering
struct renderClient *syncWithClients(size_t *count);

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

int startMasterServer(void);
int startWorkerServer(void);
