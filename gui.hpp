#ifndef GUI_HPP
#define GUI_HPP

#include "enums.h"
#include "wrap_curses.h"

class monster;
class npc;
class pc;

// std::visit wrapper
struct draw_mob
{
	WINDOW* const w;
	const point origin;
	const bool invert;

	draw_mob(WINDOW* w, const point& origin, bool invert) noexcept : w(w), origin(origin), invert(invert) {}
	draw_mob(const draw_mob& src) = delete;
	draw_mob(draw_mob&& src) = delete;
	draw_mob& operator=(const draw_mob& src) = delete;
	draw_mob& operator=(draw_mob&& src) = delete;
	~draw_mob() = default;

	void operator()(const monster* whom);
	void operator()(const npc* whom);
	void operator()(const pc* whom);
};

#endif
