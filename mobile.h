#ifndef MOBILE_H
#define MOBILE_H

#include "enums.h"
#include <type_traits>

// intended base class for things that move
struct mobile	// \todo V0.2.1+ wire-in
{
	point pos;
	int moves;

protected:
	mobile() = default;
	mobile(const point& origin, int m) noexcept(std::is_nothrow_copy_constructible_v<point>) : pos(origin), moves(m) {};
	mobile(const mobile& src) = default;
	mobile(mobile&& src) = default;
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
