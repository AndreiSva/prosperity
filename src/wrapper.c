#include <stdlib.h>
#include <stdio.h>

#include "wrapper.h"

void* xmalloc(size_t nbytes) {
    void* result = malloc(nbytes);
    if (result == NULL) {
        fprintf(stderr, "Not enough memory to malloc().\n");        
        exit(EXIT_FAILURE);
    }
    return result;
}

void* xrealloc(void* ptr, size_t nbytes) {
    void* result = realloc(ptr, nbytes);
    if (result == NULL) {
        fprintf(stderr, "Not enough memory to realloc().\n");       
        exit(EXIT_FAILURE);
    }
    return result;
}
