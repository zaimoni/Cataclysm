#include "mongroup.h"
#include "setvector.h"
#include <string.h>

static const char* const JSON_transcode[] = {
	"forest",
	"ant",
	"bee",
	"worm",
	"zombie",
	"triffid",
	"fungi",
	"goo",
	"chud",
	"sewer",
	"swamp",
	"lab",
	"nether",
	"spiral",
	"vanilla_zombie",
	"spider",
	"robot"
};

const char* JSON_key(moncat_id src)
{
	if (src) return JSON_transcode[src - 1];
	return 0;
}

using namespace cataclysm;

moncat_id JSON_parse<moncat_id>::operator()(const char* const src)
{
	if (!src || !src[0]) return mcat_null;
	ptrdiff_t i = sizeof(JSON_transcode) / sizeof(*JSON_transcode);
	while (0 < i--) {
		if (!strcmp(JSON_transcode[i], src)) return (moncat_id)(i + 1);
	}
	return mcat_null;
}

std::vector<mon_id> mongroup::moncats[num_moncats];

void mongroup::init()
{
 SET_VECTOR(moncats[mcat_forest],
	mon_squirrel, mon_rabbit, mon_deer, mon_wolf, mon_bear, mon_spider_wolf,
	mon_spider_jumping, mon_dog);
 SET_VECTOR(moncats[mcat_ant],
	mon_ant_larva, mon_ant, mon_ant_soldier, mon_ant_queen);
 SET_VECTOR(moncats[mcat_bee], mon_bee);
 SET_VECTOR(moncats[mcat_worm], mon_graboid, mon_worm, mon_halfworm);
 SET_VECTOR(moncats[mcat_zombie],
	mon_zombie, mon_zombie_shrieker, mon_zombie_spitter, mon_zombie_fast,
	mon_zombie_electric, mon_zombie_brute, mon_zombie_hulk,
	mon_zombie_necro, mon_boomer, mon_skeleton, mon_zombie_grabber,
	mon_zombie_master);
 SET_VECTOR(moncats[mcat_triffid],
	mon_triffid, mon_triffid_young, mon_vinebeast, mon_triffid_queen);
 SET_VECTOR(moncats[mcat_fungi],
	mon_fungaloid, mon_fungaloid_dormant, mon_ant_fungus, mon_zombie_fungus,
	mon_boomer_fungus, mon_spore, mon_fungaloid_queen, mon_fungal_wall);
 SET_VECTOR(moncats[mcat_goo], mon_blob);
 SET_VECTOR(moncats[mcat_chud], mon_chud, mon_one_eye, mon_crawler);
 SET_VECTOR(moncats[mcat_sewer],
	mon_sewer_fish, mon_sewer_snake, mon_sewer_rat);
 SET_VECTOR(moncats[mcat_swamp],
	mon_mosquito, mon_dragonfly, mon_centipede, mon_frog, mon_slug,
	mon_dermatik_larva, mon_dermatik);
 SET_VECTOR(moncats[mcat_lab],
	mon_zombie_scientist, mon_blob_small, mon_manhack, mon_skitterbot);
 SET_VECTOR(moncats[mcat_nether],
	mon_flying_polyp, mon_hunting_horror, mon_mi_go, mon_yugg, mon_gelatin,
	mon_flaming_eye, mon_kreck, mon_blank, mon_gozu);
 SET_VECTOR(moncats[mcat_spiral], mon_human_snail, mon_twisted_body, mon_vortex);
 SET_VECTOR(moncats[mcat_vanilla_zombie], mon_zombie);
 SET_VECTOR(moncats[mcat_spider],
	mon_spider_wolf, mon_spider_web, mon_spider_jumping, mon_spider_widow);
 SET_VECTOR(moncats[mcat_robot],
	mon_manhack, mon_skitterbot, mon_secubot, mon_copbot, mon_molebot,
	mon_tripod, mon_chickenbot, mon_tankbot);
}

moncat_id mongroup::to_mc(mon_id type)
{
	for (int i = 0; i < num_moncats; i++) {
		for (auto tmp_monid : moncats[i]) {
			if (tmp_monid == type) return (moncat_id)(i);
		}
	}
	return mcat_null;
}
