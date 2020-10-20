#include "crafting.h"
#include "keypress.h"
#include "game.h"
#include "output.h"
#include "skill.h"
#include "inventory.h"
#include "rng.h"
#include "recent_msg.h"
#include "setvector.h"
#include <sstream>

std::vector<recipe*> recipe::recipes;

// This function just defines the recipes used throughout the game.
void recipe::init()
{
 int id = -1;
 int tl, cl;

 #define RECIPE(result, category, skill1, skill2, difficulty, time) \
tl = 0; cl = 0; id++;\
recipes.push_back( new recipe(id, result, category, skill1, skill2, difficulty,\
                              time) )
 #define TOOL(...)  SET_VECTOR_STRUCT(recipes[id]->tools[tl],component,__VA_ARGS__); tl++
 #define COMP(...)  SET_VECTOR_STRUCT(recipes[id]->components[cl],component,__VA_ARGS__); cl++

/* A recipe will not appear in your menu until your level in the primary skill
 * is at least equal to the difficulty.  At that point, your chance of success
 * is still not great; a good 25% improvement over the difficulty is important
 */

// WEAPONS

 RECIPE(itm_spear_wood, CC_WEAPON, sk_null, sk_null, 0, 800);
 TOOL({ { itm_hatchet, -1 },{ itm_knife_steak, -1 },{ itm_knife_butcher, -1 },
	    { itm_knife_combat, -1 },{ itm_machete, -1 },{ itm_toolset, -1 } });
 COMP({ { itm_stick, 1 }, { itm_broom, 1 },{ itm_mop, 1 },{ itm_2x4, 1 } });

 RECIPE(itm_spear_knife, CC_WEAPON, sk_stabbing, sk_null, 1, 600);
  COMP({ { itm_stick, 1 },{ itm_broom, 1 },{ itm_mop, 1 } });
  COMP({ { itm_knife_steak, 2 },{ itm_knife_combat, 1 } });
  COMP({ { itm_string_36, 1 } });

 RECIPE(itm_longbow, CC_WEAPON, sk_archery, sk_survival, 2, 15000);
  TOOL({ { itm_hatchet, -1 },{ itm_knife_steak, -1 },{ itm_knife_butcher, -1 },
	     { itm_knife_combat, -1 },{ itm_machete, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_stick, 1 } });
  COMP({ { itm_string_36, 2 } });

 RECIPE(itm_arrow_wood, CC_WEAPON, sk_archery, sk_survival, 1, 5000);
  TOOL({ { itm_hatchet, -1 },{ itm_knife_steak, -1 },{ itm_knife_butcher, -1 },
	     { itm_knife_combat, -1 },{ itm_machete, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_stick, 1 },{ itm_broom, 1 },{ itm_mop, 1 },{ itm_2x4, 1 },{ itm_bee_sting, 1 } });
                                 
 RECIPE(itm_nailboard, CC_WEAPON, sk_null, sk_null, 0, 1000);
  TOOL({ { itm_hatchet, -1 },{ itm_hammer, -1 },{ itm_rock, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_2x4, 1 },{ itm_bat, 1 } });
  COMP({ { itm_nail, 6 } });

 RECIPE(itm_molotov, CC_WEAPON, sk_null, sk_null, 0, 500);
  COMP({ { itm_rag, 1 } });
  COMP({ { itm_whiskey, -1 },{ itm_vodka, -1 },{ itm_rum, -1 },{ itm_tequila, -1 },
	     { itm_gasoline, -1 } });

 RECIPE(itm_pipebomb, CC_WEAPON, sk_mechanics, sk_null, 1, 750);
  TOOL({ { itm_hacksaw, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_pipe, 1 } });
  COMP({ { itm_gasoline, 1 },{ itm_shot_bird, 6 },{ itm_shot_00, 2 },{ itm_shot_slug, 2 } });
  COMP({ { itm_string_36, 1 },{ itm_string_6, 1 } });

 RECIPE(itm_shotgun_sawn, CC_WEAPON, sk_gun, sk_null, 1, 500);
  TOOL({ { itm_hacksaw, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_shotgun_d, 1 },{ itm_remington_870, 1 },{ itm_mossberg_500, 1 },
	     { itm_saiga_12, 1 } });

 RECIPE(itm_bolt_wood, CC_WEAPON, sk_mechanics, sk_archery, 1, 5000);
  TOOL({ { itm_hatchet, -1 },{ itm_knife_steak, -1 },{ itm_knife_butcher, -1 },
	     { itm_knife_combat, -1 },{ itm_machete, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_stick, 1 },{ itm_broom, 1 },{ itm_mop, 1 },{ itm_2x4, 1 },{ itm_bee_sting, 1 } });

 RECIPE(itm_crossbow, CC_WEAPON, sk_mechanics, sk_archery, 3, 15000);
  TOOL({ { itm_wrench, -1 } });
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_2x4, 1 },{ itm_stick, 4 } });
  COMP({ { itm_hose, 1 } });

 RECIPE(itm_rifle_22, CC_WEAPON, sk_mechanics, sk_gun, 3, 12000);
  TOOL({ { itm_hacksaw, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_pipe, 1 } });
  COMP({ { itm_2x4, 1 } });

 RECIPE(itm_rifle_9mm, CC_WEAPON, sk_mechanics, sk_gun, 3, 14000);
  TOOL({ { itm_hacksaw, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_pipe, 1 } });
  COMP({ { itm_2x4, 1 } });

 RECIPE(itm_smg_9mm, CC_WEAPON, sk_mechanics, sk_gun, 5, 18000);
  TOOL({ { itm_hacksaw, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_hammer, -1 },{ itm_rock, -1 },{ itm_hatchet, -1 } });
  COMP({ { itm_pipe, 1 } });
  COMP({ { itm_2x4, 2 } });
  COMP({ { itm_nail, 4 } });

 RECIPE(itm_smg_45, CC_WEAPON, sk_mechanics, sk_gun, 5, 20000);
  TOOL({ { itm_hacksaw, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_hammer, -1 },{ itm_rock, -1 },{ itm_hatchet, -1 } });
  COMP({ { itm_pipe, 1 } });
  COMP({ { itm_2x4, 2 } });
  COMP({ { itm_nail, 4 } });

 RECIPE(itm_flamethrower_simple, CC_WEAPON, sk_mechanics, sk_gun, 6, 12000);
  TOOL({ { itm_hacksaw, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_pipe, 1 } });
  COMP({ { itm_hose, 2 } });
  COMP({ { itm_bottle_glass, 4 },{ itm_bottle_plastic, 6 } });

 RECIPE(itm_launcher_simple, CC_WEAPON, sk_mechanics, sk_launcher, 6, 6000);
  TOOL({ { itm_hacksaw, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_pipe, 1 } });
  COMP({ { itm_2x4, 1 } });
  COMP({ { itm_nail, 1 } });

 RECIPE(itm_shot_he, CC_WEAPON, sk_mechanics, sk_gun, 4, 2000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_superglue, 1 } });
  COMP({ { itm_shot_slug, 4 } });
  COMP({ { itm_gasoline, 1 } });

 RECIPE(itm_grenade, CC_WEAPON, sk_mechanics, sk_null, 2, 5000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_superglue, 1 },{ itm_string_36, 1 } });
  COMP({ { itm_can_food, 1 },{ itm_can_drink, 1 },{ itm_canister_empty, 1 } });
  COMP({ { itm_nail, 30 },{ itm_bb, 100 } });
  COMP({ { itm_shot_bird, 6 },{ itm_shot_00, 3 },{ itm_shot_slug, 2 },
	     { itm_gasoline, 1 } });

 RECIPE(itm_chainsaw_off, CC_WEAPON, sk_mechanics, sk_null, 4, 20000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_hammer, -1 },{ itm_hatchet, -1 } });
  TOOL({ { itm_wrench, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_motor, 1 } });
  COMP({ { itm_chain, 1 } });

 RECIPE(itm_smokebomb, CC_WEAPON, sk_cooking, sk_mechanics, 3, 7500);
  TOOL({ { itm_screwdriver, -1 },{ itm_wrench, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_water, 1 },{ itm_salt_water, 1 } });
  COMP({ { itm_candy, 1 },{ itm_cola, 1 } });
  COMP({ { itm_vitamins, 10 },{ itm_aspirin, 8 } });
  COMP({ { itm_canister_empty, 1 },{ itm_can_food, 1 } });
  COMP({ { itm_superglue, 1 } });

 RECIPE(itm_gasbomb, CC_WEAPON, sk_cooking, sk_mechanics, 4, 8000);
  TOOL({ { itm_screwdriver, -1 },{ itm_wrench, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_bleach, 2 } });
  COMP({ { itm_ammonia, 2 } });
  COMP({ { itm_canister_empty, 1 },{ itm_can_food, 1 } });
  COMP({ { itm_superglue, 1 } });

 RECIPE(itm_nx17, CC_WEAPON, sk_electronics, sk_mechanics, 8, 40000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_soldering_iron, 6 },{ itm_toolset, 6 } });
  COMP({ { itm_vacutainer, 1 } });
  COMP({ { itm_power_supply, 8 } });
  COMP({ { itm_amplifier, 8 } });

 RECIPE(itm_mininuke, CC_WEAPON, sk_mechanics, sk_electronics, 10, 40000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_wrench, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_can_food, 2 },{ itm_steel_chunk, 2 },{ itm_canister_empty, 1 } });
  COMP({ { itm_plut_cell, 6 } });
  COMP({ { itm_battery, 2 } });
  COMP({ { itm_power_supply, 1 } });

/*
 * We need a some Chemicals which arn't implemented to realistically craft this!
RECIPE(itm_c4, CC_WEAPON, sk_mechanics, sk_electronics, 4, 8000);
 TOOL({ { itm_screwdriver, -1 } });
 COMP({ { itm_can_food, 1 },{ itm_steel_chunk, 1 },{ itm_canister_empty, 1 } });
 COMP({ { itm_battery, 1 } });
 COMP({ { itm_superglue,1 } });
 COMP({ { itm_soldering_iron,1 } });
 COMP({ { itm_power_supply, 1 } });
*/

// FOOD

 RECIPE(itm_meat_cooked, CC_FOOD, sk_cooking, sk_null, 0, 5000);
  TOOL({ { itm_hotplate, 7 },{ itm_toolset, 4 },{ itm_fire, -1 } });
  TOOL({ { itm_pan, -1 },{ itm_pot, -1 } });
  COMP({ { itm_meat, 1 } });

 RECIPE(itm_dogfood, CC_FOOD, sk_cooking, sk_null, 4, 10000);
  TOOL({ { itm_hotplate, 6 },{ itm_toolset, 3 },{ itm_fire, -1 } });
  TOOL({ { itm_pot, -1 } });
  COMP({ { itm_meat, 1 } });
  COMP({ { itm_veggy,1 } });
  COMP({ { itm_water,1 } });

 RECIPE(itm_veggy_cooked, CC_FOOD, sk_cooking, sk_null, 0, 4000);
  TOOL({ { itm_hotplate, 5 },{ itm_toolset, 3 },{ itm_fire, -1 } });
  TOOL({ { itm_pan, -1 },{ itm_pot, -1 } });
  COMP({ { itm_veggy, 1} });

 RECIPE(itm_spaghetti_cooked, CC_FOOD, sk_cooking, sk_null, 0, 10000);
  TOOL({ { itm_hotplate, 4 },{ itm_toolset, 2 },{ itm_fire, -1} });
  TOOL({ { itm_pot, -1} });
  COMP({ { itm_spaghetti_raw, 1} });
  COMP({ { itm_water, 1} });

 RECIPE(itm_cooked_dinner, CC_FOOD, sk_cooking, sk_null, 0, 5000);
  TOOL({ { itm_hotplate, 3 },{ itm_toolset, 2 },{ itm_fire, -1} });
  COMP({ { itm_frozen_dinner, 1} });

 RECIPE(itm_macaroni_cooked, CC_FOOD, sk_cooking, sk_null, 1, 10000);
  TOOL({ { itm_hotplate, 4 },{ itm_toolset, 2 },{ itm_fire, -1} });
  TOOL({ { itm_pot, -1} });
  COMP({ { itm_macaroni_raw, 1} });
  COMP({ { itm_water, 1} });

 RECIPE(itm_potato_baked, CC_FOOD, sk_cooking, sk_null, 1, 15000);
  TOOL({ { itm_hotplate, 3 },{ itm_toolset, 2 },{ itm_fire, -1} });
  TOOL({ { itm_pan, -1 },{ itm_pot, -1} });
  COMP({ { itm_potato_raw, 1} });

 RECIPE(itm_tea, CC_FOOD, sk_cooking, sk_null, 0, 4000);
  TOOL({ { itm_hotplate, 2 },{ itm_toolset, 1 },{ itm_fire, -1} });
  TOOL({ { itm_pot, -1} });
  COMP({ { itm_tea_raw, 1} });
  COMP({ { itm_water, 1} });

 RECIPE(itm_coffee, CC_FOOD, sk_cooking, sk_null, 0, 4000);
  TOOL({ { itm_hotplate, 2 },{ itm_toolset, 1 },{ itm_fire, -1} });
  TOOL({ { itm_pot, -1} });
  COMP({ { itm_coffee_raw, 1} });
  COMP({ { itm_water, 1} });

 RECIPE(itm_oj, CC_FOOD, sk_cooking, sk_null, 1, 5000);
  TOOL({ { itm_rock, -1 },{ itm_toolset, -1} });
  COMP({ { itm_orange, 2} });
  COMP({ { itm_water, 1} });

 RECIPE(itm_apple_cider, CC_FOOD, sk_cooking, sk_null, 2, 7000);
  TOOL({ { itm_rock, -1 }, { itm_toolset, -1} });
  COMP({ { itm_apple, 3} });
 
 RECIPE(itm_jerky, CC_FOOD, sk_cooking, sk_null, 3, 30000);
  TOOL({ { itm_hotplate, 10 },{ itm_toolset, 5 },{ itm_fire, -1} });
  COMP({ { itm_salt_water, 1 },{ itm_salt, 4} });
  COMP({ { itm_meat, 1} });

 RECIPE(itm_V8, CC_FOOD, sk_cooking, sk_null, 2, 5000);
  COMP({ { itm_tomato, 1} });
  COMP({ { itm_broccoli, 1} });
  COMP({ { itm_zucchini, 1} });

 RECIPE(itm_broth, CC_FOOD, sk_cooking, sk_null, 2, 10000);
  TOOL({ { itm_hotplate, 5 },{ itm_toolset, 3 },{ itm_fire, -1} });
  TOOL({ { itm_pot, -1} });
  COMP({ { itm_water, 1} });
  COMP({ { itm_broccoli, 1 },{ itm_zucchini, 1 },{ itm_veggy, 1} });

 RECIPE(itm_soup, CC_FOOD, sk_cooking, sk_null, 2, 10000);
  TOOL({ { itm_hotplate, 5 },{ itm_toolset, 3 },{ itm_fire, -1} });
  TOOL({ { itm_pot, -1} });
  COMP({ { itm_broth, 2} });
  COMP({ { itm_macaroni_raw, 1 },{ itm_potato_raw, 1} });
  COMP({ { itm_tomato, 2 },{ itm_broccoli, 2 },{ itm_zucchini, 2 },{ itm_veggy, 2} });

 RECIPE(itm_bread, CC_FOOD, sk_cooking, sk_null, 4, 20000);
  TOOL({ { itm_hotplate, 8 },{ itm_toolset, 4 },{ itm_fire, -1} });
  TOOL({ { itm_pot, -1} });
  COMP({ { itm_flour, 3} });
  COMP({ { itm_water, 2} });

 RECIPE(itm_pie, CC_FOOD, sk_cooking, sk_null, 3, 25000);
  TOOL({ { itm_hotplate, 6 },{ itm_toolset, 3 },{ itm_fire, -1} });
  TOOL({ { itm_pan, -1} });
  COMP({ { itm_flour, 2} });
  COMP({ { itm_strawberries, 2 },{ itm_apple, 2 },{ itm_blueberries, 2} });
  COMP({ { itm_sugar, 2} });
  COMP({ { itm_water, 1} });

 RECIPE(itm_pizza, CC_FOOD, sk_cooking, sk_null, 3, 20000);
  TOOL({ { itm_hotplate, 8 },{ itm_toolset, 4 },{ itm_fire, -1} });
  TOOL({ { itm_pan, -1} });
  COMP({ { itm_flour, 2} });
  COMP({ { itm_veggy, 1 },{ itm_tomato, 2 },{ itm_broccoli, 1} });
  COMP({ { itm_sauce_pesto, 1 },{ itm_sauce_red, 1} });
  COMP({ { itm_water, 1} });

 RECIPE(itm_meth, CC_FOOD, sk_cooking, sk_null, 5, 20000);
  TOOL({ { itm_hotplate, 15 },{ itm_toolset, 8 },{ itm_fire, -1} });
  TOOL({ { itm_bottle_glass, -1 },{ itm_hose, -1} });
  COMP({ { itm_dayquil, 2 },{ itm_royal_jelly, 1} });
  COMP({ { itm_aspirin, 40} });
  COMP({ { itm_caffeine, 20 },{ itm_adderall, 5 },{ itm_energy_drink, 2} });

 RECIPE(itm_royal_jelly, CC_FOOD, sk_cooking, sk_null, 5, 5000);
  COMP({ { itm_honeycomb, 1} });
  COMP({ { itm_bleach, 2 },{ itm_purifier, 1} });

 RECIPE(itm_heroin, CC_FOOD, sk_cooking, sk_null, 6, 2000);
  TOOL({ { itm_hotplate, 3 },{ itm_toolset, 2 },{ itm_fire, -1} });
  TOOL({ { itm_pan, -1 },{ itm_pot, -1} });
  COMP({ { itm_salt_water, 1 },{ itm_salt, 4} });
  COMP({ { itm_oxycodone, 40} });

 RECIPE(itm_mutagen, CC_FOOD, sk_cooking, sk_firstaid, 8, 10000);
  TOOL({ { itm_hotplate, 25 },{ itm_toolset, 12 },{ itm_fire, -1} });
  COMP({ { itm_meat_tainted, 3 },{ itm_veggy_tainted, 5 },{ itm_fetus, 1 },{ itm_arm, 2 },
	     { itm_leg, 2 } });
  COMP({ { itm_bleach, 2} });
  COMP({ { itm_ammonia, 1} });

 RECIPE(itm_purifier, CC_FOOD, sk_cooking, sk_firstaid, 9, 10000);
  TOOL({ { itm_hotplate, 25 },{ itm_toolset, 12 },{ itm_fire, -1} });
  COMP({ { itm_royal_jelly, 4 },{ itm_mutagen, 2} });
  COMP({ { itm_bleach, 3} });
  COMP({ { itm_ammonia, 2} });

// ELECTRONICS

 RECIPE(itm_antenna, CC_ELECTRONIC, sk_null, sk_null, 0, 3000);
  TOOL({ { itm_hacksaw, -1 },{ itm_toolset, -1} });
  COMP({ { itm_radio, 1 },{ itm_two_way_radio, 1 },{ itm_motor, 1 },{ itm_knife_butter, 2} });

 RECIPE(itm_amplifier, CC_ELECTRONIC, sk_electronics, sk_null, 1, 4000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1} });
  COMP({ { itm_flashlight, 1 },{ itm_radio, 1 },{ itm_two_way_radio, 1 },{ itm_geiger_off, 1 },
	     { itm_goggles_nv, 1 },{ itm_transponder, 2 } });

 RECIPE(itm_power_supply, CC_ELECTRONIC, sk_electronics, sk_null, 1, 6500);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_soldering_iron, 3 },{ itm_toolset, 3 } });
  COMP({ { itm_amplifier, 2 },{ itm_soldering_iron, 1 },{ itm_electrohack, 1 },
	     { itm_battery, 800 },{ itm_geiger_off, 1 } });

 RECIPE(itm_receiver, CC_ELECTRONIC, sk_electronics, sk_null, 2, 12000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_soldering_iron, 4 },{ itm_toolset, 4 } });
  COMP({ { itm_amplifier, 2 },{ itm_radio, 1 },{ itm_two_way_radio, 1 } });

 RECIPE(itm_transponder, CC_ELECTRONIC, sk_electronics, sk_null, 2, 14000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_soldering_iron, 7 },{ itm_toolset, 7 } });
  COMP({ { itm_receiver, 3 },{ itm_two_way_radio, 1 } });

 RECIPE(itm_flashlight, CC_ELECTRONIC, sk_electronics, sk_null, 1, 10000);
  COMP({ { itm_amplifier, 1 } });
  COMP({ { itm_bottle_plastic, 1 },{ itm_bottle_glass, 1 },{ itm_can_drink, 1 } });

 RECIPE(itm_soldering_iron, CC_ELECTRONIC, sk_electronics, sk_null, 1, 20000);
  COMP({ { itm_screwdriver, 1 },{ itm_antenna, 1 },{ itm_xacto, 1 },{ itm_knife_butter, 1 } });
  COMP({ { itm_power_supply, 1 } });

 RECIPE(itm_battery, CC_ELECTRONIC, sk_electronics, sk_mechanics, 2, 5000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_ammonia, 1 },{ itm_lemon, 1 } });
  COMP({ { itm_steel_chunk, 1 },{ itm_knife_butter, 1 },{ itm_knife_steak, 1 },
	     { itm_bolt_steel, 1 } });
  COMP({ { itm_can_drink, 1 },{ itm_can_food, 1 } });

 RECIPE(itm_coilgun, CC_WEAPON, sk_electronics, sk_null, 3, 25000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_soldering_iron, 10 },{ itm_toolset, 10 } });
  COMP({ { itm_pipe, 1 } });
  COMP({ { itm_power_supply, 1 } });
  COMP({ { itm_amplifier, 1 } });

 RECIPE(itm_radio, CC_ELECTRONIC, sk_electronics, sk_null, 2, 25000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_soldering_iron, 10 },{ itm_toolset, 10 } });
  COMP({ { itm_receiver, 1 } });
  COMP({ { itm_antenna, 1 } });

 RECIPE(itm_water_purifier, CC_ELECTRONIC, sk_mechanics,sk_electronics,3,25000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_hotplate, 2 } });
  COMP({ { itm_bottle_glass, 2 },{ itm_bottle_plastic, 5 } });
  COMP({ { itm_hose, 1 } });

 RECIPE(itm_hotplate, CC_ELECTRONIC, sk_electronics, sk_null, 3, 30000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_soldering_iron, 1 },{ itm_amplifier, 1 } });
  COMP({ { itm_pan, 1 }, { itm_pot, 1 }, { itm_knife_butcher, 2 }, { itm_knife_steak, 6 },
	  { itm_knife_butter, 6 }, { itm_muffler, 1 } });

 RECIPE(itm_tazer, CC_ELECTRONIC, sk_electronics, sk_null, 3, 25000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_soldering_iron, 10 },{ itm_toolset, 10 } });
  COMP({ { itm_amplifier, 1 } });
  COMP({ { itm_power_supply, 1 } });

 RECIPE(itm_two_way_radio, CC_ELECTRONIC, sk_electronics, sk_null, 4, 30000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_soldering_iron, 14 },{ itm_toolset, 14 } });
  COMP({ { itm_amplifier, 1 } });
  COMP({ { itm_transponder, 1 } });
  COMP({ { itm_receiver, 1 } });
  COMP({ { itm_antenna, 1 } });

 RECIPE(itm_electrohack, CC_ELECTRONIC, sk_electronics, sk_computer, 4, 35000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_soldering_iron, 10 },{ itm_toolset, 10 } });
  COMP({ { itm_processor, 1 } });
  COMP({ { itm_RAM, 1 } });

 RECIPE(itm_EMPbomb, CC_ELECTRONIC, sk_electronics, sk_null, 4, 32000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_soldering_iron, 6 },{ itm_toolset, 6 } });
  COMP({ { itm_superglue, 1 },{ itm_string_36, 1 } });
  COMP({ { itm_can_food, 1 },{ itm_can_drink, 1 },{ itm_canister_empty, 1 } });
  COMP({ { itm_power_supply, 1 },{ itm_amplifier, 1 } });

 RECIPE(itm_mp3, CC_ELECTRONIC, sk_electronics, sk_computer, 5, 40000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_soldering_iron, 5 },{ itm_toolset, 5 } });
  COMP({ { itm_superglue, 1 } });
  COMP({ { itm_antenna, 1 } });
  COMP({ { itm_amplifier, 1 } });

 RECIPE(itm_geiger_off, CC_ELECTRONIC, sk_electronics, sk_null, 5, 35000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_soldering_iron, 14 },{ itm_toolset, 14 } });
  COMP({ { itm_power_supply, 1 } });
  COMP({ { itm_amplifier, 2 } });

 RECIPE(itm_UPS_off, CC_ELECTRONIC, sk_electronics, sk_null, 5, 45000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_soldering_iron, 24 },{ itm_toolset, 24 } });
  COMP({ { itm_power_supply, 4 } });
  COMP({ { itm_amplifier, 3 } });

 RECIPE(itm_bionics_battery, CC_ELECTRONIC, sk_electronics, sk_null, 6, 50000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_soldering_iron, 20 },{ itm_toolset, 20 } });
  COMP({ { itm_UPS_off, 1 },{ itm_power_supply, 6 } });
  COMP({ { itm_amplifier, 4 } });
  COMP({ { itm_plut_cell, 1 } });

 RECIPE(itm_teleporter, CC_ELECTRONIC, sk_electronics, sk_null, 8, 50000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_wrench, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_soldering_iron, 16 },{ itm_toolset, 16 } });
  COMP({ { itm_power_supply, 3 },{ itm_plut_cell, 5 } });
  COMP({ { itm_amplifier, 3 } });
  COMP({ { itm_transponder, 3 } });

// ARMOR

 RECIPE(itm_mocassins, CC_ARMOR, sk_tailor, sk_null, 1, 30000);
  TOOL({ { itm_sewing_kit,  5 } });
  COMP({ { itm_fur, 2 } });

 RECIPE(itm_boots, CC_ARMOR, sk_tailor, sk_null, 2, 35000);
  TOOL({ { itm_sewing_kit, 10 } });
  COMP({ { itm_leather, 2 } });

 RECIPE(itm_jeans, CC_ARMOR, sk_tailor, sk_null, 2, 45000);
  TOOL({ { itm_sewing_kit, 10 } });
  COMP({ { itm_rag, 6 } });

 RECIPE(itm_pants_cargo, CC_ARMOR, sk_tailor, sk_null, 3, 48000);
  TOOL({ { itm_sewing_kit, 16 } });
  COMP({ { itm_rag, 8 } });

 RECIPE(itm_pants_leather, CC_ARMOR, sk_tailor, sk_null, 4, 50000);
  TOOL({ { itm_sewing_kit, 10 } });
  COMP({ { itm_leather, 6 } });

 RECIPE(itm_tank_top, CC_ARMOR, sk_tailor, sk_null, 2, 38000);
  TOOL({ { itm_sewing_kit, 4 } });
  COMP({ { itm_rag, 4 } });

 RECIPE(itm_hoodie, CC_ARMOR, sk_tailor, sk_null, 3, 40000);
  TOOL({ { itm_sewing_kit, 14 } });
  COMP({ { itm_rag, 12 } });

 RECIPE(itm_trenchcoat, CC_ARMOR, sk_tailor, sk_null, 3, 42000);
  TOOL({ { itm_sewing_kit, 24 } });
  COMP({ { itm_rag, 11 } });

 RECIPE(itm_coat_fur, CC_ARMOR, sk_tailor, sk_null, 4, 100000);
  TOOL({ { itm_sewing_kit, 20 } });
  COMP({ { itm_fur, 10 } });

 RECIPE(itm_jacket_leather, CC_ARMOR, sk_tailor, sk_null, 5, 150000);
  TOOL({ { itm_sewing_kit, 30 } });
  COMP({ { itm_leather, 8 } });

 RECIPE(itm_gloves_light, CC_ARMOR, sk_tailor, sk_null, 1, 10000);
  TOOL({ { itm_sewing_kit, 2 } });
  COMP({ { itm_rag, 2 } });

 RECIPE(itm_gloves_fingerless, CC_ARMOR, sk_tailor, sk_null, 3, 16000);
  TOOL({ { itm_sewing_kit, 6 } });
  COMP({ { itm_leather, 2 } });

 RECIPE(itm_mask_filter, CC_ARMOR, sk_mechanics, sk_tailor, 1, 5000);
  COMP({ { itm_bottle_plastic, 1 },{ itm_bag_plastic, 2 } });
  COMP({ { itm_muffler, 1 },{ itm_bandana, 2 },{ itm_rag, 2 },{ itm_wrapper, 4 } });

 RECIPE(itm_mask_gas, CC_ARMOR, sk_tailor, sk_null, 3, 20000);
  TOOL({ { itm_wrench, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_goggles_swim, 2 },{ itm_goggles_ski, 1 } });
  COMP({ { itm_mask_filter, 3 },{ itm_muffler, 1 } });
  COMP({ { itm_hose, 1 } });

 RECIPE(itm_glasses_safety, CC_ARMOR, sk_tailor, sk_null, 1, 8000);
  TOOL({ { itm_scissors, -1 },{ itm_xacto, -1 },{ itm_knife_steak, -1 },
	     { itm_knife_combat, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_string_36, 1 },{ itm_string_6, 2 } });
  COMP({ { itm_bottle_plastic, 1 } });

 RECIPE(itm_goggles_nv, CC_ARMOR, sk_electronics, sk_tailor, 5, 40000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_goggles_ski, 1 },{ itm_goggles_welding, 1 },{ itm_mask_gas, 1 } });
  COMP({ { itm_power_supply, 1 } });
  COMP({ { itm_amplifier, 3 } });

 RECIPE(itm_hat_fur, CC_ARMOR, sk_tailor, sk_null, 2, 40000);
  TOOL({ { itm_sewing_kit, 8 } });
  COMP({ { itm_fur, 3 } });

 RECIPE(itm_helmet_chitin, CC_ARMOR, sk_tailor, sk_null, 6,  60000);
  COMP({ { itm_string_36, 1 },{ itm_string_6, 5 } });
  COMP({ { itm_chitin_piece, 5 } });

 RECIPE(itm_armor_chitin, CC_ARMOR, sk_tailor, sk_null,  7, 100000);
  COMP({ { itm_string_36, 2 },{ itm_string_6, 12 } });
  COMP({ { itm_chitin_piece, 15 } });

 RECIPE(itm_backpack, CC_ARMOR, sk_tailor, sk_null, 3, 50000);
  TOOL({ { itm_sewing_kit, 20 } });
  COMP({ { itm_rag, 20 },{ itm_fur, 16 },{ itm_leather, 12 } });

// MISC

 RECIPE(itm_superglue, CC_MISC, sk_cooking, sk_null, 2, 12000);
  TOOL({ { itm_hotplate, 5 },{ itm_toolset, 3 },{ itm_fire, -1 } });
  COMP({ { itm_water, 1 } });
  COMP({ { itm_bleach, 1 },{ itm_ant_egg, 1 } });

 RECIPE(itm_2x4, CC_MISC, sk_null, sk_null, 0, 8000);
  TOOL({ { itm_saw, -1 } });
  COMP({ { itm_stick, 1 } });

 RECIPE(itm_frame, CC_MISC, sk_mechanics, sk_null, 1, 8000);
  TOOL({ { itm_welder, 50 } });
  COMP({ { itm_steel_lump, 3 } });

 RECIPE(itm_steel_plate, CC_MISC, sk_mechanics, sk_null,4, 12000);
  TOOL({ { itm_welder, 100 } });
  COMP({ { itm_steel_lump, 8 } });

 RECIPE(itm_spiked_plate, CC_MISC, sk_mechanics, sk_null, 4, 12000);
  TOOL({ { itm_welder, 120 } });
  COMP({ { itm_steel_lump, 8 } });
  COMP({ { itm_steel_chunk, 4 } });

 RECIPE(itm_hard_plate, CC_MISC, sk_mechanics, sk_null, 4, 12000);
  TOOL({ { itm_welder, 300 } });
  COMP({ { itm_steel_lump, 24 } });

 RECIPE(itm_crowbar, CC_MISC, sk_mechanics, sk_null, 1, 1000);
  TOOL({ { itm_hatchet, -1 },{ itm_hammer, -1 },{ itm_rock, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_pipe, 1 } });

 RECIPE(itm_bayonet, CC_MISC, sk_gun, sk_null, 1, 500);
  COMP({ { itm_knife_steak, 3 },{ itm_knife_combat, 1 } });
  COMP({ { itm_string_36, 1 } });

 RECIPE(itm_tripwire, CC_MISC, sk_traps, sk_null, 1, 500);
  COMP({ { itm_string_36, 1 } });
  COMP({ { itm_superglue, 1 } });

 RECIPE(itm_board_trap, CC_MISC, sk_traps, sk_null, 2, 2500);
  TOOL({ { itm_hatchet, -1 },{ itm_hammer, -1 },{ itm_rock, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_2x4, 3 } });
  COMP({ { itm_nail, 20 } });

 RECIPE(itm_beartrap, CC_MISC, sk_mechanics, sk_traps, 2, 3000);
  TOOL({ { itm_wrench, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_steel_chunk, 2 } });

 RECIPE(itm_crossbow_trap, CC_MISC, sk_mechanics, sk_traps, 3, 4500);
  COMP({ { itm_crossbow, 1 } });
  COMP({ { itm_bolt_steel, 1 },{ itm_bolt_wood, 4 } });
  COMP({ { itm_string_36, 1 },{ itm_string_6, 2 } });

 RECIPE(itm_shotgun_trap, CC_MISC, sk_mechanics, sk_traps, 3, 5000);
  COMP({ { itm_shotgun_sawn, 1 } });
  COMP({ { itm_shot_00, 2 } });
  COMP({ { itm_string_36, 1 },{ itm_string_6, 2 } });

 RECIPE(itm_blade_trap, CC_MISC, sk_mechanics, sk_traps, 4, 8000);
  TOOL({ { itm_wrench, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_motor, 1 } });
  COMP({ { itm_machete, 1 } });
  COMP({ { itm_string_36, 1 } });

RECIPE(itm_boobytrap, CC_MISC, sk_mechanics, sk_traps,3,5000);
  COMP({ { itm_grenade,1 } });
  COMP({ { itm_string_6,1 } });
  COMP({ { itm_can_food,1 } });

 RECIPE(itm_landmine, CC_WEAPON, sk_traps, sk_mechanics, 5, 10000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_superglue, 1 } });
  COMP({ { itm_can_food, 1 },{ itm_steel_chunk, 1 },{ itm_canister_empty, 1 } });
  COMP({ { itm_nail, 100 },{ itm_bb, 200 } });
  COMP({ { itm_shot_bird, 30 },{ itm_shot_00, 15 },{ itm_shot_slug, 12 },{ itm_gasoline, 3 },
	     { itm_grenade, 1 } });

 RECIPE(itm_bandages, CC_MISC, sk_firstaid, sk_null, 1, 500);
  COMP({ { itm_rag, 1 } });
  COMP({ { itm_superglue, 1 } });

 RECIPE(itm_silencer, CC_MISC, sk_mechanics, sk_null, 1, 650);
  TOOL({ { itm_hacksaw, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_muffler, 1 },{ itm_rag, 4 } });
  COMP({ { itm_pipe, 1 } });

 RECIPE(itm_pheromone, CC_MISC, sk_cooking, sk_null, 3, 1200);
  TOOL({ { itm_hotplate, 18 },{ itm_toolset, 9 },{ itm_fire, -1 } });
  COMP({ { itm_meat_tainted, 1 } });
  COMP({ { itm_ammonia, 1 } });

 RECIPE(itm_laser_pack, CC_MISC, sk_electronics, sk_null, 5, 10000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  COMP({ { itm_superglue, 1 } });
  COMP({ { itm_plut_cell, 1 } });

 RECIPE(itm_bot_manhack, CC_MISC, sk_electronics, sk_computer, 6, 8000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_soldering_iron, 10 },{ itm_toolset, 10 } });
  COMP({ { itm_knife_steak, 4 },{ itm_knife_combat, 2 } });
  COMP({ { itm_processor, 1 } });
  COMP({ { itm_RAM, 1 } });
  COMP({ { itm_power_supply, 1 } });
  COMP({ { itm_battery, 400 },{ itm_plut_cell, 1 } });

 RECIPE(itm_bot_turret, CC_MISC, sk_electronics, sk_computer, 7, 9000);
  TOOL({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  TOOL({ { itm_soldering_iron, 14 },{ itm_toolset, 14 } });
  COMP({ { itm_smg_9mm, 1 },{ itm_uzi, 1 },{ itm_tec9, 1 },{ itm_calico, 1 },{ itm_hk_mp5, 1 } });
  COMP({ { itm_processor, 2 } });
  COMP({ { itm_RAM, 2 } });
  COMP({ { itm_power_supply, 1 } });
  COMP({ { itm_battery, 500 },{ itm_plut_cell, 1 } });
}

enum {
	CRAFTING_WIN_HEIGHT = VIEW - TABBED_HEADER_HEIGHT
};

inventory crafting_inventory(const map& m, const player& u)  // 2020-05-28 NPC-valid
{
    inventory crafting_inv;
    crafting_inv.form_from_map(m, u.pos, PICKUP_RANGE);
    crafting_inv += u.inv;
    crafting_inv += u.weapon;
    if (u.has_bionic(bio_tools)) {
        item tools(item::types[itm_toolset], messages.turn);
        tools.charges = u.power_level;
        crafting_inv += tools;
    }
    return crafting_inv;
}

void game::craft()
{
 if (u.morale_level() < MIN_MORALE_CRAFT) {	// See morale.h
  messages.add("Your morale is too low to craft...");
  return;
 }
 static constexpr const char* const labels[] = {"WEAPONS", "FOOD", "ELECTRONICS", "ARMOR", "MISC", nullptr};
 WINDOW *w_head = newwin(TABBED_HEADER_HEIGHT, SCREEN_WIDTH, 0, 0);
 WINDOW *w_data = newwin(CRAFTING_WIN_HEIGHT, SCREEN_WIDTH, TABBED_HEADER_HEIGHT, 0);
 craft_cat tab = CC_WEAPON;
 std::vector<const recipe*> current;
 std::vector<bool> available;
 item tmp;
 int line = 0, xpos, ypos;
 bool redraw = true;
 bool done = false;
 int ch;

 inventory crafting_inv(crafting_inventory(m, u));

 do {
  if (redraw) { // When we switch tabs, redraw the header
   redraw = false;
   line = 0;
   draw_tabs(w_head, tab - 1, labels); // \todo C:Whales colors were c_ltgray, h_ltgray rather than c_white, h_white
// Set current to all recipes in the current tab; available are possible to make
   pick_recipes(current, available, tab);
  }

// Clear the screen of recipe data, and draw it anew
  werase(w_data);
  mvwprintz(w_data, 20, 0, c_white, "Press ? to describe object.  Press <ENTER> to attempt to craft object.");
  wrefresh(w_data);
  for (int i = 0; i < current.size() && i < CRAFTING_WIN_HEIGHT; i++) {
   if (i == line)
    mvwprintz(w_data, i, 0, (available[i] ? h_white : h_dkgray),
		item::types[current[i]->result]->name.c_str());
   else
    mvwprintz(w_data, i, 0, (available[i] ? c_white : c_dkgray),
		item::types[current[i]->result]->name.c_str());
  }
  if (current.size() > 0) {
   nc_color col = (available[line] ? c_white : c_dkgray);
   mvwprintz(w_data, 0, VBAR_X, col, "Primary skill: %s",
             (current[line]->sk_primary == sk_null ? "N/A" :
              skill_name(current[line]->sk_primary)));
   mvwprintz(w_data, 1, VBAR_X, col, "Secondary skill: %s",
             (current[line]->sk_secondary == sk_null ? "N/A" :
              skill_name(current[line]->sk_secondary)));
   mvwprintz(w_data, 2, VBAR_X, col, "Difficulty: %d", current[line]->difficulty);
   if (current[line]->sk_primary == sk_null)
    mvwprintz(w_data, 3, VBAR_X, col, "Your skill level: N/A");
   else
    mvwprintz(w_data, 3, VBAR_X, col, "Your skill level: %d", u.sklevel[current[line]->sk_primary]);
   // for some reason 1 turn == 100 time here
   if (current[line]->time >= MINUTES(100))
    mvwprintz(w_data, 4, VBAR_X, col, "Time to complete: %d minutes", current[line]->time / MINUTES(100));
   else
    mvwprintz(w_data, 4, VBAR_X, col, "Time to complete: %d turns", current[line]->time / 100);
   mvwprintz(w_data, 5, VBAR_X, col, "Tools required:");
   if (current[line]->tools[0].size() == 0) {
    mvwputch(w_data, 6, VBAR_X, col, '>');
    mvwprintz(w_data, 6, VBAR_X + 2, c_green, "NONE");
    ypos = 6;
   } else {
    ypos = 5;
// Loop to print the required tools
    for (int i = 0; i < 5 && current[line]->tools[i].size() > 0; i++) {
     ypos++;
     xpos = VBAR_X + 2;
     mvwputch(w_data, ypos, VBAR_X, col, '>');

     for (int j = 0; j < current[line]->tools[i].size(); j++) {
	  const component& tool = current[line]->tools[i][j];
      const itype_id type = tool.type;
	  const int charges = tool.count;
      nc_color toolcol = c_red;

	  if (charges <= 0 ? crafting_inv.has_amount(type, 1) : crafting_inv.has_charges(type, charges))
       toolcol = c_green;

      std::ostringstream toolinfo;
      toolinfo << item::types[type]->name + " ";
      if (charges > 0)
       toolinfo << "(" << charges << " charges) ";
      std::string toolname = toolinfo.str();
      if (xpos + toolname.length() >= SCREEN_WIDTH) {
       xpos = VBAR_X + 2;
       ypos++;
      }
      mvwprintz(w_data, ypos, xpos, toolcol, toolname.c_str());
      xpos += toolname.length();
      if (j < current[line]->tools[i].size() - 1) {
       if (xpos >= SCREEN_WIDTH - 3) {
        xpos = VBAR_X + 2;
        ypos++;
       }
       mvwprintz(w_data, ypos, xpos, c_white, "OR ");
       xpos += 3;
      }
     }
    }
   }
 // Loop to print the required components
   ypos++;
   mvwprintz(w_data, ypos, VBAR_X, col, "Components required:");
   for (int i = 0; i < 5; i++) {
    const size_t ub = current[line]->components[i].size();
    if (0 >= ub) continue;
    mvwputch(w_data, ++ypos, VBAR_X, col, '>');
    xpos = VBAR_X + 2;
    for (int j = 0; j < ub; j++) {
	 const component& comp = current[line]->components[i][j];
     const int count = comp.count;
     const itype_id type = comp.type;
	 const itype* const i_type = item::types[type];
     nc_color compcol = c_red;
     if (i_type->count_by_charges() && count > 0)  {
      if (crafting_inv.has_charges(type, count))
       compcol = c_green;
     } else if (crafting_inv.has_amount(type, abs(count)))
      compcol = c_green;
     std::ostringstream dump;
     dump << abs(count) << "x " << i_type->name << " ";
     std::string compname = dump.str();
     if (xpos + compname.length() >= SCREEN_WIDTH) {
      ypos++;
      xpos = VBAR_X + 2;
     }
     mvwprintz(w_data, ypos, xpos, compcol, compname.c_str());
     xpos += compname.length();
     if (j < ub - 1) {
      if (xpos >= SCREEN_WIDTH - 3) {
       ypos++;
       xpos = VBAR_X + 2;
      }
      mvwprintz(w_data, ypos, xpos, c_white, "OR ");
      xpos += 3;
     }
    }
   }
  }

  wrefresh(w_data);
  ch = input();
  switch (ch) {
  case '<':
   if (tab == CC_WEAPON)
    tab = CC_MISC;
   else
    tab = craft_cat(int(tab) - 1);
   redraw = true;
   break;
  case '>':
   if (tab == CC_MISC)
    tab = CC_WEAPON;
   else
    tab = craft_cat(int(tab) + 1);
   redraw = true;
   break;
  case 'j':
   line++;
   break;
  case 'k':
   line--;
   break;
  case '\n':
   if (!available[line]) popup("You can't do that!");
   else if (item::types[current[line]->result]->m1 == LIQUID &&
            !u.has_watertight_container())
    popup("You don't have anything to store that liquid in!");
   else {
    make_craft(current[line]);
    done = true;
   }
   break;
  case '?':
   tmp = item(item::types[current[line]->result], 0);
   full_screen_popup(tmp.info(true).c_str());
   redraw = true;
   break;
  }
  if (line < 0) line = current.size() - 1;
  else if (line >= current.size()) line = 0;
 } while (ch != KEY_ESCAPE && ch != 'q' && ch != 'Q' && !done);

 werase(w_head);
 werase(w_data);
 delwin(w_head);
 delwin(w_data);
 refresh_all();
}

void game::pick_recipes(std::vector<const recipe*> &current,
                        std::vector<bool> &available, craft_cat tab)
{
 const inventory crafting_inv(crafting_inventory(m, u));

 bool have_tool[5], have_comp[5];

 current.clear();
 available.clear();
 for (const recipe* const tmp : recipe::recipes) {
// Check if the category matches the tab, and we have the requisite skills
  if (    tmp->category == tab
      && (sk_null == tmp->sk_primary   || u.sklevel[tmp->sk_primary] >= tmp->difficulty)
      && (sk_null == tmp->sk_secondary || 0 < u.sklevel[tmp->sk_secondary]))
  current.push_back(tmp);
  available.push_back(false);
 }
 for (int i = 0; i < current.size() && i < CRAFTING_WIN_HEIGHT; i++) {
//Check if we have the requisite tools and components
  for (int j = 0; j < 5; j++) {
   have_tool[j] = current[i]->tools[j].empty();
   have_comp[j] = current[i]->components[j].empty();
   if (!have_tool[j]) {
	for (const component& tool : current[i]->tools[j]) {
     const itype_id type = tool.type;
     const int req = tool.count;	// -1 => 1
	 if (req <= 0 ? crafting_inv.has_amount(type, 1) : crafting_inv.has_charges(type, req)) {
      have_tool[j] = true;
	  break;
     }
    }
   }
   if (!have_comp[j]) {
	for(const component& comp : current[i]->components[j]) {
     const itype_id type = comp.type;
     const int count = comp.count;
     if (item::types[type]->count_by_charges() && count > 0) {
      if (crafting_inv.has_charges(type, count)) {
       have_comp[j] = true;
	   break;
      }
     } else if (crafting_inv.has_amount(type, abs(count))) {
      have_comp[j] = true;
	  break;
	 }
    }
   }
  }
  if (have_tool[0] && have_tool[1] && have_tool[2] && have_tool[3] &&
      have_tool[4] && have_comp[0] && have_comp[1] && have_comp[2] &&
      have_comp[3] && have_comp[4])
   available[i] = true;
 }
}

void game::make_craft(const recipe* const making)
{
 u.assign_activity(ACT_CRAFT, making->time, making->id);
 u.moves = 0;
}

void game::complete_craft()
{
 const recipe* const making = recipe::recipes[u.activity.index]; // Which recipe is it?

// # of dice is 75% primary skill, 25% secondary (unless secondary is null)
 int skill_dice = u.sklevel[making->sk_primary] * 3;
 skill_dice += u.sklevel[sk_null == making->sk_secondary ? making->sk_primary : making->sk_secondary];
// Sides on dice is 16 plus your current intelligence
 int skill_sides = 16 + u.int_cur;

 int diff_dice = making->difficulty * 4; // Since skill level is * 4 also
 int diff_sides = 24;	// 16 + 8 (default intelligence)

 int skill_roll = dice(skill_dice, skill_sides);
 int diff_roll  = dice(diff_dice,  diff_sides);

 if (making->sk_primary != sk_null) u.practice(making->sk_primary, making->difficulty * 5 + 20);
 if (making->sk_secondary != sk_null) u.practice(making->sk_secondary, 5);

// Messed up badly; waste some components.
 if (making->difficulty != 0 && diff_roll > skill_roll * (1 + 0.1 * rng(1, 5))) {
  messages.add("You fail to make the %s, and waste some materials.", item::types[making->result]->name.c_str());
  for (const auto& comp : making->components) if (!comp.empty()) {
    std::vector<component> wasted(comp);
	for (auto& comp : wasted) comp.count = rng(0, comp.count);
    consume_items(m, u, wasted);
  }
  for(const auto& tools : making->tools) if (!tools.empty()) consume_tools(m, u, tools);
  u.activity.type = ACT_NULL;
  return;
  // Messed up slightly; no components wasted.
 } else if (diff_roll > skill_roll) {
  messages.add("You fail to make the %s, but don't waste any materials.", item::types[making->result]->name.c_str());
  u.activity.type = ACT_NULL;
  return;
 }
// If we're here, the craft was a success!
// Use up the components and tools
 for (const auto& comp : making->components) if (!comp.empty()) consume_items(m, u, comp);
 for (const auto& tools : making->tools) if (!tools.empty()) consume_tools(m, u, tools);

  // Set up the new item, and pick an inventory letter
 item newit(item::types[making->result], messages.turn, nextinv);
 if (!newit.craft_has_charges()) newit.charges = 0;
 const bool inv_ok = assign_invlet(newit, u);
 //newit = newit.in_its_container();
 if (newit.made_of(LIQUID))
  handle_liquid(newit, false, false);
 else {
// We might not have space for the item
  if (!inv_ok || u.volume_carried()+newit.volume() > u.volume_capacity()) {
   messages.add("There's no room in your inventory for the %s, so you drop it.", newit.tname().c_str());
   m.add_item(u.pos, std::move(newit));
  } else if (u.weight_carried() + newit.volume() > u.weight_capacity()) {
   messages.add("The %s is too heavy to carry, so you drop it.", newit.tname().c_str());
   m.add_item(u.pos, std::move(newit));
  } else {
   u.i_add(newit);
   messages.add("%c - %s", newit.invlet, newit.tname().c_str());
  }
 }
}

void consume_items(map& m, player& u, const std::vector<component>& components)
{
// For each set of components in the recipe, fill you_have with the list of all
// matching ingredients the player has.
 std::vector<component> player_has;
 std::vector<component> map_has;
 std::vector<component> mixed;
 std::vector<component> player_use;
 std::vector<component> map_use;
 std::vector<component> mixed_use;
 inventory map_inv;
 map_inv.form_from_map(m, u.pos, PICKUP_RANGE);

 for(const component& comp : components) {
  const itype_id type = comp.type;
  const int count = abs(comp.count);
  bool pl = false, mp = false;

  if (item::types[type]->count_by_charges() && count > 0) {
   if (u.has_charges(type, count)) {
    player_has.push_back(comp);
    pl = true;
   }
   if (map_inv.has_charges(type, count)) {
    map_has.push_back(comp);
    mp = true;
   }
   if (!pl && !mp && u.charges_of(type) + map_inv.charges_of(type) >= count)
    mixed.push_back(comp);
  } else { // Counting by units, not charges
   if (u.has_amount(type, count)) {
    player_has.push_back(comp);
    pl = true;
   }
   if (map_inv.has_amount(type, count)) {
    map_has.push_back(comp);
    mp = true;
   }
   if (!pl && !mp && u.amount_of(type) + map_inv.amount_of(type) >= count)
    mixed.push_back(comp);
  }
 }

 if (player_has.size() + map_has.size() + mixed.size() == 1) { // Only 1 choice
  if (player_has.size() == 1)
   player_use.push_back(player_has[0]);
  else if (map_has.size() == 1)
   map_use.push_back(map_has[0]);
  else
   mixed_use.push_back(mixed[0]);

 } else { // Let the player pick which component they want to use
  std::vector<std::string> options; // List for the menu_vec below
// Populate options with the names of the items
  for(const component& comp : map_has)
   options.push_back(item::types[comp.type]->name + " (nearby)");
  for(const component& comp : player_has)
   options.push_back(item::types[comp.type]->name);
  for(const component& comp : mixed)
   options.push_back(item::types[comp.type]->name + " (on person & nearby)");

  if (options.size() == 0) // This SHOULD only happen if cooking with a fire,
   return;                 // and the fire goes out.

// Get the selection via a menu popup
  int selection = menu_vec("Use which component?", options) - 1;
  if (selection < map_has.size())
   map_use.push_back(map_has[selection]);
  else if (selection < map_has.size() + player_has.size()) {
   selection -= map_has.size();
   player_use.push_back(player_has[selection]);
  } else {
   selection -= map_has.size() + player_has.size();
   mixed_use.push_back(mixed[selection]);
  }
 }

 for(const component& comp : player_use) {
  if (item::types[comp.type]->count_by_charges() && 0 < comp.count)
   u.use_charges(comp.type, comp.count);
  else
   u.use_amount(comp.type, abs(comp.count), (comp.count < 0));
 }
 for(const component& comp : map_use) {
  if (item::types[comp.type]->count_by_charges() && 0 < comp.count)
   m.use_charges(u.pos, PICKUP_RANGE, comp.type, comp.count);
  else
   m.use_amount(u.pos, PICKUP_RANGE, comp.type, abs(comp.count), (comp.count < 0));
 }
 for(const component& comp : mixed_use) {
  if (item::types[comp.type]->count_by_charges() && 0 < comp.count) {
   const int from_map = comp.count - u.charges_of(comp.type);
   u.use_charges(comp.type, u.charges_of(comp.type));
   m.use_charges(u.pos, PICKUP_RANGE, comp.type, from_map);
  } else {
   const bool in_container = (comp.count < 0);
   const int from_map = abs(comp.count) - u.amount_of(comp.type);
   u.use_amount(comp.type, u.amount_of(comp.type), in_container);
   m.use_amount(u.pos, PICKUP_RANGE, comp.type, from_map, in_container);
  }
 }
}

void consume_tools(map& m, player& u, const std::vector<component>& tools)
{
 bool found_nocharge = false;
 inventory map_inv;
 map_inv.form_from_map(m, u.pos, PICKUP_RANGE);
 std::vector<component> player_has;
 std::vector<component> map_has;
// Use charges of any tools that require charges used
 for (int i = 0; i < tools.size() && !found_nocharge; i++) {
  const itype_id type = tools[i].type;
  const int count = tools[i].count;
  if (count > 0) {
   if (u.has_charges(type, count))
    player_has.push_back(tools[i]);
   if (map_inv.has_charges(type, count))
    map_has.push_back(tools[i]);
  } else if (u.has_amount(type, 1) || map_inv.has_amount(type, 1))
   found_nocharge = true;
 }
 if (found_nocharge) return; // Default to using a tool that doesn't require charges

 if (player_has.size() == 1 && map_has.size() == 0) {
  const component& use = player_has[0];
  u.use_charges(use.type, use.count);
 } else if (map_has.size() == 1 && player_has.size() == 0) {
  const component& use = map_has[0];
  m.use_charges(u.pos, PICKUP_RANGE, use.type, use.count);
 } else { // Variety of options, list them and pick one
// Populate the list
  std::vector<std::string> options;
  for(const component& tool : map_has)
   options.push_back(item::types[tool.type]->name + " (nearby)");

  for(const component& tool : player_has)
   options.push_back(item::types[tool.type]->name);

  if (options.size() == 0) // This SHOULD only happen if cooking with a fire,
   return;                 // and the fire goes out.

// Get selection via a popup menu
  int selection = menu_vec("Use which tool?", options) - 1;
  if (selection < map_has.size()) {
   const component& use = map_has[selection];
   m.use_charges(u.pos, PICKUP_RANGE, use.type, use.count);
  } else {
   const component& use = player_has[selection - map_has.size()];
   u.use_charges(use.type, use.count);
  }
 }
}
