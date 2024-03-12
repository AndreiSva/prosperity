#include "parsing.h"

bool isnumber(char* string) {
	for (int i = 0; i < strlen(string); i++) {
		if (!isdigit(string[i])) {
			return false;
		}
	}
	return true;
}

char* CSValue_get(CSValue* csv, uint32_t col, uint32_t row) {
	bool valid_col = col < csv->cols && col > 0;
	bool valid_row = row < csv->rows && row > 0;

	if (!valid_col || !valid_row) {
		fputs("Invalid csv index", stderr);
	}

	return csv->table[col][row];
}

CSValue CSValue_parse(char* csv_string) {
	CSValue csv = {
		.cols = 0,
		.rows = 0,
		.table = NULL
	};
	
	char* line = NULL;
	while ((line = strtok(csv_string, "\n")) != NULL) {
		char* token = NULL;
		while ((token = strtok(line, ",")) != NULL) {

		}
	}

	return csv;
}
