#ifndef SAVELOAD_H
/*
This is not a normal header.  It must be used only in cpp files and
 must be after all types it is defining save-load support for.

 It also checks the preprocessor guards of other headers to discern
 which functions it should be declaring.

 A full implementation would be: istream constructor, operator >>, operator <<
*/

#include <iosfwd>

#ifdef _BIONICS_H_

std::ostream& operator<<(std::ostream& os, const bionic& src);

#endif

#ifdef _ENUMS_H_

std::istream& operator>>(std::istream& is, point& dest);
std::ostream& operator<<(std::ostream& os, const point& src);

std::istream& operator>>(std::istream& is, tripoint& dest);
std::ostream& operator<<(std::ostream& os, const tripoint& src);

#endif

#ifdef _MAPDATA_H_

std::ostream& operator<<(std::ostream& os, const spawn_point& src);

std::istream& operator>>(std::istream& is, field& dest);
std::ostream& operator<<(std::ostream& os, const field& src);

std::ostream& operator<<(std::ostream& os, const submap& src);

#endif

#ifdef _MISSION_H_

std::ostream& operator<<(std::ostream& os, const mission& src);

#endif

#ifdef _MONGROUP_H_

std::ostream& operator<<(std::ostream& os, const mongroup& src);

#endif

#ifdef _OVERMAP_H_

std::ostream& operator<<(std::ostream& os, const city& src);
std::ostream& operator<<(std::ostream& os, const om_note& src);
std::ostream& operator<<(std::ostream& os, const radio_tower& src);

#endif

#ifdef _PLDATA_H_

std::istream& operator>>(std::istream& is, player_activity& dest);
std::ostream& operator<<(std::ostream& os, const player_activity& src);

#endif

#ifdef _VEHICLE_H_

std::istream& operator>>(std::istream& is, vehicle_part& dest);
std::ostream& operator<<(std::ostream& os, const vehicle_part& src);

#endif

#endif