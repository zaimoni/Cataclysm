#include "recent_msg.h"
#ifndef SOCRATES_DAIMON
#include "output.h"
#include "keypress.h"
#include "mapdata.h"
#include "ui.h"
#include "options.h"
#include "zero.h"
#include <stdarg.h>
#endif

recent_msg messages;

#ifndef SOCRATES_DAIMON
void recent_msg::add(const char* msg, ...)
{
	if (!msg || !*msg) return;	// reject null and empty-string
    if (reject_not_whitelisted_printf(msg)) return;

	char buff[1024];
	va_list ap;
	va_start(ap, msg);
#ifdef _MSC_VER
	vsprintf_s<sizeof(buff)>(buff, msg, ap);
#else
	vsnprintf(buff, sizeof(buff), msg, ap);
#endif
	va_end(ap);
	std::string s(buff);
	if (s.empty()) return;
	if (!msgs.empty()) {
		auto& msg = msgs.back();
		if (int(msg.turn) + 3 >= int(turn) && s == msg.message) {
			msg.count++;
			msg.turn = turn;
			return;
		}
	}

	while(256 <= msgs.size()) msgs.erase(msgs.begin());
	msgs.push_back(game_message(turn, s));
}

void recent_msg::buffer() const
{
 WINDOW *w = newwin(VIEW, SCREEN_WIDTH, 0, 0);

 int offset = 0;
 int ch;
 do {
  werase(w);
  wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
             LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
  mvwprintz(w, VIEW - 1, SCREEN_WIDTH/2 - (sizeof("Press q to return")-1)/2, c_red, "Press q to return");

  int line = 1;
  int lasttime = -1;
  int i;
  for (i = 1; i <= VIEW - 5 && line <= VIEW - 2 && offset + i <= msgs.size(); i++) {
   const game_message& mtmp = msgs[msgs.size() - (offset + i)];
   calendar timepassed = turn - mtmp.turn;

   int tp = int(timepassed);
   nc_color col = (tp <=  2 ? c_ltred : (tp <=  7 ? c_white :
                   (tp <= 12 ? c_ltgray : c_dkgray)));

   if (int(timepassed) > lasttime) {
    mvwprintz(w, line, 3, c_ltblue, "%s ago:",
              timepassed.textify_period().c_str());
    line++;
    lasttime = int(timepassed);
   }

   if (line <= VIEW - 2) { // Print the actual message... we may have to split it
    std::string mes = mtmp.message;
    if (1 < mtmp.count) {
     mes += " x ";
     mes += std::to_string(mtmp.count);
    }
// Split the message into many if we must!
    while (mes.length() > SCREEN_WIDTH-2 && line <= VIEW - 2) {
     const size_t split = clamped_ub(mes.find_last_of(' ', SCREEN_WIDTH - 2), SCREEN_WIDTH - 2);
     mvwprintz(w, line, 1, c_ltgray, mes.substr(0, split).c_str());
     line++;
     mes = mes.substr(split);
    }
    if (line <= VIEW - 2) {
     mvwprintz(w, line, 1, col, mes.c_str());
     line++;
    }
   } // if (line <= 23)
  } //for (i = 1; i <= 10 && line <= 23 && offset + i <= msgs.size(); i++)
  // Arguable whether the arrows should "spread out" as the screen gets wider. 2020-10-10 zaimoni
  if (offset > 0) mvwprintz(w, VIEW - 1, SCREEN_WIDTH / 2 - 13, c_magenta, "^^^");
  if (offset + i < msgs.size()) mvwprintz(w, VIEW - 1, SCREEN_WIDTH / 2 + 10, c_magenta, "vvv");
  wrefresh(w);

  ch = input();
  point dir(get_direction(ch));
  // \todo? some sort of fast-stepping at large message counts
  if        (-1 == dir.y) {
      if (0 < offset) offset--;
  } else if (1 == dir.y) {
      if (msgs.size() > offset) offset++;
  }
 } while (ch != 'q' && ch != 'Q' && ch != ' ');

 werase(w);
 delwin(w);
}

void recent_msg::write(WINDOW* w_messages)
{
 werase(w_messages);
 int maxlength = getmaxx(w_messages) - 2;
 int line = getmaxy(w_messages) - 1;
 for (int i = msgs.size() - 1; 0 <= i && 0 <= line; i--) {
  std::string mes = msgs[i].message;
  if (1 < msgs[i].count) {
   mes += " x ";
   mes += std::to_string(msgs[i].count);
  }
// Split the message into many if we must!
  size_t split;
  while (mes.length() > maxlength && line >= 0) {
   split = mes.find_last_of(' ', maxlength);
   if (split > maxlength)
    split = maxlength;
   nc_color col = c_dkgray;
   if (int(msgs[i].turn) >= curmes)
    col = c_ltred;
   else if (int(msgs[i].turn) + 5 >= curmes)
    col = c_ltgray;
   //mvwprintz(w_messages, line, 0, col, mes.substr(0, split).c_str());
   mvwprintz(w_messages, line, 0, col, mes.substr(split + 1).c_str());
   mes = mes.substr(0, split);
   line--;
   //mes = mes.substr(split + 1);
  }
  if (line >= 0) {
   nc_color col = c_dkgray;
   if (int(msgs[i].turn) >= curmes)
    col = c_ltred;
   else if (int(msgs[i].turn) + 5 >= curmes)
    col = c_ltgray;
   mvwprintz(w_messages, line, 0, col, mes.c_str());
   line--;
  }
 }
 curmes = int(turn);
 wrefresh(w_messages);
}
#endif
