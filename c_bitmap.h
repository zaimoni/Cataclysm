#ifndef C_BITMAP_H
#define C_BITMAP_H 1

#include <stddef.h>
#include <limits.h>
#include <type_traits>

// mfb(n) converts a flag to its appropriate position in a C bitfield
#define mfb(n) (1ULL << (n))

namespace cataclysm {

template<size_t n>
struct bitmap
{
	static_assert(sizeof(unsigned long long)* CHAR_BIT >= n, "use std::bitmap or std::vector<bool> instead");
	static_assert(0 < n, "zero-bit bitmap is pointless");
	typedef typename std::conditional<sizeof(unsigned char)* CHAR_BIT >= n, unsigned char,
		typename std::conditional<sizeof(unsigned short)* CHAR_BIT >= n, unsigned short,
		typename std::conditional<sizeof(unsigned int)* CHAR_BIT >= n, unsigned int,
		typename std::conditional<sizeof(unsigned long)* CHAR_BIT >= n, unsigned long, unsigned long long>::type>::type>::type>::type type;
};

}


#define DECLARE_JSON_ENUM_BITFLAG_SUPPORT(TYPE)	\
namespace cataclysm {	\
	template<>	\
	struct JSON_parse<TYPE>	\
	{	\
		unsigned long long operator()(const std::vector<const char*>& src);	\
		std::vector<const char*> operator()(unsigned long long src);	\
	};	\
}


#define DEFINE_JSON_ENUM_BITFLAG_SUPPORT(TYPE,STATIC_REF)	\
namespace cataclysm {	\
	unsigned long long JSON_parse<TYPE>::operator()(const std::vector<const char*>& src)	\
	{	\
		if (src.empty()) return 0;	\
		unsigned ret = 0;	\
		ptrdiff_t i = sizeof(STATIC_REF) / sizeof(*STATIC_REF);	\
		while (0 < i--) {	\
			for (const auto& x : src) {	\
				if (!strcmp(STATIC_REF[i].second, x)) {	\
					ret |= STATIC_REF[i].first;	\
					break;	\
				}	\
			}	\
		}	\
		return ret;	\
	}	\
	\
	std::vector<const char*> JSON_parse<TYPE>::operator()(unsigned long long src)	\
	{	\
		std::vector<const char*> ret;	\
	\
		if (src) {	\
			ptrdiff_t i = sizeof(STATIC_REF) / sizeof(*STATIC_REF);	\
			while (0 < i--) if (src & STATIC_REF[i].first) ret.push_back(STATIC_REF[i].second);	\
		}	\
	\
		return ret;	\
	}	\
}

#endif
