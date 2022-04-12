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

static void transfer_ub_to_orthogonal_delta(int& test, int& ortho_delta, const int ub)
{
	if (ub == test) {
		ortho_delta = 1;
		test = 0;
	} else if (ub == -test) {
		ortho_delta = -1;
		test = 0;
	}
}

static void transfer_ub_to_orthogonal_delta(point& test, point& ortho_delta, const int ub)
{
	transfer_ub_to_orthogonal_delta(test.x, ortho_delta.x, ub);
	transfer_ub_to_orthogonal_delta(test.y, ortho_delta.y, ub);
}

static void transfer_ub_to_orthogonal_delta(tripoint& test, tripoint& ortho_delta, const int ub)
{
	transfer_ub_to_orthogonal_delta(test.x, ortho_delta.x, ub);
	transfer_ub_to_orthogonal_delta(test.y, ortho_delta.y, ub);
	transfer_ub_to_orthogonal_delta(test.z, ortho_delta.z, ub);
}

// note that we have several notions of "visibility" so what counts as "blocking" is a parameter
// \todo handle chess knight-move issue for == case
static void normalize_step(int& test, int& dest, const int ub)
{
	if (ub <= test) {
		test -= 2 * ub;
		dest++;
	} else if (ub <= -test) {
		test += 2 * ub;
		dest--;
	}
}

static void normalize_step(point& test, point& dest, const int ub)
{
	normalize_step(test.x, dest.x, ub);
	normalize_step(test.y, dest.y, ub);
}

static void normalize_step(tripoint& test, tripoint& dest, const int ub)
{
	normalize_step(test.x, dest.x, ub);
	normalize_step(test.y, dest.y, ub);
	normalize_step(test.z, dest.z, ub);
}

// direct template conversion for these two fails MSVC++
static std::vector<point> line_from(const point& origin, const point& delta, int distance)
{
	std::vector<point> ret;
//	if (0 >= distance) return ret;	// should not happen so don't test for it
	const auto ub = Linf_dist(delta);
	if (0 >= ub) return ret;

	point thresholds(2 * delta);	// \todo fix, or reject, signed overflow here
	point primary(0);

	transfer_ub_to_orthogonal_delta(thresholds, primary, 2 * ub);

	point scan(origin);
	point counter(0);
	while (0 <= --distance) {
		scan += primary;
		counter += thresholds;
		normalize_step(counter, scan, ub);
		ret.push_back(scan);
	}

	return ret;
}

static std::vector<tripoint> line_from(const tripoint& origin, const tripoint& delta, int distance)
{
	std::vector<tripoint> ret;
	//	if (0 >= distance) return ret;	// should not happen so don't test for it
	const auto ub = Linf_dist(delta);
	if (0 >= ub) return ret;

	auto thresholds(2 * delta);	// \todo fix, or reject, signed overflow here
	tripoint primary(0);

	transfer_ub_to_orthogonal_delta(thresholds, primary, 2 * ub);

	auto scan(origin);
	decltype(scan) counter(0);
	while (0 <= --distance) {
		scan += primary;
		counter += thresholds;
		normalize_step(counter, scan, ub);
		ret.push_back(scan);
	}

	return ret;
}

std::vector<point> continue_line(const std::vector<point>& line, int distance)
{
	return line_from(line.back(), line.back()-line.front(), distance);
}

std::vector<tripoint> continue_line(const std::vector<tripoint>& line, int distance)
{
	return line_from(line.back(), line.back() - line.front(), distance);
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
