#ifndef SAVELOAD_H
/*
This is not a normal header.  It must be used only in cpp files and
 must be after all types it is defining save-load support for.

 It also checks the preprocessor guards of other headers to discern
 which functions it should be declaring.
*/

#include <iosfwd>

#ifdef _ENUMS_H_

std::istream& operator>>(std::istream& is, point& dest);
std::ostream& operator<<(std::ostream& os, const point& src);

std::istream& operator>>(std::istream& is, tripoint& dest);
std::ostream& operator<<(std::ostream& os, const tripoint& src);

#endif

#endif