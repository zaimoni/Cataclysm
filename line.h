#ifndef _LINE_H_
#define _LINE_H_

#include "enums.h"

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
	static const point N;
	static const point NE;
	static const point E;
	static const point SE;
	static const point S;
	static const point SW;
	static const point W;
	static const point NW;
};

// The "t" value decides WHICH Bresenham line is used.
std::vector<point> line_to(int x1, int y1, int x2, int y2, int t);
inline std::vector<point> line_to(const point& pt, int x2, int y2, int t) { return line_to(pt.x, pt.y, x2, y2, t); };
inline std::vector<point> line_to(const point& pt, const point& pt2, int t) { return line_to(pt.x, pt.y, pt2.x, pt2.y, t); };
inline std::vector<point> line_to(int x1, int y1, const point& pt2, int t) { return line_to(x1, y1, pt2.x, pt2.y, t); };

// sqrt(dX^2 + dY^2)
int trig_dist(int x1, int y1, int x2, int y2);
inline int trig_dist(const point& pt, int x2, int y2) { return trig_dist(pt.x, pt.y, x2, y2); };
inline int trig_dist(const point& pt, const point& pt2) { return trig_dist(pt.x, pt.y, pt2.x, pt2.y); };
inline int trig_dist(int x1, int y1, const point& pt2) { return trig_dist(x1, y1, pt2.x, pt2.y); };

// Roguelike distance; maximum of dX and dY
int rl_dist(int x1, int y1, int x2, int y2);
inline int rl_dist(const point& pt, int x2, int y2) { return rl_dist(pt.x, pt.y, x2, y2); };
inline int rl_dist(const point& pt, const point& pt2) { return rl_dist(pt.x, pt.y, pt2.x, pt2.y); };
inline int rl_dist(int x1, int y1, const point& pt2) { return rl_dist(x1, y1, pt2.x, pt2.y); };

double slope_of(std::vector<point> line);
std::vector<point> continue_line(std::vector<point> line, int distance);

direction direction_from(int x1, int y1, int x2, int y2);
inline direction direction_from(const point& pt, int x2, int y2) { return direction_from(pt.x, pt.y, x2, y2); };
inline direction direction_from(const point& pt, const point& pt2) { return direction_from(pt.x, pt.y, pt2.x, pt2.y); };
inline direction direction_from(int x1, int y1, const point& pt2) { return direction_from(x1, y1, pt2.x, pt2.y); };

// more direction APIs
const char* direction_name(direction dir);
point direction_vector(direction dir);
direction rotate_clockwise(direction dir, int steps);	// negative is counter-clockwise

#endif
