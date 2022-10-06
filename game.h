#ifndef _GAME_H_
#define _GAME_H_

#include "reality_bubble.hpp"
#include "npc.h"
#include "pc.hpp"
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

  // UI support
  static bool relay_message(const std::string& msg);

 public:
  using npcs_t = overmap::npcs_t;

  static game* active() { return _active; }
  static const game* active_const() { return _active; }

  game();
  ~game();
  void setup();
  bool game_quit() const { return QUIT_MENU == uquit; }; // True if we actually quit the game - used in main.cpp
  void save();
  bool do_turn();
  void draw();
  void draw_ter(const point& pos);
  void draw_ter() { return draw_ter(u.pos); };

// Sound at (x, y) of intensity (vol), described to the player is (description)
  void sound(const point& pt, int vol, std::string description);
  void sound(const point& pt, int vol, const char* description) { sound(pt, vol, std::string(description)); };

  template<class T>
  bool if_visible_message(std::function<std::string()> msg, const T& view) requires requires(const pc& u) { u.see(view); }
  {
	  if (msg) {
		  if (u.see(view)) return relay_message(msg());
	  }
	  return false;
  }

  template<class M, class T>
  bool if_visible_message(const M& msg, const T& view) requires requires(const pc& u) { u.see(view); game::relay_message(msg); }
  {
	  if (u.see(view)) return relay_message(msg);
	  return false;
  }

// Explosion at (x, y) of intensity (power), with (shrapnel) chunks of shrapnel
  void explosion(const point& pt, int power, int shrapnel, bool fire);
  void flashbang(const GPS_loc& pt);
  // Move the player vertically, if (force) then they fell
  void vertical_move(int z, bool force);
  void resonance_cascade(const point& pt);
  void emp_blast(const point& pt);
  player* survivor(const point& pt);
  player* survivor(const GPS_loc& gps);
  const player* survivor(const point& pt) const { return const_cast<game*>(this)->survivor(pt); }
  const player* survivor(const GPS_loc& gps) const { return const_cast<game*>(this)->survivor(gps); }
  npc* nPC(const point& pt);
  npc* nPC(const GPS_loc& gps);
  const npc* nPC(const point& pt) const { return const_cast<game*>(this)->nPC(pt); }
  const npc* nPC(const GPS_loc& gps) const { return const_cast<game*>(this)->nPC(gps); }
  monster* mon(const point& pt);
  monster* mon(const GPS_loc& gps);
  const monster* mon(const point& pt) const { return const_cast<game*>(this)->mon(pt); }
  const monster* mon(const GPS_loc& gps) const { return const_cast<game*>(this)->mon(gps); }
  mobile* mob(const GPS_loc& gps);  // does not include vehicles
  const mobile* mob(const GPS_loc& gps) const { return const_cast<game*>(this)->mob(gps); }
  std::optional<std::variant<monster*, npc*, pc*> > mob_at(const point& pt);
  std::optional<std::variant<monster*, npc*, pc*> > mob_at(const GPS_loc& gps);
  std::optional<std::variant<const monster*, const npc*, const pc*> > mob_at(const point& pt) const;
  std::optional<std::variant<const monster*, const npc*, const pc*> > mob_at(const GPS_loc& gps) const;
  std::optional<std::vector<std::variant<monster*, npc*, pc*> > > mobs_in_range(const GPS_loc& gps, int range);
  std::optional<std::vector<std::pair<std::variant<monster*, npc*, pc*> , int> > > mobs_with_range(const GPS_loc& gps, int range);
  void forall_do(std::function<void(monster&)> op);
  void forall_do(std::function<void(const monster&)> op) const;
  void forall_do(std::function<void(player&)> op);
  void forall_do(std::function<void(const player&)> op) const;
  void forall_do(std::function<void(npc&)> op);
  void forall_do(std::function<void(const npc&)> op) const;
  size_t count(std::function<bool(const monster&)> ok) const;
  const monster* find_first(std::function<bool(const monster&)> ok) const;
  monster* find_first(std::function<bool(const monster&)> ok) { return const_cast<monster*>(const_cast<const game*>(this)->find_first(ok)); }
  const npc* find_first(std::function<bool(const npc&)> ok) const;
  npc* find_first(std::function<bool(const npc&)> ok) { return const_cast<npc*>(const_cast<const game*>(this)->find_first(ok)); }
  bool exec_first(std::function<std::optional<bool>(npc&) > op);
  void spawn(npc&& whom);
  void spawn(const monster& whom);
  void spawn(monster&& whom);
  size_t mon_count() const { return z.size(); }

  bool is_empty(const point& pt) const;
  static bool isEmpty(const point& pt) { return game::active()->is_empty(pt); }
  bool is_in_sunlight(const GPS_loc& pt) const; // Checks outdoors + sunny
  static unsigned char light_level(const GPS_loc& src);
  unsigned char light_level() const { return light_level(u.GPSpos); }
  // Kill that monster; fixes any pointers etc
  void kill_mon(monster& target) { if (!target.dead) target.killed(); }
  void kill_mon(monster& target, player* me) { if (!target.dead) target.killed(dynamic_cast<pc*>(me)); }
  void kill_mon(monster& target, const monster* z) { if (!target.dead) target.killed(z->is_friend() ? &u : nullptr); } // not nearly enough detail
  void kill_mon(const std::variant<monster*, npc*, pc*>& target) {
      if (auto _mon = std::get_if<monster*>(&target)) kill_mon(**_mon);
  }
  // Explode a monster; like kill_mon but messier
  void explode_mon(monster& target, player* me = nullptr) { if (!target.dead) _explode_mon(target, me); }
  void plfire(bool burst);	// Player fires a gun (target selection)...
// ... a gun is fired, maybe by an NPC (actual damage, etc.).
  void fire(player& p, std::vector<GPS_loc>& trajectory, bool burst);
  void throw_item(player &p, point tar, item&& thrown, std::vector<point> &trajectory);
  void throw_item(player& p, item&& thrown, std::vector<GPS_loc>& trajectory);
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
  void teleport_to(monster& z, const point& dest);

  void nuke(const point& world_div_2);
  faction* faction_by_id(int it);
  faction* random_good_faction();
  faction* random_evil_faction();

  void refresh_all();
  static bool update_map_would_scroll(const point& pt);
  void update_map(int &x, int &y);  // Called by plmove when the map updates
  void update_overmap_seen(); // Update which overmap tiles we can see

  itype* new_artifact();
  itype* new_natural_artifact(artifact_natural_property prop = ARTPROP_NULL);
  void process_artifact(item *it, player *p, bool wielded = false);	// \todo V 0.2.1 fully extend to NPCs
  static void add_artifact_messages(const std::vector<art_effect_passive>& effects);

  std::optional<point> look_around();// Look at nearby terrain	';'
  faction* list_factions(const char* title = "FACTIONS:");
  std::optional<point> find_item(item *it) const;
  std::optional<
      std::variant<GPS_loc,
        std::pair<pc*, int>,
        std::pair<npc*, int> > > find(item& it);

  void remove_item(item *it);

  signed char temperature;              // The air temperature
  weather_type weather;			// Weather pattern--SEE weather.h
  pc u;
  std::vector<monster> z;
  std::vector<monster_and_count> coming_to_stairs;
  tripoint monstair;
  npcs_t active_npc;
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
  void plmove(point delta); // Standard movement; handles attacks, traps, &c

  void pldrive(direction dir); // drive vehicle; turtle direction
  void plmove(direction dir); // Standard movement; handles attacks, traps, &c; absolute direction

  void use(computer& used);
  void wait();	// Long wait (player action)	'^'
  void open();	// Open a door			'o'
  void close();	// Close a door			'c'
  void smash();	// Smash terrain
  void craft();                    // See crafting.cpp
  void complete_craft();           // See crafting.cpp
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
  std::optional<std::vector<GPS_loc> > target(GPS_loc& tar, const zaimoni::gdi::box<point>& bounds,
                            const std::vector<const monster*>& t, int& target,
                            const std::string& prompt);

// Map updating and monster spawning
  void replace_stair_monsters();
  void update_stair_monsters();
  void spawn_mon(int shift, int shifty); // Called by update_map, sometimes
  void _explode_mon(monster& target, player* me);

// Routine loop functions, approximately in order of execution
  void cleanup_dead();     // Delete any dead NPCs/monsters
  void monmove();          // Monster movement
  void om_npcs_move();     // Movement of NPCs on the overmap (non-local)
  void activate_npcs();
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
  void draw_targets(const std::vector<std::pair<std::variant<monster*, npc*, pc*>, std::vector<GPS_loc> > >& src) const;
  int visible_monsters(std::vector<const monster*>& mon_targets, std::vector<int>& targetindices, std::function<bool(const monster&)> test) const;

// On-request draw functions
  void draw_overmap();     // Draws the overmap, allows note-taking etc.
public:
  void disp_kills();       // Display the player's kill counts
#if DEAD_FUNC
  void disp_NPCs();        // Currently UNUSED.  Lists global NPCs.
#endif
  void list_missions();    // Listed current, completed and failed missions.

// Debug functions
  void debug();           // All-encompassing debug screen.  TODO: This.
  void display_scent();   // Displays the scent map
  void mondebug() const;        // Debug monster behavior directly
  void groupdebug();      // Get into on monster groups

  // data integrity
  void z_erase(std::function<bool(monster&)> reject);

// ########################## DATA ################################

  quit_status uquit;    // Set to true if the player quits ('Q')

  calendar nextspawn; // The turn on which monsters will spawn next.
  calendar nextweather; // The turn on which weather will shift next.

  // Historically, we had a keypress recorder for tracking the last action taken.
  // There was no "clearing" or "archival" operation, however.
  // This would be a wrapper for input(), possibly ignoring canceled actions.

  std::unique_ptr<special_game> gamemode;
};

#endif
