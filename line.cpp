#include "line.h"
#include "map.h"
#include "Zaimoni.STL/Logging.h"
#include <math.h>

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

static std::vector<point> line_from(const point& origin, const point& delta, int distance)
{
	std::vector<point> ret;
//	if (0 >= distance) return ret;	// should not happen so don't test for it
	const point abs_delta(abs(delta.x), abs(delta.y));
	point counter(0);
	if (abs_delta == counter) return ret;
	const int ub = 2*Linf_dist(abs_delta);

	point thresholds(2 * delta);	// \todo fix, or reject, signed overflow here
	point primary(0);

	// \todo re-architect point and tripoint?
	if (ub == thresholds.x) {
		primary.x = 1;
		thresholds.x = 0;
	} else if (ub == -thresholds.x) {
		primary.x = -1;
		thresholds.x = 0;
	}

	if (ub == thresholds.y) {
		primary.y = 1;
		thresholds.y = 0;
	} else if (ub == -thresholds.y) {
		primary.y = -1;
		thresholds.y = 0;
	}

	// \todo handle chess knight-move issue
	point scan(origin);
	while (0 <= --distance) {
		scan += primary;
		counter += thresholds;
		if (ub <= counter.x) {
			counter.x -= ub;
			scan.x++;
		} else if (ub <= -counter.x) {
			counter.x += ub;
			scan.x--;
		}

		if (ub <= counter.y) {
			counter.y -= ub;
			scan.y++;
		} else if (ub <= -counter.y) {
			counter.y += ub;
			scan.y--;
		}
		ret.push_back(scan);
	}

	return ret;
}

std::vector<point> continue_line(const std::vector<point>& line, int distance)
{
	return line_from(line.back(), line.back()-line.front(), distance);
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

	DEBUG_FAIL_OR_LEAVE(0 > int(dir) || sizeof(translate) / sizeof(*translate) <= int(dir), return "WEIRD DIRECTION_NAME() BUG");
	return translate[dir];
}

point direction_vector(direction dir)
{
	DEBUG_FAIL_OR_LEAVE(0 > int(dir) || sizeof(Direction::vector) / sizeof(*Direction::vector) <= int(dir), return point((unsigned int)(-1) / 2, (unsigned int)(-1) / 2));
	return Direction::vector[dir];
}

direction rotate_clockwise(direction dir, int steps)
{
	steps = steps % 8;	// magic constant
	if (0 > steps) steps += 8;
	if (0 == steps) return dir;
	return direction((dir + steps) % 8);
}
