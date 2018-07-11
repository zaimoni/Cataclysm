#ifndef _SETVECTOR_H_
#define _SETVECTOR_H_
#include <vector>
#include "itype.h"
#include "mongroup.h"
#include "mapitems.h"
#include "crafting.h"
#include "mission.h"

// Zaimoni, 2018-07-10: looks like all of the MSVC-breaking setvector calls 
// were replaced by externally loaded configuration in C:DDA prior to the Coolthulu fork.
// no direct guidance for immediately bringing up the build.
#define SET_VECTOR(DEST,...)	\
{	\
auto tmp = { __VA_ARGS__ };	\
(DEST).assign(std::begin(tmp), std::end(tmp));	\
}

void setvector(std::vector <component> &vec, ... );	// auto fails: initializing struct, not scalar data.  itypedef.cpp
void setvector(std::vector <items_location_and_chance> &vec, ... );	// auto fails: initializing struct, not scalar data.  itypedef.cpp
void setvector(std::vector <style_move> &vec, ... );	// auto fails: initializing struct, not scalar data.  itypedef.cpp
template <class T> void setvec(std::vector<T> &vec, ... );
#endif
