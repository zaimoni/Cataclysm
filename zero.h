#ifndef ZERO_H

// zero-dependency header

// upper-bound clamp
template<class T, class U> void clamp_ub(T& x, const U& ref) { if (ref < x) x = ref; }
template<auto ref, class T> void clamp_ub(T& x) { if (ref < x) x = ref; }
template<class T> void clamp_ub(T& x, const T& ref) { if (ref < x) x = ref; }

// lower-bound clamp
template<class T, class U> void clamp_lb(T& x, const U& ref) { if (ref > x) x = ref; }
template<auto ref, class T> void clamp_lb(T& x) { if (ref > x) x = ref; }
template<class T> void clamp_lb(T& x, const T& ref) { if (ref > x) x = ref; }

namespace cataclysm {

template<class T, class U>
constexpr auto min(const T& lhs, const U& rhs) { return lhs < rhs ? lhs : rhs; }

template<class T, class U>
constexpr auto max(const T& lhs, const U& rhs) { return rhs < lhs ? rhs : lhs; }

}

#endif
