#ifndef _H_SERVER
#define _H_SERVER

#include <poll.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "serveroptions.h"

typedef struct {
	int client_sockfd;
	struct sockaddr_in address;
} serverClient;

typedef struct serverFeed {
	serverClient* clients;
	uint64_t num_clients;

	struct serverFeed* subfeeds;
	uint64_t num_subfeeds;
} serverFeed;

typedef struct {
	int server_sockfd;
	
} serverInstance;

int serverInstance_event_loop(serverOptions options);

#endif
