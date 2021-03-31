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
#include <errno.h>
#include "fileio.h"

#define C_RAY_HEADERSIZE 8
#define C_RAY_CHUNKSIZE 1024
#define C_RAY_PORT 2222

#include <stdio.h>
bool chunkedSend(int socket, const char *data) {
	const size_t msgLen = strlen(data) + 1; // +1 for null byte
	const size_t chunkSize = C_RAY_CHUNKSIZE;
	size_t chunks = msgLen / chunkSize;
	chunks = (msgLen % chunkSize) != 0 ? chunks + 1: chunks;
	logr(debug, "Sending %lu bytes (%lu chunks)\n", msgLen, chunks);
	
	// Send header with message length
	size_t header = htonll(msgLen);
	ssize_t err = send(socket, &header, sizeof(header), 0);
	if (err < 0) logr(error, "Failed to send header to client.\n");
	
	ssize_t n = 0;
	size_t sentChunks = 0;
	char *currentChunk = calloc(chunkSize, sizeof(*currentChunk));
	size_t leftToSend = msgLen;
	for (size_t i = 0; i < chunks; ++i) {
		size_t copylen = min(leftToSend, chunkSize);
		memcpy(currentChunk, data + (i * chunkSize), copylen);
		//printf("chunk %lu: \"%.1024s\"\n", i, currentChunk);
		n = send(socket, currentChunk, chunkSize, SO_NOSIGPIPE);
		if (n == -1) {
			logr(warning, "chunkedSend error: %s\n", strerror(errno));
			free(currentChunk);
			return -1;
		}
		ASSERT(n == chunkSize);
		sentChunks++;
		leftToSend -= min(copylen, chunkSize);
		memset(currentChunk, 0, chunkSize);
	}
	ASSERT(leftToSend == 0);
	logr(debug, "Sent %lu chunks\n", sentChunks);
	return n == -1 ? true : false;
}

ssize_t chunkedReceive(int socket, char **data) {
	// Grab header first
	//TODO: Verify we get the full header length before proceeding
	size_t headerData = 0;
	size_t chunkSize = C_RAY_CHUNKSIZE;
	ssize_t ret = recv(socket, &headerData, sizeof(headerData), 0);
	if (headerData == 0) {
		logr(warning, "Remote closed connection.\n");
		return 0;
	}
	if (ret == -1) logr(warning, "chunkedReceive header error: %s\n", strerror(errno));
	size_t msgLen = ntohll(headerData);
	size_t chunks = msgLen / chunkSize;
	chunks = (msgLen % chunkSize) != 0 ? chunks + 1: chunks;
	logr(debug, "Received header: %lu (should be %lu chunks)\n", msgLen, chunks);
	
	char *recvBuf = calloc(msgLen, sizeof(*recvBuf));
	char *currentChunk = calloc(chunkSize, sizeof(*currentChunk));
	char *scratchBuf = calloc(chunkSize, sizeof(*scratchBuf));
	size_t receivedChunks = 0;
	size_t leftToReceive = msgLen;
	for (size_t i = 0; i < chunks; ++i) {
		size_t chunkLeftToReceive = chunkSize;
		while (chunkLeftToReceive > 0) {
			ret = recv(socket, scratchBuf, chunkLeftToReceive, 0);
			if (ret == -1) {
				logr(warning, "chunkedReceive error: %s\n", strerror(errno));
				goto bail;
			}
			memcpy(currentChunk + (chunkSize - chunkLeftToReceive), scratchBuf, ret);
			chunkLeftToReceive -= ret;
			memset(scratchBuf, 0, chunkSize);
		}
		size_t len = leftToReceive > chunkSize ? chunkSize : leftToReceive;
		memcpy(recvBuf + (i * chunkSize), currentChunk, len);
		//printf("chunk %lu: \"%.1024s\"\n", i, currentChunk);
		receivedChunks++;
		leftToReceive -= min(len, chunkSize);
		memset(currentChunk, 0, chunkSize);
	}
bail:
	ASSERT(leftToReceive == 0);
	size_t finalLength = strlen(recvBuf) + 1; // +1 for null byte
	if (finalLength < msgLen) logr(error, "Chunked transfer failed. Header size of %lu != %lu. This shouldn't happen.\n", msgLen, finalLength);
	logr(debug, "Received %lu chunks, %s\n", receivedChunks, humanFileSize(finalLength));
	*data = recvBuf;
	free(currentChunk);
	free(scratchBuf);
	return ret == -1 ? -1 : finalLength;
}

#else
bool chunkedSend(int socket, const char *data) {
	logr(error, "c-ray doesn't support the proprietary networking stack on Windows. Sorry!\n");
}

ssize_t chunkedReceive(int socket, char **data) {
	logr(error, "c-ray doesn't support the proprietary networking stack on Windows. Sorry!\n");
}
#endif
