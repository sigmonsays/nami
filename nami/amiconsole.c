#include "libami.h"
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int handle_sigint(int signum);
int handle_manager_event( Manager *m, ManagerMessage *msg);
int key_ready();

Manager m;
int main(int argc, char **argv) {

	ManagerMessage resp, msg;


	int l;
	unsigned char buf[4096];


	char *k, *p, *v;

	int r;

	signal( SIGINT, (void *) handle_sigint);


	argc--;

	if (argc < 3) {
		fprintf(stderr, "crude interface to the asterisk manager console. Mostly for testing...\n\n");
		fprintf(stderr, "Usage: %s [host] [username] [password]\n", argv[0]);
		exit(1);
	}

	manager_init(&m);

	if (manager_connect( &m, argv[1] ) != 0) {
		fprintf(stderr, "ERROR: Connection to %s failed\n", argv[1] );
		exit(1);
	}

	if (manager_login( &m, argv[2], argv[3] ) != 0) {
		fprintf(stderr, "ERROR: Login to %s failed\n", argv[1] );
		exit(1);
	}

	printf("Enter Key: Value pairs; use . on a line by itself to send; ! to clear and @ to print the message.\n");

	manager_init_message( &msg );

	while (1) {

		// One receive can get more than one message..
		if (manager_recv_ready(&m) == 0) { 

			r = manager_recv_message( &m, &resp );

			handle_manager_event( &m, &resp );
		}

		// Handle all (if any) subsequent messages
		while (manager_next_message( &m, &resp) > 0) {

			handle_manager_event( &m, &resp );
		}


		if (key_ready() != 0) continue;
		

		l = read(0, buf, sizeof(buf) - 1);
		if (l == 0) continue;

		buf[l] = 0;

		if (buf[l - 1] == '\n') buf[--l] = 0;
		if (buf[l - 1] == '\r') buf[--l] = 0;

		if (l == 0) continue;

		if (strcmp(buf, "@") == 0) {
			printf("# Queue message:\n");
			manager_print_message( &msg );
			continue;
		}

		if (strcmp(buf, "!") == 0) {
			printf("# Reset message...\n");
			manager_init_message( &msg );
			continue;
		}

		if (strcmp(buf, ".") == 0) {
			printf("# Sending message:\n");
			manager_print_message( &msg );

			manager_send_message( &m, &msg );
			manager_init_message( &msg );
			continue;
		}

		p = strstr(buf, ":");
		if (!p) {
			printf("ERROR: No colon found.\n");
			continue;
		}

		*p = 0;

		p++;
		p++;

		k = buf;
		v = p;

		manager_build_message( &msg, k, v);

	}


	manager_logout(&m);
	manager_disconnect(&m);

	exit(0);
}

int handle_sigint(int signum) {

	fprintf(stderr, "Got SIGINT. Exiting...\n");

	manager_logout(&m);
	manager_disconnect(&m);
	exit(0);
}
int handle_manager_event( Manager *m, ManagerMessage *msg) {

	printf("# Received message...\n");
	manager_print_message( msg );

	return 0;
}

int key_ready() {
	fd_set rfd;
	struct timeval tv;
	int r, s;

	s = 0;

	if (s == -1) return 1;

	tv.tv_sec = 0;
	tv.tv_usec = 100;

	FD_ZERO(&rfd);
	FD_SET(s, &rfd);

	r = select( s + 1, &rfd, NULL, NULL, &tv);

	if (r == -1) return -1;
	if (r) return 0;
	return -1;
}

