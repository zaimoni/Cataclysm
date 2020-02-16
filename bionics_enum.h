#ifndef _BIONICS_ENUM_H_
#define _BIONICS_ENUM_H_

/* Thought: Perhaps a HUD bionic that changes the display of the game?
 * Showing more information or something. */

enum bionic_id {
bio_null,
// Power sources
bio_batteries, bio_metabolics, bio_solar, bio_furnace, bio_ethanol,
// Automatic / Always-On
bio_memory, bio_ears,
bio_eye_enhancer, bio_membrane, bio_targeting,
bio_gills, bio_purifier,
bio_climate, bio_storage, bio_recycler, bio_digestion,	// TODO: Ynnn
bio_tools, bio_shock, bio_heat_absorb,
bio_carbon, bio_armor_head, bio_armor_torso, bio_armor_arms, bio_armor_legs,
// Player Activated
bio_flashlight, bio_night_vision, bio_infrared, 
bio_face_mask,	// TODO
bio_ads, bio_ods, bio_scent_mask,bio_scent_vision, bio_cloak, bio_painkiller,
 bio_nanobots, bio_heatsink, bio_resonator, bio_time_freeze, bio_teleport,
 bio_blood_anal, bio_blood_filter, bio_alarm,
bio_evap, bio_lighter, bio_claws, bio_blaster, bio_laser, bio_emp,
 bio_hydraulics, bio_water_extractor, bio_magnet, bio_fingerhack, bio_lockpick,
bio_ground_sonar,
max_bio_start,
// EVERYTHING below this point is not available to brand-new players
// TODO: all these suckers
bio_banish, bio_gate_out, bio_gate_in, bio_nightfall, bio_drilldown,
bio_heatwave, bio_lightning, bio_tremor, bio_flashflood,
max_bio_good,
// Everything below THIS point is bad bionics, meant to be punishments
bio_dis_shock, bio_dis_acid, bio_drain, bio_noise, bio_power_weakness,
 bio_stiff,
max_bio
};
 
#endif
