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
#include "parsing.h"

typedef struct sockaddr_in6 net_address;

typedef struct serverClient {
    int sockfd;
    net_address address;
    socklen_t addrlen;
    SSL* ssl_object;
    struct serverFeed* feed;
    struct serverClient* pnext_serverclient;
} serverClient;

typedef struct serverFeed {
    serverClient* client;

    char* feed_name;
    
    struct serverFeed* parent_feed;

    struct serverFeed* pnext_serverfeed;
    struct serverFeed* subfeeds;
} serverFeed;

typedef struct {
    serverClient** clients;
} clientTable;

typedef struct {
    serverOptions options;
    int server_sockfd;
    serverFeed* rootfeed;
    clientTable client_table;
    
    SSL_CTX* sslctx;
    int epollfd;

    int port;
    bool running;

    bool debug_mode;
} serverInstance;

void clientTable_index(clientTable* table, serverClient* client);
serverClient* clientTable_get(clientTable* table, int client_sockfd);
void clientTable_remove(clientTable* table, serverClient* client);

int serverInstance_event_loop(serverOptions options);

serverFeed* serverFeed_new(serverFeed* parent_feed, char* feed_name);
serverFeed* serverFeed_get_feed(serverFeed* parent_feed, char* feed_name);
serverFeed* serverFeed_get_byaddr(serverFeed* parent_feed, char* addr);
void serverFeed_add_subfeed(serverFeed* parent_feed, serverFeed* child_feed);
void serverFeed_delete_subfeed(serverFeed* parent_feed, uint64_t child_index);
void serverFeed_free(serverFeed* feed);
void serverFeed_send(serverFeed* recipient, CSValue* message);

#endif
