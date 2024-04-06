#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "../src/parsing.h"

#include "test_utils.h"

int main() {
	long length = 0;
	char* csv_source = read_from_file("tests/test_csv_parse1.csv", &length);
	printf("%s\n", csv_source);
	CSValue csv = CSValue_parse(csv_source);

	puts(CSValue_get(&csv, 1, 0));
	puts(CSValue_get(&csv, 0, 1));
	puts(CSValue_get(&csv, 0, 0));
	assert(strcmp(CSValue_get(&csv, 0, 0), "name") == 0);
}

