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

void mobile::set_screenpos(point pt) { GPSpos = overmap::toGPS(pt); }

void mobile::set_screenpos(const GPS_loc& loc)
{
	GPSpos = loc;
	_set_screenpos();
}

void mobile::knockback_from(const GPS_loc& loc)
{
	if (loc == GPSpos) return; // No effect

	const auto to(GPSpos + cmp(GPSpos, loc));
	const auto g = game::active();
	const bool u_see = (bool)g->u.see(loc);

	const std::string You = grammar::capitalize(desc(grammar::noun::role::subject, grammar::article::definite));
	const auto bounce(regular_verb_agreement("bounce"));

	// First see, if we hit a monster/player/NPC
	if (auto _mob = g->mob(to)) {
		add(effect::STUNNED, TURNS(1));
		const int effective_size = knockback_size();
		const int targ_size = knockback_size();
		const int size_delta = effective_size - targ_size;
		hurt(targ_size);
		if (u_see) messages.add("%s %s off a %s!", You.c_str(), bounce.c_str(), _mob->desc(grammar::noun::role::direct_object, grammar::article::indefinite).c_str());
		if (0 <= size_delta) {
			if (0 < size_delta) knockback_from(to); // Chain reaction!
			hurt(effective_size);
			add(effect::STUNNED, TURNS(1));
		}
		return;
	}

	// If we're still in the function at this point, we're actually moving a tile!
	if (0 == to.move_cost()) { // Wait, it's a wall (or water)
		if (!handle_knockback_into_impassable(to, You)) {
			// It's some kind of wall.
			hurt(knockback_size());
			add(effect::STUNNED, TURNS(2));
			if (u_see) messages.add("%s %s off a %s.", You.c_str(), bounce.c_str(), name_of(to.ter()).c_str());
		}
	} else set_screenpos(to);	// It's no wall
}
