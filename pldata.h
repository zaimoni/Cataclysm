#ifndef _PLDATA_H_
#define _PLDATA_H_

#include "enums.h"

#include <vector>
#include <string>

enum character_type {
 PLTYPE_CUSTOM,
 PLTYPE_RANDOM,
 PLTYPE_TEMPLATE,
 PLTYPE_MAX
};

enum dis_type {
 DI_NULL,
// Temperature and weather
 DI_GLARE, DI_WET,
 DI_COLD, DI_COLD_FACE, DI_COLD_HANDS, DI_COLD_LEGS, DI_COLD_FEET,
 DI_HOT,	// Can lead to heatstroke
 DI_HEATSTROKE, DI_FBFACE, DI_FBHANDS, DI_FBFEET,
 DI_INFECTION,
 DI_COMMON_COLD, DI_FLU,
// Fields
 DI_SMOKE, DI_ONFIRE, DI_TEARGAS,
// Monsters
 DI_BOOMERED, DI_SAP, DI_SPORES, DI_FUNGUS, DI_SLIMED,
 DI_DEAF, DI_BLIND,
 DI_LYING_DOWN, DI_SLEEP,
 DI_POISON, DI_BADPOISON, DI_FOODPOISON, DI_SHAKES,
 DI_DERMATIK, DI_FORMICATION,
 DI_WEBBED,
 DI_RAT,
// Food & Drugs
 DI_PKILL1, DI_PKILL2, DI_PKILL3, DI_PKILL_L, DI_DRUNK, DI_CIG, DI_HIGH,
  DI_HALLU, DI_VISUALS, DI_IODINE, DI_TOOK_XANAX, DI_TOOK_PROZAC,
  DI_TOOK_FLUMED, DI_TOOK_VITAMINS, DI_ADRENALINE, DI_ASTHMA, DI_METH,	// XXX DI_ADRENALINE is PC-only \todo fix this
// Traps
 DI_BEARTRAP, DI_IN_PIT, DI_STUNNED, DI_DOWNED,
// Martial Arts
 DI_ATTACK_BOOST, DI_DAMAGE_BOOST, DI_DODGE_BOOST, DI_ARMOR_BOOST,
  DI_SPEED_BOOST, DI_VIPER_COMBO,
// Other
 DI_AMIGARA, DI_TELEGLOW, DI_ATTENTION, DI_EVIL,
// Inflicted by an NPC
 DI_ASKED_TO_FOLLOW, DI_ASKED_TO_LEAD, DI_ASKED_FOR_ITEM,
// NPC-only
 DI_CATCH_UP
};

DECLARE_JSON_ENUM_SUPPORT(dis_type)

enum add_type {
 ADD_NULL,
 ADD_CAFFEINE, ADD_ALCOHOL, ADD_SLEEP, ADD_PKILLER, ADD_SPEED, ADD_CIG,
 ADD_COKE
};

DECLARE_JSON_ENUM_SUPPORT(add_type)

struct disease
{
 dis_type type;
 int intensity;
 int duration;
 disease(dis_type t = DI_NULL, int d = 0, int i = 0) : type(t),intensity(i),duration(d) {}
 disease(std::istream& is);
 friend std::ostream& operator<<(std::ostream& os, const disease& src);

 int speed_boost() const;
 const char* name() const;
 std::string description() const;
};

struct addiction
{
 add_type type;
 int intensity;
 int sated;
 addiction() : type(ADD_NULL),intensity(0),sated(600) {}
 addiction(add_type t, int i = 1) : type(t), intensity(i), sated(600) {}
 addiction(std::istream& is);
 friend std::ostream& operator<<(std::ostream& os, const addiction& src);
};

enum activity_type {
 ACT_NULL = 0,
 ACT_RELOAD, ACT_READ, ACT_WAIT, ACT_CRAFT, ACT_BUTCHER, ACT_BUILD,
 ACT_VEHICLE, ACT_REFILL_VEHICLE,
 ACT_TRAIN,
 NUM_ACTIVITIES
};

DECLARE_JSON_ENUM_SUPPORT(activity_type)

struct player_activity
{
 activity_type type;
 int moves_left;
 int index;
 std::vector<int> values;
 point placement;

 player_activity(activity_type t = ACT_NULL, int turns = 0, int Index = -1)
 : type(t),moves_left(turns),index(Index),placement(-1,-1) {}
 player_activity(const player_activity &copy) = default;
};
 
enum pl_flag {
 PF_NULL = 0,
 PF_FLEET,	// -15% terrain movement cost
 PF_PARKOUR,	// Terrain movement cost of 3 or 4 are both 2
 PF_QUICK,	// +10% movement points
 PF_OPTIMISTIC,	// Morale boost
 PF_FASTHEALER,	// Heals faster
 PF_LIGHTEATER,	// Less hungry
 PF_PAINRESIST,	// Effects of pain are reduced
 PF_NIGHTVISION,// Increased sight during the night
 PF_POISRESIST,	// Resists poison, etc
 PF_FASTREADER,	// Reads books faster
 PF_TOUGH,	// Bonus to HP
 PF_THICKSKIN,	// Built-in armor of 1
 PF_PACKMULE,	// Bonus to carried volume
 PF_FASTLEARNER,// Better chance of skills leveling up
 PF_DEFT,	// Less movement penalty for melee miss
 PF_DRUNKEN,	// Having a drunk status improves melee combat
 PF_GOURMAND,	// Faster eating, higher level of max satiated
 PF_ANIMALEMPATH,// Animals attack less
 PF_TERRIFYING,	// All creatures run away more
 PF_DISRESISTANT,// Less likely to succumb to low health; TODO: Implement this
 PF_ADRENALINE,	// Big bonuses when low on HP
 PF_INCONSPICUOUS,// Less spawns due to timeouts
 PF_MASOCHIST,	// Morale boost from pain
 PF_LIGHTSTEP,	// Less noise from movement
 PF_HEARTLESS,	// No morale penalty for murder &c
 PF_ANDROID,	// Start with two bionics (occasionally one)
 PF_ROBUST,	// Mutations tend to be good (usually they tend to be bad)
 PF_MARTIAL_ARTS, // Start with a martial art

 PF_SPLIT,	// Null trait, splits between bad & good

 PF_MYOPIC,	// Smaller sight radius UNLESS wearing glasses
 PF_HEAVYSLEEPER, // Sleeps in, won't wake up to sounds as easily
 PF_ASTHMA,	// Occasionally needs medicine or suffers effects
 PF_BADBACK,	// Carries less
 PF_ILLITERATE,	// Can not read books
 PF_BADHEARING,	// Max distance for heard sounds is halved
 PF_INSOMNIA,	// Sleep may not happen
 PF_VEGETARIAN,	// Morale penalty for eating meat
 PF_GLASSJAW,	// Head HP is 15% lower
 PF_FORGETFUL,	// Skills decrement faster
 PF_LIGHTWEIGHT,// Longer DI_DRUNK and DI_HIGH
 PF_ADDICTIVE,	// Better chance of addiction / longer addictive effects
 PF_TRIGGERHAPPY,// Possible chance of unintentional burst fire
 PF_SMELLY,	// Default scent is higher
 PF_CHEMIMBALANCE,// Random tweaks to some values
 PF_SCHIZOPHRENIC,// Random bad effects, variety
 PF_JITTERY,	// Get DI_SHAKES under some circumstances
 PF_HOARDER,	// Morale penalty when volume is less than max
 PF_SAVANT,	// All skills except our best are trained more slowly
 PF_MOODSWINGS,	// Big random shifts in morale
 PF_WEAKSTOMACH,// More likely to throw up in all circumstances
 PF_WOOLALLERGY,// Can't wear wool
 PF_HPIGNORANT,	// Don't get to see exact HP numbers, just colors & symbols
 PF_TRUTHTELLER, // Worse at telling lies
 PF_UGLY, // +1 grotesqueness

 PF_MAX,
// Below this point is mutations and other mid-game perks.
// They are NOT available during character creation.
 PF_SKIN_ROUGH,//
 PF_NIGHTVISION2,//
 PF_NIGHTVISION3,//
 PF_INFRARED,//
 PF_FASTHEALER2,//
 PF_REGEN,//
 PF_FANGS,//
 PF_MEMBRANE,//
 PF_GILLS,//
 PF_SCALES,//
 PF_THICK_SCALES,//
 PF_SLEEK_SCALES,//
 PF_LIGHT_BONES,//
 PF_FEATHERS,//
 PF_LIGHTFUR,// TODO: Warmth effects
 PF_FUR,// TODO: Warmth effects
 PF_CHITIN,//
 PF_CHITIN2,//
 PF_CHITIN3,//
 PF_SPINES,//
 PF_QUILLS,//
 PF_PLANTSKIN,//
 PF_BARK,//
 PF_THORNS,
 PF_LEAVES,//
 PF_NAILS,
 PF_CLAWS,
 PF_TALONS,//
 PF_RADIOGENIC,//
 PF_MARLOSS,//
 PF_PHEROMONE_INSECT,//
 PF_PHEROMONE_MAMMAL,//
 PF_DISIMMUNE,
 PF_POISONOUS,//
 PF_SLIME_HANDS,
 PF_COMPOUND_EYES,//
 PF_PADDED_FEET,//
 PF_HOOVES,//
 PF_SAPROVORE,//
 PF_RUMINANT,//
 PF_HORNS,//
 PF_HORNS_CURLED,//
 PF_HORNS_POINTED,//
 PF_ANTENNAE,//
 PF_FLEET2,//
 PF_TAIL_STUB,//
 PF_TAIL_FIN,//
 PF_TAIL_LONG,//
 PF_TAIL_FLUFFY,//
 PF_TAIL_STING,//
 PF_TAIL_CLUB,//
 PF_PAINREC1,//
 PF_PAINREC2,//
 PF_PAINREC3,//
 PF_WINGS_BIRD,//
 PF_WINGS_INSECT,//
 PF_MOUTH_TENTACLES,//
 PF_MANDIBLES,//
 PF_CANINE_EARS,
 PF_WEB_WALKER,
 PF_WEB_WEAVER,
 PF_WHISKERS,
 PF_STR_UP,
 PF_STR_UP_2,
 PF_STR_UP_3,
 PF_STR_UP_4,
 PF_DEX_UP,
 PF_DEX_UP_2,
 PF_DEX_UP_3,
 PF_DEX_UP_4,
 PF_INT_UP,
 PF_INT_UP_2,
 PF_INT_UP_3,
 PF_INT_UP_4,
 PF_PER_UP,
 PF_PER_UP_2,
 PF_PER_UP_3,
 PF_PER_UP_4,

 PF_HEADBUMPS,//
 PF_ANTLERS,//
 PF_SLIT_NOSTRILS,//
 PF_FORKED_TONGUE,//
 PF_EYEBULGE,//
 PF_MOUTH_FLAPS,//
 PF_WINGS_STUB,//
 PF_WINGS_BAT,//
 PF_PALE,//
 PF_SPOTS,//
 PF_SMELLY2,//TODO: NPC reaction
 PF_DEFORMED,
 PF_DEFORMED2,
 PF_DEFORMED3,
 PF_HOLLOW_BONES,//
 PF_NAUSEA,//
 PF_VOMITOUS,//
 PF_HUNGER,//
 PF_THIRST,//
 PF_ROT1,//
 PF_ROT2,//
 PF_ROT3,//
 PF_ALBINO,//
 PF_SORES,//
 PF_TROGLO,//
 PF_TROGLO2,//
 PF_TROGLO3,//
 PF_WEBBED,//
 PF_BEAK,//
 PF_UNSTABLE,//
 PF_RADIOACTIVE1,//
 PF_RADIOACTIVE2,//
 PF_RADIOACTIVE3,//
 PF_SLIMY,//
 PF_HERBIVORE,//
 PF_CARNIVORE,//
 PF_PONDEROUS1,	// 10% movement penalty
 PF_PONDEROUS2, // 20%
 PF_PONDEROUS3, // 30%
 PF_SUNLIGHT_DEPENDENT,//
 PF_COLDBLOOD,//
 PF_COLDBLOOD2,//
 PF_COLDBLOOD3,//
 PF_GROWL,//
 PF_SNARL,//
 PF_SHOUT1,//
 PF_SHOUT2,//
 PF_SHOUT3,//
 PF_ARM_TENTACLES,
 PF_ARM_TENTACLES_4,
 PF_ARM_TENTACLES_8,
 PF_SHELL,
 PF_LEG_TENTACLES,

 PF_MAX2
};

enum hp_part {
 hp_head = 0,
 hp_torso,
 hp_arm_l,
 hp_arm_r,
 hp_leg_l,
 hp_leg_r,
 num_hp_parts
};
#endif
