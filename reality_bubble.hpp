#ifndef REALITY_BUBBLE_HPP
#define REALITY_BUBBLE_HPP 1

#include "map.h"

// Part of disentangling the game class.
class reality_bubble
{
public:
	reality_bubble() = default;
	reality_bubble(const reality_bubble& src) = default;
	reality_bubble(reality_bubble&& src) = default;
	reality_bubble& operator=(const reality_bubble& src) = default;
	reality_bubble& operator=(reality_bubble&& src) = default;
protected:
	~reality_bubble() = default; // relax this to public when paying off public base of game class technical debt.

public:
	// intentionally incurred technical debt -- don't want to incur a Big Bang wire-in.
	// For now (2020-12-28), do an incorrect setup as a public base class.
	// * Ok to leave reality-bubble fields in the game class as technical debt
	// * Do not migrate anything to reality bubble class that would cause problems with overlap when parallelized
	// * player (and player UI) does *not* belong here; reality_bubble doesn't care what it's centered on.
	overmap cur_om;
	map m;
	tripoint lev;	// Placement inside the overmap; x/y should be in 0...OMAPX/Y; offset from the player's submap coordinates
	  // but game::update_map thinks legal values are 0..2*OMAPX/Y
	  // lev.z is almost always cur_om.pos.z (possibly should be explicitly enforced as map loading responds to cur_om.pos.z)
	  // in savegames: lev = u.GPSpos.first+(-5,-5,0).  Should be true anytime except during reality bubble shift during a move between submaps
};

#endif
