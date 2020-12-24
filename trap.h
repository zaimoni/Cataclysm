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
 tr_engine, // core for 3x3 blade trap
 tr_blade, // edges of 3x3 blade trap
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

DECLARE_JSON_ENUM_SUPPORT(trap_id)

struct trap {
 static std::vector<const trap*> traps;

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
#ifndef SOCRATES_DAIMON
 void (*act)(game *, int x, int y);	// You stepped on it (or failed a disarm)
 void (*actm)(game *, monster *);	// Monster stepped on it
#endif
 
 trap(int pid, std::string pname, char psym, nc_color pcolor,
      int pvisibility, int pavoidance, int pdifficulty, 
      std::initializer_list<itype_id> pcomponents
#ifndef SOCRATES_DAIMON
      , void (*pact)(game *, int x, int y),
      void (*pactm)(game *, monster *)
#endif
      )
 : id(pid),sym(psym),color(pcolor),name(pname),visibility(pvisibility),avoidance(pavoidance),difficulty(pdifficulty),components(pcomponents)
#ifndef SOCRATES_DAIMON
     ,act(pact),actm(pactm)
#endif
 {
 }

 bool disarm_legal() const { return id != tr_null && difficulty < 99; }

 static void init();
};

#endif
