#include "vehicle.h"
#include "game.h"

// following symbols will be translated: 
// y, u, n, b to NW, NE, SE, SW lines correspondingly
// h, j, c to horizontal, vertical, cross correspondingly
const vpart_info vpart_info::list[num_vparts] =
{   // name        sym   color    sym_b   color_b  dmg  dur  par1 par2  item
	{ "null part",  '?', c_red,     '?', c_red,     100, 100, 0, 0, itm_null, 0,
	0 },
{ "seat",       '#', c_red,     '*', c_red,     60,  300, 0, 0, itm_seat, 1,
mfb(vpf_over) | mfb(vpf_seat) | mfb(vpf_no_reinforce) },
{ "frame",      'h', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) },
{ "frame",      'j', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) },
{ "frame",      'c', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) },
{ "frame",      'y', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) },
{ "frame",      'u', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) },
{ "frame",      'n', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) },
{ "frame",      'b', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) },
{ "frame",      '=', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) },
{ "frame",      'H', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) },
{ "frame",      '^', c_ltgray,  '#', c_ltgray,  100, 400, 0, 0, itm_frame, 1,
mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) },
{ "handle",     '^', c_ltcyan,  '#', c_ltcyan,  100, 300, 0, 0, itm_frame, 1,
mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) },
{ "board",      'h', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, itm_steel_plate, 1,
mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
{ "board",      'j', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, itm_steel_plate, 1,
mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
{ "board",      'y', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, itm_steel_plate, 1,
mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
{ "board",      'u', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, itm_steel_plate, 1,
mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
{ "board",      'n', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, itm_steel_plate, 1,
mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
{ "board",      'b', c_ltgray,  '#', c_ltgray,  100, 1000, 0, 0, itm_steel_plate, 1,
mfb(vpf_external) | mfb(vpf_mount_point) | mfb(vpf_mount_inner) | mfb(vpf_opaque) | mfb(vpf_obstacle) },
{ "roof",       '#', c_ltgray,  '#', c_dkgray,  100, 1000, 0, 0, itm_steel_plate, 1,
mfb(vpf_internal) | mfb(vpf_roof) },
{ "door",       '+', c_cyan,    '&', c_cyan,    80,  200, 0, 0, itm_frame, 2,
mfb(vpf_external) | mfb(vpf_obstacle) | mfb(vpf_openable) },
{ "windshield", '"', c_ltcyan,  '0', c_ltgray,  70,  50, 0, 0, itm_glass_sheet, 1,
mfb(vpf_over) | mfb(vpf_obstacle) | mfb(vpf_no_reinforce) },
{ "blade",      '-', c_white,   'x', c_white,   250, 100, 0, 0, itm_machete, 2,
mfb(vpf_external) | mfb(vpf_unmount_on_damage) | mfb(vpf_sharp) | mfb(vpf_no_reinforce) },
{ "blade",      '|', c_white,   'x', c_white,   350, 100, 0, 0, itm_machete, 2,
mfb(vpf_external) | mfb(vpf_unmount_on_damage) | mfb(vpf_sharp) | mfb(vpf_no_reinforce) },
{ "spike",      '.', c_white,   'x', c_white,   300, 100, 0, 0, itm_spear_knife, 2,
mfb(vpf_external) | mfb(vpf_unmount_on_damage) | mfb(vpf_sharp) | mfb(vpf_no_reinforce) },

//                                                           size
{ "large wheel",'0', c_dkgray,  'x', c_ltgray,  50,  300, 30, 0, itm_big_wheel, 3,
mfb(vpf_external) | mfb(vpf_mount_over) | mfb(vpf_wheel) | mfb(vpf_mount_point) },
{ "wheel",      'o', c_dkgray,  'x', c_ltgray,  50,  200, 10, 0, itm_wheel, 2,
mfb(vpf_external) | mfb(vpf_mount_over) | mfb(vpf_wheel) | mfb(vpf_mount_point) },
//                                                                         power type
{ "1L combustion engine",       '*', c_ltred,  '#', c_red,     80, 200, 120, AT_GAS, itm_combustion_small, 2,
mfb(vpf_internal) | mfb(vpf_engine) },
{ "2.5L combustion engine",     '*', c_ltred,  '#', c_red,     80, 300, 300, AT_GAS, itm_combustion, 3,
mfb(vpf_internal) | mfb(vpf_engine) },
{ "6L combustion engine",       '*', c_ltred,  '#', c_red,     80, 400, 800, AT_GAS, itm_combustion_large, 4,
mfb(vpf_internal) | mfb(vpf_engine) },
{ "electric motor",             '*', c_yellow,  '#', c_red,    80, 200, 70, AT_BATT, itm_motor, 3,
mfb(vpf_internal) | mfb(vpf_engine) },
{ "large electric motor",       '*', c_yellow,  '#', c_red,    80, 400, 350, AT_BATT, itm_motor_large, 4,
mfb(vpf_internal) | mfb(vpf_engine) },
{ "plasma engine",              '*', c_ltblue,  '#', c_red,    80, 250, 400, AT_PLASMA, itm_plasma_engine, 6,
mfb(vpf_internal) | mfb(vpf_engine) },
//                                                                         capacity type
{ "gasoline tank",              'O', c_ltred,  '#', c_red,     80, 150, 3000, AT_GAS, itm_metal_tank, 1,
mfb(vpf_internal) | mfb(vpf_fuel_tank) },
{ "storage battery",            'O', c_yellow,  '#', c_red,    80, 300, 1000, AT_BATT, itm_storage_battery, 2,
mfb(vpf_internal) | mfb(vpf_fuel_tank) },
{ "minireactor",                'O', c_ltgreen,  '#', c_red,    80, 700, 10000, AT_PLUT, itm_minireactor, 7,
mfb(vpf_internal) | mfb(vpf_fuel_tank) },
{ "hydrogene tank",             'O', c_ltblue,  '#', c_red,     80, 150, 3000, AT_PLASMA, itm_metal_tank, 1,
mfb(vpf_internal) | mfb(vpf_fuel_tank) },
{ "trunk",                      'H', c_brown,  '#', c_brown,    80, 300, 400, 0, itm_frame, 1,
mfb(vpf_over) | mfb(vpf_cargo) },
{ "box",                        'o', c_brown,  '#', c_brown,    60, 100, 400, 0, itm_frame, 1,
mfb(vpf_over) | mfb(vpf_cargo) },

{ "controls",   '$', c_ltgray,  '$', c_red,     10, 250, 0, 0, itm_vehicle_controls, 3,
mfb(vpf_internal) | mfb(vpf_controls) },
//                                                          bonus
{ "muffler",    '/', c_ltgray,  '/', c_ltgray,  10, 150, 40, 0, itm_muffler, 2,
mfb(vpf_internal) | mfb(vpf_muffler) },
{ "seatbelt",   ',', c_ltgray,  ',', c_red,     10, 200, 25, 0, itm_rope_6, 1,
mfb(vpf_internal) | mfb(vpf_seatbelt) },
{ "solar panel", '#', c_yellow,  'x', c_yellow, 10, 20, 30, 0, itm_solar_panel, 6,
mfb(vpf_over) | mfb(vpf_solar_panel) },

{ "mounted M249",         't', c_cyan,    '#', c_cyan,    80, 400, 0, AT_223, itm_m249, 6,
mfb(vpf_over) | mfb(vpf_turret) | mfb(vpf_cargo) },
{ "mounted flamethrower", 't', c_dkgray,  '#', c_dkgray,  80, 400, 0, AT_GAS, itm_flamethrower, 7,
mfb(vpf_over) | mfb(vpf_turret) },
{ "mounted plasma gun", 't', c_ltblue,    '#', c_ltblue,    80, 400, 0, AT_PLASMA, itm_plasma_rifle, 7,
mfb(vpf_over) | mfb(vpf_turret) },

{ "steel plating",     ')', c_ltcyan, ')', c_ltcyan, 100, 1000, 0, 0, itm_steel_plate, 3,
mfb(vpf_internal) | mfb(vpf_armor) },
{ "superalloy plating",')', c_dkgray, ')', c_dkgray, 100, 900, 0, 0, itm_alloy_plate, 4,
mfb(vpf_internal) | mfb(vpf_armor) },
{ "spiked plating",    ')', c_red,    ')', c_red,    150, 900, 0, 0, itm_spiked_plate, 3,
mfb(vpf_internal) | mfb(vpf_armor) | mfb(vpf_sharp) },
{ "hard plating",      ')', c_cyan,   ')', c_cyan,   100, 2300, 0, 0, itm_hard_plate, 4,
mfb(vpf_internal) | mfb(vpf_armor) }
};

// GENERAL GUIDELINES
// When adding a new vehicle, you MUST REMEMBER to insert it in the vhtype_id enum
//  at the bottom of veh_type.h!
// also, before using PART, you MUST call VEHICLE
//
// To determine mount position for parts (dx, dy), check this scheme:
//         orthogonal dir left: (Y-)
//                ^
//  back: X-   -------> forward dir: X+
//                v
//         orthogonal dir right (Y+)
//
// i.e, if you want to add a part to the back from the center of vehicle,
// use dx = -1, dy = 0;
// for the part 1 tile forward and two tiles left from the center of vehicle,
// use dx = 1, dy = -2.
//
// Internal parts should be added after external on the same mount point, i.e:
//  PART (0, 1, vp_seat);       // put a seat (it's external)
//  PART (0, 1, vp_controls);   // put controls for driver here
//  PART (0, 1, vp_seatbelt);   // also, put a seatbelt here
// To determine, what parts can be external, and what can not, check
//  vpart_id enum in veh_type.h file
// If you use wrong config, installation of part will fail

void game::init_vehicles()
{
    vehicle *veh;
    int index = 0;
    int pi;
	bool ok = true;
	vehicle::vtypes.push_back(new vehicle(this, (vhtype_id)index++)); // veh_null
	vehicle::vtypes.push_back(new vehicle(this, (vhtype_id)index++)); // veh_custom

#define VEHICLE(nm) { veh = new vehicle(this, (vhtype_id)index++); veh->name = nm; vehicle::vtypes.push_back(veh); }
#define PART(mdx, mdy, id) { pi = veh->install_part(mdx, mdy, id); \
    if (pi < 0) { debugmsg("init_vehicles: '%s' part '%s'(%d) can't be installed to %d,%d", veh->name.c_str(), vpart_info::list[id].name, veh->parts.size(), mdx, mdy); ok = false; } }

    //        name
    VEHICLE ("motorcycle");
    //    o
    //    ^
    //    #
    //    o

    //   dx, dy,    part_id
    PART (0, 0,     vp_frame_v2);
    PART (0, 0,     vp_seat);
    PART (0, 0,     vp_controls);
    PART (0, 0,     vp_engine_gas_small);
    PART (1, 0,     vp_frame_handle);
    PART (1, 0,     vp_fuel_tank_gas);
    PART (2, 0,     vp_wheel);
    PART (-1, 0,    vp_wheel);
    PART (-1, 0,    vp_cargo_box);

    //        name
    VEHICLE ("quad bike");
    //   0^0
    //    #
    //   0H0

    //   dx, dy,    part_id
    PART (0, 0,     vp_frame_v2);
    PART (0, 0,     vp_seat);
    PART (0, 0,     vp_controls);
    PART (0, 0,     vp_seatbelt);
    PART (1, 0,     vp_frame_cover);
    PART (1, 0,     vp_engine_gas_med);
    PART (1, 0,     vp_fuel_tank_gas);
    PART (1, 0,     vp_steel_plate);
    PART (-1,0,     vp_frame_h);
//    PART (-1,0,     vp_engine_motor);
//    PART (-1,0,     vp_fuel_tank_plut);
    PART (-1,0,     vp_cargo_trunk);
    PART (-1,0,     vp_steel_plate);
    PART (1, -1,    vp_wheel_large);
    PART (1,  1,    vp_wheel_large);
    PART (-1,-1,    vp_wheel_large);
    PART (-1, 1,    vp_wheel_large);
//     PART (1, -2,    vp_blade_h);
//     PART (1, 2,     vp_blade_h);

    //        name
    VEHICLE ("car");
    //   o--o
    //   |""|
    //   +##+
    //   +##+
    //   |HH|
    //   o++o

    //   dx, dy,    part_id
    PART (0, 0,     vp_frame_v2);
    PART (0, 0,     vp_seat);
    PART (0, 0,     vp_seatbelt);
    PART (0, 0,     vp_controls);
    PART (0, 0,     vp_roof);
    PART (0, 1,     vp_frame_v2);
    PART (0, 1,     vp_seat);
    PART (0, 1,     vp_seatbelt);
    PART (0, 1,     vp_roof);
    PART (0, -1,    vp_door);
    PART (0, 2,     vp_door);
    PART (-1, 0,     vp_frame_v2);
    PART (-1, 0,     vp_seat);
    PART (-1, 0,     vp_seatbelt);
    PART (-1, 0,     vp_roof);
    PART (-1, 1,     vp_frame_v2);
    PART (-1, 1,     vp_seat);
    PART (-1, 1,     vp_seatbelt);
    PART (-1, 1,     vp_roof);
    PART (-1, -1,    vp_door);
    PART (-1, 2,     vp_door);
    PART (1, 0,     vp_frame_h);
    PART (1, 0,     vp_window);
    PART (1, 1,     vp_frame_h);
    PART (1, 1,     vp_window);
    PART (1, -1,    vp_frame_v);
    PART (1, 2,     vp_frame_v);
    PART (2, 0,     vp_frame_h);
    PART (2, 0,     vp_engine_gas_med);
    PART (2, 1,     vp_frame_h);
    PART (2, -1,    vp_wheel);
    PART (2, 2,     vp_wheel);
    PART (-2, 0,     vp_frame_v);
    PART (-2, 0,     vp_cargo_trunk);
    PART (-2, 0,     vp_muffler);
    PART (-2, 0,     vp_roof);
    PART (-2, 1,     vp_frame_v);
    PART (-2, 1,     vp_cargo_trunk);
    PART (-2, 1,     vp_roof);
    PART (-2, -1,    vp_board_v);
    PART (-2, -1,    vp_fuel_tank_gas);
    PART (-2, 2,     vp_board_v);
    PART (-3, -1,    vp_wheel);
    PART (-3, 0,     vp_door);
    PART (-3, 1,     vp_door);
    PART (-3, 2,     vp_wheel);

    
    //        name
    VEHICLE ("truck");
    // 0-^-0
    // |"""|
    // +###+
    // |---|
    // |HHH|
    // 0HHH0

    PART (0, 0,     vp_frame_v);
    PART (0, 0,     vp_cargo_box);
    PART (0, 0,     vp_roof);
//    PART (0, 0,     vp_seatbelt);
    PART (0, -1,    vp_frame_v2);
    PART (0, -1,    vp_seat);
    PART (0, -1,    vp_seatbelt);
    PART (0, -1,     vp_roof);
    PART (0, 1,     vp_frame_v2);
    PART (0, 1,     vp_seat);
    PART (0, 1,     vp_seatbelt);
    PART (0, 1,     vp_roof);
    PART (0, -2,    vp_door);
    PART (0, 2,     vp_door);
    PART (0, -1,     vp_controls);

    PART (1, 0,     vp_frame_h);
    PART (1, 0,     vp_window);
    PART (1, -1,    vp_frame_h);
    PART (1, -1,    vp_window);
    PART (1, 1,     vp_frame_h);
    PART (1, 1,     vp_window);
    PART (1, -2,    vp_frame_v);
    PART (1, 2,     vp_frame_v);

    PART (2, -1,    vp_frame_h);
    PART (2, 0,     vp_frame_cover);
    PART (2, 0,     vp_engine_gas_med);
    PART (2, 1,     vp_frame_h);
    PART (2, -2,    vp_wheel_large);
    PART (2,  2,    vp_wheel_large);

    PART (-1, -1,   vp_board_h);
    PART (-1, 0,    vp_board_h);
    PART (-1, 1,    vp_board_h);
    PART (-1, -2,   vp_board_b);
    PART (-1, -2,   vp_fuel_tank_gas);
    PART (-1, 2,    vp_board_n);
    PART (-1, 2,    vp_fuel_tank_gas);

    PART (-2, -1,   vp_frame_v);
    PART (-2, -1,   vp_cargo_trunk);
    PART (-2, 0,    vp_frame_v);
    PART (-2, 0,    vp_cargo_trunk);
    PART (-2, 1,    vp_frame_v);
    PART (-2, 1,    vp_cargo_trunk);
    PART (-2, -2,   vp_board_v);
    PART (-2, 2,    vp_board_v);

    PART (-3, -1,   vp_frame_h);
    PART (-3, -1,   vp_cargo_trunk);
    PART (-3, 0,    vp_frame_h);
    PART (-3, 0,    vp_cargo_trunk);
    PART (-3, 1,    vp_frame_h);
    PART (-3, 1,    vp_cargo_trunk);
    PART (-3, -2,   vp_wheel_large);
    PART (-3, 2,    vp_wheel_large);

	if (vehicle::vtypes.size() != num_vehicles) {
		debugmsg("%d vehicles, %d types: fatal", vehicle::vtypes.size(), num_vehicles);
		exit(EXIT_FAILURE);
	}
	if (!ok) {
		debugmsg("errors in stock vehicle configuration: fatal", vehicle::vtypes.size(), num_vehicles);
		exit(EXIT_FAILURE);
	}
}

