#ifndef ENUM_JSON_H
#define ENUM_JSON_H

namespace cataclysm {

	template<class T>
	struct JSON_parse
	{
		T operator()(const char* src);	// fails unless specialized at link-time
	};

}

// declaration requires std::string; definition typically also requires string.h on MingWin but not MSVC
#define DECLARE_JSON_ENUM_SUPPORT(TYPE)	\
const char* JSON_key(TYPE src);	\
namespace cataclysm {	\
	template<>	\
	struct JSON_parse<TYPE>	\
	{	\
		enum { origin = 1 };	\
		TYPE operator()(const char* src);	\
		TYPE operator()(const std::string& src) { return operator()(src.c_str()); };	\
	};	\
}

#define DECLARE_JSON_ENUM_SUPPORT_ATYPICAL(TYPE,OFFSET)	\
const char* JSON_key(TYPE src);	\
namespace cataclysm {	\
	template<>	\
	struct JSON_parse<TYPE>	\
	{	\
		enum { origin = OFFSET };	\
		TYPE operator()(const char* src);	\
		TYPE operator()(const std::string& src) { return operator()(src.c_str()); };	\
	};	\
}

#define DEFINE_JSON_ENUM_SUPPORT_TYPICAL(TYPE,STATIC_REF)	\
const char* JSON_key(TYPE src)	\
{	\
	if (cataclysm::JSON_parse<TYPE>::origin <= src && (sizeof(STATIC_REF) / sizeof(*STATIC_REF))+cataclysm::JSON_parse<TYPE>::origin > src) return STATIC_REF[src - cataclysm::JSON_parse<TYPE>::origin];	\
	return 0;	\
}	\
	\
namespace cataclysm {	\
	TYPE JSON_parse<TYPE>::operator()(const char* const src)	\
	{	\
		if (!src || !src[0]) return TYPE(0);	\
		ptrdiff_t i = sizeof(STATIC_REF) / sizeof(*STATIC_REF);	\
		while (0 < i--) if (!strcmp(STATIC_REF[i], src)) return TYPE(i + JSON_parse<TYPE>::origin);	\
		return TYPE(0);	\
	}	\
}

// obsolete
#define DEFINE_JSON_ENUM_SUPPORT_HARDCODED_NONZERO(TYPE,STATIC_REF)	\
const char* JSON_key(TYPE src)	\
{	\
	if (1 <= src && (sizeof(STATIC_REF) / sizeof(*STATIC_REF))>= src) return STATIC_REF[src - 1];	\
	return 0;	\
}	\
	\
namespace cataclysm {	\
	TYPE JSON_parse<TYPE>::operator()(const char* const src)	\
	{	\
		if (!src || !src[0]) return TYPE(0);	\
		ptrdiff_t i = sizeof(STATIC_REF) / sizeof(*STATIC_REF);	\
		while (0 < i--) if (!strcmp(STATIC_REF[i], src)) return TYPE(i + 1);	\
		return TYPE(0);	\
	}	\
}

#endif

