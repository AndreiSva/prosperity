#ifndef _H_SERVER_PARSING
#define _H_SERVER_PARSING

#include <stdbool.h>
#include <stdint.h>

bool isnumber(char* string);

typedef struct {
	uint32_t cols;
	uint32_t rows;

	// table[col][row]
	char*** table;
} CSValue;

#endif
