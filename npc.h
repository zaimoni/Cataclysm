#ifndef _NPC_H_
#define _NPC_H_

#include "player.h"
#include "omdata.h"
#include "faction.h"
#include <string>
#include <memory>

namespace cataclysm {
	class action;
}

/*
 * Talk:   Trust midlow->high, fear low->mid, need doesn't matter
 * Trade:  Trust mid->high, fear low->midlow, need is a bonus
 * Follow: Trust high, fear mid->high, need low->mid
 * Defend: Trust mid->high, fear + need high
 * Kill:   Trust low->midlow, fear low->midlow, need low
 * Flee:   Trust low, fear mid->high, need low
 */

// Attitude is how we feel about the player, what we do around them
enum npc_attitude {
 NPCATT_NULL = 0,	// Don't care/ignoring player
 NPCATT_TALK,		// Move to and talk to player
 NPCATT_TRADE,		// Move to and trade with player
 NPCATT_FOLLOW,		// Follow the player
 NPCATT_FOLLOW_RUN,	// Follow the player, don't shoot monsters
 NPCATT_LEAD,		// Lead the player, wait for them if they're behind
 NPCATT_WAIT,		// Waiting for the player
 NPCATT_DEFEND,		// Kill monsters that threaten the player
 NPCATT_MUG,		// Mug the player
 NPCATT_WAIT_FOR_LEAVE,	// Attack the player if our patience runs out
 NPCATT_KILL,		// Kill the player
 NPCATT_FLEE,		// Get away from the player
 NPCATT_SLAVE,		// Following the player under duress
 NPCATT_HEAL,		// Get to the player and heal them

 NPCATT_MISSING,	// Special; missing NPC as part of mission
 NPCATT_KIDNAPPED,	// Special; kidnapped NPC as part of mission
 NPCATT_MAX
};

std::string npc_attitude_name(npc_attitude);
DECLARE_JSON_ENUM_SUPPORT_ATYPICAL(npc_attitude,0)

enum npc_mission {
 NPC_MISSION_NULL = 0,	// Nothing in particular
 NPC_MISSION_RESCUE_U,	// Find the player and aid them
 NPC_MISSION_SHELTER,	// Stay in shelter, introduce player to game
 NPC_MISSION_SHOPKEEP,	// Stay still unless combat or something and sell stuff

 NPC_MISSION_MISSING,	// Special; following player to finish mission
 NPC_MISSION_KIDNAPPED,	// Special; was kidnapped, to be rescued by player
 NUM_NPC_MISSIONS
};

DECLARE_JSON_ENUM_SUPPORT_ATYPICAL(npc_mission, 0)

enum npc_class {
 NC_NONE,
 NC_SHOPKEEP,	// Found in towns.  Stays in his shop mostly.
 NC_HACKER,	// Weak in combat but has hacking skills and equipment
 NC_DOCTOR,	// Found in towns, or roaming.  Stays in the clinic.
 NC_TRADER,	// Roaming trader, journeying between towns.
 NC_NINJA,	// Specializes in unarmed combat, carries few items
 NC_COWBOY,	// Gunslinger and survivalist
 NC_SCIENTIST,	// Uses intelligence-based skills and high-tech items
 NC_BOUNTY_HUNTER, // Resourceful and well-armored
 NC_MAX
};

std::string npc_class_name(npc_class);
DECLARE_JSON_ENUM_SUPPORT_ATYPICAL(npc_class, 0)

enum npc_action {
 npc_undecided = 0,
 npc_pause,
#if DEAD_FUNC
 npc_sleep, npc_drop_items, npc_heal_player,
#endif
  npc_heal,
 npc_melee,
 npc_talk_to_player,
 npc_goto_destination, npc_avoid_friendly_fire,
 num_npc_actions
};

enum npc_need {
 need_none,
 need_ammo, need_weapon, need_gun,
 need_food, need_drink,
 num_needs
};

DECLARE_JSON_ENUM_SUPPORT_ATYPICAL(npc_need,0)

enum npc_flag {
 NF_NULL,
// Items desired
 NF_FOOD_HOARDER,
 NF_DRUGGIE,
 NF_TECHNOPHILE,
 NF_BOOKWORM,
 NF_MAX
};

DECLARE_JSON_ENUM_BITFLAG_SUPPORT(npc_flag)

enum npc_favor_type {
 FAVOR_NULL,
 FAVOR_CASH,	// We owe cash (or goods of equivalent value)
 FAVOR_ITEM,	// We owe a specific item
 FAVOR_TRAINING,// We owe skill or style training
 NUM_FAVOR_TYPES
};

DECLARE_JSON_ENUM_SUPPORT(npc_favor_type)

struct npc_favor	// appears to be prototype/mockup
{
 npc_favor_type type;
 int value;
 itype_id item_id;
 skill skill_id;

 npc_favor() noexcept : type(FAVOR_NULL),value(0),item_id(itm_null),skill_id(sk_null) {}
};

struct npc_personality {
// All values should be in the -10 to 10 range.
 signed char aggression;
 signed char bravery;
 signed char collector;
 signed char altruism;
 npc_personality() : aggression(0),bravery(0),collector(0),altruism(0) {};
};

struct npc_opinion
{
 std::vector<npc_favor> favors;
 int trust;
 int fear;
 int value;
 int anger;
 int owed;

 int total_owed() {	// plausibly a placeholder
  int ret = owed;
  return ret;
 }

 npc_opinion(signed char T = 0, signed char F = 0, signed char V = 0, signed char A = 0, int O = 0) noexcept
     : trust (T), fear (F), value (V), anger(A), owed (O) { };

 npc_opinion(const npc_opinion &copy) = default;
 npc_opinion(npc_opinion&& copy) = default;
 ~npc_opinion() = default;
 npc_opinion& operator=(const npc_opinion& copy) = default;
 npc_opinion& operator=(npc_opinion&& copy) = default;

 npc_opinion& operator+=(const npc_opinion &rhs);
 std::string text() const;
};

npc_opinion operator+(const npc_opinion& lhs, const npc_opinion& rhs);

enum combat_engagement {
 ENGAGE_NONE = 0,
 ENGAGE_CLOSE,
 ENGAGE_WEAK,
 ENGAGE_HIT,
 ENGAGE_ALL
};

DECLARE_JSON_ENUM_SUPPORT_ATYPICAL(combat_engagement,0)

struct npc_combat_rules
{
 combat_engagement engagement;
 bool use_guns;
 bool use_grenades;

 npc_combat_rules() : engagement(ENGAGE_ALL), use_guns(true), use_grenades(true) {}
};

enum talk_topic {
 TALK_NONE = 0,	// Used to go back to last subject
 TALK_DONE,	// Used to end the conversation
 TALK_MISSION_LIST, // List available missions. Intentionally placed above START
 TALK_MISSION_LIST_ASSIGNED, // Same, but for assigned missions.

 TALK_MISSION_START, // NOT USED; start of mission topics
 TALK_MISSION_DESCRIBE, // Describe a mission
 TALK_MISSION_OFFER, // Offer a mission
 TALK_MISSION_ACCEPTED,
 TALK_MISSION_REJECTED,
 TALK_MISSION_ADVICE,
 TALK_MISSION_INQUIRE,
 TALK_MISSION_SUCCESS,
 TALK_MISSION_SUCCESS_LIE, // Lie caught!
 TALK_MISSION_FAILURE,
 TALK_MISSION_END, // NOT USED: end of mission topics

 TALK_MISSION_REWARD, // Intentionally placed below END

 TALK_SHELTER,
 TALK_SHELTER_PLANS,
 TALK_SHARE_EQUIPMENT,
 TALK_GIVE_EQUIPMENT,
 TALK_DENY_EQUIPMENT,

 TALK_TRAIN,
 TALK_TRAIN_START,
 TALK_TRAIN_FORCE,

 TALK_SUGGEST_FOLLOW,
 TALK_AGREE_FOLLOW,
 TALK_DENY_FOLLOW,

 TALK_SHOPKEEP,

 TALK_LEADER,
 TALK_LEAVE,
 TALK_PLAYER_LEADS,
 TALK_LEADER_STAYS,
 TALK_HOW_MUCH_FURTHER,

 TALK_FRIEND,
 TALK_COMBAT_COMMANDS,
 TALK_COMBAT_ENGAGEMENT,

 TALK_STRANGER_NEUTRAL,
 TALK_STRANGER_WARY,
 TALK_STRANGER_SCARED,
 TALK_STRANGER_FRIENDLY,
 TALK_STRANGER_AGGRESSIVE,
 TALK_MUG,

 TALK_DESCRIBE_MISSION,

 TALK_WEAPON_DROPPED,
 TALK_DEMAND_LEAVE,

 TALK_SIZE_UP,
 TALK_LOOK_AT,
 TALK_OPINION,

 NUM_TALK_TOPICS
};

DECLARE_JSON_ENUM_SUPPORT(talk_topic)

struct npc_chatbin
{
 std::vector<int> missions;           // values are mission uid values (expected non-null when attempting to find)
 std::vector<int> missions_assigned;  // values are mission uid values (expected non-null when attempting to find)
 int mission_selected;
 int tempvalue;
 talk_topic first_topic;

 npc_chatbin() : mission_selected(-1), tempvalue(-1), first_topic(TALK_NONE) {}
 npc_chatbin(const npc_chatbin& src) = default;
 npc_chatbin(npc_chatbin&& src) = default;
 ~npc_chatbin() = default;
 npc_chatbin& operator=(const npc_chatbin& src);
 npc_chatbin& operator=(npc_chatbin&& src) = default;
};

class npc final : public player {

public:
#undef MIN_ID
 enum {
   MIN_ID = 1
 };

 typedef std::pair<npc_action, std::unique_ptr<cataclysm::action> > ai_action;	// transition typedef

 template<class T>
 using ai_target = std::pair<T, std::variant<monster*, npc*, pc*> >;

 npc();
 npc(const cataclysm::JSON& src);
 npc(const npc &rhs) = default;
 npc(npc&& rhs) = default;
 ~npc() = default;
 npc& operator=(const npc& rhs);
 npc& operator=(npc&& rhs) = default;

 friend bool fromJSON(const cataclysm::JSON& src, npc& dest);
 friend cataclysm::JSON toJSON(const npc& src);

 bool is_npc() const override { return true; }
 int ID() const { return _id; }	// A unique ID number

 static npc* find(int id); // NPC with UID=id; null if non-existent
 static const npc* find_r(int id); // NPC with UID=id; null if non-existent

 static npc* find_alive(int id) {
     auto ret = find(id);
     return ret && !ret->marked_for_death ? ret : nullptr;
 };

 static const npc* find_alive_r(int id) {
     auto ret = find_r(id);
     return ret && !ret->marked_for_death ? ret : nullptr;
 };

// Generating our stats, etc.
 void randomize(npc_class type = NC_NONE);
 void randomize_from_faction(game *g, faction *fac);
#if DEAD_FUNC
 void make_shopkeep(game *g, oter_id type);
#endif
 void spawn_at(const GPS_loc& _GPSpos);
 skill best_skill() const;
 void starting_weapon();

// Display
 void draw(WINDOW* w, const point& pt, bool inv) const;
 void print_info(WINDOW* w);
 std::string short_description() const;

// Goal / mission functions
// void pick_long_term_goal(game *g);	// no implementation, but likely would influence perform_mission
 void perform_mission(game *g);
 int  minutes_to_u(const player& u) const; // Time in minutes it takes to reach player
 bool fac_has_value(faction_value value) const;
 bool fac_has_job(faction_job job) const;

// Interaction with the player
 void form_opinion(player *u);
 talk_topic pick_talk_topic(player *u);
 int  player_danger(player *u); // Comparable to monsters
 bool turned_hostile(); // True if our anger is at least equal to...
 int hostile_anger_level(); // ... this value!
 void make_angry(); // Called if the player attacks us
 bool wants_to_travel_with(player *p);
 int assigned_missions_value() const;
 std::vector<skill> skills_offered_to(const player& p) const; // Skills that are higher
 std::vector<itype_id> styles_offered_to(const player& p) const; // Martial Arts
// State checks
 bool is_enemy(const player* survivor = nullptr) const override; // We want to kill/mug/etc the player
 bool is_following() const; // Traveling w/ player (whether as a friend or a slave)
 bool is_friend(const player* survivor = nullptr) const; // Allies with the player
 bool is_leader() const; // Leading the player
 bool is_defending() const; // Putting the player's safety ahead of ours
// What happens when the player makes a request
 void told_to_help(game *g);
 void told_to_wait(game *g);
 void told_to_leave(game *g);
 int  follow_distance() const;	// How closely do we follow the player?
 int  speed_estimate(int speed) const; // Estimate of a target's speed, usually player


// Dialogue and bartering--see npctalk.cpp
 void talk_to_u(game *g);
// Bartering - select items we're willing to buy/sell and set prices
// Prices are later modified by g->u's barter skill; see dialogue.cpp
// init_buying() fills <indices> with the indices of items in <you>
 void init_buying(const inventory& you, std::vector<int> &indices, std::vector<int> &prices) const;
// init_selling() fills <indices> with the indices of items in our inventory
 void init_selling(std::vector<int> &indices, std::vector<int> &prices) const;


// Use and assessment of items
 int  minimum_item_value() const; // The minimum value to want to pick up an item
 int  worst_item_value() const; // Find the worst value in our inventory; dead function currently
 int  value(const item &it) const;
#if DEAD_FUNC
 bool wear_if_wanted(item it);
#endif

private:
 bool wield(int index) override;
public:
 bool has_healing_item() const;
 bool took_painkiller() const;
private:
 int pick_best_painkiller(const inventory& _inv) const;
public:
 void activate_item(item& it);

// Interaction and assessment of the world around us
 int  danger_assessment(game *g) const;
 int  average_damage_dealt() const; // Our guess at how much damage we can deal
 bool bravery_check(int diff) const;
 bool emergency(int danger) const;
 void say(game *g, std::string line, ...);
 void decide_needs();
 static void die(int id);
 void die(game *g, bool your_fault = false);    // only valid to call for npcs in the reality bubble
/* shift() works much like monster::shift(), and is called when the player moves
 * from one submap to an adjacent submap.  It updates our position (shifting by
 * 12 tiles), as well as our plans.
 */
 void shift(point delta); 

// placeholders for NPC use of activities
 void cancel_activity_query(const char* message, ...) override;

// Movement; the following are defined in npcmove.cpp
 void move(game *g); // Picks an action & a target and calls execute_action
private:
 void execute_action(game *g, const ai_action& action, int target); // Performs action
public:

// Functions which choose an action for a particular goal
 void choose_monster_target(game *g, int &enemy, int &danger,
                            int &total_danger);
 ai_action method_of_fleeing	(game *g, int enemy) const;
 ai_action method_of_attack	(game *g, int enemy, int danger) const;
 ai_action address_needs	(game *g, int danger) const;
 ai_action address_player	(game *g);
 ai_action long_term_goal_action(game *g);
 std::optional<item_spec> alt_attack_available() const;	// Do we have grenades, molotov, etc?
 int  choose_escape_item() const; // Returns index of our best escape aid

// Helper functions for ranged combat
 int  confident_range(int index = -1) const; // >= 50% chance to hit
 bool wont_hit_friend(game *g, int tarx, int tary, int index = -1) const;
 bool wont_hit_friend(game* g, point tar, int index = -1) const { return wont_hit_friend(g, tar.x, tar.y, index); }
 bool wont_hit_friend(const GPS_loc& tar, int index = -1) const;
 int can_reload() const override; // Wielding a gun that is not fully loaded
 bool need_to_reload() const; // Wielding a gun that is empty
 bool enough_time_to_reload(game *g, int target, const item &gun) const;
 bool can_fire() override;

// Physical movement from one tile to the next
 bool path_is_usable(const map& m);
 void update_path	(const map& m, const point& pt);
 bool update_path(const map& m, const point& pt, size_t longer_than);
 bool update_path(const GPS_loc& dest, size_t longer_than);
 void move_to		(game *g, point pt);
 void move_to_next	(game *g); // Next in <path>
 void avoid_friendly_fire(game *g, int target); // Maneuver so we won't shoot u

// Item discovery and fetching
 void find_item		(game *g); // Look around and pick an item
private:
 void pick_up_item	(); // Move to, or grab, our targeted item
public:
 ai_action scan_new_items(game *g, int target);

// Combat functions and player interaction functions
 void alt_attack(item_spec which, int target);
 void heal_self		(game *g);
private:
 bool best_melee_weapon(int& inv_index) const;
 bool can_wield_better_melee() const;
 bool best_gun(int target, int& inv_index, std::vector<int>& empty_guns, bool& has_better_melee) const;
 bool choose_empty_gun(const std::vector<int>& empty_guns, int& inv_index) const;
 bool reload(int inv_index);
#if PROTOTYPE
 void drop_items	(game *g, int weight, int volume); // Drop wgt and vol
 void heal_player(game* g, player& patient);
#endif
 int pick_best_food(const inventory& _inv) const;
 void mug_player(player& mark);

public:
 int can_look_for(const player& sought) const; /// @returns 0, or positive code indicating algorithm for looking
 ai_action look_for_player(player& sought);
 bool saw_player_recently() const;// Do we have an idea of where u are?

// Movement on the overmap scale
 auto has_destination() const { return goal; }	// Do we have a long-term destination?

private:
 void set_destination(game *g);	// Pick a place to go
 void go_to_destination(game *g); // Move there; on the micro scale
 void reach_destination() { goal = std::nullopt; } // We made it!

public:
// The preceding are in npcmove.cpp

    static npc get_proxy(std::string&& name, const point& origin, const it_gun& gun, unsigned int recoil, int charges);

// abstract ui
    void swim(const GPS_loc& loc) override;
    void subjective_message(const std::string& msg) const override {}
    void subjective_message(const char* msg) const override {}
    bool if_visible_message(std::function<std::string()> msg) const override;
    bool if_visible_message(std::function<std::string()> me, std::function<std::string()> other) const override { return if_visible_message(other); }
    bool if_visible_message(const char* msg) const override;
    bool ask_yn(const char* msg, std::function<bool()> ai = nullptr) const override { return ai ? ai() : false; }
    bool see_phantasm() override { return false; } // unclear how to implement this even for multi-PC case, let alone NPCs
    std::vector<item>* use_stack_at(const point& pt) const override;
    constexpr int use_active(item& it) override { return 0; } // stub

// grammatical support
    std::string subject() const override { return name; }
    std::string direct_object() const override { return name; }
    std::string indirect_object() const override { return name; }
    std::string possessive() const override;

// adapter for std::visit
    struct melee {
        npc& p;

        melee(npc& p) noexcept : p(p) {}
        melee(const melee& src) = delete;
        melee(melee&& src) = delete;
        melee& operator=(const melee& src) = delete;
        melee& operator=(melee&& src) = delete;
        ~melee() = default;

        void operator()(monster* target) const;
        void operator()(player* target) const;
    };

// #############   VALUES   ################

 npc_attitude attitude;	// What we want to do to the player
 npc_class myclass; // What's our archetype? (seems to be invariant after construction, reasonable)
 // last heard sound: dead data, in savefile but no users (same intended semantics as monster::wand,wandf?)
 countdown<point> wand;	// location

 // last seen player data (assumes player is singleton)
 countdown<point> pl;	// last saw player at; legal coordinates 0.. (SEEX/Y * MAPSIZE-1)
 point it;	// The square containing an item we want
 std::optional<OM_loc<2> > goal; // Which mapx:mapy square we want to get to

 bool fetching_item;
 bool has_new_items; // If true, we have something new and should re-equip

 std::vector<point> path;	// Our movement plans


// Personality & other defining characteristics
 faction *my_fac;
 npc_mission mission;	// (seems to be invariant after construction, not reasonable) \todo V 0.2.1+ allow relevant missions to complete
 npc_personality personality;
 npc_opinion op_of_u;	// \todo V 0.2.1+ clear this on new player in pre-existing world
 npc_chatbin chatbin;
 int patience; // Used when we expect the player to leave the area
 npc_combat_rules combat_rules;
 bool marked_for_death; // If true, we die as soon as we respawn!
 bool dead;		// If true, we need to be cleaned up
 std::vector<npc_need> needs;
 typename cataclysm::bitmap<NF_MAX>::type flags;

 static void global_reset();
 static void global_fromJSON(const cataclysm::JSON& src);
 static void global_toJSON(cataclysm::JSON& dest);
private:
 static int next_id;   // mininum/default next mission id
 int _id;	// A unique ID number
 void assign_id() { _id = next_id++; }

 void consume(item& food) override;
};

void parse_tags(std::string& phrase, player& u, npc& me);

#endif
