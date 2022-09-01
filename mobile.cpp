#include "mobile.h"

#include "game.h"
#include "vehicle.h"
#include "rng.h"

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
		if (!handle_knockback_into_impassable(to)) {
			// It's some kind of wall.
			hurt(knockback_size());
			add(effect::STUNNED, TURNS(2));
			if (u_see) messages.add("%s %s off a %s.", You.c_str(), bounce.c_str(), name_of(to.ter()).c_str());
		}
	} else set_screenpos(to);	// It's no wall
}

bool mobile::flung(int& flvel, GPS_loc& loc)
{
	const auto g = game::active();
	const int min_dam = flvel / 3;
	bool thru = true;


	if (const auto m_at = g->mob_at(loc)) {
		const auto _mob = std::visit(mobile::cast(), *m_at);
		int dam2 = min_dam + rng(0, min_dam);
		if (_mob->hitall(dam2, 40)) g->kill_mon(*m_at);
		else thru = false;
		int dam1 = min_dam + rng(0, min_dam);
		hitall(dam1, 40);

		static auto slammed = [&]() {
			std::string dname = _mob->desc(grammar::noun::role::direct_object, grammar::article::definite);
			std::string suffix = std::string(" for ") + std::to_string(dam1) + " damage!";
			std::string sname = grammar::capitalize(subject()) + " " + to_be();
			return sname + " slammed against " + dname + suffix;
		};

		g->if_visible_message(slammed, GPSpos);
	} else if (0 == loc.move_cost() && !is<swimmable>(loc.ter())) {
		std::string snd;
		if (loc.is_bashable()) thru = loc.bash(flvel, snd);
		else thru = false;
		if (!snd.empty()) messages.add("You hear a %s", snd.c_str());
		int dam1 = min_dam + rng(0, min_dam);
		hitall(dam1, 40);

		static auto slammed = [&]() {
			const auto veh = loc.veh_at();
			std::string dname = veh ? veh->first->part_info(veh->second).name : name_of(loc.ter()).c_str();
			std::string suffix = std::string(" for ") + std::to_string(dam1) + " damage!";
			std::string sname = grammar::capitalize(subject()) + " " + to_be();
			return sname + " slammed against the " + dname + suffix;
		};

		g->if_visible_message(slammed, GPSpos);
		flvel /= 2;
	}

	return thru;
}
