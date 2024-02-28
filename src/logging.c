#include <stdio.h>

#include "logging.h"

const int MAX_TIME_STRING_SIZE = 64;

char* server_get_logstr(int* log_string_length) {
	time_t t = time(NULL);
	struct tm* time_info = localtime(&t);

	char time_str[MAX_TIME_STRING_SIZE];
	int time_string_length = strftime(time_str, MAX_TIME_STRING_SIZE, "%D-%r", time_info);
	
	char* log_str = malloc(time_string_length + 256);
	*log_string_length = sprintf(log_str, "%s: ", time_str);

	return log_str;
}

void server_LOG(char* info_str) {
	int log_string_length = 0;
	char output_string[1024];
	char* log_str = server_get_logstr(&log_string_length);
	strcpy(output_string, log_str);
	strcat(output_string, info_str);

	printf("%s\n", output_string);
	free(log_str);
}
