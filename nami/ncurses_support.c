#include "ncurses_support.h"
#include "config.h"

extern config_t config;

int n_scrn_rows, n_scrn_cols;

int status_tm = 0;

int ncurses_resize(int r, int c) {
	resizeterm(r, c);
	n_scrn_rows = r;
	n_scrn_cols = c;
	return 0;
}

int ncurses_start() {

    /* initialize your non-curses data structures here */


    initscr();      /* initialize the curses library */
    keypad(stdscr, TRUE);  /* enable keyboard mapping */
    nonl();         /* tell curses not to do NL->CR/NL on output */
    cbreak();       /* take input chars one at a time, no wait for \n */
    noecho();         /* no echo input */

	if (has_colors()) {
		start_color();

		/*
		* Simple color assignment, often all we need.  Color pair 0 cannot
		* be redefined.  This example uses the same value for the color
		* pair as for the foreground color, though of course that is not
		* necessary:
		*/
		init_pair(1, COLOR_RED,     COLOR_BLACK);
		init_pair(2, COLOR_GREEN,   COLOR_BLACK);
		init_pair(3, COLOR_YELLOW,  COLOR_BLACK);
		init_pair(4, COLOR_BLUE,    COLOR_BLACK);
		init_pair(5, COLOR_CYAN,    COLOR_BLACK);
		init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(7, COLOR_WHITE,   COLOR_BLACK);
	}


	getmaxyx(stdscr, n_scrn_rows, n_scrn_cols);
	return 0;
}


int ncurses_finish(int sig) {
	endwin();
	exit(0);
}

int status_time() { return status_tm; }

int status(const char *fmt, ...) {

	time( (time_t *) &status_tm );

   /* Guess we need no more than 100 bytes. */
   int n, size = 100, buf_len;
   char *p;
   va_list ap;

	char buf1[4096];
	int tm_epoch;
	struct tm *tm1;


   if ((p = malloc (size)) == NULL) return -1;

   while (1) {
      va_start(ap, fmt);
      n = vsnprintf (p, size, fmt, ap);
      va_end(ap);

      if (n > -1 && n < size) break;

      if (n > -1) {
         size =  n + 1;
      } else {
         size *= 2;
      }

      if ((p = realloc (p, size)) == NULL) return -1;
   }

   buf_len = strlen(p);


	for(n=0; n<n_scrn_cols; n++) {
		mvprintw(n_scrn_rows - 1, n, " ");
	}

	if (buf_len) {
		time( (time_t *) &tm_epoch );
		tm_epoch -= (3600 * 5 );
		tm1 = gmtime( (time_t *) &tm_epoch );
		strftime( buf1, sizeof(buf1) - 1, (const char *) config.date_format, tm1);

		mvprintw(n_scrn_rows - 1, 0, "%s ", buf1);

		printw( "%s", p );
	}

	move(n_scrn_rows - 1, n_scrn_cols - 1);
	refresh();
   free(p);

	return 0;
}

WINDOW *create_window(int startr, int startc, int height, int width) {

	WINDOW *local_win;

	local_win = newwin(height, width, startr, startc);
	box(local_win, 0 , 0);		
	wrefresh(local_win);
	return local_win;
}


int window_draw_border( WINDOW *w) {
	box(w, 0, 0);
	return 0;
}




void destroy_window(WINDOW *local_win) {	
	int i, j;
	int r, c;

	/* box(local_win, ' ', ' '); : This won't produce the desired
	 * result of erasing the window. It will leave it's four corners 
	 * and so an ugly remnant of window. 
	 */
	wborder(local_win, ' ', ' ', ' ',' ',' ',' ',' ',' ');
	/* The parameters taken are 
	 * 1. win: the window on which to operate
	 * 2. ls: character to be used for the left side of the window 
	 * 3. rs: character to be used for the right side of the window 
	 * 4. ts: character to be used for the top side of the window 
	 * 5. bs: character to be used for the bottom side of the window 
	 * 6. tl: character to be used for the top left corner of the window 
	 * 7. tr: character to be used for the top right corner of the window 
	 * 8. bl: character to be used for the bottom left corner of the window 
	 * 9. br: character to be used for the bottom right corner of the window
	 */

	getmaxyx(local_win, r, c);

	// clear the window contents...
	for(i=1; i<r; i++) {
		for(j=1; j<c; j++) {
			mvwprintw(local_win, i, j, " ");
		}
	}

	wrefresh(local_win);
	delwin(local_win);
}


int ncurses_prompt( WINDOW *w, int r, int c, char *prompt, char *def, char *rbuf, int doecho) {

	int l;

	nocbreak();

	status("");

	if (def) {
		mvwprintw(w, r, c, "%s [%s]: ", prompt, def);

	} else {
		mvwprintw(w, r, c, "%s", prompt);
	}
	wrefresh(w);


	if (doecho == 1) echo();
	wgetstr(w, rbuf);
	if (doecho == 1) noecho();

	l = strlen(rbuf);

	if (rbuf[l - 1] == '\n') rbuf[--l] = 0;

	if (def && l == 0) {
		strcpy(rbuf, def);
	}

	status("");
	cbreak();

	return 0;
}



int scrollwin_init( ScrollingWindow *win, int max) {
	win->size = 0;
	win->max = max;
	return 0;
}

int scrollwin_append( ScrollingWindow *win, const char *fmt, ...) {

	ScrollingWindowLine ln;

   /* Guess we need no more than 100 bytes. */
   int r, n, size = 100, buf_len;
   char *p;
   va_list ap;

	char buf1[4096];
	int tm_epoch;
	struct tm *tm1;

   if ((p = malloc (size)) == NULL) return -1;

   while (1) {
      va_start(ap, fmt);
      n = vsnprintf (p, size, fmt, ap);
      va_end(ap);

      if (n > -1 && n < size) break;

      if (n > -1) {
         size =  n + 1;
      } else {
         size *= 2;
      }

      if ((p = realloc (p, size)) == NULL) return -1;
   }

	time( (time_t *) &tm_epoch );
	tm_epoch -= (3600 * 5 );
	tm1 = gmtime( (time_t *) &tm_epoch );
	strftime( buf1, sizeof(buf1) - 1, (const char *) config.date_format, tm1);

   buf_len = strlen(p);
	sprintf(ln.line, "%s %s", buf1, p);
	free(p);

	r = scrollwin_append_ln( win, &ln);
	if (r != 0) return -1;
	return 0;
}

int scrollwin_append_ln( ScrollingWindow *win, ScrollingWindowLine *ln) {

	int i;
	ScrollingWindowLine *lnp1, *lnp2;

	if (win->size == win->max) {

		for(i=1; i<win->max; i++) {

			lnp1 = win->lines + (i - 1);
			lnp2 = win->lines + i;

			memcpy( lnp1, lnp2, sizeof(ScrollingWindowLine) );
		}

		lnp1 = win->lines + (win->size - 1);

	} else {

		lnp1 = win->lines + win->size++;
	}
	memcpy( lnp1, ln, sizeof(ScrollingWindowLine));

	return 0;
}
