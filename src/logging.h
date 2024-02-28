#ifndef _H_SERVERLOGGING
#define _H_SERVERLOGGING

#include <string.h>
#include <stdlib.h>
#include <time.h>

void server_LOG(char* info_str);
void server_ERROR(char* error_str);

char* server_get_logstr(int* log_string_length);

#endif
