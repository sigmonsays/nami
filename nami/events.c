#include "events.h"

extern WINDOW *win_monitor;
extern WINDOW *win_channels;
extern WINDOW *win_messages;
extern DB *dbp;
extern ScrollingWindow win_messages_log;

extern ManagerMessage manager_last_message;

int handle_manager_event( Manager *m, ManagerMessage *msg) {

	char event[128];

	memcpy( & manager_last_message, msg, sizeof( ManagerMessage) );
	window_draw_monitor(win_monitor);
	wrefresh(win_monitor);

	if (manager_message_get( msg, "Event", event ) != 0) {
		return -1;
	}


	if (strcmp(event, "PeerStatus") == 0) {
		handle_event_peerstatus( msg );

	} else if (strcmp(event, "Newchannel") == 0) {
		handle_event_newchannel( msg );

	} else if (strcmp(event, "Newexten") == 0) {
		handle_event_newexten( msg );

	} else if (strcmp(event, "Link") == 0) {
		handle_event_link( msg );

	} else if (strcmp(event, "Unlink") == 0) {
		handle_event_unlink( msg );

	} else if (strcmp(event, "Dial") == 0) {
		handle_event_dial( msg );

	} else if (strcmp(event, "Hangup") == 0) {
		handle_event_hangup( msg );

	} else if (strcmp(event, "Newstate") == 0) {
		handle_event_newstate( msg );

	} else if (strcmp(event, "Newcallerid") == 0) {
		handle_event_newcallerid( msg );

	} else if (strcmp(event, "Status") == 0) {
		handle_event_status( msg );

	} else if (strcmp(event, "StatusComplete") == 0) {
		handle_event_statuscomplete( msg );

	} else if (strcmp(event, "OriginateSuccess") == 0) {
		handle_event_originatesuccess( msg );

	} else if (strcmp(event, "OriginateFailure") == 0) {
		handle_event_originatefailure( msg );

	} else if (strcmp(event, "Reload") == 0) {
		handle_event_reload( msg );

	} else {
		status("WARNING: Unhandled event: '%s'\n", event);

		scrollwin_append( &win_messages_log, "Unhandled Event: %s", event);
		window_draw_messages( win_messages );
		wrefresh(win_messages);
	}

	return 0;
}

/* Handles a 'PeerStatus' event
 * returns -1 on error, 0 on success
 */
int handle_event_peerstatus( ManagerMessage *msg) {
	char buf1[4096], buf2[4096];

	if (manager_message_get( msg, "PeerStatus", buf1) == 0 && manager_message_get( msg, "Peer", buf2) == 0 ) {

		scrollwin_append( &win_messages_log, "%s %s", buf2, buf1);
		window_draw_messages( win_messages );
		wrefresh(win_messages);
	}
	return 0;
}

/* Handles a 'Newchannel' event
 * returns -1 on error, 0 on success
 */
int handle_event_newchannel( ManagerMessage *msg ) {

	Channel chan;

	channel_init(&chan);

	if (event_to_channel( msg, &chan) != 0) {
		return -1;
	}


	if (channel_add( dbp, &chan ) != 0) {
		return -1;
	}

	window_draw_channels(win_channels);
	wrefresh(win_channels);

	return 0;
}


/* Handles a 'Newexten' event
 * returns -1 on error, 0 on success
 */
int handle_event_newexten( ManagerMessage *msg ) {

	char key1[255];
	Extension *extenp;

	Channel chan;


	if (manager_message_get( msg, "Uniqueid", key1) != 0) { // No Uniqueid in header
		status("ERROR: NewExten: No Uniqueid");
		return -1;
	}

	if (channel_get( dbp, key1, &chan) != 0) { // Channel not found
		status("ERROR: NewExten: Channel not found...");
		return -1;
	}

	extenp = & chan.current_exten;

	extension_init( extenp );

	if (event_to_extension( msg, extenp) != 0) { // Bad event message
		status("ERROR: NewExten: Bad message");
		return -1;
	}

	if (channel_update( dbp, &chan ) != 0) { // Channel update failed
		status("ERROR: NewExten: Channel update failed");
		return -1;
	}

	window_draw_channels(win_channels);
	wrefresh(win_channels);

	return 0;
}

/* Handles a 'Link' event
 * Returns -1 on error, 0 on success
 */
int handle_event_link( ManagerMessage *msg) {
	char key1[255];
	Channel chan1, chan2;

	if (manager_message_get( msg, "Uniqueid1", key1) != 0) return -1;

	if (channel_get( dbp, key1, &chan1) != 0) return -1;

	if (manager_message_get( msg, "Uniqueid2", key1) != 0) return -1;

	strcpy( chan1.link, key1 );

	if (channel_update(dbp, &chan1 ) != 0) return -1;

	window_draw_channels(win_channels);
	wrefresh(win_channels);

	/* Log a message */

	if (manager_message_get( msg, "Uniqueid1", key1) != 0) return -1;
	if (channel_get( dbp, key1, &chan1) != 0) return -1;
	
	if (manager_message_get( msg, "Uniqueid2", key1) != 0) return -1;
	if (channel_get( dbp, key1, &chan2) != 0) return -1;

	scrollwin_append( &win_messages_log, "%s => %s", chan1.callerid, chan2.callerid);

	window_draw_messages(win_messages);
	wrefresh(win_messages);


	return 0;
}


/* Handles a 'Unlink' event
 * Returns -1 on error, 0 on success
 */
int handle_event_unlink( ManagerMessage *msg) {

	return 0;
}


/* Handles a 'Dial' event
 * Returns -1 on error, 0 on success
 */
int handle_event_dial( ManagerMessage *msg) {

	char key1[128];
	Channel dst_chan, src_chan;

	if (manager_message_get( msg, "SrcUniqueID", key1) != 0) return -1;
	if (channel_get(dbp, key1, &src_chan) != 0) return -1;

	if (manager_message_get( msg, "DestUniqueID", key1) != 0) return -1;
	if (channel_get(dbp, key1, &dst_chan) != 0) return -1;

	dst_chan.dial = 1;

	if (channel_update(dbp, &dst_chan) != 0) return -1;

	window_draw_channels(win_channels);
	wrefresh(win_channels);

	/* log a message */
	/* 
	scrollwin_append( &win_messages_log, "%s/%s => %s", src_chan.channel, src_chan.callerid, dst_chan.channel);
	window_draw_messages(win_messages);
	wrefresh(win_messages);
	*/

	return 0;
}

/* Handles a 'Newstate' event
 * Returns -1 on error, 0 on success
 */
int handle_event_newstate( ManagerMessage *msg) {
	char key1[255], buf1[4096];

	Channel chan;

	if (manager_message_get( msg, "Uniqueid", key1) != 0) return -1;
	if (manager_message_get( msg, "State", buf1) != 0) return -1;

	if (channel_get( dbp, key1, &chan) != 0) return -1;

	strcpy(chan.state, buf1);

	if (channel_update( dbp, &chan ) != 0) return -1;

	window_draw_channels(win_channels);
	wrefresh(win_channels);

	return 0;
}


/* Handles a 'Hangup' event
 * Returns -1 on error, 0 on success
 */
int handle_event_hangup( ManagerMessage *msg) {

	char key1[255];

	if (manager_message_get( msg, "Uniqueid", key1) == 0) {

		channel_delete( dbp, key1 );

		window_draw_channels(win_channels);
		wrefresh(win_channels);

	} else {
		status("ERROR: Hangup: no Uniqueid\n");
	}

	return 0;
}



/* Handles a 'Newcallerid' event
 * Returns -1 on error, 0 on success
 */
int handle_event_newcallerid( ManagerMessage *msg) {

	char key1[255];
	char buf1[4096];

	Channel chan;

	if (manager_message_get( msg, "Uniqueid", key1) != 0) return -1;
	if (channel_get( dbp, key1, &chan) != 0) return -1;

	if (manager_message_get( msg, "CallerID", buf1) != 0) return -1;

	strcpy(chan.callerid, buf1);

	if (manager_message_get( msg, "CallerID", buf1) != 0) return -1;
	strcpy(chan.calleridname, buf1);

	channel_update( dbp, &chan );

	window_draw_channels(win_channels);
	wrefresh(win_channels);
	return 0;
}

/* Handles a 'StatusComplete' event.
 * returns -1 on error, 0 on success
 */
int handle_event_statuscomplete( ManagerMessage *msg) {
	return 0;
}

/* Handles a 'Status' event.
 * returns -1 on error, 0 on success
 */
int handle_event_status( ManagerMessage *msg) {
	return 0;
}

/* Handles a 'Reload' event.
 * returns -1 on error, 0 on success
 */
int handle_event_reload( ManagerMessage *msg) {
	scrollwin_append( &win_messages_log, "Asterisk reloaded.");
	window_draw_messages( win_messages );
	wrefresh(win_messages);
	return 0;
}

/* Handles a 'OriginateSuccess' event.
 * returns -1 on error, 0 on success
 */
int handle_event_originatesuccess( ManagerMessage *msg ) {
	scrollwin_append( &win_messages_log, "Originate Successfull");
	window_draw_messages( win_messages );
	wrefresh(win_messages);
	return 0;
}

/* Handles a 'OriginateFailure' event.
 * returns -1 on error, 0 on success
 */
int handle_event_originatefailure( ManagerMessage *msg ) {
	scrollwin_append( &win_messages_log, "Originate Failed");
	window_draw_messages( win_messages );
	wrefresh(win_messages);
	return 0;
}
