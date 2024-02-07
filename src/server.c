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
		.port = options.port,
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

	SSL_library_init();
	SSL_CTX* ctx = SSL_CTX_new(SSLv23_server_method());
	SSL_CTX_use_certificate_file(ctx, options.cert_path, SSL_FILETYPE_PEM);
	SSL_CTX_use_PrivateKey_file(ctx, options.key_path, SSL_FILETYPE_PEM);
	
	struct sockaddr_in address = {
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(main_instance.port),
		.sin_family = AF_INET,
	};

	if (bind(main_instance.server_sockfd, (struct sockaddr*) &address, sizeof(address)) == -1) {
		perror("server bind failed");
		return EXIT_FAILURE;
	}

	int epollfd = epoll_create1(0);
	while (main_instance.running) {
		
	}
	
	close(epollfd);
	serverInstance_cleanup(&main_instance);
	return 0;
}
