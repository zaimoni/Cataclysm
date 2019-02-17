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

// The "t" value decides WHICH Bresenham line is used.
std::vector <point> line_to(int x1, int y1, int x2, int y2, int t);
// sqrt(dX^2 + dY^2)
int trig_dist(int x1, int y1, int x2, int y2);
// Roguelike distance; minimum of dX and dY
int rl_dist(int x1, int y1, int x2, int y2);
int rl_dist(point a, point b);
double slope_of(std::vector<point> line);
std::vector<point> continue_line(std::vector<point> line, int distance);
direction direction_from(int x1, int y1, int x2, int y2);
const char* direction_name(direction dir);


#endif
