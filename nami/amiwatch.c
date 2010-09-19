#include "libami.h"
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

int handle_sigint(int signum);
int handle_manager_event( Manager *m, ManagerMessage *msg);

Manager m;
ManagerMessage msg;

int main(int argc, char **argv) {

	int r;

	signal( SIGINT, (void *) handle_sigint);


	argc--;

	if (argc < 3) {
		fprintf(stderr, "prints all asterisk events to stdout.\n\n");
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

	if (manager_event_mask( &m, 1) != 0) {
		fprintf(stderr, "ERROR: Failed to turn on receiving events.\n");
		exit(1);
	}

	while (1) {

		// One receive can get more than one message..
		if (manager_recv_ready(&m) == 0) { 

			r = manager_recv_message( &m, &msg );

			handle_manager_event( &m, &msg );
		}

		// Handle all (if any) subsequent messages
		while (manager_next_message( &m, &msg) > 0) {

			handle_manager_event( &m, &msg );
		}
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

	manager_print_message( msg );

	return 0;
}
