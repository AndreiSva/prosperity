#ifndef _H_WRAPPER
#define _H_WRAPPER

#include <unistd.h>

void die(char* s);
void* xmalloc(size_t nbytes);
void* xrealloc(void* ptr, size_t nbytes);

#endif
