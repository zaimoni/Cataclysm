#include "mobile.h"

#include "game.h" // \todo Just need the reality bubble data structure

#include <stdexcept>

std::optional<point> mobile::screen_pos() const { return game::active()->toScreen(GPSpos); }

point mobile::screenPos() const
{
	const auto ret = game::active()->toScreen(GPSpos);
	if (!ret) throw std::logic_error("not in reality bubble");
	return *ret;
}

std::optional<reality_bubble_loc> mobile::bubble_pos() const { return game::active()->toSubmap(GPSpos); }

reality_bubble_loc mobile::bubblePos() const
{
	const auto ret = game::active()->toSubmap(GPSpos);
	if (!ret) throw std::logic_error("not in reality bubble");
	return *ret;
}

