#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#include "serveroptions.h"
#include "parsing.h"
#include "server.h"

#ifndef SERVER_DEFAULT_PORT
const int DEFAULT_PORT = 5757;
#else
const int DEFAULT_PORT = SERVER_DEFAULT_PORT;
#endif

static void print_usage(char* progname) {
	fprintf(stderr, "Usage: %s --config path/to/config.json [--port n]\n", progname);
}

int main(int argc, char** argv) {
	serverOptions server_options = {
		.port = DEFAULT_PORT,
		.config_path = NULL
	};

	// parse cli flags
	while (true) {
		int option_index = 0;
		static struct option cli_options[] = {
			{"config" , required_argument, 0, 0},
			{"port"   , required_argument, 0, 0},
			{"help"   , no_argument      , 0, 0},
			{"version", no_argument      , 0, 0},
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
				}
		}
	}
	
	if (server_options.config_path == NULL) {
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}
	
	int exit_code = serverInstance_event_loop(server_options);
	return exit_code;
}
