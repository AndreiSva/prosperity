#include "server.h"

#define MAX_EVENTS 10
#define MAX_RETRY 3

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
		.sslctx = NULL,
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
	SSL_CTX_free(server_instance->sslctx);
}

static SSL_CTX* serverInstance_initSSL(serverInstance* server_instance, char* cert_path, char* key_path) {
	SSL_library_init();
	SSL_CTX* ctx = SSL_CTX_new(SSLv23_server_method());
	SSL_CTX_use_certificate_file(ctx, cert_path, SSL_FILETYPE_PEM);
	SSL_CTX_use_PrivateKey_file(ctx, key_path, SSL_FILETYPE_PEM);
	return ctx;
}

static void serverInstance_setup(serverInstance* server_instance, serverOptions options) {
	struct sockaddr_in address = {
		.sin_addr.s_addr = INADDR_ANY,
		.sin_port = htons(server_instance->port),
		.sin_family = AF_INET,
	};

	if (bind(server_instance->server_sockfd, (struct sockaddr*) &address, sizeof(address)) == -1) {
		perror("server bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(server_instance->server_sockfd, MAX_RETRY) == -1) {
		perror("server listen failed");
		exit(EXIT_FAILURE);
	}

	server_instance->sslctx = serverInstance_initSSL(
			server_instance, options.cert_path,
			options.key_path
	);
}

int serverInstance_event_loop(serverOptions options) {
	serverInstance main_instance = serverInstance_init(options);
	serverInstance_setup(&main_instance, options);

	int epollfd = epoll_create1(0);
	if (epollfd == -1) {
		perror("epoll_create1 failed");
		return EXIT_FAILURE;
	}
	
	struct epoll_event target_event = {
		.events = EPOLLIN,
		.data.fd = main_instance.server_sockfd
	};

	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, main_instance.server_sockfd, &target_event) == -1) {
		perror("initial epoll_ctl failed");	
		return EXIT_FAILURE;
	} 
	
	struct epoll_event events[MAX_EVENTS];
	while (main_instance.running) {
		int ready_fds = epoll_wait(epollfd, events, MAX_EVENTS, -1);	
		if (ready_fds == -1) {
			perror("epoll_wait failed");
			return EXIT_FAILURE;
		}

		for (int i = 0; i < MAX_EVENTS; i++) {
			if (events[i].data.fd == main_instance.server_sockfd) {
				// accept a new connection
			} else {
				// recieve / send data
			}
		}
	}
	
	close(epollfd);
	serverInstance_cleanup(&main_instance);
	return 0;
}
