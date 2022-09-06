#ifndef _ENUMS_H_
#define _ENUMS_H_

// kludge implementation -- want something more generalizable
constexpr int cmp(int lhs, int rhs)
{
	return (lhs < rhs) ? -1 : (lhs > rhs);
}

struct point {
 typedef int coord_type;

 int x;
 int y;
 constexpr point() noexcept : x(0), y(0) {}
 explicit constexpr point(int X) noexcept : x(X), y(X) {}	// the diagonal projection of the integers Z into the integer plane ZxZ
 constexpr point(int X, int Y)  noexcept : x (X), y (Y) {}
 point(const point &p) = default;
 point& operator=(const point& p) = default;

 constexpr point operator-() const { return point(-x, -y); }

 point& operator+=(const point& rhs) {
	 x += rhs.x;
	 y += rhs.y;
	 return *this;
 }
 point& operator-=(const point& rhs) {
	 x -= rhs.x;
	 y -= rhs.y;
	 return *this;
 }
 point& operator*=(int rhs) {
	 x *= rhs;
	 y *= rhs;
	 return *this;
 }
 point& operator/=(int rhs) {
	 x /= rhs;
	 y /= rhs;
	 return *this;
 }
 point& operator%=(int rhs) {
	 x %= rhs;
	 y %= rhs;
	 return *this;
 }
};

inline constexpr bool operator==(const point& lhs, const point& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
inline constexpr bool operator!=(const point& lhs, const point& rhs) { return lhs.x != rhs.x || lhs.y != rhs.y; }
inline point operator+(const point& lhs, const point& rhs) { return point(lhs) += rhs; }
inline point operator-(const point& lhs, const point& rhs) { return point(lhs) -= rhs; }
inline point operator*(int s, const point& pt) { return point(pt) *= s; }
inline point operator*(const point& pt, int s) { return point(pt) *= s; }
inline point operator/(const point& pt, int s) { return point(pt) /= s; }
inline point operator%(const point& pt, int s) { return point(pt) %= s; }

template<class T>
bool pointwise_test(const point& lhs, const point& rhs, T rel)
{
	return rel(lhs.x, rhs.x) && rel(lhs.y, rhs.y);
}

constexpr point cmp(point lhs, point rhs)
{
	return point(cmp(lhs.x, rhs.x), cmp(lhs.y, rhs.y));
}

struct tripoint {
 typedef int coord_type;

 int x;
 int y;
 int z;
 constexpr tripoint() noexcept : x(0), y(0), z(0) {}
 explicit constexpr tripoint(int X) noexcept : x(X), y(X), z(X) {};	// the diagonal projection of the integers Z into the integer space Z^3
 constexpr tripoint(int X, int Y, int Z) noexcept : x (X), y (Y), z (Z) {}
 tripoint(const tripoint &p) = default;
 tripoint& operator=(const tripoint& p) = default;

 tripoint& operator+=(const tripoint& rhs) {
	 x += rhs.x;
	 y += rhs.y;
	 z += rhs.z;
	 return *this;
 }
 tripoint& operator-=(const tripoint& rhs) {
	 x -= rhs.x;
	 y -= rhs.y;
	 z -= rhs.z;
	 return *this;
 }
 tripoint& operator*=(int rhs) {
	 x *= rhs;
	 y *= rhs;
	 z *= rhs;
	 return *this;
 }
 tripoint& operator/=(int rhs) {
	 x /= rhs;
	 y /= rhs;
	 z /= rhs;
	 return *this;
 }

 // canonical injection for point into tripoint is the xy-plane (makes Direction::vector, etc. work)
 tripoint& operator+=(const point& rhs) {
	 x += rhs.x;
	 y += rhs.y;
	 return *this;
 }
 tripoint& operator-=(const point& rhs) {
	 x -= rhs.x;
	 y -= rhs.y;
	 return *this;
 }
};

inline constexpr bool operator==(const tripoint& lhs, const tripoint& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z; }
inline constexpr bool operator!=(const tripoint& lhs, const tripoint& rhs) { return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z; }
inline tripoint operator+(const tripoint& lhs, const tripoint& rhs) { return tripoint(lhs) += rhs; }
inline tripoint operator-(const tripoint& lhs, const tripoint& rhs) { return tripoint(lhs) -= rhs; }
inline tripoint operator*(int s, const tripoint& pt) { return tripoint(pt) *= s; }
inline tripoint operator*(const tripoint& pt, int s) { return tripoint(pt) *= s; }

inline tripoint operator+(tripoint lhs, const point& rhs) { return lhs += rhs; }
inline tripoint operator+(const point& lhs, tripoint rhs) { return rhs += lhs; }
inline tripoint operator-(tripoint lhs, const point& rhs) { return lhs -= rhs; } // need unary minus to reverse this; evaluate necessity when use case available

inline bool operator<(const tripoint& lhs, const tripoint& rhs)
{
	if (lhs.x < rhs.x) return true;
	if (lhs.x > rhs.x) return false;
	if (lhs.y < rhs.y) return true;
	if (lhs.y > rhs.y) return false;
	return lhs.z < rhs.z;
}

template<class T>
bool pointwise_test(const tripoint& lhs, const tripoint& rhs, T rel)
{
	return rel(lhs.x, rhs.x) && rel(lhs.y, rhs.y) && rel(lhs.z, rhs.z);
}

// coordinate projections
inline constexpr point project_xy(const tripoint& src) { return point(src.x, src.y); }

#ifdef RNG_H
#ifdef ZAIMONI_STL_GDI_BOX_HPP
#include "fragment.inc/rng_box.hpp"
#endif
#endif

namespace cataclysm {

class JSON;

// exceptionally un-threadsafe; intent is to provide a standard "overflow" for reference returns
// We *always* have to set these variables to a known-good state.  They're used to fake data in out-of-bounds
// array dereferences, etc.  So it doesn't matter if the linker doesn't de-duplicate these.
template<class T>
struct discard
{
	inline static T x; // MSVC++ hard-errors on any external template instantiations when inline is present
};

// We do *not* want to declare external template instantiations here, to turn off a Clang warning.

// preprocessor blocks controlled by #if NONINLINE_EXPLICIT_INSTANTIATION
// contain the explicit template instantiations needed when the inline keyword is absent.
// If there are no issues evident by 2021-07-15, it is ok to remove all of those preprocessor blocks
// and these instructions 2021-06-16 zaimoni

}

// must specialize to be useful
template<class T> struct _ref;

bool fromJSON(const cataclysm::JSON& src, point& dest);
bool fromJSON(const cataclysm::JSON& src, tripoint& dest);

cataclysm::JSON toJSON(const point& src);
cataclysm::JSON toJSON(const tripoint& src);

#endif
