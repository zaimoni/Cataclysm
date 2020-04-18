#ifndef ZERO_H

// zero-dependency header

// upper-bound clamp
template<class T, class U> void clamp_ub(T& x, const U& ref) { if (ref < x) x = ref; }
template<class T, auto ref> void clamp_ub(T& x) { if (ref < x) x = ref; }
template<class T> void clamp_ub(T& x, const T& ref) { if (ref < x) x = ref; }

#endif
