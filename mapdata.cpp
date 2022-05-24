#include "mapdata.h"
#include "json.h"

std::map<ter_id, std::string> ter_t::tiles;

const field_t field::list[] = {
{{"",	"",	""},					'%',
 {c_white, c_white, c_white},	{true, true, true}, {false, false, false},   0},

{{"blood splatter", "blood stain", "puddle of blood"},	'%',
 {c_red, c_red, c_red},		{true, true, true}, {false, false, false},2500},

{{"bile splatter", "bile stain", "puddle of bile"},	'%',
 {c_pink, c_pink, c_pink},	{true, true, true}, {false, false, false},2500},

{{"cobwebs","webs", "thick webs"},			'}',
 {c_white, c_white, c_white},	{true, true, false},{false, false, false},   0},

{{"slime trail", "slime stain", "puddle of slime"},	'%',
 {c_ltgreen, c_ltgreen, c_green},{true, true, true},{false, false, false},2500},

{{"acid splatter", "acid streak", "pool of acid"},	'5',
 {c_ltgreen, c_green, c_green},	{true, true, true}, {true, true, true},	    10},

{{"sap splatter", "glob of sap", "pool of sap"},	'5',
 {c_yellow, c_brown, c_brown},	{true, true, true}, {true, true, true},     20},

{{"small fire",	"fire",	"raging fire"},			'4',
 {c_yellow, c_ltred, c_red},	{true, true, true}, {true, true, true},	   800},

{{"thin smoke",	"smoke", "thick smoke"},		'8',
 {c_white, c_ltgray, c_dkgray},	{true, false, false},{false, true, true},  300},

{{"hazy cloud","toxic gas","thick toxic gas"},		'8',
 {c_white, c_ltgreen, c_green}, {true, false, false},{false, true, true},  900},

{{"hazy cloud","tear gas","thick tear gas"},		'8',
 {c_white, c_yellow, c_brown},	{true, false, false},{true, true, true},   600},

{{"hazy cloud","radioactive gas", "thick radioactive gas"}, '8',
 {c_white, c_ltgreen, c_green},	{true, true, false}, {true, true, true},  1000},

{{"gas vent", "gas vent", "gas vent"}, '%',
 {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0},

{{"", "", ""}, '&', // fire vents
 {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0},

{{"fire", "fire", "fire"}, '5',
 {c_red, c_red, c_red}, {true, true, true}, {true, true, true}, 0},

{{"sparks", "electric crackle", "electric cloud"},	'9',
 {c_white, c_cyan, c_blue},	{true, true, true}, {true, true, true},	     2},

{{"odd ripple", "swirling air", "tear in reality"},	'*',
 {c_ltgray, c_dkgray, c_magenta},{true, true, false},{false, false, false},  0},

{{"", "", ""}, '&', // push items
 {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0},

{{"", "", ""}, '&', // shock vents
 {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0},

{{"", "", ""}, '&', // acid vents
 {c_white, c_white, c_white}, {true, true, true}, {false, false, false}, 0}
};


// \todo V.0.2.1 new terrain air
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

DEFINE_JSON_ENUM_SUPPORT_TYPICAL(ter_id, JSON_transcode)

bool close_door(ter_id& t)
{
	switch (t) {
	case t_door_o:
		t = t_door_c;
		return true;
	case t_door_metal_o:
		t = t_door_metal_c;
		return true;
	case t_door_glass_o:
		t = t_door_glass_c;
		return true;
	default: return false;
	}
}

bool open_door(ter_id& t, bool inside)
{
	switch (t) {
	case t_door_c:
		t = t_door_o;
		return true;
	case t_door_metal_c:
		t = t_door_metal_o;
		return true;
	case t_door_glass_c:
		t = t_door_glass_o;
		return true;
	case t_door_locked:
	case t_door_locked_alarm:
		if (inside) {
			t = t_door_o;
			return true;
		}
		[[fallthrough]];
	default: return false;
	}
}

using namespace cataclysm;

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