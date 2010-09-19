/*
 * This file contains all of the higher level manager commands
 * Only a couple for now...
 */

#include "libami.h"
#include "manager_func.h"
#include "sock.h"

/* Issue a command as if you were in the asterisk CLI
 * The results are print to stdout..
 * returns -1 on error.
 */
int manager_command(Manager *m, char *cmd) {
	int r;
	char buf[4096];

	ManagerMessage msg, resp;


	manager_init_message(&msg);
	manager_build_message(&msg, "Action", "Command");
	manager_build_message(&msg, "Command", cmd);

	if (manager_send_message(m, &msg) == -1) return -1;

	manager_init_message(&resp);
	if (manager_recv_message(m, &resp) == -1) return -1;

	manager_print_message(&resp);

	if (manager_message_get(&resp, "Response", buf) == -1) return -1;

	if (strcmp(buf, "Follows") != 0) return -1;


	while (1) {
		r = socket_recv(m->sock, buf, sizeof(buf) - 1);
		if (r == -1) return -1;

		buf[r] = 0;
		printf("%s", buf);

		if (strstr(buf, "--END COMMAND--\r\n\r\n")) break;
	}

	return 0;

}



/* Set 'Events' mask. Enable/Disable (1 / 0) receiving events.
 * Returns -1 on error, 0 success
 */
int manager_event_mask( Manager *m, int op) {

	ManagerMessage msg;

	manager_init_message( &msg );

	manager_build_message( &msg, "Action", "Events");
	manager_build_message( &msg, "EventMask", (op == 1) ? "system,call,log" : "off" );

	if (manager_send_message( m, &msg ) == -1) return -1;

	if (manager_recv_message( m, &msg ) == -1) return -1;

	m->events = op;
	return 0;
}

/* Toggle events
 * Returns -1 on error, 0 success
 */
int manager_toggle_events( Manager *m) {

	if (manager_event_mask(m, (m->events) ? 0 : 1) != 0) return -1;
	return 0;
}


/* Send a 'Ping' request..
 * Returns -1 on error, 0 success
 */
int manager_ping( Manager *m) {

	ManagerMessage msg;

	manager_init_message( &msg );

	manager_build_message( &msg, "Action", "Ping");

	if (manager_send_message( m, &msg ) == -1) return -1;

	return 0;
}

