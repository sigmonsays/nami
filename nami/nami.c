#include "nami.h"
#include "manager_status.h"
#include "config.h"
#include <time.h>
#include "build.h"

#define HALFDELAY 1

config_t config;

char *nami_configfile = NULL;

int scrn_rows = 30, scrn_cols = 80;

int chan_active_count = 0;
int chan_sel_id = 0; // selected channel id

int win_channels_cols = 35;

int win_messages_cols = 35;
int win_messages_rows = 35;

int win_monitor_rows = 35;
int win_monitor_cols = 35;

int win_originate_rows = 25;
int win_originate_cols = 55;

int win_help_rows = 25;
int win_help_cols = 50;

Manager m;
ManagerMessage manager_last_message;

WINDOW *win_channels = NULL;
WINDOW *win_messages = NULL;
WINDOW *win_monitor = NULL;
DB *dbp;

/* stores the most recent manager host/user/pass */
char mgr_user[128], mgr_pass[128], mgr_host[128];


ScrollingWindow win_messages_log;

char *activity_str = "\\-/|";
int activity = 0;

void nami_load_config() {
	int r = 0;

	if (nami_configfile && !r && load_config(nami_configfile, &config) == 0) r = 1;

	if (!r && load_config("./nami.cfg", &config) == 0) r = 1;
	if (!r && load_config("/etc/nami.cfg", &config) == 0) r = 1;

	if (!r) {
		printf("ERROR: Unable to load configuration!\n");
		printf("System defaults:\n /nami.cfg\n /etc/nami.cfg\n");
		exit(1);
	}
}

int main(int argc, char **argv) {

	int ch;
	int r;
	ManagerMessage msg;

	int tm, status_tm;

	r = 0;

	if (nami_arguments(argc, argv) != 0) exit(1);
	nami_load_config();

	dbp = manager_database_init( dbp );
	if (dbp == NULL) {
		printf("Unable to init manager database\n");
		exit(1);
	}

	manager_init(&m);

	ncurses_start();

	/* signal(SIGWINCH, (void *) nami_resize); */
   signal(SIGINT, (void *) nami_finish);      /* arrange interrupts to terminate */

	halfdelay(HALFDELAY);


	status("Welcome to nami %s Press F1 for help", NAMI_VERSION);

	nami_resize(0);

	while (1) {

		/* Clear the status messages (older than ~30 seconds )
		 */
		status_tm = status_time();
		if (status_tm > 0) {
			time( (time_t *) &tm );
			if ( (tm - status_tm) > 30) status("nami %s", NAMI_VERSION);
		}


		/* There has got to be a better way to do this 
		 * It should update every second.. if there are channels..
		 * */
		window_draw_channels(win_channels);
		wrefresh(win_channels);



		if (manager_loggedin(&m) == 0) {

			if (manager_recv_ready(&m) == 0) { 

				do_activity_update();
				wrefresh( win_messages );

				r = manager_recv_message( &m, &msg );

				handle_manager_event( &m, &msg );
			}

			while (manager_next_message( &m, &msg) > 0) {
				handle_manager_event( &m, &msg );
			}
		}

		if (key_ready() != 0) continue;

		ch = getch();

		if (ch == ERR) continue;

		if (ch == KEY_RESIZE) {
			nami_resize(0);
		}


		if (ch == KEY_F(1) || ch == '?') {
			do_help();

		} else if (ch == '\r' || ch == '\n' || ch == 12 /* ^L */ ) { // redraw the screen
			nami_resize(0);

		} else if (ch == 'C' ) { // connect using profile
			do_profiles();
			
		} else if (ch == KEY_F(2) || ch == 'c' ) { // connect

			ncurses_prompt(stdscr, scrn_rows - 1, 0,  "Host", config.default_hostname, mgr_host, 1);

			if (strlen(mgr_host)) {
				r = manager_connect( &m, mgr_host );
				if (r == 0) {
					status("Connected to %s", mgr_host);
				} else {
					status("Connect failed");
					continue;
				}

			} else {
				status("Connect cancelled");
				continue;
			}

			ncurses_prompt(stdscr, scrn_rows - 1, 0, "Username: ", NULL, mgr_user, 1);
			ncurses_prompt(stdscr, scrn_rows - 1, 0, "Password: ", NULL, mgr_pass, 0);

			status("Trying to login..");

			if (strlen(mgr_user) && strlen(mgr_pass)) {
				r = manager_login( &m, mgr_user, mgr_pass );

				if (r == 0) {
					status("Logged In!");

				} else {
					status("Login failed");
					continue;
				}

			} else {
				status("Login cancelled");
				continue;
			}

			if (manager_loggedin(&m) == 0) {
				status("Connected to %s as %s", mgr_host, mgr_user);

				scrollwin_append( &win_messages_log, "Connected to %s as %s", mgr_host, mgr_user );
				window_draw_messages(win_messages);
				wrefresh(win_messages);

				/* Turn on 'Events' */
				manager_event_mask( &m, 1);

				/* Load active channels */
				do_active_channels();
			}

		} else if (ch == 'd') { // disconnect

			if (manager_disconnect(&m) == 0) {
				status("Successfully disconnected");

				scrollwin_append( &win_messages_log, "Disconnected from %s", mgr_host);
				window_draw_messages(win_messages);
				wrefresh(win_messages);

			} else {
				status("Disconnect failed");
			}

		} else if (ch == 'H') { // Channel information
			do_channel_hangup();

		} else if (ch == 'i') { // Channel information
			do_channel_info();

		} else if (ch == 'j' || ch == KEY_DOWN ) { // selection down
			if (chan_sel_id < chan_active_count) chan_sel_id++;

		} else if (ch == 'k' || ch == KEY_UP ) { // selection up
			if (chan_sel_id > 0) chan_sel_id--;

		} else if (ch == 'l') { // logout

			if (manager_logout(&m) == 0) {
				status("Successfully logged out");

			} else {
				status("Logout failed");
			}


		} else if (ch == 'm') { // Monitor recording
			do_monitor();


		} else if (ch == ':') {
			if (do_console() != 0) status("Unable to activate console");

		} else if (ch == KEY_F(12) || ch == 'q') { 
			nami_finish(0);

		} else if (ch == 'o') {  // Originate call..

			if (do_originate() == -1) {
				status("Originate failed/cancelled");
			}

		} else if (ch == 'e') {  // toggle receiving events...
			do_events_toggle();

		} else if (ch == 'p') {  // Send a ping..

			if (do_ping() == 0) status("Sending ping...");

		} else if (ch == 'R') {  // Redirect...
			do_redirect();

		} else if (ch == 'T') {  // Set timeout on channel
			do_timeout();

		} else if (ch == 'Z') {  // Do ShowZapChannels

			do_zapshowchannels();


		} else {

			status("Unknown key '%c' (%d)", ch, ch);
		}


	}

	ncurses_finish(0);

	exit(0);
}

/* Finish/cleanup nami stuff
 */
void nami_finish(int signum) {

	char ch;

	halfdelay( 10 );

	status("");
	mvwprintw(stdscr, scrn_rows - 1, 0, "Really [y/n]? " );
	wrefresh(stdscr);

	ch = getch();

	if (ch == ERR) {
		status("Quit timed out");
		halfdelay(HALFDELAY);
		return;
	}

	if (ch != 'y' && ch != 'Y') {
		status("The dude abides...");
		halfdelay(HALFDELAY);
		return;
	}

	if (manager_connected( &m ) == 0) {
		manager_logout( &m );
		manager_disconnect( &m );
	}
	ncurses_finish(signum);
}


/* Routine to show active zap channels */
int do_zapshowchannels() {

	if (manager_connected(&m) == 0 && manager_event_mask( &m, 0) != 0) return -1;


	do_zapshowchannels_cancel();

	return 0;
}

int do_zapshowchannels_cancel() {

	if (manager_connected(&m) == 0 && manager_event_mask( &m, 1) != 0) return -1;
	return -1;
}

/* Routine to get active channels on connect
 * returns 0 on succes, -1 on error
 */
int do_active_channels() {

	ManagerMessage msg;

	if (manager_loggedin(&m) != 0) return -1;

	manager_init_message( &msg );
	manager_build_message( &msg , "Action", "Status");

	if (manager_send_message( &m, &msg) == -1) return do_channel_info_cancel();


	/* if (manager_recv_message( &m, &msg) != 0) return do_channel_info_cancel(); */

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

/* ------------------------------------------------- GUI Functions ---------------------------------- */
int do_main_window() {

	window_draw_channels(win_channels);
	wrefresh(win_channels);

	window_draw_messages(win_messages);
	wrefresh(win_messages);

	window_draw_monitor(win_monitor);
	wrefresh(win_monitor);
	return 0;
}

int do_help() {
	int r, c, ch;

	if (manager_connected(&m) == 0 && manager_event_mask(&m, 0) != 0) return -1;

	CENTER( win_help_rows, win_help_cols, scrn_rows, scrn_cols, r, c );

	WINDOW *win_help = create_window( r, c, win_help_rows, win_help_cols);

	r = 1;

	mvwprintw(win_help, r++, 1, "nami %s help", NAMI_VERSION);
	mvwprintw(win_help, r++, 1, "built: %s", NAMI_BUILD_DATE);
	r++;

	mvwprintw(win_help, r++, 1, ":                Console");
	mvwprintw(win_help, r++, 1, "F2,c             Connect");
	mvwprintw(win_help, r++, 1, "C                Connect to profile");
	mvwprintw(win_help, r++, 1, "F5,d             Disconnect");
	mvwprintw(win_help, r++, 1, "e                Toggle receiving events");
	mvwprintw(win_help, r++, 1, "F1,?             Help");
	mvwprintw(win_help, r++, 1, "H                Hangup selected call");
	mvwprintw(win_help, r++, 1, "i                Show Channel information");
	mvwprintw(win_help, r++, 1, "j                Channel selection down");
	mvwprintw(win_help, r++, 1, "k                Channel selection up");
	mvwprintw(win_help, r++, 1, "l                Logout");
	mvwprintw(win_help, r++, 1, "o                Originate Call..");
	mvwprintw(win_help, r++, 1, "p                Send a Ping..");
	mvwprintw(win_help, r++, 1, "R                Redirect call...");
	mvwprintw(win_help, r++, 1, "T                Set Absolute Timeout");
	mvwprintw(win_help, r++, 1, "Z                Show Zap Channels");
	mvwprintw(win_help, r++, 1, "F12,q            Quit");


	wrefresh(win_help);

	nocbreak(); /* disble halfdelay */
	status("Press enter key to continue...");
	ch = getch();

	halfdelay(HALFDELAY);

	destroy_window(win_help);
	status("");


	do_main_window();

	refresh();
	wrefresh(stdscr);

	if (manager_connected(&m) == 0 && manager_event_mask(&m, 1) != 0) return -1;
	return 0;
}



/* channels redraw routine...
 * */
int window_draw_channels(WINDOW *w) {

	int r, x;
	DBT key, data;


	DBC *curs;
	int i = 0, bold = 0;
	char *p, rec_type[128];

	char duration[16];

	Channel chan, chan2;
	Extension *exten;

	dbp->cursor(dbp, NULL, &curs, 0); 

	memset(&key, 0, sizeof(DBT));
	memset(&data, 0, sizeof(DBT));

	channel_init( &chan );

	data.data = &chan;
	data.ulen = sizeof(Channel);
	data.flags = DB_DBT_USERMEM;

	werase(w);
	window_draw_border(w);

	x = 1;
	i = 2;
	while ((r = curs->c_get(curs, &key, &data, DB_NEXT)) == 0) {

		strcpy(rec_type, key.data );
		p = strstr(rec_type, "/");
		if (p) *p = 0;

		if (strcmp(rec_type, "channel") != 0) continue; 

		exten = & chan.current_exten;

		if (chan_sel_id == 0) chan_sel_id = x;

		if (chan_sel_id == x) { bold = 1; wattron(w, A_BOLD); }

		if (chan.dial == 0) {

			sec2time( channel_duration( &chan ), duration);

			mvwprintw( w, i++, 2, "%d. %s %s %s [%s]", x++, chan.uniqueid, chan.channel, chan.state, duration);
			mvwprintw( w, i++, 2, "    %s => %s@%s", chan.callerid, exten->exten, exten->context);

		} else {
			mvwprintw( w, i++, 2, "%d. %s %s/%s %s", x++, chan.uniqueid, chan.channel, chan.callerid, chan.state);

		}

		if (channel_get(dbp, chan.link, &chan2) == 0) {
			exten = & chan2.current_exten;
			mvwprintw( w, i++, 2, "    [%s/%s %s]", chan2.channel, chan2.callerid, chan2.state);
		}

		if (bold) wattroff(w, A_BOLD);

		i += 1;

	}

	if (curs != NULL) curs->c_close(curs); 

	if ( manager_connected( &m ) == 0) {

		if (x) chan_active_count = x - 1;
		
		if (manager_loggedin( &m ) == 0) {

			mvwprintw(w, 0, 1, " [%s@%s] %u Active Channels ", mgr_user, mgr_host, chan_active_count);

		} else {
			mvwprintw(w, 0, 1, " [%s] Channels ", mgr_host);
		}


	} else {
		mvwprintw(w, 0, 1, " [disconnected] ");

	}

	return 0;
}


/* messages redraw routine...
 * */
int window_draw_messages(WINDOW *w) {

	int i, r = 0;
	ScrollingWindowLine *ln;
	werase(w);
	window_draw_border(w);

	mvwprintw( w, r++, ((win_messages_cols - 8)/ 2), "Messages" );
	mvwprintw( w, r++, 2, "Activity: %c", activity_str[activity] );
	
	for(i=0; i<win_messages_log.size; i++) {

		ln = & win_messages_log.lines[i];
		mvwprintw( w , i + r, 2, "%s", ln->line);
	}

	return 0;
}

/* monitor redraw routine...
 * */
int window_draw_monitor(WINDOW *w) {
	char *p;
	time_t ts;
	int i, r = 0;
	int l;
	
	ManagerMessage *msg;
	ManagerRecord *rec;

	werase(w);

	window_draw_border(w);

	time( &ts );

	mvwprintw( w, r++, ((win_monitor_cols - 8)/ 2), "Monitor" );
	mvwprintw( w, r++, 1, "Monitor: [All Events]" );

	p = ctime( &ts );
	l = strlen(p);
	if (l && p[l - 1] == '\n') p[--l] = 0;
	mvwprintw( w, r++, 1, "%s", p);

	msg = & manager_last_message;

	for(i=0; i<msg->items; i++) {
		rec = msg->records + i;
		mvwprintw(win_monitor, i + r, 2, " %d) %s: %s", i + 1, rec->key, rec->value);

	}

	return 0;
}




/* Toggles recieving events..
 */
int do_events_toggle() {
	if (manager_toggle_events( &m ) != 0) {
		status("Events: Couldn't change event mask");
		return -1;
	}

	status( "Events %s", (m.events) ? "On" : "Off" );

	return 0;
}

/* Send a Ping request... Gets back a 'Pong'...
 * -1 on error, 0 on success
 */
int do_ping() {

	if (manager_ping( &m ) != 0) {
		status("Ping failed");
		return -1;
	}
	return 0;
}


int do_activity_update() {
	mvwprintw( win_messages, 1, 2, "Activity: %c", activity_str[activity++] );
	if (activity == strlen(activity_str)) activity = 0;
	return 0;
}

int do_console() {
	int l;
	int old_events_mask;
	int rc = 0;

	char buf[255];

	old_events_mask = m.events;

	/* Turn off events.. */
	if (manager_event_mask( &m, 0) != 0) return -1;


	werase( stdscr );
	printw("Asterisk Console (This needs work!)\n> ");
	wrefresh(stdscr );


	echo();

	while ( getnstr(buf, sizeof(buf) - 1) != ERR) {

		werase( stdscr );
		printw("> %s", buf);
		wrefresh(stdscr );

		l = strlen(buf);
		if (!l) {
			printw("> ");
			continue;
		}

		if (buf[l - 1] == '\n') buf[--l] = 0;

		if (strcmp(buf, "quit") == 0) break;

		if (do_console_command(buf) != 0) {
			status("Console aborted.");
			break;
		}

		printw("> ");
	}


	/* Turn back to the normal settings... */

	noecho();

	if (manager_event_mask( &m, old_events_mask) != 0) return -1;
	do_main_window();

	return rc;
}


int do_console_command( char *cmd) {
	int r;
	char buf[4096];

	ManagerMessage msg;


	manager_init_message(&msg);
	manager_build_message(&msg, "Action", "Command");
	manager_build_message(&msg, "Command", cmd);

	if (manager_send_message(&m, &msg) == -1) return -1;

	while (1) {
		r = socket_recv(m.sock, buf, sizeof(buf) - 1);
		if (r == -1) return -1;

		buf[r] = 0;

		printw( buf );

		if (strstr(buf, "--END COMMAND--\r\n\r\n")) break;
	}

	return 0;

}


/* Creates originate dialog
 * returns -1 on error, 0 on success
 */

int do_originate() {

	int r, c;

	WINDOW *win_orig;
	char buf1[128], buf2[128];
	ManagerMessage msg;

	if (manager_event_mask( &m, 0) != 0) return -1;

	manager_init_message( &msg );
	manager_build_message( &msg, "Action", "Originate");

	CENTER( win_originate_rows, win_originate_cols, scrn_rows, scrn_cols, r, c );


	win_orig = create_window( r, c, win_originate_rows, win_originate_cols );

   keypad(win_orig, TRUE);

	window_draw_originate(win_orig);
	wrefresh(win_orig);

	r = 1;

	ncurses_prompt( win_orig, r++, 1, "Channel: Zap/g1/", NULL, buf1, 1);
	if (strlen(buf1) == 0) return do_originate_cancel( win_orig );

	sprintf(buf2, "Zap/g1/%s", buf1);
	manager_build_message( &msg, "Channel", buf2 );

	ncurses_prompt( win_orig, r++, 1, "Context", "test_callback", buf1, 1);
	if (strlen(buf1) == 0) return do_originate_cancel( win_orig );
	manager_build_message( &msg, "Context", buf1 );

	ncurses_prompt( win_orig, r++, 1, "Exten", "100", buf1, 1);
	if (strlen(buf1) == 0) return do_originate_cancel( win_orig );
	manager_build_message( &msg, "Exten", buf1 );

	ncurses_prompt( win_orig, r++, 1, "Priority", "1", buf1, 1);
	if (strlen(buf1) == 0) return do_originate_cancel( win_orig );
	manager_build_message( &msg, "Priority", buf1 );

	ncurses_prompt( win_orig, r++, 1, "CallerID: ", NULL, buf1, 1);
	if (strlen(buf1) == 0) return do_originate_cancel( win_orig );
	manager_build_message( &msg, "CallerID", buf1 );

	ncurses_prompt( win_orig, r++, 1, "Dest Chan: ", NULL, buf1, 1);
	if (strlen(buf1)) {
		sprintf(buf2, "DEST_CHANNEL=%s", buf1);
		manager_build_message( &msg, "Variable", buf2 );
	}

	/* Other parameters..
	manager_build_message( &msg, "Account", "");
	manager_build_message( &msg, "Application", "");
	manager_build_message( &msg, "Data", "");
	*/

	manager_build_message( &msg, "Async", "1");

	sprintf(buf1, "ORIG%d", (int) time(NULL));

	manager_build_message( &msg, "ActionID", buf1);

	if (manager_send_message( &m, &msg) == -1) return  do_originate_cancel(win_orig );

	do_originate_cancel( win_orig);

	status("Call created..");

	return 0;
}

/* Function to cancel the originate screen
 * Always returns -1
 */
int do_originate_cancel(WINDOW *win_orig) {

		if (manager_event_mask( &m, 1) != 0) return -1;

		destroy_window(win_orig);
		do_main_window();

		return -1;
}


/* Opens a connect using profile window
 * returns -1 on error, 0 on success
 */
int do_profiles() {

	char buf1[32];
	WINDOW *win_profile;
	int win_profile_rows = 20, win_profile_cols = 40;
	int i, r, c;

	profile_t *profile, *profiles;

	if (!config.profile_count) { 
		status("No profiles. Edit nami.cfg");
		return -1;
	}

	if (manager_connected(&m) == 0 && manager_event_mask(&m, 0) != 0) return -1;

	CENTER( win_profile_rows, win_profile_cols, scrn_rows, scrn_cols, r, c );

	win_profile = create_window( r, c, win_profile_rows, win_profile_cols);

	r = 1;

	mvwprintw(win_profile, r++, 1, "Connection Profiles");

	profiles = & config.profiles[0];

	for(i=0; i<config.profile_count; i++) {

		profile = profiles + i;


		mvwprintw(win_profile, r++, 1, "%d) %s", i + 1, profile->name );

	}

	wrefresh(win_profile);

	ncurses_prompt( win_profile, r++, 1, "Profile # ", NULL, buf1, 1);
	if (strlen(buf1) == 0) return do_profiles_cancel( win_profile );

	// Find the profile
	r = strtol(buf1, NULL, 0);	

	if (!r) return do_profiles_cancel( win_profile );

	for(i=0; i<config.profile_count; i++) {
		profile = profiles + i;
		if (r == profile->number) { i = 0; break; }
	}

	if (i != 0) {
		status("Profile not found");
		return do_profiles_cancel( win_profile );
	}

	if (do_login( profile->hostname, profile->username, profile->password) != 0) {
		
		status("Connection failed to profile: %s", profile->name);
		return do_profiles_cancel( win_profile );
	}

	return 0;
}

/* profile cancel window , always returns -1 */
int do_profiles_cancel( WINDOW *win_profile) {
	if (manager_connected(&m) == 0 && manager_event_mask( &m, 1) != 0) return -1;

	destroy_window(win_profile);
	do_main_window();
	return -1;
}


/* Draws the originate window
 */
int window_draw_originate( WINDOW *w ) {

	mvwprintw(w, 1, 2, "Originate Call");

	window_draw_border( w );

	return 0;
}




/* converts seconds to time and stores it in buf in HH:MM:SS format
 * returns -1 on error, 0 on success
 */
int sec2time(int seconds, char *buf) {
	int h, m, s;
	s = seconds;
	m = seconds / 60;
	h = m / 60;
	sprintf(buf, "%02d:%02d:%02d", h, m % 60, s % 60);
	return 0;
}


/* Handles resize event */
void nami_resize( int signum ) {

	update_window_dimensions(signum);

	if (win_channels) destroy_window( win_channels );
	if (win_messages) destroy_window( win_messages );
	if (win_monitor) destroy_window( win_monitor );


	/* setup main channels window */
	win_channels = create_window( 0, 0, scrn_rows - 1, win_channels_cols );

	/* setup "Messages" box */
	win_messages = create_window( 0, scrn_cols - win_messages_cols, win_messages_rows, win_messages_cols);

	/* setup "Monitor" box */
	win_monitor = create_window( scrn_rows - win_messages_rows, scrn_cols - win_monitor_cols, win_monitor_rows, win_monitor_cols);

	do_main_window();

	refresh();
	wrefresh(stdscr);
	wrefresh(curscr);
}

int update_window_dimensions(int signum) {

	int scrollwin_rows;
	ScrollingWindow old;

	scrn_rows = LINES;
	scrn_cols = COLS;

	old = win_messages_log;

	ncurses_resize(scrn_rows, scrn_cols);

	win_channels_cols = scrn_cols / 2;

	win_messages_cols = scrn_cols / 2;
	win_messages_rows = (scrn_rows / 2);

	win_monitor_cols = scrn_cols / 2;
	win_monitor_rows = (scrn_rows / 2) - 1;

	scrollwin_rows = win_messages_rows - 4;

	scrollwin_init( &win_messages_log, scrollwin_rows);

	if (scrollwin_rows >= old.size) {
		win_messages_log.size = old.size;
	}
	return 0;

}

/* Perform connect and login
 * returns -1 on error 0 on success
 */
int do_login(char *hostname, char *username, char *password ) {

	if (manager_connect( &m, hostname ) != 0) return -1;
	status("Connected to %s, trying to login..", hostname);


	if( manager_login( &m, username, password ) != 0) return -1;

	if (manager_loggedin(&m) == 0) {
		status("Connected to %s as %s", hostname, username);

		scrollwin_append( &win_messages_log, "Connected to %s as %s", hostname, username );
		window_draw_messages(win_messages);
		wrefresh(win_messages);

		/* Turn on 'Events' */
		manager_event_mask( &m, 1);

		/* Load active channels */
		do_active_channels();

		strcpy(mgr_host, hostname);
		strcpy(mgr_user, username);
		strcpy(mgr_pass, password);

		return 0;
	}
	return -1;
}

/* Prompt for a channel, show a "channel info" dialog
 * returns -1 on error, 0 on success
 *
 * Wow. This is pretty useless, most info returned here is already known!
 */
int do_channel_info() {

	int i, r, c;
	WINDOW *win_chaninfo;
	int win_chaninfo_rows = 20, win_chaninfo_cols = 40;

	char actionid[32], buf1[64];

	ManagerMessage msg, resp;
	ManagerRecord *rec;

	Channel chan;

	if (manager_loggedin( &m) != 0) {
		status("ERROR: Not logged in");
		return -1;
	}

	if (get_channel_from_id( dbp, chan_sel_id , &chan) != 0) {
		status("ERROR: Channel not found");
		return -1;
	}

	if (manager_event_mask( &m, 0) != 0) return -1;

	sprintf(actionid, "%d", (int) time(NULL) );
	
	manager_init_message( &msg );
	manager_build_message( &msg, "Action", "Status");
	manager_build_message( &msg, "Channel", chan.channel );
	manager_build_message( &msg, "ActionID", actionid);

	if (manager_send_message( &m, &msg) == -1) return do_channel_info_cancel();
	if (manager_recv_message( &m, &msg) != 0) return do_channel_info_cancel();

	if (manager_message_get( &msg, "Message", buf1) != 0) return do_channel_info_cancel();
	if (strcmp(buf1, "Channel status will follow") != 0) return do_channel_info_cancel();

	/* Now we read until we get a 'StatusComplete' for our actionid */
	status("Waiting for server response...");

	if (manager_recv_message( &m, &resp ) != 0) return do_channel_info_cancel();

	status("");

	/* finally show the channel info */

	CENTER( win_chaninfo_rows, win_chaninfo_cols, scrn_rows, scrn_cols, r, c );

	win_chaninfo = create_window( r, c, win_chaninfo_rows, win_chaninfo_cols);

	r = 1;
	for(i=0; i<resp.items; i++) {
		rec = resp.records + i;
		mvwprintw(win_chaninfo, r++, 1, " %d) %s: %s", i + 1, rec->key, rec->value);
	}
	wrefresh(win_chaninfo);

	getch();
	destroy_window(win_chaninfo);
	do_main_window();

	if (manager_event_mask( &m, 1) != 0) return -1;
	return 0;
}

/* Cancel the channel info 
 * always returns -1
 */

int do_channel_info_cancel() {
	if (manager_event_mask( &m, 1) != 0) return -1;
	return -1;
}

/* Hangup the selected call
 * returns 0 on succes, -1 on error
 */
int do_channel_hangup() {
	Channel chan;
	ManagerMessage msg;
	
	if (get_channel_from_id( dbp, chan_sel_id , &chan) != 0) {
		status("ERROR: Channel not found");
		return -1;
	}

	manager_init_message(&msg);
	manager_build_message(&msg, "Action", "Hangup");
	manager_build_message(&msg, "Channel", chan.channel);

	if (manager_send_message( &m, &msg ) == -1) return -1;

	return 0;
}

/* Monitor/Record call
 * returns 0 on succes, -1 on error
 */
int do_monitor() {
	Channel chan;
	ManagerMessage msg;
	char monitor_filename[1024];
	
	if (get_channel_from_id( dbp, chan_sel_id , &chan) != 0) {
		status("ERROR: Channel not found");
		return -1;
	}

	sprintf(monitor_filename, "uid%s-cid%s", chan.uniqueid, chan.callerid );

	manager_init_message(&msg);
	manager_build_message(&msg, "Action", "Monitor");
	manager_build_message(&msg, "Channel", chan.channel);
	manager_build_message(&msg, "File", monitor_filename);
	manager_build_message(&msg, "Mix", "1");

	if (manager_send_message( &m, &msg ) == -1) return -1;
	
	return 0;
}

/* Redirect channel
 * returns 0 on success, -1 on error
 */
int do_redirect() {
	Channel chan, chan2;
	ManagerMessage msg;

	int r, c;
	int win_redirect_rows = 20, win_redirect_cols = 45;
	WINDOW *win_redirect;
	char buf1[4096], extrachan[256];

	if (get_channel_from_id( dbp, chan_sel_id , &chan) != 0) {
		status("ERROR: Channel not found");
		return -1;
	}

	/* Turn off receiving events */
	if (manager_event_mask( &m, 0) != 0) return -1;

	CENTER( win_redirect_rows, win_redirect_cols, scrn_rows, scrn_cols, r, c );

	win_redirect = create_window( r, c, win_redirect_rows, win_redirect_cols );

	r = 2;


	manager_init_message( &msg );
	manager_build_message( &msg , "Action", "Redirect");
	manager_build_message( &msg , "Channel", chan.channel );

	mvwprintw(win_redirect, 1, 1, "Channel: %s", chan.channel );

	ncurses_prompt( win_redirect, r++, 1, "Context", "sip_outbound", buf1, 1);
	if (strlen(buf1) == 0) return do_redirect_cancel( win_redirect );
	manager_build_message( &msg , "Context", buf1);

	ncurses_prompt( win_redirect, r++, 1, "Extension: ", NULL, buf1, 1);
	if (strlen(buf1) == 0) return do_redirect_cancel( win_redirect );
	manager_build_message( &msg , "Exten", buf1);

	ncurses_prompt( win_redirect, r++, 1, "Priority", "1", buf1, 1);
	if (strlen(buf1) == 0) return do_redirect_cancel( win_redirect );
	manager_build_message( &msg , "Priority", buf1);


	if (0) {
		if (channel_get(dbp, chan.link, &chan2) == 0) {
			strcpy(extrachan, chan2.channel );
		} else {
			strcpy(extrachan, "");
		}
	
		ncurses_prompt( win_redirect, r++, 1, "Extra Chan", extrachan, buf1, 1);
		if (strlen(buf1)) {
			manager_build_message( &msg , "ExtraChannel", buf1); 
		}
	}

	if (manager_send_message( &m, &msg ) == -1) return do_redirect_cancel( win_redirect );

	do_redirect_cancel( win_redirect );
	return 0;
}

/* Always returns -1
 */
int do_redirect_cancel( WINDOW *w) {
	if (w) destroy_window(w);
	if (manager_event_mask( &m, 1) != 0) return -1;
	do_main_window();
	return -1;
}

/* Set a Timeout on a channel
 * returns 0 on success, -1 on error
 */
int do_timeout() {
	Channel chan;
	ManagerMessage msg;

	char buf1[4096];
	int timeout;

	if (get_channel_from_id( dbp, chan_sel_id , &chan) != 0) {
		status("ERROR: Channel not found");
		return -1;
	}

	/* Turn off receiving events */
	if (manager_event_mask( &m, 0) != 0) return -1;

	ncurses_prompt(stdscr, scrn_rows - 1, 0,  "Timeout (seconds)", "30", buf1, 1);
	timeout = strtol(buf1, NULL, 0);

	sprintf( buf1, "%d", timeout);

	manager_init_message( &msg );
	manager_build_message( &msg , "Action", "AbsoluteTimeout");
	manager_build_message( &msg , "Channel", chan.channel);
	manager_build_message( &msg , "Timeout", buf1);

	if (manager_send_message( &m, &msg ) == -1) return do_timeout_cancel();

	do_timeout_cancel();
	return 0;

}
/* Always returns -1
 */
int do_timeout_cancel() {
	if (manager_event_mask( &m, 1) != 0) return -1;
	do_main_window();
	return -1;
}



/* Parse program arguments
 * return 0 on success, -1 on error
 */
int nami_arguments(int argc, char **argv) {
	char c;
	char *opt_str = "hc:t";

   while ((c = getopt (argc, argv, opt_str)) != -1) {

		switch (c) {

			case 'h':
				nami_help();
				break;

			case 't':
				nami_testconfig();
				break;

			case 'c':
				nami_configfile = optarg;
				break;

			case '?': // unknown arg....

				/* fprintf(stderr, "Unknown argument: %c\n", optopt); */

			default:
				return -1;
				break;
		}


	}
	return 0;
}

/* Print help and exit
 */
void nami_help() {
	fprintf(stderr, "nami %s (Built: %s)\n", NAMI_VERSION, NAMI_BUILD_DATE);
	fprintf(stderr, "-c cfgfile       Use alt configuration file\n");
	fprintf(stderr, "-h               print help and exit\n");
	fprintf(stderr, "-t               Test configuration");
	fprintf(stderr, "\n");
	exit(1);
}


void nami_testconfig() {
	nami_load_config();


	print_config( &config );
	exit(0);
}
