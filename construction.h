#ifndef _CONSTRUCTION_H_ 
#define _CONSTRUCTION_H_

#include "crafting.h"
#include "mapdata.h"
#include "enums.h"

struct construction_stage
{
 ter_id terrain;
 int time; // In minutes, i.e. 10 turns
 // Intent appears to be AND of OR for each of these.
 std::vector<std::vector<itype_id> > tools;
 std::vector<std::vector<component> > components;

 construction_stage(ter_id Terrain, int Time) noexcept : terrain (Terrain), time (Time) { };
 construction_stage(const construction_stage& src) = default;
 construction_stage(construction_stage&& src) = default;
 construction_stage& operator=(const construction_stage& src) = default;
 construction_stage& operator=(construction_stage&& src) = default;
 ~construction_stage() = default;
};

struct constructable
{
 static std::vector<constructable*> constructions; // The list of constructions

 int id;
 std::string name; // Name as displayed
 int difficulty; // Carpentry skill level required
 std::vector<construction_stage> stages;
#ifndef SOCRATES_DAIMON
 bool (*able)    (map&, point);
 bool (*able_gps)(const GPS_loc&);
 void (*done)    (game *, point);
 void (*done_gps)(GPS_loc);
 void (*done_pl) (player&);
#endif

private:
 constructable(int Id, std::string Name, int Diff
#ifndef SOCRATES_DAIMON
     , decltype(able) Able, decltype(done) Done
#endif
 ) :
  id (Id), name (Name), difficulty (Diff)
#ifndef SOCRATES_DAIMON
     , able (Able), able_gps(nullptr), done(Done), done_gps(nullptr), done_pl(nullptr)
#endif
 {};

#ifndef SOCRATES_DAIMON
 constructable(int Id, std::string Name, int Diff
     , decltype(able) Able
 ) :
     id(Id), name(Name), difficulty(Diff)
     , able(Able), able_gps(nullptr), done(nullptr), done_gps(nullptr), done_pl(nullptr)
 {};

 constructable(int Id, std::string Name, int Diff
     , decltype(able_gps) Able
 ) :
     id(Id), name(Name), difficulty(Diff)
     , able(nullptr), able_gps(Able), done(nullptr), done_gps(nullptr), done_pl(nullptr)
 {};

 constructable(int Id, std::string Name, int Diff
     , decltype(able) Able, decltype(done_pl) Done
 ) :
     id(Id), name(Name), difficulty(Diff)
     , able(Able), able_gps(nullptr), done(nullptr), done_gps(nullptr), done_pl(Done)
 {};

 constructable(int Id, std::string Name, int Diff
     , decltype(able) Able, decltype(done_gps) Done
 ) :
     id(Id), name(Name), difficulty(Diff)
     , able(Able), able_gps(nullptr), done(nullptr), done_gps(Done), done_pl(nullptr)
 {};

 constructable(int Id, std::string Name, int Diff
     , decltype(able_gps) Able, decltype(done_gps) Done
 ) :
     id(Id), name(Name), difficulty(Diff)
     , able(nullptr), able_gps(Able), done(nullptr), done_gps(Done), done_pl(nullptr)
 {};
#endif

public:
 static void init();
};

#endif
