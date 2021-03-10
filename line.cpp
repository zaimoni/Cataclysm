#include "line.h"
#include "map.h"
#include "Zaimoni.STL/Logging.h"
#include <math.h>

#define SLOPE_VERTICAL 999999

std::vector <point> line_to(int x1, int y1, int x2, int y2, int t)
{
 std::vector<point> ret;
 int dx = x2 - x1;
 int dy = y2 - y1;
 int ax = abs(dx)<<1;
 int ay = abs(dy)<<1;
 int sx = cataclysm::signum(dx);
 int sy = cataclysm::signum(dy);

 point cur(x1, y1);

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

static double slope_of(const std::vector<point>& line)
{
 const auto delta = line.back() - line.front();
 if (0 == delta.x) return SLOPE_VERTICAL;
 return double(delta.y) / delta.x;
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
  if (SLOPE_VERTICAL > slope)
   end.x += int(distance / abs(slope)) * sX;
 }
 return line_to(start, end, 0);
}

const char* direction_name(direction dir)
{
	static constexpr const char* const translate[] = {
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
	DEBUG_FAIL_OR_LEAVE(0 > dir || sizeof(Direction::vector) / sizeof(*Direction::vector) <= dir, return point((unsigned int)(-1) / 2, (unsigned int)(-1) / 2));
	return Direction::vector[dir];
}

direction rotate_clockwise(direction dir, int steps)
{
	steps = steps % 8;	// magic constant
	if (0 > steps) steps += 8;
	if (0 == steps) return dir;
	return direction((dir + steps) % 8);
}
