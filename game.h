#ifndef _GAME_H_
#define _GAME_H_

#include "map.h"
#include "npc.h"
#include "event.h"
#include "mission.h"
#include "weather.h"
#include "construction.h"
#include "calendar.h"
#include "gamemode.h"
#include "monster.h"
#include "line.h"   // for direction enum
#include "recent_msg.h"
#include "Zaimoni.STL/Logging.h"
#include "Zaimoni.STL/GDI/box.hpp"
#include <memory>

#define LONG_RANGE 10
#define BLINK_SPEED 300
#define BULLET_SPEED 10000000
#define EXPLOSION_SPEED 70000000

#define PICKUP_RANGE 2

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
 monster_and_count(monster M, int C) : mon (M), count (C) {};
};

class game
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
  void add_event(event_type type, int on_turn, int faction_id = -1,
                 int x = -1, int y = -1);
  bool event_queued(event_type type) const;
// Sound at (x, y) of intensity (vol), described to the player is (description)
  void sound(int x, int y, int vol, std::string description);
  void sound(const point& pt, int vol, const std::string& description) { sound(pt.x, pt.y, vol, description); };
  void sound(const point& pt, int vol, const char* description) { sound(pt.x, pt.y, vol, description); };
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
  void resonance_cascade(int x, int y);
  void emp_blast(int x, int y);
  player* survivor(int x, int y);
  player* survivor(const point& pt);
  const player* survivor(int x, int y) const { return const_cast<game*>(this)->survivor(x, y); };
  const player* survivor(const point& pt) const { return const_cast<game*>(this)->survivor(pt); };
  npc* nPC(int x, int y);
  npc* nPC(const point& pt);
  const npc* nPC(int x, int y) const { return const_cast<game*>(this)->nPC(x, y); };
  const npc* nPC(const point& pt) const { return const_cast<game*>(this)->nPC(pt); };
  int  mon_at(int x, int y) const;	// Index of the monster at (x, y); -1 for none
  monster* mon(int x, int y);
  monster* mon(const point& pt);
  const monster* mon(int x, int y) const { return const_cast<game*>(this)->mon(x, y); };
  const monster* mon(const point& pt) const { return const_cast<game*>(this)->mon(pt); };

  bool is_empty(int x, int y) const;	// True if no PC, no monster, move cost > 0
  bool is_empty(const point& pt) const { return is_empty(pt.x, pt.y); };
  bool is_in_sunlight(const point& pt) const; // Checks outdoors + sunny
  unsigned char light_level() const;
  // Kill that monster; fixes any pointers etc
  void kill_mon(monster& target, bool u_did_it = false);
  void kill_mon(monster* target, bool u_did_it = false) { if (target) kill_mon(*target, u_did_it); };
  void kill_mon(int index, bool u_did_it = false) { assert(0 <= index && index<z.size()); kill_mon(z[index], u_did_it); };
  void explode_mon(monster& target);	// Explode a monster; like kill_mon but messier
// hit_monster_with_flags processes ammo flags (e.g. incendiary, etc)
  void hit_monster_with_flags(monster &z, unsigned int flags);
  void plfire(bool burst);	// Player fires a gun (target selection)...
// ... a gun is fired, maybe by an NPC (actual damage, etc.).
  void fire(player &p, point tar, std::vector<point> &trajectory, bool burst);
  void throw_item(player &p, point tar, const item &thrown, std::vector<point> &trajectory);
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

  void teleport(player* p = nullptr);
  void plswim(int x, int y); // Called by plmove.  Handles swimming
  // when player is thrown (by impact or something)
  void fling_player_or_monster(player *p, monster *zz, int dir, int flvel);

  void nuke(const point& world_div_2);
  int& scent(int x, int y);
  int& scent(const point& pt) { return scent(pt.x, pt.y); };
  int scent(int x, int y) const { return const_cast<game*>(this)->scent(x, y); };	// consider optimized implementation
  int scent(const point& pt) const { return const_cast<game*>(this)->scent(pt.x, pt.y); };
  void clear_scents() { memset(grscent, 0, sizeof(grscent)); }
  faction* faction_by_id(int it);
  std::vector<faction *> factions_at(int x, int y);	// dead function
  faction* random_good_faction();
  faction* random_evil_faction();

  bool sees_u(int x, int y);
  bool sees_u(const point& pt) { return sees_u(pt.x, pt.y); };
  bool sees_u(int x, int y, int &t);
  bool sees_u(const point& pt, int &t) { return sees_u(pt.x, pt.y, t); };
  bool u_see(int x, int y) const;
  bool u_see(const point& pt) const { return u_see(pt.x, pt.y); };
  bool u_see(int x, int y, int &t) const;
  bool u_see(const monster *mon) const;
  bool pl_sees(player *p, monster *mon) const;
  void refresh_all();
  GPS_loc toGPS(point screen_pos) const;
  bool toScreen(GPS_loc GPS_pos, point& screen_pos) const;
  static bool update_map_would_scroll(const point& pt);
  void update_map(int &x, int &y);  // Called by plmove when the map updates
  void update_overmap_seen(); // Update which overmap tiles we can see
  OM_loc om_location(); // levx and levy converted to overmap coordinates

  itype* new_artifact();
  itype* new_natural_artifact(artifact_natural_property prop = ARTPROP_NULL);
  void process_artifact(item *it, player *p, bool wielded = false);	// \todo V 0.2.1 fully extend to NPCs
  static void add_artifact_messages(const std::vector<art_effect_passive>& effects);

  point look_around();// Look at nearby terrain	';'
  char inv(std::string title = "Inventory:");
  std::vector<item> multidrop();
  faction* list_factions(std::string title = "FACTIONS:");
  point find_item(item *it) const;
  bool find_item(item* it, point& pos) const;
  void remove_item(item *it);

  signed char temperature;              // The air temperature
  weather_type weather;			// Weather pattern--SEE weather.h
  char nextinv;	// Determines which letter the next inv item will have
  overmap cur_om;
  map m;
  tripoint lev;	// Placement inside the overmap; x/y should be in 0...OMAPX/Y; offset from the player's submap coordinates
	// but game::update_map thinks legal values are 0..2*OMAPX/Y
    // lev.z is almost always cur_om.pos.z (possibly should be explicitly enforced as map loading responds to cur_om.pos.z)
    // in savegames: lev = u.GPSpos.first+(-5,-5,0).  Should be true anytime except during reality bubble shift during a move between submaps
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
  bool player_can_build(player &p, inventory inv, const constructable* con,
                        int level = -1, bool cont = false);
  void place_construction(const constructable *con); // See construction.cpp
  void complete_construction();               // See construction.cpp
  bool pl_choose_vehicle(point& vpos);
  bool vehicle_near ();
  void handbrake ();
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
// square.  It display information on any monster/NPC on that square, and also
// returns a Bresenham line to that square.  It is called by plfire() and
// throw().
  std::vector<point> target(point& tar, const zaimoni::gdi::box<point>& bounds, std::vector<const monster*> t, int &target, item *relevent);

// Map updating and monster spawning
  void replace_stair_monsters();
  void update_stair_monsters();
  void spawn_mon(int shift, int shifty); // Called by update_map, sometimes
  mongroup* valid_group(mon_id type, int x, int y);// Picks a group from cur_om
private:

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

  int grscent[SEEX * MAPSIZE][SEEY * MAPSIZE];	// The scent map
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
