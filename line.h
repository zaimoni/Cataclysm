#ifndef _LINE_H_
#define _LINE_H_

#include "enums.h"
#include "Zaimoni.STL/augment.STL/typetraits"

#include <vector>
#include <cmath>
#include <optional>

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

template<direction d1, direction d2> constexpr bool any(direction src) { return d1 == src || d2 == src; }

class Direction
{
public:
	static const constexpr point vector[] = {
		point(0,-1),
		point(1,-1),
		point(1,0),
		point(1,1),
		point(0,1),
		point(-1,1),
		point(-1,0),
		point(-1,-1)
	};

	static constexpr const point N = vector[NORTH];
	static constexpr const point NE = vector[NORTHEAST];
	static constexpr const point E = vector[EAST];
	static constexpr const point SE = vector[SOUTHEAST];
	static constexpr const point S = vector[SOUTH];
	static constexpr const point SW = vector[SOUTHWEST];
	static constexpr const point W = vector[WEST];
	static constexpr const point NW = vector[NORTHWEST];
};

static_assert(Direction::N  == -Direction::S);
static_assert(Direction::NE == -Direction::SW);
static_assert(Direction::E  == -Direction::W);
static_assert(Direction::SE == -Direction::NW);

// The "t" value decides WHICH Bresenham line is used.
std::vector<point> line_to(int x1, int y1, int x2, int y2, int t);
inline std::vector<point> line_to(const point& pt, int x2, int y2, int t) { return line_to(pt.x, pt.y, x2, y2, t); };
inline std::vector<point> line_to(const point& pt, const point& pt2, int t) { return line_to(pt.x, pt.y, pt2.x, pt2.y, t); };
inline std::vector<point> line_to(int x1, int y1, const point& pt2, int t) { return line_to(x1, y1, pt2.x, pt2.y, t); };

inline std::vector<point> line_to(int x1, int y1, int x2, int y2, std::optional<int> t) { return line_to(x1, y1, x2, y2, t ? *t : 0); };
inline std::vector<point> line_to(const point& pt, const point& pt2, std::optional<int> t) { return line_to(pt.x, pt.y, pt2.x, pt2.y, t ? *t : 0); };

// sqrt(dX^2 + dY^2)
template<class T>
struct dist
{
	static double L2(typename zaimoni::const_param<T>::type x0, typename zaimoni::const_param<T>::type x1) {
		if constexpr (!std::is_same_v<T, typename zaimoni::types<T>::norm>) return dist<typename zaimoni::types<T>::norm>::L2(zaimoni::norm(x0), zaimoni::norm(x1));
		else {
			if (0 == x0) return zaimoni::norm(x1);
			else if (0 > x0) return L2(zaimoni::norm(x0), x1);
			if (0 == x1) return zaimoni::norm(x0);
			else if (0 > x1) return L2(x0, zaimoni::norm(x1));
			if constexpr (!std::is_same_v<T, double>) return dist<double>::L2(x0, x1);	// requires reduction to norm-representing type to be safe
			else {
				if (x0 == x1) return x0 * sqrt(2.0);
				else if (x0 < x1) return L2(x1, x0);	// reduction
				if (sqrt(std::numeric_limits<double>::max()) / 2.0 > x0) return sqrt(x0 * x0 + x1 * x1);	// general case does not clearly risk overflow
				const double ratio = x1 / x0;
				return x0 * (sqrt(1.0 + ratio * ratio));
			}
		}
	}

	static constexpr auto Linf(typename zaimoni::const_param<T>::type x0, typename zaimoni::const_param<T>::type x1) {
		if constexpr (!std::is_same_v<T, typename zaimoni::types<T>::norm>) return dist<typename zaimoni::types<T>::norm>::Linf(zaimoni::norm(x0), zaimoni::norm(x1));
		else {
			if (0 == x0) return zaimoni::norm(x1);
			else if (0 > x0) return Linf(zaimoni::norm(x0), x1);
			if (0 == x1) return zaimoni::norm(x0);
			else if (0 > x1) return Linf(x0, zaimoni::norm(x1));
			return x0 <= x1 ? x1 : x0;
		}
	}
};

template<class T> double L2_dist(T x0, T x1) { return dist<std::remove_cv_t<T>>::L2(x0, x1); }

template<class T> constexpr auto Linf_dist(T x0, T x1) { return dist<std::remove_cv_t<T>>::Linf(x0, x1); }
constexpr auto Linf_dist(const point& pt) { return Linf_dist(pt.x, pt.y); }

inline int trig_dist(int x1, int y1, int x2, int y2) { return L2_dist(x1 - x2, y1 - y2); }
inline int trig_dist(const point& pt, int x2, int y2) { return trig_dist(pt.x, pt.y, x2, y2); };
inline int trig_dist(const point& pt, const point& pt2) { return trig_dist(pt.x, pt.y, pt2.x, pt2.y); };
inline int trig_dist(int x1, int y1, const point& pt2) { return trig_dist(x1, y1, pt2.x, pt2.y); };

// Roguelike distance; maximum of dX and dY
inline int rl_dist(int x1, int y1, int x2, int y2) { return Linf_dist(x1 - x2, y1 - y2); }
inline int rl_dist(const point& pt, int x2, int y2) { return rl_dist(pt.x, pt.y, x2, y2); };
inline int rl_dist(const point& pt, const point& pt2) { return rl_dist(pt.x, pt.y, pt2.x, pt2.y); };
inline int rl_dist(int x1, int y1, const point& pt2) { return rl_dist(x1, y1, pt2.x, pt2.y); };
// possible micro-optimization: if we are just doing a range check, we sometimes only need to confirm one coordinate to fail

std::vector<point> continue_line(const std::vector<point>& line, int distance);

inline constexpr direction direction_from(int delta_x, int delta_y)
{	// 0,0 is degenerate; C:Whales returned EAST
	if (0 == delta_y) return 0 <= delta_x ? EAST : WEST;
	if (0 == delta_x) return 0 <= delta_y ? SOUTH : NORTH;
	if (0 > delta_x) {
		const int abs_delta_x = -delta_x;
		if (0 > delta_y) {
			const int abs_delta_y = -delta_y;
			if (abs_delta_x / 2 > abs_delta_y) return WEST;
			else if (abs_delta_y / 2 > abs_delta_x) return NORTH;
			else return NORTHWEST;
		} else {
			if (abs_delta_x / 2 > delta_y) return WEST;
			else if (delta_y / 2 > abs_delta_x) return SOUTH;
			else return SOUTHWEST;
		}
	} else {
		if (0 > delta_y) {
			const int abs_delta_y = -delta_y;
			if (delta_x / 2 > abs_delta_y) return EAST;
			else if (abs_delta_y / 2 > delta_x) return NORTH;
			else return NORTHEAST;
		} else {
			if (delta_x / 2 > delta_y) return EAST;
			else if (delta_y / 2 > delta_x) return SOUTH;
			else return SOUTHEAST;
		}
	}
}

static_assert(NORTH == direction_from(Direction::N.x, Direction::N.y));
static_assert(NORTHEAST == direction_from(Direction::NE.x, Direction::NE.y));
static_assert(EAST == direction_from(Direction::E.x, Direction::E.y));
static_assert(SOUTHEAST == direction_from(Direction::SE.x, Direction::SE.y));
static_assert(SOUTH == direction_from(Direction::S.x, Direction::S.y));
static_assert(SOUTHWEST == direction_from(Direction::SW.x, Direction::SW.y));
static_assert(WEST == direction_from(Direction::W.x, Direction::W.y));
static_assert(NORTHWEST == direction_from(Direction::NW.x, Direction::NW.y));

inline direction direction_from(int x1, int y1, int x2, int y2) { return direction_from(x2 - x1, y2 - y1); }
inline direction direction_from(const point& pt, const point& pt2) { return direction_from(pt.x, pt.y, pt2.x, pt2.y); }

// more direction APIs
const char* direction_name(direction dir);
point direction_vector(direction dir);
direction rotate_clockwise(direction dir, int steps);	// negative is counter-clockwise

#endif
