// master implementation file for new-style saveload support

#include "enums.h"
#include "saveload.h"
#include <istream>
#include <ostream>

// legacy implementations assume text mode streams
// this only makes a difference for ostream

#ifdef BINARY_STREAMS
#define I_SEP
#else
#define I_SEP << " "
#endif

std::istream& operator>>(std::istream& is, point& dest)
{
	return is >> dest.x >> dest.y;
}

std::ostream& operator<<(std::ostream& os, const point& src)
{
	return os << src.x I_SEP << src.y;
}

std::istream& operator>>(std::istream& is, tripoint& dest)
{
	return is >> dest.x >> dest.y >> dest.z;
}

std::ostream& operator<<(std::ostream& os, const tripoint& src)
{
	return os << src.x I_SEP << src.y  I_SEP << src.z;
}
