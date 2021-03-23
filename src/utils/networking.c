//
//  networking.c
//  C-ray
//
//  Created by Valtteri on 5.1.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "networking.h"

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

#define MAXRCVLEN 128
#define C_RAY_HEADERSIZE 8
#define C_RAY_CHUNKSIZE 1024
#define C_RAY_PORT 2222
#define PROTO_VERSION "0.1"

enum clientState {
	Connected,
	ConnectionFailed,
	Syncing,
	Synced,
	Rendering,
	Finished
};

struct renderClient {
	struct sockaddr_in address;
	enum clientState state;
	int id;
};

bool chunkedSend(int socket, const char *data) {
	const size_t msgLen = strlen(data) + 1; // +1 for null byte
	const size_t chunkSize = C_RAY_CHUNKSIZE;
	size_t chunks = msgLen / chunkSize;
	chunks = (msgLen % chunkSize) != 0 ? chunks + 1: chunks;
	//logr(debug, "Sending %lu chunks\n", chunks);
	
	// Send header with message length
	size_t header = htonll(msgLen);
	ssize_t err = send(socket, &header, sizeof(header), SO_NOSIGPIPE);
	if (err < 0) logr(error, "Failed to send header to client.\n");
	
	ssize_t n = 0;
	size_t sentChunks = 0;
	char *currentChunk = calloc(chunkSize, sizeof(*currentChunk));
	size_t leftToSend = msgLen;
	for (size_t i = 0; i < chunks; ++i) {
		size_t copylen = leftToSend > chunkSize ? chunkSize : leftToSend;
		memcpy(currentChunk, data + (i * chunkSize), copylen);
		n = send(socket, currentChunk, chunkSize, SO_NOSIGPIPE);
		if (n == -1) {
			logr(warning, "chunkedSend error: %s\n", strerror(errno));
			free(currentChunk);
			return -1;
		}
		sentChunks++;
		leftToSend -= min(copylen, chunkSize);
		memset(currentChunk, 0, chunkSize);
	}
	ASSERT(leftToSend == 0);
	//logr(debug, "Sent %lu chunks\n", sentChunks);
	return n == -1 ? true : false;
}

ssize_t chunkedReceive(int socket, char **data) {
	// Grab header first
	size_t headerData = 0;
	size_t chunkSize = C_RAY_CHUNKSIZE;
	ssize_t err = recv(socket, &headerData, sizeof(headerData), 0);
	if (headerData == 0) logr(error, "Received header of 0 from server\n");
	if (err == -1) logr(warning, "chunkedReceive header error: %s\n", strerror(errno));
	size_t msgLen = ntohll(headerData);
	//logr(debug, "Received header: %lu\n", msgLen);
	size_t chunks = msgLen / chunkSize;
	chunks = (msgLen % chunkSize) != 0 ? chunks + 1: chunks;
	
	
	char *recvBuf = calloc(msgLen, sizeof(*recvBuf));
	char *currentChunk = calloc(chunkSize, sizeof(*currentChunk));
	size_t receivedChunks = 0;
	size_t leftToReceive = msgLen;
	for (size_t i = 0; i < chunks; ++i) {
		err = recv(socket, currentChunk, chunkSize, 0);
		if (err == -1) {
			logr(warning, "chunkedReceive error: %s\n", strerror(errno));
			break;
		}
		size_t len = leftToReceive > chunkSize ? chunkSize : leftToReceive;
		memcpy(recvBuf + (i * chunkSize), currentChunk, len);
		receivedChunks++;
		leftToReceive -= min(len, chunkSize);
		memset(currentChunk, 0, chunkSize);
	}
	ASSERT(leftToReceive == 0);
	//logr(debug, "Received %lu chunks\n", receivedChunks);
	size_t finalLength = strlen(recvBuf) + 1; // +1 for null byte
	*data = recvBuf;
	return err == -1 ? -1 : finalLength;
}

const cJSON *encodeRenderer(struct renderer *r) {
	(void)r;
	return NULL;
}

struct renderer *decodeRenderer(const cJSON *json) {
	(void)json;
	return NULL;
}

const cJSON *encodeState() {
	cJSON *state = cJSON_CreateObject();
	
	return state;
}

const cJSON *errorResponse(char *error) {
	cJSON *errorMsg = cJSON_CreateObject();
	cJSON_AddStringToObject(errorMsg, "error", error);
	return errorMsg;
}

const cJSON *goodbye() {
	cJSON *goodbye = cJSON_CreateObject();
	cJSON_AddStringToObject(goodbye, "action", "goodbye");
	return goodbye;
}

const cJSON *dispatchCommand(struct renderClient *client, const cJSON *json) {
	const cJSON *action = cJSON_GetObjectItem(json, "action");
	if (!cJSON_IsString(action)) {
		return errorResponse("No action provided");
	}
	return goodbye();
	ASSERT_NOT_REACHED();
	return NULL;
}

const cJSON *makeHandshake() {
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
	ASSERT(isSet("cluster"));
	ASSERT(isSet("nodes"));
	char *nodesString = stringPref("nodes");
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
		clients[i].id = (int)i;
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

void *clientHandler(void *arg) {
	struct renderClient *client = (struct renderClient *)threadUserData(arg);
	
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		logr(warning, "Failed to bind to socket on client %i\n", client->id);
		return NULL;
	}
	if (connect(sockfd, (struct sockaddr *)&client->address, sizeof(client->address)) != 0) {
		logr(warning, "Connection failed on client %i\n", client->id);
		return NULL;
	}
	
	char *receiveBuffer = NULL;
	
	const cJSON *handshake = makeHandshake();
	char *content = cJSON_PrintUnformatted(handshake);
	if (chunkedSend(sockfd, content)) {
		logr(error, "chunkedSend() failed, error: %s\n", strerror(errno));
	}
	free(content);
	
	for (;;) {
		//ssize_t ret = recv(sockfd, receiveBuffer, MAXRCVLEN, 0);
		ssize_t ret = chunkedReceive(sockfd, &receiveBuffer);
		if (!ret) {
			logr(info, "Client %i received nothing.\n", client->id);
			break;
		}
		const cJSON *clientResponse = cJSON_Parse(receiveBuffer);
		const cJSON *serverResponse = dispatchCommand(client, clientResponse);
		char *responseText = cJSON_PrintUnformatted(serverResponse);
		//write(sockfd, responseText, strlen(responseText));
		if (chunkedSend(sockfd, responseText)) {
			logr(error, "chunkedSend() failed, error: %s\n", strerror(errno));
		}
		free(responseText);
		free(receiveBuffer);
		receiveBuffer = NULL;
		if (containsGoodbye(serverResponse)) {
			logr(debug, "Client %i said goodbye, exiting handler thread.\n", client->id);
			break;
		}
	}
	
	close(sockfd);
	
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
	char *buf = calloc(MAXRCVLEN, sizeof(*buf));
	
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
			const cJSON *message = cJSON_Parse(buf);
			const cJSON *myResponse = dispatchCommand(NULL, message);
			char *responseText = cJSON_PrintUnformatted(myResponse);
			logr(debug, "Responding     : %s\n", responseText);
			if (chunkedSend(connectionSocket, responseText)) {
				logr(warning, "chunkedSend() failed, error %s\n", strerror(errno));
				close(connectionSocket);
				break;
			};
			free(responseText);
			if (containsGoodbye(myResponse)) {
				close(connectionSocket);
				goto end;
			}
		}
	}
end:
	free(buf);
	close(receivingSocket);
	return 0;
}

void *handleClientSync(void *arg) {
	struct renderClient *client = (struct renderClient *)threadUserData(arg);
	ASSERT(client->state == Connected);
	client->state = Syncing;
	
	
	
	// Sync successful, mark it as such
	client->state = Synced;
	return NULL;
}

struct renderClient *syncWithClients(size_t *count) {
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
	
	struct crThread *syncThreads = calloc(clientCount, sizeof(*syncThreads));
	for (size_t i = 0; i < clientCount; ++i) {
		syncThreads[i] = (struct crThread){
			.threadFunc = handleClientSync,
			.userData = &clients[i]
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
