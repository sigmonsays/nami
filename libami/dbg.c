#include "dbg.h"

int repr(char *buf) {
	int i,l;
	char c;
	l = strlen(buf);
	printf("'");
	for(i=0; i<l; i++) {
		c = buf[i];
		if (c == '\r') {
			printf("\\r");

		} else if (c == '\n') {
			printf("\\n");

		} else {
			putchar(c);
		}
	}
	printf("'\n");
	return 0;
}

