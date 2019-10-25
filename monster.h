#ifndef _MONSTER_H_
#define _MONSTER_H_

#include "mtype.h"
#include "mobile.h"

class map;
class game;
class item;
class player;

// note partial overlap with disease effects for the player
enum monster_effect_type {
ME_NULL = 0,
ME_BEARTRAP,		// Stuck in beartrap
ME_POISONED,		// Slowed, takes damage
ME_ONFIRE,		// Lit aflame
ME_STUNNED,		// Stumbling briefly
ME_DOWNED,		// Knocked down
ME_BLIND,		// Can't use sight
ME_DEAF,		// Can't use hearing
ME_TARGETED,		// Targeting locked on--for robots that shoot guns
ME_DOCILE,		// Don't attack other monsters--for tame monster
ME_HIT_BY_PLAYER,	// We shot or hit them
ME_RUN,			// For hit-and-run monsters; we're running for a bit;
NUM_MONSTER_EFFECTS
};

DECLARE_JSON_ENUM_SUPPORT(monster_effect_type)

enum monster_attitude {
MATT_NULL = 0,
MATT_FRIEND,
MATT_FLEE,
MATT_IGNORE,
MATT_FOLLOW,
MATT_ATTACK,
NUM_MONSTER_ATTITUDES
};

struct monster_effect
{
 monster_effect_type type;
 int duration;
 monster_effect() = default;
 monster_effect(monster_effect_type T, int D) : type (T), duration (D) {}
 monster_effect(const monster_effect& src) = default;
 ~monster_effect() = default;
};

class monster {
 public:
 monster();
 monster(const mtype *t);
 monster(const mtype *t, int x, int y);
 monster(const monster& src) = default;
 monster(monster&& src) = default;
 ~monster() = default;
 monster& operator=(const monster& src);
 monster& operator=(monster&& src) = default;

 monster(std::istream& is);
 friend bool fromJSON(const cataclysm::JSON& _in, monster& dest);
 friend cataclysm::JSON toJSON(const monster& src);

 void poly(const mtype *t);
 void spawn(int x, int y); // All this does is moves the monster to x,y
 void spawn(const point& pt);

// Access
 std::string name() const; 		// Returns the monster's formal name
 std::string name_with_armor() const; // Name, with whatever our armor is called
 void print_info(const player& u, WINDOW* w) const; // Prints information to w.
 char symbol() const { return type->sym; };			// Just our type's symbol; no context
 void draw(WINDOW* w, const point& pl, bool inv) const;
 nc_color color_with_effects() const;	// Color with fire, beartrapped, etc.
				// Inverts color if inv==true
 bool has_flag(m_flag f) const { return type->has_flag(f); };
 bool can_see() const;		// MF_SEES and no ME_BLIND
 bool can_hear() const;		// MF_HEARS and no ME_DEAF
 bool made_of(material m) const;	// Returns true if it's made of m
 
 void debug(player &u); 	// Gives debug info

// Movement
 void receive_moves() { moves += speed; }		// Gives us movement points
 void shift(int sx, int sy); 	// Shifts the monster to the appropriate submap
			     	// Updates current pos AND our plans
 bool wander() const { return plans.empty(); } 		// Returns true if we have no plans
 bool can_move_to(const map &m, int x, int y) const; // Can we move to (x, y)?
 bool can_move_to(const map &m, const point& pt) const { return can_move_to(m, pt.x, pt.y); };
 bool will_reach(const game *g, const point& pt) const; // Do we have plans to get to (x, y)?
 int  turns_to_reach(const map& m, const point& pt) const; // How long will it take?

 void set_dest(const point& pt, int &t); // Go in a straight line to (x, y)
				      // t determines WHICH Bresenham line
 void wander_to(int x, int y, int f); // Try to get to (x, y), we don't know
				      // the route.  Give up after f steps.
 void plan(game *g);
 void move(game *g); // Actual movement
 void footsteps(game *g, const point& pt); // noise made by movement
 void friendly_move(game *g);

 point scent_move(const game *g);	// these two don't actually move, they return where to move to
 point sound_move(const game *g);
 void hit_player(game *g, player &p, bool can_grab = true);
 void move_to(game *g, const point& pt);
 void stumble(game *g, bool moved);
 void knock_back_from(game *g, int posx, int posy);
 void knock_back_from(game *g, const point& pt) { knock_back_from(g, pt.x, pt.y); };

// Combat
 bool is_fleeing(const player &u) const;	// True if we're fleeing
 monster_attitude attitude(const player *u = NULL) const;	// See the enum above
// int morale_level(player &u);	// Looks at our HP etc.
 void process_triggers(const game *g);// Process things that anger/scare us
 void process_trigger(monster_trigger trig, int amount);// Single trigger
 int trigger_sum(const game *g, const std::vector<monster_trigger>& triggers) const;
 int  hit(game *g, player &p, body_part &bp_hit); // Returns a damage
 void hit_monster(game *g, monster& target);
 bool hurt(int dam); 	// Deals this dam damage; returns true if we dead
 int  armor_cut() const;	// Natural armor, plus any worn armor
 int  armor_bash() const;	// Natural armor, plus any worn armor
 int  dodge() const;		// Natural dodge, or 0 if we're occupied
 int  dodge_roll() const;	// For the purposes of comparing to player::hit_roll()
 int  fall_damage() const;	// How much a fall hurts us
 void die(game *g);

// Other
 void add_effect(monster_effect_type effect, int duration);
 bool has_effect(monster_effect_type effect) const; // True if we have the effect
 void rem_effect(monster_effect_type effect); // Remove a given effect
 void process_effects();	// Process long-term effects
 bool make_fungus();	// Makes this monster into a fungus version
				// Returns false if no such monster exists
 void make_friendly();
 void add_item(const item& it);	// Add an item to inventory

 bool is_static_spawn() const { return -1 != spawnmap.x; }

// TEMP VALUES
 point pos;
 countdown<point> wand;	// Wander destination - Just try to move in that direction.
 std::vector<item> inv; // Inventory
 std::vector<monster_effect> effects; // Active effects, e.g. on fire

// If we were spawned by the map, store our origin for later use
 point spawnmap;	// game::lev-based source; z coordinate lost
 point spawnpos;  // normal map position

// DEFINING VALUES
 int moves, speed;
 int hp;
 int sp_timeout;
 int friendly;	// -1: indefinitely friendly; positive: times out
 int anger, morale;
 int faction_id; // If we belong to a faction
 int mission_id; // If we're related to a mission
 const mtype *type;
 bool dead;
 bool made_footstep;
 std::string unique_name; // If we're unique

private:
 std::vector <point> plans;

 bool can_sound_move_to(const game* g, const point& pt) const;
 bool can_sound_move_to(const game* g, const point& pt, point& dest) const;
};

#endif
