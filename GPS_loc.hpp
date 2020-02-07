#ifndef GPS_LOC_HPP
#define GPS_LOC_HPP 1

#include "enums.h"
#include <limits.h>
#include <utility>

struct GPS_loc : public std::pair<tripoint, point>
{
	GPS_loc() = default;
	constexpr GPS_loc(tripoint _pos, point coord) : std::pair<tripoint, point>(_pos, coord) {};
	constexpr explicit GPS_loc(const std::pair<tripoint, point>& src) : std::pair<tripoint, point>(src) {};
	GPS_loc(const GPS_loc& src) = default;
	~GPS_loc() = default;
	GPS_loc& operator=(const GPS_loc& src) = default;
};

struct OM_loc : public std::pair<tripoint, point>
{
	OM_loc() = default;
	constexpr OM_loc(tripoint _pos, point coord) : std::pair<tripoint, point>(_pos, coord) {};
	constexpr explicit OM_loc(const std::pair<tripoint, point> & src) : std::pair<tripoint, point>(src) {};
	OM_loc(const OM_loc& src) = default;
	~OM_loc() = default;
	OM_loc& operator=(const OM_loc& src) = default;

	bool is_valid() const;
	void self_normalize();
	void self_denormalize(const tripoint& view);
};

int rl_dist(OM_loc lhs, OM_loc rhs);

template<>
struct _ref<GPS_loc>
{
	static constexpr const GPS_loc invalid = GPS_loc(tripoint(INT_MAX), point(-1));
};

template<>
struct _ref<OM_loc>
{
	static constexpr const OM_loc invalid = OM_loc(tripoint(INT_MAX), point(-1));
};


#endif
