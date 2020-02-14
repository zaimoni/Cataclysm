#ifndef _LINE_H_
#define _LINE_H_

#include "enums.h"
#include "Zaimoni.STL/augment.STL/typetraits"

#include <vector>

// this is the canonical XCOM-like enumeration
enum direction {
NORTH = 0,
NORTHEAST,
EAST,
SOUTHEAST,
SOUTH,
SOUTHWEST,
WEST,
NORTHWEST
};

class Direction
{
public:
	static constexpr const point N = point(-1, 0);
	static constexpr const point NE = point(-1, 1);
	static constexpr const point E = point(0, 1);
	static constexpr const point SE = point(1, 1);
	static constexpr const point S = point(1, 0);
	static constexpr const point SW = point(1, -1);
	static constexpr const point W = point(0, -1);
	static constexpr const point NW = point(-1, -1);
};

// The "t" value decides WHICH Bresenham line is used.
std::vector<point> line_to(int x1, int y1, int x2, int y2, int t);
inline std::vector<point> line_to(const point& pt, int x2, int y2, int t) { return line_to(pt.x, pt.y, x2, y2, t); };
inline std::vector<point> line_to(const point& pt, const point& pt2, int t) { return line_to(pt.x, pt.y, pt2.x, pt2.y, t); };
inline std::vector<point> line_to(int x1, int y1, const point& pt2, int t) { return line_to(x1, y1, pt2.x, pt2.y, t); };

// sqrt(dX^2 + dY^2)
template<class T>
struct dist
{
	static double L2(typename zaimoni::const_param<T>::type x0, typename zaimoni::const_param<T>::type x1) {
		if constexpr (!std::is_same_v<T, zaimoni::types<T>::norm>) return dist<zaimoni::types<T>::norm>::L2(zaimoni::norm(x0), zaimoni::norm(x1));
		if (0 == x0) return zaimoni::norm(x1);
		else if (0 > x0) return L2(zaimoni::norm(x0), x1);
		if (0 == x1) return zaimoni::norm(x0);
		else if (0 > x1) return L2(x0, zaimoni::norm(x1));
		if constexpr (!std::is_same_v<T, double>) return dist<double>::L2(x0, x1);	// requires reduction to norm-representing type to be safe
		if (x0 == x1) return x0 * sqrt(2.0);
		else if (x0 < x1) return L2(x1, x0);	// reduction
		if (sqrt(std::numeric_limits<double>::max())/2.0 > x0) return sqrt(x0 * x0 + x1 * x1);	// general case does not clearly risk overflow
		const double ratio = x1 / x0;
		return x0 * (sqrt(1.0 + ratio * ratio));
	}
};

template<class T>
double L2_dist(T x0, T x1) { return dist<std::remove_cv_t<T>>::L2(x0, x1); }

inline int trig_dist(int x1, int y1, int x2, int y2) { return L2_dist(x1 - x2, y1 - y2); }
inline int trig_dist(const point& pt, int x2, int y2) { return trig_dist(pt.x, pt.y, x2, y2); };
inline int trig_dist(const point& pt, const point& pt2) { return trig_dist(pt.x, pt.y, pt2.x, pt2.y); };
inline int trig_dist(int x1, int y1, const point& pt2) { return trig_dist(x1, y1, pt2.x, pt2.y); };

// Roguelike distance; maximum of dX and dY
int rl_dist(int x1, int y1, int x2, int y2);
inline int rl_dist(const point& pt, int x2, int y2) { return rl_dist(pt.x, pt.y, x2, y2); };
inline int rl_dist(const point& pt, const point& pt2) { return rl_dist(pt.x, pt.y, pt2.x, pt2.y); };
inline int rl_dist(int x1, int y1, const point& pt2) { return rl_dist(x1, y1, pt2.x, pt2.y); };
// possible micro-optimization: if we are just doing a range check, we sometimes only need to confirm one coordinate to fail

double slope_of(const std::vector<point>& line);
std::vector<point> continue_line(const std::vector<point>& line, int distance);

direction direction_from(int x1, int y1, int x2, int y2);
inline direction direction_from(const point& pt, int x2, int y2) { return direction_from(pt.x, pt.y, x2, y2); };
inline direction direction_from(const point& pt, const point& pt2) { return direction_from(pt.x, pt.y, pt2.x, pt2.y); };
inline direction direction_from(int x1, int y1, const point& pt2) { return direction_from(x1, y1, pt2.x, pt2.y); };

// more direction APIs
const char* direction_name(direction dir);
point direction_vector(direction dir);
direction rotate_clockwise(direction dir, int steps);	// negative is counter-clockwise

#endif
