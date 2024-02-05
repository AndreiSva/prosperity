#include "server.h"

static serverInstance serverInstance_init(serverOptions options) {
	serverInstance result = {
		.server_sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0),
	};
}

int serverInstance_event_loop(serverOptions options) {

	return 0;
}
