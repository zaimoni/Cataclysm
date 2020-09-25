#ifndef FRAGMENTS_INC_RNG_BOX_HPP
#define FRAGMENTS_INC_RNG_BOX_HPP 1

// enums.h is the header with point
#ifndef RNG_H
#error if including both rng.h and enums.h, do so after Zaimoni.STL/GDI/box.hpp
#endif
#ifndef ZAIMONI_STL_GDI_BOX_HPP
#error if including both rng.h and enums.h, do so after Zaimoni.STL/GDI/box.hpp
#endif
#ifndef _ENUMS_H_
#error if including both rng.h and enums.h, do so after Zaimoni.STL/GDI/box.hpp
#endif

/// <summary>
/// intended interpretation is uniform distribution within the box
/// </summary>
inline point rng(const zaimoni::gdi::box<point>& src) { return point(rng(src.tl_c().x, src.br_c().x), rng(src.tl_c().y, src.br_c().y)); }

// technically following should be in a header for enums.h and Zaimoni.STL/GDI/box.hpp only, but our inclusion point is effectively "early" (map.h).
// box.hpp is providing #include <functional>.

inline void forall_do(const zaimoni::gdi::box<point>& src, std::function<void(point)> op) { // for API completeness
	const point anchor = src.tl_c();
	const point strict_ub = src.br_c();
	point pt;
	for (pt.x = anchor.x; pt.x < strict_ub.x; ++pt.x) {
		for (pt.y = anchor.y; pt.y < strict_ub.y; ++pt.y) {
			op(pt);
		}
	}
}

inline void forall_do_inclusive(const zaimoni::gdi::box<point>& src, std::function<void(point)> op) {
	// this will need revision if we want to actually use INT_MAX upper bound
	const point anchor = src.tl_c();
	const point strict_ub = src.br_c();
	point pt;
	for (pt.x = anchor.x; pt.x <= strict_ub.x; ++pt.x) {
		for (pt.y = anchor.y; pt.y <= strict_ub.y; ++pt.y) {
			op(pt);
		}
	}
}

#endif
