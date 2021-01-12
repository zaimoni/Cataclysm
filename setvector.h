#ifndef _SETVECTOR_H_
#define _SETVECTOR_H_

// Zaimoni, 2018-07-10: looks like all of the MSVC-breaking setvector calls 
// were replaced by externally loaded configuration in C:DDA prior to the Coolthulu fork.
// no direct guidance for immediately bringing up the build.

// Eliminated outright. 2021-01-12 zaimoni
// std::initializer_list is sufficient to handle anything these macros can handle.
#define SET_VECTOR(DEST,...)	\
{	\
const auto tmp = { __VA_ARGS__ };	\
(DEST).assign(std::begin(tmp), std::end(tmp));	\
}

#define SET_VECTOR_STRUCT(DEST,TYPE,...)	\
{	\
const TYPE tmp[] = __VA_ARGS__;	\
(DEST).assign(std::begin(tmp), std::end(tmp));	\
 }

#endif
