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

	assert(strcmp(CSValue_get(&csv, 0, 0), "name") == 0);
	assert(strcmp(CSValue_get(&csv, 1, 0), "species") == 0);
	assert(strcmp(CSValue_get(&csv, 2, 0), "age") == 0);

	assert(strcmp(CSValue_get(&csv, 0, 1), "Mittens") == 0);
	assert(strcmp(CSValue_get(&csv, 1, 1), "cat") == 0);
	assert(strcmp(CSValue_get(&csv, 2, 1), "7") == 0);

	assert(strcmp(CSValue_get(&csv, 0, 2), "Roger") == 0);
	assert(strcmp(CSValue_get(&csv, 1, 2), "dog") == 0);
	assert(strcmp(CSValue_get(&csv, 2, 2), "4") == 0);

	assert(strcmp(CSValue_get(&csv, 0, 3), "Bob") == 0);
	assert(strcmp(CSValue_get(&csv, 1, 3), "human") == 0);
	assert(strcmp(CSValue_get(&csv, 2, 3), "32") == 0);
}

