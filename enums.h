#ifndef _ENUMS_H_
#define _ENUMS_H_

enum material {
MNULL = 0,
//Food Materials
LIQUID, VEGGY, FLESH, POWDER,
//Clothing
COTTON, WOOL, LEATHER, KEVLAR,
//Other
STONE, PAPER, WOOD, PLASTIC, GLASS, IRON, STEEL, SILVER
};

struct point {
 int x;
 int y;
 point(int X = 0, int Y = 0) : x (X), y (Y) {}
 point(const point &p) = default;
};

inline bool operator==(const point& lhs, const point& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
inline bool operator!=(const point& lhs, const point& rhs) { return lhs.x != rhs.x || lhs.y != rhs.y; }

struct tripoint {
 int x;
 int y;
 int z;
 tripoint(int X = 0, int Y = 0, int Z = 0) : x (X), y (Y), z (Z) {}
 tripoint(const tripoint &p) = default;
};

inline bool operator<(const tripoint& lhs, const tripoint& rhs)
{
	if (lhs.x < rhs.x) return true;
	if (lhs.x > rhs.x) return false;
	if (lhs.y < rhs.y) return true;
	if (lhs.y > rhs.y) return false;
	return lhs.z < rhs.z;
}

namespace cataclysm {

// exceptionally un-threadsafe; intent is to provide a standard "overflow" for reference returns
template<class T>
struct discard
{
	static T x;
};

template<class T>
struct JSON_parse
{
	T operator()(const char* src);	// fails unless specialized at link-time
};

}

#endif
