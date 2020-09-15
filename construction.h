#ifndef _CONSTRUCTION_H_ 
#define _CONSTRUCTION_H_

#include "crafting.h"
#include "mapdata.h"
#include "enums.h"

struct construction_stage
{
 ter_id terrain;
 int time; // In minutes, i.e. 10 turns
 std::vector<itype_id> tools[3];
 std::vector<component> components[3];

 construction_stage(ter_id Terrain, int Time) noexcept : terrain (Terrain), time (Time) { };
};

struct constructable
{
 static std::vector<constructable*> constructions; // The list of constructions

 int id;
 std::string name; // Name as displayed
 int difficulty; // Carpentry skill level required
 std::vector<construction_stage> stages;
 bool (*able)  (map&, point);
 void (*done)  (game *, point);

 constructable(int Id, std::string Name, int Diff,
               bool (*Able) (map&, point),
               void (*Done) (game *, point)) :
  id (Id), name (Name), difficulty (Diff), able (Able), done (Done) {};

 static void init();
};

#endif
