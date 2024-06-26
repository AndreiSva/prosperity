#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

#include "parsing.h"
#include "wrapper.h"

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
		return NULL;
	}

	return csv->table[col][row];
}

// TODO: add more error checking here
static void CSValue_edit_cell(CSValue* csv, char* cell, uint32_t col, uint32_t row) {
	// if we want to add a cell outside of what we've allocated already, allocate the new memory

	// cols
	if (csv->cols <= col) {
		int old_size = csv->cols;
		csv->cols = col + 1;
		csv->table = xrealloc(csv->table, sizeof(char**) * csv->cols);
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
			csv->table[i] = xrealloc(csv->table[i], sizeof(char*) * csv->rows);

			// the new row cells should be NULL
			for (int j = old_size; j < csv->rows; j++) {
				csv->table[i][j] = NULL;
			}
		}
	}

	free(csv->table[col][row]);
	char* cell_string = xmalloc(strlen(cell) + 1);
	strcpy(cell_string, cell);
	csv->table[col][row] = cell_string;
}

static void cell_addchar(char** cell, size_t* cell_length, size_t* cell_size, char c) {
	if (*cell_length >= *cell_size) {
		*cell_size += CSV_CELL_SIZE;
		*cell = xrealloc(*cell, *cell_size);
	}
	(*cell)[*cell_length] = c;
	(*cell_length)++;
}

CSValue CSValue_parse(char* csv_string) {
	CSValue csv = {
		.cols = 1,
		.rows = 1,
		.table = xmalloc(sizeof(char**))
	};

	csv.table[0] = calloc(1, sizeof(char*));

	int current_col = -1; // set this to -1 because it will get incremented back to 0 right before we add the cell
	int current_row = 0;
	int current_pos = 0;

	size_t cell_string_length = 0;
	size_t cell_string_size = CSV_CELL_SIZE;
	char* cell = xmalloc(cell_string_size);

	// this will tell us if we have to include "," in our cell or not
	bool quoted = false;
	while (csv_string[current_pos] != '\0') {
		switch (csv_string[current_pos]) {
			case ',':
				if (!quoted) {
					// we are done recording the cell, we can add it to the table
					cell[cell_string_length] = '\0';
					current_col++;
					CSValue_edit_cell(&csv, cell, current_col, current_row);

					cell_string_length = 0;
					free(cell); // CSValue_edit_cell doesn't free our cell for us
					cell = xmalloc(cell_string_size);
				} else {
					cell_addchar(&cell, &cell_string_length, &cell_string_size, csv_string[current_pos]);
				}
				break;
			case '\"':
				;
				char previous;
				if (current_pos == 0) {
					previous = '\n';
				} else {
		 			previous = csv_string[current_pos - 1];
				}

				char next = csv_string[current_pos + 1];

				if (previous == ',' || previous == '\n') {
					quoted = true;
				}

				if (next == ',' || next == '\n') {
					quoted = false;
				}

				if (previous != ',' && previous != '\n' && next != ',' && next != '\n') {
					cell_addchar(&cell, &cell_string_length, &cell_string_size, csv_string[current_pos]);
				}

				break;
			case '\n':
				// if we're at the end of the line, add the cell to the table
				if (!quoted) {
					cell[cell_string_length] = '\0';
					current_col++;
					CSValue_edit_cell(&csv, cell, current_col, current_row);

					cell_string_length = 0;
					free(cell);
					cell = xmalloc(cell_string_size);

					current_row++;
					current_col = -1;
				}
				break;
			default:
				// add the current letter to our cell and expand the cell if necessary
				cell_addchar(&cell, &cell_string_length, &cell_string_size, csv_string[current_pos]);
		}
		
		current_pos++;
	}
	
	free(cell);
	return csv;
}

size_t ustrlen(char* ustr) {
	size_t count = 0;
	int i = 0;
	while (ustr[i] != '\0') {
		if ((ustr[i] & 0xC0) != 0x80) {
			count++;
		}
		i++;
	}
	return count;
}

void CSValue_put(FILE* stream, CSValue* csv) {
	for (int i = 0; i < csv->rows; i++) {
		for (int j = 0; j < csv->cols; j++) {
			char* cell = CSValue_get(csv, j, i);

			// if our cell contains a comma it means we should quote it
			bool quoted = strchr(cell, ',') != NULL;
			
			if (quoted) {
				fputc('\"', stream);
			}

			fprintf(stream, "%s", cell);

			if (quoted) {
				fputc('\"', stream);
			}

			if (j != csv->cols - 1) {
				fputc(',', stream);
			}
		}
		fputc('\n', stream);
	}
}

void CSValue_free(CSValue* csv) {
	for (int i = 0; i < csv->cols; i++) {
		for (int j = 0; j < csv->rows; j++) {
			free(csv->table[i][j]);
		}
		free(csv->table[i]);
	}
	free(csv->table);
}
