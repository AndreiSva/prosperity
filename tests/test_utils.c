#include "test_utils.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* read_from_file(char* filename, long* length) {
	if (access(filename, F_OK) != 0) {
		perror(filename);
		exit(EXIT_FAILURE);
	}

	FILE* file = fopen(filename, "r");
	fseek(file, 0, SEEK_END);
	long size_bytes = ftell(file);
	*length = size_bytes;
	rewind(file);
	
	char* contents = malloc(size_bytes + 1);
	if (contents == NULL) {
		fputs("error allocating memory", stderr);
		exit(EXIT_FAILURE);
	}
	
	fread(contents, 1, size_bytes, file);
	if (ferror(file)) {
		fputs("unable to read file", stderr);
		free(contents);
		exit(EXIT_FAILURE);
	}

	contents[size_bytes] = '\0';
	fclose(file);
	return contents;
}
