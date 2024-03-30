#include "parsing.h"

#include <stdlib.h>
#include <stdbool.h>

#define CSV_CELL_SIZE 256

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
		.table = malloc(sizeof(char**))
	};

	char* line_saveptr = csv_string;
	char* line = strtok_r(csv_string, "\n", &line_saveptr);
	while (line != NULL) {
		// initialize the new row;
		csv.rows++;
		for (int col_index = 0; col_index < csv.cols; col_index++) {
			csv.table[col_index] = realloc(csv.table[col_index], sizeof(char*) * csv.rows);
		}

		char* cell = malloc(CSV_CELL_SIZE);
		int cell_string_length = 0;
		bool quoted = false;
		for (int current_pos = 0; current_pos < strlen(line); current_pos++) {
			switch (line[current_pos]) {
				case ',':
					if (!quoted) {
						csv.cols++;
						csv.table = realloc(csv.table, sizeof(char**) * csv.cols);
						// init the rows for the new col
						csv.table[csv.cols] = malloc(sizeof(char*) * csv.rows);
						csv.table[csv.cols][csv.rows] = cell;
						cell_string_length = 0;
						cell = malloc(CSV_CELL_SIZE);
					}
					break;
				case '\"':
					if (line[current_pos - 1] == ',') {
						quoted = true;
					}

					if (line[current_pos + 1] == ',') {
						quoted = false;
					}
					break;
				default:
					if (cell_string_length >= sizeof(cell)) {
						cell_string_length += CSV_CELL_SIZE;
						cell = realloc(cell, cell_string_length);
					}
					cell[cell_string_length] = line[current_pos];
					cell_string_length++;
			}
		}

		line = strtok_r(NULL, "\n", &line_saveptr);
	}

	return csv;
}
