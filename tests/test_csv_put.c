#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "../src/parsing.h"

#include "test_utils.h"

int main() {
	long length = 0;
	char* csv_source = read_from_file("tests/test_csv_parse.csv", &length);
	printf("%s\n", csv_source);
	CSValue csv = CSValue_parse(csv_source);

	FILE* csv_file = tmpfile();
	CSValue_put(csv_file, &csv);
	rewind(csv_file);
	fseek(csv_file, 0, SEEK_END);
	long csv_str_size = ftell(csv_file);
	rewind(csv_file);

	char csv_str[csv_str_size];
	fread(csv_str, 1, csv_str_size, csv_file);
	csv_str[csv_str_size] = '\0';

	assert(strcmp(csv_str, csv_source) == 0);
	free(csv_source);
}

