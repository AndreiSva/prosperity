#include "server.h"

serverClient* clientTable_get(clientTable table, int socketfd) {
	return table.clients[socketfd];
}

void clientTable_index(clientTable table, serverClient* client) {
	if (table.max_client < client->client_sockfd) {
		table.clients = realloc(table.clients, client->client_sockfd);
	}
	table.clients[client->client_sockfd] = client;
}

static serverInstance serverInstance_init(serverOptions options) {
	serverInstance result = {
		.server_sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0),
		.running = true,
		.rootfeed = {
			.clients = NULL,
			.num_clients = 0,

			.subfeeds = NULL,
			.num_subfeeds = 0,

			.parent_feed = NULL,
		}
	};
	return result;
}

static void serverInstance_cleanup(serverInstance* server_instance) {

}

int serverInstance_event_loop(serverOptions options) {
	serverInstance main_instance = serverInstance_init(options);

	int epollfd = epoll_create1(0);
	while (main_instance.running) {
		
	}

	return 0;
}
