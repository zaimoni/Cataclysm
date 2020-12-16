#ifndef _MISSION_H_
#define _MISSION_H_

#include "itype.h"
#include "npc.h"

enum mission_id {
 MISSION_NULL = 0,
 MISSION_GET_ANTIBIOTICS,
 MISSION_GET_SOFTWARE,
 MISSION_GET_ZOMBIE_BLOOD_ANAL,
 MISSION_RESCUE_DOG,
 MISSION_KILL_ZOMBIE_MOM,
 MISSION_REACH_SAFETY,
 MISSION_GET_BOOK,
 NUM_MISSION_IDS
};

DECLARE_JSON_ENUM_SUPPORT(mission_id)

std::string mission_dialogue(mission_id id, talk_topic state);

enum mission_origin {
 ORIGIN_NULL = 0,
 ORIGIN_GAME_START,	// Given when the game starts
 ORIGIN_OPENER_NPC,	// NPC comes up to you when the game starts
 ORIGIN_ANY_NPC,	// Any NPC
 ORIGIN_SECONDARY,	// Given at the end of another mission
 NUM_ORIGIN
};

enum mission_goal {
 MGOAL_NULL = 0,
 MGOAL_GO_TO,		// Reach a certain overmap tile
 MGOAL_FIND_ITEM,	// Find an item of a given type
 MGOAL_FIND_ANY_ITEM,	// Find an item tagged with this mission
 MGOAL_FIND_MONSTER,	// Find and retrieve a friendly monster
 MGOAL_FIND_NPC,	// Find a given NPC
 MGOAL_ASSASSINATE,	// Kill a given NPC
 MGOAL_KILL_MONSTER,	// Kill a particular hostile monster
 NUM_MGOAL
};

struct mission_type {
 static std::vector <mission_type> types; // The list of mission templates

 int id;		// Matches it to a mission_id above
 const char* name;	// The name the mission is given in menus
 mission_goal goal;	// The basic goal type
 int difficulty;	// Difficulty; TODO: come up with a scale
 int value;		// Value; determines rewards and such
 npc_favor special_reward; // If we have a special gift, not cash value
 int deadline_low, deadline_high; // Low and high deadlines (turn numbers)
 bool urgent;	// If true, the NPC will press this mission!

 std::vector<mission_origin> origins;	// Points of origin
 itype_id item_id;
 mission_id follow_up;

 bool (*place)(game *g, int x, int y, int npc_id);
 void (*start)(game *g, mission *);
 void (*end  )(game *g, mission *);
 void (*fail )(game *g, mission *);

 mission_type(int id, const char* name, mission_goal goal, int difficulty, int value,
              bool urgent,
              bool (*place)(game *, int x, int y, int npc_id),
              void (*start)(game *, mission *),
              void (*end)(game *, mission *),
              void (*fail)(game *, mission *),
              std::initializer_list<mission_origin> orgns = {}, itype_id item_id = itm_null,
              // omitting deadline_low/high makes the mission never time out
              int deadline_low = 0, int deadline_high = 0) :
  id(id), name(name), goal(goal), difficulty(difficulty), value(value),
  deadline_low(deadline_low), deadline_high(deadline_high), urgent(urgent), origins(orgns),
  item_id(item_id), follow_up(MISSION_NULL), place(place), start(start), end(end), fail(fail) {}

 mission create(game *g, int npc_id = -1); // Create a mission

 static void init();
};

struct mission {
#undef MIN_ID
 enum {
   MIN_ID = 1
 };

 const mission_type* type;
 std::string description; // Basic descriptive text
 bool failed;		// True if we've failed it!
 int value;		// Cash/Favor value of completing this
 npc_favor reward;	// If there's a special reward for completing it
 int uid;		// Unique ID number, used for referencing elsewhere
 OM_loc target;		// Marked on the player's map.
 itype_id item_id;	// Item that needs to be found (or whatever)
 int count;		// How many of that item
 int deadline;		// Turn number
 int npc_id;		// ID of a related npc
 int good_fac_id, bad_fac_id;	// IDs of the protagonist/antagonist factions
 int step;		// How much have we completed?
 mission_id follow_up;	// What mission do we get after this succeeds?
 
 const char* name() const { return type ? type->name : "NULL"; }

 mission() noexcept
 : type(nullptr),failed(false),value(0),uid(-1),target(_ref<decltype(target)>::invalid),
   item_id(itm_null),count(0),deadline(0),npc_id(-1),good_fac_id(-1),
   bad_fac_id(-1),step(0),follow_up(MISSION_NULL) {}

 friend bool fromJSON(const cataclysm::JSON& _in, mission& dest);
 friend cataclysm::JSON toJSON(const mission& src);

 void step_complete(int stage); // Partial completion
 bool is_complete(const player& u, int npc_id) const;
 void fail();

 static mission* from_id(int uid);	// only active missions
 void assign_id() { uid = next_id++; }

 static void global_reset();
 static void global_fromJSON(const cataclysm::JSON& src);
 static void global_toJSON(cataclysm::JSON& dest);
private:
 static int next_id;   // mininum/default next mission id
};


#endif
