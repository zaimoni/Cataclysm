#ifndef RECENT_MSG_H
#define RECENT_MSG_H

#include "calendar.h"
#include "wrap_curses.h"
#include <vector>

class recent_msg
{
private:
	std::vector <game_message> messages;   // Messages to be printed
	int curmes;	  // The last-seen message.
public:
	calendar turn;	// this is not a reasonable location for global time, if indeed we should be using global time rather than per-overmap time

	recent_msg() : curmes(0) {};
	recent_msg(const recent_msg& src) = delete;
	recent_msg(recent_msg&& src) = delete;
	~recent_msg() = default;
	recent_msg& operator=(const recent_msg& src) = delete;
	recent_msg& operator=(recent_msg&& src) = delete;

	void clear() {
		messages.clear();
		curmes = 0;
	}

	void add(const char* msg, ...);
#ifndef SOCRATES_DAIMON
	void buffer();
#endif
	void write(WINDOW* w_messages);
};

extern recent_msg messages;	// unique due to console, not due to exactly one PC

#endif
