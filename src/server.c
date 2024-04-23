#include "server.h"

#include "logging.h"
#include "wrapper.h"

#define MAX_EVENTS 10
#define MAX_RETRY 3

#define MSG_MAX 1024

serverClient* clientTable_get(clientTable* table, int socketfd) {
	return table->clients[socketfd];
}

void clientTable_index(clientTable* table, serverClient* client, int client_sockfd) {
	// TODO: Make this more efficient
	if (table->max_client <= client_sockfd) {
		size_t new_size = (client_sockfd + 1) * sizeof(serverClient*);
		table->clients = xrealloc(table->clients, new_size);
		table->max_client = client_sockfd;
	}
	table->clients[client_sockfd] = client;
}

static serverInstance serverInstance_init(serverOptions options) {
	int socket_type = AF_INET6;
	if (options.ipv4) {
		socket_type = AF_INET;
	}

	serverInstance result = {
		.server_sockfd = socket(socket_type, SOCK_STREAM | SOCK_NONBLOCK, 0),
		.port = options.port,
		.running = true,
		.sslctx = NULL,
		.client_table = {
			.clients = NULL,
			.max_client = 0,
		},
		.rootfeed = {
			.clients = NULL,
			.num_clients = 0,

			.subfeeds = NULL,
			.num_subfeeds = 0,

			.parent_feed = NULL,
		},
		.debug_mode = options.debug_mode
	};
	return result;
}

static void serverInstance_cleanup(serverInstance* server_instance) {
	// free the clients and close the client sockets
	for (int i = 0; i < server_instance->client_table.max_client; i++) {
		if (server_instance->client_table.clients[i] != NULL) {
			close(i);
			SSL_shutdown(server_instance->client_table.clients[i]->ssl_object);
			SSL_free(server_instance->client_table.clients[i]->ssl_object);
			free(server_instance->client_table.clients[i]);
		}
	}

	SSL_CTX_free(server_instance->sslctx);
	close(server_instance->epollfd);
}

static SSL_CTX* serverInstance_initSSL(serverInstance* server_instance, char* cert_path, char* key_path) {
	server_DEBUG(server_instance, "Initializing SSL/TLS");
	SSL_library_init();
	SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
	SSL_CTX_use_certificate_file(ctx, cert_path, SSL_FILETYPE_PEM);
	SSL_CTX_use_PrivateKey_file(ctx, key_path, SSL_FILETYPE_PEM);
	return ctx;
}

static void serverInstance_setup(serverInstance* server_instance, serverOptions options) {
	net_address address;

	if (options.ipv4) {
		address.ipv6addr = (struct sockaddr_in6) {
			.sin6_addr = in6addr_any,
			.sin6_port = htons(server_instance->port),
			.sin6_family = AF_INET6,
		};
	} else {
		address.ipv4addr = (struct sockaddr_in) {
			.sin_addr = {INADDR_ANY},
			.sin_port = htons(server_instance->port),
			.sin_family = AF_INET,
		};
	}

	int bind_result;
	if (options.ipv4) {
		bind_result = bind(server_instance->server_sockfd, (struct sockaddr*) &address.ipv4addr, sizeof(address.ipv4addr));
	} else {
		bind_result = bind(server_instance->server_sockfd, (struct sockaddr*) &address.ipv6addr, sizeof(address.ipv6addr));
	}

	if (bind_result == -1) {
		perror("server bind failed");
		exit(EXIT_FAILURE);
	}
	
	if (listen(server_instance->server_sockfd, MAX_RETRY) == -1) {
		perror("server listen failed");
		exit(EXIT_FAILURE);
	}
	
	// initialize the SSL context
	server_instance->sslctx = serverInstance_initSSL(
		server_instance, options.cert_path,
		options.key_path
	);

	if ((server_instance->epollfd = epoll_create1(0)) == -1) {
		perror("epoll_create1 failed");
		exit(EXIT_FAILURE);
	}
	
	struct epoll_event target_event = {
		.events = EPOLLIN,
		.data.fd = server_instance->server_sockfd
	};

	if (epoll_ctl(server_instance->epollfd, 
			EPOLL_CTL_ADD, server_instance->server_sockfd, &target_event) == -1) {
		perror("initial epoll_ctl failed");	
		exit(EXIT_FAILURE);
	} 
}

static int serverInstance_accept_connection(
			serverInstance* server_instance, 
			net_address* client_address, 
			socklen_t* addrlen,
			SSL* ssl_object
		) {

	struct sockaddr* client_addr = (struct sockaddr*) &client_address->ipv6addr;
	*addrlen = sizeof(client_address->ipv6addr);
	if (server_instance->options.ipv4) {
		client_addr = (struct sockaddr*) &client_address->ipv4addr;
		*addrlen = sizeof(client_address->ipv4addr);
	}
	
	// accept the connection
	int client_sockfd = accept(server_instance->server_sockfd, client_addr, addrlen);

	// if the connection stack is empty, break
	if (client_sockfd == -1) {
		if (errno != EWOULDBLOCK) {
			perror("Failed To Accept Connection");
		}
		return -1;
	}

	// add the new socket to the epoll set
	struct epoll_event target_event = {
		.events = EPOLLIN,
		.data.fd = client_sockfd,
	};

	if (epoll_ctl(server_instance->epollfd, EPOLL_CTL_ADD, client_sockfd, &target_event) == -1) {
		perror("epoll_ctl failed on client socket");
		return -1;
	}

	// perform TLS handshake
	// TODO: remember to put this in the client struct for future communication
	ssl_object = SSL_new(server_instance->sslctx);
	SSL_set_fd(ssl_object, client_sockfd);
	if (SSL_accept(ssl_object) == -1) {
		perror("TLS handshake failed");
		return -1;
	}

	return client_sockfd;
}

int serverInstance_event_loop(serverOptions options) {
	serverInstance main_instance = serverInstance_init(options);
	serverInstance_setup(&main_instance, options);

	struct epoll_event events[MAX_EVENTS];
	while (main_instance.running) {
		int ready_fds = epoll_wait(main_instance.epollfd, events, MAX_EVENTS, -1);	
		if (ready_fds == -1) {
			perror("epoll_wait failed");
			return EXIT_FAILURE;
		}

		for (int i = 0; i < ready_fds; i++) {
			int active_fd = events[i].data.fd;
			// this means that a client is initiating a connection with the listening socket
			if (active_fd == main_instance.server_sockfd) {
				server_DEBUG(&main_instance, "Accepting new client...");

				net_address client_address;
				socklen_t client_address_size = 0;
				SSL* sslobj = NULL;
				
				int client_sockfd = serverInstance_accept_connection(
					&main_instance, &client_address, &client_address_size, sslobj
				);

				if (client_sockfd == -1) {
					break;
				}

				// now we can create a new client object and add it to the root feed
				serverClient* new_client = xmalloc(sizeof(serverClient));
				new_client->address = client_address;
				new_client->addrlen = client_address_size;
				new_client->ssl_object = sslobj;
				// new_client->feed = init_feed 

				// add the client to the server's client table
				clientTable_index(&main_instance.client_table, new_client, client_sockfd);
			} else {
				// recieve / send data
				char buf[MSG_MAX];
				int bytes = SSL_read(clientTable_get(&main_instance.client_table, active_fd)->ssl_object, &buf, MSG_MAX - 1);

				if (bytes > 0) {
					// TODO: handle the message
				} else if (bytes == 0 ) {
					// TODO: handle the client disconnecting
				} else {
					// if the return value is -1, there was an error
					server_DEBUG(&main_instance, "SSL_read() error");
				}

				buf[bytes] = '\0';
				printf("%s\n", buf);
			}
		}
	}
	
	serverInstance_cleanup(&main_instance);
	return 0;
}
