#include <curses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


#ifndef HAVE_NCURSES_SUPPORT_H
#define HAVE_NCURSES_SUPPORT_H

/* structures */

#define MAX_SCROLL_WINDOW_SIZE 50

typedef struct {

	char line[4096];

	time_t tm;

} ScrollingWindowLine;


typedef struct  {

	int size, max;
	ScrollingWindowLine lines[MAX_SCROLL_WINDOW_SIZE];

} ScrollingWindow;


/* macros */


/* Calculate cords to Center a box (r1, c1) inside box (r2, c2), updating (r, c) */
#define CENTER( r1, c1, r2, c2, r, c ) \
				r = (r2 - r1) / 2; \
				c = (c2 - c1) / 2; \

/* prototypes */
int ncurses_start();
int ncurses_finish(int sig);

int status(const char *fmt, ...);
int status_time();


WINDOW *create_window(int startr, int startc, int height, int width);
void destroy_window(WINDOW *local_win);

int ncurses_prompt( WINDOW *w, int r, int c, char *prompt, char *def, char *buf, int echo);

int ncurses_resize(int r, int c);

int window_draw_border( WINDOW *w );


int scrollwin_init( ScrollingWindow *win, int max);
int scrollwin_append( ScrollingWindow *win, const char *fmt, ...);
int scrollwin_append_ln( ScrollingWindow *win, ScrollingWindowLine *ln);


#endif
