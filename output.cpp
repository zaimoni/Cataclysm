#include "output.h"
#include "options.h"
#include "ui.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#ifndef NDEBUG
#include <stdexcept>
#endif

static FILE* stderr_log = nullptr;

bool have_used_stderr_log() { return stderr_log; }

const std::string& get_stderr_logname() {
    static std::string ooao;
    if (ooao.empty()) {
        ooao = std::string("stderr")+std::to_string(time(nullptr))+".txt";
    }
    return ooao;
}

FILE* get_stderr_log() {
    // \todo would need a lock if multi-threaded
    if (!stderr_log) {
        decltype(auto) name = get_stderr_logname();
        stderr_log = fopen(name.c_str(),"ab");
    }
    return stderr_log;
}

void debuglog(const std::string& msg) {
    if (decltype(auto) err = get_stderr_log()) {
        fputs(msg.c_str(), err);
        fputc('\n', err);
    }
}

nc_color hilite(nc_color c)
{
 switch (c) {
  case c_white:		return h_white;
  case c_ltgray:	return h_ltgray;
  case c_dkgray:	return h_dkgray;
  case c_red:		return h_red;
  case c_green:		return h_green;
  case c_blue:		return h_blue;
  case c_cyan:		return h_cyan;
  case c_magenta:	return h_magenta;
  case c_brown:		return h_brown;
  case c_ltred:		return h_ltred;
  case c_ltgreen:	return h_ltgreen;
  case c_ltblue:	return h_ltblue;
  case c_ltcyan:	return h_ltcyan;
  case c_pink:		return h_pink;
  case c_yellow:	return h_yellow;
 }
 return h_white;
}

nc_color invert_color(nc_color c)
{
 if (option_table::get()[OPT_NO_CBLINK]) {
  switch (c) {
   case c_white:
   case c_ltgray:
   case c_dkgray:  return i_ltgray;
   case c_red:
   case c_ltred:   return i_red;
   case c_green:
   case c_ltgreen: return i_green;
   case c_blue:
   case c_ltblue:  return i_blue;
   case c_cyan:
   case c_ltcyan:  return i_cyan;
   case c_magenta:
   case c_pink:    return i_magenta;
   case c_brown:
   case c_yellow:  return i_brown;
  }
 }
 switch (c) {
  case c_white:   return i_white;
  case c_ltgray:  return i_ltgray;
  case c_dkgray:  return i_dkgray;
  case c_red:     return i_red;
  case c_green:   return i_green;
  case c_blue:    return i_blue;
  case c_cyan:    return i_cyan;
  case c_magenta: return i_magenta;
  case c_brown:   return i_brown;
  case c_yellow:  return i_yellow;
  case c_ltred:   return i_ltred;
  case c_ltgreen: return i_ltgreen;
  case c_ltblue:  return i_ltblue;
  case c_ltcyan:  return i_ltcyan;
  case c_pink:    return i_pink;
 }
 return i_white;
}

nc_color red_background(nc_color c)
{
 switch (c) {
  case c_white:		return c_white_red;
  case c_ltgray:	return c_ltgray_red;
  case c_dkgray:	return c_dkgray_red;
  case c_red:		return c_red_red;
  case c_green:		return c_green_red;
  case c_blue:		return c_blue_red;
  case c_cyan:		return c_cyan_red;
  case c_magenta:	return c_magenta_red;
  case c_brown:		return c_brown_red;
  case c_ltred:		return c_ltred_red;
  case c_ltgreen:	return c_ltgreen_red;
  case c_ltblue:	return c_ltblue_red;
  case c_ltcyan:	return c_ltcyan_red;
  case c_pink:		return c_pink_red;
  case c_yellow:	return c_yellow_red;
 }
 return c_white_red;
}

void mvputch(int y, int x, nc_color FG, long ch)
{
 attron(FG);
 mvaddch(y, x, ch);
 attroff(FG);
}

void mvwputch(WINDOW* w, int y, int x, nc_color FG, long ch)
{
 wattron(w, FG);
 mvwaddch(w, y, x, ch);
 wattroff(w, FG);
}

void mvwputch_inv(WINDOW* w, int y, int x, nc_color FG, long ch)
{
 nc_color HC = invert_color(FG);
 wattron(w, HC);
 mvwaddch(w, y, x, ch);
 wattroff(w, HC);
}

void mvwputch_hi(WINDOW* w, int y, int x, nc_color FG, long ch)
{
 nc_color HC = hilite(FG);
 wattron(w, HC);
 mvwaddch(w, y, x, ch);
 wattroff(w, HC);
}

void wputch(WINDOW* w, nc_color FG, long ch)
{
    wattron(w, FG);
    waddch(w, ch);
    wattroff(w, FG);
}

void draw_hline(WINDOW* w, int y, nc_color FG, long ch, int x0, int x1)
{
    if (0 > x0) x0 = 0;
    if (0 > x1) x1 = getmaxx(w);
    wattron(w, FG);
    for (int x = x0; x < x1; x++) mvwaddch(w, y, x, ch);
    wattroff(w, FG);
}

#ifndef NDEBUG
static void reject_unescaped_percent(const std::string& src)
{
    auto i = src.find('%');
    while (std::string::npos != i) {
        if (src.size() <= i + 1) throw std::logic_error("unescaped %");
        if ('%' != src.data()[i + 1]) throw std::logic_error("unescaped %");
        // technically not an error, but historically has not happened and is a symptom of now-buggy overescaping
        if (src.size() > i + 3 && '%' == src.data()[i + 2] && '%' == src.data()[i + 2]) throw std::logic_error("multi-escaped %");
        if (src.size() <= i + 2) return;
        i = src.find('%', i + 2);
    }
}
#else
#define reject_unescaped_percent(A)
#endif

/// <summary>
/// MSVC++ C standard library crashes to desktop if faced with an illegal printf escape sequence.
/// Whitelist the leading characters after %, that are used.
/// </summary>
bool reject_not_whitelisted_printf(const std::string& src)
{
    auto i = src.find('%');
    while (std::string::npos != i) {
#ifndef NDEBUG
        if (src.size() <= i + 1) throw std::logic_error("unescaped %");
        if (!strchr("%sidc.", src[i + 1])) throw std::logic_error("unexpected escape % use");
#else
        if (src.size() <= i + 1) {
            debugmsg("unescaped percent");
            return true;
        }
        if (!strchr("%sidc.", src[i + 1])) {
            debugmsg("unexpected escape percent use");
            return true;
        }
#endif
        if (src.size() <= i + 2) return false;
        i = src.find('%', i + 2);
    }
    return false;
}

void mvwaddstrz(WINDOW* w, int y, int x, nc_color FG, const char* mes)
{
    if (!mes) return;
    wattron(w, FG);
    mvwaddstr(w, y, x, mes);
    wattroff(w, FG);
}

void waddstrz(WINDOW* w, nc_color FG, const char* mes)
{
    if (!mes) return;
    wattron(w, FG);
    waddstr(w, mes);
    wattroff(w, FG);
}

void draw_tabs(WINDOW* w, int active_tab, const char* const labels[])
{
 const int win_width = getmaxx(w);
 int labels_size = 0;
 while (labels[labels_size]) labels_size++;

 int total_width = 0;
 for (int i = 0; i < labels_size; i++)
  total_width += strlen(labels[i]) + 6; // "< |four| >"

 werase(w); // not really ACID

// Draw the line under the tabs
 for (int x = 0; x < win_width; x++)
  mvwputch(w, 2, x, c_white, LINE_OXOX);

 if (total_width > win_width) return;	// insufficient space

// Extra "buffer" space per each side of each tab
 double buffer_extra = (win_width - total_width) / (labels_size * 2.);
 const int buffer = int(buffer_extra);
// Set buffer_extra to (0, 1); the "extra" whitespace that builds up
 buffer_extra = buffer_extra - buffer;
 int xpos = 0;
 double savings = 0;

 for (int i = 0; i < labels_size; i++) {
  int length = strlen(labels[i]);
  xpos += buffer + 2;
  savings += buffer_extra;
  if (savings > 1) {
   savings--;
   xpos++;
  }
  const int xpos_r = xpos + length + 1;

  mvwputch(w, 0, xpos, c_white, LINE_OXXO);
  mvwputch(w, 1, xpos, c_white, LINE_XOXO);
  mvwputch(w, 0, xpos_r, c_white, LINE_OOXX);
  mvwputch(w, 1, xpos_r, c_white, LINE_XOXO);
  if (i == active_tab) {
   mvwputch(w, 1, xpos - 2, h_white, '<');
   mvwputch(w, 1, xpos_r + 2, h_white, '>');
   mvwputch(w, 2, xpos, c_white, LINE_XOOX);
   mvwputch(w, 2, xpos_r, c_white, LINE_XXOO);
   reject_unescaped_percent(labels[i]);
   mvwprintz(w, 1, xpos + 1, h_white, labels[i]);
   for (int x = xpos + 1; x < xpos_r; x++) {
    mvwputch(w, 0, x, c_white, LINE_OXOX);
    mvwputch(w, 2, x, c_black, 'x');
   }
  } else {
   mvwputch(w, 2, xpos, c_white, LINE_XXOX);
   mvwputch(w, 2, xpos_r, c_white, LINE_XXOX);
   reject_unescaped_percent(labels[i]);
   mvwprintz(w, 1, xpos + 1, c_white, labels[i]);
   for (int x = xpos + 1; x < xpos_r; x++)
    mvwputch(w, 0, x, c_white, LINE_OXOX);
  }
  xpos += length + 1 + buffer;
 }
 wrefresh(w);
}

void debugmsg(const char* mes, ...)
{
 if (reject_not_whitelisted_printf(mes)) return;
 va_list ap;
 va_start(ap, mes);
 char buff[1024];
#ifdef _MSC_VER
 vsprintf_s<sizeof(buff)>(buff, mes, ap);
#else
 vsnprintf(buff, sizeof(buff), mes, ap);
#endif
 va_end(ap);
 reject_unescaped_percent(buff);
 attron(c_red);
 mvprintw(0, 0, "DEBUG: %s                \n  Press spacebar...", buff);
 while(getch() != ' ');
 attroff(c_red);
}

bool query_yn(const char* mes, ...)
{
 if (reject_not_whitelisted_printf(mes)) return false;
 const bool force_uc = option_table::get()[OPT_FORCE_YN];
 va_list ap;
 va_start(ap, mes);
 char buff[1024];
#ifdef _MSC_VER
 vsprintf_s<sizeof(buff)>(buff, mes, ap);
#else
 vsnprintf(buff, sizeof(buff), mes, ap);
#endif
 va_end(ap);
 int win_width = strlen(buff) + 26; // soft limit on main prompt SCREEN_WIDTH-26; C:Whales 54
 WINDOW* w = newwin(3, win_width, (VIEW - 3) / 2, 0);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w, 1, 1, c_ltred, "%s (%s)", buff,
           (force_uc ? "Y/N - Case Sensitive" : "y/n"));
 wrefresh(w);
 int ch;
 do ch = getch();
 while (ch != 'Y' && ch != 'N' && (force_uc || (ch != 'y' && ch != 'n')));
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh();
 return ch == 'Y' || ch == 'y';
}

int query_int(const char* mes, ...)
{
 if (reject_not_whitelisted_printf(mes)) return 9;
 va_list ap;
 va_start(ap, mes);
 char buff[1024];
#ifdef _MSC_VER
 vsprintf_s<sizeof(buff)>(buff, mes, ap);
#else
 vsnprintf(buff, sizeof(buff), mes, ap);
#endif
 va_end(ap);
 int win_width = strlen(buff) + 10; // soft limit on prompt SCREEN_WIDTH-10; C:Whales 70
 WINDOW* w = newwin(3, win_width, (VIEW - 3) / 2, 0);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w, 1, 1, c_ltred, "%s (0-9)", buff);
 wrefresh(w);

 int temp;
 do
  temp = getch();
 while ((temp - '0')<0 || (temp - '0')>9);
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh();

 return temp-48;
}

std::string string_input_popup(const char* mes, ...)
{
 std::string ret;
 if (reject_not_whitelisted_printf(mes)) return ret;
 va_list ap;
 va_start(ap, mes);
 char buff[1024];
#ifdef _MSC_VER
 vsprintf_s<sizeof(buff)>(buff, mes, ap);
#else
 vsnprintf(buff, sizeof(buff), mes, ap);
#endif
 va_end(ap);
 reject_unescaped_percent(buff);
 int startx = strlen(buff) + 2; // soft limit on prompt SCREEN_WIDTH-2; C:Whales 78
 WINDOW* w = newwin(3, SCREEN_WIDTH, (VIEW - 3) / 2, 0);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w, 1, 1, c_ltred, "%s", buff);
 for (int i = startx + 1; i < 79; i++)
  mvwputch(w, 1, i, c_ltgray, '_');
 int posx = startx;
 mvwputch(w, 1, posx, h_ltgray, '_');
 do {
  wrefresh(w);
  int ch = getch();
  if (ch == KEY_ESCAPE) {
   werase(w);
   wrefresh(w);
   delwin(w);
   refresh();
   return "";
  } else if (ch == '\n') {
   werase(w);
   wrefresh(w);
   delwin(w);
   refresh();
   return ret;
  } else if ((ch == KEY_BACKSPACE || ch == 127) && posx > startx) {
// Move the cursor back and re-draw it
   ret = ret.substr(0, ret.size() - 1);
   mvwputch(w, 1, posx, c_ltgray, '_');
   posx--;
   mvwputch(w, 1, posx, h_ltgray, '_');
  } else {
   ret += ch;
   mvwputch(w, 1, posx, c_magenta, ch);
   wputch(w, h_ltgray, '_');
  }
 } while (true);
}

std::string string_input_popup(int max_length, const char* mes, ...)
{
 std::string ret;
 if (reject_not_whitelisted_printf(mes)) return ret;
 va_list ap;
 va_start(ap, mes);
 char buff[1024];
#ifdef _MSC_VER
 vsprintf_s<sizeof(buff)>(buff, mes, ap);
#else
 vsnprintf(buff, sizeof(buff), mes, ap);
#endif
 va_end(ap);
 reject_unescaped_percent(buff);
 int startx = strlen(buff) + 2; // soft limit on prompt length SCREEN_WIDTH-2; C:Whales 78
 WINDOW* w = newwin(3, SCREEN_WIDTH, (VIEW - 3) / 2, 0);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w, 1, 1, c_ltred, "%s", buff);
 for (int i = startx + 1; i < 79; i++)
  mvwputch(w, 1, i, c_ltgray, '_');
 int posx = startx;
 mvwputch(w, 1, posx, h_ltgray, '_');
 do {
  wrefresh(w);
  int ch = getch();
  if (ch == KEY_ESCAPE) {
   werase(w);
   wrefresh(w);
   delwin(w);
   refresh();
   return "";
  } else if (ch == '\n') {
   werase(w);
   wrefresh(w);
   delwin(w);
   refresh();
   return ret;
  } else if ((ch == KEY_BACKSPACE || ch == 127) && posx > startx) {
// Move the cursor back and re-draw it
   ret = ret.substr(0, ret.size() - 1);
   mvwputch(w, 1, posx, c_ltgray, '_');
   posx--;
   mvwputch(w, 1, posx, h_ltgray, '_');
  } else if(ret.size() < max_length || max_length == 0) {
   ret += ch;
   mvwputch(w, 1, posx, c_magenta, ch);
   wputch(w, h_ltgray, '_');
  }
 } while (true);
}

char popup_getkey(const char* mes, ...)
{
 if (reject_not_whitelisted_printf(mes)) return ' ';
 va_list ap;
 va_start(ap, mes);
 char buff[8192];
#ifdef _MSC_VER
 vsprintf_s<sizeof(buff)>(buff, mes, ap);
#else
 vsnprintf(buff, sizeof(buff), mes, ap);
#endif
 va_end(ap);
 std::string tmp = buff;
 int width = 0;
 int height = 2;
 size_t pos = tmp.find_first_of('\n');
 while (pos != std::string::npos) {
  height++;
  if (pos > width) width = pos;
  tmp = tmp.substr(pos + 1);
  pos = tmp.find_first_of('\n');
 }
 if (width == 0 || tmp.length() > width) width = tmp.length();
 width += 2;
 if (height > VIEW) height = VIEW;
 WINDOW* w = newwin(height + 1, width, (VIEW - height) / 2, (SCREEN_WIDTH - width) / 2);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 tmp = buff;
 pos = tmp.find_first_of('\n');
 int line_num = 0;
 while (pos != std::string::npos) {
  std::string line = tmp.substr(0, pos);
  line_num++;
  reject_unescaped_percent(line);
  mvwprintz(w, line_num, 1, c_white, line.c_str());
  tmp = tmp.substr(pos + 1);
  pos = tmp.find_first_of('\n');
 }
 line_num++;
 reject_unescaped_percent(tmp);
 mvwprintz(w, line_num, 1, c_white, tmp.c_str());

 wrefresh(w);
 int ch = getch();
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh();
 return ch;
}

int menu_vec(const char* mes, const std::vector<std::string>& options)  // \todo? ESC to cancel
{
 if (options.size() == 0) {
  debugmsg("0-length menu (\"%s\")", mes);
  return -1;
 }
#ifndef NDEBUG
 if (VIEW - 3 < options.size()) throw std::logic_error("menu height exceeds screen height");
 if (36 < options.size()) throw std::logic_error("menu contains non-selectable options");
#endif
 const int height = 3 + options.size();
 int width = strlen(mes) + 2;
 for (const auto& opt : options) {
	 const auto test = opt.length() + 6;
	 if (test > width) width = test;
 }
 WINDOW* w = newwin(height, width, 1, 10);	// \todo ensure this works even when VIEW<height
 wattron(w, c_white);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 reject_unescaped_percent(mes);
 mvwprintw(w, 1, 1, mes);
 for (int i = 0; i < options.size(); i++)
  mvwprintw(w, i + 2, 1, "%c: %s", (i < 9? i + '1' :
                                   (i == 9? '0' : 'a' + i - 10)),
            options[i].c_str());
 wrefresh(w);
 int res;
 do {
  int ch = getch();
  if (ch >= '1' && ch <= '9')
   res = ch - '1' + 1;
  else if (ch == '0')
   res = 10;
  else if (ch >= 'a' && ch <= 'z')
   res = ch - 'a' + 11;
  else
   res = -1;
  if (res > options.size()) res = -1;
 } while (res == -1);
 werase(w);
 wrefresh(w);
 delwin(w);
 return (res);
}

int menu(const char* mes, const std::initializer_list<std::string>& opts)
{
 return (menu_vec(mes, opts));
}

/// <returns>(width, height)</returns>
static std::pair<int, int> _popup_size(std::string tmp)
{
    std::pair<int, int> ret(0, 2);

    size_t pos = tmp.find_first_of('\n');
    while (pos != std::string::npos) {
        ret.second++;
        if (pos > ret.first) ret.first = pos;
        tmp = tmp.substr(pos + 1);
        pos = tmp.find_first_of('\n');
    }
    if (ret.first == 0 || tmp.length() > ret.first)
        ret.first = tmp.length();
    ret.first += 2;
    if (ret.second > VIEW) ret.second = VIEW;
    return ret;
}

static WINDOW* _render_popup(std::string tmp, const std::pair<int, int>& dims, int y0)
{
    WINDOW* w = newwin(dims.second + 1, dims.first, y0, int((SCREEN_WIDTH - dims.first) / 2));
    wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
        LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX);
    size_t pos;
    int line_num = 0;
    while ((pos = tmp.find_first_of('\n')) != std::string::npos) {
        mvwaddstrz(w, ++line_num, 1, c_white, tmp.substr(0, pos).c_str());
        tmp = tmp.substr(pos + 1);
    }
    mvwaddstrz(w, ++line_num, 1, c_white, tmp.c_str());
    wrefresh(w);
    return w;
}

void popup_top(const char* mes, ...)
{
 if (reject_not_whitelisted_printf(mes)) return;
 va_list ap;
 va_start(ap, mes);
 char buff[1024];
#ifdef _MSC_VER
 vsprintf_s<sizeof(buff)>(buff, mes, ap);
#else
 vsnprintf(buff, sizeof(buff), mes, ap);
#endif
 va_end(ap);

 const auto width_height = _popup_size(buff);

 WINDOW* w = _render_popup(buff, width_height, 0);

 int ch;
 do
  ch = getch();
 while(ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh();
}

void popup(const char* mes, ...)
{
 if (reject_not_whitelisted_printf(mes)) return;
 va_list ap;
 va_start(ap, mes);
 char buff[8192];
#ifdef _MSC_VER
 vsprintf_s<sizeof(buff)>(buff, mes, ap);
#else
 vsnprintf(buff, sizeof(buff), mes, ap);
#endif
 va_end(ap);

 const auto width_height = _popup_size(buff);

 WINDOW* w = _render_popup(buff, width_height, (VIEW - width_height.second) / 2);

 int ch;
 do
  ch = getch();
 while(ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh();
}

void popup_raw(const char* mes)
{
    const auto width_height = _popup_size(mes);

    WINDOW* w = _render_popup(mes, width_height, (VIEW - width_height.second) / 2);

    int ch;
    do
        ch = getch();
    while (ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
    werase(w);
    wrefresh(w);
    delwin(w);
    refresh();
}

void popup_nowait(const char* mes, ...)
{
 if (reject_not_whitelisted_printf(mes)) return;
 va_list ap;
 va_start(ap, mes);
 char buff[8192];
#ifdef _MSC_VER
 vsprintf_s<sizeof(buff)>(buff, mes, ap);
#else
 vsnprintf(buff, sizeof(buff), mes, ap);
#endif
 va_end(ap);

 const auto width_height = _popup_size(buff);

 WINDOW* w = _render_popup(buff, width_height, (VIEW - width_height.second) / 2);

 delwin(w);
 refresh();
}

void full_screen_popup(const char* mes, ...)
{
 if (reject_not_whitelisted_printf(mes)) return;
 va_list ap;
 va_start(ap, mes);
 char buff[8192];
#ifdef _MSC_VER
 vsprintf_s<sizeof(buff)>(buff, mes, ap);
#else
 vsnprintf(buff, sizeof(buff), mes, ap);
#endif
 va_end(ap);

 WINDOW* w = _render_popup(buff, std::pair(SCREEN_WIDTH, VIEW), 0);

 int ch;
 do
  ch = getch();
 while(ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh();
}

// this translates symbol y, u, n, b to NW, NE, SE, SW lines correspondingly
// h, j, c to horizontal, vertical, cross correspondingly
long special_symbol(char sym)
{
    switch (sym)
    {
    case 'j': return LINE_XOXO;
    case 'h': return LINE_OXOX;
    case 'c': return LINE_XXXX;
    case 'y': return LINE_OXXO;
    case 'u': return LINE_OOXX;
    case 'n': return LINE_XOOX;
    case 'b': return LINE_XXOO;
    default: return sym;
    }
}
