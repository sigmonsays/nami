/* asterisk manager interface */



#include "dbg.h"

#include "sock.h"

#include "libami.h"
#include "manager_status.h"

int manager_init(Manager *m) {

	strcpy(m->host, "");
	strcpy(m->username, "");
	strcpy(m->password, "");

	manager_init_message( & m->queue );

	m->sock = -1;
	m->logged = 0;
	m->events = 0;

	return 0;
}


/* Connect to manager, trashing the connection banner
 * -1 on error, 0 success
 * */
int manager_connect(Manager *m, char *host) {
	int r;
	char buf[4096];

	strcpy(m->host, host);

	m->sock = socket_connect( host, 5038 );

	/* trash the connection banner */
	r = socket_recv(m->sock, buf, 4096);
	if (r == -1) return -1;

	buf[r] = 0;
	DBG("manager_connect: Banner: %s\n", buf);

	return 0;
}

/* returns 0 if manager is connected, -1 if not
 */
int manager_connected(Manager *m) {

	if (m->sock > 0) {
		return 0;
	}
	return -1;
}



/* Login to a connected manager.
 * -1 on error, 0 on success
 */
int manager_login(Manager *m, char *username, char *password) {
	char buf[64];

	ManagerMessage msg, resp;


	manager_init_message( &msg );

	manager_build_message(&msg, "Action", "Login");
	manager_build_message(&msg, "Username", username);
	manager_build_message(&msg, "Secret", password);
	manager_build_message(&msg, "Events", "off");

	if (manager_send_message(m, &msg ) == -1) return -1;

	manager_init_message( &resp );

	if (manager_recv_message(m, &resp) == -1) return -1;


	if (manager_message_get( &resp, "Response", buf) == -1) return -1;

	if (strcmp(buf, "Error") == 0) return -1;

	m->logged = 1;

	return 0;
}

/* Check if we're logged in, 
 * -1 on error (not logged in), 0 on success, 1 on false
 */
int manager_loggedin(Manager *m) {
	if (manager_connected(m) != 0) return -1;

	if (m->logged == 1) {
		return 0;
	}
	return 1;
}

/* Build a key-value pair
 * -1 on error, 0 on success
 */
int manager_build_message(ManagerMessage *msg, char *key, char *value) {

	int i;
	ManagerRecord *rec, *records;

	i = msg->items;

	records = msg->records;

	rec = records + i;

	manager_store_record( rec, key, value);

	msg->items++;
	return 0;
}

int manager_store_record( ManagerRecord *rec, char *key, char *value) {
	strcpy( rec->key, key);
	strcpy( rec->value, value);
	return 0;
}

/*
 * Send the manager message on the socket
 * returns -1 on error, or numbers of records sent
 */
int manager_send_message(Manager *m, ManagerMessage *msg) {

	char buf[4096];
	int i, r;
	int rc = 0;
	ManagerRecord *rec, *records;

	records = msg->records;

	for(i=0; i<msg->items; i++) {

		rec = records + i;

		DBG(">> [%s:%s]\n", rec->key, rec->value);

		sprintf(buf, "%s: %s\r\n", rec->key, rec->value);

		r = socket_send(m->sock, buf);

		if (r == -1) {
			rc = -1;
			break;
		}
		rc++;
	}

	if (rc > 0) {
		socket_send(m->sock, "\r\n");
	}
	return rc;
}


/* nullify a key-value pair
 */
int manager_init_rec( ManagerRecord *rec) {
	strcpy(rec->key, "");
	strcpy(rec->value, "");
	return 0;
}

/*
 * nullify the ManagerMessage struct 
 */
int manager_init_message( ManagerMessage *msg) {
	int i;
	ManagerRecord *rec;

	msg->items = 0;
	msg->start = 0;

	for(i=0; i<MANAGER_MAX_RECORDS; i++) {
		rec = msg->records + i;
		manager_init_rec( rec );
	}

	return 0;
}



/* check for a message
 * returns -1 on error, 1 on nothing, or 0 on success (socket ready/waiting)
 */
int manager_recv_ready(Manager *m) {

	int r, sock;

	fd_set rfd;
	struct timeval tv;

	if (m->sock == -1) return 1;

	tv.tv_sec = 0;
	tv.tv_usec = 100;

	sock = m->sock;

	FD_ZERO(&rfd);
	FD_SET(sock, &rfd);

	r = select( sock + 1, &rfd, NULL, NULL, &tv);

	if (r == -1) return -1;

	if (r) return 0;

	return 1;
}



/* Receive a message on the connected manager.
 * return -1 on error (Don't trust ManagerMessage contents), or 0 on success
 */
int manager_recv_message(Manager *m, ManagerMessage *msg) {
	int r;
	char tmp_buf[4096], buf[4096];

	char *p, *t1, *t2;

	char *p1, *p2;

	ManagerMessage *msg_p;

	msg_p = & m->queue;

	// if there is  some shit on the queue, get it off !!
	if (manager_next_message(m, msg ) > 0) {

		DBG("w00t, Got queued message (size %d)\n", msg->items);
		return 0;

	} else {

		strcpy(buf, "");
		while ( (r = recv( m->sock, tmp_buf, sizeof(buf) - 1, 0)) > 0) {
			tmp_buf[r] = 0;
			strcat(buf, tmp_buf);
			if (strstr(buf, "\r\n\r\n")) break;
		}

		/*
		printf("Net Recv: ");
		repr(buf);
		printf("\n");
		*/

		t1 = strtok_r( buf, "\r\n\r\n", &p1);

		if (!t1) return 0;

		do {

			t2 = strtok_r( t1, "\r\n", &p2);

			if (!t2) continue;
			do {


				p = strstr(t2, ":");

				if (p) {

					*p = '\0';
					p++;
					p++;

					manager_build_message(msg_p, t2, p );

					DBG(" == Queued: '%s' : '%s' (size: %d)\n", t2, p, msg_p->items);


				} else {

					/* unexpected format... */
					DBG(" !! Unexpected format received....\n");
				}

			} while ( (t2 = strtok_r(NULL, "\r\n", &p2)) );

		} while ( (t1 = strtok_r(NULL, "\r\n\r\n", &p1)) );



		// now get some shit off teh queue
		if (manager_next_message(m, msg ) > 0) {
			DBG("Using queue message (%d items)\n", msg->items);
			return 0;
		}


	}

	/* Hmmmmm */
	return 0;
}

/* get the next message out of queue
 * returns -1 on error, or # of key-value pairs in message
 * */
int manager_next_message( Manager *m, ManagerMessage *msg ) {

	int i, s;
	int event_count = 0;

	ManagerMessage *queue;
	ManagerRecord *rec;

	queue = & m->queue;

	manager_init_message(msg);

	s = queue->start;

	DBG("Queue starting at %d (items %d)\n", s, queue->items);

	for(i=s; i<queue->items; i++) {

		rec = queue->records + i;

		if (strcmp(rec->key, "Event") == 0) { // We break out on every "Event" record key-value if we already got it
			event_count++;
			if (event_count == 2) break;

		}
		manager_build_message(msg, rec->key, rec->value);
	}

	if (queue->items && i == queue->items) { // reset the queue if we reach the end

		DBG("Clearing queue\n");
		manager_init_message( queue );

	} else {
		if (event_count == 2) {
			queue->start = i;
		}
	}

	return msg->items;
}



/* Print contents of a ManagerMessage record
 */
int manager_print_message(ManagerMessage *msg) {

	char buf[4096];
	int i;
	int rc = 0;
	ManagerRecord *rec, *records;

	records = msg->records;

	printf("ManagerRecord (%d)\n", msg->items);

	for(i=0; i<msg->items; i++) {

		rec = records + i;

		sprintf(buf, " %d) %s: %s\n", i + 1, rec->key, rec->value);

		printf("%s", buf);

	}
	return rc;
}


/* Issue the logout command
 * return -1 on error.
 * This function does not disconnect the socket.
 */
int manager_logout(Manager *m) {

	ManagerMessage msg;

	if (manager_loggedin(m) != 0) return -1;

	manager_init_message(&msg);
	manager_build_message(&msg, "Action", "Logoff");

	manager_send_message(m, &msg);

	manager_recv_message(m, &msg);

	m->logged = 0;

	return 0;
}

/* disconnect the socket 
 */
int manager_disconnect(Manager *m) {
	if (m->sock == -1) return -1;

	shutdown(m->sock, 2);
	close(m->sock);
	m->sock = -1;
	m->logged = 0;
	return 0;
}


/* Get the value of a key-value pair from ManagerMessage
 * returns -1 on error. 0 on success, 1 not found
 */
int manager_message_get(ManagerMessage *msg, char *key, char *value) {
	int i;
	int found = -1;
	ManagerRecord *rec, *records;
	
	records = msg->records;

	for(i=0; i<msg->items; i++) {

		rec = records + i;

		if (strcmp(rec->key, key) == 0) {
			found = i;
			break;
		}

	}

	if (found == -1) return 1;
	strcpy(value, rec->value);
	return 0;
}
