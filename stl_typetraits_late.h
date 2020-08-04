#ifndef STL_TYPETRAITS_LATE_H
#define STL_TYPETRAITS_LATE_H 1

#include "stl_typetraits.h"	// insurance policy
#include <stdexcept>

// example model for begin,end is container of std::pair<T,int>
// example model n is rng(...,...)
template<class T>
auto use_rarity_table(long n, T begin, T end)
{
#ifndef NDEBUG
    if constexpr(is_defined<std::logic_error>::value) {
        if (!begin || !end || begin == end) throw std::logic_error("rarity table use flawed");
    }
#endif
    auto iter = begin;
    auto prev = begin;
    do {
        if (n < iter->second) return iter->first;
        n -= iter->second;
        prev = iter++;
    } while (iter != end);
    return prev->first;
}

#endif
