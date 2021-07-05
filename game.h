#ifndef _GAME_H_
#define _GAME_H_

#include "reality_bubble.hpp"
#include "npc.h"
#include "event.h"
#include "mission.h"
#include "weather.h"
#include "crafting.h"
#include "calendar.h"
#include "monster.h"
#include "line.h"   // for direction enum
#include "recent_msg.h"
#include "Zaimoni.STL/Logging.h"
#include "Zaimoni.STL/GDI/box.hpp"
#include <memory>
#include <optional>

// this is a god header and has the required includes \todo better location for these reality checks
static_assert(mobile::mp_turn == calendar::mp_turn);
// daylight should view almost the entire reality bubble, at least until Earth's curvature matters
static_assert(DAYLIGHT_LEVEL == (MAPSIZE / 2) * SEE);

#define LONG_RANGE 10
#define BLINK_SPEED 300
#define BULLET_SPEED 10000000
#define EXPLOSION_SPEED 70000000

#define PICKUP_RANGE 2

struct constructable;
struct special_game;

enum tut_type {
 TUT_NULL,
 TUT_BASIC, TUT_COMBAT,
 TUT_MAX
};

enum quit_status {
 QUIT_NO = 0,  // Still playing
 QUIT_MENU,    // Quit at the menu
 QUIT_SUICIDE, // Quit with 'Q'
 QUIT_SAVED,   // Saved and quit
 QUIT_DIED     // Actual death
};

struct monster_and_count
{
 monster mon;
 int count;
 monster_and_count(const monster& M, int C) : mon (M), count (C) {}
 monster_and_count(const monster_and_count& src) = default;
 monster_and_count(monster_and_count&& src) = default;
 ~monster_and_count() = default;
 monster_and_count& operator=(const monster_and_count& src) = default;
 monster_and_count& operator=(monster_and_count&& src) = default;
};

// arguably technical debt (implausible that game is-a reality bubble) 2020-12-28 zaimoni
class game : public reality_bubble
{
 private:
  static game* _active;	// while it is not a logic paradox to have more than one game object, historically it has been used as a singleton.
 public:
  static game* active() { return _active; }

  game();
  ~game();
  void setup();
  bool game_quit() const { return QUIT_MENU == uquit; }; // True if we actually quit the game - used in main.cpp
  void save();
  bool do_turn();
  void draw();
  void draw_ter(const point& pos);
  void draw_ter() { return draw_ter(u.pos); };
  void advance_nextinv();	// Increment the next inventory letter
  void decrease_nextinv();	// Decrement the next inventory letter
  bool assign_invlet(item& it, const player& p);
  bool assign_invlet_stacking_ok(item& it, const player& p);
  void add_event(event_type type, int on_turn, int faction_id = -1,
                 int x = -1, int y = -1);
  const event* event_queued(event_type type) const;
// Sound at (x, y) of intensity (vol), described to the player is (description)
  void sound(const point& pt, int vol, std::string description);
  void sound(const point& pt, int vol, const char* description) { sound(pt, vol, std::string(description)); };
  // creates a list of coordinates to draw footsteps
  void add_footstep(const point& pt, int volume, int distance);
// Explosion at (x, y) of intensity (power), with (shrapnel) chunks of shrapnel
  void explosion(int x, int y, int power, int shrapnel, bool fire);
  void explosion(const point& pt, int power, int shrapnel, bool fire) { explosion(pt.x, pt.y, power, shrapnel, fire); };
  // Flashback at (x, y)
  void flashbang(int x, int y);
  void flashbang(const point& pt) { flashbang(pt.x, pt.y); };
  // Move the player vertically, if (force) then they fell
  void vertical_move(int z, bool force);
  void use_computer(const point& pt);
  void resonance_cascade(const point& pt);
  void emp_blast(int x, int y);
  player* survivor(int x, int y);
  player* survivor(const point& pt);
  const player* survivor(int x, int y) const { return const_cast<game*>(this)->survivor(x, y); };
  const player* survivor(const point& pt) const { return const_cast<game*>(this)->survivor(pt); };
  npc* nPC(int x, int y);
  npc* nPC(const point& pt);
  npc* nPC(const GPS_loc& gps);
  const npc* nPC(int x, int y) const { return const_cast<game*>(this)->nPC(x, y); };
  const npc* nPC(const point& pt) const { return const_cast<game*>(this)->nPC(pt); };
  const npc* nPC(const GPS_loc& gps) const { return const_cast<game*>(this)->nPC(gps); };
  int  mon_at(int x, int y) const;	// Index of the monster at (x, y); -1 for none
  monster* mon(int x, int y);
  monster* mon(const point& pt);
  monster* mon(const GPS_loc& gps);
  const monster* mon(int x, int y) const { return const_cast<game*>(this)->mon(x, y); };
  const monster* mon(const point& pt) const { return const_cast<game*>(this)->mon(pt); };
  const monster* mon(const GPS_loc& gps) const { return const_cast<game*>(this)->mon(gps); };

  bool is_empty(int x, int y) const;	// True if no PC, no monster, move cost > 0
  bool is_empty(const point& pt) const { return is_empty(pt.x, pt.y); };
  static bool isEmpty(const point& pt) { return game::active()->is_empty(pt); }
  bool is_in_sunlight(const GPS_loc& pt) const; // Checks outdoors + sunny
  static unsigned char light_level(const GPS_loc& src);
  unsigned char light_level() const { return light_level(u.GPSpos); }
  // Kill that monster; fixes any pointers etc
  void kill_mon(monster& target) { if (!target.dead) _kill_mon(target, false); }
  void kill_mon(monster& target, player* me) { if (!target.dead) _kill_mon(target, me == &u); }
  void kill_mon(monster& target, monster* z) { if (!target.dead) _kill_mon(target, (0 != z->friendly)); } // not nearly enough detail
  // Explode a monster; like kill_mon but messier
  void explode_mon(monster& target, player* me = nullptr) { if (!target.dead) _explode_mon(target, me); }
  void plfire(bool burst);	// Player fires a gun (target selection)...
// ... a gun is fired, maybe by an NPC (actual damage, etc.).
  void fire(player &p, point tar, std::vector<point> &trajectory, bool burst);
  void throw_item(player &p, point tar, item&& thrown, std::vector<point> &trajectory);
  mission& give_mission(mission_id type); // Create the mission and assign it
// reserve_mission() creates a new mission of the given type and pushes it to
// active_missions.  The function returns the UID of the new mission, which can
// then be passed to a MacGuffin or something else that needs to track a mission
  int reserve_mission(mission_id type, int npc_id = -1);
  int reserve_random_mission(mission_origin origin, point p = point(-1, -1),
                             int npc_id = -1);
  mission* find_mission(int id); // Mission with UID=id; null if non-existent
  const mission_type* find_mission_type(int id); // Same, but returns its type
  void process_missions(); // Process missions, see if time's run out

  point teleport_destination(const point& origin, int tries); // Teleportation is safer for PCs and NPCs
  point teleport_destination_unsafe(const point& origin, int tries); // than for monsters
  void teleport(player* p = nullptr);
  void plswim(int x, int y); // Called by plmove.  Handles swimming
  // when player is thrown (by impact or something)
  void fling_player_or_monster(player *p, monster *zz, int dir, int flvel);

  void nuke(const point& world_div_2);
  faction* faction_by_id(int it);
  faction* random_good_faction();
  faction* random_evil_faction();

  std::optional<int> u_see(const GPS_loc& loc) const;
  std::optional<int> u_see(int x, int y) const;
  std::optional<int> u_see(const point& pt) const { return u_see(pt.x, pt.y); }
  void refresh_all();
  static bool update_map_would_scroll(const point& pt);
  void update_map(int &x, int &y);  // Called by plmove when the map updates
  void update_overmap_seen(); // Update which overmap tiles we can see

  itype* new_artifact();
  itype* new_natural_artifact(artifact_natural_property prop = ARTPROP_NULL);
  void process_artifact(item *it, player *p, bool wielded = false);	// \todo V 0.2.1 fully extend to NPCs
  static void add_artifact_messages(const std::vector<art_effect_passive>& effects);

  std::optional<point> look_around();// Look at nearby terrain	';'
  char inv(std::string title = "Inventory:");
  std::vector<item> multidrop();
  faction* list_factions(const char* title = "FACTIONS:");
  std::optional<point> find_item(item *it) const;
  void remove_item(item *it);

  signed char temperature;              // The air temperature
  weather_type weather;			// Weather pattern--SEE weather.h
  char nextinv;	// Determines which letter the next inv item will have
  player u;
  std::vector<monster> z;
  std::vector<monster_and_count> coming_to_stairs;
  tripoint monstair;
  std::vector<npc> active_npc;
  std::vector<faction> factions;
  std::vector<mission> active_missions; // Missions which may be assigned (globally valid list)
/*
2019-01-14 No implementation inherited from C:Whales
// \todo Dragging a piece of furniture, with a list of items contained
  ter_id dragging;
  std::vector<item> items_dragged;
  int weight_dragged; // Computed once, when you start dragging
*/
  static bool debugmon;		// not clearly the most useful type; a raw pointer could selectively turn on for one monster
// Display data... TODO: Make this more portable?
  WINDOW *w_terrain;
  WINDOW *w_minimap;
  WINDOW *w_HP;
  WINDOW *w_moninfo;
  WINDOW *w_messages;
  WINDOW *w_status;

 private:
// Game-start procedures
  bool opening_screen();// Warn about screen size, then present the main menu
  bool load_master();	// Load the master data file, with factions &c
  void load(std::string name);	// Load a player-specific save file
  void start_game();	// Starts a new game
  
// Data Initialization
  void init_vehicles();     // Initializes vehicle types

  void load_keyboard_settings(); // Load keybindings from disk
  void save_keymap();		// Save keybindings to disk

  void create_factions();   // Creates new factions (for a new game world)
  void create_starting_npcs(); // Creates NPCs that start near you

// Player actions
  void wish();	// Cheat by wishing for an item 'Z'
  void monster_wish(); // Create a monster
  void mutation_wish(); // Mutate

  void pldrive(int x, int y); // drive vehicle
  void plmove(int x, int y); // Standard movement; handles attacks, traps, &c

  void pldrive(direction dir); // drive vehicle; turtle direction
  void plmove(direction dir); // Standard movement; handles attacks, traps, &c; absolute direction

  void wait();	// Long wait (player action)	'^'
  void open();	// Open a door			'o'
  void close();	// Close a door			'c'
  void smash();	// Smash terrain
  void craft();                    // See crafting.cpp
  void make_craft(const recipe *making); // See crafting.cpp
  void complete_craft();           // See crafting.cpp
  void pick_recipes(std::vector<const recipe*> &current,
                    std::vector<bool> &available, craft_cat tab);// crafting.cpp
  void construction_menu();                   // See construction.cpp
  void place_construction(const constructable *con); // See construction.cpp
  void complete_construction();               // See construction.cpp
  std::optional<std::pair<point, vehicle*>> pl_choose_vehicle();
  void examine();// Examine nearby terrain	'e'
  // open vehicle interaction screen
  void exam_vehicle(vehicle &veh, int cx=0, int cy=0);
  void pickup(const point& pt, int min);// Pickup items; ',' or via examine()
// Pick where to put liquid; false if it's left where it was
  bool handle_liquid(item &liquid, bool from_ground, bool infinite);
  void drop();	  // Drop an item		'd'
  void drop_in_direction(); // Drop w/ direction 'D'
  void reassign_item(); // Reassign the letter of an item   '='
  void butcher(); // Butcher a corpse		'B'
  void complete_butcher(int index);	// Finish the butchering process
  void eat();	  // Eat food or fuel		'E' (or 'a')
  void use_item();// Use item; also tries E,R,W	'a'
  void wear();	  // Wear armor			'W' (or 'a')
  void takeoff(); // Remove armor		'T'
  void reload();  // Reload a wielded gun/tool	'r'
  void unload();  // Unload a wielded gun/tool	'U'
  void wield();   // Wield a weapon		'w'
  void read();    // Read a book		'R' (or 'a')
  void chat();    // Talk to a nearby NPC	'C'
  void plthrow(); // Throw an item		't'
  void help();    // Help screen		'?'

// Target is an interactive function which allows the player to choose a nearby
// square.  It displays information on any monster/NPC on that square, and also
// returns a Bresenham line to that square.  It is called by plfire() and
// throw().
  std::vector<point> target(point& tar, const zaimoni::gdi::box<point>& bounds,
                            const std::vector<const monster*>& t, int& target,
                            const std::string& prompt);

// Map updating and monster spawning
  void replace_stair_monsters();
  void update_stair_monsters();
  void spawn_mon(int shift, int shifty); // Called by update_map, sometimes
private:
  void _kill_mon(monster& target, bool u_did_it);
  void _explode_mon(monster& target, player* me);

// Routine loop functions, approximately in order of execution
  void cleanup_dead();     // Delete any dead NPCs/monsters
  void monmove();          // Monster movement
  void om_npcs_move();     // Movement of NPCs on the overmap (non-local)
  void activate_npcs();
  void check_warmth();     // Checks the player's warmth (applying clothing)
  void update_skills();    // Degrades practice levels, checks & upgrades skills
  void process_events();   // Processes and enacts long-term events
  void process_activity(); // Processes and enacts the player's activity
  void update_weather();   // Updates the temperature and weather patten
  void hallucinate();      // Prints hallucination junk to the screen
  void mon_info();         // Prints a list of nearby monsters (top right)
  void get_input();        // Gets player input and calls the proper function
  void update_scent();     // Updates the scent map
  bool is_game_over();     // Returns true if the player quit or died
  void death_screen();     // Display our stats, "GAME OVER BOO HOO"
  void write_msg();        // Prints the messages in the messages list
  void msg_buffer();       // Opens a window with old messages in it
  void draw_minimap();     // Draw the 5x5 minimap
  void draw_HP();          // Draws the player's HP and Power level
  void pl_draw(const zaimoni::gdi::box<point>& bounds) const;          // close-up map, centered on player
  int visible_monsters(std::vector<const monster*>& mon_targets, std::vector<int>& targetindices, std::function<bool(const monster&)> test) const;

// On-request draw functions
  void draw_overmap();     // Draws the overmap, allows note-taking etc.
public:
  void disp_kills();       // Display the player's kill counts
  void disp_NPCs();        // Currently UNUSED.  Lists global NPCs.
  void list_missions();    // Listed current, completed and failed missions.

// Debug functions
  void debug();           // All-encompassing debug screen.  TODO: This.
  void display_scent();   // Displays the scent map
  void mondebug();        // Debug monster behavior directly
  void groupdebug();      // Get into on monster groups

  // data integrity
  void z_erase(int z_index);	// morally z.erase(z.begin()+z_index), with required side effects

  void draw_footsteps();

// ########################## DATA ################################

  std::vector<point> footsteps;	// visual cue to monsters moving out of the players sight

  int last_target;// The last monster targeted. -1, or a positional index in z
  char run_mode; // 0 - Normal run always; 1 - Running allowed, but if a new
		 //  monsters spawns, go to 2 - No movement allowed
  int mostseen;	 // # of mons seen last turn; if this increases, run_mode++
  bool autosafemode; // is autosafemode enabled?
  int turnssincelastmon; // needed for auto run mode
  quit_status uquit;    // Set to true if the player quits ('Q')

  calendar nextspawn; // The turn on which monsters will spawn next.
  calendar nextweather; // The turn on which weather will shift next.
  std::vector<event> events;        // V 0.2.2+ convert to fake entity-component-system metaphor hosted at class event?
  // (Also, unclear if event should be a base class for polymorphism)
  int kills[num_monsters];	        // Player's kill count
  // Historically, we had a keypress recorder for tracking the last action taken.
  // There was no "clearing" or "archival" operation, however.
  // This would be a wrapper for input(), possibly ignoring canceled actions.

  std::unique_ptr<special_game> gamemode;
};

// doesn't actually have to be active.  Once NPCs up, could refactor to a member function
 // 0: no further op; 1: nulled; -1: IF_CHARGE; -2 artifact 
template<class P>
std::enable_if_t<std::is_base_of_v<player, P>, int>
use_active_item(P& u, item& it)
{
    if constexpr (std::is_same_v<player, P>) {
        if (!it.is_tool()) {
            if (!it.active) return 0;
            if (it.has_flag(IF_CHARGE)) return -1;  // process as charger gun
            it.active = false;  // restore invariant; for debugging purposes we'd use accessors and intercept on set
            return 0;
        }
        if (it.is_artifact()) return -2;
        if (!it.active) return 0;

        auto g = game::active();
        const it_tool* tmp = dynamic_cast<const it_tool*>(it.type);
        (*tmp->use)(g, &u, &it, true);
        if (tmp->turns_per_charge > 0 && int(messages.turn) % tmp->turns_per_charge == 0) it.charges--;
        // separate this so we respond to bugs reasonably
        if (it.charges <= 0) {
            (*tmp->use)(g, &u, &it, false); // turns off
            if (tmp->revert_to == itm_null) {
                it = item::null;
                return 1;
            }
            it.type = item::types[tmp->revert_to];
        }
    } else {
        // adjust this to be more lenient as buildout happens
        static_assert(unconditional_v<bool, false>, "NPC use of artifacts and items is not there yet");
    }
    return 0;
}

#endif
