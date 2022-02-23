#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#define DEBUG 0

#ifdef DEBUG
    #define PRINT_DEBUG(...) printf("[DEBUG] " __VA_ARGS__ );
#else
    #define PRINT_DEBUG(...)
#endif

#endif