#ifndef _TRAP_H_
#define _TRAP_H_

#include "itype.h"

class monster;
class game;

enum trap_id {
 tr_null,
 tr_bubblewrap,
 tr_beartrap,
 tr_beartrap_buried,
 tr_snare,
 tr_nailboard,
 tr_tripwire,
 tr_crossbow,
 tr_shotgun_2,
 tr_shotgun_1,
 tr_engine,
 tr_blade,
 tr_landmine,
 tr_telepad,
 tr_goo,
 tr_dissector,
 tr_sinkhole,
 tr_pit,
 tr_spike_pit,
 tr_lava,
 tr_portal,
 tr_ledge,
 tr_boobytrap,
 tr_temple_flood,
 tr_temple_toggle,
 tr_glow,
 tr_hum,
 tr_shadow,
 tr_drain,
 tr_snake,
 num_trap_types
};

struct trapfunc {
 static void none		(game *g, int x, int y) { };
 static void bubble		(game *g, int x, int y);
 static void beartrap		(game *g, int x, int y);
 static void snare		(game *g, int x, int y) { };
 static void board		(game *g, int x, int y);
 static void tripwire		(game *g, int x, int y);
 static void crossbow		(game *g, int x, int y);
 static void shotgun		(game *g, int x, int y);
 static void blade		(game *g, int x, int y);
 static void landmine		(game *g, int x, int y);
 static void telepad		(game *g, int x, int y);
 static void goo		(game *g, int x, int y);
 static void dissector		(game *g, int x, int y);
 static void sinkhole		(game *g, int x, int y);
 static void pit		(game *g, int x, int y);
 static void pit_spikes	(game *g, int x, int y);
 static void lava		(game *g, int x, int y);
 static void portal		(game *g, int x, int y) { };
 static void ledge		(game *g, int x, int y);
 static void boobytrap		(game *g, int x, int y);
 static void temple_flood	(game *g, int x, int y);
 static void temple_toggle	(game *g, int x, int y);
 static void glow		(game *g, int x, int y);
 static void hum		(game *g, int x, int y);
 static void shadow		(game *g, int x, int y);
 static void drain		(game *g, int x, int y);
 static void snake		(game *g, int x, int y);
};

struct trapfuncm {
 static void none	(game *g, monster *z) { };
 static void bubble	(game *g, monster *z);
 static void beartrap	(game *g, monster *z);
 static void board	(game *g, monster *z);
 static void tripwire	(game *g, monster *z);
 static void crossbow	(game *g, monster *z);
 static void shotgun	(game *g, monster *z);
 static void blade	(game *g, monster *z);
 static void snare	(game *g, monster *z) { };
 static void landmine	(game *g, monster *z);
 static void telepad	(game *g, monster *z);
 static void goo	(game *g, monster *z);
 static void dissector	(game *g, monster *z);
 static void sinkhole	(game *g, monster *z) { };
 static void pit	(game *g, monster *z);
 static void pit_spikes(game *g, monster *z);
 static void lava	(game *g, monster *z);
 static void portal	(game *g, monster *z) { };
 static void ledge	(game *g, monster *z);
 static void boobytrap (game *g, monster *z);
 static void glow	(game *g, monster *z);
 static void hum	(game *g, monster *z);
 static void drain	(game *g, monster *z);
 static void snake	(game *g, monster *z);
};

struct trap {
 static std::vector <trap*> traps;

 // don't worry about typedefs for trap actions

 int id;
 char sym;
 nc_color color;
 std::string name;
 
 int visibility;// 1 to ??, affects detection
 int avoidance;	// 0 to ??, affects avoidance
 int difficulty; // 0 to ??, difficulty of assembly & disassembly
 std::vector<itype_id> components;	// For disassembly?
 
 // \todo trap action for npcs
 void (*act)(game *, int x, int y);	// You stepped on it
 void (*actm)(game *, monster *);	// Monster stepped on it
 
 trap(int pid, char psym, nc_color pcolor, std::string pname,
      int pvisibility, int pavoidance, int pdifficulty, 
      void (*pact)(game *, int x, int y),
      void (*pactm)(game *, monster *), itype_id part = itm_null, itype_id part2 = itm_null, itype_id part3 = itm_null)
 : id(pid),sym(psym),color(pcolor),name(pname),visibility(pvisibility),avoidance(pavoidance),difficulty(pdifficulty),act(pact),actm(pactm)
 {
  if (part) components.push_back(part);
  if (part2) components.push_back(part2);
  if (part3) components.push_back(part3);
 };

 bool disarm_legal() const { return id != tr_null && difficulty < 99; };

 static void init();
};

#endif
