#ifndef _VEH_TYPE_H_
#define _VEH_TYPE_H_

#include "color.h"
#include "itype.h"

enum vpart_id
{
    vp_null = 0,

// external parts
    vp_seat,
    vp_frame_h,
    vp_frame_v,
    vp_frame_c,
    vp_frame_y,
    vp_frame_u,
    vp_frame_n,
    vp_frame_b,
    vp_frame_h2,
    vp_frame_v2,
    vp_frame_cover,
    vp_frame_handle,
    vp_board_h,
    vp_board_v,
    vp_board_y,
    vp_board_u,
    vp_board_n,
    vp_board_b,
    vp_roof,
    vp_door,
    vp_window,
    vp_blade_h,
    vp_blade_v,
    vp_spike_h,
    vp_spike_v = vp_spike_h,
    vp_wheel_large,
    vp_wheel,

    vp_engine_gas_small,
    vp_engine_gas_med,
    vp_engine_gas_large,
    vp_engine_motor,
    vp_engine_motor_large,
    vp_engine_plasma,
    vp_fuel_tank_gas,
    vp_fuel_tank_batt,
    vp_fuel_tank_plut,
    vp_fuel_tank_hydrogen,
    vp_cargo_trunk, // over
    vp_cargo_box,   // over

// pure internal parts
    vp_controls,
    vp_muffler,
    vp_seatbelt,
    vp_solar_panel,
    vp_m249,
    vp_flamethrower,
    vp_plasmagun,

// plating -- special case. mounted as internal, work as first line
// of defence and gives color to external part
    vp_steel_plate,
    vp_superalloy_plate,
    vp_spiked_plate,
    vp_hard_plate,

    num_vparts
};

DECLARE_JSON_ENUM_SUPPORT(vpart_id)

enum vpart_flags
{
    vpf_external,           // can be mounted as external part
    vpf_internal,           // can be mounted inside other part
    vpf_mount_point,        // allows mounting other parts to it
    vpf_mount_inner,        // allows mounting internal parts inside it (or over it)
    vpf_mount_over,         // allows mounting parts like cargo trunk over it
    vpf_opaque,             // can't see through it
    vpf_obstacle,           // can't pass through it
    vpf_openable,           // can open/close it
    vpf_no_reinforce,       // can't reinforce this part with armor plates
    vpf_sharp,              // cutting damage instead of bashing
    vpf_unmount_on_damage,  // when damaged, part is unmounted, rather than broken

// functional flags (only one of each can be mounted per tile)
    vpf_over,               // can be mounted over other part
    vpf_roof,               // is a roof (cover)
    vpf_wheel,              // this part touches ground (trigger traps)
    vpf_seat,               // is seat
    vpf_engine,             // is engine
    vpf_fuel_tank,          // is fuel tank
    vpf_cargo,              // is cargo
    vpf_controls,           // is controls
    vpf_muffler,            // is muffler
    vpf_seatbelt,           // is seatbelt
    vpf_solar_panel,        // is solar panel
    vpf_turret,             // is turret
    vpf_armor,              // is armor plating
    vpf_func_begin  = vpf_over,
    vpf_func_end    = vpf_armor,

    num_vpflags
};

struct vpart_info
{
	static const vpart_info list[num_vparts];

    const char *name;       // part name
    char sym;               // symbol of part as if it's looking north
    nc_color color;         // color
    char sym_broken;        // symbol of broken part as if it's looking north
    nc_color color_broken;  // color of broken part
    int dmg_mod;            // damage modifier, percent
    int durability;         // durability
    union
    {
        int par1;
        int power;      // engine (top spd), solar panel (% of 1 fuel per turn, can be > 100)
        int size;       // wheel, fuel tank, trunk
        int bonus;      // seatbelt (str), muffler (%)
    };
    union
    {
        int par2;
        int fuel_type;  // engine, fuel tank
    };
    itype_id item;      // corresponding item
    int difficulty;     // installation difficulty (mechanics requirement)
    unsigned long flags;    // flags
};


enum vhtype_id
{
    veh_null = 0,
    veh_custom,

// in-built vehicles
    veh_motorcycle,
    veh_sandbike,
    veh_car,
    veh_truck,

    num_vehicles
};

#endif
