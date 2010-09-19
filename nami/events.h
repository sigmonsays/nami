#ifndef HAVE_EVENTS_H
#define HAVE_EVENTS_H

#include "nami.h"
#include "libami.h"
#include <db.h>
#include "manager_status.h"

#include "ncurses_support.h"


/* Event prototypes */

int handle_manager_event( Manager *m, ManagerMessage *msg);

int handle_event_newchannel( ManagerMessage *msg );
int handle_event_newexten( ManagerMessage *msg );

int handle_event_link( ManagerMessage *msg);
int handle_event_unlink( ManagerMessage *msg);

int handle_event_dial( ManagerMessage *msg);
int handle_event_peerstatus( ManagerMessage *msg);
int handle_event_newstate( ManagerMessage *msg);

int handle_event_hangup( ManagerMessage *msg);
int handle_event_newcallerid( ManagerMessage *msg);
int handle_event_status( ManagerMessage *msg);
int handle_event_statuscomplete( ManagerMessage *msg);

int handle_event_reload( ManagerMessage *msg);

int handle_event_originatesuccess( ManagerMessage *msg );
int handle_event_originatefailure( ManagerMessage *msg );

#endif
