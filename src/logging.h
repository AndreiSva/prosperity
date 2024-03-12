#ifndef _H_SERVERLOGGING
#define _H_SERVERLOGGING

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

#include "server.h"

void server_LOG(char* info_str);
void server_DEBUG(serverInstance* server_instance, char* debug_str);

char* server_get_logstr(int* log_string_length);

#endif
