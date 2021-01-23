#ifndef MOBILE_H
#define MOBILE_H

#include "GPS_loc.hpp"
#include <optional>
#include <type_traits>

namespace cataclysm {

	class JSON;

}

// intended base class for things that move
class mobile
{
public:
	static constexpr const int mp_turn = 100; // move points/standard turn

	GPS_loc GPSpos; // absolute location
	int moves;

	// Waterfall lifecycle here: unused member functions ok
	// throwing interfaces only appropriate for vehicles and player; monsters and NPCs may be slightly out of bounds
	std::optional<point> screen_pos() const;
	point screenPos() const;
	std::optional<reality_bubble_loc> bubble_pos() const;
	reality_bubble_loc bubblePos() const;

protected:
	mobile() noexcept : GPSpos(_ref<GPS_loc>::invalid),moves(0) {}
	mobile(const GPS_loc& origin, int m) noexcept : GPSpos(origin), moves(m) {}
	mobile(const mobile& src) = default;
	mobile(mobile&& src) = default;

	mobile(const cataclysm::JSON& src);

	virtual ~mobile() = default;
	mobile& operator=(const mobile& src) = default;
	mobile& operator=(mobile&& src) = default;

	void set_screenpos(point pt); // could be public once synchronization with legacy point pos not needed
};

template<class T>
class countdown
{
public:
	T x;
	int remaining;	// will want more access control after save/load conversion

	countdown() = default;
	countdown(const T& src, int t) noexcept(std::is_nothrow_copy_constructible_v<T>) : x(src), remaining(t) {}
	countdown(const countdown& src) = default;
	countdown(countdown&& src) = default;
	~countdown() = default;
	countdown& operator=(const countdown& src) = default;
	countdown& operator=(countdown&& src) = default;

	void set(const T& src, int t) {
		x = src;
		remaining = t;
	}
	void set(T&& src, int t) {
		x = std::move(src);
		remaining = t;
	}

	void tick() { if (0 < remaining) --remaining; }
	bool live() const { return 0 < remaining; }
};

#endif
