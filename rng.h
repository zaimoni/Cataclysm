#ifndef RNG_H
#define RNG_H

#include <algorithm>
#include <vector> // for std::empty(),std::size()

long rng(long low, long high);
int dice(int number, int sides);

inline bool one_in(int chance) { return (chance <= 1 || rng(0, chance - 1) == 0); }
inline bool rng_lte(long low, long high, long ub) { return (high <= ub) ? true : (ub < low ? false : rng(low, high) <= ub); }

template<class ContainerType>
void shuffle_contents(ContainerType& v)
{
    if (std::empty(v)) { return; }
    auto s = std::size(v) - 1;
    for (decltype(s) i = 0; i < s; ++i) {
        using std::swap;
        swap(v[i], v[rng(i,s)]);
    }
}

#ifdef _ENUMS_H_
#ifdef ZAIMONI_STL_GDI_BOX_HPP
#include "fragment.inc/rng_box.hpp"
#endif
#endif

#endif
