#ifndef _MAPDATA_H_
#define _MAPDATA_H_

#include "item.h"
#include "trap.h"
#include "monster.h"
#include "computer.h"
#include "vehicle.h"
#include "ui.h"

#include <map>

class game;

// mfb(t_flag) converts a flag to a bit for insertion into a bitfield
#ifndef mfb
#define mfb(n) long(1 << (n))
#endif

enum t_flag {
 transparent = 0,// Player & monsters can see through/past it
 bashable,     // Player & monsters can bash this & make it the next in the list
 container,    // Items on this square are hidden until looted by the player
 door,         // Can be opened--used for NPC pathfinding.
 flammable,    // May be lit on fire
 l_flammable,  // Harder to light on fire, but still possible
 explodes,     // Explodes when on fire
 diggable,     // Digging monsters, seeding monsters, digging w/ shovel, etc.
 liquid,       // Blocks movement but isn't a wall, e.g. lava or water
 swimmable,    // You (and monsters) swim here
 sharp,	       // May do minor damage to players/monsters passing it
 rough,        // May hurt the player's feet
 sealed,       // Can't 'e' to retrieve items here
 noitem,       // Items "fall off" this space
 goes_down,    // Can '>' to go down a level
 goes_up,      // Can '<' to go up a level
 console,      // Used as a computer
 alarmed,      // Sets off an alarm if smashed
 supports_roof,// Used as a boundary for roof construction
 thin_obstacle,// passable by player and monsters, but not by vehicles
 num_t_flags   // MUST be last
};

enum ter_id {
t_null = 0,
t_hole,	// Real nothingness; makes you fall a z-level
// Ground
t_dirt, t_dirtmound, t_pit_shallow, t_pit, t_pit_spiked,
t_rock_floor, t_rubble, t_wreckage,
t_grass,
t_metal_floor,
t_pavement, t_pavement_y, t_sidewalk,
t_floor,
t_grate,
t_slime,
t_bridge,
// Walls & doors
t_wall_half, t_wall_wood, t_wall_wood_chipped, t_wall_wood_broken,
t_wall_v, t_wall_h,
t_wall_metal_v, t_wall_metal_h,
t_wall_glass_v, t_wall_glass_h,
t_wall_glass_v_alarm, t_wall_glass_h_alarm,
t_reinforced_glass_v, t_reinforced_glass_h,
t_bars,
t_door_c, t_door_b, t_door_o, t_door_locked, t_door_locked_alarm, t_door_frame,
 t_door_boarded,
t_door_metal_c, t_door_metal_o, t_door_metal_locked,
t_door_glass_c, t_door_glass_o,
t_bulletin,
t_portcullis,
t_window, t_window_alarm, t_window_empty, t_window_frame, t_window_boarded,
t_rock, t_fault /* amigara */,
t_paper,
// Tree
t_tree, t_tree_young, t_underbrush, t_shrub, t_log,
t_root_wall,
t_wax, t_floor_wax,
t_fence_v, t_fence_h,
t_railing_v, t_railing_h,
// Nether
t_marloss, t_fungus, t_tree_fungal,
// Water, lava, etc.
t_water_sh, t_water_dp, t_sewage,
t_lava,
// Embellishments
t_bed, t_toilet,
t_sandbox, t_slide, t_monkey_bars, t_backboard,
t_bench, t_table, t_pool_table,
t_gas_pump, t_gas_pump_smashed,
t_missile, t_missile_exploded,
t_counter,
t_radio_tower, t_radio_controls,
t_console_broken, t_console,
t_sewage_pipe, t_sewage_pump,
t_centrifuge,
t_column,
// Containers
t_fridge, t_dresser,
t_rack, t_bookcase,
t_dumpster,
t_vat, t_crate_c, t_crate_o,
// Staircases etc.
t_stairs_down, t_stairs_up, t_manhole, t_ladder_up, t_ladder_down, t_slope_down,
 t_slope_up, t_rope_up,
t_manhole_cover,
// Special
t_card_science, t_card_military, t_card_reader_broken, t_slot_machine,
 t_elevator_control, t_elevator_control_off, t_elevator, t_pedestal_wyrm,
 t_pedestal_temple,
// Temple tiles
t_rock_red, t_rock_green, t_rock_blue, t_floor_red, t_floor_green, t_floor_blue,
 t_switch_rg, t_switch_gb, t_switch_rb, t_switch_even,
num_terrain_types
};

const char* JSON_key(ter_id src);

namespace cataclysm {

template<>
struct JSON_parse<ter_id>
{
	ter_id operator()(const char* src);
	ter_id operator()(const std::string& src) { return operator()(src.c_str()); };
};

}

struct ter_t {
	static const ter_t list[num_terrain_types];
	static std::map<ter_id, std::string> tiles;

	std::string name;
	char sym;
	nc_color color;
	unsigned char movecost;
	trap_id trap;
	unsigned long flags;// : num_t_flags;

	static void init();
};

enum map_extra {
 mx_null = 0,
 mx_helicopter,
 mx_military,
 mx_science,
 mx_stash,
 mx_drugdeal,
 mx_supplydrop,
 mx_portal,
 mx_minefield,
 mx_wolfpack,
 mx_puddle,
 mx_crater,
 mx_fumarole,
 mx_portal_in,
 mx_anomaly,
 num_map_extras
};

// Chances are relative to eachother; e.g. a 200 chance is twice as likely
// as a 100 chance to appear.
struct map_extras {
 unsigned int chance;
 int chances[num_map_extras];
 map_extras(unsigned int embellished, int helicopter = 0, int mili = 0,
            int sci = 0, int stash = 0, int drug = 0, int supply = 0,
            int portal = 0, int minefield = 0, int wolves = 0, int puddle = 0, 
            int crater = 0, int lava = 0, int marloss = 0, int anomaly = 0)
            : chance(embellished)
 {
  chances[mx_null] = 0;
  chances[mx_helicopter] = helicopter;
  chances[mx_military] = mili;
  chances[mx_science] = sci;
  chances[mx_stash] = stash;
  chances[mx_drugdeal] = drug;
  chances[mx_supplydrop] = supply;
  chances[mx_portal] = portal;
  chances[mx_minefield] = minefield;
  chances[mx_wolfpack] = wolves;
  chances[mx_puddle] = puddle;
  chances[mx_crater] = crater;
  chances[mx_fumarole] = lava;
  chances[mx_portal_in] = marloss;
  chances[mx_anomaly] = anomaly;
 }
};

struct field_t {
 std::string name[3];
 char sym;
 nc_color color[3];
 bool transparent[3];
 bool dangerous[3];
 int halflife;	// In turns
};

enum field_id {
 fd_null = 0,
 fd_blood,
 fd_bile,
 fd_web,
 fd_slime,
 fd_acid,
 fd_sap,
 fd_fire,
 fd_smoke,
 fd_toxic_gas,
 fd_tear_gas,
 fd_nuke_gas,
 fd_gas_vent,
 fd_fire_vent,
 fd_flame_burst,
 fd_electricity,
 fd_fatigue,
 fd_push_items,
 fd_shock_vent,
 fd_acid_vent,
 num_fields
};

struct field {
 static const field_t list[];

 field_id type;
 signed char density;	// only 1..3 valid; 0 is useful to signal "expired"
 int age;

 field(field_id t = fd_null, unsigned char d=1, unsigned int a=0)
 : type(t),density(d),age(a)
 { }

 bool is_null() const
 {
  return (type == fd_null || type == fd_blood || type == fd_bile || type == fd_slime);
 }

 bool is_dangerous() const { return list[type].dangerous[density - 1]; }
 std::string name() const { return list[type].name[density - 1]; }
};

struct spawn_point {
 point pos;
 int count;
 mon_id type;
 int faction_id;
 int mission_id;
 bool friendly;
 std::string name;
 spawn_point(mon_id T = mon_null, int C = 0, int X = -1, int Y = -1,
             int FAC = -1, int MIS = -1, bool F = false,
             std::string N = "NONE") :
             pos(X,Y), count (C), type (T), faction_id (FAC),
             mission_id (MIS), friendly (F), name (N) {}

 spawn_point(std::istream& is);
};

struct submap {
 ter_id			ter[SEEX][SEEY]; // Terrain on each square
 std::vector<item>	itm[SEEX][SEEY]; // Items on each square
 trap_id		trp[SEEX][SEEY]; // Trap on each square
 field			fld[SEEX][SEEY]; // Field on each square
 int			rad[SEEX][SEEY]; // Irradiation of each square
 int active_item_count;
 int field_count;
 int turn_last_touched;
 std::vector<spawn_point> spawns;
 std::vector<vehicle> vehicles;
 computer comp;
};

#endif
