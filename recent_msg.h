#ifndef RECENT_MSG_H
#define RECENT_MSG_H

#include "calendar.h"
#ifndef SOCRATES_DAIMON
#include "wrap_curses.h"
#endif
#include <vector>

class recent_msg
{
private:
	struct game_message
	{
		calendar turn;
		int count;
		std::string message;

		game_message(calendar T = 0) noexcept : turn(T), count(1) {}
		game_message(calendar T, std::string&& M) noexcept : turn(T), count(1), message(std::move(M)) {}
		game_message(const game_message& src) = default;
		game_message(game_message&& src) = default;
		game_message& operator=(const game_message& src) = default;
		game_message& operator=(game_message && src) = default;
		~game_message() = default;
	};

	std::vector<game_message> msgs;   // Messages to be printed
	int curmes;	  // The last-seen message.

public:
	calendar turn;	// this is not a reasonable location for global time, if indeed we should be using global time rather than per-overmap time

	recent_msg() : curmes(0) {}
	recent_msg(const recent_msg& src) = delete;
	recent_msg(recent_msg&& src) = delete;
	~recent_msg() = default;
	recent_msg& operator=(const recent_msg& src) = delete;
	recent_msg& operator=(recent_msg&& src) = delete;

	void clear() {
		msgs.clear();
		curmes = 0;
	}

#ifndef SOCRATES_DAIMON
	template<class...Args> void add(const char* msg, Args...params) {
		if (!msg || !*msg) return;	// reject null and empty-string
		if constexpr (0 == sizeof...(Args)) {
			_record(msg);
		} else {
			_add(msg, params...);
		}
	}
	void buffer() const;
	void write(WINDOW* w_messages);
private:
	void _add(const char* msg, ...);
	void _record(const char* msg);
#endif
};

extern recent_msg messages;	// unique due to console, not due to exactly one PC

#endif
