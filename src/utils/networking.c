//
//  networking.c
//  C-ray
//
//  Created by Valtteri on 5.1.2020.
//  Copyright © 2020 Valtteri Koskivuori. All rights reserved.
//

#include "../includes.h"
#include "networking.h"

#include "../utils/logging.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define MAXRCVLEN 1048576 // 1MiB
#define C_RAY_PORT 2222

int start_interactive() {
	socklen_t len;
	int sockfd, connfd;
	struct sockaddr_in server_address, client_address;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		logr(error, "Socket creation failed.\n");
	}
	
	bzero(&server_address, sizeof(server_address));
	
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = htons(C_RAY_PORT);
	
	int opt_val = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));
	
	if ((bind(sockfd, (struct sockaddr*)&server_address, sizeof(server_address))) != 0) {
		logr(error, "Failed to bind to socket\n");
	}
	
	if (listen(sockfd, 5) != 0) {
		logr(error, "It wouldn't listen\n");
	}
	logr(debug, "Listening on port %i\n", C_RAY_PORT);
	
	len = sizeof(client_address);
	char *buf = calloc(MAXRCVLEN, sizeof(char));
	
	bool active = true;
	
	while (active) {
		connfd = accept(sockfd, (struct sockaddr*)&client_address, &len);
		if (connfd < 0) {
			logr(error, "Failed to accept\n");
		}
		logr(debug, "Got connection from %s\n", inet_ntoa(client_address.sin_addr));
		
		while (active) {
			size_t read = recv(connfd, buf, MAXRCVLEN, 0);
			if (!read) break;
			if (read < 0) logr(error, "Read failed\n");
			
			char *dst = calloc(read, sizeof(char));
			memcpy(dst, buf, read);
			logr(debug, "Got from client: %s\n", dst);
			if (strncmp(dst, "exit", 4) == 0) {
				active = false;
			}
			free(dst);
			
			size_t err = send(connfd, buf, read, 0);
			if (err < 0) logr(error, "Write failed\n");
			close(connfd);
			break;
		}
	}
	
	close(sockfd);
	return 0;
}

