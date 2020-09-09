#ifndef GPS_LOC_HPP
#define GPS_LOC_HPP 1

#include "enums.h"
#include <limits.h>
#include <utility>

struct GPS_loc : public std::pair<tripoint, point>
{
	GPS_loc() = default;
	constexpr GPS_loc(tripoint _pos, point coord) noexcept : std::pair<tripoint, point>(_pos, coord) {}
	constexpr explicit GPS_loc(const std::pair<tripoint, point>& src) noexcept : std::pair<tripoint, point>(src) {}
	GPS_loc(const GPS_loc& src) = default;
	~GPS_loc() = default;
	GPS_loc& operator=(const GPS_loc& src) = default;

	GPS_loc& operator+=(const point& src);
};

struct OM_loc : public std::pair<tripoint, point>
{
	OM_loc() = default;
	constexpr OM_loc(tripoint _pos, point coord) noexcept : std::pair<tripoint, point>(_pos, coord) {}
	constexpr explicit OM_loc(const std::pair<tripoint, point> & src) noexcept : std::pair<tripoint, point>(src) {}
	OM_loc(const OM_loc& src) = default;
	~OM_loc() = default;
	OM_loc& operator=(const OM_loc& src) = default;

	bool is_valid() const;
	void self_normalize();
	void self_denormalize(const tripoint& view);
};

struct reality_bubble_loc : public std::pair<int, point>
{
	reality_bubble_loc() = default;
	constexpr reality_bubble_loc(int _pos, point coord) noexcept : std::pair<int, point>(_pos, coord) {}
	constexpr explicit reality_bubble_loc(const std::pair<int, point>& src) noexcept : std::pair<int, point>(src) {}
	reality_bubble_loc(const reality_bubble_loc& src) = default;
	~reality_bubble_loc() = default;
	reality_bubble_loc& operator=(const reality_bubble_loc& src) = default;

	bool is_valid() const;
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
