#ifndef _EVENT_H_
#define _EVENT_H_

#include "enums.h"
#include "enum_json.h"
#include "zero.h"
#include <vector>

class game;

enum event_type {
 EVENT_NULL,
 EVENT_HELP,
 // \todo these two up for re-implementation: arbitrary target
 EVENT_WANTED,
 EVENT_ROBOT_ATTACK,
 EVENT_SPAWN_WYRMS,
 EVENT_AMIGARA,
 EVENT_ROOTS_DIE,
 EVENT_TEMPLE_OPEN,
 EVENT_TEMPLE_FLOOD,
 EVENT_TEMPLE_SPAWN,
 EVENT_DIM,
 EVENT_ARTIFACT_LIGHT,
 NUM_EVENT_TYPES
};

DECLARE_JSON_ENUM_SUPPORT(event_type)

class event {
	static std::vector<event> _events;

public:
 event_type type;
 int turn;
 // core data above; event-specific data below
 int faction_id;	// -1 (no faction) is legal
 point map_point;   // usage is against game::lev.x,y

 event(event_type e_t=EVENT_NULL, int t=0, int f_id= -1, int x= -1, int y = -1)
 : type(e_t),turn(t),faction_id(f_id),map_point(-1,-1) {}

 void actualize() const; // When the time runs out
 bool per_turn();  // Every turn.  Return false to request self-deletion i.e. no longer relevant

 static int are_queued() { return _events.size(); }
 static const event* queued(event_type type);
 static void add(const event& src) { _events.push_back(src); }
 static void add(event&& src) { _events.push_back(std::move(src)); }
 static void process(const Badge<game>& badge);

 // save/load support
 static void global_reset(const Badge<game>& badge);
 static void global_fromJSON(const cataclysm::JSON& src);
 static void global_toJSON(cataclysm::JSON& dest);
};

#endif
