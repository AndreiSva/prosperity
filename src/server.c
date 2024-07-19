#include <signal.h>
#include <time.h>
#include <openssl/err.h>
#include <assert.h>

#include "server.h"

#include "logging.h"
#include "wrapper.h"
#include "parsing.h"
#include "error.h"

#define MAX_EVENTS 10
#define MAX_RETRY 3
#define ERROR_STRING_SIZE 256

#define MSG_MAX 1024

#define CLIENT_TABLE_SIZE 1024

#define GUEST_POSTFIX_MULTIPLIER 10000

void serverInstance_return_err(serverFeed* feed, uint16_t code, char* msg) {
    CSValue err = CSValue_parse(NULL);
    CSValue_append_row(&err, "SENDER", "MSGTYPE", "CODE", "CONTENT", NULL);

    char code_string[256];
    sprintf(code_string, "%d", code);
    
    CSValue_append_row(&err, "@", "STaMP-error", code_string, msg, NULL);

    serverFeed_send(feed, &err);
    CSValue_free(&err);
}

static void serverInstance_route_message(serverInstance* main_instance, CSValue* message, serverFeed* sender) {
    int msgtype_index = CSValue_index_colbyname(message, "MSGTYPE", 0);

    if (msgtype_index == -1) {
        serverInstance_return_err(sender, ERROR_NOTYPE, "ERROR_NOTYPE: No message type for message");
        return;
    }

    int destination_index = CSValue_index_colbyname(message, "DESTINATION", 0);

    if (destination_index == -1) {
        serverInstance_return_err(sender, ERROR_NODEST, "ERROR_NODEST: No destination feed for message");
        return;
    }

    for (int i = 1; i < message->rows; i++) {
        char* destination_address = CSValue_get(message, destination_index, i);
        serverFeed* destination = serverFeed_get_byaddr(main_instance->rootfeed, destination_address);

        if (destination == NULL) {
            serverInstance_return_err(sender, ERROR_INVALIDDEST, "ERROR_INVALIDDEST: Destination address for message could not be resolved to a valid feed");
            return;
        }

        serverFeed_send(destination, message);
    }

}

static serverClient* serverClient_new(net_address client_address,
                                      socklen_t client_address_size, SSL* sslobj) {
    serverClient* new_client = xmalloc(sizeof(serverClient));
    new_client->ssl_object = sslobj;
    new_client->address = client_address;
    new_client->addrlen = client_address_size;
    new_client->pnext_serverclient = NULL;
    return new_client;
}


static void serverClient_free(serverClient* client) {
	SSL_shutdown(client->ssl_object);
	SSL_free(client->ssl_object);

    serverClient* head = client->pnext_serverclient;
    if (head != NULL) {
        serverClient_free(head);
    }

    // note, we don't free the 'feed' field of the client struct because we expect it to be
    // freed before the client

    close(client->sockfd);
	free(client);
}

serverClient* clientTable_get(clientTable* table, int client_sockfd) {
    uint64_t hash = client_sockfd % CLIENT_TABLE_SIZE;
    serverClient* client = table->clients[hash];
    if (client != NULL) {
        serverClient* head = client->pnext_serverclient;
        while (head != NULL) {
            if (head->sockfd == client_sockfd) {
                return head;
            }
            head = head->pnext_serverclient;
        }

        return client;
    } else {
        return NULL;
    }
}

void clientTable_index(clientTable* table, serverClient* client) {
    uint64_t hash = client->sockfd % CLIENT_TABLE_SIZE;

    if (table->clients[hash] != NULL) {
        serverClient* head = table->clients[hash];
        while (head->pnext_serverclient != NULL) {
            head = head->pnext_serverclient;
        }
        head->pnext_serverclient = client;
    } else {
        table->clients[hash] = client;
    }
}

void clientTable_remove(clientTable* table, serverClient* client) {
    uint64_t hash = client->sockfd % CLIENT_TABLE_SIZE;
    serverClient* head = table->clients[hash];

    if (table->clients[hash]->pnext_serverclient != NULL) {
        while (head->pnext_serverclient != NULL) {
            if (head->pnext_serverclient->sockfd == client->sockfd) {
                serverClient* next_head = head->pnext_serverclient->pnext_serverclient;
                head->pnext_serverclient = next_head;
                break;
            } else {
                head = head->pnext_serverclient;
            }
        }
    } else {
        table->clients[client->sockfd] = NULL;
    }

	serverClient_free(client);
}

static serverInstance serverInstance_init(serverOptions options) {
	serverInstance result = {
		.server_sockfd = socket(AF_INET6, SOCK_STREAM | SOCK_NONBLOCK, 0),
		.port = options.port,
		.running = true,
		.sslctx = NULL,
		.client_table = {
			.clients = xmalloc(sizeof(serverClient*) * CLIENT_TABLE_SIZE),
		},
		.debug_mode = options.debug_mode
	};

    // create the root feed
    // this is where all server-wide events will propagate from
    result.rootfeed = serverFeed_new(NULL, NULL);

	int use_ipv4 = !options.ipv6_only;
	int sockopt_result = setsockopt(result.server_sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &use_ipv4, sizeof(use_ipv4));

	if (sockopt_result == -1) {
		perror("setsockopt failed");
		exit(EXIT_FAILURE);
	}
	return result;
}

static void serverInstance_cleanup(serverInstance* server_instance) {
	server_LOG("Shutting Down Server...");

    // free all the feeds
    serverFeed_free(server_instance->rootfeed);

	// free the clients and close the client sockets
	for (int i = 0; i < CLIENT_TABLE_SIZE; i++) {
		// free the remaining clients
        serverClient* current_client = clientTable_get(&server_instance->client_table, i);
		clientTable_remove(&server_instance->client_table, current_client);
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
	net_address address = (struct sockaddr_in6) {
		.sin6_addr = in6addr_any,
		.sin6_port = htons(server_instance->port),
		.sin6_family = AF_INET6,
	};

	int bind_result;
	server_DEBUG(server_instance, "Binding to port...");
	bind_result = bind(server_instance->server_sockfd, (struct sockaddr*) &address, sizeof(address));

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

	*addrlen = sizeof(*client_address);

	// accept the connection
	int client_sockfd = accept(server_instance->server_sockfd, (struct sockaddr*) client_address, addrlen);

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
        SSL_free(*ssl_object);
		return -1;
	}

	return client_sockfd;
}

static char* serverInstance_gen_guestname(serverFeed* root_feed) {
    static const char* guest_prefix = "Guest";

    int rng = random() * GUEST_POSTFIX_MULTIPLIER;
    int len = snprintf( NULL, 0, "%d", rng);
    char rng_string[len + 1];
    snprintf(rng_string, len + 1, "%d", rng);

    int size = strlen(guest_prefix) + len + 1;
    char* guestname = xmalloc(size);
    guestname[0] = '\0';

    strcat(guestname, guest_prefix);
    strcat(guestname, rng_string);

    // don't create feeds with duplicate names accidentally
    if (serverFeed_get_feed(root_feed, guestname) != NULL) {
        free(guestname);
        return serverInstance_gen_guestname(root_feed);
    }

    return guestname;
}

static bool* is_running = NULL;
static void handle_sigint(int sig) {
	*is_running = false;
}

// TODO: We probably want to have this take a serverInstance instead of serverOptions
int serverInstance_event_loop(serverOptions options) {
	serverInstance main_instance = serverInstance_init(options);
	serverInstance_setup(&main_instance, options);

	struct epoll_event events[MAX_EVENTS];
    srand(time(NULL));

	// handle ctrl-c
	is_running = &main_instance.running;
	struct sigaction sigint_action = {
		.sa_handler = handle_sigint,
		.sa_flags = 0,
	};
	sigaction(SIGINT, &sigint_action, NULL);

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

				// now we can create a new client object, create a new feed and add the new feed to the root feed
				serverClient* new_client = serverClient_new(client_address, client_address_size, sslobj);

                serverFeed* new_feed = serverFeed_new(main_instance.rootfeed, serverInstance_gen_guestname(main_instance.rootfeed));
                new_feed->client = new_client;

                new_client->feed = new_feed;
                new_client->sockfd = client_sockfd;

				// add the client to the server's client table
				clientTable_index(&main_instance.client_table, new_client);
			} else {
				// recieve / send data
				char buf[MSG_MAX];
				size_t bytes = 0;
				int read_result = SSL_read_ex(clientTable_get(&main_instance.client_table, active_fd)->ssl_object, &buf, MSG_MAX - 1, &bytes);

				if (!read_result) {
					SSL* active_ssl = clientTable_get(&main_instance.client_table, active_fd)->ssl_object;
					int error = SSL_get_error(active_ssl, read_result);

					// these are retryable errors
					if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE) {
						continue;
					}

					// unrecoverable errors :(
					if (error == SSL_ERROR_SSL || error == SSL_ERROR_SYSCALL) {
						server_DEBUG(&main_instance, "Client Disconnected");

                        serverClient* active_client = clientTable_get(&main_instance.client_table, active_fd);
						clientTable_remove(
                            &main_instance.client_table, active_client
                        );
						continue;
					}
					
					if (main_instance.debug_mode) {
						char error_string[ERROR_STRING_SIZE];
						ERR_error_string_n(ERR_get_error(), error_string, ERROR_STRING_SIZE);
						server_DEBUG(&main_instance, error_string);
					}
				}

				if (bytes > 0) {
					// TODO: handle the message
					server_DEBUG(&main_instance, "Received data from client");
					buf[bytes] = '\0';

					CSValue client_data = CSValue_parse(buf);
					CSValue_put(stdout, &client_data);

                    serverInstance_route_message(&main_instance, &client_data, clientTable_get(&main_instance.client_table, active_fd)->feed);

					CSValue_free(&client_data);
				} else if (bytes == 0 ) {
					server_DEBUG(&main_instance, "Client Disconnected");

                    serverClient* active_client = clientTable_get(&main_instance.client_table, active_fd);
					clientTable_remove(&main_instance.client_table, active_client);
				}
			}
		}
	}
	
	serverInstance_cleanup(&main_instance);
	return 0;
}
