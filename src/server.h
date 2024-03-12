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
#include <errno.h>

#include "serveroptions.h"

typedef struct {
	struct sockaddr_in address;
	socklen_t addrlen;
	SSL* ssl_object;
} serverClient;

typedef struct {
	char symbol;
	bool inherit;
} serverFlag;

typedef struct serverFeed {
	serverClient* clients;
	uint64_t num_clients;

	serverFlag* flags;
	uint32_t num_flags;
	
	struct serverFeed* parent_feed;
	struct serverFeed* subfeeds;
	uint64_t num_subfeeds;
} serverFeed;

typedef struct {
	serverClient** clients;
	uint32_t max_client;
} clientTable;

typedef struct {
	serverOptions options;
	int server_sockfd;
	serverFeed rootfeed;
	clientTable client_table;
	
	SSL_CTX* sslctx;
	int epollfd;

	int port;
	bool running;

	bool debug_mode;
} serverInstance;

void clientTable_index(clientTable* table, serverClient* client, int client_sockfd);
serverClient* clientTable_get(clientTable* table, int socketfd);

int serverInstance_event_loop(serverOptions options);

#endif
