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

#include <optional>
#include <vector>

// All of these can have their bodies moved out-of-line, if needed for compile time management.

/// <summary>
/// Intended interpretation is uniform distribution within the box.
/// </summary>
inline point rng(const zaimoni::gdi::box<point>& src) { return point(rng(src.tl_c().x, src.br_c().x), rng(src.tl_c().y, src.br_c().y)); }

// Technically, following should be in a header for enums.h and Zaimoni.STL/GDI/box.hpp only, but our inclusion point is effectively "early" (map.h).
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
	const point nonstrict_ub = src.br_c();
	point pt;
	for (pt.x = anchor.x; pt.x <= nonstrict_ub.x; ++pt.x) {
		for (pt.y = anchor.y; pt.y <= nonstrict_ub.y; ++pt.y) {
			op(pt);
		}
	}
}

inline std::optional<point> find_first(const zaimoni::gdi::box<point>& src, std::function<bool(point)> test) {
	// this will need revision if we want to actually use INT_MAX upper bound
	const point anchor = src.tl_c();
	const point nonstrict_ub = src.br_c();
	point pt;
	for (pt.x = anchor.x; pt.x <= nonstrict_ub.x; ++pt.x) {
		for (pt.y = anchor.y; pt.y <= nonstrict_ub.y; ++pt.y) {
			if (test(pt)) return pt;
		}
	}
	return std::nullopt;
}

inline std::vector<point> grep(const zaimoni::gdi::box<point>& src, std::function<bool(point)> ok) { // Cf. Perl, or *NIX command line utility grep
	// this will need revision if we want to actually use INT_MAX upper bound
	std::vector<point> ret;
	const point anchor = src.tl_c();
	const point nonstrict_ub = src.br_c();
	point pt;
	for (pt.x = anchor.x; pt.x <= nonstrict_ub.x; ++pt.x) {
		for (pt.y = anchor.y; pt.y <= nonstrict_ub.y; ++pt.y) {
			if (ok(pt)) ret.push_back(pt);
		}
	}
	return ret;
}

inline point clamped(point src, const zaimoni::gdi::box<point>& bounds) {
	if (src.x < bounds.tl_c().x) src.x = bounds.tl_c().x;
	else if (bounds.br_c().x < src.x) src.x = bounds.br_c().x;
	if (src.y < bounds.tl_c().y) src.y = bounds.tl_c().y;
	else if (bounds.br_c().y < src.y) src.y = bounds.br_c().y;
	return src;
}

inline point clamped_lb(point src, const point& ref) {
	if (src.x < ref.x) src.x = ref.x;
	if (src.y < ref.y) src.y = ref.y;
	return src;
}

inline point clamped_ub(point src, const point& ref) {
	if (ref.x < src.x) src.x = ref.x;
	if (ref.y < src.y) src.y = ref.y;
	return src;
}

// \todo does not belong here -- only needs std::optional and std::function
template<class T>
std::optional<T> LasVegasChoice(int tries, std::function<T()> generate, std::function<bool(const T&)> ok)
{
	while (0 <= --tries) {
		auto test = generate();
		if (ok(test)) return test;
	}
	return std::nullopt;
}

template<class T>
std::optional<T> LasVegasChoice(int tries, std::function<T()> generate, bool (ok)(const T&))
{
	while (0 <= --tries) {
		auto test = generate();
		if (ok(test)) return test;
	}
	return std::nullopt;
}

#endif
