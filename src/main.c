#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "serveroptions.h"
#include "parsing.h"
#include "server.h"
#include "logging.h"

#ifndef DEFAULT_PORT
#define DEFAULT_PORT 5757
#endif

static void print_usage(char* progname) {
	fprintf(stderr, "Usage: %s --key <KeyFile.pem> --cert <CertFile.pem> [--port n]\n", progname);
}

int main(int argc, char** argv) {
	serverOptions server_options = {
		.port = DEFAULT_PORT,
		.config_path = NULL,
		.debug_mode = false,

		.cert_path = NULL,
		.key_path = NULL,
	};

	// parse cli flags
	while (true) {
		int option_index = 0;
		static struct option cli_options[] = {
			{"port"   , required_argument, 0, 0},
			{"cert"   , required_argument, 0, 0},
			{"key"    , required_argument, 0, 0},
			{"help"   , no_argument      , 0, 0},
			{"version", no_argument      , 0, 0},
			{"debug"  , no_argument      , 0, 0},
			{0        , 0                , 0, 0}
		};

		int option = getopt_long(argc, argv, "", cli_options, &option_index);
		if (option == -1) {
			break;
		}
		
		struct option current_option = cli_options[option_index];
		switch (option)	{
			case 0:
				if (strcmp(current_option.name, "config") == 0) {
					server_options.config_path = optarg;
				} else if (strcmp(current_option.name, "port") == 0) {
					if (!isnumber(optarg)) {
						print_usage(argv[0]);
						exit(EXIT_FAILURE);
					}
					server_options.port = atoi(optarg);
				} else if (strcmp(current_option.name, "cert") == 0) {
					server_options.cert_path = optarg;
				} else if (strcmp(current_option.name, "key") == 0) {
					server_options.key_path = optarg;
				} else if (strcmp(current_option.name, "debug") == 0) {
					server_options.debug_mode = true;
				}
		}
	}

	if (server_options.key_path == NULL || server_options.cert_path == NULL) {
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	server_LOG("Starting Server...");
	int exit_code = serverInstance_event_loop(server_options);
	return exit_code;
}
