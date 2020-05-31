#ifndef _OUTPUT_H_
#define _OUTPUT_H_

#include "color.h"
#include <cstdarg>
#include <string>
#include <vector>

//      LINE_NESW  - X for on, O for off
#define LINE_XOXO 4194424
#define LINE_OXOX 4194417
#define LINE_XXOO 4194413
#define LINE_OXXO 4194412
#define LINE_OOXX 4194411
#define LINE_XOOX 4194410
#define LINE_XXXO 4194420
#define LINE_XXOX 4194422
#define LINE_XOXX 4194421
#define LINE_OXXX 4194423
#define LINE_XXXX 4194414

bool reject_not_whitelisted_printf(const std::string& src);

void mvputch(int y, int x, nc_color FG, long ch);
void wputch(WINDOW* w, nc_color FG, long ch);
void mvwputch(WINDOW* w, int y, int x, nc_color FG, long ch);
void mvputch_inv(int y, int x, nc_color FG, long ch);
void mvwputch_inv(WINDOW *w, int y, int x, nc_color FG, long ch);
void mvputch_hi(int y, int x, nc_color FG, long ch);
void mvwputch_hi(WINDOW *w, int y, int x, nc_color FG, long ch);

template<class...Args>
void mvprintz(int y, int x, nc_color FG, const char* mes, Args...params)
{
	if (reject_not_whitelisted_printf(mes)) return;
	attron(FG);
	mvprintw(y, x, mes, params...);
	attroff(FG);
}

template<class...Args>
void mvwprintz(WINDOW* w, int y, int x, nc_color FG, const char* mes, Args...params)
{
	if (reject_not_whitelisted_printf(mes)) return;
	wattron(w, FG);
	mvwprintw(w, y, x, mes, params...);
	wattroff(w, FG);
}

template<class...Args>
void printz(nc_color FG, const char* mes, Args...params)
{
	if (reject_not_whitelisted_printf(mes)) return;
	attron(FG);
	printw(mes, params...);
	attroff(FG);
}

template<class...Args>
void wprintz(WINDOW* w, nc_color FG, const char* mes, Args...params)
{
	if (reject_not_whitelisted_printf(mes)) return;
	wattron(w, FG);
	wprintw(w, mes, params...);
	wattroff(w, FG);
}

void draw_tabs(WINDOW *w, int active_tab, const char* const labels[]);

void debugmsg(const char *mes, ...);
bool query_yn(const char *mes, ...);
int  query_int(const char *mes, ...);
std::string string_input_popup(const char *mes, ...);
std::string string_input_popup(int max_length, const char *mes, ...);
char popup_getkey(const char *mes, ...);
int  menu_vec(const char *mes, const std::vector<std::string>& options);
int  menu(const char *mes, ...);
void popup_top(const char *mes, ...); // Displayed at the top of the screen
void popup(const char *mes, ...);
void popup_nowait(const char *mes, ...); // Doesn't wait for spacebar
void full_screen_popup(const char *mes, ...);

nc_color hilite(nc_color c);
nc_color invert_color(nc_color c);
nc_color red_background(nc_color c);
nc_color rand_color();
char rand_char();
long special_symbol (char sym);

#endif
