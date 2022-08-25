#ifndef GPS_LOC_HPP
#define GPS_LOC_HPP 1

#include "enums.h"
#include <limits.h>
#include <utility>
#include <variant>
#include <optional>
#include <vector>
#include <string>

struct field;
class item;
class player;
class vehicle;
enum ter_id : int;
enum trap_id : int;
enum vhtype_id : int;

constexpr int OMAP = 180;
constexpr int OMAPX = OMAP;
constexpr int OMAPY = OMAP;

// normalized range for GPS_loc.second is 0..SEE-1, 0..SEE-1
// reality_bubble_loc.second == GPS_loc.second for normalized GPS_loc corresponding to reality_bubble_loc
struct GPS_loc : public std::pair<tripoint, point>
{
	GPS_loc() = default;
	constexpr GPS_loc(tripoint _pos, point coord) noexcept : std::pair<tripoint, point>(_pos, coord) {}
	constexpr explicit GPS_loc(const std::pair<tripoint, point>& src) noexcept : std::pair<tripoint, point>(src) {}
	GPS_loc(const GPS_loc& src) = default;
	~GPS_loc() = default;
	GPS_loc& operator=(const GPS_loc& src) = default;

	GPS_loc& operator+=(const point& src);
	GPS_loc& operator+=(const tripoint& src);

	// following in map.cpp
	ter_id& ter();
	ter_id ter() const;
	bool is_outside() const;
	bool is_transparent() const;
	bool is_bashable() const;
	std::optional<std::pair<vehicle*, int>> veh_at() const;
	trap_id& trap_at();
	trap_id trap_at() const;
	field& field_at();
	const field& field_at() const { return const_cast<GPS_loc*>(this)->field_at(); }
	std::vector<item>& items_at();
	const std::vector<item>& items_at() const { return const_cast<GPS_loc*>(this)->items_at(); }
	int move_cost() const;
	void add(const item& new_item);
	void add(item&& new_item);
	bool add(field&& src);
	std::optional<std::vector<GPS_loc> > sees(const GPS_loc& dest, int range) const;
	bool can_see(const GPS_loc& dest, int range) const;

	// follwing in mapgen.cpp
	vehicle* add_vehicle(vhtype_id type, int deg);

	// following thin adapters in game.cpp
	bool is_empty() const;
	void explosion(int power, int shrapnel, bool fire) const;	// may not be a reasonable API
	void sound(int vol, const char* description) const;
	void sound(int vol, const std::string& description) const;
	bool bash(int str, std::string& sound, int* res = nullptr) const;

	// following thin adapters in map.cpp
	void shoot(int& dam, bool hit_items, unsigned flags);
	bool hard_landing(item&& thrown, player* p = nullptr); // for thrown objects
	bool displace_water();
};

// \todo evaluate whether these should be out-of-line defined (likely a matter of binary size, compile+link time)
inline GPS_loc operator+(GPS_loc lhs, const point& rhs) { return lhs += rhs; }
inline GPS_loc operator+(const point& lhs, GPS_loc rhs) { return rhs += lhs; }
inline GPS_loc operator+(GPS_loc lhs, const tripoint& rhs) { return lhs += rhs; }
inline GPS_loc operator+(const tripoint& lhs, GPS_loc rhs) { return rhs += lhs; }
inline GPS_loc operator+(GPS_loc lhs, const std::variant<point, tripoint>& rhs) {
	if (auto test = std::get_if<point>(&rhs)) return lhs += *test;
	return lhs += std::get<tripoint>(rhs);
}
inline GPS_loc operator+(const std::variant<point, tripoint>& lhs, GPS_loc rhs) {
	if (auto test = std::get_if<point>(&lhs)) return rhs += *test;
	return rhs += std::get<tripoint>(lhs);
}

// following in line.cpp
std::vector<GPS_loc> continue_line(const std::vector<GPS_loc>& line, int distance);

// following in overmap.cpp
std::variant<point, tripoint> operator-(const GPS_loc& lhs, const GPS_loc& rhs);
std::variant<point, tripoint> cmp(const GPS_loc& lhs, const GPS_loc& rhs);
int rl_dist(GPS_loc lhs, GPS_loc rhs);

struct _OM_loc : public std::pair<tripoint, point>
{
protected:
	_OM_loc() = default;
	constexpr _OM_loc(tripoint _pos, point coord) noexcept : std::pair<tripoint, point>(_pos, coord) {}
	constexpr explicit _OM_loc(const std::pair<tripoint, point> & src) noexcept : std::pair<tripoint, point>(src) {}
	_OM_loc(const _OM_loc& src) = default;
	~_OM_loc() = default;
	_OM_loc& operator=(const _OM_loc& src) = default;

public:
	void self_normalize(int scale = 2);
	void self_denormalize(const tripoint& view, int scale = 2);
};

template<size_t scale>
struct OM_loc : public _OM_loc
{
	static_assert(1 == scale || 2 == scale);

	OM_loc() = default;
	constexpr OM_loc(tripoint _pos, point coord) noexcept : _OM_loc(_pos, coord) {}
	constexpr explicit OM_loc(const std::pair<tripoint, point> &src) noexcept : _OM_loc(src) {}
	OM_loc(const OM_loc & src) = default;
	~OM_loc() = default;
	OM_loc& operator=(const OM_loc & src) = default;

	static bool in_bounds(int x, int y) { return 0 <= x && 2 * OMAP / scale > x && 0 <= y && 2 * OMAP / scale > y; }
	static bool in_bounds(const point& pt) { return 0 <= pt.x && 2 * OMAP / scale > pt.x && 0 <= pt.y && 2 * OMAP / scale > pt.y; }
	bool in_bounds() const { return in_bounds(second); }
	bool is_valid() const { return tripoint(INT_MAX) != first || in_bounds(); }
	void self_normalize() { return static_cast<_OM_loc*>(this)->self_normalize(scale); }
	void self_denormalize(const tripoint& view) { return static_cast<_OM_loc*>(this)->self_denormalize(view, scale); }
};

int rl_dist(OM_loc<1> lhs, OM_loc<1> rhs);
int rl_dist(OM_loc<2> lhs, OM_loc<2> rhs);

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

template<>
struct _ref<GPS_loc>
{
	static constexpr const GPS_loc invalid = GPS_loc(tripoint(INT_MAX), point(-1));
};

template<>
struct _ref<OM_loc<2>>
{
	static constexpr const OM_loc<2> invalid = OM_loc<2>(tripoint(INT_MAX), point(-1));
};

template<>
struct _ref<OM_loc<1>>
{
	static constexpr const OM_loc<1> invalid = OM_loc<1>(tripoint(INT_MAX), point(-1));
};


#endif
