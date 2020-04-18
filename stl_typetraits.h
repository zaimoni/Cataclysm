#ifndef STL_TYPETRAITS_H
#define STL_TYPETRAITS_H

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

// check that C++ class is defined
template <class T, class Enable = void>
struct is_defined
{
	static constexpr bool value = false;
};

template <class T>
struct is_defined<T, std::enable_if_t<sizeof(T)> >
{
	static constexpr bool value = true;
};

#endif
