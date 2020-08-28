#include "line.h"
#include "map.h"
#include <math.h>
#include "Zaimoni.STL/Logging.h"

#define SGN(a) (((a)<0) ? -1 : 1)
#define SLOPE_VERTICAL 999999

std::vector <point> line_to(int x1, int y1, int x2, int y2, int t)
{
 std::vector<point> ret;
 int dx = x2 - x1;
 int dy = y2 - y1;
 int ax = abs(dx)<<1;
 int ay = abs(dy)<<1;
 int sx = SGN(dx);
 int sy = SGN(dy);
 if (dy == 0) sy = 0;
 if (dx == 0) sx = 0;
 point cur;
 cur.x = x1;
 cur.y = y1;

 int xmin = (x1 < x2 ? x1 : x2), ymin = (y1 < y2 ? y1 : y2),
     xmax = (x1 > x2 ? x1 : x2), ymax = (y1 > y2 ? y1 : y2);

 xmin -= abs(dx);
 ymin -= abs(dy);
 xmax += abs(dx);
 ymax += abs(dy);

 if (ax == ay) {
  do {
   cur.y += sy;
   cur.x += sx;
   ret.push_back(cur);
  } while ((cur.x != x2 || cur.y != y2) &&
           (cur.x >= xmin && cur.x <= xmax && cur.y >= ymin && cur.y <= ymax));
 } else if (ax > ay) {
  do {
   if (t > 0) {
    cur.y += sy;
    t -= ax;
   }
   cur.x += sx;
   t += ay;
   ret.push_back(cur);
  } while ((cur.x != x2 || cur.y != y2) &&
           (cur.x >= xmin && cur.x <= xmax && cur.y >= ymin && cur.y <= ymax));
 } else {
  do {
   if (t > 0) {
    cur.x += sx;
    t -= ay;
   }
   cur.y += sy;
   t += ax;
   ret.push_back(cur);
  } while ((cur.x != x2 || cur.y != y2) &&
           (cur.x >= xmin && cur.x <= xmax && cur.y >= ymin && cur.y <= ymax));
 }
 return ret;
}

double slope_of(const std::vector<point>& line)
{
 const auto delta = line.back() - line.front();
 if (0 == delta.x) return SLOPE_VERTICAL;
 return (delta.y / delta.x);
}

std::vector<point> continue_line(const std::vector<point>& line, int distance)
{
 point start = line.back(), end = line.back();
 double slope = slope_of(line);
 int sX = (line.front().x < line.back().x ? 1 : -1),
     sY = (line.front().y < line.back().y ? 1 : -1);
 if (abs(slope) == 1) {
  end.x += distance * sX;
  end.y += distance * sY;
 } else if (abs(slope) < 1) {
  end.x += distance * sX;
  end.y += int(distance * abs(slope) * sY);
 } else {
  end.y += distance * sY;
  if (slope != SLOPE_VERTICAL)
   end.x += int(distance / abs(slope)) * sX;
 }
 return line_to(start, end, 0);
}

direction direction_from(int x1, int y1, int x2, int y2)
{
 int dx = x2 - x1;
 int dy = y2 - y1;
 if (dx < 0) {
  if (abs(dx) / 2 > abs(dy) || dy == 0) return WEST;
  else if (abs(dy) / 2 > abs(dx)) {
   return (dy < 0) ? NORTH : SOUTH;
  } else {
   return (dy < 0) ? NORTHWEST : SOUTHWEST;
  }
 } else {
  if (dx / 2 > abs(dy) || dy == 0) return EAST;
  else if (abs(dy) / 2 > dx || dx == 0) {
   return (dy < 0) ? NORTH : SOUTH;
  } else {
   return (dy < 0) ? NORTHEAST : SOUTHEAST;
  }
 }
}

const char* direction_name(direction dir)
{
	static const char* translate[] = {
		"north",
		"northeast",
		"east",
		"southeast",
		"south",
		"southwest",
		"west",
		"northwest"
	};

	DEBUG_FAIL_OR_LEAVE(0 > dir || sizeof(translate) / sizeof(*translate) <= dir, return "WEIRD DIRECTION_NAME() BUG");
	return translate[dir];
}

point direction_vector(direction dir)
{
	static const constexpr point _dir_vectors[] = {
		point(0,-1),
		point(1,-1),
		point(1,0),
		point(1,1),
		point(0,1),
		point(-1,1),
		point(-1,0),
		point(-1,-1)
	};

	static_assert(_dir_vectors[NORTH] == -_dir_vectors[SOUTH]);
	static_assert(_dir_vectors[NORTHEAST] == -_dir_vectors[SOUTHWEST]);
	static_assert(_dir_vectors[EAST] == -_dir_vectors[WEST]);
	static_assert(_dir_vectors[SOUTHEAST] == -_dir_vectors[NORTHWEST]);

	DEBUG_FAIL_OR_LEAVE(0 > dir || sizeof(_dir_vectors) / sizeof(*_dir_vectors) <= dir, return point((unsigned int)(-1) / 2, (unsigned int)(-1) / 2));
	return _dir_vectors[dir];
}

direction rotate_clockwise(direction dir, int steps)
{
	steps = steps % 8;	// magic constant
	if (0 > steps) steps += 8;
	if (0 == steps) return dir;
	return direction((dir + steps) % 8);
}
