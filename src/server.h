#ifndef _H_SERVER
#define _H_SERVER

#include <sys/epoll.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <openssl/ssl.h>
#include <unistd.h>
#include <stdlib.h>

#include "serveroptions.h"

typedef struct {
	int client_sockfd;
	struct sockaddr_in address;
} serverClient;

typedef struct serverFeed {
	serverClient* clients;
	uint64_t num_clients;
	
	struct serverFeed* parent_feed;
	struct serverFeed* subfeeds;
	uint64_t num_subfeeds;
} serverFeed;

typedef struct {
	serverClient** clients;
	int max_client;
} clientTable;

typedef struct {
	int server_sockfd;
	serverFeed rootfeed;
	
	SSL_CTX* sslctx;
	int port;
	bool running;
} serverInstance;

void clientTable_index(clientTable table, serverClient* client);
serverClient* clientTable_get(clientTable table, int socketfd);

int serverInstance_event_loop(serverOptions options);

#endif
