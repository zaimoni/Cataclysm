#ifndef MOBILE_H
#define MOBILE_H

#include "GPS_loc.hpp"
#include <type_traits>

namespace cataclysm {

	class JSON;

}

// intended base class for things that move
class mobile
{
public:
	GPS_loc GPSpos; // absolute location
	int moves;

protected:
	mobile() noexcept : GPSpos(_ref<GPS_loc>::invalid),moves(0) {}
	mobile(const GPS_loc& origin, int m) noexcept : GPSpos(origin), moves(m) {}
	mobile(const mobile& src) = default;
	mobile(mobile&& src) = default;

	mobile(const cataclysm::JSON& src);

	virtual ~mobile() = default;
	mobile& operator=(const mobile& src) = default;
	mobile& operator=(mobile&& src) = default;
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
