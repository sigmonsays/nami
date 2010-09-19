#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef DEBUG
#define DBG(fmt...) do { \
	 printf("DEBUG: " ); \
	 printf(fmt); \
} while (0);

#else
#define DBG(fmt...) do { \
} while (0);
#endif



int repr(char *buf);
