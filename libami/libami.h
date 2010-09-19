#ifndef HAVE_AMI_H
#define HAVE_AMI_H


#include <time.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>


#define MANAGER_MAX_RECORDS 128

typedef struct {
	char key[128];
	char value[255];
} ManagerRecord;

typedef struct {

	ManagerRecord records[MANAGER_MAX_RECORDS];

	int start; // where to start (if this message is treated as a queue)
	int items; // How many records there are 

} ManagerMessage;


typedef struct {
	char host[128];

	char username[64];
	char password[64];

	int sock;

	/* if we're receiving events */
	int events;

	/* if we're logged in */
	int logged;

	/* place to store messages (queue) */
	ManagerMessage queue;

} Manager;



/* prototypes */
int manager_init(Manager *m);
int manager_connect(Manager *m, char *host);

int manager_login(Manager *m, char *username, char *password);
int manager_login_md5(Manager *m, char *username, char *password);

int manager_logout(Manager *m);
int manager_disconnect(Manager *m);



int manager_init_rec( ManagerRecord *rec);

int manager_init_message( ManagerMessage *msg);
int manager_build_message(ManagerMessage *msg, char *key, char *value);
int manager_store_record( ManagerRecord *rec, char *key, char *value);

int manager_send_message(Manager *m, ManagerMessage *msg);
int manager_recv_message(Manager *m, ManagerMessage *msg);

int manager_next_message( Manager *m, ManagerMessage *msg );

int manager_print_message(ManagerMessage *msg);

int manager_message_get(ManagerMessage *msg, char *key, char *value);


int manager_recv_ready(Manager *m);
int manager_connected(Manager *m);


int manager_loggedin(Manager *m);

int manager_event_mask( Manager *m, int op);
int manager_toggle_events( Manager *m);
int manager_ping( Manager *m);

#endif
