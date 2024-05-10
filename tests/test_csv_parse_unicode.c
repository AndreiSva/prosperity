#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "../src/parsing.h"

#include "test_utils.h"

int main() {
	long length = 0;
	char* csv_source = read_from_file("tests/test_csv_parse_unicode.csv", &length);
	printf("%s\n", csv_source);
	CSValue csv = CSValue_parse(csv_source);
	free(csv_source);

	assert(strcmp(CSValue_get(&csv, 0, 0), "emoji") == 0);
	assert(strcmp(CSValue_get(&csv, 1, 0), "meaning") == 0);

	assert(strcmp(CSValue_get(&csv, 0, 1), "😭") == 0);
	assert(strcmp(CSValue_get(&csv, 1, 1), "sad human") == 0);

	assert(strcmp(CSValue_get(&csv, 0, 2), "❤️") == 0);
	assert(strcmp(CSValue_get(&csv, 1, 2), "love is in the air") == 0);
	CSValue_free(&csv);
}
