#ifndef _PLDATA_H_
#define _PLDATA_H_

#include "enums.h"
#include "enum_json.h"
#include "pldata_enum.h"

#include <vector>

DECLARE_JSON_ENUM_SUPPORT(dis_type)
DECLARE_JSON_ENUM_SUPPORT(add_type)

struct disease
{
 dis_type type;
 int intensity;
 int duration;
 disease(dis_type t = DI_NULL, int d = 0, int i = 0) : type(t),intensity(i),duration(d) {}

 int speed_boost() const;
 const char* name() const;
 std::string invariant_desc() const;
};

struct addiction
{
 add_type type;
 int intensity;
 int sated;
 addiction() : type(ADD_NULL),intensity(0),sated(600) {}
 addiction(add_type t, int i = 1) : type(t), intensity(i), sated(600) {}
};

DECLARE_JSON_ENUM_SUPPORT(activity_type)

struct player_activity
{
 activity_type type;
 int moves_left;
 int index;
 std::vector<int> values;
 point placement;

 player_activity(activity_type t = ACT_NULL, int turns = 0, int Index = -1)
 : type(t),moves_left(turns),index(Index),placement(-1,-1) {}
 player_activity(const player_activity &copy) = default;
};
 
DECLARE_JSON_ENUM_SUPPORT(pl_flag)
DECLARE_JSON_ENUM_SUPPORT_ATYPICAL(hp_part,0)

#endif
