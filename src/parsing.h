#ifndef _H_SERVER_PARSING
#define _H_SERVER_PARSING

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

bool isnumber(char* string);

typedef struct {
	uint32_t cols;
	uint32_t rows;

	// table[col][row]
	char*** table;
} CSValue;

// returns the value of a cell at <col> and <row>
char* CSValue_get(CSValue* csv, uint32_t col, uint32_t row);

// prints a CSValue object to <stream>
void CSValue_put(FILE* stream, CSValue* csv);

void CSValue_edit_cell(CSValue* csv, char* cell, uint32_t col, uint32_t row);
void CSValue_append_row(CSValue* csv, ...);

// parses a valid CSV string
CSValue CSValue_parse(char* csv_string);

void CSValue_free(CSValue* csv);

// returns the length of a utf-8 encoded string
size_t ustrlen(char* ustr);

int CSValue_index_colbyname(CSValue* csv, char* colname, uint32_t row);
int CSValue_index_rowbyname(CSValue* csv, char* rowname, uint32_t col);


#endif
