#ifndef __CATACURSE__
#define __CATACURSE__

// CBI tileset port
// * assumes wide PDCurses (32-bit chtype)
// * worked by overriding A_INVIS as A_TILESET
// * does not react to color, instead it replaces code points

// mappling tiles to char,color pairs would allow a more transparent override scheme (but won't work for other reasons)

typedef int	chtype;
typedef unsigned short	attr_t;
typedef unsigned int u_int32_t;

#define __CHARTEXT	0x000000ff	/* bits for 8-bit characters */             //<---------not used
#define __NORMAL	0x00000000	/* Added characters are normal. */          //<---------not used
#define __STANDOUT	0x00000100	/* Added characters are standout. */        //<---------not used
#define __UNDERSCORE	0x00000200	/* Added characters are underscored. */ //<---------not used
#define __REVERSE	0x00000400	/* Added characters are reverse             //<---------not used
					   video. */
#define __BLINK		0x00000800	/* Added characters are blinking. */
#define __DIM		0x00001000	/* Added characters are dim. */             //<---------not used
#define __BOLD		0x00002000	/* Added characters are bold. */
#define __BLANK		0x00004000	/* Added characters are blanked. */         //<---------not used
#define __PROTECT	0x00008000	/* Added characters are protected. */       //<---------not used
#define __ALTCHARSET	0x00010000	/* Added characters are ACS */          //<---------not used
#define __COLOR		0x03fe0000	/* Color bits */
#define __ATTRIBUTES	0x03ffff00	/* All 8-bit attribute bits */          //<---------not used

struct WINDOW;

#define	A_NORMAL	__NORMAL
#define	A_STANDOUT	__STANDOUT
#define	A_UNDERLINE	__UNDERSCORE
#define	A_REVERSE	__REVERSE
#define	A_BLINK		__BLINK
#define	A_DIM		__DIM
#define	A_BOLD		__BOLD
#define	A_BLANK		__BLANK
#define	A_PROTECT	__PROTECT
#define	A_ALTCHARSET	__ALTCHARSET
#define	A_ATTRIBUTES	__ATTRIBUTES
#define	A_CHARTEXT	__CHARTEXT
#define	A_COLOR		__COLOR

#define	COLOR_BLACK	0x00        //RGB{0,0,0}
#define	COLOR_RED	0x01        //RGB{196, 0, 0}
#define	COLOR_GREEN	0x02        //RGB{0,196,0}
#define	COLOR_YELLOW	0x03    //RGB{196,180,30}
#define	COLOR_BLUE	0x04        //RGB{0, 0, 196}
#define	COLOR_MAGENTA	0x05    //RGB{196, 0, 180}
#define	COLOR_CYAN	0x06        //RGB{0, 170, 200}
#define	COLOR_WHITE	0x07        //RGB{196, 196, 196}

#define	COLOR_PAIR(n)	((((u_int32_t)n) << 17) & A_COLOR)
#define	PAIR_NUMBER(n)	((((u_int32_t)n) & A_COLOR) >> 17)

#define    KEY_MIN        0x101    /* minimum extended key value */ //<---------not used
#define    KEY_BREAK      0x101    /* break key */                  //<---------not used
#define    KEY_DOWN       0x102    /* down arrow */
#define    KEY_UP         0x103    /* up arrow */
#define    KEY_LEFT       0x104    /* left arrow */
#define    KEY_RIGHT      0x105    /* right arrow*/
#define    KEY_HOME       0x106    /* home key */                   //<---------not used
#define    KEY_BACKSPACE  0x107    /* Backspace */                  //<---------not used

/* Curses external declarations. */

#define stdscr 0
//WINDOW *stdscr; //Define this for portability, strscr will basically be window[0] anyways

#define getmaxyx(w, y, x)  (y = getmaxy(w), x = getmaxx(w))

//Curses Functions
#define	ERR	(-1)			/* Error return. */
#define	OK	(0)			/* Success return. */
WINDOW *newwin(int nlines, int ncols, int begin_y, int begin_x);
int delwin(WINDOW *win);
int wborder(WINDOW *win, chtype ls, chtype rs, chtype ts, chtype bs, chtype tl, chtype tr, chtype bl, chtype br);
int wrefresh(WINDOW *win);
int refresh(void);
int getch(void);
int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...);
int mvprintw(int y, int x, const char *fmt, ...);
int werase(WINDOW *win);
int start_color(void);
int init_pair(short pair, short f, short b);

int clear(void);
int erase(void);
int endwin(void);
int mvwaddch(WINDOW *win, int y, int x, const chtype ch);
int wclear(WINDOW *win);
int wprintw(WINDOW *win, const char *fmt, ...);
WINDOW *initscr(void);
int mvaddch(int y, int x, const chtype ch);
int wattron(WINDOW *win, int attrs);
int wattroff(WINDOW *win, int attrs);
int attron(int attrs);
int attroff(int attrs);
int waddch(WINDOW *win, const chtype ch);
int printw(const char *fmt,...);
int getmaxx(WINDOW *win);
int getmaxy(WINDOW *win);

int wmove(WINDOW* win, int y, int x);
inline int move(int y, int x) { return wmove(stdscr, y, x); }

int waddnstr(WINDOW* win, const char* str, int n);
inline int waddstr(WINDOW* win, const char* str) { return waddnstr(win, str, -1); }
inline int addstr(const char* str) { return waddstr(stdscr, str); }
inline int addnstr(const char* str, int n) { return waddnstr(stdscr, str, n); }

int mvwaddnstr(WINDOW* win, int y, int x, const char* str, int n);
inline int mvwaddstr(WINDOW* win, int y, int x, const char* str) { return mvwaddnstr(win, y, x, str, -1); }
inline int mvaddstr(int y, int x, const char* str) { return mvwaddstr(stdscr, y, x, str); }
inline int mvaddnstr(int y, int x, const char* str, int n) { return mvwaddnstr(stdscr, y, x, str, n); }

//PORTABILITY, DUMMY FUNCTIONS
int noecho(void);
int cbreak(void);
int keypad(WINDOW* faux, bool bf);
int curs_set(int visibility);
void timeout(int delay);

#define CURSES_HAS_TILESET 1
// tileset extensions
bool load_tile(const char* src);
void flush_tilesheets();
bool mvwaddbgtile(WINDOW *win, int y, int x, const char* bgtile);
bool mvwaddfgtile(WINDOW *win, int y, int x, const char* fgtile);

#endif
