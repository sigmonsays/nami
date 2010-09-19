/* This file contains functions for storing and tracking calls
 * It uses berkeley db
 *
 * */

#include "libami.h"
#include "manager_status.h"



DB *manager_database_init( DB *dbp ) {
	int r;
	u_int32_t flags;

	r = db_create( &dbp, NULL, 0);
	if (r != 0) return NULL;

	flags = DB_CREATE;
	r = dbp->open( dbp, NULL, NULL, NULL, DB_BTREE, flags, 0 );
	if (r != 0) return NULL;

	return dbp;
}

/*
 * ----------------------------------------------[ Extension Routines ]------------------------------------------------------------------------
 */


/* init extension structure */
int extension_init(Extension *ext) {

	strcpy(ext->context, "(none)");
	strcpy(ext->exten, "(none)");
	strcpy(ext->priority, "(none)");

	strcpy(ext->app, "(none)");
	strcpy(ext->appdata, "(none)");
	strcpy(ext->uniqueid, "(none)");

	return 0;
} 

/* convert a manager 'Newexten' event to the Extension struct
 * returns 0 on succes, -1 on error
 */
int event_to_extension(ManagerMessage *msg, Extension *ext) {

	char buf1[256];

	if (manager_message_get( msg, "Context", buf1) != 0) return -1;
	strcpy(ext->context, buf1);

	if (manager_message_get( msg, "Extension", buf1) != 0) return -1;
	strcpy(ext->exten, buf1);

	if (manager_message_get( msg, "Priority", buf1) != 0) return -1;
	strcpy(ext->priority, buf1);

	if (manager_message_get( msg, "Application", buf1) != 0) return -1;
	strcpy(ext->app, buf1);

	if (manager_message_get( msg, "AppData", buf1) != 0) return -1;
	strcpy(ext->appdata, buf1);

	if (manager_message_get( msg, "Uniqueid", buf1) != 0) return -1;
	strcpy(ext->uniqueid, buf1);

	return 0;
}


/*
 * ----------------------------------------------[ Channel Routines ]------------------------------------------------------------------------
 */

/* init channel structure */
int channel_init(Channel *chan) {

	strcpy( chan->channel, "(none)" );
	strcpy( chan->callerid, "(none)" );
	strcpy( chan->calleridname, "(none)" );
	strcpy( chan->uniqueid, "(none)" );
	strcpy( chan->state, "-" );
	strcpy( chan->desc, "-" );

	strcpy( chan->link, "-" );

	chan->dial = 0;

	extension_init( &chan->current_exten );

	return 0;

}

/* Delete a channel from the database. 
 * Return 0 on succes, -1 on error.. (Record not found)
 */
int channel_delete( DB *db, char *uid ) {

	int r;
	DBT key;
	char buf[128];


	memset(&key, 0, sizeof(DBT));

	sprintf(buf, "channel/%s", uid);

	key.data = buf;
	key.size = strlen(buf) + 1;

	r = db->del(db, NULL, &key, 0);

	if (r != 0 ) {
		return -1;
	}

	return 0;
}

/* Convert a NewChan message event to a channel struct
 * returns 0 on success, -1 on error
 */
int event_to_channel( ManagerMessage *msg, Channel *chan) {

	char buf1[256];

	if (manager_message_get( msg, "Channel", buf1) != 0) return -1;
	strcpy(chan->channel, buf1);

	if (manager_message_get( msg, "CallerID", buf1) != 0) return -1;
	strcpy(chan->callerid, buf1);

	if (manager_message_get( msg, "CallerIDName", buf1) != 0) return -1;
	strcpy(chan->calleridname, buf1);

	if (manager_message_get( msg, "Uniqueid", buf1) != 0) return -1;
	strcpy(chan->uniqueid, buf1);

	return 0;
}

int channel_add( DB *db, Channel *chan) {

	int r;
	char uid[128];

	DBT key, data;


	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	sprintf(uid, "channel/%s", chan->uniqueid);

	/* printf( "Channel add '%s'\n", uid); */

	key.data = uid;
	key.size = strlen(uid) + 1;

	data.data = chan;
	data.size = sizeof(Channel);

	r = db->put(db, NULL, &key, &data, DB_NOOVERWRITE);

	if (r != 0) {
		/* possibly ( if r == DB_KEYEXIST */
		/* printf("Key exists...\n"); */
		return -1;
	}

	/* printf("New Channel: '%s'\n", uid); */

	return 0;
}

/* get a channel based on 'uniqueid' 
 * return -1 on error, 0 on success
 */
int channel_get( DB *db, char *uniqueid, Channel *chan) {

	int r;
	DBT key, data;
	char buf[128];


	memset(chan, 0, sizeof(Channel));

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	sprintf(buf, "channel/%s", uniqueid);

	/* printf(" Channel get key = '%s'\n", buf); */

	key.data = buf;
	key.size = strlen(buf) + 1;

	data.data = chan;
	data.ulen = sizeof(Channel);
	data.flags = DB_DBT_USERMEM;

	r = db->get(db, NULL, &key, &data, 0);

	if (r != 0) { // database get failed

		/* db->err(db, r, "db says: "); */
		return -1;
	}

	return 0;
}


/* replace a channel based on unique id with a channel
 * returns 0 on succss, -1 on error..
 **/

int channel_update( DB *db, Channel *chan) {


	if (channel_delete( db, chan->uniqueid) != 0) return -1;

	if (channel_add( db, chan) != 0) return -1;
	return 0;
}


/* gets the duration (in seconds) of a channel
 * -1 on error
 */
int channel_duration( Channel *chan) {
	time_t t;
	int s;
	char *p, buf[128];

	strcpy(buf, chan->uniqueid);

	p = strstr(buf, ".");
	if (!p) return -1;

	*p = 0;

	time(&t);
	s = strtol(buf, NULL, 0);

	return (t - s);
}


/* Get channel from numeric id. Channel #'s start at 1
 * returns 0 on success. -1 on error
 */
int get_channel_from_id(DB *dbp, int chan_id, Channel *chan) {

	DBC *curs;
	DBT key, data;
	int i, r, found = 0;

	if (chan_id < 1) return -1;

	dbp->cursor(dbp, NULL, &curs, 0); 

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	data.data = chan;
	data.ulen = sizeof(Channel);
	data.flags = DB_DBT_USERMEM;

	i = 0;
	while ((r = curs->c_get(curs, &key, &data, DB_NEXT)) == 0) {
		if (++i == chan_id) { found = i; break; }
	}

	if (found == 0) return -1;
	return 0;
}

/*
 * ----------------------------------------------[ Linked Channel Routines ]------------------------------------------------------------------------
 */


/* init linked channel structure */
int linkedchannel_init(LinkedChannel *chan) {
	channel_init( & chan->src );
	channel_init( & chan->dst );
	return 0;
}

