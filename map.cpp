#include "map.h"
#include "output.h"
#include "rng.h"
#include "game.h"
#include "line.h"
#include "mapbuffer.h"
#include <cmath>
#include <stdlib.h>
#include <fstream>
#include "posix_time.h"
#include "JSON.h"
#include "recent_msg.h"

#include <iostream>

std::map<ter_id, std::string> ter_t::tiles;

const ter_t ter_t::list[num_terrain_types] = {  // MUST match enum ter_id!
	{ "nothing",	     ' ', c_white,   2, tr_null,
	mfb(transparent) | mfb(diggable) },
{ "empty space",      '#', c_black,   2, tr_ledge,
mfb(transparent) },
{ "dirt",	     '.', c_brown,   2, tr_null,
mfb(transparent) | mfb(diggable) },
{ "mound of dirt",    '#', c_brown,   3, tr_null,
mfb(transparent) | mfb(diggable) },
{ "shallow pit",	     '0', c_yellow,  8, tr_null,
mfb(transparent) | mfb(diggable) },
{ "pit",              '0', c_brown,  10, tr_pit,
mfb(transparent) | mfb(diggable) },
{ "spiked pit",       '0', c_ltred,  10, tr_spike_pit,
mfb(transparent) | mfb(diggable) },
{ "rock floor",       '.', c_ltgray,  2, tr_null,
mfb(transparent) },
{ "pile of rubble",   '#', c_ltgray,  4, tr_null,
mfb(transparent) | mfb(rough) | mfb(diggable) },
{ "metal wreckage",   '#', c_cyan,    5, tr_null,
mfb(transparent) | mfb(rough) | mfb(sharp) | mfb(container) },
{ "grass",	     '.', c_green,   2, tr_null,
mfb(transparent) | mfb(diggable) },
{ "metal floor",      '.', c_ltcyan,  2, tr_null,
mfb(transparent) },
{ "pavement",	     '.', c_dkgray,  2, tr_null,
mfb(transparent) },
{ "yellow pavement",  '.', c_yellow,  2, tr_null,
mfb(transparent) },
{ "sidewalk",         '.', c_ltgray,  2, tr_null,
mfb(transparent) },
{ "floor",	     '.', c_cyan,    2, tr_null,
mfb(transparent) | mfb(l_flammable) },
{ "metal grate",      '#', c_dkgray,  2, tr_null,
mfb(transparent) },
{ "slime",            '~', c_green,   6, tr_null,
mfb(transparent) | mfb(container) | mfb(flammable) },
{ "walkway",          '#', c_yellow,  2, tr_null,
mfb(transparent) },
{ "half-built wall",  '#', c_ltred,   4, tr_null,
mfb(transparent) | mfb(bashable) | mfb(flammable) | mfb(noitem) },
{ "wooden wall",      '#', c_ltred,   0, tr_null,
mfb(bashable) | mfb(flammable) | mfb(noitem) | mfb(supports_roof) },
{ "chipped wood wall",'#', c_ltred,   0, tr_null,
mfb(bashable) | mfb(flammable) | mfb(noitem) | mfb(supports_roof) },
{ "broken wood wall", '&', c_ltred,   0, tr_null,
mfb(transparent) | mfb(bashable) | mfb(flammable) | mfb(noitem) |
mfb(supports_roof) },
{ "wall",             '|', c_ltgray,  0, tr_null,
mfb(flammable) | mfb(noitem) | mfb(supports_roof) },
{ "wall",             '-', c_ltgray,  0, tr_null,
mfb(flammable) | mfb(noitem) | mfb(supports_roof) },
{ "metal wall",       '|', c_cyan,    0, tr_null,
mfb(noitem) | mfb(noitem) | mfb(supports_roof) },
{ "metal wall",       '-', c_cyan,    0, tr_null,
mfb(noitem) | mfb(noitem) | mfb(supports_roof) },
{ "glass wall",       '|', c_ltcyan,  0, tr_null,
mfb(transparent) | mfb(bashable) | mfb(noitem) | mfb(supports_roof) },
{ "glass wall",       '-', c_ltcyan,  0, tr_null,
mfb(transparent) | mfb(bashable) | mfb(noitem) | mfb(supports_roof) },
{ "glass wall",       '|', c_ltcyan,  0, tr_null, // Alarmed
mfb(transparent) | mfb(bashable) | mfb(alarmed) | mfb(noitem) |
mfb(supports_roof) },
{ "glass wall",       '-', c_ltcyan,  0, tr_null, // Alarmed
mfb(transparent) | mfb(bashable) | mfb(alarmed) | mfb(noitem) |
mfb(supports_roof) },
{ "reinforced glass", '|', c_ltcyan,  0, tr_null,
mfb(transparent) | mfb(bashable) | mfb(noitem) | mfb(supports_roof) },
{ "reinforced glass", '-', c_ltcyan,  0, tr_null,
mfb(transparent) | mfb(bashable) | mfb(noitem) | mfb(supports_roof) },
{ "metal bars",       '"', c_ltgray,  0, tr_null,
mfb(transparent) | mfb(noitem) },
{ "closed wood door", '+', c_brown,   0, tr_null,
mfb(bashable) | mfb(flammable) | mfb(door) | mfb(noitem) | mfb(supports_roof) },
{ "damaged wood door",'&', c_brown,   0, tr_null,
mfb(transparent) | mfb(bashable) | mfb(flammable) | mfb(noitem) |
mfb(supports_roof) },
{ "open wood door",  '\'', c_brown,   2, tr_null,
mfb(flammable) | mfb(transparent) | mfb(supports_roof) },
{ "closed wood door", '+', c_brown,   0, tr_null,	// Actually locked
mfb(bashable) | mfb(flammable) | mfb(noitem) | mfb(supports_roof) },
{ "closed wood door", '+', c_brown,   0, tr_null, // Locked and alarmed
mfb(bashable) | mfb(flammable) | mfb(alarmed) | mfb(noitem) |
mfb(supports_roof) },
{ "empty door frame", '.', c_brown,   2, tr_null,
mfb(flammable) | mfb(transparent) | mfb(supports_roof) },
{ "boarded up door",  '#', c_brown,   0, tr_null,
mfb(bashable) | mfb(flammable) | mfb(noitem) | mfb(supports_roof) },
{ "closed metal door",'+', c_cyan,    0, tr_null,
mfb(noitem) | mfb(supports_roof) },
{ "open metal door", '\'', c_cyan,    2, tr_null,
mfb(transparent) | mfb(supports_roof) },
{ "closed metal door",'+', c_cyan,    0, tr_null, // Actually locked
mfb(noitem) | mfb(supports_roof) },
{ "closed glass door",'+', c_ltcyan,  0, tr_null,
mfb(transparent) | mfb(bashable) | mfb(door) | mfb(noitem) | mfb(supports_roof) },
{ "open glass door", '\'', c_ltcyan,  2, tr_null,
mfb(transparent) | mfb(supports_roof) },
{ "bulletin board",   '6', c_blue,    0, tr_null,
mfb(flammable) | mfb(noitem) },
{ "makeshift portcullis", '&', c_cyan, 0, tr_null,
mfb(noitem) },
{ "window",	     '"', c_ltcyan,  0, tr_null,
mfb(transparent) | mfb(bashable) | mfb(flammable) | mfb(noitem) |
mfb(supports_roof) },
{ "window",	     '"', c_ltcyan,  0, tr_null, // Actually alarmed
mfb(transparent) | mfb(bashable) | mfb(flammable) | mfb(alarmed) | mfb(noitem) |
mfb(supports_roof) },
{ "empty window",     '0', c_yellow,  8, tr_null,
mfb(transparent) | mfb(flammable) | mfb(noitem) | mfb(supports_roof) },
{ "window frame",     '0', c_ltcyan,  8, tr_null,
mfb(transparent) | mfb(sharp) | mfb(flammable) | mfb(noitem) |
mfb(supports_roof) },
{ "boarded up window",'#', c_brown,   0, tr_null,
mfb(bashable) | mfb(flammable) | mfb(noitem) | mfb(supports_roof) },
{ "solid rock",       '#', c_white,   0, tr_null,
mfb(noitem) | mfb(supports_roof) },
{ "odd fault",        '#', c_magenta, 0, tr_null,
mfb(noitem) | mfb(supports_roof) },
{ "paper wall",       '#', c_white,   0, tr_null,
mfb(bashable) | mfb(flammable) | mfb(noitem) },
{ "tree",	     '7', c_green,   0, tr_null,
mfb(flammable) | mfb(noitem) | mfb(supports_roof) },
{ "young tree",       '1', c_green,   0, tr_null,
mfb(transparent) | mfb(bashable) | mfb(flammable) | mfb(noitem) },
{ "underbrush",       '#', c_ltgreen, 6, tr_null,
mfb(transparent) | mfb(bashable) | mfb(diggable) | mfb(container) | mfb(rough) |
mfb(flammable) },
{ "shrub",            '#', c_green,   0, tr_null,
mfb(transparent) | mfb(bashable) | mfb(container) | mfb(flammable) },
{ "log",              '1', c_brown,   4, tr_null,
mfb(transparent) | mfb(flammable) | mfb(diggable) },
{ "root wall",        '#', c_brown,   0, tr_null,
mfb(noitem) | mfb(supports_roof) },
{ "wax wall",         '#', c_yellow,  0, tr_null,
mfb(container) | mfb(flammable) | mfb(noitem) | mfb(supports_roof) },
{ "wax floor",        '.', c_yellow,  2, tr_null,
mfb(transparent) | mfb(l_flammable) },
{ "picket fence",     '|', c_brown,   3, tr_null,
mfb(transparent) | mfb(diggable) | mfb(flammable) | mfb(noitem) | mfb(thin_obstacle) },
{ "picket fence",     '-', c_brown,   3, tr_null,
mfb(transparent) | mfb(diggable) | mfb(flammable) | mfb(noitem) | mfb(thin_obstacle) },
{ "railing",          '|', c_yellow,  3, tr_null,
mfb(transparent) | mfb(noitem) | mfb(thin_obstacle) },
{ "railing",          '-', c_yellow,  3, tr_null,
mfb(transparent) | mfb(noitem) | mfb(thin_obstacle) },
{ "marloss bush",     '1', c_dkgray,  0, tr_null,
mfb(transparent) | mfb(bashable) | mfb(flammable) },
{ "fungal bed",       '#', c_dkgray,  3, tr_null,
mfb(transparent) | mfb(l_flammable) | mfb(diggable) },
{ "fungal tree",      '7', c_dkgray,  0, tr_null,
mfb(flammable) | mfb(noitem) },
{ "shallow water",    '~', c_ltblue,  5, tr_null,
mfb(transparent) | mfb(liquid) | mfb(swimmable) },
{ "deep water",       '~', c_blue,    0, tr_null,
mfb(transparent) | mfb(liquid) | mfb(swimmable) },
{ "sewage",           '~', c_ltgreen, 6, tr_null,
mfb(transparent) | mfb(swimmable) },
{ "lava",             '~', c_red,     4, tr_lava,
mfb(transparent) | mfb(liquid) },
{ "bed",              '#', c_magenta, 5, tr_null,
mfb(transparent) | mfb(container) | mfb(flammable) },
{ "toilet",           '&', c_white,   0, tr_null,
mfb(transparent) | mfb(bashable) | mfb(l_flammable) },
{ "sandbox",          '#', c_yellow,  3, tr_null,
mfb(transparent) },
{ "slide",            '#', c_ltcyan,  4, tr_null,
mfb(transparent) },
{ "monkey bars",      '#', c_cyan,    4, tr_null,
mfb(transparent) },
{ "backboard",        '7', c_red,     0, tr_null,
mfb(transparent) },
{ "bench",            '#', c_brown,   3, tr_null,
mfb(transparent) | mfb(flammable) },
{ "table",            '#', c_red,     4, tr_null,
mfb(transparent) | mfb(flammable) },
{ "pool table",       '#', c_green,   4, tr_null,
mfb(transparent) | mfb(flammable) },
{ "gasoline pump",    '&', c_red,     0, tr_null,
mfb(transparent) | mfb(explodes) | mfb(noitem) },
{ "smashed gas pump", '&', c_ltred,   0, tr_null,
mfb(transparent) | mfb(noitem) },
{ "missile",          '#', c_ltblue,  0, tr_null,
mfb(explodes) | mfb(noitem) },
{ "blown-out missile",'#', c_ltgray,  0, tr_null,
mfb(noitem) },
{ "counter",	     '#', c_blue,    4, tr_null,
mfb(transparent) | mfb(flammable) },
{ "radio tower",      '&', c_ltgray,  0, tr_null,
mfb(noitem) },
{ "radio controls",   '6', c_green,   0, tr_null,
mfb(transparent) | mfb(bashable) | mfb(noitem) },
{ "broken console",   '6', c_ltgray,  0, tr_null,
mfb(transparent) | mfb(noitem) },
{ "computer console", '6', c_blue,    0, tr_null,
mfb(transparent) | mfb(console) | mfb(noitem) },
{ "sewage pipe",      '1', c_ltgray,  0, tr_null,
mfb(transparent) },
{ "sewage pump",      '&', c_ltgray,  0, tr_null,
mfb(noitem) },
{ "centrifuge",       '{', c_magenta, 0, tr_null,
mfb(transparent) },
{ "column",           '1', c_ltgray,  0, tr_null,
mfb(flammable) },
{ "refrigerator",     '{', c_ltcyan,  0, tr_null,
mfb(container) },
{ "dresser",          '{', c_brown,   0, tr_null,
mfb(transparent) | mfb(container) | mfb(flammable) },
{ "display rack",     '{', c_ltgray,  0, tr_null,
mfb(transparent) | mfb(container) | mfb(l_flammable) },
{ "book case",        '{', c_brown,   0, tr_null,
mfb(container) | mfb(flammable) },
{ "dumpster",	     '{', c_green,   0, tr_null,
mfb(container) },
{ "cloning vat",      '0', c_ltcyan,  0, tr_null,
mfb(transparent) | mfb(bashable) | mfb(container) | mfb(sealed) },
{ "crate",            '{', c_brown,   0, tr_null,
mfb(transparent) | mfb(bashable) | mfb(container) | mfb(sealed) |
mfb(flammable) },
{ "open crate",       '{', c_brown,   0, tr_null,
mfb(transparent) | mfb(bashable) | mfb(container) | mfb(flammable) },
{ "stairs down",      '>', c_yellow,  2, tr_null,
mfb(transparent) | mfb(goes_down) | mfb(container) },
{ "stairs up",        '<', c_yellow,  2, tr_null,
mfb(transparent) | mfb(goes_up) | mfb(container) },
{ "manhole",          '>', c_dkgray,  2, tr_null,
mfb(transparent) | mfb(goes_down) | mfb(container) },
{ "ladder",           '<', c_dkgray,  2, tr_null,
mfb(transparent) | mfb(goes_up) | mfb(container) },
{ "ladder",           '>', c_dkgray,  2, tr_null,
mfb(transparent) | mfb(goes_down) | mfb(container) },
{ "downward slope",   '>', c_brown,   2, tr_null,
mfb(transparent) | mfb(goes_down) | mfb(container) },
{ "upward slope",     '<', c_brown,   2, tr_null,
mfb(transparent) | mfb(goes_up) | mfb(container) },
{ "rope leading up",  '<', c_white,   2, tr_null,
mfb(transparent) | mfb(goes_up) },
{ "manhole cover",    '0', c_dkgray,  2, tr_null,
mfb(transparent) },
{ "card reader",	     '6', c_pink,    0, tr_null,	// Science
mfb(noitem) },
{ "card reader",	     '6', c_pink,    0, tr_null,	// Military
mfb(noitem) },
{ "broken card reader",'6',c_ltgray,  0, tr_null,
mfb(noitem) },
{ "slot machine",     '6', c_green,   0, tr_null,
mfb(bashable) | mfb(noitem) },
{ "elevator controls",'6', c_ltblue,  0, tr_null,
mfb(noitem) },
{ "powerless controls",'6',c_ltgray,  0, tr_null,
mfb(noitem) },
{ "elevator",         '.', c_magenta, 2, tr_null,
0 },
{ "dark pedestal",    '&', c_dkgray,  0, tr_null,
mfb(transparent) },
{ "light pedestal",   '&', c_white,   0, tr_null,
mfb(transparent) },
{ "red stone",        '#', c_red,     0, tr_null,
0 },
{ "green stone",      '#', c_green,   0, tr_null,
0 },
{ "blue stone",       '#', c_blue,    0, tr_null,
0 },
{ "red floor",        '.', c_red,     2, tr_null,
mfb(transparent) },
{ "green floor",      '.', c_green,   2, tr_null,
mfb(transparent) },
{ "blue floor",       '.', c_blue,    2, tr_null,
mfb(transparent) },
{ "yellow switch",    '6', c_yellow,  0, tr_null,
mfb(transparent) },
{ "cyan switch",      '6', c_cyan,    0, tr_null,
mfb(transparent) },
{ "purple switch",    '6', c_magenta, 0, tr_null,
mfb(transparent) },
{ "checkered switch", '6', c_white,   0, tr_null,
mfb(transparent) }

};

// from codegen.py3
static const char* JSON_transcode[num_terrain_types] = {
	"t_null",
	"t_hole",
	"t_dirt",
	"t_dirtmound",
	"t_pit_shallow",
	"t_pit",
	"t_pit_spiked",
	"t_rock_floor",
	"t_rubble",
	"t_wreckage",
	"t_grass",
	"t_metal_floor",
	"t_pavement",
	"t_pavement_y",
	"t_sidewalk",
	"t_floor",
	"t_grate",
	"t_slime",
	"t_bridge",
	"t_wall_half",
	"t_wall_wood",
	"t_wall_wood_chipped",
	"t_wall_wood_broken",
	"t_wall_v",
	"t_wall_h",
	"t_wall_metal_v",
	"t_wall_metal_h",
	"t_wall_glass_v",
	"t_wall_glass_h",
	"t_wall_glass_v_alarm",
	"t_wall_glass_h_alarm",
	"t_reinforced_glass_v",
	"t_reinforced_glass_h",
	"t_bars",
	"t_door_c",
	"t_door_b",
	"t_door_o",
	"t_door_locked",
	"t_door_locked_alarm",
	"t_door_frame",
	"t_door_boarded",
	"t_door_metal_c",
	"t_door_metal_o",
	"t_door_metal_locked",
	"t_door_glass_c",
	"t_door_glass_o",
	"t_bulletin",
	"t_portcullis",
	"t_window",
	"t_window_alarm",
	"t_window_empty",
	"t_window_frame",
	"t_window_boarded",
	"t_rock",
	"t_fault",
	"t_paper",
	"t_tree",
	"t_tree_young",
	"t_underbrush",
	"t_shrub",
	"t_log",
	"t_root_wall",
	"t_wax",
	"t_floor_wax",
	"t_fence_v",
	"t_fence_h",
	"t_railing_v",
	"t_railing_h",
	"t_marloss",
	"t_fungus",
	"t_tree_fungal",
	"t_water_sh",
	"t_water_dp",
	"t_sewage",
	"t_lava",
	"t_bed",
	"t_toilet",
	"t_sandbox",
	"t_slide",
	"t_monkey_bars",
	"t_backboard",
	"t_bench",
	"t_table",
	"t_pool_table",
	"t_gas_pump",
	"t_gas_pump_smashed",
	"t_missile",
	"t_missile_exploded",
	"t_counter",
	"t_radio_tower",
	"t_radio_controls",
	"t_console_broken",
	"t_console",
	"t_sewage_pipe",
	"t_sewage_pump",
	"t_centrifuge",
	"t_column",
	"t_fridge",
	"t_dresser",
	"t_rack",
	"t_bookcase",
	"t_dumpster",
	"t_vat",
	"t_crate_c",
	"t_crate_o",
	"t_stairs_down",
	"t_stairs_up",
	"t_manhole",
	"t_ladder_up",
	"t_ladder_down",
	"t_slope_down",
	"t_slope_up",
	"t_rope_up",
	"t_manhole_cover",
	"t_card_science",
	"t_card_military",
	"t_card_reader_broken",
	"t_slot_machine",
	"t_elevator_control",
	"t_elevator_control_off",
	"t_elevator",
	"t_pedestal_wyrm",
	"t_pedestal_temple",
	"t_rock_red",
	"t_rock_green",
	"t_rock_blue",
	"t_floor_red",
	"t_floor_green",
	"t_floor_blue",
	"t_switch_rg",
	"t_switch_gb",
	"t_switch_rb",
	"t_switch_even"
};

const char* JSON_key(ter_id src)
{
	if (src) return JSON_transcode[src];
	return 0;
}

using namespace cataclysm;

template<> ter_id discard<ter_id>::x = t_null;
template<> trap_id discard<trap_id>::x = tr_null;
template<> field discard<field>::x = field();

ter_id JSON_parse<ter_id>::operator()(const char* const src)
{
	if (!src || !src[0]) return t_null;
	size_t i = num_terrain_types;
	while(0 < --i) {
		if (!strcmp(JSON_transcode[i], src)) return (ter_id)(i);
	}
	return t_null;
}

void ter_t::init()
{
	if (!JSON::cache.count("tiles")) return;
	auto& j_tiles = JSON::cache["tiles"];
	auto keys = j_tiles.keys();
	JSON_parse<ter_id> parse;
	while (!keys.empty()) {
		auto& k = keys.back();
		if (const auto terrain_id = parse(k)) {
			auto& src = j_tiles[k];
			// invariant: src.is_scalar()
			tiles[terrain_id] = src.scalar();	// XXX \todo would be nice if this was a non-const reference return so std::move was an option
			j_tiles.unset(k);
		}
		keys.pop_back();	// reference k dies
	}
	if (j_tiles.empty()) JSON::cache.erase("tiles");	// reference j_tiles dies
}

#define SGN(a) (((a)<0) ? -1 : 1)
#define INBOUNDS(x, y) \
 (x >= 0 && x < SEEX * my_MAPSIZE && y >= 0 && y < SEEY * my_MAPSIZE)

enum astar_list {
 ASL_NONE,
 ASL_OPEN,
 ASL_CLOSED
};

vehicle* map::veh_at(int x, int y, int &part_num)
{
 if (!inbounds(x, y)) return NULL;    // Out-of-bounds - null vehicle
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 x %= SEEX;
 y %= SEEY;

 // must check 3x3 map chunks, as vehicle part may span to neighbour chunk
 // we presume that vehicles don't intersect (they shouldn't by any means)
 for (int mx = -1; mx <= 1; mx++) {
  for (int my = -1; my <= 1; my++) {
   int nonant1 = nonant + mx + my * my_MAPSIZE;
   if (nonant1 < 0 || nonant1 >= my_MAPSIZE * my_MAPSIZE)
    continue; // out of grid
   for (int i = 0; i < grid[nonant1]->vehicles.size(); i++) {
    vehicle *veh = &(grid[nonant1]->vehicles[i]);
    int part = veh->part_at (x - (veh->pos.x + mx * SEEX),
                             y - (veh->pos.y + my * SEEY));
    if (part >= 0) {
     part_num = part;
     return veh;
    }
   }
  }
 }
 return NULL;
}

vehicle* map::veh_at(int x, int y)
{
 int part = 0;
 vehicle *veh = veh_at(x, y, part);
 return veh;
}

void map::board_vehicle(game *g, int x, int y, player *p)
{
 if (!p) {
  debugmsg ("map::board_vehicle: null player");
  return;
 }

 int part = 0;
 vehicle *veh = veh_at(x, y, part);
 if (!veh) {
  debugmsg ("map::board_vehicle: vehicle not found");
  return;
 }

 int seat_part = veh->part_with_feature (part, vpf_seat);
 if (part < 0) {
  debugmsg ("map::board_vehicle: boarding %s (not seat)",
            veh->part_info(part).name);
  return;
 }
 if (veh->parts[seat_part].passenger) {
  player *psg = veh->get_passenger (seat_part);
  debugmsg ("map::board_vehicle: passenger (%s) is already there",
            psg ? psg->name.c_str() : "<null>");
  return;
 }
 veh->parts[seat_part].passenger = 1;
 p->posx = x;
 p->posy = y;
 p->in_vehicle = true;
 if (p == &g->u &&
     (x < SEEX * int(my_MAPSIZE / 2) || y < SEEY * int(my_MAPSIZE / 2) ||
      x >= SEEX * (1 + int(my_MAPSIZE / 2)) ||
      y >= SEEY * (1 + int(my_MAPSIZE / 2))   ))
  g->update_map(x, y);
}

void map::unboard_vehicle(game *g, int x, int y)
{
 int part = 0;
 vehicle *veh = veh_at(x, y, part);
 if (!veh) {
  debugmsg ("map::unboard_vehicle: vehicle not found");
  return;
 }
 int seat_part = veh->part_with_feature (part, vpf_seat, false);
 if (part < 0) {
  debugmsg ("map::unboard_vehicle: unboarding %s (not seat)",
            veh->part_info(part).name);
  return;
 }
 player *psg = veh->get_passenger(seat_part);
 if (!psg) {
  debugmsg ("map::unboard_vehicle: passenger not found");
  return;
 }
 psg->in_vehicle = false;
 psg->driving_recoil = 0;
 veh->parts[seat_part].passenger = 0;
 veh->skidding = true;
}

void map::destroy_vehicle (vehicle *veh)
{
 if (!veh) {
  debugmsg("map::destroy_vehicle was passed NULL");
  return;
 }
 int sm = veh->sm.x + veh->sm.y * my_MAPSIZE;
 for (int i = 0; i < grid[sm]->vehicles.size(); i++) {
  if (&(grid[sm]->vehicles[i]) == veh) {
   grid[sm]->vehicles.erase (grid[sm]->vehicles.begin() + i);
   return;
  }
 }
 debugmsg ("destroy_vehicle can't find it! sm=%d", sm);
}

bool map::displace_vehicle (game *g, int &x, int &y, int dx, int dy, bool test=false)
{
 int x2 = x + dx;
 int y2 = y + dy;
 int srcx = x;
 int srcy = y;
 int dstx = x2;
 int dsty = y2;

 if (!inbounds(srcx, srcy)) {
  debugmsg ("map::displace_vehicle: coords out of bounds %d,%d->%d,%d",
            srcx, srcy, dstx, dsty);
  return false;
 }

 int src_na = int(srcx / SEEX) + int(srcy / SEEY) * my_MAPSIZE;
 srcx %= SEEX;
 srcy %= SEEY;

 int dst_na = int(dstx / SEEX) + int(dsty / SEEY) * my_MAPSIZE;
 dstx %= SEEX;
 dsty %= SEEY;

 if (test) return src_na != dst_na;

 // first, let's find our position in current vehicles vector
 int our_i = -1;
 for (int i = 0; i < grid[src_na]->vehicles.size(); i++) {
  if (grid[src_na]->vehicles[i].pos.x == srcx &&
      grid[src_na]->vehicles[i].pos.y == srcy) {
   our_i = i;
   break;
  }
 }
 if (our_i < 0) {
  debugmsg ("displace_vehicle our_i=%d", our_i);
  return false;
 }
 // move the vehicle
 vehicle *veh = &(grid[src_na]->vehicles[our_i]);
 // don't let it go off grid
 if (!inbounds(x2, y2)) veh->stop();

 // record every passenger inside
 std::vector<int> psg_parts = veh->boarded_parts();
 std::vector<player *> psgs;
 for (int p = 0; p < psg_parts.size(); p++)
  psgs.push_back (veh->get_passenger (psg_parts[p]));

 int rec = abs(veh->velocity) / 5 / 100;

 bool need_update = false;
 int upd_x, upd_y;
 // move passengers
 for (int i = 0; i < psg_parts.size(); i++) {
  player *psg = psgs[i];
  int p = psg_parts[i];
  if (!psg) {
   const point origin(veh->global() + veh->parts[p].precalc_d[0]);
   debugmsg ("empty passenger part %d pcoord=%d,%d u=%d,%d?", p, 
             origin.x, origin.y, g->u.posx, g->u.posy);
   continue;
  }
  int trec = rec - psgs[i]->sklevel[sk_driving];
  if (trec < 0) trec = 0;
  // add recoil
  psg->driving_recoil = rec;
  // displace passenger taking in account vehicle movement (dx, dy)
  // and turning: precalc_dx/dy [0] contains previous frame direction,
  // and precalc_dx/dy[1] should contain next direction
  psg->posx += dx + veh->parts[p].precalc_d[1].x - veh->parts[p].precalc_d[0].x;
  psg->posy += dy + veh->parts[p].precalc_d[1].y - veh->parts[p].precalc_d[0].y;
  if (psg == &g->u) { // if passemger is you, we need to update the map
   need_update = true;
   upd_x = psg->posx;
   upd_y = psg->posy;
  }
 }
 for (auto& part : veh->parts) part.precalc_d[0] = part.precalc_d[1];

 veh->pos.x = dstx;
 veh->pos.y = dsty;
 if (src_na != dst_na) {
  vehicle veh1 = *veh;
  veh1.sm = point(int(x2 / SEEX),int(y2 / SEEY));
  grid[dst_na]->vehicles.push_back (veh1);
  grid[src_na]->vehicles.erase (grid[src_na]->vehicles.begin() + our_i);
 }

 x += dx;
 y += dy;

 bool was_update = false;
 if (need_update &&
     (upd_x < SEEX * int(my_MAPSIZE / 2) || upd_y < SEEY *int(my_MAPSIZE / 2) ||
      upd_x >= SEEX * (1+int(my_MAPSIZE / 2)) ||
      upd_y >= SEEY * (1+int(my_MAPSIZE / 2))   )) {
// map will shift, so adjust vehicle coords we've been passed
  if (upd_x < SEEX * int(my_MAPSIZE / 2))
   x += SEEX;
  else if (upd_x >= SEEX * (1+int(my_MAPSIZE / 2)))
   x -= SEEX;
  if (upd_y < SEEY * int(my_MAPSIZE / 2))
   y += SEEY;
  else if (upd_y >= SEEY * (1+int(my_MAPSIZE / 2)))
   y -= SEEY;
  g->update_map(upd_x, upd_y);
  was_update = true;
 }
 return (src_na != dst_na) || was_update;
}

void map::vehmove(game *g)
{
 // give vehicles movement points
 for (int i = 0; i < my_MAPSIZE; i++) {
  for (int j = 0; j < my_MAPSIZE; j++) {
   int sm = i + j * my_MAPSIZE;
   for (int v = 0; v < grid[sm]->vehicles.size(); v++) {
    vehicle *veh = &(grid[sm]->vehicles[v]);
    // velocity is ability to make more one-tile steps per turn
    veh->gain_moves (abs (veh->velocity));
   }
  }
 }
// move vehicles
 bool sm_change;
 int count = 0;
 do {
  sm_change = false;
  for (int i = 0; i < my_MAPSIZE; i++) {
   for (int j = 0; j < my_MAPSIZE; j++) {
    int sm = i + j * my_MAPSIZE;

    for (int v = 0; v < grid[sm]->vehicles.size(); v++) {
     vehicle *veh = &(grid[sm]->vehicles[v]);
     bool pl_ctrl = veh->player_in_control(&g->u);
     while (!sm_change && veh->moves > 0 && veh->velocity != 0) {
      int x = veh->pos.x + i * SEEX;
      int y = veh->pos.y + j * SEEY;
      if (has_flag(swimmable, x, y) &&
          move_cost_ter_only(x, y) == 0) { // deep water
       if (pl_ctrl) messages.add("Your %s sank.", veh->name.c_str());
       veh->unboard_all ();
// destroy vehicle (sank to nowhere)
       grid[sm]->vehicles.erase (grid[sm]->vehicles.begin() + v);
       v--;
       break;
      }
// one-tile step take some of movement
      int mpcost = 500 * move_cost_ter_only(i * SEEX + veh->pos.x,
                                            j * SEEY + veh->pos.y);
      veh->moves -= mpcost;

      if (!veh->valid_wheel_config()) { // not enough wheels
       veh->velocity += veh->velocity < 0 ? 2000 : -2000;
       for (int ep = 0; ep < veh->external_parts.size(); ep++) {
        int p = veh->external_parts[ep];
		const point origin(x + veh->parts[p].precalc_d[0].x, y + veh->parts[p].precalc_d[0].y);
        ter_id &pter = ter(origin.x, origin.y);
        if (pter == t_dirt || pter == t_grass)
         pter = t_dirtmound;
       }
      } // !veh->valid_wheel_config()

      if (veh->skidding && one_in(4)) // might turn uncontrollably while skidding
       veh->move.init (veh->move.dir() +
                       (one_in(2) ? -15 * rng(1, 3) : 15 * rng(1, 3)));
      else if (pl_ctrl && rng(0, 4) > g->u.sklevel[sk_driving] && one_in(20)) {
       messages.add("You fumble with the %s's controls.", veh->name.c_str());
       veh->turn (one_in(2) ? -15 : 15);
      }
 // eventually send it skidding if no control
      if (!veh->boarded_parts().size() && one_in (10))
       veh->skidding = true;
      tileray mdir; // the direction we're moving
      if (veh->skidding) // if skidding, it's the move vector
       mdir = veh->move;
      else if (veh->turn_dir != veh->face.dir())
       mdir.init (veh->turn_dir); // driver turned vehicle, get turn_dir
      else
       mdir = veh->face;          // not turning, keep face.dir
      mdir.advance (veh->velocity < 0? -1 : 1);
      int dx = mdir.dx();           // where do we go
      int dy = mdir.dy();           // where do we go
      bool can_move = true;
// calculate parts' mount points @ next turn (put them into precalc[1])
      veh->precalc_mounts(1, veh->skidding ? veh->turn_dir : mdir.dir());

      int imp = 0;
// find collisions
      for (int ep = 0; ep < veh->external_parts.size(); ep++) {
       int p = veh->external_parts[ep];
// coords of where part will go due to movement (dx/dy)
// and turning (precalc_dx/dy [1])
	   const point ds(x + dx + veh->parts[p].precalc_d[1].x, y + dy + veh->parts[p].precalc_d[1].y);
       if (can_move) imp += veh->part_collision (x, y, p, ds);
       if (veh->velocity == 0) can_move = false;
       if (!can_move) break;
      }

      int coll_turn = 0;
      if (imp > 0) {
       if (imp > 100) veh->damage_all(imp / 20, imp / 10, 1);// shake veh because of collision
       std::vector<int> ppl = veh->boarded_parts();
       int vel2 = imp * k_mvel * 100 / (veh->total_mass() / 8);
       for (int ps = 0; ps < ppl.size(); ps++) {
        player *psg = veh->get_passenger (ppl[ps]);
        if (!psg) {
         debugmsg ("throw passenger: empty passenger at part %d", ppl[ps]);
         continue;
        }
        int throw_roll = rng (vel2/100, vel2/100 * 2);
        int psblt = veh->part_with_feature (ppl[ps], vpf_seatbelt);
        int sb_bonus = psblt >= 0? veh->part_info(psblt).bonus : 0;
        bool throw_it = throw_roll > (psg->str_cur + sb_bonus) * 3;
        std::string psgname;
		const char* psgverb;
        if (psg == &g->u) {
         psgname = "You";
         psgverb = "were";
        } else {
         psgname = psg->name;
         psgverb = "was";
        }
        if (throw_it) {
         if (psgname.length())
          messages.add("%s %s hurled from the %s's seat by the power of impact!",
                      psgname.c_str(), psgverb, veh->name.c_str());
		 const point origin(x + veh->parts[ppl[ps]].precalc_d[0].x, y + veh->parts[ppl[ps]].precalc_d[0].y);
         g->m.unboard_vehicle(g, origin.x, origin.y);
         g->fling_player_or_monster(psg, 0, mdir.dir() + rng(0, 60) - 30,
                                    (vel2/100 - sb_bonus < 10 ? 10 :
                                     vel2/100 - sb_bonus));
        } else if (veh->part_with_feature (ppl[ps], vpf_controls) >= 0) {
         int lose_ctrl_roll = rng (0, imp);
         if (lose_ctrl_roll > psg->dex_cur * 2 + psg->sklevel[sk_driving] * 3) {
          if (psgname.length())
           messages.add("%s lose%s control of the %s.", psgname.c_str(),
                       (psg == &g->u ? "" : "s"), veh->name.c_str());
          int turn_amount = (rng (1, 3) * sqrt (vel2) / 2) / 15;
          if (turn_amount < 1) turn_amount = 1;
          turn_amount *= 15;
          if (turn_amount > 120) turn_amount = 120;
          //veh->skidding = true;
          //veh->turn (one_in (2)? turn_amount : -turn_amount);
          coll_turn = one_in (2)? turn_amount : -turn_amount;
         }
        }
       }
      }
// now we're gonna handle traps we're standing on (if we're still moving).
// this is done here before displacement because
// after displacement veh reference would be invdalid.
// damn references!
      if (can_move) {
       for (int ep = 0; ep < veh->external_parts.size(); ep++) {
        int p = veh->external_parts[ep];
		const point origin(x + veh->parts[p].precalc_d[0].x, y + veh->parts[p].precalc_d[0].y);
        if (veh->part_flag(p, vpf_wheel) && one_in(2))
         if (displace_water (origin.x, origin.y) && pl_ctrl)
          messages.add("You hear a splash!");
        veh->handle_trap(origin.x, origin.y, p);
       }
      }

      int last_turn_dec = 1;
      if (veh->last_turn < 0) {
       veh->last_turn += last_turn_dec;
       if (veh->last_turn > -last_turn_dec) veh->last_turn = 0;
      } else if (veh->last_turn > 0) {
       veh->last_turn -= last_turn_dec;
       if (veh->last_turn < last_turn_dec) veh->last_turn = 0;
      }
      int slowdown = veh->skidding? 200 : 20; // mph lost per tile when coasting
      float kslw = (0.1 + veh->k_dynamics()) / ((0.1) + veh->k_mass());
      slowdown = (int) (slowdown * kslw);
      if (veh->velocity < 0)
       veh->velocity += slowdown;
      else
       veh->velocity -= slowdown;
      if (abs(veh->velocity) < 100)
       veh->stop();

      if (pl_ctrl) {
// a bit of delay for animation
       timespec ts;   // Timespec for the animation
       ts.tv_sec = 0;
       ts.tv_nsec = 50000000;
       nanosleep (&ts, 0);
      }

      if (can_move) {
// accept new direction
       if (veh->skidding)
        veh->face.init (veh->turn_dir);
       else
        veh->face = mdir;
       veh->move = mdir;
       if (coll_turn) {
        veh->skidding = true;
        veh->turn (coll_turn);
       }
// accept new position
// if submap changed, we need to process grid from the beginning.
       sm_change = displace_vehicle (g, x, y, dx, dy);
      } else // can_move
       veh->stop();
// redraw scene
      g->draw();
      if (sm_change)
       break;
     } // while (veh->moves
     if (sm_change)
      break;
    } //for v
    if (sm_change)
     break;
   } // for j
   if (sm_change)
    break;
  } // for i
  count++;
//        if (count > 3)
//            debugmsg ("vehmove count:%d", count);
  if (count > 10)
   break;
 } while (sm_change);
}

bool map::displace_water (int x, int y)
{
    if (move_cost_ter_only(x, y) > 0 && has_flag(swimmable, x, y)) // shallow water
    { // displace it
        int dis_places = 0, sel_place = 0;
        for (int pass = 0; pass < 2; pass++)
        { // we do 2 passes.
        // first, count how many non-water places around
        // then choose one within count and fill it with water on second pass
            if (pass)
            {
                sel_place = rng (0, dis_places - 1);
                dis_places = 0;
            }
            for (int tx = -1; tx <= 1; tx++)
                for (int ty = -1; ty <= 1; ty++)
                {
                    if ((!tx && !ty) || move_cost_ter_only(x + tx, y + ty) == 0)
                        continue;
                    ter_id ter0 = ter (x + tx, y + ty);
                    if (ter0 == t_water_sh ||
                        ter0 == t_water_dp)
                        continue;
                    if (pass && dis_places == sel_place)
                    {
                        ter (x + tx, y + ty) = t_water_sh;
                        ter (x, y) = t_dirt;
                        return true;
                    }
                    dis_places++;
                }
        }
    }
    return false;
}

ter_id& map::ter(int x, int y)
{
 if (!INBOUNDS(x, y)) return (discard<ter_id>::x = t_null);	// Out-of-bounds - null terrain 
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 x %= SEEX;
 y %= SEEY;
 return grid[nonant]->ter[x][y];
}

std::string map::tername(int x, int y)
{
 return ter_t::list[ter(x, y)].name;
}

std::string map::features(int x, int y)
{
// This is used in an info window that is 46 characters wide, and is expected
// to take up one line.  So, make sure it does that.
 std::string ret;
 if (has_flag(bashable, x, y))
  ret += "Smashable. ";	// 11 chars (running total)
 if (has_flag(diggable, x, y))
  ret += "Diggable. ";	// 21 chars
 if (has_flag(rough, x, y))
  ret += "Rough. ";	// 28 chars
 if (has_flag(sharp, x, y))
  ret += "Sharp. ";	// 35 chars
 return ret;
}

int map::move_cost(int x, int y)
{
 int vpart = -1;
 vehicle *veh = veh_at(x, y, vpart);
 if (veh) {  // moving past vehicle cost
  int dpart = veh->part_with_feature(vpart, vpf_obstacle);
  if (dpart >= 0 &&
      (!veh->part_flag(dpart, vpf_openable) || !veh->parts[dpart].open))
   return 0;
  else
   return 8;
 }
 return ter_t::list[ter(x, y)].movecost;
}

int map::move_cost_ter_only(int x, int y)
{
 return ter_t::list[ter(x, y)].movecost;
}

bool map::trans(int x, int y)
{
 // Control statement is a problem. Normally returning false on an out-of-bounds
 // is how we stop rays from going on forever.  Instead we'll have to include
 // this check in the ray loop.
 int vpart = -1;
 vehicle *veh = veh_at(x, y, vpart);
 bool tertr;
 if (veh) {
  tertr = !veh->part_flag(vpart, vpf_opaque) || veh->parts[vpart].hp <= 0;
  if (!tertr) {
   int dpart = veh->part_with_feature(vpart, vpf_openable);
   if (dpart >= 0 && veh->parts[dpart].open)
    tertr = true; // open opaque door
  }
 } else
  tertr = ter_t::list[ter(x, y)].flags & mfb(transparent);
 return tertr &&
        (field_at(x, y).type == 0 ||	// Fields may obscure the view, too
        fieldlist[field_at(x, y).type].transparent[field_at(x, y).density - 1]);
}

bool map::has_flag(t_flag flag, int x, int y)
{
 if (flag == bashable) {
  int vpart;
  vehicle *veh = veh_at(x, y, vpart);
  if (veh && veh->parts[vpart].hp > 0 && // if there's a vehicle part here...
      veh->part_with_feature (vpart, vpf_obstacle) >= 0) {// & it is obstacle...
   int p = veh->part_with_feature (vpart, vpf_openable);
   if (p < 0 || !veh->parts[p].open) // and not open door
    return true;
  }
 }
 return ter_t::list[ter(x, y)].flags & mfb(flag);
}

bool map::has_flag_ter_only(t_flag flag, int x, int y)
{
 return ter_t::list[ter(x, y)].flags & mfb(flag);
}

bool map::is_destructable(int x, int y)
{
 return (has_flag(bashable, x, y) ||
         (move_cost(x, y) == 0 && !has_flag(liquid, x, y)));
}

bool map::is_destructable_ter_only(int x, int y)
{
 return (has_flag_ter_only(bashable, x, y) ||
         (move_cost_ter_only(x, y) == 0 && !has_flag(liquid, x, y)));
}

bool map::is_outside(int x, int y)
{
 bool out = (
         ter(x    , y    ) != t_floor && ter(x - 1, y - 1) != t_floor &&
         ter(x - 1, y    ) != t_floor && ter(x - 1, y + 1) != t_floor &&
         ter(x    , y - 1) != t_floor && ter(x    , y    ) != t_floor &&
         ter(x    , y + 1) != t_floor && ter(x + 1, y - 1) != t_floor &&
         ter(x + 1, y    ) != t_floor && ter(x + 1, y + 1) != t_floor &&
         ter(x    , y    ) != t_floor_wax &&
         ter(x - 1, y - 1) != t_floor_wax &&
         ter(x - 1, y    ) != t_floor_wax &&
         ter(x - 1, y + 1) != t_floor_wax &&
         ter(x    , y - 1) != t_floor_wax &&
         ter(x    , y    ) != t_floor_wax &&
         ter(x    , y + 1) != t_floor_wax &&
         ter(x + 1, y - 1) != t_floor_wax &&
         ter(x + 1, y    ) != t_floor_wax &&
         ter(x + 1, y + 1) != t_floor_wax   );
 if (out) {
  int vpart;
  vehicle *veh = veh_at (x, y, vpart);
  if (veh && veh->is_inside(vpart))
   out = false;
 }
 return out;
}

bool map::flammable_items_at(int x, int y)
{
 for (int i = 0; i < i_at(x, y).size(); i++) {
  item *it = &(i_at(x, y)[i]);
  if (it->made_of(PAPER) || it->made_of(WOOD) || it->made_of(COTTON) ||
      it->made_of(POWDER) || it->made_of(VEGGY) || it->is_ammo() ||
      it->type->id == itm_whiskey || it->type->id == itm_vodka ||
      it->type->id == itm_rum || it->type->id == itm_tequila)
   return true;
 }
 return false;
}

point map::random_outdoor_tile()
{
 std::vector<point> options;
 for (int x = 0; x < SEEX * my_MAPSIZE; x++) {
  for (int y = 0; y < SEEY * my_MAPSIZE; y++) {
   if (is_outside(x, y))
    options.push_back(point(x, y));
  }
 }
 if (options.empty()) // Nowhere is outdoors!
  return point(-1, -1);

 return options[rng(0, options.size() - 1)];
}

bool map::bash(int x, int y, int str, int *res)
{
	std::string discard;
	return bash(x, y, str, discard, res);
}

bool map::bash(int x, int y, int str, std::string &sound, int *res)
{
 sound = "";
 bool smashed_web = false;
 if (field_at(x, y).type == fd_web) {
  smashed_web = true;
  remove_field(x, y);
 }

 for (int i = 0; i < i_at(x, y).size(); i++) {	// Destroy glass items (maybe)
  if (i_at(x, y)[i].made_of(GLASS) && one_in(2)) {
   if (sound == "")
    sound = "A " + i_at(x, y)[i].tname() + " shatters!  ";
   else
    sound = "Some items shatter!  ";
   for (int j = 0; j < i_at(x, y)[i].contents.size(); j++)
    i_at(x, y).push_back(i_at(x, y)[i].contents[j]);
   i_rem(x, y, i);
   i--;
  }
 }

 int result = -1;
 int vpart;
 vehicle *veh = veh_at(x, y, vpart);
 if (veh) {
  veh->damage (vpart, str, 1);
  result = str;
  sound += "crash!";
  return true;
 }

 switch (ter(x, y)) {
 case t_wall_wood:
  result = rng(0, 120);
  if (res) *res = result;
  if (str >= result && str >= rng(0, 120)) {
   sound += "crunch!";
   ter(x, y) = t_wall_wood_chipped;
   const int num_boards = rng(0, 2);
   for (int i = 0; i < num_boards; i++) add_item(x, y, item::types[itm_2x4], 0);
   return true;
  } else {
   sound += "whump!";
   return true;
  }
  break;

 case t_wall_wood_chipped:
  result = rng(0, 100);
  if (res) *res = result;
  if (str >= result && str >= rng(0, 100)) {
   sound += "crunch!";
   ter(x, y) = t_wall_wood_broken;
   const int num_boards = rng(3, 8);
   for (int i = 0; i < num_boards; i++) add_item(x, y, item::types[itm_2x4], 0);
   return true;
  } else {
   sound += "whump!";
   return true;
  }
  break;

 case t_wall_wood_broken:
  result = rng(0, 80);
  if (res) *res = result;
  if (str >= result && str >= rng(0, 80)) {
   sound += "crash!";
   ter(x, y) = t_dirt;
   const int num_boards = rng(4, 10);
   for (int i = 0; i < num_boards; i++) add_item(x, y, item::types[itm_2x4], 0);
   return true;
  } else {
   sound += "whump!";
   return true;
  }
  break;

 case t_door_c:
 case t_door_locked:
 case t_door_locked_alarm:
  result = rng(0, 40);
  if (res) *res = result;
  if (str >= result) {
   sound += "smash!";
   ter(x, y) = t_door_b;
   return true;
  } else {
   sound += "whump!";
   return true;
  }
  break;

 case t_door_b:
  result = rng(0, 30);
  if (res) *res = result;
  if (str >= result) {
   sound += "crash!";
   ter(x, y) = t_door_frame;
   const int num_boards = rng(2, 6);
   for (int i = 0; i < num_boards; i++) add_item(x, y, item::types[itm_2x4], 0);
   return true;
  } else {
   sound += "wham!";
   return true;
  }
  break;

 case t_window:
 case t_window_alarm:
  result = rng(0, 6);
  if (res) *res = result;
  if (str >= result) {
   sound += "glass breaking!";
   ter(x, y) = t_window_frame;
   return true;
  } else {
   sound += "whack!";
   return true;
  }
  break;

 case t_door_boarded:
  result = dice(3, 50);
  if (res) *res = result;
  if (str >= result) {
   sound += "crash!";
   ter(x, y) = t_door_frame;
   const int num_boards = rng(0, 2);
   for (int i = 0; i < num_boards; i++) add_item(x, y, item::types[itm_2x4], 0);
   return true;
  } else {
   sound += "wham!";
   return true;
  }
  break;

 case t_window_boarded:
  result = dice(3, 30);
  if (res) *res = result;
  if (str >= result) {
   sound += "crash!";
   ter(x, y) = t_window_frame;
   const int num_boards = rng(0, 2) * rng(0, 1);
   for (int i = 0; i < num_boards; i++) add_item(x, y, item::types[itm_2x4], 0);
   return true;
  } else {
   sound += "wham!";
   return true;
  }
  break;

 case t_paper:
  result = dice(2, 6) - 2;
  if (res) *res = result;
  if (str >= result) {
   sound += "rrrrip!";
   ter(x, y) = t_dirt;
   return true;
  } else {
   sound += "slap!";
   return true;
  }
  break;

 case t_toilet:
  result = dice(8, 4) - 8;
  if (res) *res = result;
  if (str >= result) {
   sound += "porcelain breaking!";
   ter(x, y) = t_rubble;
   return true;
  } else {
   sound += "whunk!";
   return true;
  }
  break;

 case t_dresser:
 case t_bookcase:
  result = dice(3, 45);
  if (res) *res = result;
  if (str >= result) {
   sound += "smash!";
   ter(x, y) = t_floor;
   const int num_boards = rng(4, 12);
   for (int i = 0; i < num_boards; i++) add_item(x, y, item::types[itm_2x4], 0);
   return true;
  } else {
   sound += "whump.";
   return true;
  }
  break;

 case t_wall_glass_h:
 case t_wall_glass_v:
 case t_wall_glass_h_alarm:
 case t_wall_glass_v_alarm:
 case t_door_glass_c:
  result = rng(0, 20);
  if (res) *res = result;
  if (str >= result) {
   sound += "glass breaking!";
   ter(x, y) = t_floor;
   return true;
  } else {
   sound += "whack!";
   return true;
  }
  break;

 case t_reinforced_glass_h:
 case t_reinforced_glass_v:
  result = rng(60, 100);
  if (res) *res = result;
  if (str >= result) {
   sound += "glass breaking!";
   ter(x, y) = t_floor;
   return true;
  } else {
   sound += "whack!";
   return true;
  }
  break;

 case t_tree_young:
  result = rng(0, 50);
  if (res) *res = result;
  if (str >= result) {
   sound += "crunch!";
   ter(x, y) = t_underbrush;
   const int num_sticks = rng(0, 3);
   for (int i = 0; i < num_sticks; i++) add_item(x, y, item::types[itm_stick], 0);
   return true;
  } else {
   sound += "whack!";
   return true;
  }
  break;

 case t_underbrush:
  result = rng(0, 30);
  if (res) *res = result;
  if (str >= result && !one_in(4)) {
   sound += "crunch.";
   ter(x, y) = t_dirt;
   return true;
  } else {
   sound += "brush.";
   return true;
  }
  break;

 case t_shrub:
  if (str >= rng(0, 30) && str >= rng(0, 30) && str >= rng(0, 30) && one_in(2)){
   sound += "crunch.";
   ter(x, y) = t_underbrush;
   return true;
  } else {
   sound += "brush.";
   return true;
  }
  break;

 case t_marloss:
  result = rng(0, 40);
  if (res) *res = result;
  if (str >= result) {
   sound += "crunch!";
   ter(x, y) = t_fungus;
   return true;
  } else {
   sound += "whack!";
   return true;
  }
  break;

 case t_vat:
  result = dice(2, 20);
  if (res) *res = result;
  if (str >= result) {
   sound += "ker-rash!";
   ter(x, y) = t_floor;
   return true;
  } else {
   sound += "plunk.";
   return true;
  }
 case t_crate_c:
 case t_crate_o:
  result = dice(4, 20);
  if (res) *res = result;
  if (str >= result) {
   sound += "smash";
   ter(x, y) = t_dirt;
   const int num_boards = rng(1, 5);
   for (int i = 0; i < num_boards; i++) add_item(x, y, item::types[itm_2x4], 0);
   return true;
  } else {
   sound += "wham!";
   return true;
  }
 }
 if (res) *res = result;
 if (move_cost(x, y) == 0) {
  sound += "thump!";
  return true;
 }
 return smashed_web;// If we kick empty space, the action is cancelled
}

// map::destroy is only called (?) if the terrain is NOT bashable.
void map::destroy(game *g, int x, int y, bool makesound)
{
 switch (ter(x, y)) {

 case t_gas_pump:
  if (makesound && one_in(3))
   g->explosion(x, y, 40, 0, true);
  else {
   for (int i = x - 2; i <= x + 2; i++) {
    for (int j = y - 2; j <= y + 2; j++) {
     if (move_cost(i, j) > 0 && one_in(3))
      add_item(i, j, item::types[itm_gasoline], 0);
     if (move_cost(i, j) > 0 && one_in(6))
      add_item(i, j, item::types[itm_steel_chunk], 0);
    }
   }
  }
  ter(x, y) = t_rubble;
  break;

 case t_door_c:
 case t_door_b:
 case t_door_locked:
 case t_door_boarded:
  ter(x, y) = t_door_frame;
  for (int i = x - 2; i <= x + 2; i++) {
   for (int j = y - 2; j <= y + 2; j++) {
    if (move_cost(i, j) > 0 && one_in(6))
     add_item(i, j, item::types[itm_2x4], 0);
   }
  }
  break;

 case t_wall_v:
 case t_wall_h:
  for (int i = x - 2; i <= x + 2; i++) {
   for (int j = y - 2; j <= y + 2; j++) {
    if (move_cost(i, j) > 0 && one_in(5))
     add_item(i, j, item::types[itm_rock], 0);
    if (move_cost(i, j) > 0 && one_in(4))
     add_item(i, j, item::types[itm_2x4], 0);
   }
  }
  ter(x, y) = t_rubble;
  break;

 default:
  if (makesound && has_flag(explodes, x, y) && one_in(2))
   g->explosion(x, y, 40, 0, true);
  ter(x, y) = t_rubble;
 }

 if (makesound) g->sound(x, y, 40, "SMASH!!");
}

void map::shoot(game *g, int x, int y, int &dam, bool hit_items, unsigned flags)
{
 if (dam < 0) return;

 if (has_flag(alarmed, x, y) && !g->event_queued(EVENT_WANTED)) {
  g->sound(g->u.posx, g->u.posy, 30, "An alarm sounds!");
  g->add_event(EVENT_WANTED, int(messages.turn) + 300, 0, g->lev.x, g->lev.y);
 }

 int vpart;
 vehicle *veh = veh_at(x, y, vpart);
 if (veh) {
  bool inc = (flags & mfb(IF_AMMO_INCENDIARY) || flags & mfb(IF_AMMO_FLAME));
  dam = veh->damage (vpart, dam, inc? 2 : 0, hit_items);
 }

 switch (ter(x, y)) {

 case t_wall_wood_broken:
 case t_door_b:
  if (hit_items || one_in(8)) {	// 1 in 8 chance of hitting the door
   dam -= rng(20, 40);
   if (dam > 0)
    ter(x, y) = t_dirt;
  } else
   dam -= rng(0, 1);
  break;


 case t_door_c:
 case t_door_locked:
 case t_door_locked_alarm:
  dam -= rng(15, 30);
  if (dam > 0)
   ter(x, y) = t_door_b;
  break;

 case t_door_boarded:
  dam -= rng(15, 35);
  if (dam > 0)
   ter(x, y) = t_door_b;
  break;

 case t_window:
 case t_window_alarm:
  dam -= rng(0, 5);
  ter(x, y) = t_window_frame;
  break;

 case t_window_boarded:
  dam -= rng(10, 30);
  if (dam > 0)
   ter(x, y) = t_window_frame;
  break;

 case t_wall_glass_h:
 case t_wall_glass_v:
 case t_wall_glass_h_alarm:
 case t_wall_glass_v_alarm:
  dam -= rng(0, 8);
  ter(x, y) = t_floor;
  break;

 case t_paper:
  dam -= rng(4, 16);
  if (dam > 0)
   ter(x, y) = t_dirt;
  if (flags & mfb(IF_AMMO_INCENDIARY))
   add_field(g, x, y, fd_fire, 1);
  break;

 case t_gas_pump:
  if (hit_items || one_in(3)) {
   if (dam > 15) {
    if (flags & mfb(IF_AMMO_INCENDIARY) || flags & mfb(IF_AMMO_FLAME))
     g->explosion(x, y, 40, 0, true);
    else {
     for (int i = x - 2; i <= x + 2; i++) {
      for (int j = y - 2; j <= y + 2; j++) {
       if (move_cost(i, j) > 0 && one_in(3))
        add_item(i, j, item::types[itm_gasoline], 0);
      }
     }
    }
    ter(x, y) = t_gas_pump_smashed;
   }
   dam -= 60;
  }
  break;

 case t_vat:
  if (dam >= 10) {
   g->sound(x, y, 15, "ke-rash!");
   ter(x, y) = t_floor;
  } else
   dam = 0;
  break;

 default:
  if (move_cost(x, y) == 0 && !trans(x, y))
   dam = 0;	// TODO: Bullets can go through some walls?
  else
   dam -= (rng(0, 1) * rng(0, 1) * rng(0, 1));
 }

 if (flags & mfb(IF_AMMO_TRAIL) && !one_in(4))
  add_field(g, x, y, fd_smoke, rng(1, 2));

// Set damage to 0 if it's less
 if (dam < 0)
  dam = 0;

// Check fields?
 field *fieldhit = &(field_at(x, y));
 switch (fieldhit->type) {
  case fd_web:
   if (flags & mfb(IF_AMMO_INCENDIARY) || flags & mfb(IF_AMMO_FLAME))
    add_field(g, x, y, fd_fire, fieldhit->density - 1);
   else if (dam > 5 + fieldhit->density * 5 && one_in(5 - fieldhit->density)) {
    dam -= rng(1, 2 + fieldhit->density * 2);
    remove_field(x, y);
   }
   break;
 }

// Now, destroy items on that tile.

 if ((move_cost(x, y) == 2 && !hit_items) || !INBOUNDS(x, y))
  return;	// Items on floor-type spaces won't be shot up.

 bool destroyed;
 for (int i = 0; i < i_at(x, y).size(); i++) {
  destroyed = false;
  switch (i_at(x, y)[i].type->m1) {
   case GLASS:
   case PAPER:
    if (dam > rng(2, 8) && one_in(i_at(x, y)[i].volume()))
     destroyed = true;
    break;
   case PLASTIC:
    if (dam > rng(2, 10) && one_in(i_at(x, y)[i].volume() * 3))
     destroyed = true;
    break;
   case VEGGY:
   case FLESH:
    if (dam > rng(10, 40))
     destroyed = true;
    break;
   case COTTON:
   case WOOL:
    i_at(x, y)[i].damage++;
    if (i_at(x, y)[i].damage >= 5)
     destroyed = true;
    break;
  }
  if (destroyed) {
   for (int j = 0; j < i_at(x, y)[i].contents.size(); j++)
    i_at(x, y).push_back(i_at(x, y)[i].contents[j]);
   i_rem(x, y, i);
   i--;
  }
 }
}

bool map::hit_with_acid(game *g, int x, int y)
{
 if (move_cost(x, y) != 0)
  return false; // Didn't hit the tile!

 switch (ter(x, y)) {
  case t_wall_glass_v:
  case t_wall_glass_h:
  case t_wall_glass_v_alarm:
  case t_wall_glass_h_alarm:
  case t_vat:
   ter(x, y) = t_floor;
   break;

  case t_door_c:
  case t_door_locked:
  case t_door_locked_alarm:
   if (one_in(3))
    ter(x, y) = t_door_b;
   break;

  case t_door_b:
   if (one_in(4))
    ter(x, y) = t_door_frame;
   else
    return false;
   break;

  case t_window:
  case t_window_alarm:
   ter(x, y) = t_window_empty;
   break;

  case t_wax:
   ter(x, y) = t_floor_wax;
   break;

  case t_toilet:
  case t_gas_pump:
  case t_gas_pump_smashed:
   return false;

  case t_card_science:
  case t_card_military:
   ter(x, y) = t_card_reader_broken;
   break;
 }

 return true;
}

void map::marlossify(int x, int y)
{
 int type = rng(1, 9);
 switch (type) {
  case 1:
  case 2:
  case 3:
  case 4: ter(x, y) = t_fungus;      break;
  case 5:
  case 6:
  case 7: ter(x, y) = t_marloss;     break;
  case 8: ter(x, y) = t_tree_fungal; break;
  case 9: ter(x, y) = t_slime;       break;
 }
}

bool map::open_door(int x, int y, bool inside)
{
 if (ter(x, y) == t_door_c) {
  ter(x, y) = t_door_o;
  return true;
 } else if (ter(x, y) == t_door_metal_c) {
  ter(x, y) = t_door_metal_o;
  return true;
 } else if (ter(x, y) == t_door_glass_c) {
  ter(x, y) = t_door_glass_o;
  return true;
 } else if (inside &&
            (ter(x, y) == t_door_locked || ter(x, y) == t_door_locked_alarm)) {
  ter(x, y) = t_door_o;
  return true;
 }
 return false;
}

void map::translate(ter_id from, ter_id to)
{
 if (from == to) {
  debugmsg("map::translate %s => %s", ter_t::list[from].name.c_str(), ter_t::list[from].name.c_str());
  return;
 }
 for (int x = 0; x < SEEX * my_MAPSIZE; x++) {
  for (int y = 0; y < SEEY * my_MAPSIZE; y++) {
   if (ter(x, y) == from)
    ter(x, y) = to;
  }
 }
}

bool map::close_door(int x, int y)
{
 if (ter(x, y) == t_door_o) {
  ter(x, y) = t_door_c;
  return true;
 } else if (ter(x, y) == t_door_metal_o) {
  ter(x, y) = t_door_metal_c;
  return true;
 } else if (ter(x, y) == t_door_glass_o) {
  ter(x, y) = t_door_glass_c;
  return true;
 }
 return false;
}

int& map::radiation(int x, int y)
{
 if (!INBOUNDS(x, y)) return discard<int>::x = 0;
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 x %= SEEX;
 y %= SEEY;
 return grid[nonant]->rad[x][y];
}

std::vector<item>& map::i_at(int x, int y)
{
 if (!INBOUNDS(x, y)) {
  discard<std::vector<item> >::x.clear();
  return discard<std::vector<item> >::x;
 }
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 x %= SEEX;
 y %= SEEY;
 return grid[nonant]->itm[x][y];
}

item map::water_from(int x, int y)
{
 item ret(item::types[itm_water], 0);
 if (ter(x, y) == t_water_sh && one_in(3))
  ret.poison = rng(1, 4);
 else if (ter(x, y) == t_water_dp && one_in(4))
  ret.poison = rng(1, 4);
 else if (ter(x, y) == t_sewage)
  ret.poison = rng(1, 7);
 else if (ter(x, y) == t_toilet && !one_in(3))
  ret.poison = rng(1, 3);

 return ret;
}

void map::i_rem(int x, int y, int index)
{
 auto& stack = i_at(x, y);
 if (index > stack.size() - 1) return;
 stack.erase(stack.begin() + index);
}

void map::i_clear(int x, int y)
{
 i_at(x, y).clear();
}

point map::find_item(item *it) const
{
 point ret;
 for (ret.x = 0; ret.x < SEEX * my_MAPSIZE; ret.x++) {
  for (ret.y = 0; ret.y < SEEY * my_MAPSIZE; ret.y++) {
   for (int i = 0; i < i_at(ret.x, ret.y).size(); i++) {
    if (it == &i_at(ret.x, ret.y)[i])
     return ret;
   }
  }
 }
 ret.x = -1;
 ret.y = -1;
 return ret;
}

void map::add_item(int x, int y, const itype* type, int birthday)
{
 if (type->is_style()) return;
 item tmp(type, birthday);
 tmp = tmp.in_its_container();
 if (tmp.made_of(LIQUID) && has_flag(swimmable, x, y)) return;
 add_item(x, y, tmp);
}

void map::add_item(int x, int y, item new_item)
{
 if (new_item.is_style())
  return;
 if (!INBOUNDS(x, y))
  return;
 if (new_item.made_of(LIQUID) && has_flag(swimmable, x, y))
  return;
 if (has_flag(noitem, x, y) || i_at(x, y).size() >= 26) {// Too many items there
  std::vector<point> okay;
  for (int i = x - 1; i <= x + 1; i++) {
   for (int j = y - 1; j <= y + 1; j++) {
    if (INBOUNDS(i, j) && move_cost(i, j) > 0 && !has_flag(noitem, i, j) &&
        i_at(i, j).size() < 26)
     okay.push_back(point(i, j));
   }
  }
  if (okay.size() == 0) {
   for (int i = x - 2; i <= x + 2; i++) {
    for (int j = y - 2; j <= y + 2; j++) {
     if (INBOUNDS(i, j) && move_cost(i, j) > 0 && !has_flag(noitem, i, j) &&
         i_at(i, j).size() < 26)
      okay.push_back(point(i, j));
    }
   }
  }
  if (okay.size() == 0)// STILL?
   return;
  point choice = okay[rng(0, okay.size() - 1)];
  add_item(choice.x, choice.y, new_item);
  return;
 }
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 x %= SEEX;
 y %= SEEY;
 grid[nonant]->itm[x][y].push_back(new_item);
 if (new_item.active)
  grid[nonant]->active_item_count++;
}

void map::process_active_items(game *g)
{
 for (int gx = 0; gx < my_MAPSIZE; gx++) {
  for (int gy = 0; gy < my_MAPSIZE; gy++) {
   if (grid[gx + gy * my_MAPSIZE]->active_item_count > 0)
    process_active_items_in_submap(g, gx + gy * my_MAPSIZE);
  }
 }
}
     
void map::process_active_items_in_submap(game *g, int nonant)
{
 for (int i = 0; i < SEEX; i++) {
  for (int j = 0; j < SEEY; j++) {
   std::vector<item>& items = grid[nonant]->itm[i][j];
   for (int n = 0; n < items.size(); n++) {
    if (items[n].active) {
     if (!items[n].is_tool()) { // It's probably a charger gun
      items[n].active = false;
      items[n].charges = 0;
     } else { 
      const it_tool* const tmp = dynamic_cast<const it_tool*>(items[n].type);
      (*tmp->use)(g, &(g->u), &(items[n]), true);
      if (tmp->turns_per_charge > 0 && int(messages.turn) % tmp->turns_per_charge ==0)
       items[n].charges--;
      if (items[n].charges <= 0) {
       (*tmp->use)(g, &(g->u), &(items[n]), false);
       if (tmp->revert_to == itm_null || items[n].charges == -1) {
        items.erase(items.begin() + n);
        grid[nonant]->active_item_count--;
        n--;
       } else
        items[n].type = item::types[tmp->revert_to];
      }
     }
    }
   }
  }
 }
}

void map::use_amount(point origin, int range, itype_id type, int quantity, bool use_container)
{
 for (int radius = 0; radius <= range && quantity > 0; radius++) {
  for (int x = origin.x - radius; x <= origin.x + radius; x++) {
   for (int y = origin.y - radius; y <= origin.y + radius; y++) {
    if (rl_dist(origin, x, y) <= radius) {
     for (int n = 0; n < i_at(x, y).size() && quantity > 0; n++) {
      item* curit = &(i_at(x, y)[n]);
      bool used_contents = false;
      for (int m = 0; m < curit->contents.size() && quantity > 0; m++) {
       if (curit->contents[m].type->id == type) {
        quantity--;
        curit->contents.erase(curit->contents.begin() + m);
        m--;
        used_contents = true;
       }
      }
      if (use_container && used_contents) {
       i_rem(x, y, n);
       n--;
      } else if (curit->type->id == type && quantity > 0) {
       quantity--;
       i_rem(x, y, n);
       n--;
      }
     }
    }
   }
  }
 }
}

void map::use_charges(point origin, int range, itype_id type, int quantity)
{
 for (int radius = 0; radius <= range && quantity > 0; radius++) {
  for (int x = origin.x - radius; x <= origin.x + radius; x++) {
   for (int y = origin.y - radius; y <= origin.y + radius; y++) {
    if (rl_dist(origin, x, y) <= radius) {
     for (int n = 0; n < i_at(x, y).size(); n++) {
      item* curit = &(i_at(x, y)[n]);
// Check contents first
      for (int m = 0; m < curit->contents.size() && quantity > 0; m++) {
       if (curit->contents[m].type->id == type) {
        if (curit->contents[m].charges <= quantity) {
         quantity -= curit->contents[m].charges;
         if (curit->contents[m].destroyed_at_zero_charges()) {
          curit->contents.erase(curit->contents.begin() + m);
          m--;
         } else
          curit->contents[m].charges = 0;
        } else {
         curit->contents[m].charges -= quantity;
         return;
        }
       }
      }
// Now check the actual item
      if (curit->type->id == type) {
       if (curit->charges <= quantity) {
        quantity -= curit->charges;
        if (curit->destroyed_at_zero_charges()) {
         i_rem(x, y, n);
         n--;
        } else
         curit->charges = 0;
       } else {
        curit->charges -= quantity;
        return;
       }
      }
     }
    }
   }
  }
 }
}
 
trap_id& map::tr_at(int x, int y)
{
 if (!INBOUNDS(x, y)) return (discard<trap_id>::x = tr_null);	// Out-of-bounds, return our null trap
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 x %= SEEX;
 y %= SEEY;
 if (x < 0 || x >= SEEX || y < 0 || y >= SEEY) {
  debugmsg("tr_at contained bad x:y %d:%d", x, y);
  return (discard<trap_id>::x = tr_null);	// Out-of-bounds, return our null trap
 }

 if (ter_t::list[ grid[nonant]->ter[x][y] ].trap != tr_null) return (discard<trap_id>::x = ter_t::list[ grid[nonant]->ter[x][y] ].trap);
 
 return grid[nonant]->trp[x][y];
}

void map::add_trap(int x, int y, trap_id t)
{
 if (!INBOUNDS(x, y)) return;
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 x %= SEEX;
 y %= SEEY;
 grid[nonant]->trp[x][y] = t;
}

void map::disarm_trap(game *g, int x, int y)
{
 const auto tr_id = tr_at(x, y);
 if (tr_id == tr_null) {
  debugmsg("Tried to disarm a trap where there was none (%d %d)", x, y);
  return;
 }
 const trap* const tr = trap::traps[tr_id];
 int diff = tr->difficulty;
 int roll = rng(g->u.sklevel[sk_traps], 4 * g->u.sklevel[sk_traps]);
 while ((rng(5, 20) < g->u.per_cur || rng(1, 20) < g->u.dex_cur) && roll < 50)
  roll++;
 if (roll >= diff) {
  messages.add("You disarm the trap!");
  for (const auto item_id : tr->components) {
   if (item_id != itm_null) add_item(x, y, item::types[item_id], 0);
  }
  tr_at(x, y) = tr_null;
  if(diff > 1.25*g->u.sklevel[sk_traps]) // failure might have set off trap
   g->u.practice(sk_traps, 1.5*(diff - g->u.sklevel[sk_traps]));
 } else if (roll >= diff * .8) {
  messages.add("You fail to disarm the trap.");
  if(diff > 1.25*g->u.sklevel[sk_traps])
   g->u.practice(sk_traps, 1.5*(diff - g->u.sklevel[sk_traps]));
 } else {
  messages.add("You fail to disarm the trap, and you set it off!");
  (*tr->act)(g, x, y);
  if(diff - roll <= 6)
   // Give xp for failing, but not if we failed terribly (in which
   // case the trap may not be disarmable).
   g->u.practice(sk_traps, 2*diff);
 }
}
 
field& map::field_at(int x, int y)
{
 if (!INBOUNDS(x, y)) return (discard<field>::x = field());
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 x %= SEEX;
 y %= SEEY;
 return grid[nonant]->fld[x][y];
}

bool map::add_field(game *g, int x, int y, field_id t, unsigned char density)
{
 if (!INBOUNDS(x, y)) return false;
 if (field_at(x, y).type == fd_web && t == fd_fire) density++;
 else if (!field_at(x, y).is_null()) // Blood & bile are null too
  return false;
 if (density > 3) density = 3;
 if (density <= 0) return false;
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;
 x %= SEEX;
 y %= SEEY;
 if (grid[nonant]->fld[x][y].type == fd_null) grid[nonant]->field_count++;
 grid[nonant]->fld[x][y] = field(t, density);
 if (g != NULL && x == g->u.posx && y == g->u.posy && grid[nonant]->fld[x][y].is_dangerous()) {
  g->cancel_activity_query("You're in a %s!", fieldlist[t].name[density - 1].c_str());
 }
 return true;
}

void map::remove_field(int x, int y)
{
 if (!INBOUNDS(x, y)) return;
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;
 x %= SEEX;
 y %= SEEY;
 if (grid[nonant]->fld[x][y].type != fd_null) grid[nonant]->field_count--;
 grid[nonant]->fld[x][y] = field();
}

computer* map::computer_at(int x, int y)
{
 if (!INBOUNDS(x, y)) return NULL;
/*
 int nonant;
 cast_to_nonant(x, y, nonant);
*/
 int nonant = int(x / SEEX) + int(y / SEEY) * my_MAPSIZE;

 x %= SEEX;
 y %= SEEY;
 if (grid[nonant]->comp.name == "") return NULL;
 return &(grid[nonant]->comp);
}

void map::debug()
{
 mvprintw(0, 0, "MAP DEBUG");
 getch();
 for (int i = 0; i <= SEEX * 2; i++) {
  for (int j = 0; j <= SEEY * 2; j++) {
   if (i_at(i, j).size() > 0) {
    mvprintw(1, 0, "%d, %d: %d items", i, j, i_at(i, j).size());
    mvprintw(2, 0, "%c, %d", i_at(i, j)[0].symbol(), i_at(i, j)[0].color());
    getch();
   }
  }
 }
 getch();
}

void map::draw(game *g, WINDOW* w, point center)
{
 for (int i = 0; i < my_MAPSIZE * my_MAPSIZE; i++) {
  if (!grid[i])
   debugmsg("grid %d (%d, %d) is null! mapbuffer size = %d",
            i, i % my_MAPSIZE, i / my_MAPSIZE, MAPBUFFER.size());
 }
 int light = g->u.sight_range(g->light_level());
 for  (int realx = center.x - SEEX; realx <= center.x + SEEX; realx++) {
  for (int realy = center.y - SEEY; realy <= center.y + SEEY; realy++) {
   const int dist = rl_dist(g->u.posx, g->u.posy, realx, realy);
   if (dist > light) {
    mvwputch(w, realy+SEEY - center.y, realx+SEEX - center.x, (g->u.has_disease(DI_BOOMERED) ? c_magenta : c_dkgray),'#');
   } else if (dist <= g->u.clairvoyance() ||
              sees(g->u.posx, g->u.posy, realx, realy, light))
    drawsq(w, g->u, realx, realy, false, true, center.x, center.y);
   else
    mvwputch(w, realy+SEEY - center.y, realx+SEEX - center.x, c_black,'#');
  }
 }
 int atx = SEEX + g->u.posx - center.x, aty = SEEY + g->u.posy - center.y;
 if (atx >= 0 && atx < SEEX * 2 + 1 && aty >= 0 && aty < SEEY * 2 + 1)
  mvwputch(w, aty, atx, g->u.color(), '@');
}

void map::drawsq(WINDOW* w, player &u, int x, int y, bool invert,
                 bool show_items, int cx, int cy)
{
 if (!INBOUNDS(x, y)) return;	// Out of bounds
 if (cx == -1) cx = u.posx;
 if (cy == -1) cy = u.posy;
 const int k = x + SEEX - cx;
 const int j = y + SEEY - cy;
 nc_color tercol;
 const auto terrain = ter(x, y);
 long sym = ter_t::list[terrain].sym;
 bool hi = false;
 bool normal_tercol = false;
 bool drew_field = false; 
 mvwputch(w, j, k, c_black, ' ');	// actively clear
 if (u.has_disease(DI_BOOMERED))
  tercol = c_magenta;
 else if ((u.is_wearing(itm_goggles_nv) && u.has_active_item(itm_UPS_on)) ||
          u.has_active_bionic(bio_night_vision))
  tercol = c_ltgreen;
 else {
  normal_tercol = true;
  tercol = ter_t::list[terrain].color;
 }
 // background tile should show no matter what
 if (ter_t::tiles.count(terrain)) {
	 if (mvwaddbgtile(w, j, k, ter_t::tiles[terrain].c_str())) sym = 0;
 }

 if (move_cost(x, y) == 0 && has_flag(swimmable, x, y) && !u.underwater)
  show_items = false;	// Can only see underwater items if WE are underwater
// If there's a trap here, and we have sufficient perception, draw that instead
 const auto tr_id = tr_at(x, y);
 if (tr_id != tr_null){
   const trap* const tr = trap::traps[tr_id];
   if (u.per_cur - u.encumb(bp_eyes) >= tr->visibility) {
     tercol = tr->color;
     if (tr->sym == '%') {
       switch(rng(1, 5)) {
       case 1: sym = '*'; break;
       case 2: sym = '0'; break;
       case 3: sym = '8'; break;
       case 4: sym = '&'; break;
       case 5: sym = '+'; break;
       }
     } else sym = tr->sym;
   }
 }
   // If there's a field here, draw that instead (unless its symbol is %)
   if (field_at(x, y).type != fd_null && fieldlist[field_at(x, y).type].sym != '&') {
  tercol = fieldlist[field_at(x, y).type].color[field_at(x, y).density - 1];
  drew_field = true;
  if (fieldlist[field_at(x, y).type].sym == '*') {
   switch (rng(1, 5)) {
    case 1: sym = '*'; break;
    case 2: sym = '0'; break;
    case 3: sym = '8'; break;
    case 4: sym = '&'; break;
    case 5: sym = '+'; break;
   }
  } else if (fieldlist[field_at(x, y).type].sym != '%') {
   sym = fieldlist[field_at(x, y).type].sym;
   drew_field = false;
  }
 }
// If there's items here, draw those instead
 if (show_items && i_at(x, y).size() > 0 && !drew_field) {
  if ((ter_t::list[ter(x, y)].sym != '.')) hi = true;
  else {
   tercol = i_at(x, y)[i_at(x, y).size() - 1].color();
   if (i_at(x, y).size() > 1) invert = !invert;
   sym = i_at(x, y)[i_at(x, y).size() - 1].symbol();
  }
 }

 int veh_part = 0;
 const vehicle* const veh = veh_at(x, y, veh_part);
 if (veh) {
  sym = special_symbol (veh->face.dir_symbol(veh->part_sym(veh_part)));
  if (normal_tercol) tercol = veh->part_color(veh_part);
 }

 if (sym) {
	 if (invert) mvwputch_inv(w, j, k, tercol, sym);
	 else if (hi) mvwputch_hi(w, j, k, tercol, sym);
	 else mvwputch(w, j, k, tercol, sym);
 }
}

#if 0
//WIP: faster map::sees
bool map::sees(int Fx, int Fy, int Tx, int Ty, int range, int &tc)
{
 int dx = abs(Tx - Fx);
 int dy = abs(Ty - Fy);
 if (dx == 0) { // Infinite slope, special case
  tc = 0; // doesn't matter who even cares
  for (int y = Fy; y <= Ty; y++) {
   if (!trans(x, y))
    return false;
  }
  return true;
 }
 for (int x = Fx; x <= Tx; x++) {
  int Yhl = 
#endif

bool map::sees(int Fx, int Fy, int Tx, int Ty, int range)
{
  int tc = 0;
  return sees(Fx, Fy, Tx, Ty, range, tc);
}

/*
map::sees based off code by Steve Register [arns@arns.freeservers.com]
http://roguebasin.roguelikedevelopment.org/index.php?title=Simple_Line_of_Sight
*/
bool map::sees(int Fx, int Fy, int Tx, int Ty, int range, int &tc)
{
 int dx = Tx - Fx;
 int dy = Ty - Fy;
 int ax = abs(dx) << 1;
 int ay = abs(dy) << 1;
 int sx = SGN(dx);
 int sy = SGN(dy);
 int x = Fx;
 int y = Fy;
 int t = 0;
 int st;
 
 if (range >= 0 && (abs(dx) > range || abs(dy) > range))
  return false;	// Out of range!
 if (ax > ay) { // Mostly-horizontal line
  st = SGN(ay - (ax >> 1));
// Doing it "backwards" prioritizes straight lines before diagonal.
// This will help avoid creating a string of zombies behind you and will
// promote "mobbing" behavior (zombies surround you to beat on you)
  for (tc = abs(ay - (ax >> 1)) * 2 + 1; tc >= -1; tc--) {
   t = tc * st;
   x = Fx;
   y = Fy;
   do {
    if (t > 0) {
     y += sy;
     t -= ax;
    }
    x += sx;
    t += ay;
    if (x == Tx && y == Ty) {
     tc *= st;
     return true;
    }
   } while ((trans(x, y)) && (INBOUNDS(x,y)));
  }
  return false;
 } else { // Same as above, for mostly-vertical lines
  st = SGN(ax - (ay >> 1));
  for (tc = abs(ax - (ay >> 1)) * 2 + 1; tc >= -1; tc--) {
   t = tc * st;
   x = Fx;
   y = Fy;
   do {
    if (t > 0) {
     x += sx;
     t -= ay;
    }
    y += sy;
    t += ax;
    if (x == Tx && y == Ty) {
     tc *= st;
     return true;
    }
   } while ((trans(x, y)) && (INBOUNDS(x,y)));
  }
  return false;
 }
 return false; // Shouldn't ever be reached, but there it is.
}

bool map::clear_path(int Fx, int Fy, int Tx, int Ty, int range, int cost_min,
                     int cost_max, int &tc)
{
 int dx = Tx - Fx;
 int dy = Ty - Fy;
 int ax = abs(dx) << 1;
 int ay = abs(dy) << 1;
 int sx = SGN(dx);
 int sy = SGN(dy);
 int x = Fx;
 int y = Fy;
 int t = 0;
 int st;
 
 if (range >= 0 && (abs(dx) > range || abs(dy) > range))
  return false;	// Out of range!
 if (ax > ay) { // Mostly-horizontal line
  st = SGN(ay - (ax >> 1));
// Doing it "backwards" prioritizes straight lines before diagonal.
// This will help avoid creating a string of zombies behind you and will
// promote "mobbing" behavior (zombies surround you to beat on you)
  for (tc = abs(ay - (ax >> 1)) * 2 + 1; tc >= -1; tc--) {
   t = tc * st;
   x = Fx;
   y = Fy;
   do {
    if (t > 0) {
     y += sy;
     t -= ax;
    }
    x += sx;
    t += ay;
    if (x == Tx && y == Ty) {
     tc *= st;
     return true;
    }
   } while (move_cost(x, y) >= cost_min && move_cost(x, y) <= cost_max &&
            INBOUNDS(x, y));
  }
  return false;
 } else { // Same as above, for mostly-vertical lines
  st = SGN(ax - (ay >> 1));
  for (tc = abs(ax - (ay >> 1)) * 2 + 1; tc >= -1; tc--) {
  t = tc * st;
  x = Fx;
  y = Fy;
   do {
    if (t > 0) {
     x += sx;
     t -= ay;
    }
    y += sy;
    t += ax;
    if (x == Tx && y == Ty) {
     tc *= st;
     return true;
    }
   } while (move_cost(x, y) >= cost_min && move_cost(x, y) <= cost_max &&
            INBOUNDS(x,y));
  }
  return false;
 }
 return false; // Shouldn't ever be reached, but there it is.
}

// Bash defaults to true.
std::vector<point> map::route(int Fx, int Fy, int Tx, int Ty, bool bash)
{
/* TODO: If the origin or destination is out of bound, figure out the closest
 * in-bounds point and go to that, then to the real origin/destination.
 */

 if (!INBOUNDS(Fx, Fy) || !INBOUNDS(Tx, Ty)) {
  int linet;
  if (sees(Fx, Fy, Tx, Ty, -1, linet))
   return line_to(Fx, Fy, Tx, Ty, linet);
  else {
   return std::vector<point>();
  }
 }
// First, check for a simple straight line on flat ground
 int linet = 0;
 if (clear_path(Fx, Fy, Tx, Ty, -1, 2, 2, linet))
  return line_to(Fx, Fy, Tx, Ty, linet);
/*
 if (move_cost(Tx, Ty) == 0)
  debugmsg("%d:%d wanted to move to %d:%d, a %s!", Fx, Fy, Tx, Ty,
           tername(Tx, Ty).c_str());
 if (move_cost(Fx, Fy) == 0)
  debugmsg("%d:%d, a %s, wanted to move to %d:%d!", Fx, Fy,
           tername(Fx, Fy).c_str(), Tx, Ty);
*/
 std::vector<point> open;
 // Zaimoni: we're taking a memory management hit converting from C99 stack-allocated arrays to std::vector here.
 std::vector<std::vector<astar_list> > list(SEEX * my_MAPSIZE, std::vector<astar_list>(SEEY * my_MAPSIZE, ASL_NONE)); // Init to not being on any list
 std::vector<std::vector<int> > score(SEEX * my_MAPSIZE, std::vector<int>(SEEY * my_MAPSIZE, 0));	// No score!
 std::vector<std::vector<int> > gscore(SEEX * my_MAPSIZE, std::vector<int>(SEEY * my_MAPSIZE, 0));	// No score!
 std::vector<std::vector<point> > parent(SEEX * my_MAPSIZE, std::vector<point>(SEEY * my_MAPSIZE, point(-1, -1)));

 int startx = Fx - 4, endx = Tx + 4, starty = Fy - 4, endy = Ty + 4;
 if (Tx < Fx) {
  startx = Tx - 4;
  endx = Fx + 4;
 }
 if (Ty < Fy) {
  starty = Ty - 4;
  endy = Fy + 4;
 }
 if (startx < 0)
  startx = 0;
 if (starty < 0)
  starty = 0;
 if (endx > SEEX * my_MAPSIZE - 1)
  endx = SEEX * my_MAPSIZE - 1;
 if (endy > SEEY * my_MAPSIZE - 1)
  endy = SEEY * my_MAPSIZE - 1;

 list[Fx][Fy] = ASL_OPEN;
 open.push_back(point(Fx, Fy));

 bool done = false;

 do {
  //debugmsg("Open.size() = %d", open.size());
  int best = 9999;
  int index = -1;
  for (int i = 0; i < open.size(); i++) {
   if (i == 0 || score[open[i].x][open[i].y] < best) {
    best = score[open[i].x][open[i].y];
    index = i;
   }
  }
  for (int x = open[index].x - 1; x <= open[index].x + 1; x++) {
   for (int y = open[index].y - 1; y <= open[index].y + 1; y++) {
    if (x == open[index].x && y == open[index].y)
     y++;	// Skip the current square
    if (x == Tx && y == Ty) {
     done = true;
     parent[x][y] = open[index];
    } else if (x >= startx && x <= endx && y >= starty && y <= endy &&
               (move_cost(x, y) > 0 || (bash && has_flag(bashable, x, y)))) {
     if (list[x][y] == ASL_NONE) {	// Not listed, so make it open
      list[x][y] = ASL_OPEN;
      open.push_back(point(x, y));
      parent[x][y] = open[index];
      gscore[x][y] = gscore[open[index].x][open[index].y] + move_cost(x, y);
      if (ter(x, y) == t_door_c)
       gscore[x][y] += 4;	// A turn to open it and a turn to move there
      else if (move_cost(x, y) == 0 && (bash && has_flag(bashable, x, y)))
       gscore[x][y] += 18;	// Worst case scenario with damage penalty
      score[x][y] = gscore[x][y] + 2 * rl_dist(x, y, Tx, Ty);
     } else if (list[x][y] == ASL_OPEN) { // It's open, but make it our child
      int newg = gscore[open[index].x][open[index].y] + move_cost(x, y);
      if (ter(x, y) == t_door_c)
       newg += 4;	// A turn to open it and a turn to move there
      else if (move_cost(x, y) == 0 && (bash && has_flag(bashable, x, y)))
       newg += 18;	// Worst case scenario with damage penalty
      if (newg < gscore[x][y]) {
       gscore[x][y] = newg;
       parent[x][y] = open[index];
       score [x][y] = gscore[x][y] + 2 * rl_dist(x, y, Tx, Ty);
      }
     }
    }
   }
  }
  list[open[index].x][open[index].y] = ASL_CLOSED;
  open.erase(open.begin() + index);
 } while (!done && open.size() > 0);

 std::vector<point> tmp;
 std::vector<point> ret;
 if (done) {
  point cur(Tx, Ty);
  while (cur.x != Fx || cur.y != Fy) {
   //debugmsg("Retracing... (%d:%d) => [%d:%d] => (%d:%d)", Tx, Ty, cur.x, cur.y, Fx, Fy);
   tmp.push_back(cur);
   if (rl_dist(cur, parent[cur.x][cur.y])>1){
    debugmsg("Jump in our route! %d:%d->%d:%d", cur.x, cur.y,
             parent[cur.x][cur.y].x, parent[cur.x][cur.y].y);
    return ret;
   }
   cur = parent[cur.x][cur.y];
  }
  for (int i = tmp.size() - 1; i >= 0; i--)
   ret.push_back(tmp[i]);
 }
 return ret;
}

void map::save(overmap *om, unsigned int turn, int x, int y)
{
 for (int gridx = 0; gridx < my_MAPSIZE; gridx++) {
  for (int gridy = 0; gridy < my_MAPSIZE; gridy++)
   saven(om, turn, x, y, gridx, gridy);
 }
}

void map::load(game *g, int wx, int wy)
{
 for (int gridx = 0; gridx < my_MAPSIZE; gridx++) {
  for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
   if (!loadn(g, wx, wy, gridx, gridy))
    loadn(g, wx, wy, gridx, gridy);
  }
 }
}

void map::shift(game *g, int wx, int wy, const point delta)
{
// Special case of 0-shift; refresh the map
 if (delta.x == 0 && delta.y == 0) {
  return; // Skip this?
  for (int gridx = 0; gridx < my_MAPSIZE; gridx++) {
   for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
    if (!loadn(g, wx+ delta.x, wy+ delta.y, gridx, gridy))
     loadn(g, wx+ delta.x, wy+ delta.y, gridx, gridy);
   }
  }
  return;
 }

// if player is driving vehicle, (s)he must be shifted with vehicle too
 if (g->u.in_vehicle) {
  g->u.posx -= delta.x * SEEX;
  g->u.posy -= delta.y * SEEY;
 }

// Shift the map sx submaps to the right and sy submaps down.
// sx and sy should never be bigger than +/-1.
// wx and wy are our position in the world, for saving/loading purposes.
 if (delta.x >= 0) {
  for (int gridx = 0; gridx < my_MAPSIZE; gridx++) {
   if (delta.y >= 0) {
    for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
     if (gridx + delta.x < my_MAPSIZE && gridy + delta.y < my_MAPSIZE)
      copy_grid(gridx + gridy * my_MAPSIZE,
                gridx + delta.x + (gridy + delta.y) * my_MAPSIZE);
     else if (!loadn(g, wx + delta.x, wy + delta.y, gridx, gridy))
      loadn(g, wx + delta.x, wy + delta.y, gridx, gridy);
    }
   } else { // sy < 0; work through it backwards
    for (int gridy = my_MAPSIZE - 1; gridy >= 0; gridy--) {
     if (gridx + delta.x < my_MAPSIZE && gridy + delta.y >= 0)
      copy_grid(gridx + gridy * my_MAPSIZE,
                gridx + delta.x + (gridy + delta.y) * my_MAPSIZE);
     else if (!loadn(g, wx + delta.x, wy + delta.y, gridx, gridy))
      loadn(g, wx + delta.x, wy + delta.y, gridx, gridy);
    }
   }
  }
 } else { // sx < 0; work through it backwards
  for (int gridx = my_MAPSIZE - 1; gridx >= 0; gridx--) {
   if (delta.y >= 0) {
    for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
     if (gridx + delta.x >= 0 && gridy + delta.y < my_MAPSIZE)
      copy_grid(gridx + gridy * my_MAPSIZE,
                gridx + delta.x + (gridy + delta.y) * my_MAPSIZE);
     else if (!loadn(g, wx + delta.x, wy + delta.y, gridx, gridy))
      loadn(g, wx + delta.x, wy + delta.y, gridx, gridy);
    }
   } else { // sy < 0; work through it backwards
    for (int gridy = my_MAPSIZE - 1; gridy >= 0; gridy--) {
     if (gridx + delta.x >= 0 && gridy + delta.y >= 0)
      copy_grid(gridx + gridy * my_MAPSIZE,
                gridx + delta.x + (gridy + delta.y) * my_MAPSIZE);
     else if (!loadn(g, wx + delta.x, wy + delta.y, gridx, gridy))
      loadn(g, wx + delta.x, wy + delta.y, gridx, gridy);
    }
   }
  }
 }
}

// saven saves a single nonant.  worldx and worldy are used for the file
// name and specifies where in the world this nonant is.  gridx and gridy are
// the offset from the top left nonant:
// 0,0 1,0 2,0
// 0,1 1,1 2,1
// 0,2 1,2 2,2
void map::saven(overmap *om, unsigned int turn, int worldx, int worldy,
                int gridx, int gridy)
{
 const int n = gridx + gridy * my_MAPSIZE;

 if (grid[n]->ter[0][0] == t_null) return;
 int abs_x = om->pos.x * OMAPX * 2 + worldx + gridx,
     abs_y = om->pos.y * OMAPY * 2 + worldy + gridy;

 MAPBUFFER.add_submap(abs_x, abs_y, om->pos.z, grid[n]);
}

// worldx & worldy specify where in the world this is;
// gridx & gridy specify which nonant:
// 0,0  1,0  2,0
// 0,1  1,1  2,1
// 0,2  1,2  2,2 etc
bool map::loadn(game *g, int worldx, int worldy, int gridx, int gridy)
{
 int absx = g->cur_om.pos.x * OMAPX * 2 + worldx + gridx,
     absy = g->cur_om.pos.y * OMAPY * 2 + worldy + gridy,
     gridn = gridx + gridy * my_MAPSIZE;
 submap * const tmpsub = MAPBUFFER.lookup_submap(absx, absy, g->cur_om.pos.z);
 if (tmpsub) {
  grid[gridn] = tmpsub;
  for (int i = 0; i < grid[gridn]->vehicles.size(); i++) {
   grid[gridn]->vehicles[i].sm = point(gridx,gridy);
  }
 } else { // It doesn't exist; we must generate it!
  map tmp_map;
// overx, overy is where in the overmap we need to pull data from
// Each overmap square is two nonants; to prevent overlap, generate only at
//  squares divisible by 2.
  int newmapx = worldx + gridx - ((worldx + gridx) % 2);
  int newmapy = worldy + gridy - ((worldy + gridy) % 2);
  if (worldx + gridx < 0) newmapx = worldx + gridx;
  if (worldy + gridy < 0) newmapy = worldy + gridy;
  if (worldx + gridx < 0) newmapx = worldx + gridx;
  tmp_map.generate(g, &(g->cur_om), newmapx, newmapy, int(messages.turn));
  return false;
 }
 return true;
}

void map::copy_grid(int to, int from)
{
 grid[to] = grid[from];
 for (int i = 0; i < grid[to]->vehicles.size(); i++) {
  int ind = grid[to]->vehicles.size() - 1;
  grid[to]->vehicles[ind].sm.x = to % my_MAPSIZE;
  grid[to]->vehicles[ind].sm.y = to / my_MAPSIZE;
 }
}

void map::spawn_monsters(game *g)
{
 for (int gx = 0; gx < my_MAPSIZE; gx++) {
  for (int gy = 0; gy < my_MAPSIZE; gy++) {
   int n = gx + gy * my_MAPSIZE;
   for (int i = 0; i < grid[n]->spawns.size(); i++) {
    for (int j = 0; j < grid[n]->spawns[i].count; j++) {
     int tries = 0;
	 point m(grid[n]->spawns[i].pos);
     monster tmp(mtype::types[grid[n]->spawns[i].type]);
     tmp.spawnmap.x = g->lev.x + gx;
     tmp.spawnmap.y = g->lev.y + gy;
     tmp.faction_id = grid[n]->spawns[i].faction_id;
     tmp.mission_id = grid[n]->spawns[i].mission_id;
     if (grid[n]->spawns[i].name != "NONE") tmp.unique_name = grid[n]->spawns[i].name;
     if (grid[n]->spawns[i].friendly) tmp.friendly = -1;
     int fx = m.x + gx * SEEX, fy = m.y + gy * SEEY;

     while ((!g->is_empty(fx, fy) || !tmp.can_move_to(g->m, fx, fy)) &&  tries < 10) {
      m.x = (grid[n]->spawns[i].pos.x + rng(-3, 3)) % SEEX;
      m.y = (grid[n]->spawns[i].pos.y + rng(-3, 3)) % SEEY;
      if (m.x < 0) m.x += SEEX;
      if (m.y < 0) m.y += SEEY;
      fx = m.x + gx * SEEX;
      fy = m.y + gy * SEEY;
      tries++;
     }
     if (tries != 10) {
      tmp.spawnpos = point(fx,fy);
      tmp.spawn(fx, fy);
      g->z.push_back(tmp);
     }
    }
   }
   grid[n]->spawns.clear();
  }
 }
}

void map::clear_spawns()
{
 for (int i = 0; i < my_MAPSIZE * my_MAPSIZE; i++)
  grid[i]->spawns.clear();
}

void map::clear_traps()
{
 for (int i = 0; i < my_MAPSIZE * my_MAPSIZE; i++) {
  for (int x = 0; x < SEEX; x++) {
   for (int y = 0; y < SEEY; y++)
    grid[i]->trp[x][y] = tr_null;
  }
 }
}


bool map::inbounds(int x, int y)
{
 return (x >= 0 && x < SEEX * my_MAPSIZE && y >= 0 && y < SEEY * my_MAPSIZE);
}

