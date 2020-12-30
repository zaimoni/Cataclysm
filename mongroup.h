#ifndef _MONGROUP_H_
#define _MONGROUP_H_

#include "mtype.h"
#include "enums.h"

enum moncat_id {
 mcat_null = 0,
 mcat_forest,
 mcat_ant,
 mcat_bee,
 mcat_worm,
 mcat_zombie,
 mcat_triffid,
 mcat_fungi,
 mcat_goo,
 mcat_chud,
 mcat_sewer,
 mcat_swamp,
 mcat_lab,
 mcat_nether,
 mcat_spiral,
 mcat_vanilla_zombie,	// Defense mode only
 mcat_spider,		// Defense mode only
 mcat_robot,		// Defense mode only
 num_moncats
};

DECLARE_JSON_ENUM_SUPPORT(moncat_id)

struct mongroup {
 static std::vector<mon_id> moncats[num_moncats];

 moncat_id type;
 point pos;		// coordinate range 0...2*OMAPX/Y-1
 unsigned char radius;
 unsigned int population;
 bool dying;

 mongroup() noexcept : type(mcat_null), pos(-1, -1), radius(0), population(0), dying(false) {};	// \todo consider defaulting this
 mongroup(moncat_id ptype, int pposx, int pposy, unsigned char prad, unsigned int ppop) noexcept
 : type(ptype),pos(pposx,pposy),radius(prad),population(ppop),dying(false) {}
 mongroup(moncat_id ptype, const point& ppos, unsigned char prad, unsigned int ppop) noexcept
	 : type(ptype), pos(ppos), radius(prad), population(ppop), dying(false) {}


 friend bool fromJSON(const cataclysm::JSON& _in, mongroup& dest);
 friend cataclysm::JSON toJSON(const mongroup& src);

 bool add_one();
 bool is_safe() const { return mcat_null == type || mcat_forest == type; }
 static moncat_id to_mc(mon_id type);	// Monster type to monster category
 static void init();
};

#endif
