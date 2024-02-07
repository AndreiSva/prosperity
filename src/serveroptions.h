#ifndef _H_SERVEROPTIONS
#define _H_SERVEROPTIONS

typedef struct {
	int port;
	char* config_path;

	char* cert_path;
	char* key_path;
} serverOptions;

#endif
