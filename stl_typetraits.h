#ifndef STL_VECTOR_H
#define STL_VECTOR_H

#include <type_traits>

namespace cataclysm {

// not fully correct, but should handle the simple cases correctly
template<class T, class V>
typename std::enable_if<sizeof(unsigned long long)>=sizeof(V),bool>::type any(const T& src, V rhs)
{
	for (const auto test : src) {
	  if (test == rhs) return true;
	}
	return false;
}

template<class T, class V>
typename std::enable_if<sizeof(unsigned long long) < sizeof(V), bool>::type any(const T& src, const V& rhs)
{
	for (const auto& test : src) {
		if (test == rhs) return true;
	}
	return false;
}

}

#endif
