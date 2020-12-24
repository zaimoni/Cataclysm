#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "skill.h"
#include "bionics.h"
#include "morale.h"
#include "inventory.h"
#include "mutation.h"
#include "mobile.h"
#include "bodypart.h"
#include "pldata.h"
#include "zero.h"

enum art_effect_passive;
class game;
struct mission;
class monster;
struct trap;

struct special_attack
{
 std::string text;
 int bash;
 int cut;
 int stab;

 special_attack() noexcept : bash(0), cut(0), stab(0) { };
 special_attack(std::string&& src) noexcept : text(std::move(src)), bash(0), cut(0), stab(0) { };
};

std::string random_first_name(bool male);
std::string random_last_name();

class player : public mobile {
public:
 player();
 player(const cataclysm::JSON& src);
 player(const player &rhs) = default;
 player(player&& rhs) = default;
 virtual ~player() = default;
 friend cataclysm::JSON toJSON(const player& src);

 player& operator=(const player& rhs);
 player& operator=(player&& rhs) = default;

// newcharacter.cpp 
 bool create(game *g, character_type type, std::string tempname = "");
 void normalize();	// Starting set up of HP
// </newcharacter.cpp>

 void pick_name(); // Picks a name from NAMES_*

 virtual bool is_npc() const { return false; }	// Overloaded for NPCs in npc.h
 nc_color color() const;				// What color to draw us as
 std::pair<int, nc_color> hp_color(hp_part part) const;

 void disp_info(game *g);	// '@' key; extended character info
 void disp_morale();		// '%' key; morale info
 void disp_status(WINDOW* w, game *g);// On-screen data

 void reset(game* g = nullptr);// Resets movement points, stats, applies effects
 void update_morale();	// Ticks down morale counters and removes them
 int  current_speed(game* g = nullptr) const; // Number of movement points we get a turn
 int  run_cost(int base_cost); // Adjust base_cost
 int  swim_speed();	// Our speed when swimming

 int is_cold_blooded() const;
 bool has_trait(int flag) const;
 bool has_mutation(int flag) const;
 void toggle_trait(int flag);

 bool has_bionic(bionic_id b) const;
 bool has_active_bionic(bionic_id b) const;
 void add_bionic(bionic_id b);
 void charge_power(int amount);
 void power_bionics(game *g);
 void activate_bionic(int b, game *g);	// \todo V 0.2.1 extend to NPCs

 void mutate();
 void mutate_towards(pl_flag mut);
 void remove_mutation(pl_flag mut);
 bool has_child_flag(pl_flag mut) const;
 void remove_child_flag(pl_flag mut);

 int  sight_range(int light_level) const;
 int  overmap_sight_range(int light_level) const;
 int  clairvoyance() const; // Sight through walls &c
 bool has_two_arms() const;
// bool can_wear_boots();	// no definition
 bool is_armed() const;	// True if we're wielding something; true for bionics
 bool unarmed_attack() const; // False if we're wielding something; true for bionics
 bool avoid_trap(const trap *tr) const;

 void pause(); // '.' command; pauses & reduces recoil

 // npcmove.cpp (inherited location)
 // Physical movement from one tile to the next
 bool can_move_to(const map& m, const point& pt) const;
 bool can_enter(const GPS_loc& _GPSpos) const;
 bool landing_zone_ok();   // returns true if final location is deemed ok; that is, no-op is true
 bool move_away_from(const map& m, const point& tar, point& dest) const;
 virtual int can_reload() const; // Wielding a gun that is not fully loaded; return value is inventory index, or -1

 // melee.cpp
 int  hit_mon(game *g, monster *z, bool allow_grab = true);
 void hit_player(game *g, player &p, bool allow_grab = true);

 int base_damage(bool real_life = true, int stat = -999) const;
 int base_to_hit(bool real_life = true, int stat = -999) const;

 technique_id pick_defensive_technique(game *g, const monster *z, player *p);

 void perform_defensive_technique(technique_id technique, game *g, monster *z,
                                  player *p, body_part &bp_hit, int &side,
                                  int &bash_dam, int &cut_dam, int &stab_dam);

 int dodge(); // Returns the players's dodge, modded by clothing etc
 int  dodge_roll();// For comparison to hit_roll()

// ranged.cpp (at some point, historically)
 int throw_range(int index) const; // Range of throwing item; -1:ERR 0:Can't throw
 int ranged_dex_mod	(bool real_life = true) const;
 int ranged_per_mod	(bool real_life = true) const;
 int throw_dex_mod	(bool real_life = true) const;

// Mental skills and stats
 int comprehension_percent(skill s, bool real_life = true) const;
 int read_speed		(bool real_life = true) const;
 int talk_skill() const; // Skill at convincing NPCs of stuff
 int intimidation() const; // Physical intimidation

// Converts bphurt to a hp_part (if side == 0, the left), then does/heals dam
// hit() processes damage through armor
 void hit   (game *g, body_part bphurt, int side, int  dam, int  cut);	// \todo V 0.2.1 enable for NPCs?
// hurt() doesn't--effects of disease, what have you
 void hurt  (game *g, body_part bphurt, int side, int  dam);	// \todo V 0.2.1 enable for NPCs?

 void heal(body_part healed, int side, int dam);	// dead function?
 void heal(hp_part healed, int dam);
 void healall(int dam);
 void hurtall(int dam);
 // checks armor. if vary > 0, then damage to parts are random within 'vary' percent (1-100)
 void hitall(game *g, int dam, int vary = 0);
// Sends us flying one tile
 void knock_back_from(game *g, int x, int y);
 void knock_back_from(game *g, const point& pt) { knock_back_from(g, pt.x, pt.y); };

 int hp_percentage() const;	// % of HP remaining, overall
 std::pair<hp_part, int> worst_injury() const;
 std::pair<itype_id, int> would_heal(const std::pair<hp_part, int>& injury) const;

 void get_sick();	// Process diseases	\todo V 0.2.1 enable for NPCs
// infect() gives us a chance to save (mostly from armor)
 void infect(dis_type type, body_part vector, int strength, int duration);
// add_disease() does NOT give us a chance to save
 void add_disease(dis_type type, int duration, int intensity = 0, int max_intensity = -1);
 void rem_disease(dis_type type);
 bool has_disease(dis_type type) const;
 int  disease_level(dis_type type) const;
 int  disease_intensity(dis_type type) const;

 void add_addiction(add_type type, int strength);
#if DEAD_FUNC
 void rem_addiction(add_type type);
#endif
 bool has_addiction(add_type type) const;
 int  addiction_level(add_type type) const;

 void die(map& m);
 void suffer(game *g);	// \todo V 0.2.1 extend fully to NPCs
 void vomit();	// \todo V 0.2.1 extend to NPCs
 
 int  lookup_item(char let) const;
 bool eat(int index);	// Eat item; returns false on fail
 virtual bool wield(int index);// Wield item; returns false on fail
 void pick_style(); // Pick a style
 bool wear(char let);	// Wear item; returns false on fail
 bool wear_item(const item& to_wear);
 const it_armor* wear_is_performable(const item& to_wear) const;
 bool takeoff(map& m, char let);// Take off item; returns false on fail	\todo V 0.2.1 extend to NPC? (this is UI-driven so maybe not)
 void use(game *g, char let);	// Use a tool \todo V 0.2.1 extend to NPCs? (UI-driven)
 bool install_bionics(game *g, const it_bionic* type);	// Install bionics \todo V 0.2.1 enable for NPCs
 void read(game *g, char let);	// Read a book	V 0.2.1 \todo enable for NPCs? (UI driven)
 void try_to_sleep();	// '$' command; adds DIS_LYING_DOWN	\todo V 0.2.1 extend to NPCs
 bool can_sleep(const map& m) const;	// Checked each turn during DIS_LYING_DOWN

 int warmth(body_part bp) const;	// Warmth provided by armor &c; \todo cf game::check_warmth which might belong over in player
 int encumb(body_part bp) const;	// Encumberance from armor &c
 int armor_bash(body_part bp) const;	// Bashing resistance
 int armor_cut(body_part bp) const;	// Cutting  resistance
 int resist(body_part bp) const;	// Infection &c resistance
 bool wearing_something_on(body_part bp) const; // True if wearing something on bp

 void practice(skill s, int amount);	// Practice a skill

 void assign_activity(activity_type type, int moves, int index = -1);	// \todo V 0.2.5+ extend to NPCs
 void assign_activity(activity_type type, int moves, const point& pt, int index = -1);
 void cancel_activity();
 virtual void cancel_activity_query(const char* message, ...);

 void accept(mission* miss); // Just assign an existing mission
 void fail(const mission& miss); // move to failed list, if relevant
 void wrap_up(mission* miss);

 int weight_carried() const;
 int volume_carried() const;
 int weight_capacity(bool real_life = true) const;
 int volume_capacity() const;
 int morale_level() const;	// Modified by traits, &c
 void add_morale(morale_type type, int bonus, int max_bonus = 0, const itype* item_type = nullptr);
 int get_morale(morale_type type, const itype* item_type = nullptr) const;

 void sort_inv();	// Sort inventory by type
 std::string weapname(bool charges = true) const;

 void i_add(item it);
 bool has_active_item(itype_id id) const;
 int  active_item_charges(itype_id id) const;
 void process_active_items(game *g);
 item i_rem(char let);	// Remove item from inventory; returns ret_null on fail
 item i_rem(itype_id type);// Remove first item w/ this type; fail is ret_null
 void remove_weapon();
 item unwield();    // like remove_weapon, but returns what was unwielded
 void remove_mission_items(int mission_id);
 item i_remn(int index);// Remove item from inventory; returns ret_null on fail
 item& i_at(char let);	// Returns the item with inventory letter let
 const item& i_at(char let) const { return const_cast<player*>(this)->i_at(let); };
 item& i_of_type(itype_id type); // Returns the first item with this type
 const item& i_of_type(itype_id type) const { return const_cast<player*>(this)->i_of_type(type); };
 std::vector<item> inv_dump() const; // Inventory + weapon + worn (for death, etc)
 bool remove_item(item* it);
 int  butcher_factor() const;	// Automatically picks our best butchering tool
 item* pick_usb(); // Pick a usb drive, interactively if it matters
 bool is_wearing(itype_id it) const;	// Are we wearing a specific itype?
 bool has_artifact_with(art_effect_passive effect) const;

// has_amount works ONLY for quantity.
// has_charges works ONLY for charges.
 void use_amount(itype_id it, int quantity, bool use_container = false);
 void use_charges(itype_id it, int quantity);// Uses up charges
 bool has_amount(itype_id it, int quantity) const;
 bool has_charges(itype_id it, int quantity) const;
 int  amount_of(itype_id it) const;
 int  charges_of(itype_id it) const;

 bool has_watertight_container() const;
 bool has_weapon_or_armor(char let) const;	// Has an item with invlet let
 bool has_item(char let) const;		// Has an item with invlet let
 bool has_item(item *it) const;		// Has a specific item
 bool has_mission_item(int mission_id) const;	// Has item with mission_id
 bool has_ammo(ammotype at) const;// Returns a list of indices of the ammo
 std::vector<int> have_ammo(ammotype at) const;// Returns a list of indices of the ammo
 std::pair<int, item*> have_item(char let);
 std::pair<int, const item*> have_item(char let) const;
 item* decode_item_index(int n);
 const item* decode_item_index(int n) const;

// abstract ui
 virtual bool see_phantasm();  // would not be const for multi-PC case

// integrity checks
 void screenpos_set(point pt);
 void screenpos_set(int x, int y);
 void screenpos_add(point delta);

// ---------------VALUES-----------------
 point pos;
 bool in_vehicle;       // Means player sit inside vehicle on the tile he is now
 player_activity activity;
 player_activity backlog;
// _missions vectors are of mission IDs
 std::vector<int> active_missions;
 std::vector<int> completed_missions;
 std::vector<int> failed_missions;
 int active_mission;	// index into active_missions vector, above
 
 std::string name;
 bool male;
 bool my_traits[PF_MAX2];
 bool my_mutations[PF_MAX2];
 int mutation_category_level[NUM_MUTATION_CATEGORIES];
 std::vector<bionic> my_bionics;
// Current--i.e. modified by disease, pain, etc.
 int str_cur, dex_cur, int_cur, per_cur;
// Maximum--i.e. unmodified by disease
 int str_max, dex_max, int_max, per_max;
 int power_level, max_power_level;
 int hunger, thirst, fatigue, health;
 bool underwater;
 int oxygen;
 unsigned int recoil;
 unsigned int driving_recoil;
 unsigned int scent;
 int dodges_left, blocks_left;
 int stim, pain, pkill, radiation;
 int cash;
 int hp_cur[num_hp_parts], hp_max[num_hp_parts];

 std::vector<morale_point> morale;

 int xp_pool;
 int sklevel   [num_skill_types];
 int skexercise[num_skill_types];
 
 bool inv_sorted;	// V 0.2.1+ use or eliminate, this appears to be a no-op tracer
 inventory inv;
 itype_id last_item;
 std::vector <item> worn;	// invariant: dynamic cast to it_armor is non-null
 std::vector<itype_id> styles;
 itype_id style_selected;
 item weapon;
 
 std::vector <disease> illness;
 std::vector <addiction> addictions;

private:
 // melee.cpp
 int  hit_roll() const; // Our basic hit roll, compared to our target's dodge roll
 bool scored_crit(int target_dodge = 0) const; // Critical hit?

 // V 0.2.1 \todo player variants of these so we can avoid null pointers as signal for player; alternately, unify API
 int roll_bash_damage(const monster *z, bool crit) const;
 int roll_cut_damage(const monster *z, bool crit) const;
 int roll_stab_damage(const monster *z, bool crit) const;
 int roll_stuck_penalty(const monster *z, bool stabbing) const;

 technique_id pick_technique(const game *g, const monster *z, const player *p, bool crit, bool allowgrab) const;
 void perform_technique(technique_id technique, game *g, monster *z, player *p,
                       int &bash_dam, int &cut_dam, int &pierce_dam, int &pain);

 void perform_special_attacks(game *g, monster *z, player *p, int &bash_dam, int &cut_dam, int &pierce_dam);	// V 0.2.1 \todo enable for NPCs
 std::vector<special_attack> mutation_attacks(const monster *z, const player *p) const;	// V 0.2.1 \todo enable for NPCs

 void melee_special_effects(game *g, monster *z, player *p, bool crit, int &bash_dam, int &cut_dam, int &stab_dam);

 // player.cpp
// absorb() reduces dam and cut by your armor (and bionics, traits, etc)
 void absorb(game *g, body_part bp, int &dam, int &cut);	// \todo V 0.2.1 enable for NPCs?
};

inventory crafting_inventory(const map& m, const player& u);

#endif
