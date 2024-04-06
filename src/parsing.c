#include "parsing.h"

#include <stddef.h>
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
	bool valid_col = col < csv->cols && col >= 0;
	bool valid_row = row < csv->rows && row >= 0;

	if (!valid_col || !valid_row) {
		fputs("Invalid csv index", stderr);
	}

	return csv->table[col][row];
}

// CSValue is supposed to be immutable once parsed, so this can be static
static void CSValue_edit_cell(CSValue* csv, char* cell, uint32_t col, uint32_t row) {
	// if we want to add a cell outside of what we've allocated already, allocate the new memory

	// cols
	if (csv->cols <= col) {
		int old_size = csv->cols;
		csv->cols = col + 1;
		csv->table = realloc(csv->table, sizeof(char**) * csv->cols);
		for (int i = old_size; i < csv->cols; i++) {
				// initialize every row in the new columns to NULL
				// this is so we can free() them without causing issues later
				csv->table[i] = calloc(csv->rows, sizeof(char*));
		}
	}
	
	// rows
	if (csv->rows <= row) {
		int old_size = csv->rows;
		csv->rows = row + 1;
		for (int i = 0; i < csv->cols; i++) {
			// grow the column at index i
			csv->table[i] = realloc(csv->table[i], sizeof(char*) * csv->rows);

			// the new row cells should be NULL
			for (int j = old_size; j < csv->rows; j++) {
				csv->table[i][j] = NULL;
			}
		}
	}

	free(csv->table[col][row]);
	char* cell_string = malloc(strlen(cell) + 1);
	strcpy(cell_string, cell);
	csv->table[col][row] = cell_string;
}

CSValue CSValue_parse(char* csv_string) {
	CSValue csv = {
		.cols = 1,
		.rows = 1,
		.table = malloc(sizeof(char**))
	};

	csv.table[0] = calloc(1, sizeof(char*));

	char* line_saveptr = csv_string;
	char* line = strtok_r(csv_string, "\n", &line_saveptr);


	int current_col = -1; // set this to -1 because it will get incremented back to 0 right before we add the cell
	int current_row = 0;
	while (line != NULL) {
		char* cell = malloc(CSV_CELL_SIZE);
		int cell_string_length = 0;

		// this will tell us if we have to include "," in our cell or not
		bool quoted = false;

		for (int current_pos = 0; current_pos < strlen(line); current_pos++) {
			switch (line[current_pos]) {
				case ',':
					if (!quoted) {
						// we are done recording the cell, we can add it to the table
						cell[cell_string_length] = '\0';
						current_col++;
						CSValue_edit_cell(&csv, cell, current_col, current_row);

						cell_string_length = 0;
						free(cell); // CSValue_edit_cell doesn't free our cell for us
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
					// add the current letter to our cell and expand the cell if necessary
					if (cell_string_length >= sizeof(cell)) {
						cell_string_length += CSV_CELL_SIZE;
						cell = realloc(cell, cell_string_length);
					}
					cell[cell_string_length] = line[current_pos];
					cell_string_length++;
			}
			
			// if we're at the end of the line, add the cell to the table
			if (current_pos == strlen(line) - 1) {
				cell[cell_string_length] = '\0';
				current_col++;
				CSValue_edit_cell(&csv, cell, current_col, current_row);

				cell_string_length = 0;
				free(cell);
				cell = malloc(CSV_CELL_SIZE);
			}
		}

		current_col = -1;
		current_row++;
		line = strtok_r(NULL, "\n", &line_saveptr);
	}

	free(csv_string);
	return csv;
}
