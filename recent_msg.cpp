#include "recent_msg.h"
#include "output.h"
#include "keypress.h"
#include "mapdata.h"

#include <stdarg.h>
#include <sstream>

recent_msg messages;

void recent_msg::add(const char* msg, ...)
{
	if (!msg || !*msg) return;	// reject NULL and empty-string

	char buff[1024];
	va_list ap;
	va_start(ap, msg);
	vsprintf(buff, msg, ap);
	va_end(ap);
	std::string s(buff);
	if (s.empty()) return;
	if (!messages.empty()) {
		auto& msg = messages.back();
		if (int(msg.turn) + 3 >= int(turn) && s == msg.message) {
			msg.count++;
			msg.turn = turn;
			return;
		}
	}

	while(256 <= messages.size()) messages.erase(messages.begin());
	messages.push_back(game_message(turn, s));
}

void recent_msg::buffer()
{
 WINDOW *w = newwin(VIEW, SCREEN_WIDTH, 0, 0);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w, VIEW - 1, 32, c_red, "Press q to return");

 int offset = 0;
 char ch;
 do {
  werase(w);
  wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
             LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
  mvwprintz(w, VIEW - 1, 32, c_red, "Press q to return");

  int line = 1;
  int lasttime = -1;
  int i;
  for (i = 1; i <= 20 && line <= 23 && offset + i <= messages.size(); i++) {
   game_message *mtmp = &(messages[ messages.size() - (offset + i) ]);
   calendar timepassed = turn - mtmp->turn;
   
   int tp = int(timepassed);
   nc_color col = (tp <=  2 ? c_ltred : (tp <=  7 ? c_white :
                   (tp <= 12 ? c_ltgray : c_dkgray)));

   if (int(timepassed) > lasttime) {
    mvwprintz(w, line, 3, c_ltblue, "%s ago:",
              timepassed.textify_period().c_str());
    line++;
    lasttime = int(timepassed);
   }

   if (line <= 23) { // Print the actual message... we may have to split it
    std::string mes = mtmp->message;
    if (mtmp->count > 1) {
     std::stringstream mesSS;
     mesSS << mes << " x " << mtmp->count;
     mes = mesSS.str();
    }
// Split the message into many if we must!
    size_t split;
    while (mes.length() > 78 && line <= 23) {
     split = mes.find_last_of(' ', 78);
     if (split > 78)
      split = 78;
     mvwprintz(w, line, 1, c_ltgray, mes.substr(0, split).c_str());
     line++;
     mes = mes.substr(split);
    }
    if (line <= 23) {
     mvwprintz(w, line, 1, col, mes.c_str());
     line++;
    }
   } // if (line <= 23)
  } //for (i = 1; i <= 10 && line <= 23 && offset + i <= messages.size(); i++)
  if (offset > 0)
   mvwprintz(w, 24, 27, c_magenta, "^^^");
  if (offset + i < messages.size())
   mvwprintz(w, 24, 51, c_magenta, "vvv");
  wrefresh(w);

  ch = input();
  point dir(get_direction(ch));
  if (dir.y == -1 && offset > 0) offset--;
  if (dir.y == 1 && offset < messages.size()) offset++;

 } while (ch != 'q' && ch != 'Q' && ch != ' ');

 werase(w);
 delwin(w);
}

void recent_msg::write(WINDOW* w_messages)
{
 werase(w_messages);
 int maxlength = SCREEN_WIDTH - (SEEX * 2 + 10);	// Matches size of w_messages
 int line = 8;
 for (int i = messages.size() - 1; i >= 0 && line < 9; i--) {
  std::string mes = messages[i].message;
  if (messages[i].count > 1) {
   std::stringstream mesSS;
   mesSS << mes << " x " << messages[i].count;
   mes = mesSS.str();
  }
// Split the message into many if we must!
  size_t split;
  while (mes.length() > maxlength && line >= 0) {
   split = mes.find_last_of(' ', maxlength);
   if (split > maxlength)
    split = maxlength;
   nc_color col = c_dkgray;
   if (int(messages[i].turn) >= curmes)
    col = c_ltred;
   else if (int(messages[i].turn) + 5 >= curmes)
    col = c_ltgray;
   //mvwprintz(w_messages, line, 0, col, mes.substr(0, split).c_str());
   mvwprintz(w_messages, line, 0, col, mes.substr(split + 1).c_str());
   mes = mes.substr(0, split);
   line--;
   //mes = mes.substr(split + 1);
  }
  if (line >= 0) {
   nc_color col = c_dkgray;
   if (int(messages[i].turn) >= curmes)
    col = c_ltred;
   else if (int(messages[i].turn) + 5 >= curmes)
    col = c_ltgray;
   mvwprintz(w_messages, line, 0, col, mes.c_str());
   line--;
  }
 }
 curmes = int(turn);
 wrefresh(w_messages);
}
