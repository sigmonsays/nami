#include <stdlib.h>

#include "libami.h"
#include "ncurses_support.h"
#include "config.h"
#include "events.h"
#include "sock.h"

/* prototypes */


void nami_load_config();

int nami_arguments(int argc, char **argv);
void nami_help();
void nami_testconfig();

void nami_finish(int signum);

int key_ready();

int window_draw_channels(WINDOW *w);
int window_draw_messages(WINDOW *w);
int window_draw_monitor(WINDOW *w);
int window_draw_originate( WINDOW *w );

int do_main_window();
int do_help();

int do_originate();
int do_originate_cancel(WINDOW *win_orig);


int do_activity_update();


int do_events_toggle();
int do_ping();

int do_console();
int do_console_command( char *cmd);

int do_login(char *hostname, char *username, char *password );

int sec2time(int seconds, char *buf);


void nami_resize( int signum );
int update_window_dimensions(int signum);

int do_profiles();
int do_profiles_cancel( WINDOW *win_profile);

int do_channel_info();
int do_channel_info_cancel();
int do_channel_hangup();

int do_active_channels();
int do_monitor();

int do_redirect();
int do_redirect_cancel( WINDOW *w);

int do_timeout();
int do_timeout_cancel();



int do_zapshowchannels();
int do_zapshowchannels_cancel();
