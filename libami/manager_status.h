#ifndef HAVE_MANAGER_STATUS_H
#define HAVE_MANAGER_STATUS_H

#include <db.h>
#include <string.h>



#include <db.h>

#include "libami.h"

typedef struct {

	char context[128];
	char exten[255];
	char priority[16];

	char app[255];
	char appdata[255];
	char uniqueid[128];

} Extension;

typedef struct {

	char channel[128];
	char callerid[255];
	char calleridname[255];
	char uniqueid[128];

	/* every channel has a current extension 
	 * I don't think I want to store all extensions, unless I really _HAVE_ to....
	 * */
	Extension current_exten;

	char state[64];

	char desc[128];

	char link[128];

	int dial;

} Channel;

typedef struct {

	Channel src;
	Channel dst;

} LinkedChannel;


/* prototypes */

DB *manager_database_init( DB *dbp );

int channel_init(Channel *chan);
int channel_add( DB *db, Channel *chan);
int channel_delete( DB *db, char *uid );
int channel_get( DB *db, char *uniqueid, Channel *chan);
int channel_update( DB *db, Channel *chan);
int channel_duration( Channel *chan);
int get_channel_from_id(DB *dbp, int chan_id, Channel *chan);

int event_to_channel( ManagerMessage *msg, Channel *chan);

int extension_init(Extension *ext);
int event_to_extension(ManagerMessage *msg, Extension *ext);


int linkedchannel_init(LinkedChannel *chan);


#endif
