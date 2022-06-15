#include "mtype.h"
#include "item.h"
#ifndef SOCRATES_DAIMON
#include "monattack.h"
#include "mondeath.h"
#endif
#include "json.h"
#include "game_aux.hpp"

#include "Zaimoni.STL/Logging.h"

std::vector<const mtype*> mtype::types;
std::map<int, std::string> mtype::tiles;

static auto default_anger(monster_species spec)
{
	switch (spec) {
	case species_insect: return mfb(MTRIG_FRIEND_DIED);
	default: return 0ULL;
	}
}

static auto default_fears(monster_species spec)
{
	switch (spec) {
	case species_mammal: return mfb(MTRIG_HURT) | mfb(MTRIG_FIRE) | mfb(MTRIG_FRIEND_DIED);
	case species_insect: return mfb(MTRIG_HURT) | mfb(MTRIG_FIRE);
	case species_worm: return mfb(MTRIG_HURT);
	case species_plant: return mfb(MTRIG_HURT) | mfb(MTRIG_FIRE);
	case species_fungus: return mfb(MTRIG_HURT) | mfb(MTRIG_FIRE);
	case species_nether: return mfb(MTRIG_HURT);
	default: return 0ULL;
	}
}

// Default constructor
mtype::mtype()
: id(0), name("human"), species(species_none), sym(' '), color(c_white), size(MS_MEDIUM), mat(FLESH), flags(0),
  anger(0), placate(0), fear(0), frequency(0), difficulty(0), agro(0),
  morale(0), speed(0), melee_skill(0), melee_dice(0), melee_sides(0), melee_cut(0),
  sk_dodge(0), armor_bash(0), armor_cut(0), item_chance(0), hp(0), sp_freq(0)
#ifndef SOCRATES_DAIMON
  , dies(nullptr), sp_attack(nullptr), special_attack(nullptr)
#endif
{
}

// Non-default (messy)
mtype::mtype(int pid, std::string pname, monster_species pspecies, char psym,
	nc_color pcolor, m_size psize, material pmat,
	unsigned char pfreq, unsigned int pdiff, signed char pagro,
	int pmorale, unsigned int pspeed, unsigned char pml_skill,
	unsigned char pml_dice, unsigned char pml_sides, unsigned char pml_cut,
	unsigned char pdodge, unsigned char parmor_bash,
	unsigned char parmor_cut, signed char pitem_chance, int php,
	unsigned char psp_freq,
#ifndef SOCRATES_DAIMON
	void(*pdies)      (game *, monster *),
	void(*psp_attack)(game *, monster *),
#endif
	std::string pdescription)
: id(pid), name(pname), description(pdescription), species(pspecies), sym(psym), color(pcolor), size(psize), mat(pmat), flags(0),
  anger(default_anger(species)), placate(0), fear(default_fears(species)), frequency(pfreq), difficulty(pdiff), agro(pagro),
  morale(pmorale), speed(pspeed), melee_skill(pml_skill), melee_dice(pml_dice), melee_sides(pml_sides), melee_cut(pml_cut),
  sk_dodge(pdodge), armor_bash(parmor_bash), armor_cut(parmor_cut), item_chance(pitem_chance), hp(php), sp_freq(psp_freq)
#ifndef SOCRATES_DAIMON
  , dies(pdies), sp_attack(psp_attack), special_attack(nullptr)
#endif
{
	assert(MS_TINY <= size && MS_HUGE >= size);
}

#ifndef SOCRATES_DAIMON
mtype::mtype(int pid, std::string pname, monster_species pspecies, char psym,
	nc_color pcolor, m_size psize, material pmat,
	unsigned char pfreq, unsigned int pdiff, signed char pagro,
	int pmorale, unsigned int pspeed, unsigned char pml_skill,
	unsigned char pml_dice, unsigned char pml_sides, unsigned char pml_cut,
	unsigned char pdodge, unsigned char parmor_bash,
	unsigned char parmor_cut, signed char pitem_chance, int php,
	unsigned char psp_freq,
	decltype(dies) pdies,
	decltype(special_attack) psp_attack,
	std::string pdescription)
	: id(pid), name(pname), description(pdescription), species(pspecies), sym(psym), color(pcolor), size(psize), mat(pmat), flags(0),
	anger(default_anger(species)), placate(0), fear(default_fears(species)), frequency(pfreq), difficulty(pdiff), agro(pagro),
	morale(pmorale), speed(pspeed), melee_skill(pml_skill), melee_dice(pml_dice), melee_sides(pml_sides), melee_cut(pml_cut),
	sk_dodge(pdodge), armor_bash(parmor_bash), armor_cut(parmor_cut), item_chance(pitem_chance), hp(php), sp_freq(psp_freq)
	, dies(pdies), sp_attack(nullptr), special_attack(psp_attack)
{
	assert(MS_TINY <= size && MS_HUGE >= size);
}
#endif

nc_color mtype::danger() const
{
	if (30 <= difficulty) return c_red;
	if (15 <= difficulty) return c_ltred;
	if (8 <= difficulty) return c_white;
	if (0 < agro) return c_ltgray;
	return c_dkgray;
}

const itype* mtype::chunk_material() const
{
	switch (mat)
	{
	case FLESH: return item::types[has_flag(MF_POISON) ? itm_meat_tainted : itm_meat];
	case VEGGY: return item::types[has_flag(MF_POISON) ? itm_veggy_tainted : itm_veggy];
	default: return nullptr;	// no other materials have chunks: inherited from C:Whales
	}
}

int mtype::chunk_count() const
{
	switch(size) {
	case MS_TINY:   return 1;
	case MS_SMALL:  return 2;
	case MS_MEDIUM: return 4;
	case MS_LARGE:  return 8;
	case MS_HUGE:   return 16;
	default: return 0;
	}
}

#ifndef SOCRATES_DAIMON
void mtype::do_special_attack(monster& viewpoint) const
{
	if (special_attack) special_attack(viewpoint);
	if (sp_attack) sp_attack(active_game(), &viewpoint);
}
#endif

static const char* JSON_transcode[num_monsters] = {
	"mon_null",
	"mon_squirrel",
	"mon_rabbit",
	"mon_deer",
	"mon_wolf",
	"mon_bear",
	"mon_dog",
	"mon_ant_larva",
	"mon_ant",
	"mon_ant_soldier",
	"mon_ant_queen",
	"mon_ant_fungus",
	"mon_fly",
	"mon_bee",
	"mon_wasp",
	"mon_graboid",
	"mon_worm",
	"mon_halfworm",
	"mon_zombie",
	"mon_zombie_shrieker",
	"mon_zombie_spitter",
	"mon_zombie_electric",
	"mon_zombie_fast",
	"mon_zombie_brute",
	"mon_zombie_hulk",
	"mon_zombie_fungus",
	"mon_boomer",
	"mon_boomer_fungus",
	"mon_skeleton",
	"mon_zombie_necro",
	"mon_zombie_scientist",
	"mon_zombie_soldier",
	"mon_zombie_grabber",
	"mon_zombie_master",
	"mon_triffid",
	"mon_triffid_young",
	"mon_triffid_queen",
	"mon_creeper_hub",
	"mon_creeper_vine",
	"mon_biollante",
	"mon_vinebeast",
	"mon_triffid_heart",
	"mon_fungaloid",
	"mon_fungaloid_dormant",
	"mon_fungaloid_young",
	"mon_spore",
	"mon_fungaloid_queen",
	"mon_fungal_wall",
	"mon_blob",
	"mon_blob_small",
	"mon_chud",
	"mon_one_eye",
	"mon_crawler",
	"mon_sewer_fish",
	"mon_sewer_snake",
	"mon_sewer_rat",
	"mon_rat_king",
	"mon_mosquito",
	"mon_dragonfly",
	"mon_centipede",
	"mon_frog",
	"mon_slug",
	"mon_dermatik_larva",
	"mon_dermatik",
	"mon_spider_wolf",
	"mon_spider_web",
	"mon_spider_jumping",
	"mon_spider_trapdoor",
	"mon_spider_widow",
	"mon_dark_wyrm",
	"mon_amigara_horror",
	"mon_dog_thing",
	"mon_headless_dog_thing",
	"mon_thing",
	"mon_human_snail",
	"mon_twisted_body",
	"mon_vortex",
	"mon_flying_polyp",
	"mon_hunting_horror",
	"mon_mi_go",
	"mon_yugg",
	"mon_gelatin",
	"mon_flaming_eye",
	"mon_kreck",
	"mon_blank",
	"mon_gozu",
	"mon_shadow",
	"mon_breather_hub",
	"mon_breather",
	"mon_shadow_snake",
	"mon_eyebot",
	"mon_manhack",
	"mon_skitterbot",
	"mon_secubot",
	"mon_copbot",
	"mon_molebot",
	"mon_tripod",
	"mon_chickenbot",
	"mon_tankbot",
	"mon_turret",
	"mon_exploder",
	"mon_hallu_zom",
	"mon_hallu_bee",
	"mon_hallu_ant",
	"mon_hallu_mom",
	"mon_generator"
};

DEFINE_JSON_ENUM_SUPPORT_TYPICAL(mon_id, JSON_transcode)

using namespace cataclysm;

// This function populates the master list of monster types.
// If you edit this function, you'll also need to edit:
//  * mtype.h - enum mon_id MUST match the order of this list!
//  * monster.cpp - void make_fungus() should be edited, or fungal infection
//                  will simply kill the monster
//  * mongroupdef.cpp - void init_moncats() should be edited, so the monster
//                      spawns with the proper group
// PLEASE NOTE: The description is AT MAX 4 lines of 46 characters each.

void mtype::init()
{
 int id = 0;
 mtype* working = nullptr;
// Null monster named "None".
 types.push_back(new mtype);

#ifdef SOCRATES_DAIMON
#define mon(name, species, sym, color, size, mat, \
            freq, diff, agro, morale, speed, melee_skill, melee_dice,\
            melee_sides, melee_cut, dodge, arm_bash, arm_cut, item_chance, HP,\
            sp_freq, death, sp_att, desc) \
id++;\
working = new mtype(id, name, species, sym, color, size, mat,\
freq, diff, agro, morale, speed, melee_skill, melee_dice, melee_sides,\
melee_cut, dodge, arm_bash, arm_cut, item_chance, HP, sp_freq, desc);	\
types.push_back(working)
#else
#define mon(name, species, sym, color, size, mat, \
            freq, diff, agro, morale, speed, melee_skill, melee_dice,\
            melee_sides, melee_cut, dodge, arm_bash, arm_cut, item_chance, HP,\
            sp_freq, death, sp_att, desc) \
id++;\
working = new mtype(id, name, species, sym, color, size, mat,\
freq, diff, agro, morale, speed, melee_skill, melee_dice, melee_sides,\
melee_cut, dodge, arm_bash, arm_cut, item_chance, HP, sp_freq, death, sp_att,\
desc);	\
types.push_back(working)
#endif

// PLEASE NOTE: The description is AT MAX 4 lines of 46 characters each.

// FOREST ANIMALS
mon("squirrel",	species_mammal, 'r',	c_ltgray,	MS_TINY,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 50,  0,-99, -8,140,  0,  1,  1,  0,  4,  0,  0,  0,  1,  0,
	&mdeath::normal,	&mattack::none, "\
A small woodland animal."
);
working->flags = mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_ANIMAL) | mfb(MF_FUR) | mfb(MF_WARM);

mon("rabbit",	species_mammal, 'r',	c_white,	MS_TINY,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 10,  0,-99, -7,180, 0,  0,  0,  0,  6,  0,  0,  0,  4,  0,
	&mdeath::normal,	&mattack::none, "\
A cute wiggling nose, cotton tail, and\n\
delicious flesh."
);
working->flags = mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_ANIMAL) | mfb(MF_FUR) | mfb(MF_WARM);

mon("deer",	species_mammal, 'd',	c_brown,	MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3,  1,-99, -5,300,  4,  3,  3,  0,  3,  0,  0,  0, 80, 0,
	&mdeath::normal,	&mattack::none, "\
A large buck, fast-moving and strong."
);
working->flags = mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_ANIMAL) | mfb(MF_FUR) | mfb(MF_WARM);

mon("wolf",	species_mammal, 'w',	c_dkgray,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  4, 12,  0, 20,165, 14,  2,  3,  4,  4,  1,  0,  0, 28,  0,
	&mdeath::normal,	&mattack::none, "\
A vicious and fast pack hunter."
);
working->flags = mfb(MF_HIT_AND_RUN) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_ANIMAL) | mfb(MF_FUR) | mfb(MF_WARM);
working->anger = mfb(MTRIG_HURT) | mfb(MTRIG_PLAYER_WEAK) | mfb(MTRIG_TIME);
working->placate = mfb(MTRIG_MEAT);

mon("bear",	species_mammal, 'B',	c_dkgray,	MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2, 10,-10, 40,140, 10,  3,  4,  6,  3,  2,  0,  0, 90, 0,
	&mdeath::normal,	&mattack::none, "\
Remember, only YOU can prevent forest fires."
);
working->flags = mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_ANIMAL) | mfb(MF_FUR) | mfb(MF_WARM);
working->anger = mfb(MTRIG_PLAYER_CLOSE);
working->placate = mfb(MTRIG_MEAT);

// DOMESTICATED ANIMALS
mon("dog",	species_mammal, 'd',	c_white,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 1,   5,  2, 15,150, 12,  2,  3,  3,  3,  0,  0,  0, 25,  0,
	&mdeath::normal,	&mattack::none, "\
A medium-sized domesticated dog, gone feral."
);
working->flags = mfb(MF_HIT_AND_RUN) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_ANIMAL) | mfb(MF_FUR) | mfb(MF_WARM);

// INSECTOIDS
mon("ant larva",species_insect, 'a',	c_white,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1,  0, -1, 10, 65,  4,  1,  3,  0,  0,  0,  0,  0, 10,  0,
	&mdeath::normal,	&mattack::none, "\
The size of a large cat, this pulsating mass\n\
of glistening white flesh turns your stomach."
);
working->flags = mfb(MF_POISON) | mfb(MF_SMELLS);

mon("giant ant",species_insect, 'a',	c_brown,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 20,  7, 15, 60,100,  9,  1,  6,  4,  2,  4,  8,-40, 40,  0,
	&mdeath::normal,	&mattack::none, "\
A red ant the size of a crocodile. It is\n\
covered in chitinous armor, and has a pair of\n\
vicious mandibles."
);
working->flags = mfb(MF_SMELLS);

mon("soldier ant",species_insect, 'a',	c_blue,		MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2, 16, 25,100,115, 12,  2,  4,  6,  2,  5, 10,-50, 80,  0,
	&mdeath::normal,	&mattack::none, "\
Darker in color than the other ants, this\n\
more aggressive variety has even larger\n\
mandibles."
);
working->flags = mfb(MF_SMELLS);

mon("queen ant",species_insect, 'a',	c_ltred,	MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0, 13,  0,100, 60,  6,  3,  4,  4,  1,  6, 14,-40,180, 1,
	&mdeath::normal,	&mattack::antqueen, "\
This ant has a long, bloated thorax, bulging\n\
with hundreds of small ant eggs.  It moves\n\
slowly, tending to nearby eggs and laying\n\
still more."
);
working->flags = mfb(MF_QUEEN) | mfb(MF_SMELLS);

mon("fungal insect",species_fungus, 'a',c_ltgray,	MS_MEDIUM,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  5, 80,100, 75,  5,  1,  5,  3,  1,  1,  1,  0, 30, 60,
	&mdeath::normal,	&mattack::fungus, "\
This insect is pale gray in color, its\n\
chitin weakened by the fungus sprouting\n\
from every joint on its body."
);
working->flags = mfb(MF_POISON) | mfb(MF_SMELLS);

mon("giant fly",species_insect, 'a',	c_ltgray,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  8, 50, 30,120,  3,  1,  3,  0,  5,  0,  0,  0, 25,  0,
	&mdeath::normal,	&mattack::none, "\
A large housefly the size of a small dog.\n\
It buzzes around incessantly."
);
working->flags = mfb(MF_HIT_AND_RUN) | mfb(MF_FLIES) | mfb(MF_POISON) | mfb(MF_SMELLS) | mfb(MF_STUMBLES);

mon("giant bee",species_insect, 'a',	c_yellow,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2, 15,-10, 50,140,  4,  1,  1,  5,  6,  0,  5,-50, 20,  0,
	&mdeath::normal,	&mattack::none, "\
A honey bee the size of a small dog. It\n\
buzzes angrily through the air, dagger-\n\
sized sting pointed forward."
);
working->flags = mfb(MF_HIT_AND_RUN) | mfb(MF_FLIES) | mfb(MF_VENOM) | mfb(MF_SMELLS) | mfb(MF_STUMBLES);
working->anger = mfb(MTRIG_FRIEND_DIED) | mfb(MTRIG_PLAYER_CLOSE);

mon("giant wasp",species_insect, 'a', 	c_red,		MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2, 22,  5, 60,150,  6,  1,  3,  7,  7,  0,  7,-40, 35, 0,
	&mdeath::normal,	&mattack::none, "\
An evil-looking, slender-bodied wasp with\n\
a vicious sting on its abdomen."
);
working->flags = mfb(MF_FLIES) | mfb(MF_VENOM) | mfb(MF_POISON) | mfb(MF_SMELLS);
working->anger = mfb(MTRIG_FRIEND_DIED) | mfb(MTRIG_PLAYER_CLOSE) | mfb(MTRIG_SOUND);

// GIANT WORMS

mon("graboid",	species_worm, 'S',	c_red,		MS_HUGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 17, 30,120,180, 11,  3,  8,  4,  0,  5,  5,  0,180,  0,
	&mdeath::worm,		&mattack::none, "\
A hideous slithering beast with a tri-\n\
sectional mouth that opens to reveal\n\
hundreds of writhing tongues. Most of its\n\
enormous body is hidden underground."
);
working->flags = mfb(MF_DIGS) | mfb(MF_DESTROYS) | mfb(MF_HEARS) | mfb(MF_GOODHEARING) | mfb(MF_WARM) | mfb(MF_LEATHER);

mon("giant worm",species_worm, 'S',	c_pink,		MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 30, 10, 30,100, 85,  9,  4,  5,  2,  0,  2,  0,  0, 50,  0,
	&mdeath::worm,		&mattack::none, "\
Half of this monster is emerging from a\n\
hole in the ground. It looks like a huge\n\
earthworm, but the end has split into a\n\
large, fanged mouth."
);
working->flags = mfb(MF_DIGS) | mfb(MF_HEARS) | mfb(MF_GOODHEARING) | mfb(MF_WARM) | mfb(MF_LEATHER);

mon("half worm",species_worm, 's',	c_pink,		MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  2, 10, 40, 80,  5,  3,  5,  0,  0,  0,  0,  0, 20,  0,
	&mdeath::normal,	&mattack::none, "\
A portion of a giant worm that is still alive."
);
working->flags = mfb(MF_DIGS) | mfb(MF_HEARS) | mfb(MF_GOODHEARING) | mfb(MF_WARM) | mfb(MF_LEATHER);

// ZOMBIES
mon("zombie",	species_zombie, 'Z',	c_ltgreen,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 90,  3,100,100, 70,  8,  1,  5,  0,  1,  0,  0, 40, 50,  0,
	&mdeath::normal,	&mattack::none, "\
A human body, stumbling slowly forward on\n\
uncertain legs, possessed with an unstoppable\n\
rage."
);
working->flags = mfb(MF_POISON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_WARM) | mfb(MF_BASHES) | mfb(MF_STUMBLES);

mon("shrieker zombie",species_zombie, 'Z',c_magenta,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  4, 10,100,100, 95,  9,  1,  2,  0,  4,  0,  0, 45, 50, 10,
	&mdeath::normal,	&mattack::shriek, "\
This zombie's jaw has been torn off, leaving\n\
a gaping hole from mid-neck up."
);
working->flags = mfb(MF_POISON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_WARM) | mfb(MF_BASHES) | mfb(MF_STUMBLES);

mon("spitter zombie",species_zombie, 'Z',c_yellow,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  4,  9,100,100,105,  8,  1,  5,  0,  4,  0,  0, 30, 60, 20,
	&mdeath::acid,		&mattack::acid,	"\
This zombie's mouth is deformed into a round\n\
spitter, and its body throbs with a dense\n\
yellow fluid."
);
working->flags = mfb(MF_ACIDPROOF) | mfb(MF_POISON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS)
               | mfb(MF_WARM) | mfb(MF_BASHES) | mfb(MF_STUMBLES);

mon("shocker zombie",species_zombie,'Z',c_ltcyan,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3, 10,100,100,110,  8,  1,  6,  0,  4,  0,  0, 40, 65, 25,
	&mdeath::normal,	&mattack::shockstorm, "\
This zombie's flesh is pale blue, and it\n\
occasionally crackles with small bolts of\n\
lightning."
);
working->flags = mfb(MF_ELECTRIC) | mfb(MF_POISON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS)
               | mfb(MF_WARM) | mfb(MF_BASHES) | mfb(MF_STUMBLES);

mon("fast zombie",species_zombie, 'Z',	c_ltred,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  6, 12,100,100,150, 10,  1,  4,  3,  4,  0,  0, 45, 40,  0,
	&mdeath::normal,	&mattack::none, "\
This deformed, sinewy zombie stays close to\n\
the ground, loping forward faster than most\n\
humans ever could."
);
working->flags = mfb(MF_HIT_AND_RUN) | mfb(MF_POISON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS)
               | mfb(MF_WARM) | mfb(MF_BASHES) | mfb(MF_STUMBLES);

mon("zombie brute",species_zombie, 'Z',	c_red,		MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  4, 25,100,100,115,  9,  4,  4,  2,  0,  6,  3, 60, 80,  0,
	&mdeath::normal,	&mattack::none, "\
A hideous beast of a zombie, bulging with\n\
distended muscles on both arms and legs."
);
working->flags = mfb(MF_POISON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_WARM) | mfb(MF_BASHES) | mfb(MF_STUMBLES);

mon("zombie hulk",species_zombie, 'Z',	c_blue,		MS_HUGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 50,100,100,130,  9,  4,  8,  0,  0, 12,  8, 80,260,  0,
	&mdeath::normal,	&mattack::none, "\
A zombie that has somehow grown to the size of\n\
6 men, with arms as wide as a trash can."
);
working->flags = mfb(MF_DESTROYS) | mfb(MF_ATTACKMON) | mfb(MF_POISON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_WARM) | mfb(MF_LEATHER)
               | mfb(MF_BASHES) | mfb(MF_STUMBLES);

mon("fungal zombie",species_fungus, 'Z',c_ltgray,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2,  6,100,100, 45,  6,  1,  6,  0,  0,  0,  0, 20, 40, 50,
	&mdeath::normal,	&mattack::fungus, "\
A diseased zombie. Fungus sprouts from its\n\
mouth and eyes, and thick gray mold grows all\n\
over its translucent flesh."
);
working->flags = mfb(MF_POISON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_WARM) | mfb(MF_BASHES) | mfb(MF_STUMBLES);

mon("boomer",	species_zombie, 'Z',	c_pink,		MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  6,  5,100,100, 55,  7,  2,  4,  0,  1,  3,  0, 35, 40,  20,
	&mdeath::boomer,	&mattack::boomer, "\
A bloated zombie sagging with fat. It emits a\n\
horrible odor, and putrid, pink sludge drips\n\
from its mouth."
);
working->flags = mfb(MF_POISON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_WARM) | mfb(MF_BASHES) | mfb(MF_STUMBLES);

mon("fungal boomer",species_fungus, 'B',c_ltgray,	MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1,  7,100,100, 40,  5,  2,  6,  0,  0,  3,  0, 20, 20, 30,
	&mdeath::fungus,	&mattack::fungus, "\
A bloated zombie that is coated with slimy\n\
gray mold. Its flesh is translucent and gray,\n\
and it dribbles a gray sludge from its mouth."
);
working->flags = mfb(MF_POISON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_WARM) | mfb(MF_BASHES) | mfb(MF_STUMBLES);

mon("skeleton",	species_zombie, 'Z',	c_white,	MS_MEDIUM,	STONE,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  7,  8,100,100, 90, 10,  1,  5,  3,  2,  0, 15,  0, 40, 0,
	&mdeath::normal,	&mattack::none, "\
A skeleton picked clean of all but a few\n\
rotten scraps of flesh, somehow still in\n\
motion."
);
working->flags = mfb(MF_HARDTOSHOOT) | mfb(MF_SEES) | mfb(MF_HEARS);

mon("zombie necromancer",species_zombie, 'Z',c_dkgray,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3, 13,  0,100,100,  4,  2,  3,  0,  4,  0,  0, 50,140, 4,
	&mdeath::normal,	&mattack::resurrect, "\
A zombie with jet black skin and glowing red\n\
eyes.  As you look at it, you're gripped by a\n\
feeling of dread and terror."
);
working->flags = mfb(MF_POISON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_WARM) | mfb(MF_BASHES) | mfb(MF_STUMBLES);
working->anger = mfb(MTRIG_HURT) | mfb(MTRIG_PLAYER_WEAK);

mon("zombie scientist",species_zombie, 'Z',c_ltgray,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 20,  3,100,100, 75,  7,  1,  3,  0,  1,  0,  0, 50, 35, 20,
	&mdeath::normal,	&mattack::science, "\
A zombie wearing a tattered lab coat and\n\
some sort of utility belt.  It looks weaker\n\
than most zombies, but more resourceful too."
);
working->flags = mfb(MF_ACIDPROOF) | mfb(MF_POISON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS)
               | mfb(MF_WARM) | mfb(MF_BASHES) | mfb(MF_STUMBLES);

mon("zombie soldier",	species_zombie,	'Z',c_ltblue,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 20,100,100, 80, 12,  2,  4,  0,  0,  8, 16, 60,100, 0,
	&mdeath::normal,	&mattack::none, "\
This zombie was clearly a soldier before.\n\
Its tattered armor gives it strong defense,\n\
and it is much more physically fit than\n\
most zombies."
);
working->flags = mfb(MF_POISON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_WARM) | mfb(MF_BASHES);

mon("grabber zombie",	species_zombie,	'Z',c_green,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  6,  8,100,100, 70, 10,  1,  2,  0,  4,  0,  0, 40, 65, 0,
	&mdeath::normal,	&mattack::none, "\
This zombie seems to have slightly longer\n\
than ordinary arms, and constantly gropes\n\
at its surroundings as it moves."
);
working->flags = mfb(MF_GRABS) | mfb(MF_POISON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS)
               | mfb(MF_WARM) | mfb(MF_BASHES) | mfb(MF_STUMBLES);

mon("master zombie",	species_zombie, 'M',c_yellow,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 16,  5,100, 90,  4,  1,  3,  0,  4,  0,  0, 60,120, 3,
	&mdeath::normal,	&mattack::upgrade, "\
This zombie seems to have a cloud of black\n\
dust surrounding it.  It also seems to have\n\
a better grasp of movement than most..."
);
working->flags = mfb(MF_POISON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_WARM) | mfb(MF_BASHES);
working->anger = mfb(MTRIG_HURT) | mfb(MTRIG_PLAYER_WEAK);

// PLANTS & FUNGI
mon("triffid",	species_plant, 'F',	c_ltgreen,	MS_MEDIUM,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
 	 24, 16, 20,100, 75,  9,  2,  4,  5,  0, 10,  2,  0, 80,  0,
	&mdeath::normal,	&mattack::none, "\
A plant that grows as high as your head,\n\
with one thick, bark-coated stalk\n\
supporting a flower-like head with a sharp\n\
sting within."
);
working->flags = mfb(MF_NOHEAD) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_BASHES);

mon("young triffid",species_plant, 'f',	c_ltgreen,	MS_SMALL,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 15,  2,  0, 10, 65,  7,  1,  4,  3,  0,  0,  0,  0, 40,  0,
	&mdeath::normal,	&mattack::none, "\
A small triffid, only a few feet tall. It\n\
has not yet developed bark, but its sting\n\
is still sharp and deadly."
);
working->flags = mfb(MF_NOHEAD) | mfb(MF_HEARS) | mfb(MF_SMELLS);

mon("queen triffid",species_plant, 'F',	c_red,		MS_LARGE,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3, 28,100,200, 85, 14,  2,  7,  8,  0, 10,  4,  0,280, 2,
	&mdeath::normal,	&mattack::growplants, "\
A very large triffid, with a particularly\n\
vicious sting and thick bark.  As it\n\
moves, plant matter drops off its body\n\
and immediately takes root."
);
working->flags = mfb(MF_NOHEAD) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_BASHES);

mon("creeper hub",species_plant, 'V',	c_dkgray,	MS_MEDIUM,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0, 16,100,100,100,  0,  0,  0,  0,  0,  8,  0,  0,100, 2,
	&mdeath::kill_vines,	&mattack::grow_vine, "\
A thick stalk, rooted to the ground.\n\
It rapidly sprouts thorny vines in all\n\
directions."
);
working->flags = mfb(MF_IMMOBILE) | mfb(MF_NOHEAD);

mon("creeping vine",species_plant, 'v',	c_green,	MS_TINY,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  4,100,100, 75,  0,  0,  0,  0,  0,  2,  0,  0,  2, 2,
	&mdeath::vine_cut,	&mattack::vine, "\
A thorny vine.  It twists wildly as\n\
it grows, spreading rapidly."
);
working->flags = mfb(MF_IMMOBILE) | mfb(MF_NOHEAD) | mfb(MF_HARDTOSHOOT) | mfb(MF_PLASTIC);

mon("biollante",species_plant, 'F',	c_magenta,	MS_LARGE,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0, 20,100,100,100,  0,  0,  0,  0,  0,  0,  0,-80,120, 2,
	&mdeath::normal,	&mattack::spit_sap, "\
A thick stalk topped with a purple\n\
flower.  The flower's petals are closed,\n\
and pulsate ominously."
);
working->flags = mfb(MF_IMMOBILE) | mfb(MF_NOHEAD);

mon("vinebeast",species_plant, 'V',	c_red,		MS_LARGE,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  8, 14, 60, 40, 75, 15,  2,  4,  2,  4, 10,  0,  0,100, 0,
	&mdeath::normal,	&mattack::none,	"\
This appears to be a mass of vines, moving\n\
with surprising speed.  It is so thick and\n\
tangled that you cannot see what lies in\n\
the middle."
);
working->flags = mfb(MF_NOHEAD) | mfb(MF_GRABS) | mfb(MF_HARDTOSHOOT) | mfb(MF_PLASTIC) | mfb(MF_SWIMS) | mfb(MF_HEARS) | mfb(MF_GOODHEARING);

mon("triffid heart",species_plant, 'T',	c_red,		MS_HUGE,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
          0, 45,100,100,100,  0,  0,  0,  0,  0, 14,  4,  0,300, 5,
	&mdeath::triffid_heart,	&mattack::triffid_heartbeat, "\
A knot of roots that looks bizarrely like a\n\
heart.  It beats slowly with sap, powering\n\
the root walls around it."
);
working->flags = mfb(MF_IMMOBILE) | mfb(MF_NOHEAD) | mfb(MF_QUEEN);

mon("fungaloid",species_fungus, 'F',	c_ltgray,	MS_MEDIUM,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 12, 12,  0,100, 45,  8,  3,  3,  0,  0,  4,  0,  0, 80, 200,
	&mdeath::fungus,	&mattack::fungus, "\
A pale white fungus, one meaty gray stalk\n\
supporting a bloom at the top. A few\n\
tendrils extend from the base, allowing\n\
mobility and a weak attack."
);
working->flags = mfb(MF_NOHEAD) | mfb(MF_POISON) | mfb(MF_HEARS) | mfb(MF_SMELLS);
working->anger = mfb(MTRIG_PLAYER_WEAK) | mfb(MTRIG_PLAYER_CLOSE);

// This is a "dormant" fungaloid that doesn't waste CPU cycles ;)
mon("fungaloid",species_fungus, 'F',	c_ltgray,	MS_MEDIUM,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  0,  0,100,  1,  8,  2,  4,  0,  0,  4,  0,  0,  1, 0,
	&mdeath::fungusawake,	&mattack::none, "\
A pale white fungus, one meaty gray stalk\n\
supporting a bloom at the top. A few\n\
tendrils extend from the base, allowing\n\
mobility and a weak attack."
);
working->flags = mfb(MF_NOHEAD) | mfb(MF_POISON) | mfb(MF_HEARS) | mfb(MF_SMELLS);

mon("young fungaloid",species_fungus, 'f',c_ltgray,	MS_SMALL,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  6,  6,100,100, 65,  8,  1,  4,  6,  0,  4,  4,  0, 70, 0,
	&mdeath::normal,	&mattack::none, "\
A fungal tendril just a couple feet tall.  Its\n\
exterior is hardened into a leathery bark and\n\
covered in thorns; it also moves faster than\n\
full-grown fungaloids."
);
working->flags = mfb(MF_NOHEAD) | mfb(MF_POISON) | mfb(MF_HEARS) | mfb(MF_SMELLS);

mon("spore",	species_fungus, 'o',	c_ltgray,	MS_TINY,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  1,-50,100,100,  0,  0,  0,  0,  6,  0,  0,  0,  5, 50,
	&mdeath::disintegrate,	&mattack::plant, "\
A wispy spore, about the size of a fist,\n\
wafting on the breeze."
);
working->flags = mfb(MF_NOHEAD) | mfb(MF_FLIES) | mfb(MF_POISON) | mfb(MF_STUMBLES);

mon("fungal spire",species_fungus, 'T',	c_ltgray,	MS_HUGE,	STONE,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0, 40,100,100,100,  0,  0,  0,  0,  0, 10, 10,  0,300, 5,
	&mdeath::fungus,	&mattack::fungus_sprout, "\
An enormous fungal spire, towering 30 feet\n\
above the ground.  It pulsates slowly,\n\
continuously growing new defenses."
);
working->flags = mfb(MF_IMMOBILE) | mfb(MF_NOHEAD) | mfb(MF_POISON) | mfb(MF_QUEEN);

mon("fungal wall",species_fungus, 'F',	c_dkgray,	MS_HUGE,	VEGGY,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  5,100,100,100,  0,  0,  0,  0,  0, 10, 10,  0, 60, 8,
	&mdeath::disintegrate,	&mattack::fungus, "\
A veritable wall of fungus, grown as a\n\
natural defense by the fungal spire. It\n\
looks very tough, and spews spores at an\n\
alarming rate."
);
working->flags = mfb(MF_IMMOBILE) | mfb(MF_NOHEAD) | mfb(MF_POISON);

// BLOBS & SLIMES &c
mon("blob",	species_nether, 'O',	c_dkgray,	MS_MEDIUM,	LIQUID,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 10, 19,100,100, 85,  9,  2,  4,  0,  0,  6,  0,  0, 85, 30,
	&mdeath::blobsplit,	&mattack::formblob, "\
A black blob of viscous goo that oozes\n\
across the ground like a mass of living\n\
oil."
);
working->flags = mfb(MF_NOHEAD) | mfb(MF_ACIDPROOF) | mfb(MF_POISON) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_GOODHEARING);

mon("small blob",species_nether, 'o',	c_dkgray,	MS_SMALL,	LIQUID,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1,  2,100,100, 50,  6,  1,  4,  0,  0,  2,  0,  0, 50, 0,
	&mdeath::blobsplit,	&mattack::none, "\
A small blob of viscous goo that oozes\n\
across the ground like a mass of living\n\
oil."
);
working->flags = mfb(MF_NOHEAD) | mfb(MF_ACIDPROOF) | mfb(MF_POISON) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_GOODHEARING);

// CHUDS & SUBWAY DWELLERS
mon("C.H.U.D.",	species_none, 'S',	c_ltgray,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 50,  8, 30, 30,110, 10,  1,  5,  0,  3,  0,  0, 25, 60, 0,
	&mdeath::normal,	&mattack::none,	"\
Cannibalistic Humanoid Underground Dweller.\n\
A human, turned pale and mad from years in\n\
the subways."
);
working->flags = mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_WARM) | mfb(MF_BASHES);
working->fear = mfb(MTRIG_HURT) | mfb(MTRIG_FIRE);

mon("one-eyed mutant",species_none, 'S',c_ltred,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  5, 18, 30, 30,130, 20,  2,  4,  0,  5,  0,  0, 40, 80, 0,
	&mdeath::normal,	&mattack::none,	"\
A relatively humanoid mutant with purple\n\
hair and a grapefruit-sized bloodshot eye."
);
working->flags = mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_WARM) | mfb(MF_BASHES);
working->fear = mfb(MTRIG_HURT) | mfb(MTRIG_FIRE);

mon("crawler mutant",species_none, 'S',	c_red,		MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2, 16, 40, 40, 80, 10,  2,  6,  0,  1,  8,  0,  0,180, 0,
	&mdeath::normal,	&mattack::none,	"\
Two or three humans fused together somehow,\n\
slowly dragging their thick-hided, hideous\n\
body across the ground."
);
working->flags = mfb(MF_POISON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_WARM) | mfb(MF_BASHES);
working->fear = mfb(MTRIG_HURT) | mfb(MTRIG_FIRE);

mon("sewer fish",species_none, 's',	c_ltgreen,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 30, 13,100,100,120, 17,  1,  3,  3,  6,  0,  0,  0, 20, 0,
	&mdeath::normal,	&mattack::none,	"\
A large green fish, it's mouth lined with\n\
three rows of razor-sharp teeth."
);
working->flags = mfb(MF_AQUATIC) | mfb(MF_SEES) | mfb(MF_SMELLS) | mfb(MF_WARM);
working->fear = mfb(MTRIG_HURT);

mon("sewer snake",species_none, 's',	c_yellow,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 15,  2, 20, 40, 60, 12,  1,  2,  5,  1,  0,  0,  0, 40, 0,
	&mdeath::normal,	&mattack::none,	"\
A large snake, turned pale yellow from its\n\
underground life."
);
working->flags = mfb(MF_SWIMS) | mfb(MF_VENOM) | mfb(MF_SEES) | mfb(MF_SMELLS) | mfb(MF_WARM) | mfb(MF_LEATHER);
working->fear = mfb(MTRIG_HURT);

mon("sewer rat",species_mammal, 's',	c_dkgray,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 18,  3, 20, 40,105, 10,  1,  2,  1,  2,  0,  0,  0, 30, 0,
	&mdeath::normal,	&mattack::none, "\
A large, mangy rat with red eyes.  It\n\
scampers quickly across the ground, squeaking\n\
hungrily."
);
working->flags = mfb(MF_SWIMS) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_ANIMAL) | mfb(MF_FUR) | mfb(MF_WARM);

mon("rat king",species_mammal, 'S',	c_dkgray,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0, 18, 10,100, 40,  4,  1,  3,  1,  0,  0,  0,  0,220, 3,
	&mdeath::ratking,	&mattack::ratking, "\
A group of several rats, their tails\n\
knotted together in a filthy mass.  A wave\n\
of nausea washes over you in its presence."
);

// SWAMP CREATURES
mon("giant mosquito",species_insect, 'y',c_ltgray,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 22, 12, 20, 20,120,  8,  1,  1,  1,  5,  0,  0,  0, 20, 0,
	&mdeath::normal,	&mattack::none, "\
An enormous mosquito, fluttering erratically,\n\
its face dominated by a long, spear-tipped\n\
proboscis."
);
working->flags = mfb(MF_HIT_AND_RUN) | mfb(MF_FLIES) | mfb(MF_VENOM) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_STUMBLES);

mon("giant dragonfly",species_insect, 'y',c_ltgreen,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  6, 13, 20,100,155, 12,  1,  3,  6,  5,  0,  6,-20, 70, 0,
	&mdeath::normal,	&mattack::none, "\
A ferocious airborne predator, flying swiftly\n\
through the air, its mouth a cluster of fangs."
);
working->flags = mfb(MF_HIT_AND_RUN) | mfb(MF_FLIES) | mfb(MF_SEES) | mfb(MF_SMELLS);

mon("giant centipede",species_insect, 'a',c_ltgreen,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  7,  9, 20,100,120, 10,  1,  3,  5,  2,  0,  8,-30, 60, 0,
	&mdeath::normal,	&mattack::none, "\
A meter-long centipede, moving swiftly on\n\
dozens of thin legs, a pair of venomous\n\
pincers attached to its head."
);
working->flags = mfb(MF_VENOM) | mfb(MF_POISON) | mfb(MF_HEARS) | mfb(MF_SMELLS);

mon("giant frog",species_none, 'F',	c_green,	MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  5, 10, 10,100, 90,  8,  2,  3,  0,  2,  4,  0,  0, 70, 5,
	&mdeath::normal,	&mattack::leap, "\
A thick-skinned green frog.  It eyes you\n\
much as you imagine it might eye an insect."
);
working->flags = mfb(MF_SWIMS) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_LEATHER);
working->fear = mfb(MTRIG_HURT);

mon("giant slug",species_none, 'S',	c_yellow,	MS_HUGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  4, 16, 20,100, 60,  7,  1,  5,  1,  0,  8,  2,  0,190, 10,
	&mdeath::normal,	&mattack::acid, "\
A gigantic slug, the size of a small car.\n\
It moves slowly, dribbling acidic goo from\n\
its fang-lined mouth."
);
working->flags = mfb(MF_ACIDTRAIL) | mfb(MF_ACIDPROOF) | mfb(MF_SEES) | mfb(MF_SMELLS) | mfb(MF_BASHES);
working->fear = mfb(MTRIG_PLAYER_CLOSE) | mfb(MTRIG_HURT);

mon("dermatik larva",species_insect, 'i',c_white,	MS_TINY,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  4,-20,-20, 20,  1,  1,  2,  2,  1,  0,  0,  0, 10, 0,
	&mdeath::normal,	&mattack::none, "\
A fat, white grub the size of your foot, with\n\
a set of mandibles that look more suited for\n\
digging than fighting."
);
working->flags = mfb(MF_DIGS) | mfb(MF_POISON) | mfb(MF_HEARS) | mfb(MF_SMELLS);

mon("dermatik",	species_insect, 'i',	c_red,		MS_TINY,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3, 18, 30, 30,100,  5,  1,  1,  6, 7,   0,  6,  0, 60, 50,
	&mdeath::normal,	&mattack::dermatik, "\
A wasp-like flying insect, smaller than most\n\
mutated wasps.  It does not looke very\n\
threatening, but has a large ovipositor in\n\
place of a sting."
);
working->flags = mfb(MF_FLIES) | mfb(MF_POISON) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_STUMBLES);


// SPIDERS
mon("wolf spider",species_insect, 's',	c_brown,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 20, 20,100,110, 7,   1,  1,  8,  6,  2,  8,-70, 40, 0,
	&mdeath::normal,	&mattack::none, "\
A large, brown spider, which moves quickly\n\
and aggressively."
);
working->flags = mfb(MF_VENOM) | mfb(MF_HEARS) | mfb(MF_SMELLS);

mon("web spider",species_insect, 's',	c_yellow,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 16, 30, 80,120,  5,  1,  1,  7,  5,  2,  7,-70, 35, 0,
	&mdeath::normal,	&mattack::none, "\
A yellow spider the size of a dog.  It lives\n\
in webs, waiting for prey to become\n\
entangled before pouncing and biting."
);
working->flags = mfb(MF_WEBWALK) | mfb(MF_VENOM) | mfb(MF_HEARS) | mfb(MF_SMELLS);

mon("jumping spider",species_insect, 's',c_white,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2, 14, 40, 80,100,  7,  1,  1,  4,  8,  0,  3,-60, 30, 2,
	&mdeath::normal,	&mattack::leap, "\
A small, almost cute-looking spider.  It\n\
leaps so quickly that it almost appears to\n\
instantaneously move from one place to\n\
another."
);
working->flags = mfb(MF_HIT_AND_RUN) | mfb(MF_VENOM) | mfb(MF_HEARS) | mfb(MF_SMELLS);

mon("trap door spider",species_insect, 's',c_blue,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 20, 60,100,110,  5,  1,  2,  7,  3,  2,  8,-80, 70, 0,
	&mdeath::normal,	&mattack::none, "\
A large spider with a bulbous thorax.  It\n\
creates a subterranean nest and lies in\n\
wait for prey to fall in and become trapped\n\
in its webs."
);
working->flags = mfb(MF_WEBWALK) | mfb(MF_VENOM) | mfb(MF_HEARS) | mfb(MF_SMELLS);

mon("black widow",species_insect, 's',	c_dkgray,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 20,-10,100, 90,  6,  1,  1,  6,  3,  0,  3,-50, 40, 0,
	&mdeath::normal,	&mattack::none, "\
A spider with a characteristic red\n\
hourglass on its black carapace.  It is\n\
known for its highly toxic venom."
);
working->flags = mfb(MF_WEBWALK) | mfb(MF_BADVENOM) | mfb(MF_HEARS) | mfb(MF_SMELLS);
working->anger = mfb(MTRIG_PLAYER_WEAK) | mfb(MTRIG_PLAYER_CLOSE) | mfb(MTRIG_HURT);

// UNEARTHED HORRORS
mon("dark wyrm",species_none, 'S',	c_blue,		MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 20,100,100,100,  8,  2,  6,  4,  4,  6,  0,  0,120, 0,
	&mdeath::normal,	&mattack::none, "\
A huge, black worm, its flesh glistening\n\
with an acidic, blue slime.  It has a gaping\n\
round mouth lined with dagger-like teeth."
);
working->flags = mfb(MF_SUNDEATH) | mfb(MF_DESTROYS) | mfb(MF_ACIDTRAIL) | mfb(MF_ACIDPROOF) | mfb(MF_POISON)
               | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_GOODHEARING);

mon("Amigara horror",species_none, 'H',	c_white,	MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 30,100,100, 70, 10,  2,  4,  0,  2,  0,  0,  0,250, 0,
	&mdeath::amigara,	&mattack::fear_paralyze, "\
A spindly body, standing at least 15 feet\n\
tall.  It looks vaguely human, but its face is\n\
grotesquely stretched out, and its limbs are\n\
distorted to the point of being tentacles."
);
working->flags = mfb(MF_HARDTOSHOOT) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_STUMBLES);

// This "dog" is, in fact, a disguised Thing
mon("dog",	species_nether, 'd',	c_white,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3,  5,100,100,150, 12,  2,  3,  3,  3,  0,  0,  0, 25, 40,
	&mdeath::thing,		&mattack::dogthing, "\
A medium-sized domesticated dog, gone feral."
);
working->flags = mfb(MF_FRIENDLY_SPECIAL) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_ANIMAL) | mfb(MF_FUR) | mfb(MF_WARM);

mon("tentacle dog",species_nether, 'd',	c_dkgray,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 14,100,100,120, 12,  2,  4,  0,  3,  0,  0,  0,120, 5,
	&mdeath::thing,		&mattack::tentacle, "\
A dog's body with a mass of ropy, black\n\
tentacles extending from its head."
);
working->flags = mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_BASHES);

mon("Thing",	species_nether, 'H',	c_dkgray,	MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 25,100,100,135, 14,  2,  4,  0,  5,  8,  0,  0,160, 5,
	&mdeath::melt,		&mattack::tentacle, "\
An amorphous black creature which seems to\n\
sprout tentacles rapidly."
);
working->flags = mfb(MF_NOHEAD) | mfb(MF_PLASTIC) | mfb(MF_ACIDPROOF) | mfb(MF_ATTACKMON) | mfb(MF_SWIMS)
               | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_BASHES);

mon("human snail",species_none, 'h',	c_green,	MS_LARGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 20, 10,  0, 30, 50,  4,  1,  5,  0,  0,  6, 12,  0, 50, 15,
	&mdeath::normal,	&mattack::acid, "\
A large snail, with an oddly human face."
);
working->flags = mfb(MF_ACIDTRAIL) | mfb(MF_ACIDPROOF) | mfb(MF_POISON) | mfb(MF_HEARS) | mfb(MF_SMELLS);
working->anger = mfb(MTRIG_PLAYER_WEAK) | mfb(MTRIG_FRIEND_DIED);
working->fear = mfb(MTRIG_PLAYER_CLOSE) | mfb(MTRIG_HURT);

mon("twisted body",species_none, 'h',	c_pink,		MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  5, 12,100,100, 90,  5,  2,  4,  0,  6,  0,  0,  0, 65, 0,
	&mdeath::normal,	&mattack::none, "\
A human body, but with its limbs, neck, and\n\
hair impossibly twisted."
);
working->flags = mfb(MF_POISON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_GOODHEARING);

mon("vortex",	species_none, 'v',	c_white,	MS_SMALL,	POWDER,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2, 30,100,100,120,  0,  0,  0,  0,  0,  0,  0,  0, 20, 6,
	&mdeath::melt,		&mattack::vortex, "\
A twisting spot in the air, with some kind\n\
of morphing mass at its center."
);
working->flags = mfb(MF_NOHEAD) | mfb(MF_FRIENDLY_SPECIAL) | mfb(MF_HARDTOSHOOT) | mfb(MF_PLASTIC) | mfb(MF_FLIES)
               | mfb(MF_HEARS) | mfb(MF_GOODHEARING) | mfb(MF_STUMBLES);

// NETHER WORLD INHABITANTS
mon("flying polyp",species_nether, 'H',	c_dkgray,	MS_HUGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 42,100,100,280, 16,  3,  8,  6,  7,  8,  0,  0,350, 0,
	&mdeath::melt,		&mattack::none, "\
An amorphous mass of twisting black flesh\n\
that flies through the air swiftly."
);
working->flags = mfb(MF_NOHEAD) | mfb(MF_PLASTIC) | mfb(MF_ATTACKMON) | mfb(MF_HIT_AND_RUN) | mfb(MF_FLIES)
               | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_GOODHEARING) | mfb(MF_BASHES);

mon("hunting horror",species_nether, 'h',c_dkgray,	MS_SMALL,	MNULL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 10, 28,100,100,180, 15,  3,  4,  0,  6,  0,  0,  0, 80, 0,
	&mdeath::melt,		&mattack::none, "\
A ropy, worm-like creature that flies on\n\
bat-like wings. Its form continually\n\
shifts and changes, twitching and\n\
writhing."
);
working->flags = mfb(MF_SUNDEATH) | mfb(MF_NOHEAD) | mfb(MF_HARDTOSHOOT) | mfb(MF_PLASTIC)
               | mfb(MF_HIT_AND_RUN) | mfb(MF_FLIES) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS);

mon("Mi-go",	species_nether, 'H',	c_pink,		MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  5, 26, 20, 30,120, 14,  5,  3, 10,  7,  4, 12,  0,110, 0,
	&mdeath::normal,	&mattack::none, "\
A pinkish, fungoid crustacean-like\n\
creature with numerous pairs of clawed\n\
appendages and a head covered with waving\n\
antennae."
);
working->flags = mfb(MF_POISON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_WARM) | mfb(MF_BASHES);

mon("yugg",	species_nether, 'H',	c_white,	MS_HUGE,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3, 32,100,100, 80, 12,  3,  5,  8,  1,  6,  0,  0,320, 20,
	&mdeath::normal,	&mattack::gene_sting, "\
An enormous white flatworm writhing\n\
beneath the earth. Poking from the\n\
ground is a bulbous head dominated by a\n\
pink mouth, lined with rows of fangs."
);
working->flags = mfb(MF_DIGS) | mfb(MF_DESTROYS) | mfb(MF_VENOM) | mfb(MF_POISON)
               | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_BASHES);

mon("gelatinous blob",species_nether, 'O',c_ltgray,	MS_LARGE,	LIQUID,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  5, 20,  0,100, 40,  8,  2,  3,  0,  0, 10,  0,  0,200, 4,
	&mdeath::melt,		&mattack::formblob, "\
A shapeless blob the size of a cow.  It\n\
oozes slowly across the ground, small\n\
chunks falling off of its sides."
);
working->flags = mfb(MF_NOHEAD) | mfb(MF_PLASTIC) | mfb(MF_HEARS) | mfb(MF_SMELLS);

mon("flaming eye",species_nether, 'E',	c_red,		MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  5, 27,  0,100, 90,  0,  0,  0,  0,  1,  3,  0,  0,300, 12,
	&mdeath::normal,	&mattack::stare, "\
An eyeball the size of an easy chair and\n\
covered in rolling blue flames. It floats\n\
through the air."
);
working->flags = mfb(MF_FIREY) | mfb(MF_NOHEAD) | mfb(MF_FLIES) | mfb(MF_SEES) | mfb(MF_WARM);

mon("kreck",	species_nether, 'h',	c_ltred,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  9,  6,100,100,105,  6,  1,  3,  1,  5,  0,  5,  0, 35, 0,
	&mdeath::melt,		&mattack::none, "\
A small humanoid, the size of a dog, with\n\
twisted red flesh and a distended neck. It\n\
scampers across the ground, panting and\n\
grunting."
);
working->flags = mfb(MF_HIT_AND_RUN) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_WARM) | mfb(MF_BASHES);

mon("blank body",species_nether, 'h',	c_white,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3,  5,  0,100, 80,  9,  1,  4,  0,  1,  0,  0,  0,100, 10,
	&mdeath::normal,	&mattack::shriek, "\
This looks like a human body, but its\n\
flesh is snow-white and its face has no\n\
features save for a perfectly round\n\
mouth."
);
working->flags = mfb(MF_SUNDEATH) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_ANIMAL) | mfb(MF_WARM);

mon("Gozu",	species_nether, 'G',	c_white,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 1,  20, 10,100, 80, 12,  2,  5,  0,  5,  0,  0,  0,400, 20,
	&mdeath::normal,	&mattack::fear_paralyze, "\
A beast with the body of a slightly-overweight\n\
man and the head of a cow.  It walks slowly,\n\
milky white drool dripping from its mouth,\n\
wearing only a pair of white underwear."
);
working->flags = mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_ANIMAL) | mfb(MF_FUR) | mfb(MF_WARM) | mfb(MF_BASHES);

mon("shadow",	species_nether,'S',	c_dkgray,	MS_TINY,	MNULL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0, 14,100,100, 90,  6,  1,  2,  0,  4,  0,  0,  0, 60, 20,
	&mdeath::melt,		&mattack::disappear, "\
A strange dark area in the area.  It whispers\n\
softly as it moves."
);
working->flags = mfb(MF_SUNDEATH) | mfb(MF_NOHEAD) | mfb(MF_ELECTRIC) | mfb(MF_GRABS) | mfb(MF_WEBWALK) | mfb(MF_ACIDPROOF)
               | mfb(MF_HARDTOSHOOT) | mfb(MF_PLASTIC) | mfb(MF_HIT_AND_RUN) | mfb(MF_FLIES)
	           | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_GOODHEARING);

// The hub
mon("breather",	species_nether,'O',	c_pink,		MS_MEDIUM,	MNULL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  2,  0,  0,100,  0,  0,  0,  0,  0,  0,  0,  0,100, 6,
	&mdeath::kill_breathers,&mattack::breathe, "\
A strange, immobile pink goo.  It seems to\n\
be breathing slowly."
);
working->flags = mfb(MF_IMMOBILE);

// The branches
mon("breather",	species_nether,'o',	c_pink,		MS_MEDIUM,	MNULL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  2,  0,  0,100,  0,  0,  0,  0,  0,  0,  0,  0,100, 6,
	&mdeath::melt,		&mattack::breathe, "\
A strange, immobile pink goo.  It seems to\n\
be breathing slowly."
);
working->flags = mfb(MF_IMMOBILE);

mon("shadow snake",species_none, 's',	c_dkgray,	MS_SMALL,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  6,100,100, 90, 12,  1,  4,  5,  1,  0,  0,  0, 40, 20,
	&mdeath::melt,		&mattack::disappear, "\
A large snake, translucent black.");
working->flags = mfb(MF_SUNDEATH) | mfb(MF_PLASTIC) | mfb(MF_SWIMS) | mfb(MF_SEES) | mfb(MF_SMELLS) | mfb(MF_WARM) | mfb(MF_LEATHER);

// ROBOTS
mon("eyebot",	species_robot, 'r',	c_ltblue,	MS_SMALL,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 20,  2,  0,100,120, 0,  0,  0,  0,  3,  10, 10, 70,  20, 30,
	&mdeath::normal,	&mattack::photograph, "\
A roughly spherical robot that hovers about\n\
five feet of the ground.  Its front side is\n\
dominated by a huge eye and a flash bulb.\n\
Frequently used for reconnaissance."
);
working->flags = mfb(MF_NOHEAD) | mfb(MF_ELECTRONIC) | mfb(MF_FLIES) | mfb(MF_SEES);

mon("manhack",	species_robot, 'r',	c_green,	MS_TINY,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 18,  7,100,100,130, 12,  1,  1,  8,  4,  0,  6, 10, 15, 0,
	&mdeath::normal,	&mattack::none, "\
A fist-sized robot that flies swiftly through\n\
the air.  It's covered with whirring blades\n\
and has one small, glowing red eye."
);
working->flags = mfb(MF_NOHEAD) | mfb(MF_ELECTRONIC) | mfb(MF_HIT_AND_RUN) | mfb(MF_FLIES) | mfb(MF_SEES);

mon("skitterbot",species_robot, 'r',	c_ltred,	MS_SMALL,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	 10, 13,100,100,105,  0,  0,  0,  0,  0, 12, 12, 60, 40, 5,
	&mdeath::normal,	&mattack::tazer, "\
A robot with an insectoid design, about\n\
the size of a small dog.  It skitters\n\
quickly across the ground, two electric\n\
prods at the ready."
);
working->flags = mfb(MF_ELECTRONIC) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_GOODHEARING);

mon("secubot",	species_robot, 'R',	c_dkgray,	MS_SMALL,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  7, 19,100,100, 70,  0,  0,  0,  0,  0, 14, 14, 80, 80, 2,
	&mdeath::explode,	&mattack::smg, "\
A boxy robot about four feet high.  It moves\n\
slowly on a set of treads, and is armed with\n\
a large machine gun type weapon.  It is\n\
heavily armored."
);
working->flags = mfb(MF_ELECTRONIC) | mfb(MF_ATTACKMON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_BASHES);

mon("copbot",	species_robot, 'R',	c_dkgray,	MS_MEDIUM,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0, 12,100, 40,100,  4,  3,  2,  0,  8, 12,  8, 80, 80, 3,
	&mdeath::normal,	&mattack::copbot, "\
A blue-painted robot that moves quickly on a\n\
set of three omniwheels.  It has a nightstick\n\
readied, and appears to be well-armored."
);
working->flags = mfb(MF_ELECTRONIC) | mfb(MF_ATTACKMON) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_BASHES);

mon("molebot",	species_robot, 'R',	c_brown,	MS_MEDIUM,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  2, 17,100,100, 40, 13,  1,  4, 10,  0, 14, 14, 82, 80, 0,
	&mdeath::normal,	&mattack::none,	"\
A snake-shaped robot that tunnels through the\n\
ground slowly.  When it emerges from the\n\
ground it can attack with its large, spike-\n\
covered head."
);
working->flags = mfb(MF_DIGS) | mfb(MF_ELECTRONIC) | mfb(MF_HEARS) | mfb(MF_GOODHEARING);

mon("tripod robot",species_robot, 'R',	c_white,	MS_LARGE,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  5, 26,100,100, 90, 15,  2,  4,  7,  0, 12,  8, 82, 80, 10,
	&mdeath::normal,	&mattack::flamethrower, "\
A 8-foot-tall robot that walks on three long\n\
legs.  It has a pair of spiked tentacles, as\n\
well as a flamethrower mounted on its head."
);
working->flags = mfb(MF_ELECTRONIC) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_GOODHEARING) | mfb(MF_BASHES);

mon("chicken walker",species_robot, 'R',c_red,		MS_LARGE,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  3, 32,100,100,115, 0,  0,  0,  0,  0,  18, 14, 85, 90, 5,
	&mdeath::explode,	&mattack::smg, "\
A 10-foot-tall, heavily-armored robot that\n\
walks on a pair of legs with the knees\n\
facing backwards.  It's armed with a\n\
nasty-looking machine gun."
);
working->flags = mfb(MF_ELECTRONIC) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_BASHES);

mon("tankbot",	species_robot, 'R',	c_blue,		MS_HUGE,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  1, 52,100,100,100, 0,  0,  0,  0,  0,  22, 20, 92,240, 4,
	&mdeath::normal,	&mattack::multi_robot, "\
This fearsome robot is essentially an\n\
autonomous tank.  It moves surprisingly fast\n\
on its treads, and is armed with a variety of\n\
deadly weapons."
);
working->flags = mfb(MF_NOHEAD) | mfb(MF_ELECTRONIC) | mfb(MF_DESTROYS) | mfb(MF_ATTACKMON)
               | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_GOODHEARING) | mfb(MF_BASHES);

mon("turret",	species_robot, 't',	c_ltgray,	MS_SMALL,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0, 14,100,100,100,  0,  0,  0,  0,  0, 14, 16, 88, 30, 1,
	&mdeath::explode,	&mattack::smg, "\
A small, round turret which extends from\n\
the floor.  Two SMG barrels swivel 360\n\
degrees."
);
working->flags = mfb(MF_IMMOBILE) | mfb(MF_NOHEAD) | mfb(MF_FRIENDLY_SPECIAL) | mfb(MF_ELECTRONIC) | mfb(MF_SEES);

mon("exploder",	species_robot, 'm',	c_ltgray,	MS_LARGE,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0, 14,100,100,100,  0,  0,  0,  0,  0,  0,  0, 88,  1, 1,
	&mdeath::explode,	&mattack::none, "\
A small, round turret which extends from\n\
the floor.  Two SMG barrels swivel 360\n\
degrees."
);
working->flags = mfb(MF_IMMOBILE);


// HALLUCINATIONS
mon("zombie",	species_hallu, 'Z',	c_ltgreen,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  0,100,100, 65,  3,  0,  0,  0,  0,  0,  0,  0,  1,  20,
	&mdeath::disappear,	&mattack::disappear, "\
A human body, stumbling slowly forward on\n\
uncertain legs, possessed with an\n\
unstoppable rage."
);
working->flags = mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS) | mfb(MF_STUMBLES);

mon("giant bee",species_hallu, 'a',	c_yellow,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  0,100,100,180,  2,  0,  0,  0,  0,  0,  0,  0,  1,  20,
	&mdeath::disappear,	&mattack::disappear, "\
A honey bee the size of a small dog. It\n\
buzzes angrily through the air, dagger-\n\
sized sting pointed forward."
);
working->flags = mfb(MF_FLIES) | mfb(MF_SMELLS);

mon("giant ant",species_hallu, 'a',	c_brown,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  0,100,100,100,  3,  0,  0,  0,  0,  0,  0,  0,  1,  20,
	&mdeath::disappear,	&mattack::disappear, "\
A red ant the size of a crocodile. It is\n\
covered in chitinous armor, and has a\n\
pair of vicious mandibles."
);
working->flags = mfb(MF_SMELLS);

mon("your mother",species_hallu, '@',	c_white,	MS_MEDIUM,	FLESH,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  0,100,100,100,  3,  0,  0,  0,  0,  0,  0,  0,  5,  20,
	&mdeath::disappear,	&mattack::disappear, "\
Mom?"
);
working->flags = mfb(MF_GUILT) | mfb(MF_SEES) | mfb(MF_HEARS) | mfb(MF_SMELLS);

mon("generator", species_none, 'G',	c_white,	MS_LARGE,	STEEL,
//	frq dif agr mor spd msk mdi m## cut dge bsh cut itm  HP special freq
	  0,  0,  0,  0,100,  0,  0,  0,  0,  0,  2,  2,  0,500, 1,
	&mdeath::gameover,	&mattack::generator, "\
Your precious generator, noisily humming\n\
away.  Defend it at all costs!"
);
working->flags = mfb(MF_IMMOBILE) | mfb(MF_NOHEAD) | mfb(MF_ACIDPROOF);

	assert(num_monsters == types.size());	// postcondition check

{	// install monster tiles
	if (!JSON::cache.count("tiles")) return;
	auto& j_tiles = JSON::cache["tiles"];
	auto keys = j_tiles.keys();
	JSON_parse<mon_id> parse;
	while (!keys.empty()) {
		auto& k = keys.back();
		if (const auto id = parse(k)) {
			auto& src = j_tiles[k];
			// invariant: src.is_scalar()
			tiles[id] = src.scalar();	// XXX \todo would be nice if this was a non-const reference return so std::move was an option
			j_tiles.unset(k);
		}
		keys.pop_back();	// reference k dies
	}
	if (j_tiles.empty()) JSON::cache.erase("tiles");	// reference j_tiles dies
}

}
