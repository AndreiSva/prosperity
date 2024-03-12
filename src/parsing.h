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

char* CSValue_get(CSValue* csv, uint32_t col, uint32_t row);
CSValue CSValue_parse(char* csv_string);

#endif
