#ifndef _MAPDATA_H_
#define _MAPDATA_H_

#include "trap.h"
#include "rational.hpp"

#include <map>

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

enum ter_id : int {
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

DECLARE_JSON_ENUM_SUPPORT_ATYPICAL(ter_id,0)

template<int lb, int ub> constexpr bool any(ter_id src) {
    static_assert(lb < ub);
    return lb == src || ub == src;
}

struct ter_t {
	static const ter_t list[num_terrain_types];
	static std::map<ter_id, std::string> tiles;
	static constexpr const std::pair<ter_id, std::pair<rational, std::pair<int, int> > > water_from_terrain[] = {
		{t_water_sh, {rational(1, 3), std::pair(1, 4)}},
		{t_water_dp, {rational(1, 4), std::pair(1, 4)}},
		{t_sewage, {rational(1), std::pair(1, 7)}},
		{t_toilet, {rational(2, 3), std::pair(1, 3)}}
	};

	std::string name;
	char sym;
	nc_color color;
	unsigned char movecost;
	trap_id trap;
	unsigned long flags;// : num_t_flags;

	static void init();
};

template<t_flag flag> bool is(ter_id terrain) { return ter_t::list[terrain].flags & mfb(flag); }
inline int move_cost_of(ter_id terrain) { return ter_t::list[terrain].movecost; }
inline const std::string& name_of(ter_id terrain) { return ter_t::list[terrain].name; }

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

DECLARE_JSON_ENUM_SUPPORT(field_id)

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

field_id bleeds(const monster& mon);
inline field_id bleeds(const monster* mon) { return mon ? bleeds(*mon) : fd_blood; };

#endif
