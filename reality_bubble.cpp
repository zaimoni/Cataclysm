#include "reality_bubble.hpp"

// cf map::loadn
GPS_loc reality_bubble::toGPS(point screen_pos) const	// \todo overflow checking
{
	auto anchor(cur_om.pos);
	anchor.x *= 2 * OMAP;	// don't scale z coordinate
	anchor.y *= 2 * OMAP;
	anchor.x += lev.x;
	anchor.y += lev.y;
	//  historically cur_om.pos.z == lev.z
	if (0 > screen_pos.x) {
		anchor.x += screen_pos.x / SEE;
		screen_pos.x %= SEE;
		anchor.x--;
		screen_pos.x += SEE;
	}
	else if (SEE <= screen_pos.x) {
		anchor.x += screen_pos.x / SEE;
		screen_pos.x %= SEE;
	}
	if (0 > screen_pos.y) {
		anchor.y += screen_pos.y / SEE;
		screen_pos.y %= SEE;
		anchor.y--;
		screen_pos.y += SEE;
	}
	else if (SEE <= screen_pos.y) {
		anchor.y += screen_pos.y / SEE;
		screen_pos.y %= SEE;
	}
	return GPS_loc(anchor, screen_pos);
}

std::optional<point> reality_bubble::toScreen(GPS_loc GPS_pos) const
{
	if (GPS_pos.first.z != cur_om.pos.z) return std::nullopt;	// \todo? z-level change target
	const auto anchor(toGPS(point(0, 0)));	// \todo would be nice to short-circuit this stage, but may be moot after modeling Earth's radius.  Also, cache target
	if (GPS_pos.first.x < anchor.first.x || GPS_pos.first.y < anchor.first.y) return std::nullopt;
	point delta;
	delta.x = GPS_pos.first.x - anchor.first.x;
	if (MAPSIZE <= delta.x) return std::nullopt;
	delta.y = GPS_pos.first.y - anchor.first.y;
	if (MAPSIZE <= delta.y) return std::nullopt;
	return GPS_pos.second + SEE * delta;
}

bool reality_bubble::toScreen(const GPS_loc& GPS_pos, point& screen_pos) const
{
	if (const auto pos = toScreen(GPS_pos)) {
		screen_pos = *pos;
		return true;
	}
	return false;
}

std::optional<reality_bubble_loc> reality_bubble::toSubmap(GPS_loc GPS_pos) const
{
	if (GPS_pos.first.z != cur_om.pos.z) return std::nullopt;	// \todo? z-level change target
	const auto anchor(toGPS(point(0, 0)));	// \todo would be nice to short-circuit this stage, but may be moot after modeling Earth's radius.  Also, cache target
	if (GPS_pos.first.x < anchor.first.x || GPS_pos.first.y < anchor.first.y) return std::nullopt;
	point delta;
	delta.x = GPS_pos.first.x - anchor.first.x;
	if (MAPSIZE <= delta.x) return std::nullopt;
	delta.y = GPS_pos.first.y - anchor.first.y;
	if (MAPSIZE <= delta.y) return std::nullopt;
	return reality_bubble_loc(delta.x + delta.y * MAPSIZE, GPS_pos.second);
}

OM_loc reality_bubble::om_location()
{
	return OM_loc(cur_om.pos, (project_xy(lev) + point(MAPSIZE / 2))/2);
}
