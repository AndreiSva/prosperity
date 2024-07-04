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

typedef struct sockaddr_in6 net_address;

typedef struct {
	net_address address;
	socklen_t addrlen;
	SSL* ssl_object;
} serverClient;

typedef struct serverFlag {
	char* flag_name;
	char* flag_value;
	
	struct serverFlag* subflags;
	struct serverFlag* pnext_flag;

	bool inherit;
} serverFlag;

typedef struct serverFeed {
	serverClient* client;
	serverFlag* flags;
	
	struct serverFeed* parent_feed;

	struct serverFeed* pnext_serverfeed;
	struct serverFeed* subfeeds;
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
serverClient* clientTable_get(clientTable* table, int client_sockfd);
void clientTable_remove(clientTable* table, int client_sockfd);

int serverInstance_event_loop(serverOptions options);

serverFlag* serverFlag_new(char* flag_name, char* flag_value);
serverFlag* serverFlag_get_byaddr(serverFlag* root_flag, char* addr);
void serverFlag_add_subflag(serverFlag* parent_flag, serverFlag* subflag);
void serverFlag_free(serverFlag* flag);

serverFeed* serverFeed_new(serverFeed* parent_feed, serverFlag* flags);
void serverFeed_add_subfeed(serverFeed* parent_feed, serverFeed* child_feed);
void serverFeed_delete_subfeed(serverFeed* parent_feed, uint64_t child_index);
void serverFeed_free(serverFeed* feed);

#endif
