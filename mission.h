#ifndef _MISSION_H_
#define _MISSION_H_

#include "itype.h"
#include "npc.h"

enum mission_id {
 MISSION_NULL,
 MISSION_GET_ANTIBIOTICS,
 MISSION_GET_SOFTWARE,
 MISSION_GET_ZOMBIE_BLOOD_ANAL,
 MISSION_RESCUE_DOG,
 MISSION_KILL_ZOMBIE_MOM,
 MISSION_REACH_SAFETY,
 NUM_MISSION_IDS
};

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
 std::string name;	// The name the mission is given in menus
 mission_goal goal;	// The basic goal type
 int difficulty;	// Difficulty; TODO: come up with a scale
 int value;		// Value; determines rewards and such
 npc_favor special_reward; // If we have a special gift, not cash value
 int deadline_low, deadline_high; // Low and high deadlines (turn numbers)
 bool urgent;	// If true, the NPC will press this mission!

 std::vector<mission_origin> origins;	// Points of origin
 itype_id item_id;
 mission_id follow_up;

 bool (*place)(game *g, int x, int y);
 void (*start)(game *g, mission *);
 void (*end  )(game *g, mission *);
 void (*fail )(game *g, mission *);

 mission_type(int ID, std::string NAME, mission_goal GOAL, int DIF, int VAL,
              bool URGENT,
              bool (*PLACE)(game *, int x, int y),
              void (*START)(game *, mission *),
              void (*END  )(game *, mission *),
              void (*FAIL )(game *, mission *)) :
  id (ID), name (NAME), goal (GOAL), difficulty (DIF), value (VAL),
  urgent(URGENT), place (PLACE), start (START), end (END), fail (FAIL)
  {
   deadline_low = 0;
   deadline_high = 0;
   item_id = itm_null;
   follow_up = MISSION_NULL;
  };

 mission create(game *g, int npc_id = -1); // Create a mission

 static void init();
};

struct mission {
 const mission_type* type;
 std::string description; // Basic descriptive text
 bool failed;		// True if we've failed it!
 int value;		// Cash/Favor value of completing this
 npc_favor reward;	// If there's a special reward for completing it
 int uid;		// Unique ID number, used for referencing elsewhere
 point target;		// Marked on the player's map.  (-1,-1) for none
 itype_id item_id;	// Item that needs to be found (or whatever)
 int count;		// How many of that item
 int deadline;		// Turn number
 int npc_id;		// ID of a related npc
 int good_fac_id, bad_fac_id;	// IDs of the protagonist/antagonist factions
 int step;		// How much have we completed?
 mission_id follow_up;	// What mission do we get after this succeeds?
 
 std::string name() const;

 mission()
 {
  type = NULL;
  description = "";
  failed = false;
  value = 0;
  uid = -1;
  target = point(-1, -1);
  item_id = itm_null;
  count = 0;
  deadline = 0;
  npc_id = -1;
  good_fac_id = -1;
  bad_fac_id = -1;
  step = 0;
 }

 mission(std::istream& src);
 friend std::ostream& operator<<(std::ostream& os, const mission& src);
};

#endif
