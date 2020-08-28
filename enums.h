#ifndef _ENUMS_H_
#define _ENUMS_H_

// kludge implementation -- want something more generalizable
constexpr int cmp(int lhs, int rhs)
{
	return (lhs < rhs) ? -1 : (lhs > rhs);
}

constexpr int is_between(int lb, int src , int ub)
{
	return lb <= src && src <= ub;
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
};

inline constexpr bool operator==(const tripoint& lhs, const tripoint& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z; }
inline constexpr bool operator!=(const tripoint& lhs, const tripoint& rhs) { return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z; }
inline tripoint operator+(const tripoint& lhs, const tripoint& rhs) { return tripoint(lhs) += rhs; }
inline tripoint operator-(const tripoint& lhs, const tripoint& rhs) { return tripoint(lhs) -= rhs; }
inline tripoint operator*(int s, const tripoint& pt) { return tripoint(pt) *= s; }
inline tripoint operator*(const tripoint& pt, int s) { return tripoint(pt) *= s; }

inline bool operator<(const tripoint& lhs, const tripoint& rhs)
{
	if (lhs.x < rhs.x) return true;
	if (lhs.x > rhs.x) return false;
	if (lhs.y < rhs.y) return true;
	if (lhs.y > rhs.y) return false;
	return lhs.z < rhs.z;
}

namespace cataclysm {

class JSON;

// exceptionally un-threadsafe; intent is to provide a standard "overflow" for reference returns
template<class T>
struct discard
{
	static T x;
};

}

// must specialize to be useful
template<class T> struct _ref;

bool fromJSON(const cataclysm::JSON& src, point& dest);
bool fromJSON(const cataclysm::JSON& src, tripoint& dest);

cataclysm::JSON toJSON(const point& src);
cataclysm::JSON toJSON(const tripoint& src);

#endif
