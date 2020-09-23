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

#endif
