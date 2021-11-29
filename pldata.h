#ifndef _PLDATA_H_
#define _PLDATA_H_

#include "enums.h"
#include "enum_json.h"
#include "pldata_enum.h"

#include <vector>

DECLARE_JSON_ENUM_SUPPORT(dis_type)
DECLARE_JSON_ENUM_SUPPORT(add_type)

struct stat_delta {
    int Str;
    int Dex;
    int Per;
    int Int;
};

struct disease
{
 dis_type type;
 int intensity;
 int duration;
 disease(dis_type t = DI_NULL, int d = 0, int i = 0) noexcept : type(t),intensity(i),duration(d) {}

 int speed_boost() const;
 const char* name() const;
 std::string invariant_desc() const;
};

struct addiction
{
 add_type type;
 int intensity;
 int sated;	// time-scaled
 addiction() : type(ADD_NULL),intensity(0),sated(600) {}
 addiction(add_type t, int i = 1) : type(t), intensity(i), sated(600) {}
};

stat_delta addict_stat_effects(const addiction& add);   // \todo -> member function
std::string addiction_name(const addiction& cur);   // \todo -> member function
std::string addiction_text(const addiction& cur);   // \todo -> member function

DECLARE_JSON_ENUM_SUPPORT(activity_type)

struct player_activity
{
 activity_type type;
 int moves_left;
 int index;
 std::vector<int> values;
 point placement; // low priority to convert to GPS coordinates (needed for NPC activities outside of reality bubble) 2020-08-13 zaimoni

 player_activity(activity_type t = ACT_NULL, int turns = 0, int Index = -1) noexcept
 : type(t),moves_left(turns),index(Index),placement(-1,-1) {}
 player_activity(const player_activity &copy) = default;
 player_activity(player_activity&& copy) = default;
 player_activity& operator=(const player_activity& copy) = default;
 player_activity& operator=(player_activity && copy) = default;

 void clear() noexcept {    // Technically ok to sink this into a *.cpp 2020-08-11 zaimoni
     type = ACT_NULL;
     decltype(values)().swap(values);
 }
};
 
DECLARE_JSON_ENUM_SUPPORT(pl_flag)
DECLARE_JSON_ENUM_SUPPORT_ATYPICAL(hp_part,0)

#endif
