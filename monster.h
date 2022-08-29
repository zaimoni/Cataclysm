#ifndef _MONSTER_H_
#define _MONSTER_H_

#include "mtype.h"
#include "bodypart.h"
#include "mobile.h"
#include "zero.h"
#include <functional>

class map;
class game;
class item;
class player;
class pc;

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
 monster_effect(monster_effect_type T, int D) noexcept : type (T), duration (D) {}
 monster_effect(const monster_effect& src) = default;
 ~monster_effect() = default;
};

class monster : public mobile {
 public:
 monster() noexcept;
 monster(const mtype *t) noexcept;
 monster(const mtype* t, point origin) noexcept;
 monster(const mtype* t, GPS_loc origin) noexcept;
 monster(const mtype* t, int x, int y) noexcept : monster(t, point(x, y)) {}
 monster(const monster& src) = default;
 monster(monster&& src) = default;
 ~monster() = default;
 monster& operator=(const monster& src);
 monster& operator=(monster&& src) = default;

 friend bool fromJSON(const cataclysm::JSON& _in, monster& dest);
 friend cataclysm::JSON toJSON(const monster& src);

 void poly(const mtype *t);
 void spawn(int x, int y); // All this does is moves the monster to x,y
 void spawn(const point& pt);
 void spawn(const GPS_loc& loc);

// Access
 std::string name() const; 		// Returns the monster's formal name
 std::string name_with_armor() const; // Name, with whatever our armor is called
 void print_info(const player& u, WINDOW* w) const; // Prints information to w.
 char symbol() const { return type->sym; };			// Just our type's symbol; no context
 void draw(WINDOW* w, const point& pl, bool inv) const; // Inverts color if inv==true
 nc_color color_with_effects() const;	// Color with fire, beartrapped, etc.
 bool has_flag(m_flag f) const { return type->has_flag(f); };
 bool can_see() const;		// MF_SEES and no ME_BLIND
 bool can_hear() const;		// MF_HEARS and no ME_DEAF
 bool made_of(material m) const;	// Returns true if it's made of m
 bool ignitable() const;    // legal to apply ME_ONFIRE to
 std::optional<int> see(const player& u) const;

 void debug(player &u); 	// Gives debug info

// Movement
 void receive_moves() { moves += speed; }		// Gives us movement points
 void shift(const point& delta); 	// Shifts the monster to the appropriate submap
			     	// Updates current pos AND our plans
 bool wander() const { return plans.empty(); } 		// Returns true if we have no plans
 bool can_move_to(const map &m, int x, int y) const; // Can we move to (x, y)?
 bool can_move_to(const map &m, const point& pt) const { return can_move_to(m, pt.x, pt.y); }
 bool can_enter(const map& m, const point& pt) const;
 bool will_reach(const game *g, const point& pt) const; // Do we have plans to get to (x, y)?
 int  turns_to_reach(const map& m, const point& pt) const; // How long will it take?

 void set_dest(const point& pt, int t); // Go in a straight line to (x, y)
				      // t determines WHICH Bresenham line
 void wander_to(const point& pt, int f); // Try to get to (x, y), we don't know
				      // the route.  Give up after f steps.
 void plan(game *g);
 void move(game *g); // Actual movement
 void footsteps(game *g, const point& pt); // noise made by movement
 void friendly_move(game *g);

 std::optional<point> scent_move();	// these two don't actually move, they return where to move to
 point sound_move();
 void hit_player(game *g, player &p, bool can_grab = true);
 void move_to(game *g, const point& pt);
 void stumble(game *g, bool moved);

// Combat
 bool is_fleeing(const player &u) const;	// True if we're fleeing
 monster_attitude attitude(const player* u = nullptr) const;	// See the enum above
// int morale_level(player &u);	// Looks at our HP etc.
 void process_triggers(const game *g);// Process things that anger/scare us
 void process_trigger(monster_trigger trig, int amount);// Single trigger
 int trigger_sum(const game* g, typename cataclysm::bitmap<N_MONSTER_TRIGGERS>::type triggers) const;
 int  hit(game *g, player &p, body_part &bp_hit); // Returns a damage
 void hit_monster(monster& target) const;
 bool hit_by_blob(monster* origin = nullptr, bool force = true);
 bool hurt(int dam) override; 	// Deals this dam damage; returns true if we dead
 bool hitall(int dam, int vary = 0) override { return hurt(dam); } // as required by C:Whales behavior of monster|player::fling
 int  armor_cut() const;	// Natural armor, plus any worn armor
 int  armor_bash() const;	// Natural armor, plus any worn armor
 int  dodge() const;		// Natural dodge, or 0 if we're occupied
 int  dodge_roll() const override; // For the purposes of comparing to player::hit_roll()
 int  melee_skill() const override { return type->melee_skill; }
 int min_technique_power() const override { return type->size; }
 int  fall_damage() const;	// How much a fall hurts us
 void killed(pc* by = nullptr);
 void fling(int dir, int flvel); // thrown by impact (or something)

// Other
 int knockback_size() const override { return type->size; }

 void add(effect src, int duration) override;
 bool has(effect src) const override;
 void add_effect(monster_effect_type effect, int duration);
 bool has_effect(monster_effect_type effect) const; // True if we have the effect
 void rem_effect(monster_effect_type effect); // Remove a given effect
 void process_effects();	// Process long-term effects
 bool make_fungus();	// Makes this monster into a fungus version
				// Returns false if no such monster exists
 void make_friendly();
 void make_friendly(const player& u);
 void make_ally(const player& u);
 void make_threat(const player& u);
 void add_item(const item& it);	// Add an item to inventory
 bool is_enemy(const player* survivor = nullptr) const;
 bool is_friend(const player* survivor = nullptr) const;
 bool is_enemy(const monster* z) const;
 bool is_friend(const monster* z) const;

 bool is_static_spawn() const { return was_spawned; }
 void am_static_spawned(const Badge<map>& auth) { was_spawned = true; }

 bool if_visible_message(std::function<std::string()> other) const;
 bool if_visible_message(const char* msg) const;

 // grammatical support
 std::string subject() const override;
 std::string direct_object() const override;
 std::string indirect_object() const override;
 std::string possessive() const override;

 // integrity checks
 void screenpos_set(point pt);
 void screenpos_set(int x, int y);
 void screenpos_add(point delta);

 // adapter for std::visit
 struct is_enemy_of {
     const monster& z;

     is_enemy_of(const monster& z) noexcept : z(z) {}
     is_enemy_of(const is_enemy_of& src) = delete;
     is_enemy_of(is_enemy_of&& src) = delete;
     is_enemy_of& operator=(const is_enemy_of& src) = delete;
     is_enemy_of& operator=(is_enemy_of&& src) = delete;
     ~is_enemy_of() = default;

     auto operator()(const monster* target) const { return z.is_enemy(target); }
     auto operator()(const player* target) const { return z.is_enemy(target); }
 };

// TEMP VALUES
 point pos;
 countdown<point> wand;	// Wander destination - Just try to move in that direction.
 std::vector<item> inv; // Inventory
 std::vector<monster_effect> effects; // Active effects, e.g. on fire

// DEFINING VALUES
 int speed;
 int hp;
 int sp_timeout;
 int friendly;	// -1: indefinitely friendly; positive: times out.  Hard-coded to PC
 int anger, morale;
 int faction_id; // If we belong to a faction
 int mission_id; // If we're related to a mission
 const mtype *type;
 bool dead;
 bool made_footstep;
 std::string unique_name; // If we're unique

private:
 bool was_spawned; // by map
 std::vector <point> plans;

 bool can_sound_move_to(const point& pt) const;

 void _set_screenpos() override { if (auto pt = screen_pos()) pos = *pt; }
 bool handle_knockback_into_impassable(const GPS_loc& dest) override;

 void make_friendly(int duration);

 void die(); // \todo? inline into sole caller
};

template<m_flag f> bool is(const monster* z) { return z && z->has_flag(f); }
template<m_flag f> bool is_not(const monster* z) { return z && !z->has_flag(f); }

#endif
