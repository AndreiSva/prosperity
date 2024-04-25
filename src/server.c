#include "server.h"

#include "logging.h"
#include "wrapper.h"
#include "parsing.h"

#define MAX_EVENTS 10
#define MAX_RETRY 3

#define MSG_MAX 1024

void serverClient_free(serverClient *client, int client_sockfd) {
	close(client_sockfd);
	SSL_shutdown(client->ssl_object);
	SSL_free(client->ssl_object);
	free(client);
}

serverClient* clientTable_get(clientTable* table, int client_sockfd) {
	return table->clients[client_sockfd];
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

void clientTable_remove(clientTable* table, int client_sockfd) {
	serverClient* removed_client = clientTable_get(table, client_sockfd);
	serverClient_free(removed_client, client_sockfd);
	table->clients[client_sockfd] = NULL;
	if (client_sockfd == table->max_client) {
		for (int i = client_sockfd; i >= 0; i--) {
			if (table->clients[i] != NULL) {
				table->max_client = i;
				break;
			}
		}
	}
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
			.client = NULL,
			.subfeeds = NULL,
			.parent_feed = NULL,
		},
		.debug_mode = options.debug_mode
	};
	return result;
}

static void serverInstance_cleanup(serverInstance* server_instance) {
	// free the clients and close the client sockets
	for (int i = 0; i < server_instance->client_table.max_client; i++) {
		// free the remaining clients
		serverClient* current_client = server_instance->client_table.clients[i];
		if (current_client != NULL) {
			// remember the index of the client in the table is it's sockfd
			serverClient_free(current_client, i);
		}
	}

	SSL_CTX_free(server_instance->sslctx);
	close(server_instance->epollfd);
	close(server_instance->server_sockfd);
	free(server_instance->client_table.clients);
}

static SSL_CTX* serverInstance_initSSL(serverInstance* server_instance, char* cert_path, char* key_path) {
	server_DEBUG(server_instance, "Initializing SSL/TLS");
	SSL_library_init();
	SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());

	int cert_result = SSL_CTX_use_certificate_file(ctx, cert_path, SSL_FILETYPE_PEM);
	if (cert_result != 1) {
		fprintf(stderr, "Failed to load certificate file: %s\n", cert_path);
		exit(EXIT_FAILURE);
	}

	int key_result = SSL_CTX_use_PrivateKey_file(ctx, key_path, SSL_FILETYPE_PEM);
	if (key_result != 1) {
		fprintf(stderr, "Failed to load key file: %s\n", key_path);
		exit(EXIT_FAILURE);
	}

	int key_check = SSL_CTX_check_private_key(ctx);
	if (key_check != 1) {
		fprintf(stderr, "Key and certificate do not match\n");
		exit(EXIT_FAILURE);
	}

	return ctx;
}

static void serverInstance_setup(serverInstance* server_instance, serverOptions options) {
	net_address address;

	if (options.ipv4) {
		address.ipv4addr = (struct sockaddr_in) {
			.sin_addr = {INADDR_ANY},
			.sin_port = htons(server_instance->port),
			.sin_family = AF_INET,
		};
	} else {
		address.ipv6addr = (struct sockaddr_in6) {
			.sin6_addr = in6addr_any,
			.sin6_port = htons(server_instance->port),
			.sin6_family = AF_INET6,
		};
	}

	int bind_result;
	server_DEBUG(server_instance, "Binding to port...");
	if (options.ipv4) {
		server_DEBUG(server_instance, "Using IPv4");
		bind_result = bind(server_instance->server_sockfd, (struct sockaddr*) &address.ipv4addr, sizeof(address.ipv4addr));
	} else {
		server_DEBUG(server_instance, "Using IPv6");
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
			SSL** ssl_object
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
	*ssl_object = SSL_new(server_instance->sslctx);
	SSL_set_fd(*ssl_object, client_sockfd);
	if (SSL_accept(*ssl_object) == -1) {
		perror("TLS handshake failed");
		return -1;
	}

	return client_sockfd;
}

// TODO: We probably want to have this take a serverInstance instead of serverOptions
int serverInstance_event_loop(serverOptions options) {
	serverInstance main_instance = serverInstance_init(options);
	serverInstance_setup(&main_instance, options);

	struct epoll_event events[MAX_EVENTS];
	while (main_instance.running) {
		int ready_fds = epoll_wait(main_instance.epollfd, events, MAX_EVENTS, -1);	
		if (ready_fds == -1) {
			// Unix has some interesting behavior with signals and epoll_wait
			if (errno == EINTR) {
				continue;
			}

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
					&main_instance, &client_address, &client_address_size, &sslobj
				);

				if (client_sockfd == -1) {
					break;
				}

				// now we can create a new client object and add it to the root feed
				serverClient* new_client = xmalloc(sizeof(serverClient));
				new_client->ssl_object = sslobj;
				new_client->address = client_address;
				new_client->addrlen = client_address_size;

				// add the client to the server's client table
				clientTable_index(&main_instance.client_table, new_client, client_sockfd);
			} else {
				// recieve / send data
				char buf[MSG_MAX];
				size_t bytes = 0;
				int read_result = SSL_read_ex(clientTable_get(&main_instance.client_table, active_fd)->ssl_object, &buf, MSG_MAX - 1, &bytes);

				if (!read_result) {
					server_DEBUG(&main_instance, "SSL_read_ex failed");
					SSL* active_ssl = clientTable_get(&main_instance.client_table, active_fd)->ssl_object;
					int error = SSL_get_error(active_ssl, read_result);

					// these are retryable errors
					if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
						continue;
					}
				}

				if (bytes > 0) {
					// TODO: handle the message
					server_DEBUG(&main_instance, "Received data from client");
					buf[bytes] = '\0';

					CSValue client_data = CSValue_parse(buf);
					CSValue_put(stdout, &client_data);

					CSValue_free(&client_data);
				} else if (bytes == 0 ) {
					server_DEBUG(&main_instance, "Client Disconnected");
					clientTable_remove(&main_instance.client_table, active_fd);
				}
			}
		}
	}
	
	serverInstance_cleanup(&main_instance);
	return 0;
}
