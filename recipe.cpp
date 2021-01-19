#include "crafting.h"

std::vector<recipe*> recipe::recipes;

// This function just defines the recipes used throughout the game.
void recipe::init()
{
 int id = -1;

/* A recipe will not appear in your menu until your level in the primary skill
 * is at least equal to the difficulty.  At that point, your chance of success
 * is still not great; a good 25% improvement over the difficulty is important
 */

// WEAPONS

 recipes.push_back(new recipe(++id, itm_spear_wood, CC_WEAPON, sk_null, sk_null, 0, 800));
  recipes.back()->tools.push_back({ { itm_hatchet, -1 }, { itm_knife_steak, -1 }, { itm_knife_butcher, -1 }, { itm_knife_combat, -1 }, { itm_machete, -1 }, { itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_stick, 1 }, { itm_broom, 1 },{ itm_mop, 1 },{ itm_2x4, 1 } });

 recipes.push_back(new recipe(++id, itm_spear_knife, CC_WEAPON, sk_stabbing, sk_null, 1, 600));
  recipes.back()->components.push_back({ { itm_stick, 1 },{ itm_broom, 1 },{ itm_mop, 1 } });
  recipes.back()->components.push_back({ { itm_knife_steak, 2 },{ itm_knife_combat, 1 } });
  recipes.back()->components.push_back({ { itm_string_36, 1 } });

 recipes.push_back(new recipe(++id, itm_longbow, CC_WEAPON, sk_archery, sk_survival, 2, 15000));
  recipes.back()->tools.push_back({ { itm_hatchet, -1 },{ itm_knife_steak, -1 },{ itm_knife_butcher, -1 }, { itm_knife_combat, -1 },{ itm_machete, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_stick, 1 } });
  recipes.back()->components.push_back({ { itm_string_36, 2 } });

 recipes.push_back(new recipe(++id, itm_arrow_wood, CC_WEAPON, sk_archery, sk_survival, 1, 5000));
  recipes.back()->tools.push_back({ { itm_hatchet, -1 },{ itm_knife_steak, -1 },{ itm_knife_butcher, -1 }, { itm_knife_combat, -1 },{ itm_machete, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_stick, 1 },{ itm_broom, 1 },{ itm_mop, 1 },{ itm_2x4, 1 },{ itm_bee_sting, 1 } });
                                 
 recipes.push_back(new recipe(++id, itm_nailboard, CC_WEAPON, sk_null, sk_null, 0, 1000));
  recipes.back()->tools.push_back({ { itm_hatchet, -1 },{ itm_hammer, -1 },{ itm_rock, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_2x4, 1 },{ itm_bat, 1 } });
  recipes.back()->components.push_back({ { itm_nail, 6 } });

 recipes.push_back(new recipe(++id, itm_molotov, CC_WEAPON, sk_null, sk_null, 0, 500));
  recipes.back()->components.push_back({ { itm_rag, 1 } });
  recipes.back()->components.push_back({ { itm_whiskey, -1 },{ itm_vodka, -1 },{ itm_rum, -1 },{ itm_tequila, -1 }, { itm_gasoline, -1 } });

 recipes.push_back(new recipe(++id, itm_pipebomb, CC_WEAPON, sk_mechanics, sk_null, 1, 750));
  recipes.back()->tools.push_back({ { itm_hacksaw, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_pipe, 1 } });
  recipes.back()->components.push_back({ { itm_gasoline, 1 },{ itm_shot_bird, 6 },{ itm_shot_00, 2 },{ itm_shot_slug, 2 } });
  recipes.back()->components.push_back({ { itm_string_36, 1 },{ itm_string_6, 1 } });

 recipes.push_back(new recipe(++id, itm_shotgun_sawn, CC_WEAPON, sk_gun, sk_null, 1, 500));
  recipes.back()->tools.push_back({ { itm_hacksaw, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_shotgun_d, 1 },{ itm_remington_870, 1 },{ itm_mossberg_500, 1 }, { itm_saiga_12, 1 } });

 recipes.push_back(new recipe(++id, itm_bolt_wood, CC_WEAPON, sk_mechanics, sk_archery, 1, 5000));
  recipes.back()->tools.push_back({ { itm_hatchet, -1 },{ itm_knife_steak, -1 },{ itm_knife_butcher, -1 }, { itm_knife_combat, -1 },{ itm_machete, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_stick, 1 },{ itm_broom, 1 },{ itm_mop, 1 },{ itm_2x4, 1 },{ itm_bee_sting, 1 } });

 recipes.push_back(new recipe(++id, itm_crossbow, CC_WEAPON, sk_mechanics, sk_archery, 3, 15000));
  recipes.back()->tools.push_back({ { itm_wrench, -1 } });
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_2x4, 1 },{ itm_stick, 4 } });
  recipes.back()->components.push_back({ { itm_hose, 1 } });

 recipes.push_back(new recipe(++id, itm_rifle_22, CC_WEAPON, sk_mechanics, sk_gun, 3, 12000));
  recipes.back()->tools.push_back({ { itm_hacksaw, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_pipe, 1 } });
  recipes.back()->components.push_back({ { itm_2x4, 1 } });

 recipes.push_back(new recipe(++id, itm_rifle_9mm, CC_WEAPON, sk_mechanics, sk_gun, 3, 14000));
  recipes.back()->tools.push_back({ { itm_hacksaw, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_pipe, 1 } });
  recipes.back()->components.push_back({ { itm_2x4, 1 } });

 recipes.push_back(new recipe(++id, itm_smg_9mm, CC_WEAPON, sk_mechanics, sk_gun, 5, 18000));
  recipes.back()->tools.push_back({ { itm_hacksaw, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_hammer, -1 },{ itm_rock, -1 },{ itm_hatchet, -1 } });
  recipes.back()->components.push_back({ { itm_pipe, 1 } });
  recipes.back()->components.push_back({ { itm_2x4, 2 } });
  recipes.back()->components.push_back({ { itm_nail, 4 } });

 recipes.push_back(new recipe(++id, itm_smg_45, CC_WEAPON, sk_mechanics, sk_gun, 5, 20000));
  recipes.back()->tools.push_back({ { itm_hacksaw, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_hammer, -1 },{ itm_rock, -1 },{ itm_hatchet, -1 } });
  recipes.back()->components.push_back({ { itm_pipe, 1 } });
  recipes.back()->components.push_back({ { itm_2x4, 2 } });
  recipes.back()->components.push_back({ { itm_nail, 4 } });

 recipes.push_back(new recipe(++id, itm_flamethrower_simple, CC_WEAPON, sk_mechanics, sk_gun, 6, 12000));
  recipes.back()->tools.push_back({ { itm_hacksaw, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_pipe, 1 } });
  recipes.back()->components.push_back({ { itm_hose, 2 } });
  recipes.back()->components.push_back({ { itm_bottle_glass, 4 },{ itm_bottle_plastic, 6 } });

 recipes.push_back(new recipe(++id, itm_launcher_simple, CC_WEAPON, sk_mechanics, sk_launcher, 6, 6000));
  recipes.back()->tools.push_back({ { itm_hacksaw, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_pipe, 1 } });
  recipes.back()->components.push_back({ { itm_2x4, 1 } });
  recipes.back()->components.push_back({ { itm_nail, 1 } });

 recipes.push_back(new recipe(++id, itm_shot_he, CC_WEAPON, sk_mechanics, sk_gun, 4, 2000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_superglue, 1 } });
  recipes.back()->components.push_back({ { itm_shot_slug, 4 } });
  recipes.back()->components.push_back({ { itm_gasoline, 1 } });

 recipes.push_back(new recipe(++id, itm_grenade, CC_WEAPON, sk_mechanics, sk_null, 2, 5000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_superglue, 1 },{ itm_string_36, 1 } });
  recipes.back()->components.push_back({ { itm_can_food, 1 },{ itm_can_drink, 1 },{ itm_canister_empty, 1 } });
  recipes.back()->components.push_back({ { itm_nail, 30 },{ itm_bb, 100 } });
  recipes.back()->components.push_back({ { itm_shot_bird, 6 },{ itm_shot_00, 3 },{ itm_shot_slug, 2 }, { itm_gasoline, 1 } });

 recipes.push_back(new recipe(++id, itm_chainsaw_off, CC_WEAPON, sk_mechanics, sk_null, 4, 20000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_hammer, -1 },{ itm_hatchet, -1 } });
  recipes.back()->tools.push_back({ { itm_wrench, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_motor, 1 } });
  recipes.back()->components.push_back({ { itm_chain, 1 } });

 recipes.push_back(new recipe(++id, itm_smokebomb, CC_WEAPON, sk_cooking, sk_mechanics, 3, 7500));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_wrench, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_water, 1 },{ itm_salt_water, 1 } });
  recipes.back()->components.push_back({ { itm_candy, 1 },{ itm_cola, 1 } });
  recipes.back()->components.push_back({ { itm_vitamins, 10 },{ itm_aspirin, 8 } });
  recipes.back()->components.push_back({ { itm_canister_empty, 1 },{ itm_can_food, 1 } });
  recipes.back()->components.push_back({ { itm_superglue, 1 } });

 recipes.push_back(new recipe(++id, itm_gasbomb, CC_WEAPON, sk_cooking, sk_mechanics, 4, 8000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_wrench, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_bleach, 2 } });
  recipes.back()->components.push_back({ { itm_ammonia, 2 } });
  recipes.back()->components.push_back({ { itm_canister_empty, 1 },{ itm_can_food, 1 } });
  recipes.back()->components.push_back({ { itm_superglue, 1 } });

 recipes.push_back(new recipe(++id, itm_nx17, CC_WEAPON, sk_electronics, sk_mechanics, 8, 40000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_soldering_iron, 6 },{ itm_toolset, 6 } });
  recipes.back()->components.push_back({ { itm_vacutainer, 1 } });
  recipes.back()->components.push_back({ { itm_power_supply, 8 } });
  recipes.back()->components.push_back({ { itm_amplifier, 8 } });

 recipes.push_back(new recipe(++id, itm_mininuke, CC_WEAPON, sk_mechanics, sk_electronics, 10, 40000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_wrench, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_can_food, 2 },{ itm_steel_chunk, 2 },{ itm_canister_empty, 1 } });
  recipes.back()->components.push_back({ { itm_plut_cell, 6 } });
  recipes.back()->components.push_back({ { itm_battery, 2 } });
  recipes.back()->components.push_back({ { itm_power_supply, 1 } });

/*
 * We need a some Chemicals which arn't implemented to realistically craft this!
recipes.push_back(new recipe(++id, itm_c4, CC_WEAPON, sk_mechanics, sk_electronics, 4, 8000));
 recipes.back()->tools.push_back({ { itm_screwdriver, -1 } });
 recipes.back()->components.push_back({ { itm_can_food, 1 },{ itm_steel_chunk, 1 },{ itm_canister_empty, 1 } });
 recipes.back()->components.push_back({ { itm_battery, 1 } });
 recipes.back()->components.push_back({ { itm_superglue,1 } });
 recipes.back()->components.push_back({ { itm_soldering_iron,1 } });
 recipes.back()->components.push_back({ { itm_power_supply, 1 } });
*/

// FOOD

 recipes.push_back(new recipe(++id, itm_meat_cooked, CC_FOOD, sk_cooking, sk_null, 0, 5000));
  recipes.back()->tools.push_back({ { itm_hotplate, 7 },{ itm_toolset, 4 },{ itm_fire, -1 } });
  recipes.back()->tools.push_back({ { itm_pan, -1 },{ itm_pot, -1 } });
  recipes.back()->components.push_back({ { itm_meat, 1 } });

 recipes.push_back(new recipe(++id, itm_dogfood, CC_FOOD, sk_cooking, sk_null, 4, 10000));
  recipes.back()->tools.push_back({ { itm_hotplate, 6 },{ itm_toolset, 3 },{ itm_fire, -1 } });
  recipes.back()->tools.push_back({ { itm_pot, -1 } });
  recipes.back()->components.push_back({ { itm_meat, 1 } });
  recipes.back()->components.push_back({ { itm_veggy,1 } });
  recipes.back()->components.push_back({ { itm_water,1 } });

 recipes.push_back(new recipe(++id, itm_veggy_cooked, CC_FOOD, sk_cooking, sk_null, 0, 4000));
  recipes.back()->tools.push_back({ { itm_hotplate, 5 },{ itm_toolset, 3 },{ itm_fire, -1 } });
  recipes.back()->tools.push_back({ { itm_pan, -1 },{ itm_pot, -1 } });
  recipes.back()->components.push_back({ { itm_veggy, 1} });

 recipes.push_back(new recipe(++id, itm_spaghetti_cooked, CC_FOOD, sk_cooking, sk_null, 0, 10000));
  recipes.back()->tools.push_back({ { itm_hotplate, 4 },{ itm_toolset, 2 },{ itm_fire, -1} });
  recipes.back()->tools.push_back({ { itm_pot, -1} });
  recipes.back()->components.push_back({ { itm_spaghetti_raw, 1} });
  recipes.back()->components.push_back({ { itm_water, 1} });

 recipes.push_back(new recipe(++id, itm_cooked_dinner, CC_FOOD, sk_cooking, sk_null, 0, 5000));
  recipes.back()->tools.push_back({ { itm_hotplate, 3 },{ itm_toolset, 2 },{ itm_fire, -1} });
  recipes.back()->components.push_back({ { itm_frozen_dinner, 1} });

 recipes.push_back(new recipe(++id, itm_macaroni_cooked, CC_FOOD, sk_cooking, sk_null, 1, 10000));
  recipes.back()->tools.push_back({ { itm_hotplate, 4 },{ itm_toolset, 2 },{ itm_fire, -1} });
  recipes.back()->tools.push_back({ { itm_pot, -1} });
  recipes.back()->components.push_back({ { itm_macaroni_raw, 1} });
  recipes.back()->components.push_back({ { itm_water, 1} });

 recipes.push_back(new recipe(++id, itm_potato_baked, CC_FOOD, sk_cooking, sk_null, 1, 15000));
  recipes.back()->tools.push_back({ { itm_hotplate, 3 },{ itm_toolset, 2 },{ itm_fire, -1} });
  recipes.back()->tools.push_back({ { itm_pan, -1 },{ itm_pot, -1} });
  recipes.back()->components.push_back({ { itm_potato_raw, 1} });

 recipes.push_back(new recipe(++id, itm_tea, CC_FOOD, sk_cooking, sk_null, 0, 4000));
  recipes.back()->tools.push_back({ { itm_hotplate, 2 },{ itm_toolset, 1 },{ itm_fire, -1} });
  recipes.back()->tools.push_back({ { itm_pot, -1} });
  recipes.back()->components.push_back({ { itm_tea_raw, 1} });
  recipes.back()->components.push_back({ { itm_water, 1} });

 recipes.push_back(new recipe(++id, itm_coffee, CC_FOOD, sk_cooking, sk_null, 0, 4000));
  recipes.back()->tools.push_back({ { itm_hotplate, 2 },{ itm_toolset, 1 },{ itm_fire, -1} });
  recipes.back()->tools.push_back({ { itm_pot, -1} });
  recipes.back()->components.push_back({ { itm_coffee_raw, 1} });
  recipes.back()->components.push_back({ { itm_water, 1} });

 recipes.push_back(new recipe(++id, itm_oj, CC_FOOD, sk_cooking, sk_null, 1, 5000));
  recipes.back()->tools.push_back({ { itm_rock, -1 },{ itm_toolset, -1} });
  recipes.back()->components.push_back({ { itm_orange, 2} });
  recipes.back()->components.push_back({ { itm_water, 1} });

 recipes.push_back(new recipe(++id, itm_apple_cider, CC_FOOD, sk_cooking, sk_null, 2, 7000));
  recipes.back()->tools.push_back({ { itm_rock, -1 }, { itm_toolset, -1} });
  recipes.back()->components.push_back({ { itm_apple, 3} });
 
 recipes.push_back(new recipe(++id, itm_jerky, CC_FOOD, sk_cooking, sk_null, 3, 30000));
  recipes.back()->tools.push_back({ { itm_hotplate, 10 },{ itm_toolset, 5 },{ itm_fire, -1} });
  recipes.back()->components.push_back({ { itm_salt_water, 1 },{ itm_salt, 4} });
  recipes.back()->components.push_back({ { itm_meat, 1} });

 recipes.push_back(new recipe(++id, itm_V8, CC_FOOD, sk_cooking, sk_null, 2, 5000));
  recipes.back()->components.push_back({ { itm_tomato, 1} });
  recipes.back()->components.push_back({ { itm_broccoli, 1} });
  recipes.back()->components.push_back({ { itm_zucchini, 1} });

 recipes.push_back(new recipe(++id, itm_broth, CC_FOOD, sk_cooking, sk_null, 2, 10000));
  recipes.back()->tools.push_back({ { itm_hotplate, 5 },{ itm_toolset, 3 },{ itm_fire, -1} });
  recipes.back()->tools.push_back({ { itm_pot, -1} });
  recipes.back()->components.push_back({ { itm_water, 1} });
  recipes.back()->components.push_back({ { itm_broccoli, 1 },{ itm_zucchini, 1 },{ itm_veggy, 1} });

 recipes.push_back(new recipe(++id, itm_soup, CC_FOOD, sk_cooking, sk_null, 2, 10000));
  recipes.back()->tools.push_back({ { itm_hotplate, 5 },{ itm_toolset, 3 },{ itm_fire, -1} });
  recipes.back()->tools.push_back({ { itm_pot, -1} });
  recipes.back()->components.push_back({ { itm_broth, 2} });
  recipes.back()->components.push_back({ { itm_macaroni_raw, 1 },{ itm_potato_raw, 1} });
  recipes.back()->components.push_back({ { itm_tomato, 2 },{ itm_broccoli, 2 },{ itm_zucchini, 2 },{ itm_veggy, 2} });

 recipes.push_back(new recipe(++id, itm_bread, CC_FOOD, sk_cooking, sk_null, 4, 20000));
  recipes.back()->tools.push_back({ { itm_hotplate, 8 },{ itm_toolset, 4 },{ itm_fire, -1} });
  recipes.back()->tools.push_back({ { itm_pot, -1} });
  recipes.back()->components.push_back({ { itm_flour, 3} });
  recipes.back()->components.push_back({ { itm_water, 2} });

 recipes.push_back(new recipe(++id, itm_pie, CC_FOOD, sk_cooking, sk_null, 3, 25000));
  recipes.back()->tools.push_back({ { itm_hotplate, 6 },{ itm_toolset, 3 },{ itm_fire, -1} });
  recipes.back()->tools.push_back({ { itm_pan, -1} });
  recipes.back()->components.push_back({ { itm_flour, 2} });
  recipes.back()->components.push_back({ { itm_strawberries, 2 },{ itm_apple, 2 },{ itm_blueberries, 2} });
  recipes.back()->components.push_back({ { itm_sugar, 2} });
  recipes.back()->components.push_back({ { itm_water, 1} });

 recipes.push_back(new recipe(++id, itm_pizza, CC_FOOD, sk_cooking, sk_null, 3, 20000));
  recipes.back()->tools.push_back({ { itm_hotplate, 8 },{ itm_toolset, 4 },{ itm_fire, -1} });
  recipes.back()->tools.push_back({ { itm_pan, -1} });
  recipes.back()->components.push_back({ { itm_flour, 2} });
  recipes.back()->components.push_back({ { itm_veggy, 1 },{ itm_tomato, 2 },{ itm_broccoli, 1} });
  recipes.back()->components.push_back({ { itm_sauce_pesto, 1 },{ itm_sauce_red, 1} });
  recipes.back()->components.push_back({ { itm_water, 1} });

 recipes.push_back(new recipe(++id, itm_meth, CC_FOOD, sk_cooking, sk_null, 5, 20000));
  recipes.back()->tools.push_back({ { itm_hotplate, 15 },{ itm_toolset, 8 },{ itm_fire, -1} });
  recipes.back()->tools.push_back({ { itm_bottle_glass, -1 },{ itm_hose, -1} });
  recipes.back()->components.push_back({ { itm_dayquil, 2 },{ itm_royal_jelly, 1} });
  recipes.back()->components.push_back({ { itm_aspirin, 40} });
  recipes.back()->components.push_back({ { itm_caffeine, 20 },{ itm_adderall, 5 },{ itm_energy_drink, 2} });

 recipes.push_back(new recipe(++id, itm_royal_jelly, CC_FOOD, sk_cooking, sk_null, 5, 5000));
  recipes.back()->components.push_back({ { itm_honeycomb, 1} });
  recipes.back()->components.push_back({ { itm_bleach, 2 },{ itm_purifier, 1} });

 recipes.push_back(new recipe(++id, itm_heroin, CC_FOOD, sk_cooking, sk_null, 6, 2000));
  recipes.back()->tools.push_back({ { itm_hotplate, 3 },{ itm_toolset, 2 },{ itm_fire, -1} });
  recipes.back()->tools.push_back({ { itm_pan, -1 },{ itm_pot, -1} });
  recipes.back()->components.push_back({ { itm_salt_water, 1 },{ itm_salt, 4} });
  recipes.back()->components.push_back({ { itm_oxycodone, 40} });

 recipes.push_back(new recipe(++id, itm_mutagen, CC_FOOD, sk_cooking, sk_firstaid, 8, 10000));
  recipes.back()->tools.push_back({ { itm_hotplate, 25 },{ itm_toolset, 12 },{ itm_fire, -1} });
  recipes.back()->components.push_back({ { itm_meat_tainted, 3 },{ itm_veggy_tainted, 5 },{ itm_fetus, 1 },{ itm_arm, 2 }, { itm_leg, 2 } });
  recipes.back()->components.push_back({ { itm_bleach, 2} });
  recipes.back()->components.push_back({ { itm_ammonia, 1} });

 recipes.push_back(new recipe(++id, itm_purifier, CC_FOOD, sk_cooking, sk_firstaid, 9, 10000));
  recipes.back()->tools.push_back({ { itm_hotplate, 25 },{ itm_toolset, 12 },{ itm_fire, -1} });
  recipes.back()->components.push_back({ { itm_royal_jelly, 4 },{ itm_mutagen, 2} });
  recipes.back()->components.push_back({ { itm_bleach, 3} });
  recipes.back()->components.push_back({ { itm_ammonia, 2} });

// ELECTRONICS

 recipes.push_back(new recipe(++id, itm_antenna, CC_ELECTRONIC, sk_null, sk_null, 0, 3000));
  recipes.back()->tools.push_back({ { itm_hacksaw, -1 },{ itm_toolset, -1} });
  recipes.back()->components.push_back({ { itm_radio, 1 },{ itm_two_way_radio, 1 },{ itm_motor, 1 },{ itm_knife_butter, 2} });

 recipes.push_back(new recipe(++id, itm_amplifier, CC_ELECTRONIC, sk_electronics, sk_null, 1, 4000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1} });
  recipes.back()->components.push_back({ { itm_flashlight, 1 },{ itm_radio, 1 },{ itm_two_way_radio, 1 },{ itm_geiger_off, 1 }, { itm_goggles_nv, 1 },{ itm_transponder, 2 } });

 recipes.push_back(new recipe(++id, itm_power_supply, CC_ELECTRONIC, sk_electronics, sk_null, 1, 6500));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_soldering_iron, 3 },{ itm_toolset, 3 } });
  recipes.back()->components.push_back({ { itm_amplifier, 2 },{ itm_soldering_iron, 1 },{ itm_electrohack, 1 }, { itm_battery, 800 },{ itm_geiger_off, 1 } });

 recipes.push_back(new recipe(++id, itm_receiver, CC_ELECTRONIC, sk_electronics, sk_null, 2, 12000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_soldering_iron, 4 },{ itm_toolset, 4 } });
  recipes.back()->components.push_back({ { itm_amplifier, 2 },{ itm_radio, 1 },{ itm_two_way_radio, 1 } });

 recipes.push_back(new recipe(++id, itm_transponder, CC_ELECTRONIC, sk_electronics, sk_null, 2, 14000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_soldering_iron, 7 },{ itm_toolset, 7 } });
  recipes.back()->components.push_back({ { itm_receiver, 3 },{ itm_two_way_radio, 1 } });

 recipes.push_back(new recipe(++id, itm_flashlight, CC_ELECTRONIC, sk_electronics, sk_null, 1, 10000));
  recipes.back()->components.push_back({ { itm_amplifier, 1 } });
  recipes.back()->components.push_back({ { itm_bottle_plastic, 1 },{ itm_bottle_glass, 1 },{ itm_can_drink, 1 } });

 recipes.push_back(new recipe(++id, itm_soldering_iron, CC_ELECTRONIC, sk_electronics, sk_null, 1, 20000));
  recipes.back()->components.push_back({ { itm_screwdriver, 1 },{ itm_antenna, 1 },{ itm_xacto, 1 },{ itm_knife_butter, 1 } });
  recipes.back()->components.push_back({ { itm_power_supply, 1 } });

 recipes.push_back(new recipe(++id, itm_battery, CC_ELECTRONIC, sk_electronics, sk_mechanics, 2, 5000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_ammonia, 1 },{ itm_lemon, 1 } });
  recipes.back()->components.push_back({ { itm_steel_chunk, 1 },{ itm_knife_butter, 1 },{ itm_knife_steak, 1 }, { itm_bolt_steel, 1 } });
  recipes.back()->components.push_back({ { itm_can_drink, 1 },{ itm_can_food, 1 } });

 recipes.push_back(new recipe(++id, itm_coilgun, CC_WEAPON, sk_electronics, sk_null, 3, 25000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_soldering_iron, 10 },{ itm_toolset, 10 } });
  recipes.back()->components.push_back({ { itm_pipe, 1 } });
  recipes.back()->components.push_back({ { itm_power_supply, 1 } });
  recipes.back()->components.push_back({ { itm_amplifier, 1 } });

 recipes.push_back(new recipe(++id, itm_radio, CC_ELECTRONIC, sk_electronics, sk_null, 2, 25000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_soldering_iron, 10 },{ itm_toolset, 10 } });
  recipes.back()->components.push_back({ { itm_receiver, 1 } });
  recipes.back()->components.push_back({ { itm_antenna, 1 } });

 recipes.push_back(new recipe(++id, itm_water_purifier, CC_ELECTRONIC, sk_mechanics,sk_electronics,3,25000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_hotplate, 2 } });
  recipes.back()->components.push_back({ { itm_bottle_glass, 2 },{ itm_bottle_plastic, 5 } });
  recipes.back()->components.push_back({ { itm_hose, 1 } });

 recipes.push_back(new recipe(++id, itm_hotplate, CC_ELECTRONIC, sk_electronics, sk_null, 3, 30000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_soldering_iron, 1 },{ itm_amplifier, 1 } });
  recipes.back()->components.push_back({ { itm_pan, 1 }, { itm_pot, 1 }, { itm_knife_butcher, 2 }, { itm_knife_steak, 6 }, { itm_knife_butter, 6 }, { itm_muffler, 1 } });

 recipes.push_back(new recipe(++id, itm_tazer, CC_ELECTRONIC, sk_electronics, sk_null, 3, 25000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_soldering_iron, 10 },{ itm_toolset, 10 } });
  recipes.back()->components.push_back({ { itm_amplifier, 1 } });
  recipes.back()->components.push_back({ { itm_power_supply, 1 } });

 recipes.push_back(new recipe(++id, itm_two_way_radio, CC_ELECTRONIC, sk_electronics, sk_null, 4, 30000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_soldering_iron, 14 },{ itm_toolset, 14 } });
  recipes.back()->components.push_back({ { itm_amplifier, 1 } });
  recipes.back()->components.push_back({ { itm_transponder, 1 } });
  recipes.back()->components.push_back({ { itm_receiver, 1 } });
  recipes.back()->components.push_back({ { itm_antenna, 1 } });

 recipes.push_back(new recipe(++id, itm_electrohack, CC_ELECTRONIC, sk_electronics, sk_computer, 4, 35000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_soldering_iron, 10 },{ itm_toolset, 10 } });
  recipes.back()->components.push_back({ { itm_processor, 1 } });
  recipes.back()->components.push_back({ { itm_RAM, 1 } });

 recipes.push_back(new recipe(++id, itm_EMPbomb, CC_ELECTRONIC, sk_electronics, sk_null, 4, 32000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_soldering_iron, 6 },{ itm_toolset, 6 } });
  recipes.back()->components.push_back({ { itm_superglue, 1 },{ itm_string_36, 1 } });
  recipes.back()->components.push_back({ { itm_can_food, 1 },{ itm_can_drink, 1 },{ itm_canister_empty, 1 } });
  recipes.back()->components.push_back({ { itm_power_supply, 1 },{ itm_amplifier, 1 } });

 recipes.push_back(new recipe(++id, itm_mp3, CC_ELECTRONIC, sk_electronics, sk_computer, 5, 40000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_soldering_iron, 5 },{ itm_toolset, 5 } });
  recipes.back()->components.push_back({ { itm_superglue, 1 } });
  recipes.back()->components.push_back({ { itm_antenna, 1 } });
  recipes.back()->components.push_back({ { itm_amplifier, 1 } });

 recipes.push_back(new recipe(++id, itm_geiger_off, CC_ELECTRONIC, sk_electronics, sk_null, 5, 35000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_soldering_iron, 14 },{ itm_toolset, 14 } });
  recipes.back()->components.push_back({ { itm_power_supply, 1 } });
  recipes.back()->components.push_back({ { itm_amplifier, 2 } });

 recipes.push_back(new recipe(++id, itm_UPS_off, CC_ELECTRONIC, sk_electronics, sk_null, 5, 45000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_soldering_iron, 24 },{ itm_toolset, 24 } });
  recipes.back()->components.push_back({ { itm_power_supply, 4 } });
  recipes.back()->components.push_back({ { itm_amplifier, 3 } });

 recipes.push_back(new recipe(++id, itm_bionics_battery, CC_ELECTRONIC, sk_electronics, sk_null, 6, 50000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_soldering_iron, 20 },{ itm_toolset, 20 } });
  recipes.back()->components.push_back({ { itm_UPS_off, 1 },{ itm_power_supply, 6 } });
  recipes.back()->components.push_back({ { itm_amplifier, 4 } });
  recipes.back()->components.push_back({ { itm_plut_cell, 1 } });

 recipes.push_back(new recipe(++id, itm_teleporter, CC_ELECTRONIC, sk_electronics, sk_null, 8, 50000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_wrench, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_soldering_iron, 16 },{ itm_toolset, 16 } });
  recipes.back()->components.push_back({ { itm_power_supply, 3 },{ itm_plut_cell, 5 } });
  recipes.back()->components.push_back({ { itm_amplifier, 3 } });
  recipes.back()->components.push_back({ { itm_transponder, 3 } });

// ARMOR

 recipes.push_back(new recipe(++id, itm_mocassins, CC_ARMOR, sk_tailor, sk_null, 1, 30000));
  recipes.back()->tools.push_back({ { itm_sewing_kit,  5 } });
  recipes.back()->components.push_back({ { itm_fur, 2 } });

 recipes.push_back(new recipe(++id, itm_boots, CC_ARMOR, sk_tailor, sk_null, 2, 35000));
  recipes.back()->tools.push_back({ { itm_sewing_kit, 10 } });
  recipes.back()->components.push_back({ { itm_leather, 2 } });

 recipes.push_back(new recipe(++id, itm_jeans, CC_ARMOR, sk_tailor, sk_null, 2, 45000));
  recipes.back()->tools.push_back({ { itm_sewing_kit, 10 } });
  recipes.back()->components.push_back({ { itm_rag, 6 } });

 recipes.push_back(new recipe(++id, itm_pants_cargo, CC_ARMOR, sk_tailor, sk_null, 3, 48000));
  recipes.back()->tools.push_back({ { itm_sewing_kit, 16 } });
  recipes.back()->components.push_back({ { itm_rag, 8 } });

 recipes.push_back(new recipe(++id, itm_pants_leather, CC_ARMOR, sk_tailor, sk_null, 4, 50000));
  recipes.back()->tools.push_back({ { itm_sewing_kit, 10 } });
  recipes.back()->components.push_back({ { itm_leather, 6 } });

 recipes.push_back(new recipe(++id, itm_tank_top, CC_ARMOR, sk_tailor, sk_null, 2, 38000));
  recipes.back()->tools.push_back({ { itm_sewing_kit, 4 } });
  recipes.back()->components.push_back({ { itm_rag, 4 } });

 recipes.push_back(new recipe(++id, itm_hoodie, CC_ARMOR, sk_tailor, sk_null, 3, 40000));
  recipes.back()->tools.push_back({ { itm_sewing_kit, 14 } });
  recipes.back()->components.push_back({ { itm_rag, 12 } });

 recipes.push_back(new recipe(++id, itm_trenchcoat, CC_ARMOR, sk_tailor, sk_null, 3, 42000));
  recipes.back()->tools.push_back({ { itm_sewing_kit, 24 } });
  recipes.back()->components.push_back({ { itm_rag, 11 } });

 recipes.push_back(new recipe(++id, itm_coat_fur, CC_ARMOR, sk_tailor, sk_null, 4, 100000));
  recipes.back()->tools.push_back({ { itm_sewing_kit, 20 } });
  recipes.back()->components.push_back({ { itm_fur, 10 } });

 recipes.push_back(new recipe(++id, itm_jacket_leather, CC_ARMOR, sk_tailor, sk_null, 5, 150000));
  recipes.back()->tools.push_back({ { itm_sewing_kit, 30 } });
  recipes.back()->components.push_back({ { itm_leather, 8 } });

 recipes.push_back(new recipe(++id, itm_gloves_light, CC_ARMOR, sk_tailor, sk_null, 1, 10000));
  recipes.back()->tools.push_back({ { itm_sewing_kit, 2 } });
  recipes.back()->components.push_back({ { itm_rag, 2 } });

 recipes.push_back(new recipe(++id, itm_gloves_fingerless, CC_ARMOR, sk_tailor, sk_null, 3, 16000));
  recipes.back()->tools.push_back({ { itm_sewing_kit, 6 } });
  recipes.back()->components.push_back({ { itm_leather, 2 } });

 recipes.push_back(new recipe(++id, itm_mask_filter, CC_ARMOR, sk_mechanics, sk_tailor, 1, 5000));
  recipes.back()->components.push_back({ { itm_bottle_plastic, 1 },{ itm_bag_plastic, 2 } });
  recipes.back()->components.push_back({ { itm_muffler, 1 },{ itm_bandana, 2 },{ itm_rag, 2 },{ itm_wrapper, 4 } });

 recipes.push_back(new recipe(++id, itm_mask_gas, CC_ARMOR, sk_tailor, sk_null, 3, 20000));
  recipes.back()->tools.push_back({ { itm_wrench, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_goggles_swim, 2 },{ itm_goggles_ski, 1 } });
  recipes.back()->components.push_back({ { itm_mask_filter, 3 },{ itm_muffler, 1 } });
  recipes.back()->components.push_back({ { itm_hose, 1 } });

 recipes.push_back(new recipe(++id, itm_glasses_safety, CC_ARMOR, sk_tailor, sk_null, 1, 8000));
  recipes.back()->tools.push_back({ { itm_scissors, -1 },{ itm_xacto, -1 },{ itm_knife_steak, -1 },
	     { itm_knife_combat, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_string_36, 1 },{ itm_string_6, 2 } });
  recipes.back()->components.push_back({ { itm_bottle_plastic, 1 } });

 recipes.push_back(new recipe(++id, itm_goggles_nv, CC_ARMOR, sk_electronics, sk_tailor, 5, 40000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_goggles_ski, 1 },{ itm_goggles_welding, 1 },{ itm_mask_gas, 1 } });
  recipes.back()->components.push_back({ { itm_power_supply, 1 } });
  recipes.back()->components.push_back({ { itm_amplifier, 3 } });

 recipes.push_back(new recipe(++id, itm_hat_fur, CC_ARMOR, sk_tailor, sk_null, 2, 40000));
  recipes.back()->tools.push_back({ { itm_sewing_kit, 8 } });
  recipes.back()->components.push_back({ { itm_fur, 3 } });

 recipes.push_back(new recipe(++id, itm_helmet_chitin, CC_ARMOR, sk_tailor, sk_null, 6,  60000));
  recipes.back()->components.push_back({ { itm_string_36, 1 },{ itm_string_6, 5 } });
  recipes.back()->components.push_back({ { itm_chitin_piece, 5 } });

 recipes.push_back(new recipe(++id, itm_armor_chitin, CC_ARMOR, sk_tailor, sk_null,  7, 100000));
  recipes.back()->components.push_back({ { itm_string_36, 2 },{ itm_string_6, 12 } });
  recipes.back()->components.push_back({ { itm_chitin_piece, 15 } });

 recipes.push_back(new recipe(++id, itm_backpack, CC_ARMOR, sk_tailor, sk_null, 3, 50000));
  recipes.back()->tools.push_back({ { itm_sewing_kit, 20 } });
  recipes.back()->components.push_back({ { itm_rag, 20 },{ itm_fur, 16 },{ itm_leather, 12 } });

// MISC

 recipes.push_back(new recipe(++id, itm_superglue, CC_MISC, sk_cooking, sk_null, 2, 12000));
  recipes.back()->tools.push_back({ { itm_hotplate, 5 },{ itm_toolset, 3 },{ itm_fire, -1 } });
  recipes.back()->components.push_back({ { itm_water, 1 } });
  recipes.back()->components.push_back({ { itm_bleach, 1 },{ itm_ant_egg, 1 } });

 recipes.push_back(new recipe(++id, itm_2x4, CC_MISC, sk_null, sk_null, 0, 8000));
  recipes.back()->tools.push_back({ { itm_saw, -1 } });
  recipes.back()->components.push_back({ { itm_stick, 1 } });

 recipes.push_back(new recipe(++id, itm_frame, CC_MISC, sk_mechanics, sk_null, 1, 8000));
  recipes.back()->tools.push_back({ { itm_welder, 50 } });
  recipes.back()->components.push_back({ { itm_steel_lump, 3 } });

 recipes.push_back(new recipe(++id, itm_steel_plate, CC_MISC, sk_mechanics, sk_null,4, 12000));
  recipes.back()->tools.push_back({ { itm_welder, 100 } });
  recipes.back()->components.push_back({ { itm_steel_lump, 8 } });

 recipes.push_back(new recipe(++id, itm_spiked_plate, CC_MISC, sk_mechanics, sk_null, 4, 12000));
  recipes.back()->tools.push_back({ { itm_welder, 120 } });
  recipes.back()->components.push_back({ { itm_steel_lump, 8 } });
  recipes.back()->components.push_back({ { itm_steel_chunk, 4 } });

 recipes.push_back(new recipe(++id, itm_hard_plate, CC_MISC, sk_mechanics, sk_null, 4, 12000));
  recipes.back()->tools.push_back({ { itm_welder, 300 } });
  recipes.back()->components.push_back({ { itm_steel_lump, 24 } });

 recipes.push_back(new recipe(++id, itm_crowbar, CC_MISC, sk_mechanics, sk_null, 1, 1000));
  recipes.back()->tools.push_back({ { itm_hatchet, -1 },{ itm_hammer, -1 },{ itm_rock, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_pipe, 1 } });

 recipes.push_back(new recipe(++id, itm_bayonet, CC_MISC, sk_gun, sk_null, 1, 500));
  recipes.back()->components.push_back({ { itm_knife_steak, 3 },{ itm_knife_combat, 1 } });
  recipes.back()->components.push_back({ { itm_string_36, 1 } });

 recipes.push_back(new recipe(++id, itm_tripwire, CC_MISC, sk_traps, sk_null, 1, 500));
  recipes.back()->components.push_back({ { itm_string_36, 1 } });
  recipes.back()->components.push_back({ { itm_superglue, 1 } });

 recipes.push_back(new recipe(++id, itm_board_trap, CC_MISC, sk_traps, sk_null, 2, 2500));
  recipes.back()->tools.push_back({ { itm_hatchet, -1 },{ itm_hammer, -1 },{ itm_rock, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_2x4, 3 } });
  recipes.back()->components.push_back({ { itm_nail, 20 } });

 recipes.push_back(new recipe(++id, itm_beartrap, CC_MISC, sk_mechanics, sk_traps, 2, 3000));
  recipes.back()->tools.push_back({ { itm_wrench, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_steel_chunk, 2 } });

 recipes.push_back(new recipe(++id, itm_crossbow_trap, CC_MISC, sk_mechanics, sk_traps, 3, 4500));
  recipes.back()->components.push_back({ { itm_crossbow, 1 } });
  recipes.back()->components.push_back({ { itm_bolt_steel, 1 },{ itm_bolt_wood, 4 } });
  recipes.back()->components.push_back({ { itm_string_36, 1 },{ itm_string_6, 2 } });

 recipes.push_back(new recipe(++id, itm_shotgun_trap, CC_MISC, sk_mechanics, sk_traps, 3, 5000));
  recipes.back()->components.push_back({ { itm_shotgun_sawn, 1 } });
  recipes.back()->components.push_back({ { itm_shot_00, 2 } });
  recipes.back()->components.push_back({ { itm_string_36, 1 },{ itm_string_6, 2 } });

 recipes.push_back(new recipe(++id, itm_blade_trap, CC_MISC, sk_mechanics, sk_traps, 4, 8000));
  recipes.back()->tools.push_back({ { itm_wrench, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_motor, 1 } });
  recipes.back()->components.push_back({ { itm_machete, 1 } });
  recipes.back()->components.push_back({ { itm_string_36, 1 } });

recipes.push_back(new recipe(++id, itm_boobytrap, CC_MISC, sk_mechanics, sk_traps,3,5000));
  recipes.back()->components.push_back({ { itm_grenade,1 } });
  recipes.back()->components.push_back({ { itm_string_6,1 } });
  recipes.back()->components.push_back({ { itm_can_food,1 } });

 recipes.push_back(new recipe(++id, itm_landmine, CC_WEAPON, sk_traps, sk_mechanics, 5, 10000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_superglue, 1 } });
  recipes.back()->components.push_back({ { itm_can_food, 1 },{ itm_steel_chunk, 1 },{ itm_canister_empty, 1 } });
  recipes.back()->components.push_back({ { itm_nail, 100 },{ itm_bb, 200 } });
  recipes.back()->components.push_back({ { itm_shot_bird, 30 },{ itm_shot_00, 15 },{ itm_shot_slug, 12 },{ itm_gasoline, 3 },
	     { itm_grenade, 1 } });

 recipes.push_back(new recipe(++id, itm_bandages, CC_MISC, sk_firstaid, sk_null, 1, 500));
  recipes.back()->components.push_back({ { itm_rag, 1 } });
  recipes.back()->components.push_back({ { itm_superglue, 1 } });

 recipes.push_back(new recipe(++id, itm_silencer, CC_MISC, sk_mechanics, sk_null, 1, 650));
  recipes.back()->tools.push_back({ { itm_hacksaw, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_muffler, 1 },{ itm_rag, 4 } });
  recipes.back()->components.push_back({ { itm_pipe, 1 } });

 recipes.push_back(new recipe(++id, itm_pheromone, CC_MISC, sk_cooking, sk_null, 3, 1200));
  recipes.back()->tools.push_back({ { itm_hotplate, 18 },{ itm_toolset, 9 },{ itm_fire, -1 } });
  recipes.back()->components.push_back({ { itm_meat_tainted, 1 } });
  recipes.back()->components.push_back({ { itm_ammonia, 1 } });

 recipes.push_back(new recipe(++id, itm_laser_pack, CC_MISC, sk_electronics, sk_null, 5, 10000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->components.push_back({ { itm_superglue, 1 } });
  recipes.back()->components.push_back({ { itm_plut_cell, 1 } });

 recipes.push_back(new recipe(++id, itm_bot_manhack, CC_MISC, sk_electronics, sk_computer, 6, 8000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_soldering_iron, 10 },{ itm_toolset, 10 } });
  recipes.back()->components.push_back({ { itm_knife_steak, 4 },{ itm_knife_combat, 2 } });
  recipes.back()->components.push_back({ { itm_processor, 1 } });
  recipes.back()->components.push_back({ { itm_RAM, 1 } });
  recipes.back()->components.push_back({ { itm_power_supply, 1 } });
  recipes.back()->components.push_back({ { itm_battery, 400 },{ itm_plut_cell, 1 } });

 recipes.push_back(new recipe(++id, itm_bot_turret, CC_MISC, sk_electronics, sk_computer, 7, 9000));
  recipes.back()->tools.push_back({ { itm_screwdriver, -1 },{ itm_toolset, -1 } });
  recipes.back()->tools.push_back({ { itm_soldering_iron, 14 },{ itm_toolset, 14 } });
  recipes.back()->components.push_back({ { itm_smg_9mm, 1 },{ itm_uzi, 1 },{ itm_tec9, 1 },{ itm_calico, 1 },{ itm_hk_mp5, 1 } });
  recipes.back()->components.push_back({ { itm_processor, 2 } });
  recipes.back()->components.push_back({ { itm_RAM, 2 } });
  recipes.back()->components.push_back({ { itm_power_supply, 1 } });
  recipes.back()->components.push_back({ { itm_battery, 500 },{ itm_plut_cell, 1 } });
}
