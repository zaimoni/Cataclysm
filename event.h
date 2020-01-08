#ifndef _EVENT_H_
#define _EVENT_H_

#include "enums.h"

class game;

enum event_type {
 EVENT_NULL,
 EVENT_HELP,
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

struct event {
 event_type type;
 int turn;
 int faction_id;	// -1 (no faction) is legal
 point map_point;

 event(event_type e_t=EVENT_NULL, int t=0, int f_id= -1, int x= -1, int y = -1)
 : type(e_t),turn(t),faction_id(f_id),map_point(-1,-1) {}

 void actualize(game *g) const; // When the time runs out
 void per_turn(game *g);  // Every turn
};

#endif
