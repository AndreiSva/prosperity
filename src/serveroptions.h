#ifndef _H_SERVEROPTIONS
#define _H_SERVEROPTIONS

#include <stdbool.h>

typedef struct {
	int port;
	char* config_path;
	bool debug_mode;

	char* cert_path;
	char* key_path;
} serverOptions;

#endif
