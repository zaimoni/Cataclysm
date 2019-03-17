#include "player.h"
#include "game.h"
#include "keypress.h"
#include "options.h"
#include "stl_typetraits.h"
#include "recent_msg.h"
#include "saveload.h"

#include <math.h>

using namespace cataclysm;

#include "addiction.h"	// XXX the function definitions, at least, should be inlined somewhere around here
#include "disease.h"	// XXX the function definitions, at least, should be inlined somewhere around here

#include <fstream>

template<> item discard<item>::x = item();

// start prototype for morale.cpp
const std::string morale_point::data[NUM_MORALE_TYPES] = {
	"This is a bug",
	"Enjoyed %i",
	"Music",
	"Marloss Bliss",
	"Good Feeling",

	"Nicotine Craving",
	"Caffeine Craving",
	"Alcohol Craving",
	"Opiate Craving",
	"Speed Craving",
	"Cocaine Craving",

	"Disliked %i",
	"Ate Meat",
	"Wet",
	"Bad Feeling",
	"Killed Innocent",
	"Killed Friend",
	"Killed Mother",

	"Moodswing",
	"Read %i",
	"Heard Disturbing Scream"
};

std::string morale_point::name() const
{
	std::string ret(data[type]);
	std::string item_name(item_type ? item_type->name : "");
	size_t it = ret.find("%i");
	while (it != std::string::npos) {
		ret.replace(it, 2, item_name);
		it = ret.find("%i");
	}
	return ret;
}
// end prototype for morale.cpp

// start prototype for disease.cpp
int disease::speed_boost() const
{
	switch (type) {
	case DI_COLD:		return 0 - duration/5;
	case DI_HEATSTROKE:	return -15;
	case DI_INFECTION:	return -80;
	case DI_SAP:		return -25;
	case DI_SPORES:	return -15;
	case DI_SLIMED:	return -25;
	case DI_BADPOISON:	return -10;
	case DI_FOODPOISON:	return -20;
	case DI_WEBBED:	return -25;
	case DI_ADRENALINE:	return (duration > 150 ? 40 : -10);
	case DI_ASTHMA:	return 0 - duration/5;
	case DI_METH:		return (duration > 600 ? 50 : -40);
	default:		return 0;
	}
}

const char* disease::name() const
{
	switch (type) {
	case DI_GLARE:		return "Glare";
	case DI_COLD:		return "Cold";
	case DI_COLD_FACE:	return "Cold face";
	case DI_COLD_HANDS:	return "Cold hands";
	case DI_COLD_LEGS:	return "Cold legs";
	case DI_COLD_FEET:	return "Cold feet";
	case DI_HOT:		return "Hot";
	case DI_HEATSTROKE:	return "Heatstroke";
	case DI_FBFACE:	return "Frostbite - Face";
	case DI_FBHANDS:	return "Frostbite - Hands";
	case DI_FBFEET:	return "Frostbite - Feet";
	case DI_COMMON_COLD:	return "Common Cold";
	case DI_FLU:		return "Influenza";
	case DI_SMOKE:		return "Smoke";
	case DI_TEARGAS:	return "Tear gas";
	case DI_ONFIRE:	return "On Fire";
	case DI_BOOMERED:	return "Boomered";
	case DI_SAP:		return "Sap-coated";
	case DI_SPORES:	return "Spores";
	case DI_SLIMED:	return "Slimed";
	case DI_DEAF:		return "Deaf";
	case DI_BLIND:		return "Blind";
	case DI_STUNNED:	return "Stunned";
	case DI_DOWNED:	return "Downed";
	case DI_POISON:	return "Poisoned";
	case DI_BADPOISON:	return "Badly Poisoned";
	case DI_FOODPOISON:	return "Food Poisoning";
	case DI_SHAKES:	return "Shakes";
	case DI_FORMICATION:	return "Bugs Under Skin";
	case DI_WEBBED:	return "Webbed";
	case DI_RAT:		return "Ratting";
	case DI_DRUNK:
		if (duration > 2200) return "Wasted";
		if (duration > 1400) return "Trashed";
		if (duration > 800)  return "Drunk";
		return "Tipsy";

	case DI_CIG:		return "Cigarette";
	case DI_HIGH:		return "High";
	case DI_VISUALS:	return "Hallucinating";

	case DI_ADRENALINE:
		if (duration > 150) return "Adrenaline Rush";
		return "Adrenaline Comedown";

	case DI_ASTHMA:
		if (duration > 800) return "Heavy Asthma";
		return "Asthma";

	case DI_METH:
		if (duration > 600) return "High on Meth";
		return "Meth Comedown";

	case DI_IN_PIT:	return "Stuck in Pit";

	case DI_ATTACK_BOOST:  return "Hit Bonus";
	case DI_DAMAGE_BOOST:  return "Damage Bonus";
	case DI_DODGE_BOOST:   return "Dodge Bonus";
	case DI_ARMOR_BOOST:   return "Armor Bonus";
	case DI_SPEED_BOOST:   return "Attack Speed Bonus";
	case DI_VIPER_COMBO:
		switch (intensity) {
		case 1: return "Snakebite Unlocked!";
		case 2: return "Viper Strike Unlocked!";
		default: return "VIPER BUG!!!!";
		}

//	case DI_NULL:		return "";
	default:		return NULL;
	}
}

std::string disease::description() const
{
	int strpen, dexpen, intpen, perpen;
	std::stringstream stream;
	switch (type) {
	case DI_NULL: return "None";
	case DI_GLARE: return "Perception - 1";

	case DI_COLD:
		stream << "Your body in general is uncomfortably cold.\n";
		if (duration >= 5) stream << "Speed -" << int(duration / 5) << "%;";
		if (duration >= 80) stream << "       Dexterity - " << int(duration / 80);
		return stream.str();

	case DI_COLD_FACE:
		stream << "Your face is cold.";
		if (duration >= 100) stream << "  It may become frostbitten.";
		stream << "\n";
		if (duration >= 80) stream << "Perception - " << int(duration / 80);
		return stream.str();

	case DI_COLD_HANDS:
		stream << "Your hands are cold.";
		if (duration >= 100) stream << "  They may become frostbitten.";
		stream << "\n";
		if (duration >= 40) stream << "Dexterity - " << int(duration / 40);
		return stream.str();

	case DI_COLD_LEGS:	// XXX omitted from disease::speed_boost \todo fix
		stream << "Your legs are cold.\n";
		if (duration >= 2) stream << "Speed -" << (duration > 60 ? 30 : int(duration / 2)) << "%";
		return stream.str();

	case DI_COLD_FEET:	// XXX omitted from disease::speed_boost \todo fix
		stream << "Your feet are cold.";
		if (duration >= 100) stream << "  They may become frostbitten.";
		stream << "\n";
		if (duration >= 4) stream << "Speed -" << (duration > 60 ? 15 : int(duration / 4)) << "%";
		return stream.str();

	case DI_HOT: return " You are uncomfortably hot.\n\
Intelligence - 2\n\
You may start suffering heatstroke.";

	case DI_HEATSTROKE:	return "Speed -15%;     Strength - 2;    Intelligence - 2;     Perception - 1";
	case DI_FBFACE:	return "Perception - 2";
	case DI_FBHANDS: return "Dexterity - 2";
	case DI_FBFEET:	return "Speed -40%;     Strength - 1";

	case DI_COMMON_COLD:	return "Increased thirst;  Frequent coughing\n\
Strength - 3;  Dexterity - 1;  Intelligence - 2;  Perception - 1\n\
Symptoms alleviated by medication (Dayquil or Nyquil).";

	case DI_FLU:		return "Increased thirst;  Frequent coughing;  Occasional vomiting\n\
Strength - 4;  Dexterity - 2;  Intelligence - 2;  Perception - 1\n\
Symptoms alleviated by medication (Dayquil or Nyquil).";

	case DI_SMOKE:		return "Strength - 1;     Dexterity - 1;\n\
Occasionally you will cough, costing movement and creating noise.\n\
Loss of health - Torso";

	case DI_TEARGAS:	return "Strength - 2;     Dexterity - 2;    Intelligence - 1;    Perception - 4\n\
Occasionally you will cough, costing movement and creating noise.\n\
Loss of health - Torso";

	case DI_ONFIRE:	return "Loss of health - Entire Body\n\
Your clothing and other equipment may be consumed by the flames.";

	case DI_BOOMERED:	return "Perception - 5\n\
Range of Sight: 1;     All sight is tinted magenta";

	case DI_SAP:		return "Dexterity - 3;   Speed - 25";

	case DI_SPORES:	return "Speed -40%\
You can feel the tiny spores sinking directly into your flesh.";

	case DI_SLIMED:	return "Speed -40%;     Dexterity - 2";
	case DI_DEAF:		return "Sounds will not be reported.  You cannot talk with NPCs.";
	case DI_BLIND:		return "Range of Sight: 0";
	case DI_STUNNED:	return "Your movement is randomized.";
	case DI_DOWNED:	return "You're knocked to the ground.  You have to get up before you can move.";

	case DI_POISON:	return "Perception - 1;    Dexterity - 1;   Strength - 2 IF not resistant\n\
Occasional pain and/or damage.";

	case DI_BADPOISON:	return "Perception - 2;    Dexterity - 2;\n\
Strength - 3 IF not resistant, -1 otherwise\n\
Frequent pain and/or damage.";

	case DI_FOODPOISON:	return "Speed - 35%;     Strength - 3;     Dexterity - 1;     Perception - 1\n\
Your stomach is extremely upset, and you keep having pangs of pain and nausea.";

	case DI_SHAKES:	return "Strength - 1;     Dexterity - 4;";

	case DI_FORMICATION:	return "Strength - 1;     Intelligence - 2;\n\
You stop to scratch yourself frequently; high intelligence helps you resist\n\
this urge.";

	case DI_WEBBED:	return "Strength - 1;     Dexterity - 4;    Speed - 25";

	case DI_RAT:
		intpen = int(duration / 20);
		perpen = int(duration / 25);
		strpen = int(duration / 50);
		stream << "You feal nauseated and rat-like.\n";
		if (intpen > 0) stream << "Intelligence - " << intpen << ";   ";
		if (perpen > 0) stream << "Perception - " << perpen << ";   ";
		if (strpen > 0) stream << "Strength - " << strpen << ";   ";
		return stream.str();

	case DI_DRUNK:
		perpen = int(duration / 1000);
		dexpen = int(duration / 1000);
		intpen = int(duration / 700);
		strpen = int(duration / 1500);
		if (strpen > 0) stream << "Strength - " << strpen << ";    ";
		else if (duration <= 600) stream << "Strength + 1;    ";
		if (dexpen > 0) stream << "Dexterity - " << dexpen << ";    ";
		if (intpen > 0) stream << "Intelligence - " << intpen << ";    ";
		if (perpen > 0) stream << "Perception - " << perpen;
		return stream.str();

	case DI_CIG:
		if (duration >= 600)
			return "Strength - 1;     Dexterity - 1\n\
You smoked too much.";
		return "Dexterity + 1;     Intelligence + 1;     Perception + 1";

	case DI_HIGH: return "Intelligence - 1;     Perception - 1";
	case DI_VISUALS: return "You can't trust everything that you see.";

	case DI_ADRENALINE:
		if (duration > 150) return "Speed +80;   Strength + 5;   Dexterity + 3;   Intelligence - 8;   Perception + 1";
		return "Strength - 2;     Dexterity - 1;     Intelligence - 1;     Perception - 1";

	case DI_ASTHMA:
		stream << "Speed - " << int(duration / 5) << "%;     Strength - 2;     " << "Dexterity - 3";
		return stream.str();

	case DI_METH:
		if (duration > 600) return "Speed +50;  Strength + 2;  Dexterity + 2;  Intelligence + 3;  Perception + 3";
		return "Speed -40;   Strength - 3;   Dexterity - 2;   Intelligence - 2";

	case DI_IN_PIT: return "You're stuck in a pit.  Sight distance is limited and you have to climb out.";

	case DI_ATTACK_BOOST:
		stream << "To-hit bonus + " << intensity;
		return stream.str();

	case DI_DAMAGE_BOOST:
		stream << "Damage bonus + " << intensity;
		return stream.str();

	case DI_DODGE_BOOST:
		stream << "Dodge bonus + " << intensity;
		return stream.str();

	case DI_ARMOR_BOOST:
		stream << "Armor bonus + " << intensity;
		return stream.str();

	case DI_SPEED_BOOST:
		stream << "Attack speed + " << intensity;
		return stream.str();

	case DI_VIPER_COMBO:
		switch (intensity) {
		case 1: return "Your next strike will be a Snakebite, using your hand in a cone shape.  This\n\
will deal piercing damage.";
		case 2: return "Your next strike will be a Viper Strike.  It requires both arms to be in good\n\
condition, and deals massive damage.";
		}

	default: return "Who knows?  This is probably a bug.";
	}
}
// end prototype for disease.cpp

// XXX inappropriate forward declares \todo lift definitions above uses
nc_color encumb_color(int level);
bool activity_is_suspendable(activity_type type);

player::player()
: pos(-1,-1)
{
 str_cur = 8;
 str_max = 8;
 dex_cur = 8;
 dex_max = 8;
 int_cur = 8;
 int_max = 8;
 per_cur = 8;
 per_max = 8;
 underwater = false;
 dodges_left = 1;
 blocks_left = 1;
 power_level = 0;
 max_power_level = 0;
 hunger = 0;
 thirst = 0;
 fatigue = 0;
 stim = 0;
 pain = 0;
 pkill = 0;
 radiation = 0;
 cash = 0;
 recoil = 0;
 driving_recoil = 0;
 scent = 500;
 health = 0;
 name = "";
 male = true;
 inv_sorted = true;
 moves = 100;
 oxygen = 0;
 active_mission = -1;
 in_vehicle = false;
 style_selected = itm_null;
 xp_pool = 0;
 last_item = itype_id(itm_null);
 for (int i = 0; i < num_skill_types; i++) {
  sklevel[i] = 0;
  skexercise[i] = 0;
 }
 for (int i = 0; i < PF_MAX2; i++)
  my_traits[i] = false;
 for (int i = 0; i < PF_MAX2; i++)
  my_mutations[i] = false;

 mutation_category_level[0] = 5; // Weigh us towards no category for a bit
 for (int i = 1; i < NUM_MUTATION_CATEGORIES; i++)
  mutation_category_level[i] = 0;
}

void player::normalize()
{
 weapon = item::null;
 style_selected = itm_null;
 int max_hp = 60 + str_max * 3;
 if (has_trait(PF_TOUGH)) max_hp = int(max_hp * 1.2);

 for (int i = 0; i < num_hp_parts; i++) hp_cur[i] = hp_max[i] = max_hp;
}

void player::pick_name()
{
 std::stringstream ss;
 ss << random_first_name(male) << " " << random_last_name();
 name = ss.str();
}
 
void player::reset(game *g)
{
// Reset our stats to normal levels
// Any persistent buffs/debuffs will take place in disease.h,
// player::suffer(), etc.
 str_cur = str_max;
 dex_cur = dex_max;
 int_cur = int_max;
 per_cur = per_max;
// We can dodge again!
 dodges_left = 1;
 blocks_left = 1;
// Didn't just pick something up
 last_item = itype_id(itm_null);
// Bionic buffs
 if (has_active_bionic(bio_hydraulics))
  str_cur += 20;
 if (has_bionic(bio_eye_enhancer))
  per_cur += 2;
 if (has_bionic(bio_carbon))
  dex_cur -= 2;
 if (has_bionic(bio_armor_head))
  per_cur--;
 if (has_bionic(bio_armor_arms))
  dex_cur--;
 if (has_bionic(bio_metabolics) && power_level < max_power_level &&
     hunger < 100) {
  hunger += 2;
  power_level++;
 }

// Trait / mutation buffs
 if (has_trait(PF_THICK_SCALES))
  dex_cur -= 2;
 if (has_trait(PF_CHITIN2) || has_trait(PF_CHITIN3))
  dex_cur--;
 if (has_trait(PF_COMPOUND_EYES) && !wearing_something_on(bp_eyes))
  per_cur++;
 if (has_trait(PF_ARM_TENTACLES) || has_trait(PF_ARM_TENTACLES_4) ||
     has_trait(PF_ARM_TENTACLES_8))
  dex_cur++;
// Pain
 if (pain > pkill) {
  str_cur  -=     int((pain - pkill) / 15);
  dex_cur  -=     int((pain - pkill) / 15);
  per_cur  -=     int((pain - pkill) / 20);
  int_cur  -= 1 + int((pain - pkill) / 25);
 }
// Morale
 if (abs(morale_level()) >= 100) {
  str_cur  += int(morale_level() / 180);
  dex_cur  += int(morale_level() / 200);
  per_cur  += int(morale_level() / 125);
  int_cur  += int(morale_level() / 100);
 }
// Radiation
 if (radiation > 0) {
  str_cur  -= int(radiation / 80);
  dex_cur  -= int(radiation / 110);
  per_cur  -= int(radiation / 100);
  int_cur  -= int(radiation / 120);
 }
// Stimulants
 dex_cur += int(stim / 10);
 per_cur += int(stim /  7);
 int_cur += int(stim /  6);
 if (stim >= 30) { 
  dex_cur -= int(abs(stim - 15) /  8);
  per_cur -= int(abs(stim - 15) / 12);
  int_cur -= int(abs(stim - 15) / 14);
 }

// Set our scent towards the norm
 int norm_scent = 500;
 if (has_trait(PF_SMELLY)) norm_scent = 800;
 if (has_trait(PF_SMELLY2)) norm_scent = 1200;

 if (scent < norm_scent) scent = int((scent + norm_scent) / 2) + 1;
 else if (scent > norm_scent) scent = int((scent + norm_scent) / 2) - 1;

// Give us our movement points for the turn.
 moves += current_speed(g);

// Floor for our stats.  No stat changes should occur after this!
 if (dex_cur < 0) dex_cur = 0;
 if (str_cur < 0) str_cur = 0;
 if (per_cur < 0) per_cur = 0;
 if (int_cur < 0) int_cur = 0;
 
 int mor = morale_level();
 int xp_frequency = 10 - int(mor / 20);
 if (xp_frequency < 1) xp_frequency = 1;
 if (int(messages.turn) % xp_frequency == 0) xp_pool++;

 if (xp_pool > 800) xp_pool = 800;
}

void player::update_morale()
{
 for (int i = 0; i < morale.size(); i++) {
  if (0 > morale[i].bonus) morale[i].bonus++;
  else if (0 < morale[i].bonus) morale[i].bonus--;

  if (morale[i].bonus == 0) {
   morale.erase(morale.begin() + i);
   i--;
  }
 }
}

int player::current_speed(game *g) const
{
 int newmoves = 100; // Start with 100 movement points...
// Minus some for weight...
 int carry_penalty = 0;
 if (weight_carried() > int(weight_capacity() * .25))
  carry_penalty = 75 * double((weight_carried() - int(weight_capacity() * .25))/
                              (weight_capacity() * .75));

 newmoves -= carry_penalty;

 if (pain > pkill) {
  int pain_penalty = int((pain - pkill) * .7);
  if (pain_penalty > 60) pain_penalty = 60;
  newmoves -= pain_penalty;
 }
 if (pkill >= 10) {
  int pkill_penalty = int(pkill * .1);
  if (pkill_penalty > 30) pkill_penalty = 30;
  newmoves -= pkill_penalty;
 }

 if (abs(morale_level()) >= 100) {
  int morale_bonus = int(morale_level() / 25);
  if (morale_bonus < -10) morale_bonus = -10;
  else if (morale_bonus > 10) morale_bonus = 10;
  newmoves += morale_bonus;
 }

 if (radiation >= 40) {
  int rad_penalty = radiation / 40;
  if (rad_penalty > 20) rad_penalty = 20;
  newmoves -= rad_penalty;
 }

 if (thirst > 40) newmoves -= int((thirst - 40) / 10);
 if (hunger > 100) newmoves -= int((hunger - 100) / 10);

 newmoves += (stim > 40 ? 40 : stim);

 for (const auto& cond : illness) newmoves += cond.speed_boost();

 if (has_trait(PF_QUICK)) newmoves = int(newmoves * 1.10);

 if (g != NULL) {
  if (has_trait(PF_SUNLIGHT_DEPENDENT) && !g->is_in_sunlight(pos)) newmoves -= (g->light_level() >= 12 ? 5 : 10);
  if (has_trait(PF_COLDBLOOD3) && g->temperature < 60) newmoves -= int( (65 - g->temperature) / 2);
  else if (has_trait(PF_COLDBLOOD2) && g->temperature < 60) newmoves -= int( (65 - g->temperature) / 3);
  else if (has_trait(PF_COLDBLOOD) && g->temperature < 60) newmoves -= int( (65 - g->temperature) / 5);
 }

 if (has_artifact_with(AEP_SPEED_UP)) newmoves += 20;
 if (has_artifact_with(AEP_SPEED_DOWN)) newmoves -= 20;

 if (newmoves < 1) newmoves = 1;

 return newmoves;
}

int player::run_cost(int base_cost)
{
 int movecost = base_cost;
 if (has_trait(PF_PARKOUR) && base_cost > 100) {
  movecost *= .5;
  if (movecost < 100)
   movecost = 100;
 }
 if (hp_cur[hp_leg_l] == 0)
  movecost += 50;
 else if (hp_cur[hp_leg_l] < 40)
  movecost += 25;
 if (hp_cur[hp_leg_r] == 0)
  movecost += 50;
 else if (hp_cur[hp_leg_r] < 40)
  movecost += 25;

 if (has_trait(PF_FLEET) && base_cost == 100)
  movecost = int(movecost * .85);
 if (has_trait(PF_FLEET2) && base_cost == 100)
  movecost = int(movecost * .7);
 if (has_trait(PF_PADDED_FEET) && !wearing_something_on(bp_feet))
  movecost = int(movecost * .9);
 if (has_trait(PF_LIGHT_BONES))
  movecost = int(movecost * .9);
 if (has_trait(PF_HOLLOW_BONES))
  movecost = int(movecost * .8);
 if (has_trait(PF_WINGS_INSECT))
  movecost -= 15;
 if (has_trait(PF_LEG_TENTACLES))
  movecost += 20;
 if (has_trait(PF_PONDEROUS1))
  movecost = int(movecost * 1.1);
 if (has_trait(PF_PONDEROUS2))
  movecost = int(movecost * 1.2);
 if (has_trait(PF_PONDEROUS3))
  movecost = int(movecost * 1.3);
 movecost += encumb(bp_mouth) * 5 + encumb(bp_feet) * 5 +
             encumb(bp_legs) * 3;
 if (!wearing_something_on(bp_feet) && !has_trait(PF_PADDED_FEET) &&
     !has_trait(PF_HOOVES))
  movecost += 15;

 return movecost;
}
 

int player::swim_speed()
{
 int ret = 440 + 2 * weight_carried() - 50 * sklevel[sk_swimming];
 if (has_trait(PF_WEBBED))
  ret -= 60 + str_cur * 5;
 if (has_trait(PF_TAIL_FIN))
  ret -= 100 + str_cur * 10;
 if (has_trait(PF_SLEEK_SCALES))
  ret -= 100;
 if (has_trait(PF_LEG_TENTACLES))
  ret -= 60;
 ret += (50 - sklevel[sk_swimming] * 2) * abs(encumb(bp_legs));
 ret += (80 - sklevel[sk_swimming] * 3) * abs(encumb(bp_torso));
 if (sklevel[sk_swimming] < 10) {
  for (int i = 0; i < worn.size(); i++)
   ret += (worn[i].volume() * (10 - sklevel[sk_swimming])) / 2;
 }
 ret -= str_cur * 6 + dex_cur * 4;
// If (ret > 500), we can not swim; so do not apply the underwater bonus.
 if (underwater && ret < 500)
  ret -= 50;
 if (ret < 30)
  ret = 30;
 return ret;
}

nc_color player::color() const
{
 if (has_disease(DI_ONFIRE))
  return c_red;
 if (has_disease(DI_STUNNED))
  return c_ltblue;
 if (has_disease(DI_BOOMERED))
  return c_pink;
 if (underwater)
  return c_blue;
 if (has_active_bionic(bio_cloak) || has_artifact_with(AEP_INVISIBLE))
  return c_dkgray;
 return c_white;
}

void player::load_info(game *g, std::string data)
{
 std::stringstream dump;
 dump << data;
 int inveh;
 int styletmp;
 dump >> pos >> str_cur >> str_max >> dex_cur >> dex_max >>
         int_cur >> int_max >> per_cur >> per_max >> power_level >>
         max_power_level >> hunger >> thirst >> fatigue >> stim >>
         pain >> pkill >> radiation >> cash >> recoil >> driving_recoil >>
         inveh >> scent >> moves >> underwater >> dodges_left >> blocks_left >>
         oxygen >> active_mission >> xp_pool >> male >> health >> styletmp >> activity >> backlog;

 in_vehicle = inveh != 0;
 style_selected = itype_id(styletmp);

 for (int i = 0; i < PF_MAX2; i++)
  dump >> my_traits[i];

 for (int i = 0; i < PF_MAX2; i++)
  dump >> my_mutations[i];

 for (int i = 0; i < NUM_MUTATION_CATEGORIES; i++)
  dump >> mutation_category_level[i];

 for (int i = 0; i < num_hp_parts; i++)
  dump >> hp_cur[i] >> hp_max[i];
 for (int i = 0; i < num_skill_types; i++)
  dump >> sklevel[i] >> skexercise[i];

 int numstyles, typetmp;
 dump >> numstyles;
 for (int i = 0; i < numstyles; i++) {
  dump >> typetmp;
  styles.push_back( itype_id(typetmp) );
 }

 int numill;
 disease illtmp;
 dump >> numill;
 for (int i = 0; i < numill; i++) {
  dump >> typetmp >> illtmp.duration;
  illtmp.type = dis_type(typetmp);
  illness.push_back(illtmp);
 }

 int numadd = 0;
 addiction addtmp;
 dump >> numadd;
 for (int i = 0; i < numadd; i++) {
  dump >> typetmp >> addtmp.intensity >> addtmp.sated;
  addtmp.type = add_type(typetmp);
  addictions.push_back(addtmp);
 }

 int numbio = 0;
 bionic biotmp;
 dump >> numbio;
 for (int i = 0; i < numbio; i++) my_bionics.push_back(bionic(dump));

 int nummor;
 morale_point mortmp;
 dump >> nummor;
 for (int i = 0; i < nummor; i++) {
  int mortype;
  int item_id;
  dump >> mortmp.bonus >> mortype >> item_id;
  mortmp.type = morale_type(mortype);
  mortmp.item_type = (0 >= item_id) ? NULL : item::types[item_id];
  morale.push_back(mortmp);
 }

 int nummis = 0;
 int mistmp;
 dump >> nummis;
 for (int i = 0; i < nummis; i++) {
  dump >> mistmp;
  active_missions.push_back(mistmp);
 }
 dump >> nummis;
 for (int i = 0; i < nummis; i++) {
  dump >> mistmp;
  completed_missions.push_back(mistmp);
 }
 dump >> nummis;
 for (int i = 0; i < nummis; i++) {
  dump >> mistmp;
  failed_missions.push_back(mistmp);
 }
 
}

std::string player::save_info()
{
 std::stringstream dump;
 dump << pos  << " " << str_cur << " " << str_max << " " <<
         dex_cur << " " << dex_max << " " << int_cur << " " << int_max << " " <<
         per_cur << " " << per_max << " " << power_level << " " <<
         max_power_level << " " << hunger << " " << thirst << " " << fatigue <<
         " " << stim << " " << pain << " " << pkill << " " << radiation <<
         " " << cash << " " << recoil << " " << driving_recoil << " " <<
         (in_vehicle? 1 : 0) << " " << scent << " " << moves << " " <<
         underwater << " " << dodges_left << " " << blocks_left << " " <<
         oxygen << " " << active_mission << " " << xp_pool << " " << male <<
         " " << health << " " << style_selected << " " <<
         activity << " " << backlog << " ";

 for (int i = 0; i < PF_MAX2; i++)
  dump << my_traits[i] << " ";
 for (int i = 0; i < PF_MAX2; i++)
  dump << my_mutations[i] << " ";
 for (int i = 0; i < NUM_MUTATION_CATEGORIES; i++)
  dump << mutation_category_level[i] << " ";
 for (int i = 0; i < num_hp_parts; i++)
  dump << hp_cur[i] << " " << hp_max[i] << " ";
 for (int i = 0; i < num_skill_types; i++)
  dump << int(sklevel[i]) << " " << skexercise[i] << " ";

 dump << styles.size() << " ";
 for (int i = 0; i < styles.size(); i++)
  dump << int(styles[i]) << " ";

 dump << illness.size() << " ";
 for (int i = 0; i < illness.size();  i++)
  dump << int(illness[i].type) << " " << illness[i].duration << " ";

 dump << addictions.size() << " ";
 for (int i = 0; i < addictions.size(); i++)
  dump << int(addictions[i].type) << " " << addictions[i].intensity << " " <<
          addictions[i].sated << " ";

 dump << my_bionics.size() << " ";
 for(const auto& bio : my_bionics)  dump << bio << " ";

 dump << morale.size() << " ";
 for (int i = 0; i < morale.size(); i++) {
  dump << morale[i].bonus << " " << morale[i].type << " ";
  if (morale[i].item_type == NULL)
   dump << "0";
  else
   dump << morale[i].item_type->id;
  dump << " ";
 }

 dump << " " << active_missions.size() << " ";
 for (int i = 0; i < active_missions.size(); i++)
  dump << active_missions[i] << " ";

 dump << " " << completed_missions.size() << " ";
 for (int i = 0; i < completed_missions.size(); i++)
  dump << completed_missions[i] << " ";

 dump << " " << failed_missions.size() << " ";
 for (int i = 0; i < failed_missions.size(); i++)
  dump << failed_missions[i] << " ";

 dump << std::endl;

 for (size_t i = 0; i < inv.size(); i++) {
  for(const auto& it : inv.stack_at(i)) {
   dump << "I " << it << std::endl;
   for(const auto& it_2 : it.contents) dump << "C " << it_2 << std::endl;	// \todo blocker: V 0.2.0 should have been handled already
  }
 }
 for(const auto& it : worn) dump << "W " << it << std::endl;
 if (!weapon.is_null()) dump << "w " << weapon << std::endl;
 for(const auto& it : weapon.contents) dump << "c " << it << std::endl;	// \todo blocker: V 0.2.0 should have been handled already

 return dump.str();
}

void player::disp_info(game *g)
{
 int line;
 std::vector<std::string> effect_name;
 std::vector<std::string> effect_text;
 for(const auto& cond : illness) {
  const auto name = cond.name();
  if (!name) continue;
  effect_name.push_back(name);
  effect_text.push_back(cond.description());
 }
 if (abs(morale_level()) >= 100) {
  bool pos = (morale_level() > 0);
  effect_name.push_back(pos ? "Elated" : "Depressed");
  std::stringstream morale_text;
  if (abs(morale_level()) >= 200)
   morale_text << "Dexterity" << (pos ? " +" : " ") <<
                   int(morale_level() / 200) << "   ";
  if (abs(morale_level()) >= 180)
   morale_text << "Strength" << (pos ? " +" : " ") <<
                  int(morale_level() / 180) << "   ";
  if (abs(morale_level()) >= 125)
   morale_text << "Perception" << (pos ? " +" : " ") <<
                  int(morale_level() / 125) << "   ";
  morale_text << "Intelligence" << (pos ? " +" : " ") <<
                 int(morale_level() / 100) << "   ";
  effect_text.push_back(morale_text.str());
 }
 if (pain - pkill > 0) {
  effect_name.push_back("Pain");
  std::stringstream pain_text;
  if (pain - pkill >= 15)
   pain_text << "Strength -" << int((pain - pkill) / 15) << "   Dexterity -" <<
                int((pain - pkill) / 15) << "   ";
  if (pain - pkill >= 20)
   pain_text << "Perception -" << int((pain - pkill) / 15) << "   ";
  pain_text << "Intelligence -" << 1 + int((pain - pkill) / 25);
  effect_text.push_back(pain_text.str());
 }
 if (stim > 0) {
  int dexbonus = int(stim / 10);
  int perbonus = int(stim /  7);
  int intbonus = int(stim /  6);
  if (abs(stim) >= 30) { 
   dexbonus -= int(abs(stim - 15) /  8);
   perbonus -= int(abs(stim - 15) / 12);
   intbonus -= int(abs(stim - 15) / 14);
  }
  
  if (dexbonus < 0)
   effect_name.push_back("Stimulant Overdose");
  else
   effect_name.push_back("Stimulant");
  std::stringstream stim_text;
  stim_text << "Speed +" << stim << "   Intelligence " <<
               (intbonus > 0 ? "+ " : "") << intbonus << "   Perception " <<
               (perbonus > 0 ? "+ " : "") << perbonus << "   Dexterity "  <<
               (dexbonus > 0 ? "+ " : "") << dexbonus;
  effect_text.push_back(stim_text.str());
 } else if (stim < 0) {
  effect_name.push_back("Depressants");
  std::stringstream stim_text;
  int dexpen = int(stim / 10);
  int perpen = int(stim /  7);
  int intpen = int(stim /  6);
// Since dexpen etc. are always less than 0, no need for + signs
  stim_text << "Speed " << stim << "   Intelligence " << intpen <<
               "   Perception " << perpen << "   Dexterity " << dexpen;
  effect_text.push_back(stim_text.str());
 }

 if (g->is_in_sunlight(pos)) {
	 if ((has_trait(PF_TROGLO) && g->weather == WEATHER_SUNNY) ||
		 (has_trait(PF_TROGLO2) && g->weather != WEATHER_SUNNY)) {
		 effect_name.push_back("In Sunlight");
		 effect_text.push_back("The sunlight irritates you.\n\
Strength - 1;    Dexterity - 1;    Intelligence - 1;    Dexterity - 1");
	 } else if (has_trait(PF_TROGLO2)) {
		 effect_name.push_back("In Sunlight");
		 effect_text.push_back("The sunlight irritates you badly.\n\
Strength - 2;    Dexterity - 2;    Intelligence - 2;    Dexterity - 2");
	 } else if (has_trait(PF_TROGLO3)) {
		 effect_name.push_back("In Sunlight");
		 effect_text.push_back("The sunlight irritates you terribly.\n\
Strength - 4;    Dexterity - 4;    Intelligence - 4;    Dexterity - 4");
	 }
 }

 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].sated < 0 &&
      addictions[i].intensity >= MIN_ADDICTION_LEVEL) {
   effect_name.push_back(addiction_name(addictions[i]));
   effect_text.push_back(addiction_text(addictions[i]));
  }
 }

 WINDOW* w_grid    = newwin(VIEW, SCREEN_WIDTH,  0,  0);
 WINDOW* w_stats   = newwin( 9, VIEW + 1,  2,  0);
 WINDOW* w_encumb  = newwin( 9, VIEW + 1, 12,  0);
 WINDOW* w_traits  = newwin( 9, VIEW + 1,  2, 27);
 WINDOW* w_effects = newwin( 9, VIEW + 1, 12, 27);
 WINDOW* w_skills  = newwin( 9, VIEW + 1,  2, 54);
 WINDOW* w_speed   = newwin( 9, VIEW + 1, 12, 54);
 WINDOW* w_info    = newwin( 3, SCREEN_WIDTH, 22,  0);
// Print name and header
 mvwprintw(w_grid, 0, 0, "%s - %s", name.c_str(), (male ? "Male" : "Female"));
 mvwprintz(w_grid, 0, 39, c_ltred, "| Press TAB to cycle, ESC or q to return.");
// Main line grid
 for (int i = 0; i < SCREEN_WIDTH; i++) {
  mvwputch(w_grid,  1, i, c_ltgray, LINE_OXOX);
  mvwputch(w_grid, 21, i, c_ltgray, LINE_OXOX);
  mvwputch(w_grid, 11, i, c_ltgray, LINE_OXOX);
  if (i > 1 && i < 21) {
   mvwputch(w_grid, i, 26, c_ltgray, LINE_XOXO);
   mvwputch(w_grid, i, 53, c_ltgray, LINE_XOXO);
  }
 }
 mvwputch(w_grid,  1, 26, c_ltgray, LINE_OXXX);
 mvwputch(w_grid,  1, 53, c_ltgray, LINE_OXXX);
 mvwputch(w_grid, 21, 26, c_ltgray, LINE_XXOX);
 mvwputch(w_grid, 21, 53, c_ltgray, LINE_XXOX);
 mvwputch(w_grid, 11, 26, c_ltgray, LINE_XXXX);
 mvwputch(w_grid, 11, 53, c_ltgray, LINE_XXXX);
 wrefresh(w_grid);	// w_grid should stay static.

// First!  Default STATS screen.
 mvwprintz(w_stats, 0, 10, c_ltgray, "STATS");
 mvwprintz(w_stats, 2,  2, c_ltgray, "Strength:%s(%d)",
           (str_max < 10 ? "         " : "        "), str_max);
 mvwprintz(w_stats, 3,  2, c_ltgray, "Dexterity:%s(%d)",
           (dex_max < 10 ? "        "  : "       "),  dex_max);
 mvwprintz(w_stats, 4,  2, c_ltgray, "Intelligence:%s(%d)",
           (int_max < 10 ? "     "     : "    "),     int_max);
 mvwprintz(w_stats, 5,  2, c_ltgray, "Perception:%s(%d)",
           (per_max < 10 ? "       "   : "      "),   per_max);

 nc_color status = c_white;

 if (str_cur <= 0)
  status = c_dkgray;
 else if (str_cur < str_max / 2)
  status = c_red;
 else if (str_cur < str_max)
  status = c_ltred;
 else if (str_cur == str_max)
  status = c_white;
 else if (str_cur < str_max * 1.5)
  status = c_ltgreen;
 else
  status = c_green;
 mvwprintz(w_stats,  2, (str_cur < 10 ? 17 : 16), status, "%d", str_cur);

 if (dex_cur <= 0)
  status = c_dkgray;
 else if (dex_cur < dex_max / 2)
  status = c_red;
 else if (dex_cur < dex_max)
  status = c_ltred;
 else if (dex_cur == dex_max)
  status = c_white;
 else if (dex_cur < dex_max * 1.5)
  status = c_ltgreen;
 else
  status = c_green;
 mvwprintz(w_stats,  3, (dex_cur < 10 ? 17 : 16), status, "%d", dex_cur);

 if (int_cur <= 0)
  status = c_dkgray;
 else if (int_cur < int_max / 2)
  status = c_red;
 else if (int_cur < int_max)
  status = c_ltred;
 else if (int_cur == int_max)
  status = c_white;
 else if (int_cur < int_max * 1.5)
  status = c_ltgreen;
 else
  status = c_green;
 mvwprintz(w_stats,  4, (int_cur < 10 ? 17 : 16), status, "%d", int_cur);

 if (per_cur <= 0)
  status = c_dkgray;
 else if (per_cur < per_max / 2)
  status = c_red;
 else if (per_cur < per_max)
  status = c_ltred;
 else if (per_cur == per_max)
  status = c_white;
 else if (per_cur < per_max * 1.5)
  status = c_ltgreen;
 else
  status = c_green;
 mvwprintz(w_stats,  5, (per_cur < 10 ? 17 : 16), status, "%d", per_cur);

 wrefresh(w_stats);

// Next, draw encumberment.
 mvwprintz(w_encumb, 0, 6, c_ltgray, "ENCUMBERANCE");
 mvwprintz(w_encumb, 2, 2, c_ltgray, "Head................");
 mvwprintz(w_encumb, 2,(encumb(bp_head) >= 0 && encumb(bp_head) < 10 ? 21 : 20),
           encumb_color(encumb(bp_head)), "%d", encumb(bp_head));
 mvwprintz(w_encumb, 3, 2, c_ltgray, "Eyes................");
 mvwprintz(w_encumb, 3,(encumb(bp_eyes) >= 0 && encumb(bp_eyes) < 10 ? 21 : 20),
           encumb_color(encumb(bp_eyes)), "%d", encumb(bp_eyes));
 mvwprintz(w_encumb, 4, 2, c_ltgray, "Mouth...............");
 mvwprintz(w_encumb, 4,(encumb(bp_mouth)>=0 && encumb(bp_mouth) < 10 ? 21 : 20),
           encumb_color(encumb(bp_mouth)), "%d", encumb(bp_mouth));
 mvwprintz(w_encumb, 5, 2, c_ltgray, "Torso...............");
 mvwprintz(w_encumb, 5,(encumb(bp_torso)>=0 && encumb(bp_torso) < 10 ? 21 : 20),
           encumb_color(encumb(bp_torso)), "%d", encumb(bp_torso));
 mvwprintz(w_encumb, 6, 2, c_ltgray, "Hands...............");
 mvwprintz(w_encumb, 6,(encumb(bp_hands)>=0 && encumb(bp_hands) < 10 ? 21 : 20),
           encumb_color(encumb(bp_hands)), "%d", encumb(bp_hands));
 mvwprintz(w_encumb, 7, 2, c_ltgray, "Legs................");
 mvwprintz(w_encumb, 7,(encumb(bp_legs) >= 0 && encumb(bp_legs) < 10 ? 21 : 20),
           encumb_color(encumb(bp_legs)), "%d", encumb(bp_legs));
 mvwprintz(w_encumb, 8, 2, c_ltgray, "Feet................");
 mvwprintz(w_encumb, 8,(encumb(bp_feet) >= 0 && encumb(bp_feet) < 10 ? 21 : 20),
           encumb_color(encumb(bp_feet)), "%d", encumb(bp_feet));
 wrefresh(w_encumb);

// Next, draw traits.
 line = 2;
 std::vector <pl_flag> traitslist;
 mvwprintz(w_traits, 0, 9, c_ltgray, "TRAITS");
 for (int i = 0; i < PF_MAX2; i++) {
  if (my_traits[i]) {
   traitslist.push_back(pl_flag(i));
   if (line < 9) {
    const auto& tr = mutation_branch::traits[i];
    mvwprintz(w_traits, line, 1, (0 < tr.points ? c_ltgreen : c_ltred), tr.name.c_str());
    line++;
   }
  }
 }
 wrefresh(w_traits);

// Next, draw effects.
 line = 2;
 mvwprintz(w_effects, 0, 8, c_ltgray, "EFFECTS");
 for (int i = 0; i < effect_name.size() && line < 9; i++) {
  mvwprintz(w_effects, line, 1, c_ltgray, effect_name[i].c_str());
  line++;
 }
 wrefresh(w_effects);

// Next, draw skills.
 line = 2;
 std::vector <skill> skillslist;
 mvwprintz(w_skills, 0, 11, c_ltgray, "SKILLS");
 for (int i = 1; i < num_skill_types; i++) {
  if (sklevel[i] > 0) {
   skillslist.push_back(skill(i));
   if (line < 9) {
    mvwprintz(w_skills, line, 1, c_ltblue, "%s:",
              skill_name(skill(i)).c_str());
    mvwprintz(w_skills, line,19, c_ltblue, "%d%s(%s%d%%%%)", sklevel[i],
              (sklevel[i] < 10 ? " " : ""),
              (skexercise[i] < 10 && skexercise[i] >= 0 ? " " : ""),
              (skexercise[i] <  0 ? 0 : skexercise[i]));
    line++;
   }
  }
 }
 wrefresh(w_skills);

// Finally, draw speed.
 mvwprintz(w_speed, 0, 11, c_ltgray, "SPEED");
 mvwprintz(w_speed, 1,  1, c_ltgray, "Base Move Cost:");
 mvwprintz(w_speed, 2,  1, c_ltgray, "Current Speed:");
 int newmoves = current_speed(g);
 int pen = 0;
 line = 3;
 if (weight_carried() > int(weight_capacity() * .25)) {
  pen = 75 * double((weight_carried() - int(weight_capacity() * .25)) /
                    (weight_capacity() * .75));
  mvwprintz(w_speed, line, 1, c_red, "Overburdened        -%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 pen = int(morale_level() / 25);
 if (abs(pen) >= 4) {
  if (pen > 10)
   pen = 10;
  else if (pen < -10)
   pen = -10;
  if (pen > 0)
   mvwprintz(w_speed, line, 1, c_green, "Good mood           +%s%d%%%%",
             (pen < 10 ? " " : ""), pen);
  else
   mvwprintz(w_speed, line, 1, c_red, "Depressed           -%s%d%%%%",
             (abs(pen) < 10 ? " " : ""), abs(pen));
  line++;
 }
 pen = int((pain - pkill) * .7);
 if (pen > 60)
  pen = 60;
 if (pen >= 1) {
  mvwprintz(w_speed, line, 1, c_red, "Pain                -%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (pkill >= 10) {
  pen = int(pkill * .1);
  mvwprintz(w_speed, line, 1, c_red, "Painkillers         -%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (stim != 0) {
  pen = stim;
  if (pen > 0)
   mvwprintz(w_speed, line, 1, c_green, "Stimulants          +%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  else
   mvwprintz(w_speed, line, 1, c_red, "Depressants         -%s%d%%%%",
            (abs(pen) < 10 ? " " : ""), abs(pen));
  line++;
 }
 if (thirst > 40) {
  pen = int((thirst - 40) / 10);
  mvwprintz(w_speed, line, 1, c_red, "Thirst              -%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (hunger > 100) {
  pen = int((hunger - 100) / 10);
  mvwprintz(w_speed, line, 1, c_red, "Hunger              -%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if (has_trait(PF_SUNLIGHT_DEPENDENT) && !g->is_in_sunlight(pos)) {
  pen = (g->light_level() >= 12 ? 5 : 10);
  mvwprintz(w_speed, line, 1, c_red, "Out of Sunlight     -%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  line++;
 }
 if ((has_trait(PF_COLDBLOOD) || has_trait(PF_COLDBLOOD2) ||
      has_trait(PF_COLDBLOOD3)) && g->temperature < 65) {
  if (has_trait(PF_COLDBLOOD3))
   pen = int( (65 - g->temperature) / 2);
  else if (has_trait(PF_COLDBLOOD2))
   pen = int( (65 - g->temperature) / 3);
  else
   pen = int( (65 - g->temperature) / 2);
  mvwprintz(w_speed, line, 1, c_red, "Cold-Blooded        -%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
  line++;
 }

 for(const auto& cond : illness) {
  int move_adjust = cond.speed_boost();
  if (0 == move_adjust) continue;
  const bool good = move_adjust > 0;
  const nc_color col = (good ? c_green : c_red);
  mvwprintz(w_speed, line,  1, col, cond.name());
  mvwprintz(w_speed, line, 21, col, (good ? "+" : "-"));
  if (good) move_adjust = -move_adjust;
  mvwprintz(w_speed, line, (move_adjust >= 10 ? 22 : 23), col, "%d%%%%", move_adjust);
 }
 if (has_trait(PF_QUICK)) {
  pen = int(newmoves * .1);
  mvwprintz(w_speed, line, 1, c_green, "Quick               +%s%d%%%%",
            (pen < 10 ? " " : ""), pen);
 }
 int runcost = run_cost(100);
 nc_color col = (runcost <= 100 ? c_green : c_red);
 mvwprintz(w_speed, 1, (runcost  >= 100 ? 21 : (runcost  < 10 ? 23 : 22)), col,
           "%d", runcost);
 col = (newmoves >= 100 ? c_green : c_red);
 mvwprintz(w_speed, 2, (newmoves >= 100 ? 21 : (newmoves < 10 ? 23 : 22)), col,
           "%d", newmoves);
 wrefresh(w_speed);

 refresh();
 int curtab = 1;
 int min, max;
 line = 0;
 bool done = false;

// Initial printing is DONE.  Now we give the player a chance to scroll around
// and "hover" over different items for more info.
 do {
  werase(w_info);
  switch (curtab) {
  case 1:	// Stats tab
   mvwprintz(w_stats, 0, 0, h_ltgray, "          STATS           ");
   if (line == 0) {
    mvwprintz(w_stats, 2, 2, h_ltgray, "Strength:");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Strength affects your melee damage, the amount of weight you can carry, your\n\
total HP, your resistance to many diseases, and the effectiveness of actions\n\
which require brute force.");
   } else if (line == 1) {
    mvwprintz(w_stats, 3, 2, h_ltgray, "Dexterity:");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Dexterity affects your chance to hit in melee combat, helps you steady your\n\
gun for ranged combat, and enhances many actions that require finesse."); 
   } else if (line == 2) {
    mvwprintz(w_stats, 4, 2, h_ltgray, "Intelligence:");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Intelligence is less important in most situations, but it is vital for more\n\
complex tasks like electronics crafting. It also affects how much skill you\n\
can pick up from reading a book.");
   } else if (line == 3) {
    mvwprintz(w_stats, 5, 2, h_ltgray, "Perception:");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Perception is the most important stat for ranged combat. It's also used for\n\
detecting traps and other things of interest.");
   }
   wrefresh(w_stats);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     line++;
     if (line == 4)
      line = 0;
     break;
    case 'k':
     line--;
     if (line == -1)
      line = 3;
     break;
    case '\t':
     mvwprintz(w_stats, 0, 0, c_ltgray, "          STATS           ");
     wrefresh(w_stats);
     line = 0;
     curtab++;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   mvwprintz(w_stats, 2, 2, c_ltgray, "Strength:");
   mvwprintz(w_stats, 3, 2, c_ltgray, "Dexterity:");
   mvwprintz(w_stats, 4, 2, c_ltgray, "Intelligence:");
   mvwprintz(w_stats, 5, 2, c_ltgray, "Perception:");
   wrefresh(w_stats);
   break;
  case 2:	// Encumberment tab
   mvwprintz(w_encumb, 0, 0, h_ltgray, "      ENCUMBERANCE        ");
   if (line == 0) {
    mvwprintz(w_encumb, 2, 2, h_ltgray, "Head");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Head encumberance has no effect; it simply limits how much you can put on.");
   } else if (line == 1) {
    mvwprintz(w_encumb, 3, 2, h_ltgray, "Eyes");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Perception -%d when checking traps or firing ranged weapons;\n\
Perception -%.1f when throwing items", encumb(bp_eyes),
double(double(encumb(bp_eyes)) / 2));
   } else if (line == 2) {
    mvwprintz(w_encumb, 4, 2, h_ltgray, "Mouth");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Running costs +%d movement points", encumb(bp_mouth) * 5);
   } else if (line == 3) {
    mvwprintz(w_encumb, 5, 2, h_ltgray, "Torso");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Melee skill -%d;      Dodge skill -%d;\n\
Swimming costs +%d movement points;\n\
Melee attacks cost +%d movement points", encumb(bp_torso), encumb(bp_torso),
encumb(bp_torso) * (80 - sklevel[sk_swimming] * 3), encumb(bp_torso) * 20);
   } else if (line == 4) {
    mvwprintz(w_encumb, 6, 2, h_ltgray, "Hands");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Reloading costs +%d movement points;\n\
Dexterity -%d when throwing items", encumb(bp_hands) * 30, encumb(bp_hands));
   } else if (line == 5) {
    mvwprintz(w_encumb, 7, 2, h_ltgray, "Legs");
    std::string sign = (encumb(bp_legs) >= 0 ? "+" : "");
    std::string osign = (encumb(bp_legs) < 0 ? "+" : "-");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Running costs %s%d movement points;  Swimming costs %s%d movement points;\n\
Dodge skill %s%.1f", sign.c_str(), encumb(bp_legs) * 3,
                     sign.c_str(), encumb(bp_legs) *(50 - sklevel[sk_swimming]),
                     osign.c_str(), double(double(encumb(bp_legs)) / 2));
   } else if (line == 6) {
    mvwprintz(w_encumb, 8, 2, h_ltgray, "Feet");
    mvwprintz(w_info, 0, 0, c_magenta, "\
Running costs %s%d movement points", (encumb(bp_feet) >= 0 ? "+" : ""),
encumb(bp_feet) * 5);
   }
   wrefresh(w_encumb);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     line++;
     if (line == 7)
      line = 0;
     break;
    case 'k':
     line--;
     if (line == -1)
      line = 6;
     break;
    case '\t':
     mvwprintz(w_encumb, 0, 0, c_ltgray, "      ENCUMBERANCE        ");
     wrefresh(w_encumb);
     line = 0;
     curtab++;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   mvwprintz(w_encumb, 2, 2, c_ltgray, "Head");
   mvwprintz(w_encumb, 3, 2, c_ltgray, "Eyes");
   mvwprintz(w_encumb, 4, 2, c_ltgray, "Mouth");
   mvwprintz(w_encumb, 5, 2, c_ltgray, "Torso");
   mvwprintz(w_encumb, 6, 2, c_ltgray, "Hands");
   mvwprintz(w_encumb, 7, 2, c_ltgray, "Legs");
   mvwprintz(w_encumb, 8, 2, c_ltgray, "Feet");
   wrefresh(w_encumb);
   break;
  case 3:	// Traits tab
   mvwprintz(w_traits, 0, 0, h_ltgray, "         TRAITS           ");
   if (line <= 2) {
    min = 0;
    max = 7;
    if (traitslist.size() < max)
     max = traitslist.size();
   } else if (line >= traitslist.size() - 3) {
    min = (traitslist.size() < 8 ? 0 : traitslist.size() - 7);
    max = traitslist.size();
   } else {
    min = line - 3;
    max = line + 4;
    if (traitslist.size() < max) max = traitslist.size();
   }
   for (int i = min; i < max; i++) {
    mvwprintz(w_traits, 2 + i - min, 1, c_ltgray, "                         ");
	if (traitslist[i] >= PF_MAX2) continue;	// XXX out of bounds dereference \todo better recovery strategy
	const auto& tr = mutation_branch::traits[traitslist[i]];
	status = (tr.points > 0 ? c_ltgreen : c_ltred);
    if (i == line)
     mvwprintz(w_traits, 2 + i - min, 1, hilite(status), tr.name.c_str());
    else
     mvwprintz(w_traits, 2 + i - min, 1, status, tr.name.c_str());
   }
   if (line >= 0 && line < traitslist.size())
    mvwprintz(w_info, 0, 0, c_magenta, "%s",
		mutation_branch::traits[traitslist[line]].description.c_str());
   wrefresh(w_traits);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     if (line < traitslist.size() - 1)
      line++;
     break;
    case 'k':
     if (line > 0)
      line--;
     break;
    case '\t':
     mvwprintz(w_traits, 0, 0, c_ltgray, "         TRAITS           ");
     for (int i = 0; i < traitslist.size() && i < 7; i++) {
      mvwprintz(w_traits, i + 2, 1, c_black, "xxxxxxxxxxxxxxxxxxxxxxxxx");
	  if (traitslist[i] >= PF_MAX2) continue;	// XXX out of bounds dereference \todo better recovery strategy
	  const auto& tr = mutation_branch::traits[traitslist[i]];
      mvwprintz(w_traits, i + 2, 1, (0 < tr.points ? c_ltgreen : c_ltred), tr.name.c_str());
     }
     wrefresh(w_traits);
     line = 0;
     curtab++;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   break;

  case 4:	// Effects tab
   mvwprintz(w_effects, 0, 0, h_ltgray, "        EFFECTS           ");
   if (line <= 2) {
    min = 0;
    max = 7;
    if (effect_name.size() < max)
     max = effect_name.size();
   } else if (line >= effect_name.size() - 3) {
    min = (effect_name.size() < 8 ? 0 : effect_name.size() - 7);
    max = effect_name.size();
   } else {
    min = line - 2;
    max = line + 4;
    if (effect_name.size() < max)
     max = effect_name.size();
   }
   for (int i = min; i < max; i++) {
    if (i == line)
     mvwprintz(w_effects, 2 + i - min, 1, h_ltgray, effect_name[i].c_str());
    else
     mvwprintz(w_effects, 2 + i - min, 1, c_ltgray, effect_name[i].c_str());
   }
   if (line >= 0 && line < effect_text.size())
    mvwprintz(w_info, 0, 0, c_magenta, effect_text[line].c_str());
   wrefresh(w_effects);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     if (line < effect_name.size() - 1)
      line++;
     break;
    case 'k':
     if (line > 0)
      line--;
     break;
    case '\t':
     mvwprintz(w_effects, 0, 0, c_ltgray, "        EFFECTS           ");
     for (int i = 0; i < effect_name.size() && i < 7; i++)
      mvwprintz(w_effects, i + 2, 1, c_ltgray, effect_name[i].c_str());
     wrefresh(w_effects);
     line = 0;
     curtab++;
     break;
    case 'q':
    case KEY_ESCAPE:
     done = true;
   }
   break;

  case 5:	// Skills tab
   mvwprintz(w_skills, 0, 0, h_ltgray, "           SKILLS         ");
   if (line <= 2) {
    min = 0;
    max = 7;
    if (skillslist.size() < max)
     max = skillslist.size();
   } else if (line >= skillslist.size() - 3) {
    min = (skillslist.size() < 8 ? 0 : skillslist.size() - 7);
    max = skillslist.size();
   } else {
    min = line - 3;
    max = line + 4;
    if (skillslist.size() < max)
     max = skillslist.size();
    if (min < 0)
     min = 0;
   }
   for (int i = min; i < max; i++) {
    if (i == line) {
     if (skexercise[skillslist[i]] >= 100)
      status = h_pink;
     else
      status = h_ltblue;
    } else {
     if (skexercise[skillslist[i]] < 0)
      status = c_ltred;
     else
      status = c_ltblue;
    }
    mvwprintz(w_skills, 2 + i - min, 1, c_ltgray, "                         ");
    if (skexercise[i] >= 100) {
     mvwprintz(w_skills, 2 + i - min, 1, status, "%s:",
               skill_name(skillslist[i]).c_str());
     mvwprintz(w_skills, 2 + i - min,19, status, "%d (%s%d%%%%)",
               sklevel[skillslist[i]],
               (skexercise[skillslist[i]] < 10 &&
                skexercise[skillslist[i]] >= 0 ? " " : ""),
               (skexercise[skillslist[i]] <  0 ? 0 :
                skexercise[skillslist[i]]));
    } else {
     mvwprintz(w_skills, 2 + i - min, 1, status, "%s:",
               skill_name(skillslist[i]).c_str());
     mvwprintz(w_skills, 2 + i - min,19, status, "%d (%s%d%%%%)", 
               sklevel[skillslist[i]],
               (skexercise[skillslist[i]] < 10 &&
                skexercise[skillslist[i]] >= 0 ? " " : ""),
               (skexercise[skillslist[i]] <  0 ? 0 :
                skexercise[skillslist[i]]));
    }
   }
   werase(w_info);
   if (line >= 0 && line < skillslist.size())
    mvwprintz(w_info, 0, 0, c_magenta,
              skill_description(skillslist[line]).c_str());
   wrefresh(w_skills);
   wrefresh(w_info);
   switch (input()) {
    case 'j':
     if (line < skillslist.size() - 1)
      line++;
     break;
    case 'k':
     if (line > 0)
      line--;
     break;
    case '\t':
     mvwprintz(w_skills, 0, 0, c_ltgray, "           SKILLS         ");
     for (int i = 0; i < skillslist.size() && i < 7; i++) {
      if (skexercise[skillslist[i]] < 0)
       status = c_ltred;
      else
       status = c_ltblue;
      mvwprintz(w_skills, i + 2,  1, status, "%s:",
                skill_name(skillslist[i]).c_str());
      mvwprintz(w_skills, i + 2, 19, status, "%d (%s%d%%%%)",
                sklevel[skillslist[i]],
                (skexercise[skillslist[i]] < 10 &&
                 skexercise[skillslist[i]] >= 0 ? " " : ""),
                (skexercise[skillslist[i]] <  0 ? 0 :
                 skexercise[skillslist[i]]));
     }
     wrefresh(w_skills);
     line = 0;
     curtab = 1;
     break;
    case 'q':
    case 'Q':
    case KEY_ESCAPE:
     done = true;
   }
  }
 } while (!done);
 
 werase(w_info);
 werase(w_grid);
 werase(w_stats);
 werase(w_encumb);
 werase(w_traits);
 werase(w_effects);
 werase(w_skills);
 werase(w_speed);
 werase(w_info);

 delwin(w_info);
 delwin(w_grid);
 delwin(w_stats);
 delwin(w_encumb);
 delwin(w_traits);
 delwin(w_effects);
 delwin(w_skills);
 delwin(w_speed);
 erase();
}

void player::disp_morale()
{
 WINDOW *w = newwin(VIEW, SCREEN_WIDTH, 0, 0);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 mvwprintz(w, 1,  1, c_white, "Morale Modifiers:");
 mvwprintz(w, 2,  1, c_ltgray, "Name");
 mvwprintz(w, 2, 20, c_ltgray, "Value");

 for (int i = 0; i < morale.size(); i++) {
  int b = morale[i].bonus;

  int bpos = 24;
  if (abs(b) >= 10) bpos--;
  if (abs(b) >= 100) bpos--;
  if (b < 0) bpos--;

  mvwprintz(w, i + 3,  1, (b < 0 ? c_red : c_green), morale[i].name().c_str());
  mvwprintz(w, i + 3, bpos, (b < 0 ? c_red : c_green), "%d", b);
 }

 int mor = morale_level();
 int bpos = 24;
 if (abs(mor) >= 10) bpos--;
 if (abs(mor) >= 100) bpos--;
 if (mor < 0) bpos--;
 mvwprintz(w, 20, 1, (mor < 0 ? c_red : c_green), "Total:");
 mvwprintz(w, 20, bpos, (mor < 0 ? c_red : c_green), "%d", mor);

 wrefresh(w);
 getch();
 werase(w);
 delwin(w);
}
 

void player::disp_status(WINDOW *w, game *g)
{
 mvwprintz(w, 1, 0, c_ltgray, "Weapon: %s", weapname().c_str());
 if (weapon.is_gun()) {
   int adj_recoil = recoil + driving_recoil;
       if (adj_recoil >= 36)
   mvwprintz(w, 1, 30, c_red,    "Recoil");
  else if (adj_recoil >= 20)
   mvwprintz(w, 1, 30, c_ltred,  "Recoil");
  else if (adj_recoil >= 4)
   mvwprintz(w, 1, 30, c_yellow, "Recoil");
  else if (adj_recoil > 0)
   mvwprintz(w, 1, 30, c_ltgray, "Recoil");
 }

      if (hunger > 2800)
  mvwprintz(w, 2, 0, c_red,    "Starving!");
 else if (hunger > 1400)
  mvwprintz(w, 2, 0, c_ltred,  "Near starving");
 else if (hunger > 300)
  mvwprintz(w, 2, 0, c_ltred,  "Famished");
 else if (hunger > 100)
  mvwprintz(w, 2, 0, c_yellow, "Very hungry");
 else if (hunger > 40)
  mvwprintz(w, 2, 0, c_yellow, "Hungry");
 else if (hunger < 0)
  mvwprintz(w, 2, 0, c_green,  "Full");

      if (thirst > 520)
  mvwprintz(w, 2, 15, c_ltred,  "Parched");
 else if (thirst > 240)
  mvwprintz(w, 2, 15, c_ltred,  "Dehydrated");
 else if (thirst > 80)
  mvwprintz(w, 2, 15, c_yellow, "Very thirsty");
 else if (thirst > 40)
  mvwprintz(w, 2, 15, c_yellow, "Thirsty");
 else if (thirst < 0)
  mvwprintz(w, 2, 15, c_green,  "Slaked");

      if (fatigue > 575)
  mvwprintz(w, 2, 30, c_red,    "Exhausted");
 else if (fatigue > 383)
  mvwprintz(w, 2, 30, c_ltred,  "Dead tired");
 else if (fatigue > 191)
  mvwprintz(w, 2, 30, c_yellow, "Tired");

 mvwprintz(w, 2, 41, c_white, "XP: ");
 nc_color col_xp = c_dkgray;
 if (xp_pool >= 100)
  col_xp = c_white;
 else if (xp_pool >  0)
  col_xp = c_ltgray;
 mvwprintz(w, 2, 45, col_xp, "%d", xp_pool);

 nc_color col_pain = c_yellow;
 if (pain - pkill >= 60)
  col_pain = c_red;
 else if (pain - pkill >= 40)
  col_pain = c_ltred;
 if (pain - pkill > 0)
  mvwprintz(w, 3, 0, col_pain, "Pain: %d", pain - pkill);

 vehicle *veh = g->m.veh_at(pos);
 int dmor = 0;
 //int dmor = (in_vehicle && veh) ? 0 : 9;

 int morale_cur = morale_level ();
 nc_color col_morale = c_white;
 if (morale_cur >= 10)
  col_morale = c_green;
 else if (morale_cur <= -10)
  col_morale = c_red;
 if (morale_cur >= 100)
  mvwprintz(w, 3, 10 + dmor, col_morale, ":D");
 else if (morale_cur >= 10)
  mvwprintz(w, 3, 10 + dmor, col_morale, ":)");
 else if (morale_cur > -10)
  mvwprintz(w, 3, 10 + dmor, col_morale, ":|");
 else if (morale_cur > -100)
  mvwprintz(w, 3, 10 + dmor, col_morale, ":(");
 else
  mvwprintz(w, 3, 10 + dmor, col_morale, "D:");

 if (in_vehicle && veh) {
  veh->print_fuel_indicator (w, 3, 49);
  nc_color col_indf1 = c_ltgray;

  float strain = veh->strain();
  nc_color col_vel = strain <= 0? c_ltblue :
                     (strain <= 0.2? c_yellow :
                     (strain <= 0.4? c_ltred : c_red));

  bool has_turrets = false;
  for (int p = 0; p < veh->parts.size(); p++) {
   if (veh->part_flag (p, vpf_turret)) {
    has_turrets = true;
    break;
   }
  }

  if (has_turrets) {
   mvwprintz(w, 3, 25, col_indf1, "Gun:");
   mvwprintz(w, 3, 29, veh->turret_mode ? c_ltred : c_ltblue,
                       veh->turret_mode ? "auto" : "off ");
  }

  if (veh->cruise_on) {
   if(OPTIONS[OPT_USE_METRIC_SYS]) {
    mvwprintz(w, 3, 33, col_indf1, "{Km/h....>....}");
    mvwprintz(w, 3, 38, col_vel, "%4d", int(veh->velocity * 0.0161f));
    mvwprintz(w, 3, 43, c_ltgreen, "%4d", int(veh->cruise_velocity * 0.0161f));
   }
   else {
    mvwprintz(w, 3, 34, col_indf1, "{mph....>....}");
    mvwprintz(w, 3, 38, col_vel, "%4d", veh->velocity / 100);
    mvwprintz(w, 3, 43, c_ltgreen, "%4d", veh->cruise_velocity / 100);
   }
  } else {
   if(OPTIONS[OPT_USE_METRIC_SYS]) {
    mvwprintz(w, 3, 33, col_indf1, "  {Km/h....}  ");
    mvwprintz(w, 3, 40, col_vel, "%4d", int(veh->velocity * 0.0161f));
   }
   else {
    mvwprintz(w, 3, 34, col_indf1, "  {mph....}  ");
    mvwprintz(w, 3, 40, col_vel, "%4d", veh->velocity / 100);
   }
  }

  if (veh->velocity != 0) {
   nc_color col_indc = veh->skidding? c_red : c_green;
   int dfm = veh->face.dir() - veh->move.dir();
   mvwprintz(w, 3, 21, col_indc, dfm < 0? "L" : ".");
   mvwprintz(w, 3, 22, col_indc, dfm == 0? "0" : ".");
   mvwprintz(w, 3, 23, col_indc, dfm > 0? "R" : ".");
  }
 } else {  // Not in vehicle
  nc_color col_str = c_white, col_dex = c_white, col_int = c_white,
           col_per = c_white, col_spd = c_white;
  if (str_cur < str_max)
   col_str = c_red;
  if (str_cur > str_max)
   col_str = c_green;
  if (dex_cur < dex_max)
   col_dex = c_red;
  if (dex_cur > dex_max)
   col_dex = c_green;
  if (int_cur < int_max)
   col_int = c_red;
  if (int_cur > int_max)
   col_int = c_green;
  if (per_cur < per_max)
   col_per = c_red;
  if (per_cur > per_max)
   col_per = c_green;
  int spd_cur = current_speed();
  if (current_speed() < 100)
   col_spd = c_red;
  if (current_speed() > 100)
   col_spd = c_green;

  mvwprintz(w, 3, 13, col_str, "Str %s%d", str_cur >= 10 ? "" : " ", str_cur);
  mvwprintz(w, 3, 20, col_dex, "Dex %s%d", dex_cur >= 10 ? "" : " ", dex_cur);
  mvwprintz(w, 3, 27, col_int, "Int %s%d", int_cur >= 10 ? "" : " ", int_cur);
  mvwprintz(w, 3, 34, col_per, "Per %s%d", per_cur >= 10 ? "" : " ", per_cur);
  mvwprintz(w, 3, 41, col_spd, "Spd %s%d", spd_cur >= 10 ? "" : " ", spd_cur);
 }
}

bool player::has_trait(int flag) const
{
 if (flag == PF_NULL) return true;
 return my_traits[flag];
}

bool player::has_mutation(int flag) const
{
 return (flag == PF_NULL) ? true : my_mutations[flag];
}

void player::toggle_trait(int flag)
{
 my_traits[flag] = !my_traits[flag];
 my_mutations[flag] = !my_mutations[flag];
}

bool player::has_bionic(bionic_id b) const
{
 for (const auto& bionic : my_bionics) if (bionic.id == b) return true;
 return false;
}

bool player::has_active_bionic(bionic_id b) const
{
 for (const auto& bionic : my_bionics) if (bionic.id == b) return bionic.powered;
 return false;
}

void player::add_bionic(bionic_id b)
{
 for (int i = 0; i < my_bionics.size(); i++) {
  if (my_bionics[i].id == b)
   return;	// No duplicates!
 }
 char newinv;
 if (my_bionics.size() == 0)
  newinv = 'a';
 else
  newinv = my_bionics[my_bionics.size() - 1].invlet + 1;
 my_bionics.push_back(bionic(b, newinv));
}

void player::charge_power(int amount)
{
 power_level += amount;
 if (power_level > max_power_level)
  power_level = max_power_level;
 if (power_level < 0)
  power_level = 0;
}

void player::power_bionics(game *g)
{
 WINDOW *wBio = newwin(VIEW, SCREEN_WIDTH, 0, 0);
 werase(wBio);
 std::vector <bionic> passive;	// \todo should these two be std::vector<bionic*>?
 std::vector <bionic> active;
 mvwprintz(wBio, 0, 0, c_blue, "BIONICS -");
 mvwprintz(wBio, 0,10, c_white,
           "Activating.  Press '!' to examine your implants.");

 for (int i = 0; i < SCREEN_WIDTH; i++) {
  mvwputch(wBio,  1, i, c_ltgray, LINE_OXOX);
  mvwputch(wBio, 21, i, c_ltgray, LINE_OXOX);
 }
 for (int i = 0; i < my_bionics.size(); i++) {
  const auto& bio_type = bionic::type[my_bionics[i].id];
  if ( bio_type.power_source || !bio_type.activated)
   passive.push_back(my_bionics[i]);
  else
   active.push_back(my_bionics[i]);
 }
 nc_color type;
 if (passive.size() > 0) {
  mvwprintz(wBio, 2, 0, c_ltblue, "Passive:");
  for (int i = 0; i < passive.size(); i++) {
   const auto& bio_type = bionic::type[passive[i].id];
   type = (bio_type.power_source ? c_ltcyan : c_cyan);
   mvwputch(wBio, 3 + i, 0, type, passive[i].invlet);
   mvwprintz(wBio, 3 + i, 2, type, bio_type.name.c_str());
  }
 }
 if (active.size() > 0) {
  mvwprintz(wBio, 2, 32, c_ltblue, "Active:");
  for (int i = 0; i < active.size(); i++) {
   type = (active[i].powered ? c_red : c_ltred);
   const auto& bio_type = bionic::type[active[i].id];
   mvwputch(wBio, 3 + i, 32, type, active[i].invlet);
   mvwprintz(wBio, 3 + i, 34, type,
             (active[i].powered ? "%s - ON" : "%s - %d PU / %d trns"),
             bio_type.name.c_str(),
             bio_type.power_cost,
             bio_type.charge_time);
  }
 }

 wrefresh(wBio);
 char ch;
 bool activating = true;
 do {
  bionic *tmp = NULL;
  int b;
  ch = getch();
  if (ch == '!') {
   activating = !activating;
   if (activating)
    mvwprintz(wBio, 0, 10, c_white, "Activating.  Press '!' to examine your implants.");
   else
    mvwprintz(wBio, 0, 10, c_white, "Examining.  Press '!' to activate your implants.");
  } else if (ch == ' ')
   ch = KEY_ESCAPE;
  else if (ch != KEY_ESCAPE) {
   for (int i = 0; i < my_bionics.size(); i++) {
    if (ch == my_bionics[i].invlet) {
     tmp = &my_bionics[i];
     b = i;
     ch = KEY_ESCAPE;
    }
   }
   if (tmp) {
	const auto& bio_type = bionic::type[tmp->id];
    if (activating) {
     if (bio_type.activated) {
      if (tmp->powered) {
       tmp->powered = false;
       messages.add("%s powered off.", bio_type.name.c_str());
      } else if (power_level >= bio_type.power_cost || (weapon.type->id == itm_bio_claws && tmp->id == bio_claws))
       activate_bionic(b, g);
     } else
      mvwprintz(wBio, 22, 0, c_ltred, "\
You can not activate %s!  To read a description of \
%s, press '!', then '%c'.", bio_type.name.c_str(),
                            bio_type.name.c_str(), tmp->invlet);
    } else {	// Describing bionics, not activating them!
// Clear the lines first
     ch = 0;
     mvwprintz(wBio, 22, 0, c_ltgray, "\
                                                                               \
                                                                               \
                                                                             ");
     mvwprintz(wBio, 22, 0, c_ltblue, bio_type.description.c_str());
    }
   }
  }
  wrefresh(wBio);
 } while (ch != KEY_ESCAPE);
 werase(wBio);
 wrefresh(wBio);
 delwin(wBio);
 erase();
}

int player::sight_range(int light_level) const
{
 int ret = light_level;
 if (((is_wearing(itm_goggles_nv) && has_active_item(itm_UPS_on)) ||
     has_active_bionic(bio_night_vision)) &&
     ret < 12)
  ret = 12;
 if (has_trait(PF_NIGHTVISION) && ret < 12) ret += 1;
 if (has_trait(PF_NIGHTVISION2) && ret < 12) ret += 4;
 if (has_trait(PF_NIGHTVISION3) && ret < 12) ret = 12;
 if (underwater && !has_bionic(bio_membrane) && !has_trait(PF_MEMBRANE) &&
     !is_wearing(itm_goggles_swim))
  ret = 1;
 if (has_disease(DI_BOOMERED)) ret = 1;
 if (has_disease(DI_IN_PIT)) ret = 1;
 if (has_disease(DI_BLIND)) ret = 0;
 if (ret > 4 && has_trait(PF_MYOPIC) && !is_wearing(itm_glasses_eye) &&
     !is_wearing(itm_glasses_monocle))
  ret = 4;
 return ret;
}

int player::overmap_sight_range(int light_level) const
{
 int sight = sight_range(light_level);
 // low-light interferes with overmap sight range
 if (4*SEE >= sight) return (SEE > sight) ? 0 : SEE/2;
 // technology overrides
 if (has_amount(itm_binoculars, 1)) return 20;
 return 10;	// baseline
}

int player::clairvoyance() const
{
 if (has_artifact_with(AEP_CLAIRVOYANCE)) return 3;
 return 0;
}

bool player::has_two_arms() const
{
 if (has_bionic(bio_blaster) || hp_cur[hp_arm_l] < 10 || hp_cur[hp_arm_r] < 10)
  return false;
 return true;
}

bool player::avoid_trap(const trap* const tr) const
{
 int myroll = dice(3, dex_cur + sklevel[sk_dodge] * 1.5);
 int traproll;
 if (per_cur - encumb(bp_eyes) >= tr->visibility)
  traproll = dice(3, tr->avoidance);
 else
  traproll = dice(6, tr->avoidance);
 if (has_trait(PF_LIGHTSTEP)) myroll += dice(2, 6);
 return myroll >= traproll;
}

void player::pause()
{
 moves = 0;
 if (recoil > 0) {
  if (str_cur + 2 * sklevel[sk_gun] >= recoil)
   recoil = 0;
  else {
   recoil -= str_cur + 2 * sklevel[sk_gun];
   recoil = int(recoil / 2);
  }
 }

// Meditation boost for Toad Style
 if (weapon.type->id == itm_style_toad && activity.type == ACT_NULL) {
  int arm_amount = 1 + (int_cur - 6) / 3 + (per_cur - 6) / 3;
  int arm_max = (int_cur + per_cur) / 2;
  if (arm_amount > 3) arm_amount = 3;
  if (arm_max > 20) arm_max = 20;
  add_disease(DI_ARMOR_BOOST, 2, arm_amount, arm_max);
 }
}

int player::throw_range(int index) const
{
 item tmp;
 if (index == -1) tmp = weapon;
 else if (index == -2) return -1;
 else tmp = inv[index];

 if (tmp.weight() > int(str_cur * 15)) return 0;
 int ret = int((str_cur * 8) / (tmp.weight() > 0 ? tmp.weight() : 10));
 ret -= int(tmp.volume() / 10);
 if (ret < 1) return 1;
// Cap at double our strength + skill
 if (ret > str_cur * 1.5 + sklevel[sk_throw]) return str_cur * 1.5 + sklevel[sk_throw];
 return ret;
}
 
int player::ranged_dex_mod(bool real_life) const
{
 int dex = (real_life ? dex_cur : dex_max);
 if (dex == 8) return 0;
 const int dex_delta = 8 - dex;
 if (dex > 8) return (real_life ? (0 - rng(0, -dex_delta)) : dex_delta);

 int deviation = 0;
 if (dex < 6) deviation = 2 * dex_delta;
 else deviation = 3 * dex_delta / 2;

 return (real_life ? rng(0, deviation) : deviation);
}

int player::ranged_per_mod(bool real_life) const
{
 int per = (real_life ? per_cur : per_max);
 if (per == 8) return 0;
 if (16 < per) per = 16;
 const int per_delta = 8 - per;

 int deviation = 0;
 if (8 < per) {
  deviation = 3 * (0 - (per > 16 ? 8 : per - 8));
  if (real_life && one_in(per)) deviation = 0 - rng(0, abs(deviation));
 } else {
  if (per < 4) deviation = 5 * (8 - per);
  else if (per < 6) deviation = 2.5 * (8 - per);
  else /* if (per < 8) */ deviation = 2 * (8 - per);
  if (real_life) deviation = rng(0, deviation);
 }
 return deviation;
}

int player::throw_dex_mod(bool real_life) const
{
 int dex = (real_life ? dex_cur : dex_max);
 if (dex == 8 || dex == 9) return 0;
 if (dex >= 10) return (real_life ? 0 - rng(0, dex - 9) : 9 - dex);
 
 int deviation = 8 - dex;
 if (dex < 4) deviation *= 4;
 else if (dex < 6) deviation *= 3;
 else deviation *= 2;

 return (real_life ? rng(0, deviation) : deviation);
}

int player::comprehension_percent(skill s, bool real_life) const
{
 double intel = (double)(real_life ? int_cur : int_max);
 if (intel == 0.) intel = 1.;
 double percent = 80.; // double temporarily, since we divide a lot
 int learned = (real_life ? sklevel[s] : 4);
 if (learned > intel / 2) percent /= 1 + ((learned - intel / 2) / (intel / 3));
 else if (!real_life && intel > 8) percent += 125 - 1000 / intel;

 if (has_trait(PF_FASTLEARNER)) percent += 50.;
 return (int)(percent);
}

int player::read_speed(bool real_life) const
{
 int intel = (real_life ? int_cur : int_max);
 int ret = 1000 - 50 * (intel - 8);
 if (has_trait(PF_FASTREADER)) ret *= .8;
 if (ret < 100) ret = 100;
 return (real_life ? ret : ret / 10);
}

int player::talk_skill() const
{
 int ret = int_cur + per_cur + sklevel[sk_speech] * 3;
 if (has_trait(PF_DEFORMED)) ret -= 4;
 else if (has_trait(PF_DEFORMED2)) ret -= 6;

 return ret;
}

int player::intimidation() const
{
 int ret = str_cur * 2;
 if (weapon.is_gun()) ret += 10;
 if (weapon.damage_bash() >= 12 || weapon.damage_cut() >= 12) ret += 5;
 if (has_trait(PF_DEFORMED2)) ret += 3;
 if (stim > 20) ret += 2;
 if (has_disease(DI_DRUNK)) ret -= 4;

 return ret;
}

void player::hit(game *g, body_part bphurt, int side, int dam, int cut)
{
 int painadd = 0;
 if (has_disease(DI_SLEEP)) {
  messages.add("You wake up!");
  rem_disease(DI_SLEEP);
 } else if (has_disease(DI_LYING_DOWN))
  rem_disease(DI_LYING_DOWN);

 absorb(g, bphurt, dam, cut);

 dam += cut;
 if (dam <= 0) return;

 rem_disease(DI_SPEED_BOOST);
 if (dam >= 6) rem_disease(DI_ARMOR_BOOST);

 if (!is_npc()) g->cancel_activity_query("You were hurt!");

 if (has_artifact_with(AEP_SNAKES) && dam >= 6) {
  int snakes = int(dam / 6);
  std::vector<point> valid;
  for (int x = pos.x - 1; x <= pos.x + 1; x++) {
   for (int y = pos.y - 1; y <= pos.y + 1; y++) {
    if (g->is_empty(x, y)) valid.push_back( point(x, y) );
   }
  }
  if (snakes > valid.size()) snakes = valid.size();
  if (snakes == 1) messages.add("A snake sprouts from your body!");
  else if (snakes >= 2) messages.add("Some snakes sprout from your body!");
  monster snake(mtype::types[mon_shadow_snake]);
  for (int i = 0; i < snakes; i++) {
   int index = rng(0, valid.size() - 1);
   point sp = valid[index];
   valid.erase(valid.begin() + index);
   snake.spawn(sp);
   snake.friendly = -1;
   g->z.push_back(snake);
  }
 }
  
 if (has_trait(PF_PAINRESIST))
  painadd = (sqrt(double(cut)) + dam + cut) / (rng(4, 6));
 else
  painadd = (sqrt(double(cut)) + dam + cut) / 4;
 pain += painadd;

 switch (bphurt) {
 case bp_eyes:
  pain++;
  if (dam > 5 || cut > 0) {
   int minblind = int((dam + cut) / 10);
   if (minblind < 1) minblind = 1;
   int maxblind = int((dam + cut) /  4);
   if (maxblind > 5) maxblind = 5;
   add_disease(DI_BLIND, rng(minblind, maxblind));
  }

 case bp_mouth: // Fall through to head damage
 case bp_head: 
  pain++;
  hp_cur[hp_head] -= dam;
  if (hp_cur[hp_head] < 0) hp_cur[hp_head] = 0;
 break;
 case bp_torso:
  recoil += int(dam / 5);
  hp_cur[hp_torso] -= dam;
  if (hp_cur[hp_torso] < 0) hp_cur[hp_torso] = 0;
 break;
 case bp_hands: // Fall through to arms
 case bp_arms:
  if (side == 1 || side == 3 || weapon.is_two_handed(*this)) recoil += int(dam / 3);
  if (side == 0 || side == 3) {
   hp_cur[hp_arm_l] -= dam;
   if (hp_cur[hp_arm_l] < 0) hp_cur[hp_arm_l] = 0;
  }
  if (side == 1 || side == 3) {
   hp_cur[hp_arm_r] -= dam;
   if (hp_cur[hp_arm_r] < 0) hp_cur[hp_arm_r] = 0;
  }
 break;
 case bp_feet: // Fall through to legs
 case bp_legs:
  if (side == 0 || side == 3) {
   hp_cur[hp_leg_l] -= dam;
   if (hp_cur[hp_leg_l] < 0) hp_cur[hp_leg_l] = 0;
  }
  if (side == 1 || side == 3) {
   hp_cur[hp_leg_r] -= dam;
   if (hp_cur[hp_leg_r] < 0) hp_cur[hp_leg_r] = 0;
  }
 break;
 default:
  debugmsg("Wacky body part hit!");
 }
 if (has_trait(PF_ADRENALINE) && !has_disease(DI_ADRENALINE) &&
     (hp_cur[hp_head] < 25 || hp_cur[hp_torso] < 15))
  add_disease(DI_ADRENALINE, 200);
}

void player::hurt(game *g, body_part bphurt, int side, int dam)
{
 if (has_disease(DI_SLEEP) && rng(0, dam) > 2) {
  messages.add("You wake up!");
  rem_disease(DI_SLEEP);
 } else if (has_disease(DI_LYING_DOWN))
  rem_disease(DI_LYING_DOWN);

 if (dam <= 0) return;

 if (!is_npc()) g->cancel_activity_query("You were hurt!");

 int painadd = dam / (has_trait(PF_PAINRESIST) ? 3 : 2);
 pain += painadd;

 switch (bphurt) {
 case bp_eyes:	// Fall through to head damage
 case bp_mouth:	// Fall through to head damage
 case bp_head:
  pain++;
  if (0 > (hp_cur[hp_head] -= dam)) hp_cur[hp_head] = 0;
 break;
 case bp_torso:
  if (0 > (hp_cur[hp_torso] -= dam)) hp_cur[hp_torso] = 0;
 break;
 case bp_hands:	// Fall through to arms
 case bp_arms:
  if (side == 0 || side == 3) {
   if (0 > (hp_cur[hp_arm_l] -= dam)) hp_cur[hp_arm_l] = 0;
  }
  if (side == 1 || side == 3) {
   if (0 > (hp_cur[hp_arm_r] -= dam)) hp_cur[hp_arm_r] = 0;
  }
 break;
 case bp_feet:	// Fall through to legs
 case bp_legs:
  if (side == 0 || side == 3) {
   if (0 > (hp_cur[hp_leg_l] -= dam)) hp_cur[hp_leg_l] = 0;
  }
  if (side == 1 || side == 3) {
   if (0 > (hp_cur[hp_leg_r] -= dam)) hp_cur[hp_leg_r] = 0;
  }
 break;
 default:
  debugmsg("Wacky body part hurt!");
 }
 if (has_trait(PF_ADRENALINE) && !has_disease(DI_ADRENALINE) && (hp_cur[hp_head] < 25 || hp_cur[hp_torso] < 15))
  add_disease(DI_ADRENALINE, 200);
}

void player::heal(body_part healed, int side, int dam)
{
 hp_part healpart;
 switch (healed) {
 case bp_eyes:	// Fall through to head damage
 case bp_mouth:	// Fall through to head damage
 case bp_head:
  healpart = hp_head;
 break;
 case bp_torso:
  healpart = hp_torso;
 break;
 case bp_hands:
// Shouldn't happen, but fall through to arms
  debugmsg("Heal against hands!");
 case bp_arms:
  if (side == 0)
   healpart = hp_arm_l;
  else
   healpart = hp_arm_r;
 break;
 case bp_feet:
// Shouldn't happen, but fall through to legs
  debugmsg("Heal against feet!");
 case bp_legs:
  if (side == 0)
   healpart = hp_leg_l;
  else
   healpart = hp_leg_r;
 break;
 default:
  debugmsg("Wacky body part healed!");
  healpart = hp_torso;
 }
 hp_cur[healpart] += dam;
 if (hp_cur[healpart] > hp_max[healpart])
  hp_cur[healpart] = hp_max[healpart];
}

void player::heal(hp_part healed, int dam)
{
 hp_cur[healed] += dam;
 if (hp_cur[healed] > hp_max[healed])
  hp_cur[healed] = hp_max[healed];
}

void player::healall(int dam)
{
 for (int i = 0; i < num_hp_parts; i++) {
  if (hp_cur[i] > 0) {
   hp_cur[i] += dam;
   if (hp_cur[i] > hp_max[i])
    hp_cur[i] = hp_max[i];
  }
 }
}

void player::hurtall(int dam)
{
 for (int i = 0; i < num_hp_parts; i++) {
  int painadd = 0;
  hp_cur[i] -= dam;
   if (hp_cur[i] < 0)
     hp_cur[i] = 0;
  if (has_trait(PF_PAINRESIST))
   painadd = dam / 3;
  else
   painadd = dam / 2;
  pain += painadd;
 }
}

void player::hitall(game *g, int dam, int vary)
{
 if (has_disease(DI_SLEEP)) {
  messages.add("You wake up!");
  rem_disease(DI_SLEEP);
 } else if (has_disease(DI_LYING_DOWN))
  rem_disease(DI_LYING_DOWN);

 for (int i = 0; i < num_hp_parts; i++) {
  int ddam = vary? dam * rng (100 - vary, 100) / 100 : dam;
  int cut = 0;
  absorb(g, (body_part) i, ddam, cut);
  int painadd = 0;
  hp_cur[i] -= ddam;
   if (hp_cur[i] < 0)
     hp_cur[i] = 0;
  if (has_trait(PF_PAINRESIST))
   painadd = dam / 3 / 4;
  else
   painadd = dam / 2 / 4;
  pain += painadd;
 }
}

void player::knock_back_from(game *g, int x, int y)
{
 if (x == pos.x && y == pos.y) return; // No effect
 point to(pos);
 if (x < pos.x) to.x++;
 else if (x > pos.x) to.x--;
 if (y < pos.y) to.y++;
 else if (y > pos.y) to.y--;

 bool u_see = (!is_npc() || g->u_see(to));

 std::string You = (is_npc() ? name : "You");
 const char* const s = (is_npc() ? "s" : "");

// First, see if we hit a monster
 if (monster* const z = g->mon(to)) {
  hit(g, bp_torso, 0, z->type->size, 0);
  add_disease(DI_STUNNED, 1);
  if ((str_max - 6) / 4 > z->type->size) {
   z->knock_back_from(g, pos); // Chain reaction!
   z->hurt((str_max - 6) / 4);
   z->add_effect(ME_STUNNED, 1);
  } else if ((str_max - 6) / 4 == z->type->size) {
   z->hurt((str_max - 6) / 4);
   z->add_effect(ME_STUNNED, 1);
  }

  if (u_see) messages.add("%s bounce%s off a %s!", You.c_str(), s, z->name().c_str());

  return;
 }

 if (npc* const p = g->nPC(to)) {
  hit(g, bp_torso, 0, 3, 0);
  add_disease(DI_STUNNED, 1);
  p->hit(g, bp_torso, 0, 3, 0);
  if (u_see) messages.add("%s bounce%s off %s!", You.c_str(), s, p->name.c_str());
  return;
 }

// If we're still in the function at this point, we're actually moving a tile!
 if (g->m.move_cost(to) == 0) { // Wait, it's a wall (or water)

  if (g->m.has_flag(swimmable, to)) {
   if (!is_npc())
    g->plswim(to.x, to.y);
// TODO: NPCs can't swim!
  } else { // It's some kind of wall.
   hurt(g, bp_torso, 0, 3);
   add_disease(DI_STUNNED, 2);
   if (u_see) messages.add("%s bounce%s off a %s.", name.c_str(), s, g->m.tername(to).c_str());
  }

 } else pos == to;	// It's no wall	
}

int player::hp_percentage() const
{
 int total_cur = 0, total_max = 0;
// Head and torso HP are weighted 3x and 2x, respectively
 total_cur = hp_cur[hp_head] * 3 + hp_cur[hp_torso] * 2;
 total_max = hp_max[hp_head] * 3 + hp_max[hp_torso] * 2;
 for (int i = hp_arm_l; i < num_hp_parts; i++) {
  total_cur += hp_cur[i];
  total_max += hp_max[i];
 }
 return (100 * total_cur) / total_max;
}

void player::get_sick()
{
 if (health > 0 && rng(0, health + 10) < health) health--;
 if (health < 0 && rng(0, 10 - health) < (0 - health)) health++;
 if (one_in(12)) health -= 1;

 if (game::debugmon) debugmsg("Health: %d", health);

 if (has_trait(PF_DISIMMUNE)) return;

 if (!has_disease(DI_FLU) && !has_disease(DI_COMMON_COLD) &&
     one_in(900 + 10 * health + (has_trait(PF_DISRESISTANT) ? 300 : 0))) {
  if (one_in(6))
   infect(DI_FLU, bp_mouth, 3, rng(40000, 80000));
  else
   infect(DI_COMMON_COLD, bp_mouth, 3, rng(20000, 60000));
 }
}

void player::infect(dis_type type, body_part vector, int strength, int duration)
{
 if (dice(strength, 3) > dice(resist(vector), 3)) add_disease(type, duration);
}

// 2nd person singular.  Would need 3rd person for npcs
const char* describe(dis_type type)
{
	switch (type) {
	case DI_GLARE: return "The sunlight's glare makes it hard to see.";
	case DI_WET: return "You're getting soaked!";
	case DI_HEATSTROKE: "You have heatstroke!";
	case DI_FBFACE: return "Your face is frostbitten.";
	case DI_FBHANDS: return "Your hands are frostbitten.";
	case DI_FBFEET: return "Your feet are frostbitten.";
	case DI_COMMON_COLD: "You feel a cold coming on...";
	case DI_FLU: return "You feel a flu coming on...";
	case DI_ONFIRE: return "You're on fire!";
	case DI_SMOKE: return "You inhale a lungful of thick smoke.";
	case DI_TEARGAS: return "You inhale a lungful of tear gas.";
	case DI_BOOMERED: return "You're covered in bile!";
	case DI_SAP: return "You're coated in sap!";
	case DI_SPORES: return "You're covered in tiny spores!";
	case DI_SLIMED: return "You're covered in thick goo!";
	case DI_LYING_DOWN: return "You lie down to go to sleep...";
	case DI_FORMICATION: return "There's bugs crawling under your skin!";
	case DI_WEBBED: return "You're covered in webs!";
	case DI_DRUNK:
	case DI_HIGH: return "You feel lightheaded.";
	case DI_ADRENALINE: return "You feel a surge of adrenaline!";
	case DI_ASTHMA: return "You can't breathe... asthma attack!";
	case DI_DEAF: return "You're deafened!";
	case DI_BLIND: return "You're blinded!";
	case DI_STUNNED: return "You're stunned!";
	case DI_DOWNED: return "You're knocked to the floor!";
	case DI_AMIGARA: return "You can't look away from the fautline...";
	default: return NULL;
	}
}

void player::add_disease(dis_type type, int duration, int intensity, int max_intensity)
{
 if (duration == 0) return;
 int i = 0;
 for(auto& ill : illness) {
  if (type != ill.type) continue;
  illness[i].duration += duration;
  illness[i].intensity += intensity;
  if (max_intensity != -1 && illness[i].intensity > max_intensity) illness[i].intensity = max_intensity;
  return;
 }
 if (!is_npc()) {
	if (DI_ADRENALINE == type) moves += 800;	// V 0.2.1: handle NPC adrenaline
	messages.add(describe(type));
 }
 disease tmp(type, duration, intensity);
 illness.push_back(tmp);
}

void player::rem_disease(dis_type type)
{
 for (int i = 0; i < illness.size(); i++) {
  if (illness[i].type == type) illness.erase(illness.begin() + i);
 }
}

bool player::has_disease(dis_type type) const
{
 for (int i = 0; i < illness.size(); i++) {
  if (illness[i].type == type) return true;
 }
 return false;
}

int player::disease_level(dis_type type) const
{
 for (int i = 0; i < illness.size(); i++) {
  if (illness[i].type == type) return illness[i].duration;
 }
 return 0;
}

int player::disease_intensity(dis_type type) const
{
 for (int i = 0; i < illness.size(); i++) {
  if (illness[i].type == type) return illness[i].intensity;
 }
 return 0;
}

void player::add_addiction(add_type type, int strength)
{
 if (type == ADD_NULL) return;
 int timer = 1200;
 if (has_trait(PF_ADDICTIVE)) {
  strength = int(strength * 1.5);
  timer = 800;
 }
 for(auto& a : addictions) {
  if (type != a.type) continue;
  if (a.sated <   0) a.sated = timer;
  else if (a.sated < 600) a.sated += timer;	// TODO: Make this variable?
  else a.sated += int((3000 - a.sated) / 2);	// definitely could be longer than above
  if ((rng(0, strength) > rng(0, a.intensity * 5) || rng(0, 500) < strength) && a.intensity < 20)
   a.intensity++;
  return;
 }
 if (rng(0, 100) < strength) addictions.push_back(addiction(type, 1));
}

bool player::has_addiction(add_type type) const
{
 for(const auto& a : addictions) if (a.type == type && a.intensity >= MIN_ADDICTION_LEVEL) return true;
 return false;
}

void player::rem_addiction(add_type type)
{
 for (int i = 0; i < addictions.size(); i++) {
  if (addictions[i].type == type) {
   addictions.erase(addictions.begin() + i);
   return;
  }
 }
}

int player::addiction_level(add_type type) const
{
 for(const auto& a : addictions) if (a.type == type) return a.intensity;
 return 0;
}

void player::suffer(game *g)
{
 for (int i = 0; i < my_bionics.size(); i++) {
  if (my_bionics[i].powered) activate_bionic(i, g);
 }
 if (underwater) {
  if (!has_trait(PF_GILLS)) oxygen--;
  if (oxygen < 0) {
   if (has_bionic(bio_gills) && power_level > 0) {
    oxygen += 5;
    power_level--;
   } else {
    messages.add("You're drowning!");
    hurt(g, bp_torso, 0, rng(1, 4));
   }
  }
 }
 for (int i = 0; i < illness.size(); i++) {
  dis_effect(g, *this, illness[i]);
  illness[i].duration--;
  if (illness[i].duration < MIN_DISEASE_AGE)// Cap permanent disease age
   illness[i].duration = MIN_DISEASE_AGE;
  if (illness[i].duration == 0) {
   illness.erase(illness.begin() + i);
   i--;
  }
 }
 if (!has_disease(DI_SLEEP)) {
  int timer = has_trait(PF_ADDICTIVE) ? -4000 : -3600;
  for (int i = 0; i < addictions.size(); i++) {
   if (addictions[i].sated <= 0 && addictions[i].intensity >= MIN_ADDICTION_LEVEL)
    addict_effect(g, addictions[i]);
   addictions[i].sated--;
   if (!one_in(addictions[i].intensity - 2) && addictions[i].sated > 0)
    addictions[i].sated -= 1;
   if (addictions[i].sated < timer - (100 * addictions[i].intensity)) {
    if (addictions[i].intensity <= 2) {
     addictions.erase(addictions.begin() + i);
     i--;
    } else {
     addictions[i].intensity = int(addictions[i].intensity / 2);
     addictions[i].intensity--;
     addictions[i].sated = 0;
    }
   }
  }
  if (has_trait(PF_CHEMIMBALANCE)) {
   if (one_in(3600)) {
    messages.add("You suddenly feel sharp pain for no reason.");
    pain += 3 * rng(1, 3);
   }
   if (one_in(3600)) {
    int pkilladd = 5 * rng(-1, 2);
    if (pkilladd > 0)
     messages.add("You suddenly feel numb.");
    else if (pkilladd < 0)
     messages.add("You suddenly ache.");
    pkill += pkilladd;
   }
   if (one_in(3600)) {
    messages.add("You feel dizzy for a moment.");
    moves -= rng(10, 30);
   }
   if (one_in(3600)) {
    int hungadd = 5 * rng(-1, 3);
    if (hungadd > 0)
     messages.add("You suddenly feel hungry.");
    else
     messages.add("You suddenly feel a little full.");
    hunger += hungadd;
   }
   if (one_in(3600)) {
    messages.add("You suddenly feel thirsty.");
    thirst += 5 * rng(1, 3);
   }
   if (one_in(3600)) {
    messages.add("You feel fatigued all of a sudden.");
    fatigue += 10 * rng(2, 4);
   }
   if (one_in(4800)) {
    if (one_in(3))
     add_morale(MORALE_FEELING_GOOD, 20, 100);
    else
     add_morale(MORALE_FEELING_BAD, -20, -100);
   }
  }
  if ((has_trait(PF_SCHIZOPHRENIC) || has_artifact_with(AEP_SCHIZO)) && one_in(2400)) { // Every 4 hours or so
   int i;
   switch(rng(0, 11)) {
    case 0:
     add_disease(DI_HALLU, 3600);
     break;
    case 1:
     add_disease(DI_VISUALS, rng(15, 60));
     break;
    case 2:
     messages.add("From the south you hear glass breaking.");
     break;
    case 3:
     messages.add("YOU SHOULD QUIT THE GAME IMMEDIATELY.");
     add_morale(MORALE_FEELING_BAD, -50, -150);
     break;
    case 4:
     for (i = 0; i < 10; i++) {
      messages.add("XXXXXXXXXXXXXXXXXXXXXXXXXXX");
     }
     break;
    case 5:
     messages.add("You suddenly feel so numb...");
     pkill += 25;
     break;
    case 6:
     messages.add("You start to shake uncontrollably.");
     add_disease(DI_SHAKES, 10 * rng(2, 5));
     break;
    case 7:
     for (i = 0; i < 10; i++) {
      monster phantasm(mtype::types[mon_hallu_zom + rng(0, 3)], pos.x + rng(-10, 10), pos.y + rng(-10, 10));
      if (!g->mon(phantasm.pos)) g->z.push_back(phantasm);
     }
     break;
    case 8:
     messages.add("It's a good time to lie down and sleep.");
     add_disease(DI_LYING_DOWN, 200);
     break;
    case 9:
     messages.add("You have the sudden urge to SCREAM!");
     g->sound(pos, 10 + 2 * str_cur, "AHHHHHHH!");
     break;
    case 10:
     messages.add(std::string(name + name + name + name + name + name + name +
                            name + name + name + name + name + name + name +
                            name + name + name + name + name + name).c_str());
     break;
    case 11:
     add_disease(DI_FORMICATION, 600);
     break;
   }
  }

  if (has_trait(PF_JITTERY) && !has_disease(DI_SHAKES)) {
   if (stim > 50 && one_in(300 - stim)) add_disease(DI_SHAKES, 300 + stim);
   else if (hunger > 80 && one_in(500 - hunger)) add_disease(DI_SHAKES, 400);
  }

  if (has_trait(PF_MOODSWINGS) && one_in(3600)) {
   if (rng(1, 20) > 9)	// 55% chance
    add_morale(MORALE_MOODSWING, -100, -500);
   else			// 45% chance
    add_morale(MORALE_MOODSWING, 100, 500);
  }

  if (has_trait(PF_VOMITOUS) && one_in(4200)) vomit();

  if (has_trait(PF_SHOUT1) && one_in(3600)) g->sound(pos, 10 + 2 * str_cur, "You shout loudly!");
  if (has_trait(PF_SHOUT2) && one_in(2400)) g->sound(pos, 15 + 3 * str_cur, "You scream loudly!");
  if (has_trait(PF_SHOUT3) && one_in(1800)) g->sound(pos, 20 + 4 * str_cur, "You let out a piercing howl!");
 }	// Done with while-awake-only effects

 if (has_trait(PF_ASTHMA) && one_in(3600 - stim * 50)) {
  bool auto_use = has_charges(itm_inhaler, 1);
  if (underwater) {
   oxygen = int(oxygen / 2);
   auto_use = false;
  }
  if (has_disease(DI_SLEEP)) {
   rem_disease(DI_SLEEP);
   messages.add("Your asthma wakes you up!");
   auto_use = false;
  }
  if (auto_use)
   use_charges(itm_inhaler, 1);
  else {
   add_disease(DI_ASTHMA, 50 * rng(1, 4));
   if (!is_npc()) g->cancel_activity_query("You have an asthma attack!");
  }
 }

 if (pain > 0) {
  if (has_trait(PF_PAINREC1) && one_in(600)) pain--;
  if (has_trait(PF_PAINREC2) && one_in(300)) pain--;
  if (has_trait(PF_PAINREC3) && one_in(150)) pain--;
 }

 if (g->is_in_sunlight(pos)) {
  if (has_trait(PF_LEAVES) && one_in(600)) hunger--;

  if (has_trait(PF_ALBINO) && one_in(20)) {
   messages.add("The sunlight burns your skin!");
   if (has_disease(DI_SLEEP)) {
    rem_disease(DI_SLEEP);
    messages.add("You wake up!");
   }
   hurtall(1);
  }

  if ((has_trait(PF_TROGLO) || has_trait(PF_TROGLO2)) && g->weather == WEATHER_SUNNY) {
   str_cur--;
   dex_cur--;
   int_cur--;
   per_cur--;
  }
  if (has_trait(PF_TROGLO2)) {
   str_cur--;
   dex_cur--;
   int_cur--;
   per_cur--;
  }
  if (has_trait(PF_TROGLO3)) {
   str_cur -= 4;
   dex_cur -= 4;
   int_cur -= 4;
   per_cur -= 4;
  }
 }

 if (has_trait(PF_SORES)) {
  for (int i = bp_head; i < num_bp; i++) {
   const int nonstrict_lb = 5 + 4 * abs(encumb(body_part(i)));
   if (pain < nonstrict_lb) pain = nonstrict_lb;
  }
 }

 if (has_trait(PF_SLIMY)) {
  auto& fd = g->m.field_at(pos);
  if (fd.type == fd_null) g->m.add_field(g, pos, fd_slime, 1);
  else if (fd.type == fd_slime && fd.density < 3) fd.density++;
 }

 if (has_trait(PF_WEB_WEAVER) && one_in(3)) {
  auto& fd = g->m.field_at(pos);
  if (fd.type == fd_null) g->m.add_field(g, pos, fd_web, 1);
  else if (fd.type == fd_web && fd.density < 3) fd.density++;
 }

 if (has_trait(PF_RADIOGENIC) && int(messages.turn) % 50 == 0 && radiation >= 10) {
  radiation -= 10;
  healall(1);
 }

 if (has_trait(PF_RADIOACTIVE1)) {
  auto& rad = g->m.radiation(pos.x, pos.y);
  if (rad < 10 && one_in(50)) rad++;
 }
 if (has_trait(PF_RADIOACTIVE2)) {
  auto& rad = g->m.radiation(pos.x, pos.y);
  if (rad < 20 && one_in(25)) rad++;
 }
 if (has_trait(PF_RADIOACTIVE3)) {
  auto& rad = g->m.radiation(pos.x, pos.y);
  if (rad < 30 && one_in(10)) rad++;
 }

 if (has_trait(PF_UNSTABLE) && one_in(28800)) mutate(g);	// Average once per 2 days
 if (has_artifact_with(AEP_MUTAGENIC) && one_in(28800)) mutate(g);
 if (has_artifact_with(AEP_FORCE_TELEPORT) && one_in(600)) g->teleport(this);

 const auto rad = g->m.radiation(pos.x, pos.y);
 if (is_wearing(itm_hazmat_suit)) {
  if (radiation < int((100 * rad) / 20))
   radiation += rng(0, rad / 20);
 } else if (radiation < int((100 * rad) / 8))
  radiation += rng(0, rad / 8);

 if (rng(1, 2500) < radiation && (int(messages.turn) % 150 == 0 || radiation > 2000)){
  mutate(g);
  if (radiation > 2000) radiation = 2000;
  radiation /= 2;
  radiation -= 5;
  if (radiation < 0) radiation = 0;
 }

// Negative bionics effects
 if (has_bionic(bio_dis_shock) && one_in(1200)) {
  messages.add("You suffer a painful electrical discharge!");
  pain++;
  moves -= 150;
 }
 if (has_bionic(bio_dis_acid) && one_in(1500)) {
  messages.add("You suffer a burning acidic discharge!");
  hurtall(1);
 }
 if (has_bionic(bio_drain) && power_level > 0 && one_in(600)) {
  messages.add("Your batteries discharge slightly.");
  power_level--;
 }
 if (has_bionic(bio_noise) && one_in(500)) {
  messages.add("A bionic emits a crackle of noise!");
  g->sound(pos, 60, "");
 }
 if (has_bionic(bio_power_weakness) && max_power_level > 0 && power_level >= max_power_level * .75)
  str_cur -= 3;

// Artifact effects
 if (has_artifact_with(AEP_ATTENTION)) add_disease(DI_ATTENTION, 3);

 if (dex_cur < 0) dex_cur = 0;
 if (str_cur < 0) str_cur = 0;
 if (per_cur < 0) per_cur = 0;
 if (int_cur < 0) int_cur = 0;
}

void player::vomit()
{
 messages.add("You throw up heavily!");
 hunger += rng(30, 50);
 thirst += rng(30, 50);
 moves -= 100;
 int i = illness.size();
 while (0 <= --i) {
  auto& ill = illness[i];
  if (DI_FOODPOISON == ill.type) {
   if (0 > (ill.duration -= 300)) rem_disease(DI_FOODPOISON);
  } else if (DI_DRUNK == ill.type) {
   if (0 > (ill.duration -= rng(1, 5) * 100)) rem_disease(DI_DRUNK);
  }
 }
 rem_disease(DI_PKILL1);
 rem_disease(DI_PKILL2);
 rem_disease(DI_PKILL3);
 rem_disease(DI_SLEEP);
}

int player::weight_carried() const
{
 int ret = 0;
 ret += weapon.weight();
 for (const auto& it : worn) ret += it.weight();
 for (size_t i = 0; i < inv.size(); i++) {
  for (int j = 0; j < inv.stack_at(i).size(); j++)
   ret += inv.stack_at(i)[j].weight();
 }
 return ret;
}

int player::volume_carried() const
{
 int ret = 0;
 for (size_t i = 0; i < inv.size(); i++) {
  for (const auto& it : inv.stack_at(i)) ret += it.volume();
 }
 return ret;
}

int player::weight_capacity(bool real_life) const
{
 int str = (real_life ? str_cur : str_max);
 int ret = 400 + str * 35;
 if (has_trait(PF_BADBACK)) ret = int(ret * .65);
 if (has_trait(PF_LIGHT_BONES)) ret = int(ret * .80);
 if (has_trait(PF_HOLLOW_BONES)) ret = int(ret * .60);
 if (has_artifact_with(AEP_CARRY_MORE)) ret += 200;
 return ret;
}

int player::volume_capacity() const
{
 int ret = 2;	// A small bonus (the overflow)
 for (const auto& it : worn) {
  ret += dynamic_cast<const it_armor*>(it.type)->storage;
 }
 if (has_bionic(bio_storage)) ret += 6;
 if (has_trait(PF_SHELL)) ret += 16;
 if (has_trait(PF_PACKMULE)) ret = int(ret * 1.4);
 return ret;
}

int player::morale_level() const
{
 int ret = 0;
 for (const auto& tmp : morale) ret += tmp.bonus;

 if (has_trait(PF_HOARDER)) {
  int pen = int((volume_capacity()-volume_carried()) / 2);
  if (pen > 70) pen = 70;
  if (has_disease(DI_TOOK_XANAX)) pen = int(pen / 7);
  else if (has_disease(DI_TOOK_PROZAC)) pen = int(pen / 2);
  ret -= pen;
 }

 if (has_trait(PF_MASOCHIST)) {
  int bonus = pain / 2.5;
  if (bonus > 25) bonus = 25;
  if (has_disease(DI_TOOK_PROZAC)) bonus = int(bonus / 3);
  ret += bonus;
 }

 if (has_trait(PF_OPTIMISTIC)) {
  if (ret < 0) {	// Up to -30 is canceled out
   ret += 30;
   if (ret > 0) ret = 0;
  } else		// Otherwise, we're just extra-happy
   ret += 20;
 }

 if (has_disease(DI_TOOK_PROZAC) && ret < 0) ret = int(ret / 4);

 return ret;
}

void player::add_morale(morale_type type, int bonus, int max_bonus, const itype* item_type)
{
 bool placed = false;

 for (auto& tmp : morale) {
  if (tmp.type == type && tmp.item_type == item_type) {
   if (abs(tmp.bonus) < abs(max_bonus) || max_bonus == 0) {
    tmp.bonus += bonus;
    if (abs(tmp.bonus) > abs(max_bonus) && max_bonus != 0)
     tmp.bonus = max_bonus;
   }
   placed = true;
   break;
  }
 }
 if (!placed) { // Didn't increase an existing point, so add a new one
  morale_point tmp(type, item_type, bonus);
  morale.push_back(tmp);
 }
}
 
void player::sort_inv()
{
 // guns ammo weaps armor food tools books other
 std::vector< std::vector<item> > types[8];
 std::vector<item> tmp;
 for (size_t i = 0; i < inv.size(); i++) {
  const auto& tmp = inv.stack_at(i);
  if (tmp[0].is_gun()) types[0].push_back(tmp);
  else if (tmp[0].is_ammo()) types[1].push_back(tmp);
  else if (tmp[0].is_armor()) types[3].push_back(tmp);
  else if (tmp[0].is_tool() || tmp[0].is_gunmod()) types[5].push_back(tmp);
  else if (tmp[0].is_food() || tmp[0].is_food_container()) types[4].push_back(tmp);
  else if (tmp[0].is_book()) types[6].push_back(tmp);
  else if (tmp[0].is_weap()) types[2].push_back(tmp);
  else types[7].push_back(tmp);
 }
 inv.clear();
 for (const auto& stack : types) {
  for(const auto& it : stack) inv.push_back(it);
 }
 inv_sorted = true;
}

void player::i_add(item it)
{
 last_item = itype_id(it.type->id);
 if (it.is_food() || it.is_ammo() || it.is_gun()  || it.is_armor() || 
     it.is_book() || it.is_tool() || it.is_weap() || it.is_food_container())
  inv_sorted = false;
 if (it.is_ammo()) {	// Possibly combine with other ammo
  for (size_t i = 0; i < inv.size(); i++) {
   auto& obj = inv[i];
   if (obj.type->id != it.type->id) continue;
   const it_ammo* const ammo = dynamic_cast<const it_ammo*>(obj.type);
   if (obj.charges < ammo->count) {
	obj.charges += it.charges;
    if (obj.charges > ammo->count) {
     it.charges = obj.charges - ammo->count;
	 obj.charges = ammo->count;
    } else it.charges = 0;	// requires full working copy to be valid
   }
  }
  if (it.charges > 0) inv.push_back(it);
  return;
 }
 if (it.is_artifact() && it.is_tool()) {
  game::add_artifact_messages(dynamic_cast<const it_artifact_tool*>(it.type)->effects_carried);
 }
 inv.push_back(it);
}

bool player::has_active_item(itype_id id) const
{
 if (weapon.type->id == id && weapon.active) return true;
 for (size_t i = 0; i < inv.size(); i++) {
  if (inv[i].type->id == id && inv[i].active) return true;
 }
 return false;
}

int player::active_item_charges(itype_id id) const
{
 int max = 0;
 if (weapon.type->id == id && weapon.active) max = weapon.charges;
 for (size_t i = 0; i < inv.size(); i++) {
  for (int j = 0; j < inv.stack_at(i).size(); j++) {
   if (inv.stack_at(i)[j].type->id == id && inv.stack_at(i)[j].active &&
       inv.stack_at(i)[j].charges > max)
    max = inv.stack_at(i)[j].charges;
  }
 }
 return max;
}

void player::process_active_items(game *g)
{
 const it_tool* tmp;
 if (weapon.is_artifact() && weapon.is_tool()) g->process_artifact(&weapon, this, true);
 else if (weapon.active) {
  if (weapon.has_flag(IF_CHARGE)) { // We're chargin it up!
   if (weapon.charges == 8) {
    bool maintain = false;
    if (has_charges(itm_UPS_on, 4)) {
     use_charges(itm_UPS_on, 4);
     maintain = true;
    } else if (has_charges(itm_UPS_off, 4)) {
     use_charges(itm_UPS_off, 4);
     maintain = true;
    }
    if (maintain) {
     if (one_in(20)) {
      messages.add("Your %s discharges!", weapon.tname().c_str());
      point target(pos.x + rng(-12, 12), pos.y + rng(-12, 12));
      std::vector<point> traj = line_to(pos, target, 0);
      g->fire(*this, target, traj, false);
     } else
      messages.add("Your %s beeps alarmingly.", weapon.tname().c_str());
    }
   } else {
    if (has_charges(itm_UPS_on, 1 + weapon.charges)) {
     use_charges(itm_UPS_on, 1 + weapon.charges);
     weapon.poison++;
    } else if (has_charges(itm_UPS_off, 1 + weapon.charges)) {
     use_charges(itm_UPS_off, 1 + weapon.charges);
     weapon.poison++;
    } else {
     messages.add("Your %s spins down.", weapon.tname().c_str());
     if (weapon.poison <= 0) {
      weapon.charges--;
      weapon.poison = weapon.charges - 1;
     } else weapon.poison--;
     if (weapon.charges == 0) weapon.active = false;
    }
    if (weapon.poison >= weapon.charges) {
     weapon.charges++;
     weapon.poison = 0;
    }
   }
   return;
  } // if (weapon.has_flag(IF_CHARGE))

     
  if (!weapon.is_tool()) {
   debugmsg("%s is active, but it is not a tool.", weapon.tname().c_str());
   return;
  }
  tmp = dynamic_cast<const it_tool*>(weapon.type);
  (*tmp->use)(g, this, &weapon, true);
  if (tmp->turns_per_charge > 0 && int(messages.turn) % tmp->turns_per_charge == 0)
   weapon.charges--;
  if (weapon.charges <= 0) {
   (*tmp->use)(g, this, &weapon, false);
   if (tmp->revert_to == itm_null)
    weapon = item::null;
   else
    weapon.type = item::types[tmp->revert_to];
  }
 }
 for (size_t i = 0; i < inv.size(); i++) {
  for (int j = 0; j < inv.stack_at(i).size(); j++) {	// XXX usage is weird here \todo analyze
   item *tmp_it = &(inv.stack_at(i)[j]);
   if (tmp_it->is_artifact() && tmp_it->is_tool())
    g->process_artifact(tmp_it, this);
   if (tmp_it->active) {
    tmp = dynamic_cast<const it_tool*>(tmp_it->type);
    (*tmp->use)(g, this, tmp_it, true);
    if (tmp->turns_per_charge > 0 && int(messages.turn) % tmp->turns_per_charge == 0)
    tmp_it->charges--;
    if (tmp_it->charges <= 0) {
     (*tmp->use)(g, this, tmp_it, false);
     if (tmp->revert_to == itm_null) {
      if (inv.stack_at(i).size() == 1) {
       inv.remove_stack(i);
       i--;
       j = 0;
      } else {
       inv.stack_at(i).erase(inv.stack_at(i).begin() + j);
       j--;
      }
     } else
      tmp_it->type = item::types[tmp->revert_to];
    }
   }
  }
 }
 for (auto& it : worn) {
  if (it.is_artifact()) g->process_artifact(&it, this);
 }
}

item player::remove_weapon()
{
 item tmp = weapon;
 weapon = item::null;
// We need to remove any boosts related to our style
 rem_disease(DI_ATTACK_BOOST);
 rem_disease(DI_DODGE_BOOST);
 rem_disease(DI_DAMAGE_BOOST);
 rem_disease(DI_SPEED_BOOST);
 rem_disease(DI_ARMOR_BOOST);
 rem_disease(DI_VIPER_COMBO);
 return tmp;
}

void player::remove_mission_items(int mission_id)
{
 if (mission_id == -1) return;
 if (weapon.mission_id == mission_id)
  remove_weapon();
 else {
  for (int i = 0; i < weapon.contents.size(); i++) {
   if (weapon.contents[i].mission_id == mission_id)
    remove_weapon();
  }
 }
 for (size_t i = 0; i < inv.size(); i++) {
  for (int j = 0; j < inv.stack_at(i).size() && j > 0; j++) {
   if (inv.stack_at(i)[j].mission_id == mission_id) {
    if (inv.stack_at(i).size() == 1) {
     inv.remove_item(i, j);
     i--;
     j = -999;
    } else {
     inv.remove_item(i, j);
     j--;
    }
   } else {
    bool rem = false;
    for (int k = 0; !rem && k < inv.stack_at(i)[j].contents.size(); k++) {
     if (inv.stack_at(i)[j].contents[k].mission_id == mission_id) {
      if (inv.stack_at(i).size() == 1) {
       inv.remove_item(i, j);
       i--;
       j = 0;
      } else {
       inv.remove_item(i, j);
       j--;
      }
      rem = true;
     }
    }
   }
  }
 }
}

item player::i_rem(char let)
{
 item tmp;
 if (weapon.invlet == let) {
  if (weapon.type->id > num_items && weapon.type->id < num_all_items)
   return item::null;
  tmp = weapon;
  weapon = item::null;
  return tmp;
 }
 for (int i = 0; i < worn.size(); i++) {
  if (worn[i].invlet == let) {
   tmp = worn[i];
   worn.erase(worn.begin() + i);
   return tmp;
  }
 }
 if (inv.index_by_letter(let) != -1) return inv.remove_item_by_letter(let);
 return item::null;
}

item player::i_rem(itype_id type)
{
 if (weapon.type->id == type) return remove_weapon();
 for (size_t i = 0; i < inv.size(); i++) {
  if (inv[i].type->id == type) return inv.remove_item(i);
 }
 return item::null;
}

item& player::i_at(char let)
{
 if (let == KEY_ESCAPE) return (discard<item>::x = item::null);
 if (weapon.invlet == let) return weapon;
 for (auto& it : worn) {
  if (it.invlet == let) return it;
 }
 int index = inv.index_by_letter(let);
 if (index == -1) return (discard<item>::x = item::null);
 return inv[index];
}

item& player::i_of_type(itype_id type)
{
 if (weapon.type->id == type) return weapon;
 for(auto& it : worn) {
  if (it.type->id == type) return it;
 }
 for (size_t i = 0; i < inv.size(); i++) {
  if (inv[i].type->id == type) return inv[i];
 }
 return (discard<item>::x = item::null);
}

std::vector<item> player::inv_dump() const
{
 std::vector<item> ret;
 if (weapon.type->id != 0 && weapon.type->id < num_items) ret.push_back(weapon);
 for(const auto& it : worn) ret.push_back(it);
 for(size_t i = 0; i < inv.size(); i++) {
  for (const auto& it : inv.stack_at(i)) ret.push_back(it);
 }
 return ret;
}

item player::i_remn(int index)
{
 if (index > inv.size() || index < 0) return item::null;
 return inv.remove_item(index);
}

void player::use_amount(itype_id it, int quantity, bool use_container)
{
 bool used_weapon_contents = false;
 for (int i = 0; i < weapon.contents.size(); i++) {
  if (weapon.contents[0].type->id == it) {
   quantity--;
   weapon.contents.erase(weapon.contents.begin() + 0);
   i--;
   used_weapon_contents = true;
  }
 }
 if (use_container && used_weapon_contents)
  remove_weapon();

 if (weapon.type->id == it) {
  quantity--;
  remove_weapon();
 }

 inv.use_amount(it, quantity, use_container);
}

void player::use_charges(itype_id it, int quantity)
{
 if (it == itm_toolset) {
  power_level -= quantity;
  if (power_level < 0)
   power_level = 0;
  return;
 }
// Start by checking weapon contents
 for (int i = 0; i < weapon.contents.size(); i++) {
  if (weapon.contents[i].type->id == it) {
   if (weapon.contents[i].charges > 0 &&
       weapon.contents[i].charges <= quantity) {
    quantity -= weapon.contents[i].charges;
    if (weapon.contents[i].destroyed_at_zero_charges()) {
     weapon.contents.erase(weapon.contents.begin() + i);
     i--;
    } else
     weapon.contents[i].charges = 0;
    if (quantity == 0)
     return;
   } else {
    weapon.contents[i].charges -= quantity;
    return;
   }
  }
 }
  
 if (weapon.type->id == it) {
  if (weapon.charges > 0 && weapon.charges <= quantity) {
   quantity -= weapon.charges;
   if (weapon.destroyed_at_zero_charges())
    remove_weapon();
   else
    weapon.charges = 0;
   if (quantity == 0) return;
   } else {
    weapon.charges -= quantity;
    return;
   }
  }

 inv.use_charges(it, quantity);
}

int player::butcher_factor() const
{
 int lowest_factor = 999;
 for (size_t i = 0; i < inv.size(); i++) {
  for (int j = 0; j < inv.stack_at(i).size(); j++) {
   const item *cur_item = &(inv.stack_at(i)[j]);
   if (cur_item->damage_cut() >= 10 && !cur_item->has_flag(IF_SPEAR)) {
    int factor = cur_item->volume() * 5 - cur_item->weight() * 1.5 - cur_item->damage_cut();
    if (cur_item->damage_cut() <= 20) factor *= 2;
    if (factor < lowest_factor) lowest_factor = factor;
   }
  }
 }
 if (weapon.damage_cut() >= 10 && !weapon.has_flag(IF_SPEAR)) {
  int factor = weapon.volume() * 5 - weapon.weight() * 1.5 - weapon.damage_cut();
  if (weapon.damage_cut() <= 20) factor *= 2;
  if (factor < lowest_factor) lowest_factor = factor;
 }
 return lowest_factor;
}

int player::pick_usb() const
{
 std::vector<int> drives;
 for (size_t i = 0; i < inv.size(); i++) {
  if (inv[i].type->id == itm_usb_drive) {
   if (inv[i].contents.empty()) return i; // No need to pick, use an empty one by default!
   drives.push_back(i);
  }
 }

 if (drives.empty()) return -1; // None available!
 if (1 == drives.size()) return drives[0];	// exactly one.

 std::vector<std::string> selections;
 for (int i = 0; i < drives.size() && i < 9; i++)
  selections.push_back( inv[drives[i]].tname() );

 int select = menu_vec("Choose drive:", selections);

 return drives[ select - 1 ];
}

bool player::is_wearing(itype_id it) const
{
 for (const auto& obj : worn) {
  if (obj.type->id == it) return true;
 }
 return false;
}

bool player::has_artifact_with(art_effect_passive effect) const
{
 if (weapon.is_artifact() && weapon.is_tool()) {
  const it_artifact_tool* const tool = dynamic_cast<const it_artifact_tool*>(weapon.type);
  if (any(tool->effects_wielded, effect)) return true;
  if (any(tool->effects_carried, effect)) return true;
 }
 for (size_t i = 0; i < inv.size(); i++) {
  const auto& it = inv[i];
  if (it.is_artifact() && it.is_tool()) {
   if (any(dynamic_cast<const it_artifact_tool*>(it.type)->effects_carried, effect)) return true;
  }
 }
 for (const auto& it : worn) {
  if (!it.is_artifact()) continue;
  if (any(dynamic_cast<const it_artifact_armor*>(it.type)->effects_worn, effect)) return true;
 }
 return false;
}
   

bool player::has_amount(itype_id it, int quantity) const
{
 if (it == itm_toolset) return has_bionic(bio_tools);
 return (amount_of(it) >= quantity);
}

int player::amount_of(itype_id it) const
{
 if (it == itm_toolset && has_bionic(bio_tools)) return 1;
 int quantity = 0;
 if (weapon.type->id == it) quantity++;
 for (const auto& obj : weapon.contents) {
  if (obj.type->id == it) quantity++;
 }
 quantity += inv.amount_of(it);
 return quantity;
}

bool player::has_charges(itype_id it, int quantity) const
{
 return (charges_of(it) >= quantity);
}

int player::charges_of(itype_id it) const
{
 if (it == itm_toolset) return has_bionic(bio_tools) ? power_level : 0;

 int quantity = 0;
 if (weapon.type->id == it) quantity += weapon.charges;
 for (const auto& obj : weapon.contents) {
  if (obj.type->id == it) quantity += obj.charges;
 }
 quantity += inv.charges_of(it);
 return quantity;
}

bool player::has_watertight_container() const
{
 for (size_t i = 0; i < inv.size(); i++) {
  if (inv[i].is_container() && inv[i].contents.empty()) {
   const it_container* const cont = dynamic_cast<const it_container*>(inv[i].type);
   if (cont->flags & mfb(con_wtight) && cont->flags & mfb(con_seals))
    return true;
  }
 }
 return false;
}

bool player::has_weapon_or_armor(char let) const
{
 if (weapon.invlet == let) return true;
 for (const auto& it : worn) if (it.invlet == let) return true;
 return false;
}

bool player::has_item(char let) const
{
 return (has_weapon_or_armor(let) || inv.index_by_letter(let) != -1);
}

bool player::has_item(item *it) const
{
 if (it == &weapon) return true;
 for (int i = 0; i < worn.size(); i++) {
  if (it == &(worn[i])) return true;
 }
 return inv.has_item(it);
}

bool player::has_mission_item(int mission_id) const
{
 if (mission_id == -1) return false;
 if (weapon.mission_id == mission_id) return true;
 for (const auto& it : weapon.contents) if (it.mission_id == mission_id) return true;
 for (size_t i = 0; i < inv.size(); i++) {
  for (int j = 0; j < inv.stack_at(i).size(); j++) {
   if (inv.stack_at(i)[j].mission_id == mission_id) return true;
   for (int k = 0; k < inv.stack_at(i)[j].contents.size(); k++) {
    if (inv.stack_at(i)[j].contents[k].mission_id == mission_id) return true;
   }
  }
 }
 return false;
}

int player::lookup_item(char let) const
{
 if (weapon.invlet == let) return -1;

 for (size_t i = 0; i < inv.size(); i++) {
  if (inv[i].invlet == let) return i;
 }

 return -2; // -2 is for "item not found"
}

bool player::eat(game *g, int index)
{
 const it_comest *comest = NULL;
 item *eaten = NULL;
 int which = -3; // Helps us know how to delete the item which got eaten
 if (index == -2) {
  messages.add("You do not have that item.");
  return false;
 } else if (index == -1) {
  if (weapon.is_food_container(*this)) {
   eaten = &weapon.contents[0];
   which = -2;
   if (weapon.contents[0].is_food()) comest = dynamic_cast<const it_comest*>(weapon.contents[0].type);
  } else if (weapon.is_food(*this)) {
   eaten = &weapon;
   which = -1;
   if (weapon.is_food()) comest = dynamic_cast<const it_comest*>(weapon.type);
  } else {
   if (!is_npc())
    messages.add("You can't eat your %s.", weapon.tname().c_str());
   else
    debugmsg("%s tried to eat a %s", name.c_str(), weapon.tname().c_str());
   return false;
  }
 } else {
  if (inv[index].is_food_container(*this)) {
   eaten = &(inv[index].contents[0]);
   which = index + inv.size();
   if (inv[index].contents[0].is_food()) comest = dynamic_cast<const it_comest*>(inv[index].contents[0].type);
  } else if (inv[index].is_food(*this)) {
   eaten = &inv[index];
   which = index;
   if (inv[index].is_food()) comest = dynamic_cast<const it_comest*>(inv[index].type);
  } else {
   if (!is_npc())
    messages.add("You can't eat your %s.", inv[index].tname().c_str());
   else
    debugmsg("%s tried to eat a %s", name.c_str(), inv[index].tname().c_str());
   return false;
  }
 }
 if (eaten == NULL) return false;

 if (eaten->is_ammo()) { // For when bionics let you eat fuel
  charge_power(eaten->charges / 20);
  eaten->charges = 0;
 } else if (!eaten->type->is_food() && !eaten->is_food_container(*this)) {
// For when bionics let you burn organic materials
  int charge = (eaten->volume() + eaten->weight()) / 2;
  if (eaten->type->m1 == LEATHER || eaten->type->m2 == LEATHER) charge /= 4;
  if (eaten->type->m1 == WOOD    || eaten->type->m2 == WOOD) charge /= 2;
  charge_power(charge);
 } else { // It's real food!  i.e. an it_comest
// Remember, comest points to the it_comest data
  if (comest == NULL) {
   debugmsg("player::eat(%s); comest is NULL!", eaten->tname().c_str());
   return false;
  }
  if (comest->tool != itm_null) {
   bool has = has_amount(comest->tool, 1);
   if (item::types[comest->tool]->count_by_charges()) has = has_charges(comest->tool, 1);
   if (!has) {
    if (!is_npc()) messages.add("You need a %s to consume that!", item::types[comest->tool]->name.c_str());
    return false;
   }
  }
  bool overeating = (!has_trait(PF_GOURMAND) && hunger < 0 &&
                     comest->nutr >= 15);
  bool spoiled = eaten->rotten();

  last_item = itype_id(eaten->type->id);

  if (overeating && !is_npc() && !query_yn("You're full.  Force yourself to eat?")) return false;

  if (has_trait(PF_CARNIVORE) && eaten->made_of(VEGGY) && comest->nutr > 0) {
   if (!is_npc())
    messages.add("You can only eat meat!");
   else
    messages.add("Carnivore %s tried to eat plants!", name.c_str());
   return false;
  }

  if (has_trait(PF_VEGETARIAN) && eaten->made_of(FLESH) && !is_npc() && !query_yn("Really eat that meat? (The poor animals!)")) return false;

  if (spoiled) {
   if (is_npc()) return false;
   if (!has_trait(PF_SAPROVORE) && !query_yn("This %s smells awful!  Eat it?", eaten->tname().c_str())) return false;
   messages.add("Ick, this %s doesn't taste so good...",eaten->tname().c_str());
   if (!has_trait(PF_SAPROVORE) && (!has_bionic(bio_digestion) || one_in(3)))
    add_disease(DI_FOODPOISON, rng(60, (comest->nutr + 1) * 60));
   hunger -= rng(0, comest->nutr);
   thirst -= comest->quench;
   if (!has_trait(PF_SAPROVORE) && !has_bionic(bio_digestion)) health -= 3;
  } else {
   hunger -= comest->nutr;
   thirst -= comest->quench;
   if (has_bionic(bio_digestion)) hunger -= rng(0, comest->nutr);
   else if (!has_trait(PF_GOURMAND)) {
    if ((overeating && rng(-200, 0) > hunger)) vomit();
   }
   health += comest->healthy;
  }
// At this point, we've definitely eaten the item, so use up some turns.
  moves -= has_trait(PF_GOURMAND) ? 150 : 250;
// If it's poisonous... poison us.  TODO: More several poison effects
  if (eaten->poison >= rng(2, 4)) add_disease(DI_POISON, eaten->poison * 100);
  if (eaten->poison > 0) add_disease(DI_FOODPOISON, eaten->poison * 300);

// Descriptive text
  if (!is_npc()) {
   if (eaten->made_of(LIQUID)) messages.add("You drink your %s.", eaten->tname().c_str());
   else if (comest->nutr >= 5) messages.add("You eat your %s.", eaten->tname().c_str());
  } else if (g->u_see(pos)) messages.add("%s eats a %s.", name.c_str(), eaten->tname().c_str());

  if (item::types[comest->tool]->is_tool()) use_charges(comest->tool, 1); // Tools like lighters get used
  if (comest->stim > 0) {
   if (comest->stim < 10 && stim < comest->stim) {
    stim += comest->stim;
    if (stim > comest->stim) stim = comest->stim;
   } else if (comest->stim >= 10 && stim < comest->stim * 3) stim += comest->stim;
  }
 
  (*comest->use)(g, this, eaten, false);
  add_addiction(comest->add, comest->addict);
  if (has_bionic(bio_ethanol) && comest->use == &iuse::alcohol) charge_power(rng(2, 8));

  if (has_trait(PF_VEGETARIAN) && eaten->made_of(FLESH)) {
   if (!is_npc()) messages.add("You feel bad about eating this meat...");
   add_morale(MORALE_VEGETARIAN, -75, -400);
  }
  if ((has_trait(PF_HERBIVORE) || has_trait(PF_RUMINANT)) && eaten->made_of(FLESH)) {
   if (!one_in(3)) vomit();
   if (comest->quench >= 2) thirst += int(comest->quench / 2);
   if (comest->nutr >= 2) hunger += int(comest->nutr * .75);
  }
  if (has_trait(PF_GOURMAND)) {
   if (comest->fun < -2)
    add_morale(MORALE_FOOD_BAD, comest->fun * 2, comest->fun * 4, comest);
   else if (comest->fun > 0)
    add_morale(MORALE_FOOD_GOOD, comest->fun * 3, comest->fun * 6, comest);
   if (!is_npc() && (hunger < -60 || thirst < -60)) messages.add("You can't finish it all!");
   if (hunger < -60) hunger = -60;
   if (thirst < -60) thirst = -60;
  } else {
   if (comest->fun < 0)
    add_morale(MORALE_FOOD_BAD, comest->fun * 2, comest->fun * 6, comest);
   else if (comest->fun > 0)
    add_morale(MORALE_FOOD_GOOD, comest->fun * 2, comest->fun * 4, comest);
   if (!is_npc() && (hunger < -20 || thirst < -20)) messages.add("You can't finish it all!");
   if (hunger < -20) hunger = -20;
   if (thirst < -20) thirst = -20;
  }
 }
 
 eaten->charges--;
 if (eaten->charges <= 0) {
  if (which == -1) weapon = item::null;
  else if (which == -2) {
   weapon.contents.erase(weapon.contents.begin());
   if (!is_npc()) messages.add("You are now wielding an empty %s.", weapon.tname().c_str());
  } else if (which >= 0 && which < inv.size())
   inv.remove_item(which);
  else if (which >= inv.size()) {
   which -= inv.size();
   inv[which].contents.erase(inv[which].contents.begin());
   if (!is_npc()) messages.add("%c - an empty %s", inv[which].invlet, inv[which].tname().c_str());
   if (inv.stack_at(which).size() > 0) inv.restack(this);
   inv_sorted = false;
  }
 }
 return true;
}

bool player::wield(game *g, int index)
{
 if (weapon.has_flag(IF_NO_UNWIELD)) {
  messages.add("You cannot unwield your %s!  Withdraw them with 'p'.", weapon.tname().c_str());
  return false;
 }
 if (index == -3) {
  bool pickstyle = (!styles.empty());
  if (weapon.is_style()) remove_weapon();
  else if (!is_armed()) {
   if (!pickstyle) {
    messages.add("You are already wielding nothing.");
    return false;
   }
  } else if (volume_carried() + weapon.volume() < volume_capacity()) {
   inv.push_back(remove_weapon());
   inv_sorted = false;
   moves -= 20;
   recoil = 0;
   if (!pickstyle) return true;
  } else if (query_yn("No space in inventory for your %s.  Drop it?", weapon.tname().c_str())) {
   g->m.add_item(pos, remove_weapon());
   recoil = 0;
   if (!pickstyle) return true;
  } else return false;

  if (pickstyle) {
   weapon = item(item::types[style_selected], 0 );
   weapon.invlet = ':';
   return true;
  }
 }
 if (index == -1) {
  messages.add("You're already wielding that!");
  return false;
 } else if (index == -2) {
  messages.add("You don't have that item.");
  return false;
 }

 if (inv[index].is_two_handed(*this) && !has_two_arms()) {
  messages.add("You cannot wield a %s with only one arm.", inv[index].tname().c_str());
  return false;
 }
 if (!is_armed()) {
  weapon = inv.remove_item(index);
  if (weapon.is_artifact() && weapon.is_tool()) game::add_artifact_messages(dynamic_cast<const it_artifact_tool*>(weapon.type)->effects_wielded);
  moves -= 30;
  last_item = itype_id(weapon.type->id);
  return true;
 } else if (volume_carried() + weapon.volume() - inv[index].volume() < volume_capacity()) {
  item tmpweap = remove_weapon();
  weapon = inv.remove_item(index);
  inv.push_back(tmpweap);
  inv_sorted = false;
  moves -= 45;
  if (weapon.is_artifact() && weapon.is_tool()) game::add_artifact_messages(dynamic_cast<const it_artifact_tool*>(weapon.type)->effects_wielded);
  last_item = itype_id(weapon.type->id);
  return true;
 } else if (query_yn("No space in inventory for your %s.  Drop it?",
                     weapon.tname().c_str())) {
  g->m.add_item(pos, remove_weapon());
  weapon = inv[index];
  inv.remove_item(index);
  inv_sorted = false;
  moves -= 30;
  if (weapon.is_artifact() && weapon.is_tool()) {
   game::add_artifact_messages(dynamic_cast<const it_artifact_tool*>(weapon.type)->effects_wielded);
  }
  last_item = itype_id(weapon.type->id);
  return true;
 }

 return false;
}

void player::pick_style() // Style selection menu
{
 std::vector<std::string> options;
 options.push_back("No style");
 for (const auto style : styles) options.push_back(item::types[style]->name);
 int selection = menu_vec("Select a style", options);
 style_selected = (2 <= selection) ? styles[selection - 2] : itm_null;
}

bool player::wear(game *g, char let)
{
 item* to_wear = NULL;
 int index = -1;
 if (weapon.invlet == let) {
  to_wear = &weapon;
  index = -2;
 } else {
  for (size_t i = 0; i < inv.size(); i++) {
   if (inv[i].invlet == let) {
    to_wear = &(inv[i]);
    index = i;
	break;
   }
  }
 }

 if (to_wear == NULL) {
  messages.add("You don't have item '%c'.", let);
  return false;
 }

 if (!wear_item(*to_wear)) return false;

 if (-2 == index) weapon = item::null;
 else inv.remove_item(index);

 return true;
}

bool player::wear_item(const item& to_wear)
{
 if (!to_wear.is_armor()) {
  messages.add("Putting on a %s would be tricky.", to_wear.tname().c_str());
  return false;
 }
 const it_armor* const armor = dynamic_cast<const it_armor*>(to_wear.type);

// Make sure we're not wearing 2 of the item already
 int count = 0;
 for (const auto& it : worn) {
  if (it.type->id == to_wear.type->id) count++;
 }
 if (2 <= count) {
  messages.add("You can't wear more than two %s at once.", to_wear.tname().c_str());
  return false;
 }
 if (has_trait(PF_WOOLALLERGY) && to_wear.made_of(WOOL)) {
  messages.add("You can't wear that, it's made of wool!");
  return false;
 }
 if (armor->covers & mfb(bp_head) && encumb(bp_head) != 0) {
  messages.add("You can't wear a%s helmet!", wearing_something_on(bp_head) ? "nother" : "");
  return false;
 }
 if (armor->covers & mfb(bp_hands) && has_trait(PF_WEBBED)) {
  messages.add("You cannot put %s over your webbed hands.", armor->name.c_str());
  return false;
 }
 if (armor->covers & mfb(bp_hands) && has_trait(PF_TALONS)) {
  messages.add("You cannot put %s over your talons.", armor->name.c_str());
  return false;
 }
 if (armor->covers & mfb(bp_mouth) && has_trait(PF_BEAK)) {
  messages.add("You cannot put a %s over your beak.", armor->name.c_str());
  return false;
 }
 if (armor->covers & mfb(bp_feet) && has_trait(PF_HOOVES)) {
  messages.add("You cannot wear footwear on your hooves.");
  return false;
 }
 if (armor->covers & mfb(bp_head) && has_trait(PF_HORNS_CURLED)) {
  messages.add("You cannot wear headgear over your horns.");
  return false;
 }
 if (armor->covers & mfb(bp_torso) && has_trait(PF_SHELL)) {
  messages.add("You cannot wear anything over your shell.");
  return false;
 }
 if (armor->covers & mfb(bp_head) && !to_wear.made_of(WOOL) &&
     !to_wear.made_of(COTTON) && !to_wear.made_of(LEATHER) &&
     (has_trait(PF_HORNS_POINTED) || has_trait(PF_ANTENNAE) ||
      has_trait(PF_ANTLERS))) {
	 messages.add("You cannot wear a helmet over your %s.",
             (has_trait(PF_HORNS_POINTED) ? "horns" :
              (has_trait(PF_ANTENNAE) ? "antennae" : "antlers")));
  return false;
 }
 if (armor->covers & mfb(bp_feet) && wearing_something_on(bp_feet)) {
  messages.add("You're already wearing footwear!");
  return false;
 }
 messages.add("You put on your %s.", to_wear.tname().c_str());
 if (to_wear.is_artifact()) game::add_artifact_messages(dynamic_cast<const it_artifact_armor*>(to_wear.type)->effects_worn);
 moves -= 350; // TODO: Make this variable?
 last_item = itype_id(to_wear.type->id);
 worn.push_back(to_wear);
 for (body_part i = bp_head; i < num_bp; i = body_part(i + 1)) {
  if (armor->covers & mfb(i) && encumb(i) >= 4)
   messages.add("Your %s %s very encumbered! %s",
              body_part_name(body_part(i), 2).c_str(),
              (i == bp_head || i == bp_torso || i == bp_mouth ? "is" : "are"),
              encumb_text(body_part(i)).c_str());
 }
 return true;
}

bool player::takeoff(map& m, char let)
{
 for (int i = 0; i < worn.size(); i++) {
  const auto& it = worn[i];
  if (it.invlet == let) {
   if (volume_capacity() - (dynamic_cast<const it_armor*>(it.type))->storage > volume_carried() + it.type->volume) {
    inv.push_back(it);
    worn.erase(worn.begin() + i);
    inv_sorted = false;
    return true;
   } else if (query_yn("No room in inventory for your %s.  Drop it?", worn[i].tname().c_str())) {
    m.add_item(pos, it);
    worn.erase(worn.begin() + i);
    return true;
   } else return false;
  }
 }
 messages.add("You are not wearing that item.");
 return false;
}

void player::use(game *g, char let)
{
 item* used = &i_at(let);
 item copy;
 bool replace_item = false;
 if (inv.index_by_letter(let) != -1) {
  copy = inv.remove_item_by_letter(let);
  copy.invlet = let;
  used = &copy;
  replace_item = true;
 }
 
 if (used->is_null()) {
  messages.add("You do not have that item.");
  return;
 }

 last_item = itype_id(used->type->id);

 if (used->is_tool()) {

  const it_tool* const tool = dynamic_cast<const it_tool*>(used->type);
  if (tool->charges_per_use == 0 || used->charges >= tool->charges_per_use) {
   (*tool->use)(g, this, used, false);
   used->charges -= tool->charges_per_use;
  } else
   messages.add("Your %s has %d charges but needs %d.", used->tname().c_str(), used->charges, tool->charges_per_use);

  if (tool->use == &iuse::dogfood) replace_item = false;

  if (replace_item && used->invlet != 0) inv.add_item_keep_invlet(copy);
  else if (used->invlet == 0 && used == &weapon) remove_weapon();
  return;

 } else if (used->is_gunmod()) {

  if (sklevel[sk_gun] == 0) {
   messages.add("You need to be at least level 1 in the firearms skill before you can modify guns.");
   if (replace_item) inv.add_item(copy);
   return;
  }
  char gunlet = g->inv("Select gun to modify:");
  const it_gunmod* const mod = dynamic_cast<const it_gunmod*>(used->type);
  item& gun = i_at(gunlet);
  if (gun.is_null()) {
   messages.add("You do not have that item.");
   if (replace_item) inv.add_item(copy);
   return;
  } else if (!gun.is_gun()) {
   messages.add("That %s is not a gun.", gun.tname().c_str());
   if (replace_item) inv.add_item(copy);
   return;
  }
  const it_gun* const guntype = dynamic_cast<const it_gun*>(gun.type);
  if (guntype->skill_used == sk_archery || guntype->skill_used == sk_launcher) {
   messages.add("You cannot mod your %s.", gun.tname().c_str());
   if (replace_item) inv.add_item(copy);
   return;
  }
  if (guntype->skill_used == sk_pistol && !mod->used_on_pistol) {
   messages.add("That %s cannot be attached to a handgun.", used->tname().c_str());
   if (replace_item) inv.add_item(copy);
   return;
  } else if (guntype->skill_used == sk_shotgun && !mod->used_on_shotgun) {
   messages.add("That %s cannot be attached to a shotgun.", used->tname().c_str());
   if (replace_item) inv.add_item(copy);
   return;
  } else if (guntype->skill_used == sk_smg && !mod->used_on_smg) {
   messages.add("That %s cannot be attached to a submachine gun.", used->tname().c_str());
   if (replace_item) inv.add_item(copy);
   return;
  } else if (guntype->skill_used == sk_rifle && !mod->used_on_rifle) {
   messages.add("That %s cannot be attached to a rifle.", used->tname().c_str());
   if (replace_item) inv.add_item(copy);
   return;
  } else if (mod->acceptible_ammo_types != 0 &&
             !(mfb(guntype->ammo) & mod->acceptible_ammo_types)) {
   messages.add("That %s cannot be used on a %s gun.", used->tname().c_str(), ammo_name(guntype->ammo).c_str());
   if (replace_item) inv.add_item(copy);
   return;
  } else if (gun.contents.size() >= 4) {
   messages.add("Your %s already has 4 mods installed!  To remove the mods, press 'U' while wielding the unloaded gun.", gun.tname().c_str());
   if (replace_item) inv.add_item(copy);
   return;
  }
  if ((mod->id == itm_clip || mod->id == itm_clip2) && gun.clip_size() <= 2) {
   messages.add("You can not extend the ammo capacity of your %s.", gun.tname().c_str());
   if (replace_item) inv.add_item(copy);
   return;
  }
  for (const auto& it : gun.contents) {
   if (it.type->id == used->type->id) {
    messages.add("Your %s already has a %s.", gun.tname().c_str(), used->tname().c_str());
    if (replace_item) inv.add_item(copy);
    return;
   } else if (mod->newtype != AT_NULL &&
        (dynamic_cast<const it_gunmod*>(it.type))->newtype != AT_NULL) {
    messages.add("Your %s's caliber has already been modified.", gun.tname().c_str());
    if (replace_item) inv.add_item(copy);
    return;
   } else if ((mod->id == itm_barrel_big || mod->id == itm_barrel_small) &&
              (it.type->id == itm_barrel_big ||
			   it.type->id == itm_barrel_small)) {
    messages.add("Your %s already has a barrel replacement.", gun.tname().c_str());
    if (replace_item) inv.add_item(copy);
    return;
   } else if ((mod->id == itm_clip || mod->id == itm_clip2) &&
              (it.type->id == itm_clip ||
			   it.type->id == itm_clip2)) {
    messages.add("Your %s already has its clip size extended.", gun.tname().c_str());
    if (replace_item) inv.add_item(copy);
    return;
   }
  }
  messages.add("You attach the %s to your %s.", used->tname().c_str(), gun.tname().c_str());
  gun.contents.push_back(replace_item ? copy : i_rem(let));
  return;

 } else if (used->is_bionic()) {

  const it_bionic* const tmp = dynamic_cast<const it_bionic*>(used->type);
  if (install_bionics(g, tmp)) {
   if (!replace_item) i_rem(let);
  } else if (replace_item) inv.add_item(copy);
  return;

 } else if (used->is_food() || used->is_food_container()) {
  if (replace_item) inv.add_item(copy);
  eat(g, lookup_item(let));
  return;
 } else if (used->is_book()) {
  if (replace_item) inv.add_item(copy);
  read(g, let);
  return;
 } else if (used->is_armor()) {
  if (replace_item) inv.add_item(copy);
  wear(g, let);
  return;
 } else
  messages.add("You can't do anything interesting with your %s.", used->tname().c_str());

 if (replace_item) inv.add_item(copy);
}

void player::read(game *g, char ch)
{
 vehicle *veh = g->m.veh_at(pos);
 if (veh && veh->player_in_control(*this)) {
  messages.add("It's bad idea to read while driving.");
  return;
 }
 if (morale_level() < MIN_MORALE_READ) {	// See morale.h
  messages.add("What's the point of reading?  (Your morale is too low!)");
  return;
 }
// Check if reading is okay
 if (g->light_level() < 8) {
  messages.add("It's too dark to read!");
  return;
 }

// Find the object
 int index = -1;
 if (weapon.invlet == ch) index = -2;
 else {
  for (size_t i = 0; i < inv.size(); i++) {
   if (inv[i].invlet == ch) {
    index = i;
	break;
   }
  }
 }

 if (index == -1) {
  messages.add("You do not have that item.");
  return;
 }

// Some macguffins can be read, but they aren't treated like books.
 const it_macguffin* mac = NULL;
 item *used = NULL;
 if (index == -2 && weapon.is_macguffin()) {
  mac = dynamic_cast<const it_macguffin*>(weapon.type);
  used = &weapon;
 } else if (index >= 0 && inv[index].is_macguffin()) {
  mac = dynamic_cast<const it_macguffin*>(inv[index].type);
  used = &(inv[index]);
 }
 if (mac != NULL) {
  (*mac->use)(g, this, used, false);
  return;
 }

 if ((index >=  0 && !inv[index].is_book()) || (index == -2 && !weapon.is_book())) {
  messages.add("Your %s is not good reading material.", (-2 == index ? weapon : inv[index]).tname().c_str());
  return;
 }

 const it_book* const book = dynamic_cast<const it_book*>((-2 == index) ? weapon.type : inv[index].type);
 if (book->intel > 0 && has_trait(PF_ILLITERATE)) {
  messages.add("You're illiterate!");
  return;
 } else if (book->intel > int_cur) {
  messages.add("This book is way too complex for you to understand.");
  return;
 } else if (book->req > sklevel[book->type]) {
  messages.add("The %s-related jargon flies over your head!", skill_name(book->type).c_str());
  return;
 } else if (book->level <= sklevel[book->type] && book->fun <= 0 &&
            !query_yn("Your %s skill won't be improved.  Read anyway?",
                      skill_name(book->type).c_str()))
  return;

// Base read_speed() is 1000 move points (1 minute per tmp->time)
 int time = book->time * read_speed();
 activity = player_activity(ACT_READ, time, index);
 moves = 0;
}
 
void player::try_to_sleep(const map& m)
{
 switch (const auto terrain = m.ter(pos))
 {
 case t_floor: break;
 case t_bed:
	 messages.add("This bed is a comfortable place to sleep.");
	 break;
 default:
	{
	const auto& t_data = ter_t::list[terrain];
    messages.add("It's %shard to get to sleep on this %s.", t_data.movecost <= 2 ? "a little " : "", t_data.name.c_str());
	}
 }
 add_disease(DI_LYING_DOWN, 300);
}

bool player::can_sleep(const map& m) const
{
 int sleepy = 0;
 if (has_addiction(ADD_SLEEP)) sleepy -= 3;
 if (has_trait(PF_INSOMNIA)) sleepy -= 8;

 int vpart = -1;
 const vehicle* const veh = m.veh_at(pos, vpart);
 if (veh && veh->part_with_feature(vpart, vpf_seat) >= 0) sleepy += 4;
 else if (m.ter(pos) == t_bed) sleepy += 5;
 else if (m.ter(pos) == t_floor) sleepy += 1;
 else sleepy -= m.move_cost(pos);
 sleepy = (192 > fatigue) ? (192-fatigue)/4 : (fatigue-192)/16;
 sleepy += rng(-8, 8);
 sleepy -= 2 * stim;
 return 0 < sleepy;
}

int player::warmth(body_part bp) const
{
 int ret = 0;
 for (const auto& it : worn) {
  const it_armor* const armor = dynamic_cast<const it_armor*>(it.type);
  if (armor->covers & mfb(bp)) ret += armor->warmth;
 }
 return ret;
}

int player::encumb(body_part bp) const
{
 int ret = 0;
 int layers = 0;
 for (const auto& it : worn) {
  const it_armor* const armor = dynamic_cast<const it_armor*>(it.type);
  if (armor->covers & mfb(bp) ||
      (bp == bp_torso && (armor->covers & mfb(bp_arms)))) {
   ret += armor->encumber;
   if (armor->encumber >= 0 || bp != bp_torso) layers++;
  }
 }
 if (layers > 1) ret += (layers - 1) * (bp == bp_torso ? .5 : 2);// Easier to layer on torso
 if (volume_carried() > volume_capacity() - 2 && bp != bp_head) ret += 3;

// Bionics and mutation
 if ((bp == bp_head  && has_bionic(bio_armor_head))  ||
     (bp == bp_torso && has_bionic(bio_armor_torso)) ||
     (bp == bp_legs  && has_bionic(bio_armor_legs)))
  ret += 2;
 if (has_bionic(bio_stiff) && bp != bp_head && bp != bp_mouth) ret += 1;
 if (has_trait(PF_CHITIN3) && bp != bp_eyes && bp != bp_mouth) ret += 1;
 if (has_trait(PF_SLIT_NOSTRILS) && bp == bp_mouth) ret += 1;
 if (bp == bp_hands &&
     (has_trait(PF_ARM_TENTACLES) || has_trait(PF_ARM_TENTACLES_4) ||
      has_trait(PF_ARM_TENTACLES_8)))
  ret += 3;
 return ret;
}

int player::armor_bash(body_part bp) const
{
 int ret = 0;
 for (const auto& it : worn) {
  const it_armor* const armor = dynamic_cast<const it_armor*>(it.type);
  if (armor->covers & mfb(bp)) ret += armor->dmg_resist;
 }
 if (has_bionic(bio_carbon)) ret += 2;
 if (bp == bp_head && has_bionic(bio_armor_head)) ret += 3;
 else if (bp == bp_arms && has_bionic(bio_armor_arms)) ret += 3;
 else if (bp == bp_torso && has_bionic(bio_armor_torso)) ret += 3;
 else if (bp == bp_legs && has_bionic(bio_armor_legs)) ret += 3;
 if (has_trait(PF_FUR)) ret++;
 if (has_trait(PF_CHITIN)) ret += 2;
 if (has_trait(PF_SHELL) && bp == bp_torso) ret += 6;
 ret += rng(0, disease_intensity(DI_ARMOR_BOOST));
 return ret;
}

int player::armor_cut(body_part bp) const
{
 int ret = 0;
 for (const auto& it : worn) {
  const it_armor* const armor = dynamic_cast<const it_armor*>(it.type);
  if (armor->covers & mfb(bp)) ret += armor->cut_resist;
 }
 if (has_bionic(bio_carbon)) ret += 4;
 if (bp == bp_head && has_bionic(bio_armor_head)) ret += 3;
 else if (bp == bp_arms && has_bionic(bio_armor_arms)) ret += 3;
 else if (bp == bp_torso && has_bionic(bio_armor_torso)) ret += 3;
 else if (bp == bp_legs && has_bionic(bio_armor_legs)) ret += 3;
 if (has_trait(PF_THICKSKIN)) ret++;
 if (has_trait(PF_SCALES)) ret += 2;
 if (has_trait(PF_THICK_SCALES)) ret += 4;
 if (has_trait(PF_SLEEK_SCALES)) ret += 1;
 if (has_trait(PF_CHITIN)) ret += 2;
 if (has_trait(PF_CHITIN2)) ret += 4;
 if (has_trait(PF_CHITIN3)) ret += 8;
 if (has_trait(PF_SHELL) && bp == bp_torso) ret += 14;
 ret += rng(0, disease_intensity(DI_ARMOR_BOOST));
 return ret;
}

void player::absorb(game *g, body_part bp, int &dam, int &cut)
{
 int arm_bash = 0, arm_cut = 0;
 if (has_active_bionic(bio_ads)) {
  if (dam > 0 && power_level > 1) {
   dam -= rng(1, 8);
   power_level--;
  }
  if (cut > 0 && power_level > 1) {
   cut -= rng(0, 4);
   power_level--;
  }
  if (dam < 0) dam = 0;
  if (cut < 0) cut = 0;
 }
// See, we do it backwards, which assumes the player put on their jacket after
//  their T shirt, for example.  TODO: don't assume! ASS out of U & ME, etc.
 for (int i = worn.size() - 1; i >= 0; i--) {
  const it_armor* const tmp = dynamic_cast<const it_armor*>(worn[i].type);
  if ((tmp->covers & mfb(bp)) && tmp->storage < 20) {
   arm_bash = tmp->dmg_resist;
   arm_cut  = tmp->cut_resist;
   switch (worn[i].damage) {
   case 1:
    arm_bash *= .8;
    arm_cut  *= .9;
    break;
   case 2:
    arm_bash *= .7;
    arm_cut  *= .7;
    break;
   case 3:
    arm_bash *= .5;
    arm_cut  *= .4;
    break;
   case 4:
    arm_bash *= .2;
    arm_cut  *= .1;
    break;
   }
// Wool, leather, and cotton clothing may be damaged by CUTTING damage
   if ((worn[i].made_of(WOOL)   || worn[i].made_of(LEATHER) ||
        worn[i].made_of(COTTON) || worn[i].made_of(GLASS)   ||
        worn[i].made_of(WOOD)   || worn[i].made_of(KEVLAR)) &&
       rng(0, tmp->cut_resist * 2) < cut && !one_in(cut))
    worn[i].damage++;
// Kevlar, plastic, iron, steel, and silver may be damaged by BASHING damage
   if ((worn[i].made_of(PLASTIC) || worn[i].made_of(IRON)   ||
        worn[i].made_of(STEEL)   || worn[i].made_of(SILVER) ||
        worn[i].made_of(STONE))  &&
       rng(0, tmp->dmg_resist * 2) < dam && !one_in(dam))
    worn[i].damage++;
   if (worn[i].damage >= 5) {
    if (!is_npc()) messages.add("Your %s is completely destroyed!", worn[i].tname().c_str());
    else if (g->u_see(pos))
     messages.add("%s's %s is destroyed!", name.c_str(), worn[i].tname().c_str());
    worn.erase(worn.begin() + i);
   }
  }
  dam -= arm_bash;
  cut -= arm_cut;
 }
 if (has_bionic(bio_carbon)) {
  dam -= 2;
  cut -= 4;
 }
 if (bp == bp_head && has_bionic(bio_armor_head)) {
  dam -= 3;
  cut -= 3;
 } else if (bp == bp_arms && has_bionic(bio_armor_arms)) {
  dam -= 3;
  cut -= 3;
 } else if (bp == bp_torso && has_bionic(bio_armor_torso)) {
  dam -= 3;
  cut -= 3;
 } else if (bp == bp_legs && has_bionic(bio_armor_legs)) {
  dam -= 3;
  cut -= 3;
 }
 if (has_trait(PF_THICKSKIN))
  cut--;
 if (has_trait(PF_SCALES))
  cut -= 2;
 if (has_trait(PF_THICK_SCALES))
  cut -= 4;
 if (has_trait(PF_SLEEK_SCALES))
  cut -= 1;
 if (has_trait(PF_FEATHERS))
  dam--;
 if (has_trait(PF_FUR))
  dam--;
 if (has_trait(PF_CHITIN)) cut -= 2;
 if (has_trait(PF_CHITIN2)) {
  dam--;
  cut -= 4;
 }
 if (has_trait(PF_CHITIN3)) {
  dam -= 2;
  cut -= 8;
 }
 if (has_trait(PF_PLANTSKIN)) dam--;
 if (has_trait(PF_BARK)) dam -= 2;
 if (bp == bp_feet && has_trait(PF_HOOVES)) cut--;
 if (has_trait(PF_LIGHT_BONES)) dam *= 1.4;
 if (has_trait(PF_HOLLOW_BONES)) dam *= 1.8;
 if (dam < 0) dam = 0;
 if (cut < 0) cut = 0;
}
  
int player::resist(body_part bp) const
{
 int ret = 0;
 for (const auto& it : worn) {
  const it_armor* const armor = dynamic_cast<const it_armor*>(it.type);
  if (armor->covers & mfb(bp) ||
      (bp == bp_eyes && (armor->covers & mfb(bp_head)))) // Head protection works on eyes too (e.g. baseball cap)
   ret += armor->env_resist;
 }
 if (bp == bp_mouth && has_bionic(bio_purifier) && ret < 5) {
  ret += 2;
  if (ret == 6) ret = 5;
 }
 return ret;
}

bool player::wearing_something_on(body_part bp) const
{
 for (const auto& it : worn) {
  if ((dynamic_cast<const it_armor*>(it.type))->covers & mfb(bp)) return true;
 }
 return false;
}

void player::practice(skill s, int amount)
{
 skill savant = sk_null;
 int savant_level = 0, savant_exercise = 0;
 if (skexercise[s] < 0)
  amount += (amount >= -1 * skexercise[s] ? -1 * skexercise[s] : amount);
 if (has_trait(PF_SAVANT)) {
// Find our best skill
  for (int i = 1; i < num_skill_types; i++) {
   if (sklevel[i] >= savant_level) {
    savant = skill(i);
    savant_level = sklevel[i];
    savant_exercise = skexercise[i];
   } else if (sklevel[i] == savant_level && skexercise[i] > savant_exercise) {
    savant = skill(i);
    savant_exercise = skexercise[i];
   }
  }
 }
 while (amount > 0 && xp_pool >= (1 + sklevel[s])) {
  amount -= sklevel[s] + 1;
  if ((savant == sk_null || savant == s || !one_in(2)) &&
      rng(0, 100) < comprehension_percent(s)) {
   xp_pool -= (1 + sklevel[s]);
   skexercise[s]++;
  }
 }
}

void player::assign_activity(activity_type type, int moves, int index)
{
 if (backlog.type == type && backlog.index == index && query_yn("Resume task?")) {
  activity = backlog;
  backlog = player_activity();
 } else
  activity = player_activity(type, moves, index);
}

void player::cancel_activity()
{
 if (activity_is_suspendable(activity.type)) backlog = activity;
 activity.type = ACT_NULL;
}

std::vector<int> player::has_ammo(ammotype at) const
{
 std::vector<int> ret;
 bool newtype = true;
 for (size_t a = 0; a < inv.size(); a++) {
  if (!inv[a].is_ammo()) continue;
  const it_ammo* ammo = dynamic_cast<const it_ammo*>(inv[a].type);
  if (ammo->type != at) continue;
  bool newtype = true;
  for (const auto n : ret) {
   const auto& it = inv[n];
   if (ammo->id == it.type->id && it.charges == it.charges) {
// They're effectively the same; don't add it to the list
// TODO: Bullets may become rusted, etc., so this if statement may change
    newtype = false;
	break;
   }
  }
  if (newtype) ret.push_back(a);
 }
 return ret;
}

std::string player::weapname(bool charges) const
{
 if (!(weapon.is_tool() && dynamic_cast<const it_tool*>(weapon.type)->max_charges <= 0) && weapon.charges >= 0 && charges) {
  std::stringstream dump;
  dump << weapon.tname().c_str() << " (" << weapon.charges << ")";
  return dump.str();
 } else if (weapon.is_null()) return "fists";

 else if (weapon.is_style()) { // Styles get bonus-bars!
  std::stringstream dump;
  dump << weapon.tname();

  switch (weapon.type->id) {
   case itm_style_capoeira:
    if (has_disease(DI_DODGE_BOOST))
     dump << " +Dodge";
    if (has_disease(DI_ATTACK_BOOST))
     dump << " +Attack";
    break;

   case itm_style_ninjutsu:
   case itm_style_leopard:
    if (has_disease(DI_ATTACK_BOOST))
     dump << " +Attack";
    break;

   case itm_style_crane:
    if (has_disease(DI_DODGE_BOOST))
     dump << " +Dodge";
    break;

   case itm_style_dragon:
    if (has_disease(DI_DAMAGE_BOOST))
     dump << " +Damage";
    break;

   case itm_style_tiger: {
    dump << " [";
    int intensity = disease_intensity(DI_DAMAGE_BOOST);
    for (int i = 1; i <= 5; i++) {
     if (intensity >= i * 2)
      dump << "*";
     else
      dump << ".";
    }
    dump << "]";
   } break;

   case itm_style_centipede: {
    dump << " [";
    int intensity = disease_intensity(DI_SPEED_BOOST);
    for (int i = 1; i <= 8; i++) {
     if (intensity >= i * 4)
      dump << "*";
     else
      dump << ".";
    }
    dump << "]";
   } break;

   case itm_style_venom_snake: {
    dump << " [";
    int intensity = disease_intensity(DI_VIPER_COMBO);
    for (int i = 1; i <= 2; i++) {
     if (intensity >= i)
      dump << "C";
     else
      dump << ".";
    }
    dump << "]";
   } break;

   case itm_style_lizard: {
    dump << " [";
    int intensity = disease_intensity(DI_ATTACK_BOOST);
    for (int i = 1; i <= 4; i++) {
     if (intensity >= i)
      dump << "*";
     else
      dump << ".";
    }
    dump << "]";
   } break;

   case itm_style_toad: {
    dump << " [";
    int intensity = disease_intensity(DI_ARMOR_BOOST);
    for (int i = 1; i <= 5; i++) {
     if (intensity >= 5 + i)
      dump << "!";
     else if (intensity >= i)
      dump << "*";
     else
      dump << ".";
    }
    dump << "]";
   } break;

  } // switch (weapon.type->id)
  return dump.str();
 } else
  return weapon.tname();
}

nc_color encumb_color(int level)
{
 if (level < 0)
  return c_green;
 if (level == 0)
  return c_ltgray;
 if (level < 4)
  return c_yellow;
 if (level < 7)
  return c_ltred;
 return c_red;
}

bool activity_is_suspendable(activity_type type)
{
 if (type == ACT_NULL || type == ACT_RELOAD)
  return false;
 return true;
}

static bool file_to_string_vector(const char* src, std::vector<std::string>& dest)
{
	if (!src || !src[0]) return false;
	std::ifstream fin;
	fin.exceptions(std::ios::badbit);	// throw on hardware failure
	fin.open(src);
	if (!fin.is_open()) {
		debugmsg((std::string("Could not open ")+std::string(src)).c_str());
		return false;
	}
	std::string x;
	do {
		getline(fin, x);
		if (!x.empty()) dest.push_back(x);
	} while (!fin.eof());
	fin.close();
	return !dest.empty();
}

std::string random_first_name(bool male)
{
	static std::vector<std::string> mr;
	static std::vector<std::string> mrs;
	static bool have_tried_to_read_mr_file = false;
	static bool have_tried_to_read_mrs_file = false;

	std::vector<std::string>& first_names = male ? mr : mrs;
	if (!first_names.empty()) return first_names[rng(0, first_names.size() - 1)];
	if (male ? have_tried_to_read_mr_file : have_tried_to_read_mrs_file) return "";
	if (male) have_tried_to_read_mr_file = true;
	else have_tried_to_read_mrs_file = true;

	if (!file_to_string_vector(male ? "data/NAMES_MALE" : "data/NAMES_FEMALE", first_names)) return "";
	return first_names[rng(0, first_names.size() - 1)];
}

std::string random_last_name()
{
 static std::vector<std::string> last_names;
 static bool have_tried_to_read_file = false;

 if (!last_names.empty()) return last_names[rng(0, last_names.size() - 1)];
 if (have_tried_to_read_file) return "";
 have_tried_to_read_file = true;
 if (!file_to_string_vector("data/NAMES_LAST", last_names)) return "";
 return last_names[rng(0, last_names.size() - 1)];
}

