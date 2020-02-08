#include "overmap.h"
#include "game.h"
#include "keypress.h"
#include "saveload.h"
#include "JSON.h"
#include "om_cache.hpp"
#include "stl_limits.h"
#include "line.h"
#include "rng.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <fstream>
#include <sstream>

#include "Zaimoni.STL/GDI/box.hpp"

GPS_loc overmap::toGPS(const point& screen_pos) { return game::active()->toGPS(screen_pos); }

OM_loc overmap::toOvermap(const GPS_loc GPSpos)
{
	OM_loc ret(GPSpos.first,point(0));
	ret.second.x = (ret.first.x % (2 * OMAP));
	ret.second.y = (ret.first.y % (2 * OMAP));
	if (0 <= ret.first.x) {
		ret.first.x /= (2 * OMAP);
	} else {
		ret.first.x = ((ret.first.x + 1) / (2 * OMAP)) - 1;
		assert(0 >= ret.second.x);	// \todo remove this once live (test of algorithmic correctness)
		ret.second.x += (2 * OMAP -1);
	}
	if (0 <= ret.first.y) {
		ret.first.y /= (2 * OMAP);
	}
	else {
		ret.first.y = ((ret.first.y + 1) / (2 * OMAP)) - 1;
		assert(0 >= ret.second.y);	// \todo remove this once live (test of algorithmic correctness)
		ret.second.y += (2 * OMAP - 1);
	}
	ret.second /= 2;
	return ret;
}

// start inlined GPS_loc.cpp
void OM_loc::self_normalize()
{
    while (0 > second.x && INT_MIN < first.x) {
        second.x += OMAPX;
        first.x--;
    };
    while (OMAPX <= second.x && INT_MAX > first.x) {
        second.x -= OMAPX;
        first.x++;
    };
    while (0 > second.y && INT_MIN < first.y) {
        second.y += OMAPY;
        first.y--;
    };
    while (OMAPY <= second.y && INT_MAX > first.y) {
        second.y -= OMAPY;
        first.y++;
    };
}

OM_loc overmap::normalize(const OM_loc& OMpos)
{
    OM_loc ret(OMpos);
    ret.self_normalize();
    return ret;
}

void OM_loc::self_denormalize(const tripoint& view)
{
#define DENORMALIZE_COORD(X)    \
    if (view.X < first.X) {   \
        do second.X += OMAP; \
        while (view.X < --first.X);   \
    } else if (view.X > first.X) {    \
        do second.X -= OMAP; \
        while (view.X > ++first.X);   \
    }

    DENORMALIZE_COORD(x)
    DENORMALIZE_COORD(y)
#undef DENORMALIZE_COORD
}


bool OM_loc::is_valid() const
{
    if (_ref<OM_loc>::invalid.first == first) return zaimoni::gdi::box(point(0), point(OMAP)).contains(second);
    return true;
}

// would prefer for this to be a free function but we have to have some way to distinguish between overmap and GPS coordinates
int rl_dist(OM_loc lhs, OM_loc rhs)
{
    if (!lhs.is_valid() || !rhs.is_valid()) return INT_MAX;

    if (lhs.first == rhs.first) return rl_dist(lhs.second, rhs.second);

    // release block \todo expand following in a non-overflowing way (always return INT_MAX if overflow)
    // prototype reductions (can overflow)
    lhs.self_normalize();
    rhs.self_normalize();
    rhs.self_denormalize(lhs.first);
    return rl_dist(lhs.second, rhs.second);
}


#define STREETCHANCE 2
#define NUM_FOREST 250
#define TOP_HIWAY_DIST 140
#define MIN_ANT_SIZE 8
#define MAX_ANT_SIZE 20
#define MIN_GOO_SIZE 1
#define MAX_GOO_SIZE 2
#define MIN_RIFT_SIZE 6
#define MAX_RIFT_SIZE 16
#define SETTLE_DICE 2
#define SETTLE_SIDES 2
#define HIVECHANCE 180	//Chance that any given forest will be a hive
#define SWAMPINESS 8	//Affects the size of a swamp
#define SWAMPCHANCE 850	// 1/SWAMPCHANCE Chance that a swamp will spawn instead of forest

using namespace cataclysm;

template<> bool discard<bool>::x = false;
template<> oter_id discard<oter_id>::x = ot_null;

const map_extras no_extras(0);
const map_extras road_extras(
	// %%% HEL MIL SCI STA DRG SUP PRT MIN WLF PUD CRT FUM 1WY ART
	50, 40, 50, 120, 200, 30, 10, 5, 80, 20, 200, 10, 8, 2, 3);
const map_extras field_extras(
	60, 40, 15, 40, 80, 10, 10, 3, 50, 30, 300, 10, 8, 1, 3);
const map_extras subway_extras(
	// %%% HEL MIL SCI STA DRG SUP PRT MIN WLF PUD CRT FUM 1WY ART
	75, 0, 5, 12, 5, 5, 0, 7, 0, 0, 120, 0, 20, 1, 3);
const map_extras build_extras(
	90, 0, 5, 12, 0, 10, 0, 5, 5, 0, 0, 60, 8, 1, 3);

// LINE_**** corresponds to the ACS_**** macros in ncurses, and are patterned
// the same way; LINE_NESW, where X indicates a line and O indicates no line
// (thus, LINE_OXXX looks like 'T'). LINE_ is defined in output.h.  The ACS_
// macros can't be used here, since ncurses hasn't been initialized yet.

// Order MUST match enum oter_id above!

const oter_t oter_t::list[num_ter_types] = {
	{ "nothing",		'%',	c_white,	0, no_extras, false, false },
{ "crater",		'O',	c_red,		2, field_extras, false, false },
{ "field",		'.',	c_brown,	2, field_extras, false, false },
{ "forest",		'F',	c_green,	3, field_extras, false, false },
{ "forest",		'F',	c_green,	4, field_extras, false, false },
{ "swamp",		'F',	c_cyan,		4, field_extras, false, false },
{ "bee hive",		'8',	c_yellow,	3, field_extras, false, false },
{ "forest",		'F',	c_green,	3, field_extras, false, false },
/* The tile above is a spider pit. */
{ "fungal bloom",	'T',	c_ltgray,	2, field_extras, false, false },
{ "highway",		'H',	c_dkgray,	2, road_extras, false, false },
{ "highway",		'=',	c_dkgray,	2, road_extras, false, false },
{ "BUG",			'%',	c_magenta,	0, no_extras, false, false },
{ "road",          LINE_XOXO,	c_dkgray,	2, road_extras, false, false },
{ "road",          LINE_OXOX,	c_dkgray,	2, road_extras, false, false },
{ "road",          LINE_XXOO,	c_dkgray,	2, road_extras, false, false },
{ "road",          LINE_OXXO,	c_dkgray,	2, road_extras, false, false },
{ "road",          LINE_OOXX,	c_dkgray,	2, road_extras, false, false },
{ "road",          LINE_XOOX,	c_dkgray,	2, road_extras, false, false },
{ "road",          LINE_XXXO,	c_dkgray,	2, road_extras, false, false },
{ "road",          LINE_XXOX,	c_dkgray,	2, road_extras, false, false },
{ "road",          LINE_XOXX,	c_dkgray,	2, road_extras, false, false },
{ "road",          LINE_OXXX,	c_dkgray,	2, road_extras, false, false },
{ "road",          LINE_XXXX,	c_dkgray,	2, road_extras, false, false },
{ "road, manhole", LINE_XXXX,	c_yellow,	2, road_extras, true, false },
{ "bridge",		'|',	c_dkgray,	2, road_extras, false, false },
{ "bridge",		'-',	c_dkgray,	2, road_extras, false, false },
{ "river",		'R',	c_blue,		1, no_extras, false, false },
{ "river bank",		'R',	c_ltblue,	1, no_extras, false, false },
{ "river bank",		'R',	c_ltblue,	1, no_extras, false, false },
{ "river bank",		'R',	c_ltblue,	1, no_extras, false, false },
{ "river bank",		'R',	c_ltblue,	1, no_extras, false, false },
{ "river bank",		'R',	c_ltblue,	1, no_extras, false, false },
{ "river bank",		'R',	c_ltblue,	1, no_extras, false, false },
{ "river bank",		'R',	c_ltblue,	1, no_extras, false, false },
{ "river bank",		'R',	c_ltblue,	1, no_extras, false, false },
{ "river bank",		'R',	c_ltblue,	1, no_extras, false, false },
{ "river bank",		'R',	c_ltblue,	1, no_extras, false, false },
{ "river bank",		'R',	c_ltblue,	1, no_extras, false, false },
{ "river bank",		'R',	c_ltblue,	1, no_extras, false, false },
{ "house",		'^',	c_ltgreen,	5, build_extras, false, false },
{ "house",		'>',	c_ltgreen,	5, build_extras, false, false },
{ "house",		'v',	c_ltgreen,	5, build_extras, false, false },
{ "house",		'<',	c_ltgreen,	5, build_extras, false, false },
{ "house",		'^',	c_ltgreen,	5, build_extras, false, false },
{ "house",		'>',	c_ltgreen,	5, build_extras, false, false },
{ "house",		'v',	c_ltgreen,	5, build_extras, false, false },
{ "house",		'<',	c_ltgreen,	5, build_extras, false, false },
{ "parking lot",		'O',	c_dkgray,	1, build_extras, false, false },
{ "park",		'O',	c_green,	2, build_extras, false, false },
{ "gas station",		'^',	c_ltblue,	5, build_extras, false, false },
{ "gas station",		'>',	c_ltblue,	5, build_extras, false, false },
{ "gas station",		'v',	c_ltblue,	5, build_extras, false, false },
{ "gas station",		'<',	c_ltblue,	5, build_extras, false, false },
{ "pharmacy",		'^',	c_ltred,	5, build_extras, false, false },
{ "pharmacy",		'>',	c_ltred,	5, build_extras, false, false },
{ "pharmacy",		'v',	c_ltred,	5, build_extras, false, false },
{ "pharmacy",		'<',	c_ltred,	5, build_extras, false, false },
{ "grocery store",	'^',	c_green,	5, build_extras, false, false },
{ "grocery store",	'>',	c_green,	5, build_extras, false, false },
{ "grocery store",	'v',	c_green,	5, build_extras, false, false },
{ "grocery store",	'<',	c_green,	5, build_extras, false, false },
{ "hardware store",	'^',	c_cyan,		5, build_extras, false, false },
{ "hardware store",	'>',	c_cyan,		5, build_extras, false, false },
{ "hardware store",	'v',	c_cyan,		5, build_extras, false, false },
{ "hardware store",	'<',	c_cyan,		5, build_extras, false, false },
{ "electronics store",   '^',	c_yellow,	5, build_extras, false, false },
{ "electronics store",   '>',	c_yellow,	5, build_extras, false, false },
{ "electronics store",   'v',	c_yellow,	5, build_extras, false, false },
{ "electronics store",   '<',	c_yellow,	5, build_extras, false, false },
{ "sporting goods store",'^',	c_ltcyan,	5, build_extras, false, false },
{ "sporting goods store",'>',	c_ltcyan,	5, build_extras, false, false },
{ "sporting goods store",'v',	c_ltcyan,	5, build_extras, false, false },
{ "sporting goods store",'<',	c_ltcyan,	5, build_extras, false, false },
{ "liquor store",	'^',	c_magenta,	5, build_extras, false, false },
{ "liquor store",	'>',	c_magenta,	5, build_extras, false, false },
{ "liquor store",	'v',	c_magenta,	5, build_extras, false, false },
{ "liquor store",	'<',	c_magenta,	5, build_extras, false, false },
{ "gun store",		'^',	c_red,		5, build_extras, false, false },
{ "gun store",		'>',	c_red,		5, build_extras, false, false },
{ "gun store",		'v',	c_red,		5, build_extras, false, false },
{ "gun store",		'<',	c_red,		5, build_extras, false, false },
{ "clothing store",	'^',	c_blue,		5, build_extras, false, false },
{ "clothing store",	'>',	c_blue,		5, build_extras, false, false },
{ "clothing store",	'v',	c_blue,		5, build_extras, false, false },
{ "clothing store",	'<',	c_blue,		5, build_extras, false, false },
{ "library",		'^',	c_brown,	5, build_extras, false, false },
{ "library",		'>',	c_brown,	5, build_extras, false, false },
{ "library",		'v',	c_brown,	5, build_extras, false, false },
{ "library",		'<',	c_brown,	5, build_extras, false, false },
{ "restaurant",		'^',	c_pink,		5, build_extras, false, false },
{ "restaurant",		'>',	c_pink,		5, build_extras, false, false },
{ "restaurant",		'v',	c_pink,		5, build_extras, false, false },
{ "restaurant",		'<',	c_pink,		5, build_extras, false, false },
{ "subway station",	'S',	c_yellow,	5, build_extras, true, false },
{ "subway station",	'S',	c_yellow,	5, build_extras, true, false },
{ "subway station",	'S',	c_yellow,	5, build_extras, true, false },
{ "subway station",	'S',	c_yellow,	5, build_extras, true, false },
{ "police station",	'^',	c_dkgray,	5, build_extras, false, false },
{ "police station",	'>',	c_dkgray,	5, build_extras, false, false },
{ "police station",	'v',	c_dkgray,	5, build_extras, false, false },
{ "police station",	'<',	c_dkgray,	5, build_extras, false, false },
{ "bank",		'^',	c_ltgray,	5, no_extras, false, false },
{ "bank",		'>',	c_ltgray,	5, no_extras, false, false },
{ "bank",		'v',	c_ltgray,	5, no_extras, false, false },
{ "bank",		'<',	c_ltgray,	5, no_extras, false, false },
{ "bar",			'^',	c_pink,		5, build_extras, false, false },
{ "bar",			'>',	c_pink,		5, build_extras, false, false },
{ "bar",			'v',	c_pink,		5, build_extras, false, false },
{ "bar",			'<',	c_pink,		5, build_extras, false, false },
{ "pawn shop",		'^',	c_white,	5, build_extras, false, false },
{ "pawn shop",		'>',	c_white,	5, build_extras, false, false },
{ "pawn shop",		'v',	c_white,	5, build_extras, false, false },
{ "pawn shop",		'<',	c_white,	5, build_extras, false, false },
{ "mil. surplus",	'^',	c_white,	5, build_extras, false, false },
{ "mil. surplus",	'>',	c_white,	5, build_extras, false, false },
{ "mil. surplus",	'v',	c_white,	5, build_extras, false, false },
{ "mil. surplus",	'<',	c_white,	5, build_extras, false, false },
{ "megastore",		'M',	c_ltblue,	5, build_extras, false, false },
{ "megastore",		'M',	c_blue,		5, build_extras, false, false },
{ "hospital",		'H',	c_ltred,	5, build_extras, false, false },
{ "hospital",		'H',	c_red,		5, build_extras, false, false },
{ "mansion",		'M',	c_ltgreen,	5, build_extras, false, false },
{ "mansion",		'M',	c_green,	5, build_extras, false, false },
{ "evac shelter",	'+',	c_white,	2, no_extras, true, false },
{ "evac shelter",	'+',	c_white,	2, no_extras, false, true },
{ "science lab",		'L',	c_ltblue,	5, no_extras, false, false },
{ "science lab",		'L',	c_blue,		5, no_extras, true, false },
{ "science lab",		'L',	c_ltblue,	5, no_extras, false, false },
{ "science lab",		'L',	c_cyan,		5, no_extras, false, false },
{ "nuclear plant",	'P',	c_ltgreen,	5, no_extras, false, false },
{ "nuclear plant",	'P',	c_ltgreen,	5, no_extras, false, false },
{ "military bunker",	'B',	c_dkgray,	2, no_extras, true, true },
{ "military outpost",	'M',	c_dkgray,	2, build_extras, false, false },
{ "missile silo",	'0',	c_ltgray,	2, no_extras, false, false },
{ "missile silo",	'0',	c_ltgray,	2, no_extras, false, false },
{ "strange temple",	'T',	c_magenta,	5, no_extras, true, false },
{ "strange temple",	'T',	c_pink,		5, no_extras, true, false },
{ "strange temple",	'T',	c_pink,		5, no_extras, false, false },
{ "strange temple",	'T',	c_yellow,	5, no_extras, false, false },
{ "sewage treatment",	'P',	c_red,		5, no_extras, true, false },
{ "sewage treatment",	'P',	c_green,	5, no_extras, false, true },
{ "sewage treatment",	'P',	c_green,	5, no_extras, false, false },
{ "mine entrance",	'M',	c_ltgray,	5, no_extras, true, false },
{ "mine shaft",		'O',	c_dkgray,	5, no_extras, true, true },
{ "mine",		'M',	c_brown,	2, no_extras, false, false },
{ "mine",		'M',	c_brown,	2, no_extras, false, false },
{ "mine",		'M',	c_brown,	2, no_extras, false, false },
{ "spiral cavern",	'@',	c_pink,		2, no_extras, false, false },
{ "spiral cavern",	'@',	c_pink,		2, no_extras, false, false },
{ "radio tower",         'X',    c_ltgray,       2, no_extras, false, false },
{ "toxic waste dump",	'D',	c_pink,		2, no_extras, false, false },
{ "cave",		'C',	c_brown,	2, field_extras, false, false },
{ "rat cave",		'C',	c_dkgray,	2, no_extras, true, false },
{ "cavern",		'0',	c_ltgray,	2, no_extras, false, false },
{ "anthill",		'%',	c_brown,	2, no_extras, true, false },
{ "solid rock",		'%',	c_dkgray,	5, no_extras, false, false },
{ "rift",		'^',	c_red,		2, no_extras, false, false },
{ "hellmouth",		'^',	c_ltred,	2, no_extras, true, false },
{ "slime pit",		'~',	c_ltgreen,	2, no_extras, true, false },
{ "slime pit",		'~',	c_ltgreen,	2, no_extras, false, false },
{ "triffid grove",	'T',	c_ltred,	5, no_extras, true, false },
{ "triffid roots",	'T',	c_ltred,	5, no_extras, true, true },
{ "triffid heart",	'T',	c_red,		5, no_extras, false, true },
{ "basement",		'O',	c_dkgray,	5, no_extras, false, true },
{ "subway station",	'S',	c_yellow,	5, subway_extras, false, true },
{ "subway",        LINE_XOXO,	c_dkgray,	5, subway_extras, false, false },
{ "subway",        LINE_OXOX,	c_dkgray,	5, subway_extras, false, false },
{ "subway",        LINE_XXOO,	c_dkgray,	5, subway_extras, false, false },
{ "subway",        LINE_OXXO,	c_dkgray,	5, subway_extras, false, false },
{ "subway",        LINE_OOXX,	c_dkgray,	5, subway_extras, false, false },
{ "subway",        LINE_XOOX,	c_dkgray,	5, subway_extras, false, false },
{ "subway",        LINE_XXXO,	c_dkgray,	5, subway_extras, false, false },
{ "subway",        LINE_XXOX,	c_dkgray,	5, subway_extras, false, false },
{ "subway",        LINE_XOXX,	c_dkgray,	5, subway_extras, false, false },
{ "subway",        LINE_OXXX,	c_dkgray,	5, subway_extras, false, false },
{ "subway",        LINE_XXXX,	c_dkgray,	5, subway_extras, false, false },
{ "sewer",         LINE_XOXO,	c_green,	5, no_extras, false, false },
{ "sewer",         LINE_OXOX,	c_green,	5, no_extras, false, false },
{ "sewer",         LINE_XXOO,	c_green,	5, no_extras, false, false },
{ "sewer",         LINE_OXXO,	c_green,	5, no_extras, false, false },
{ "sewer",         LINE_OOXX,	c_green,	5, no_extras, false, false },
{ "sewer",         LINE_XOOX,	c_green,	5, no_extras, false, false },
{ "sewer",         LINE_XXXO,	c_green,	5, no_extras, false, false },
{ "sewer",         LINE_XXOX,	c_green,	5, no_extras, false, false },
{ "sewer",         LINE_XOXX,	c_green,	5, no_extras, false, false },
{ "sewer",         LINE_OXXX,	c_green,	5, no_extras, false, false },
{ "sewer",         LINE_XXXX,	c_green,	5, no_extras, false, false },
{ "ant tunnel",    LINE_XOXO,	c_brown,	5, no_extras, false, false },
{ "ant tunnel",    LINE_OXOX,	c_brown,	5, no_extras, false, false },
{ "ant tunnel",    LINE_XXOO,	c_brown,	5, no_extras, false, false },
{ "ant tunnel",    LINE_OXXO,	c_brown,	5, no_extras, false, false },
{ "ant tunnel",    LINE_OOXX,	c_brown,	5, no_extras, false, false },
{ "ant tunnel",    LINE_XOOX,	c_brown,	5, no_extras, false, false },
{ "ant tunnel",    LINE_XXXO,	c_brown,	5, no_extras, false, false },
{ "ant tunnel",    LINE_XXOX,	c_brown,	5, no_extras, false, false },
{ "ant tunnel",    LINE_XOXX,	c_brown,	5, no_extras, false, false },
{ "ant tunnel",    LINE_OXXX,	c_brown,	5, no_extras, false, false },
{ "ant tunnel",    LINE_XXXX,	c_brown,	5, no_extras, false, false },
{ "ant food storage",	'O',	c_green,	5, no_extras, false, false },
{ "ant larva chamber",	'O',	c_white,	5, no_extras, false, false },
{ "ant queen chamber",	'O',	c_red,		5, no_extras, false, false },
{ "cavern",		'0',	c_ltgray,	5, no_extras, false, false },
{ "tutorial room",	'O',	c_cyan,		5, no_extras, false, false }
};

struct omspec_place
{
	// Able functions - true if p is valid
	static bool never(overmap *om, point p) { return false; }
	static bool always(overmap *om, point p) { return true; }
	static bool water(overmap *om, point p); // Only on rivers
	static bool land(overmap *om, point p); // Only on land (no rivers)
	static bool forest(overmap *om, point p); // Forest
	static bool wilderness(overmap *om, point p); // Forest or fields
	static bool by_highway(overmap *om, point p); // Next to existing highways
};

// Set min or max to -1 to ignore them

const overmap_special overmap_special::specials[NUM_OMSPECS] = {

// Terrain	 MIN MAX DISTANCE
{ ot_crater,	   0, 10,  0, -1, mcat_null, 0, 0, 0, 0,
&omspec_place::land, mfb(OMS_FLAG_BLOB) },

{ ot_hive, 	   0, 50, 10, -1, mcat_bee, 20, 60, 2, 4,
&omspec_place::forest, mfb(OMS_FLAG_3X3) },

{ ot_house_north,   0,100,  0, -1, mcat_null, 0, 0, 0, 0,
&omspec_place::by_highway, mfb(OMS_FLAG_ROTATE_ROAD) },

{ ot_s_gas_north,   0,100,  0, -1, mcat_null, 0, 0, 0, 0,
&omspec_place::by_highway, mfb(OMS_FLAG_ROTATE_ROAD) },

{ ot_house_north,   0, 50, 20, -1, mcat_null, 0, 0, 0, 0,  // Woods cabin
&omspec_place::forest, mfb(OMS_FLAG_ROTATE_RANDOM) | mfb(OMS_FLAG_ROTATE_ROAD) },

{ ot_temple_stairs, 0,  3, 20, -1, mcat_null, 0, 0, 0, 0,
&omspec_place::forest, 0 },

{ ot_lab_stairs,	   0, 30,  8, -1, mcat_null, 0, 0, 0, 0,
&omspec_place::land, mfb(OMS_FLAG_ROAD) },

// Terrain	 MIN MAX DISTANCE
{ ot_bunker,	   2, 10,  4, -1, mcat_null, 0, 0, 0, 0,
&omspec_place::land, mfb(OMS_FLAG_ROAD) },

{ ot_outpost,	   0, 10,  4, -1, mcat_null, 0, 0, 0, 0,
&omspec_place::wilderness, 0 },

{ ot_silo,	   0,  1, 30, -1, mcat_null, 0, 0, 0, 0,
&omspec_place::wilderness, mfb(OMS_FLAG_ROAD) },

{ ot_radio_tower,   1,  5,  0, 20, mcat_null, 0, 0, 0, 0,
&omspec_place::by_highway, 0 },

{ ot_mansion_entrance, 0, 8, 0, -1, mcat_null, 0, 0, 0, 0,
&omspec_place::by_highway, mfb(OMS_FLAG_3X3_SECOND) },

{ ot_mansion_entrance, 0, 4, 10, -1, mcat_null, 0, 0, 0, 0,
&omspec_place::wilderness, mfb(OMS_FLAG_3X3_SECOND) },

{ ot_megastore_entrance, 0, 5, 0, 10, mcat_null, 0, 0, 0, 0,
&omspec_place::by_highway, mfb(OMS_FLAG_3X3_SECOND) },

{ ot_hospital_entrance, 1, 5, 3, 15, mcat_null, 0, 0, 0, 0,
&omspec_place::by_highway, mfb(OMS_FLAG_3X3_SECOND) },

{ ot_sewage_treatment, 1,  5, 10, 20, mcat_null, 0, 0, 0, 0,
&omspec_place::land, mfb(OMS_FLAG_PARKING_LOT) },

{ ot_mine_entrance,  0,  5,  15, -1, mcat_null, 0, 0, 0, 0,
&omspec_place::wilderness, mfb(OMS_FLAG_PARKING_LOT) },

// Terrain	 MIN MAX DISTANCE
{ ot_anthill,	   0, 30,  10, -1, mcat_ant, 10, 30, 1000, 2000,
&omspec_place::wilderness, 0 },

{ ot_spider_pit,	   0,500,  0, -1, mcat_null, 0, 0, 0, 0,
&omspec_place::forest, 0 },

{ ot_slimepit,	   0,  4,  0, -1, mcat_goo, 2, 10, 100, 200,
&omspec_place::land, 0 },

{ ot_fungal_bloom,  0,  3,  5, -1, mcat_fungi, 600, 1200, 30, 50,
&omspec_place::wilderness, 0 },

{ ot_triffid_grove, 0,  4,  0, -1, mcat_triffid, 800, 1300, 12, 20,
&omspec_place::forest, 0 },

{ ot_river_center,  0, 10, 10, -1, mcat_null, 0, 0, 0, 0,
&omspec_place::always, mfb(OMS_FLAG_BLOB) },

// Terrain	 MIN MAX DISTANCE
{ ot_shelter,       5, 10,  5, 10, mcat_null, 0, 0, 0, 0,
&omspec_place::wilderness, mfb(OMS_FLAG_ROAD) },

{ ot_cave,	   0, 30,  0, -1, mcat_null, 0, 0, 0, 0,
&omspec_place::wilderness, 0 },

{ ot_toxic_dump,	   0,  5, 15, -1, mcat_null, 0, 0, 0, 0,
&omspec_place::wilderness,0 }

};

#if PROTOTYPE
void settlement_building(settlement &set, int x, int y);	// #include "settlement.h" when bringing this up
#endif

double dist(int x1, int y1, int x2, int y2)
{
 return sqrt(double(pow(x1-x2, 2.0) + pow(y1-y2, 2.0)));
}

bool is_river(oter_id ter)
{
 if (ter == ot_null || (ter >= ot_bridge_ns && ter <= ot_river_nw))
  return true;
 return false;
}

bool is_ground(oter_id ter)
{
 if (ter <= ot_road_nesw_manhole)
  return true;
 return false;
}

bool is_wall_material(oter_id ter)
{
 if (is_ground(ter) ||
     (ter >= ot_house_north && ter <= ot_nuke_plant))
  return true;
 return false;
}

static oter_id _shop()
{
    static constexpr const oter_id typical_shop[] = {
      ot_s_lot,
      ot_s_gas_north,
      ot_s_pharm_north,
      ot_s_grocery_north,
      ot_s_hardware_north,
      ot_s_sports_north,
      ot_s_liquor_north,
      ot_s_gun_north,
      ot_s_clothes_north,
      ot_s_library_north,
      ot_s_restaurant_north,
      ot_sub_station_north,
      ot_bank_north,
      ot_bar_north,
      ot_s_electronics_north,
      ot_pawn_north,
      ot_mil_surplus_north
    };

    if (one_in(20)) return ot_police_north;
    return typical_shop[rng(0, sizeof(typical_shop)/sizeof(*typical_shop)-1)];
}

static oter_id shop(int dir)
{
#ifndef NDEBUG
 if (-4 > dir || 3 < dir) throw std::logic_error("invalid rotation specifier");
#endif
 oter_id ret = _shop();
 if (ot_s_lot == ret) return ret;
 if (dir < 0) dir += 4;
 switch (dir) {
  case 0:                         break;
  case 1: ret = oter_id(ret + 1); break;
  case 2: ret = oter_id(ret + 2); break;
  case 3: ret = oter_id(ret + 3); break;
  default: throw std::string("Bad rotation of shop.");
 }
 return ret;
}

static oter_id house(int dir)
{
#ifndef NDEBUG
 if (-4 > dir || 3 < dir) throw std::logic_error("invalid rotation specifier");
#endif
 bool base = one_in(30);
 if (dir < 0) dir += 4;
 switch (dir) {
  case 0:  return base ? ot_house_base_north : ot_house_north;
  case 1:  return base ? ot_house_base_east  : ot_house_east;
  case 2:  return base ? ot_house_base_south : ot_house_south;
  case 3:  return base ? ot_house_base_west  : ot_house_west;
  default: throw std::string("Bad rotation of house.");
 }
}

// *** BEGIN overmap FUNCTIONS ***

oter_id& overmap::ter(int x, int y)
{
 if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY) return (discard<oter_id>::x = ot_null);
 return t[x][y];
}

oter_id& overmap::ter(OM_loc OMpos)
{
    OMpos.self_normalize();
    return om_cache::get().create(OMpos.first).ter(OMpos.second);
}

oter_id overmap::ter_c(OM_loc OMpos)
{
    OMpos.self_normalize();
    const auto om = om_cache::get().r_get(OMpos.first);
    if (!om) return ot_null;
    return om->ter(OMpos.second);
}

std::vector<mongroup*> overmap::monsters_at(int x, int y)
{
 std::vector<mongroup*> ret;
 if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY) return ret;
 for (auto& _group : zg) if (trig_dist(x, y, _group.pos) <= _group.radius) ret.push_back(&_group);
 return ret;
}

std::vector<const mongroup*> overmap::monsters_at(int x, int y) const
{
    std::vector<const mongroup*> ret;
    if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY) return ret;
    for (auto& _group : zg) if (trig_dist(x, y, _group.pos) <= _group.radius) ret.push_back(&_group);
    return ret;
}

bool overmap::is_safe(const OM_loc& loc)
{
    if (0 <= loc.second.x && OMAPX > loc.second.x && 0 <= loc.second.y && OMAPY > loc.second.y) {
        auto om = om_cache::get().r_get(loc.first);
        if (!om) return false;  // not yet generated: scary
        for (auto& gr : om->monsters_at(loc.second.x, loc.second.y)) if (!gr->is_safe()) return false;
        return true;
    }
    return is_safe(normalize(loc));
}

bool& overmap::seen(int x, int y)
{
 if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY) return (discard<bool>::x = false);
 return s[x][y];
}

bool& overmap::seen(OM_loc OMpos)
{
    OMpos.self_normalize();
    return om_cache::get().create(OMpos.first).seen(OMpos.second);
}

bool overmap::seen_c(OM_loc OMpos)
{
    OMpos.self_normalize();
    const auto om = om_cache::get().r_get(OMpos.first);
    if (!om) return false;
    return om->seen(OMpos.second);
}

void overmap::expose(OM_loc OMpos)
{
    OMpos.self_normalize();
    const auto om = om_cache::get().r_get(OMpos.first);
    // just read access first.  If we have to update, then redo the overmap retrieval to signal write access.
    if (!om || !om->seen(OMpos.second)) om_cache::get().create(OMpos.first).seen(OMpos.second) = true;
}


bool overmap::has_note(const point& pt) const
{
 for (const auto& note : notes) {
     if (note.x == pt.x && note.y == pt.y) return true;
 }
 return false;
}

bool overmap::has_note(OM_loc OMpos, std::string& dest)
{
    OMpos.self_normalize();
    const auto om = om_cache::get().r_get(OMpos.first);
    if (!om) return false;
    for (const auto& note : om->notes) {
        if (note.x == OMpos.second.x && note.y == OMpos.second.y) {
            dest = note.text;
            return true;
        }
    }
    return false;
}

void overmap::add_note(const point& pt, std::string message)
{
 const bool non_empty = 0 < message.length();
 for (int i = 0; i < notes.size(); i++) {
  if (notes[i].x == pt.x && notes[i].y == pt.y) {
   if (non_empty) notes[i].text = std::move(message);
   else EraseAt(notes, i);
   return;
  }
 }
 if (non_empty) notes.push_back(om_note(pt.x, pt.y, notes.size(), std::move(message)));
}

point overmap::find_note(point origin, const std::string& text) const
{
 int closest = 9999;
 point ret(-1, -1);
 for (int i = 0; i < notes.size(); i++) {
  int dist = rl_dist(origin, notes[i].x, notes[i].y);
  if (notes[i].text.find(text) != std::string::npos && dist < closest) {
   closest = dist;
   ret = point(notes[i].x, notes[i].y);
  }
 }
 return ret;
}

void overmap::delete_note(const point& pt)
{
 size_t i = notes.size();
 while (0 < i) {
     auto& note = notes[--i];
     if (note.x == pt.x && note.y == pt.y) EraseAt(notes, i);
 }
}

point overmap::display_notes() const
{
 std::string title = "Notes:";
 WINDOW* w_notes = newwin(VIEW, SCREEN_WIDTH, 0, 0);
 const int maxitems = 20;	// Number of items to show at one time.
 char ch = '.';
 int start = 0, cur_it;
 mvwprintz(w_notes, 0, 0, c_ltgray, title.c_str());
 do{
  if (ch == '<' && start > 0) {
   for (int i = 1; i < VIEW; i++)
    mvwprintz(w_notes, i, 0, c_black, "                                                     ");
   start -= maxitems;
   if (start < 0) start = 0;
   mvwprintw(w_notes, maxitems + 2, 0, "         ");
  }
  if (ch == '>' && cur_it < notes.size()) {
   start = cur_it;
   mvwprintw(w_notes, maxitems + 2, 12, "            ");
   for (int i = 1; i < VIEW; i++)
    mvwprintz(w_notes, i, 0, c_black, "                                                     ");
  }
  int cur_line = 2;
  int last_line = -1;
  char cur_let = 'a';
  for (cur_it = start; cur_it < start + maxitems && cur_line < 23; cur_it++) {
   if (cur_it < notes.size()) {
   mvwputch (w_notes, cur_line, 0, c_white, cur_let++);
   mvwprintz(w_notes, cur_line, 2, c_ltgray, "- %s", notes[cur_it].text.c_str());
   } else{
    last_line = cur_line - 2;
    break;
   }
   cur_line++;
  }

  if(last_line == -1) last_line = 23;
  if (start > 0)
   mvwprintw(w_notes, maxitems + 4, 0, "< Go Back");
  if (cur_it < notes.size())
   mvwprintw(w_notes, maxitems + 4, 12, "> More notes"); 
  if(ch >= 'a' && ch <= 't'){
   int chosen_line = (int)(ch % (int)'a');
   if(chosen_line < last_line)
    return point(notes[start + chosen_line].x, notes[start + chosen_line].y); 
  }
  mvwprintz(w_notes, 0, 40, c_white, "Press letter to center on note");
  mvwprintz(w_notes, 24, 40, c_white, "Spacebar - Return to map  ");
  wrefresh(w_notes);
  ch = getch();
 } while(ch != ' ');
 delwin(w_notes);
 return point(-1,-1);
}

void overmap::clear_terrain(oter_id src)
{
	for (int i = 0; i < OMAPY; i++) {
		for (int j = 0; j < OMAPX; j++) {
			t[i][j] = src;
		}
	}
}

void overmap::generate(game *g, const overmap* north, const overmap* east, const overmap* south, const overmap* west)
{
 erase();	// curses setup
 clear();
 move(0, 0);

 clear_seen(); // Start by setting all squares to unseen
 clear_terrain(ot_field);	// Start by setting everything to open field

 std::vector<city> road_points;	// cities and roads_out together
 std::vector<point> river_start;// West/North endpoints of rivers
 std::vector<point> river_end;	// East/South endpoints of rivers

// Determine points where rivers & roads should connect w/ adjacent maps
 if (north) {
  for (int i = 2; i < OMAPX - 2; i++) {
   const auto north_terrain = north->ter(i, OMAPY - 1);
   if (is_river(north_terrain))
    ter(i, 0) = ot_river_center;
   if (north_terrain == ot_river_center &&
       north->ter(i - 1, OMAPY - 1) == ot_river_center &&
       north->ter(i + 1, OMAPY - 1) == ot_river_center) {
    if (river_start.size() == 0 ||
        river_start[river_start.size() - 1].x < i - 6)
     river_start.push_back(point(i, 0));
   }
   if (north_terrain == ot_road_nesw) roads_out.push_back(city(i, 0, 0));    // 2019-12-22 interpolated
  }
  for (const auto& n_road_out : north->roads_out) if (n_road_out.y == OMAPY - 1) roads_out.push_back(city(n_road_out.x, 0, 0));
 }
 int rivers_from_north = river_start.size();
 if (west) {
  for (int i = 2; i < OMAPY - 2; i++) {
   const auto west_terrain = west->ter(OMAPX - 1, i);
   if (is_river(west_terrain)) ter(0, i) = ot_river_center;
   if (west_terrain == ot_river_center &&
       west->ter(OMAPX - 1, i - 1) == ot_river_center &&
       west->ter(OMAPX - 1, i + 1) == ot_river_center) {
    if (river_start.size() == rivers_from_north ||
        river_start[river_start.size() - 1].y < i - 6)
     river_start.push_back(point(0, i));
   }
   if (west_terrain == ot_road_nesw) roads_out.push_back(city(0, i, 0));    // 2019-12-22 interpolated
  }
  for (const auto& w_road_out : west->roads_out) if (w_road_out.x == OMAPX - 1) roads_out.push_back(city(0, w_road_out.y, 0));
 }
 if (south) {
  for (int i = 2; i < OMAPX - 2; i++) {
   const auto south_terrain = south->ter(i, 0);
   if (is_river(south_terrain)) ter(i, OMAPY - 1) = ot_river_center;
   if (south_terrain == ot_river_center &&
       south->ter(i - 1, 0) == ot_river_center &&
       south->ter(i + 1, 0) == ot_river_center) {
    if (river_end.size() == 0 ||
        river_end[river_end.size() - 1].x < i - 6)
     river_end.push_back(point(i, OMAPY - 1));
   }
   if (south_terrain == ot_road_nesw) roads_out.push_back(city(i, OMAPY - 1, 0));
  }
  for (const auto& s_road_out : south->roads_out) if (s_road_out.y == 0) roads_out.push_back(city(s_road_out.x, OMAPY - 1, 0));
 }
 int rivers_to_south = river_end.size();
 if (east) {
  for (int i = 2; i < OMAPY - 2; i++) {
   const auto east_terrain = east->ter(0, i);
   if (is_river(east_terrain)) ter(OMAPX - 1, i) = ot_river_center;
   if (east_terrain == ot_river_center &&
       east->ter(0, i - 1) == ot_river_center &&
       east->ter(0, i + 1) == ot_river_center) {
    if (river_end.size() == rivers_to_south ||
        river_end[river_end.size() - 1].y < i - 6)
     river_end.push_back(point(OMAPX - 1, i));
   }
   if (east_terrain == ot_road_nesw) roads_out.push_back(city(OMAPX - 1, i, 0));
  }
  for(const auto& e_road_out : east->roads_out) if (e_road_out.x == 0) roads_out.push_back(city(OMAPX - 1, e_road_out.y, 0));
 }
// Even up the start and end points of rivers. (difference of 1 is acceptable)
// Also ensure there's at least one of each.
 std::vector<point> new_rivers;
 if (!north || !west) {
  while (river_start.empty() || river_start.size() + 1 < river_end.size()) {
   new_rivers.clear();
   if (!north) new_rivers.push_back( point(rng(10, OMAPX - 11), 0) );
   if (!west) new_rivers.push_back( point(0, rng(10, OMAPY - 11)) );
   river_start.push_back( new_rivers[rng(0, new_rivers.size() - 1)] );
  }
 }
 if (!south || !east) {
  while (river_end.empty() || river_end.size() + 1 < river_start.size()) {
   new_rivers.clear();
   if (!south) new_rivers.push_back( point(rng(10, OMAPX - 11), OMAPY - 1) );
   if (!east) new_rivers.push_back( point(OMAPX - 1, rng(10, OMAPY - 11)) );
   river_end.push_back( new_rivers[rng(0, new_rivers.size() - 1)] );
  }
 }
// Now actually place those rivers.
 if (river_start.size() > river_end.size() && river_end.size() > 0) {
  std::vector<point> river_end_copy = river_end;
  int index;
  while (!river_start.empty()) {
   index = rng(0, river_start.size() - 1);
   if (!river_end.empty()) {
    place_river(river_start[index], river_end[0]);
    river_end.erase(river_end.begin());
   } else
    place_river(river_start[index],
                river_end_copy[rng(0, river_end_copy.size() - 1)]);
   river_start.erase(river_start.begin() + index);
  }
 } else if (river_end.size() > river_start.size() && river_start.size() > 0) {
  std::vector<point> river_start_copy = river_start;
  int index;
  while (!river_end.empty()) {
   index = rng(0, river_end.size() - 1);
   if (!river_start.empty()) {
    place_river(river_start[0], river_end[index]);
    river_start.erase(river_start.begin());
   } else
    place_river(river_start_copy[rng(0, river_start_copy.size() - 1)],
                river_end[index]);
   river_end.erase(river_end.begin() + index);
  }
 } else if (river_end.size() > 0) {
  if (river_start.size() != river_end.size())
   river_start.push_back( point(rng(OMAPX * .25, OMAPX * .75),
                                rng(OMAPY * .25, OMAPY * .75)));
  for (int i = 0; i < river_start.size(); i++)
   place_river(river_start[i], river_end[i]);
 }
    
// Cities, forests, and settlements come next.
// These're agnostic of adjacent maps, so it's very simple.
 int mincit = 0;
 if (north == NULL && east == NULL && west == NULL && south == NULL)
  mincit = 1;	// The first map MUST have a city, for the player to start in!
 place_cities(cities, mincit);
 place_forest();

// Ideally we should have at least two exit points for roads, on different sides
 if (roads_out.size() < 2) { 
  std::vector<city> viable_roads;
  int tmp;
// Populate viable_roads with one point for each neighborless side.
// Make sure these points don't conflict with rivers. 
// TODO: In theory this is a potential infinte loop...
  if (north == NULL) {
   do
    tmp = rng(10, OMAPX - 11);
   while (is_river(ter(tmp, 0)) || is_river(ter(tmp - 1, 0)) ||
          is_river(ter(tmp + 1, 0)) );
   viable_roads.push_back(city(tmp, 0, 0));
  }
  if (east == NULL) {
   do
    tmp = rng(10, OMAPY - 11);
   while (is_river(ter(OMAPX - 1, tmp)) || is_river(ter(OMAPX - 1, tmp - 1))||
          is_river(ter(OMAPX - 1, tmp + 1)));
   viable_roads.push_back(city(OMAPX - 1, tmp, 0));
  }
  if (south == NULL) {
   do
    tmp = rng(10, OMAPX - 11);
   while (is_river(ter(tmp, OMAPY - 1)) || is_river(ter(tmp - 1, OMAPY - 1))||
          is_river(ter(tmp + 1, OMAPY - 1)));
   viable_roads.push_back(city(tmp, OMAPY - 1, 0));
  }
  if (west == NULL) {
   do
    tmp = rng(10, OMAPY - 11);
   while (is_river(ter(0, tmp)) || is_river(ter(0, tmp - 1)) ||
          is_river(ter(0, tmp + 1)));
   viable_roads.push_back(city(0, tmp, 0));
  }
  while (roads_out.size() < 2 && !viable_roads.empty()) {
   tmp = rng(0, viable_roads.size() - 1);
   roads_out.push_back(viable_roads[tmp]);
   viable_roads.erase(viable_roads.begin() + tmp);
  }
 }
// Compile our master list of roads; it's less messy if roads_out is first
 for (int i = 0; i < roads_out.size(); i++)
  road_points.push_back(roads_out[i]);
 for (int i = 0; i < cities.size(); i++)
  road_points.push_back(cities[i]);
// And finally connect them via "highways"
 place_hiways(road_points, ot_road_null);
// Place specials
 place_specials();
// Make the roads out road points;
 for (int i = 0; i < roads_out.size(); i++)
  ter(roads_out[i].x, roads_out[i].y) = ot_road_nesw;
// Clean up our roads and rivers
 polish();
// Place the monsters, now that the terrain is laid out
 place_mongroups();
 place_radios();

 save(g->u.name);
}

void overmap::generate_sub(const overmap* above)
{
 std::vector<city> subway_points;
 std::vector<city> sewer_points;
 std::vector<city> ant_points;
 std::vector<city> goo_points;
 std::vector<city> lab_points;
 std::vector<point> shaft_points;
 std::vector<city> mine_points;
 std::vector<point> bunker_points;
 std::vector<point> shelter_points;
 std::vector<point> triffid_points;
 std::vector<point> temple_points;

 clear_seen(); // Start by setting all squares to unseen
 clear_terrain(ot_rock);	// Start by setting everything to solid rock

 for (int i = 0; i < OMAPX; i++) {
  for (int j = 0; j < OMAPY; j++) {
   const auto above_terrain = above->ter(i, j);
   if (is_between<ot_sub_station_north, ot_sub_station_west>(above_terrain)) {
    ter(i, j) = ot_subway_nesw;
    subway_points.push_back(city(i, j, 0));

   } else if (above_terrain == ot_road_nesw_manhole) {
    ter(i, j) = ot_sewer_nesw;
    sewer_points.push_back(city(i, j, 0));

   } else if (above_terrain == ot_sewage_treatment) {
    for (int x = i-1; x <= i+1; x++) {
     for (int y = j-1; y <= j+1; y++) {
      ter(x, y) = ot_sewage_treatment_under;
     }
    }
    ter(i, j) = ot_sewage_treatment_hub;
    sewer_points.push_back(city(i, j, 0));

   } else if (above_terrain == ot_spider_pit)
    ter(i, j) = ot_spider_pit_under;

   else if (above_terrain == ot_cave && pos.z == -1) {
	ter(i, j) = one_in(3) ? ot_cave_rat : ot_cave;

   } else if (above_terrain == ot_cave_rat && pos.z == -2)
    ter(i, j) = ot_cave_rat;

   else if (above_terrain == ot_anthill) {
    int size = rng(MIN_ANT_SIZE, MAX_ANT_SIZE);
    ant_points.push_back(city(i, j, size));
    zg.push_back(mongroup(mcat_ant, i * 2, j * 2, size * 1.5, rng(6000, 8000)));

   } else if (above_terrain == ot_slimepit_down) {
    int size = rng(MIN_GOO_SIZE, MAX_GOO_SIZE);
    goo_points.push_back(city(i, j, size));

   } else if (above_terrain == ot_forest_water)
    ter(i, j) = ot_cavern;

   else if (above_terrain == ot_triffid_grove ||
            above_terrain == ot_triffid_roots)
    triffid_points.push_back( point(i, j) );

   else if (above_terrain == ot_temple_stairs)
    temple_points.push_back( point(i, j) );

   else if (above_terrain == ot_lab_core ||
            (pos.z == -1 && above_terrain == ot_lab_stairs))
    lab_points.push_back(city(i, j, rng(1, 5 + pos.z)));

   else if (above_terrain == ot_lab_stairs)
    ter(i, j) = ot_lab;

   else if (above_terrain == ot_bunker && pos.z == -1)
    bunker_points.push_back( point(i, j) );

   else if (above_terrain == ot_shelter)
    shelter_points.push_back( point(i, j) );

   else if (above_terrain == ot_mine_entrance)
    shaft_points.push_back( point(i, j) );

   else if (above_terrain == ot_mine_shaft ||
	        above_terrain == ot_mine_down    ) {
    ter(i, j) = ot_mine;
    mine_points.push_back(city(i, j, rng(6 + pos.z, 10 + pos.z)));

   } else if (above_terrain == ot_mine_finale) {
    for (int x = i - 1; x <= i + 1; x++) {
     for (int y = j - 1; y <= j + 1; y++)
      ter(x, y) = ot_spiral;
    }
    ter(i, j) = ot_spiral_hub;
    zg.push_back(mongroup(mcat_spiral, i * 2, j * 2, 2, 200));

   } else if (above_terrain == ot_silo) {
	const auto abs_z = (0<pos.z) ? pos.z : -pos.z;
	ter(i, j) = (rng(2, 7) < abs_z || rng(2, 7) < abs_z) ? ot_silo_finale : ot_silo;
   }
  }
 }

 for (const auto& pt : goo_points) build_slimepit(pt);
 place_hiways(sewer_points,  ot_sewer_nesw);
 polish(ot_sewer_ns, ot_sewer_nesw);
 place_hiways(subway_points, ot_subway_nesw);
 for (const auto& pt : subway_points) ter(pt.x, pt.y) = ot_subway_station;
 for (const auto& pt : lab_points) build_lab(pt);
 for (const auto& pt : ant_points) build_anthill(pt);
 polish(ot_subway_ns, ot_subway_nesw);
 polish(ot_ants_ns, ot_ants_nesw);
 for (const auto& above_c : above->cities) {
  if (one_in(3)) zg.push_back(mongroup(mcat_chud, above_c.x * 2, above_c.y * 2, above_c.s, above_c.s * 20));
  if (!one_in(8)) zg.push_back(mongroup(mcat_sewer, above_c.x * 2, above_c.y * 2, above_c.s * 3.5, above_c.s * 70));
 }
 place_rifts();
 for(const auto& pt : mine_points) build_mine(pt);
// Basements done last so sewers, etc. don't overwrite them
 for (int i = 0; i < OMAPX; i++) {
  for (int j = 0; j < OMAPY; j++) {
   if (is_between<ot_house_base_north, ot_house_base_west>(above->ter(i, j)))
    ter(i, j) = ot_basement;
  }
 }

 for (const auto& pt : shaft_points) ter(pt) = ot_mine_shaft;
 for (const auto& pt : bunker_points) ter(pt) = ot_bunker;
 for (const auto& pt : shelter_points) ter(pt) = ot_shelter_under;
 for (const auto& pt : triffid_points) ter(pt) = (pos.z == -1) ? ot_triffid_roots : ot_triffid_finale;
 for (const auto& pt : temple_points) ter(pt) = (pos.z == -5) ? ot_temple_finale : ot_temple_stairs;

 save(game::active()->u.name);
}

void overmap::make_tutorial()
{
 if (pos.z == TUTORIAL_Z-1) {
  for (int i = 0; i < OMAPX; i++) {
   for (int j = 0; j < OMAPY; j++)
    ter(i, j) = ot_rock;
  }
 }
 ter(50, 50) = ot_tutorial;
 zg.clear();
}

bool overmap::find_closest(point origin, oter_id type, int type_range, OM_loc& dest, int dist, bool must_be_seen) const
{
    const int max = (dist == 0 ? OMAPX / 2 : dist);
    auto t_at = ter(origin);
    if (t_at >= type && t_at < type + type_range && (!must_be_seen || seen(origin))) {
        dest = OM_loc(pos, origin);
        return true;
    }

    for (dist = 1; dist <= max; dist++) {
        int i = 8 * dist;
        while (0 < i--) {
            point pt = origin + zaimoni::gdi::Linf_border_sweep<point>(dist, i, origin.x, origin.y);
            if ((t_at = ter(pt)) >= type && t_at < type + type_range && (!must_be_seen || seen(pt))) {  // this null-terrains if cross-overmap is attempted
                dest = OM_loc(pos, pt);
                return true;
            }
        }
    }
    return false;
}

#if DEAD_FUNC
// reimplement as above before taking live
std::vector<point> overmap::find_all(point origin, oter_id type, int type_range,
                            int &dist, bool must_be_seen)
{
 std::vector<point> res;
 int max = (dist == 0 ? OMAPX / 2 : dist);
 for (dist = 0; dist <= max; dist++) {
  for (int x = origin.x - dist; x <= origin.x + dist; x++) {
   for (int y = origin.y - dist; y <= origin.y + dist; y++) {
    if (ter(x, y) >= type && ter(x, y) < type + type_range &&
        (!must_be_seen || seen(x, y)))
     res.push_back(point(x, y));
   }
  }
 }
 return res;
}
#endif

std::vector<point> overmap::find_terrain(const std::string& term) const
{
 std::vector<point> found;
 for (int x = 0; x < OMAPX; x++) {
  for (int y = 0; y < OMAPY; y++) {
   if (seen(x, y) && oter_t::list[ter(x, y)].name.find(term) != std::string::npos)
    found.push_back( point(x, y) );
  }
 }
 return found;
}

const city* overmap::closest_city(point p) const
{
 const city* ret = 0;
 int distance = 999;
 for (const auto& c : cities) {
     int dist = rl_dist(p, c.x, c.y);
     if (dist < distance || (dist == distance && c.s < ret->s)) {
         ret = &c;
         distance = dist;
     }
 }
 return ret;
}

point overmap::random_house_in_city(const city* c) const
{
 if (!c) return point(-1, -1);

 std::vector<point> valid;
 int startx = c->x - c->s,
     endx   = c->x + c->s,
     starty = c->y - c->s,
     endy   = c->y + c->s;
 for (int x = startx; x <= endx; x++) {
  for (int y = starty; y <= endy; y++) {
   if (is_between<ot_house_north, ot_house_west>(ter(x, y))) valid.push_back( point(x, y) );
  }
 }
 if (valid.empty()) return point(-1, -1);

 return valid[ rng(0, valid.size() - 1) ];
}

int overmap::dist_from_city(point p) const
{
 int distance = 999;
 for (int i = 0; i < cities.size(); i++) {
  int dist = rl_dist(p, cities[i].x, cities[i].y);
  dist -= cities[i].s;
  if (dist < distance) distance = dist;
 }
 return distance;
}

void overmap::draw(WINDOW *w, game *g, point& curs, point& orig, char &ch, bool blink) const
{
 constexpr const bool legend = true;
 constexpr const int om_w = 51;	// overmap width

 bool note_here = false, npc_here = false;
 std::string note_text, npc_name;
 
 OM_loc target(_ref<OM_loc>::invalid);
 if (g->u.active_mission >= 0 &&
     g->u.active_mission < g->u.active_missions.size()) {
    target = mission::from_id(g->u.active_missions[g->u.active_mission])->target;
    if (target.is_valid()) target.self_denormalize(pos);
 }
/* First, determine if we're close enough to the edge to need to load an
 * adjacent overmap, and load it/them. */
 const int y_delta = ((curs.y < VIEW / 2) ? -1 : ((curs.y >= OMAPY - VIEW / 2 - 1) ? 1 : 0));
 const int x_delta = ((curs.x < om_w / 2) ? -1 : ((curs.x >= OMAPX - om_w / 2) ? 1 : 0));

// Now actually draw the map
  bool csee = false;
  oter_id ccur_ter;
  for (int i = -om_w / 2; i < om_w / 2; i++) {
   for (int j = -VIEW / 2; j <= (ch == 'j' ? VIEW / 2 + 1 : VIEW / 2); j++) {
    OM_loc scan(pos, point(curs.x+i, curs.y+j));
    oter_id cur_ter = overmap::ter_c(scan);
	nc_color ter_color;
	long ter_sym;
	int omx = scan.second.x;
	int omy = scan.second.y;
    bool see = overmap::seen_c(scan);
	npc_name = "";
    npc_here = false;
    note_here = overmap::has_note(scan, note_text);
    if (omx >= 0 && omx < OMAPX && omy >= 0 && omy < OMAPY) { // It's in-bounds
	 for (const auto& _npc : npcs) {
	  const auto om = toOvermap(_npc.GPSpos);
      if (om.second == scan.second) {
       npc_here = true;
       npc_name = _npc.name;
	   break;
      }
	 }
    }   // don't try to recover npc if out of bounds \todo fix this
    if (see) {
     if (note_here && blink) {
      ter_color = c_yellow;
      ter_sym = 'N';
     } else if (orig == scan.second && blink) {
      ter_color = g->u.color();
      ter_sym = '@';
     } else if (npc_here && blink) {
      ter_color = c_pink;
      ter_sym = '@';
     } else if (target.is_valid() && target.second == scan.second && blink) {
      ter_color = c_red;
      ter_sym = '*';
     } else {
      if (cur_ter >= num_ter_types || cur_ter < 0) {
       debugmsg("Bad ter %d (%d, %d)", cur_ter, omx, omy);
	   cur_ter = ot_null;	// certainly don't use an invalid array dereference
	  }
	  decltype(oter_t::list[0])& terrain = oter_t::list[cur_ter];
      ter_color = terrain.color;
      ter_sym = terrain.sym;
     }
    } else { // We haven't explored this tile yet
     ter_color = c_dkgray;
     ter_sym = '#';
    }
    if (j == 0 && i == 0) {
     mvwputch_hi (w, VIEW / 2, om_w / 2,     ter_color, ter_sym);
     csee = see;
     ccur_ter = cur_ter;
    } else
     mvwputch    (w, VIEW / 2 + j, om_w / 2 + i, ter_color, ter_sym);
   }
  }
  if (target.is_valid() && blink && zaimoni::gdi::box(curs - point(om_w, VIEW)/2, point(om_w, VIEW)).contains(target.second)) {
   switch (direction_from(curs.x, curs.y, target.second)) {
    case NORTH:      mvwputch(w,  0, om_w / 2, c_red, '^');       break;
    case NORTHEAST:  mvwputch(w,  0, om_w - 2, c_red, LINE_OOXX); break;
    case EAST:       mvwputch(w, VIEW / 2, om_w - 2, c_red, '>');       break;
    case SOUTHEAST:  mvwputch(w, VIEW - 1, om_w - 2, c_red, LINE_XOOX); break;
    case SOUTH:      mvwputch(w, VIEW - 1, om_w / 2, c_red, 'v');       break;
    case SOUTHWEST:  mvwputch(w, VIEW - 1,  0, c_red, LINE_XXOO); break;
    case WEST:       mvwputch(w, VIEW / 2,  0, c_red, '<');       break;
    case NORTHWEST:  mvwputch(w,  0,  0, c_red, LINE_OXXO); break;
   }
  }
  if (has_note(OM_loc(pos, curs), note_text)) {
   for (int i = 0; i < note_text.length(); i++)
    mvwputch(w, 1, i, c_white, LINE_OXOX);
   mvwputch(w, 1, note_text.length(), c_white, LINE_XOOX);
   mvwputch(w, 0, note_text.length(), c_white, LINE_XOXO);
   mvwprintz(w, 0, 0, c_yellow, note_text.c_str());
  } else if (npc_here) {
   for (int i = 0; i < npc_name.length(); i++)
    mvwputch(w, 1, i, c_white, LINE_OXOX);
   mvwputch(w, 1, npc_name.length(), c_white, LINE_XOOX);
   mvwputch(w, 0, npc_name.length(), c_white, LINE_XOXO);
   mvwprintz(w, 0, 0, c_yellow, npc_name.c_str());
  }
  if (legend) {
// Draw the vertical line
   for (int j = 0; j < VIEW; j++)
    mvwputch(w, j, om_w, c_white, LINE_XOXO);
// Clear the legend
   for (int i = om_w; i < SCREEN_WIDTH; i++) {
    for (int j = 0; j < VIEW; j++)
     mvwputch(w, j, i, c_black, 'x');
   }

   if (csee) {
	decltype(oter_t::list[ccur_ter])& terrain = oter_t::list[ccur_ter];
    mvwputch(w, 1, om_w, terrain.color, terrain.sym);
    mvwprintz(w, 1, om_w + 2, terrain.color, "%s", terrain.name.c_str());
   } else
    mvwprintz(w, 1, om_w, c_dkgray, "# Unexplored");

   if (target.is_valid()) {
    int distance = rl_dist(orig, target.second);
    mvwprintz(w, 3, om_w, c_white, "Distance to target: %d", distance);
   }
   mvwprintz(w, VIEW - 8, om_w, c_magenta,           "Use movement keys to pan.  ");
   mvwprintz(w, VIEW - 7, om_w, c_magenta,           "0 - Center map on character");
   mvwprintz(w, VIEW - 6, om_w, c_magenta,           "t - Toggle legend          ");
   mvwprintz(w, VIEW - 5, om_w, c_magenta,           "/ - Search                 ");
   mvwprintz(w, VIEW - 4, om_w, c_magenta,           "N - Add a note             ");
   mvwprintz(w, VIEW - 3, om_w, c_magenta,           "D - Delete a note          ");
   mvwprintz(w, VIEW - 2, om_w, c_magenta,           "L - List notes             ");
   mvwprintz(w, VIEW - 1, om_w, c_magenta,           "Esc or q - Return to game  ");
  }
// Done with all drawing!
  wrefresh(w);
}

point overmap::choose_point(game *g)    // not const due to overmap::add_note
{
 WINDOW* w_map = newwin(VIEW, SCREEN_WIDTH, 0, 0);
 WINDOW* w_search = newwin(13, 27, 3, 51);
 timeout(BLINK_SPEED);	// Enable blinking!
 bool blink = true;
 point curs((g->lev.x + int(MAPSIZE / 2)), (g->lev.y + int(MAPSIZE / 2)));
 curs /= 2;
 point orig(curs);
 char ch = 0;
 point ret(-1, -1);
 
 do {  
  draw(w_map, g, curs, orig, ch, blink);
  ch = input();
  if (ch != ERR) blink = true;	// If any input is detected, make the blinkies on
  point dir(get_direction(ch));
  if (dir.x != -2) curs += dir;
  else if (ch == '0') curs = orig;
  else if (ch == '\n') ret = curs;
  else if (ch == KEY_ESCAPE || ch == 'q' || ch == 'Q') ret = point(-1, -1);
  else if (ch == 'N') {
   timeout(-1);
   add_note(curs, string_input_popup(49, "Enter note")); // 49 char max
   timeout(BLINK_SPEED);
  } else if(ch == 'D'){
   timeout(-1);
   if (has_note(curs) && query_yn("Really delete note?")) delete_note(curs);
   timeout(BLINK_SPEED);
  } else if (ch == 'L'){
   timeout(-1);
   point p(display_notes());
   if (p.x != -1) curs = p;
   timeout(BLINK_SPEED);
   wrefresh(w_map);
  } else if (ch == '/') {
   point tmp(curs);
   timeout(-1);
   std::string term = string_input_popup("Search term:");
   timeout(BLINK_SPEED);
   draw(w_map, g, curs, orig, ch, blink);
   point found = find_note(curs, term);
   if (found.x == -1) {	// Didn't find a note
    std::vector<point> terlist(find_terrain(term));
    if (!terlist.empty()){
     int i = 0;
     //Navigate through results
     do {
      //Draw search box
      wborder(w_search, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
              LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
      mvwprintz(w_search, 1, 1, c_red, "Find place:");
      mvwprintz(w_search, 2, 1, c_ltblue, "                         ");
      mvwprintz(w_search, 2, 1, c_ltblue, "%s", term.c_str());
      mvwprintz(w_search, 4, 1, c_white,
       "'<' '>' Cycle targets.");
      mvwprintz(w_search, 10, 1, c_white, "Enter/Spacebar to select.");
      mvwprintz(w_search, 11, 1, c_white, "q to return.");
      ch = input();
      if (ch == ERR) blink = !blink;
      else if (ch == '<') {
       if(++i > terlist.size() - 1) i = 0;
      } else if(ch == '>'){
       if(--i < 0) i = terlist.size() - 1;
      }
      curs = terlist[i];
      draw(w_map, g, curs, orig, ch, blink);
      wrefresh(w_search);
      timeout(BLINK_SPEED);
     } while(ch != '\n' && ch != ' ' && ch != 'q'); 
     //If q is hit, return to the last position
     if(ch == 'q') curs = tmp;
     ch = '.';
    }
   }
   if (found.x != -1) curs = found;
  }/* else if (ch == 't')  *** Legend always on for now! ***
   legend = !legend;
*/
  else if (ch == ERR)	// Hit timeout on input, so make characters blink
   blink = !blink;
 } while (ch != KEY_ESCAPE && ch != 'q' && ch != 'Q' && ch != ' ' &&
          ch != '\n');
 timeout(-1);
 werase(w_map);
 wrefresh(w_map);
 delwin(w_map);
 werase(w_search);
 wrefresh(w_search);
 delwin(w_search);
 erase();
 g->refresh_all();
 return ret;
}

void overmap::first_house(int &x, int &y)
{
 std::vector<point> valid;
 for (int i = 0; i < OMAPX; i++) {
  for (int j = 0; j < OMAPY; j++) {
   if (ter(i, j) == ot_shelter)
    valid.push_back( point(i, j) );
  }
 }
 if (valid.size() == 0) {
  debugmsg("Couldn't find a shelter!");
  x = 1;
  y = 1;
  return;
 }
 int index = rng(0, valid.size() - 1);
 x = valid[index].x;
 y = valid[index].y;
}

void overmap::process_mongroups()
{
 for (auto& _group : zg) {
   if (!_group.dying) continue;
   rational_scale<4, 5>(_group.population); // _group.population *= .8;
   rational_scale<9, 10>(_group.radius);    // _group.radius *= .9;
 }
}
  
void overmap::place_forest()
{
 int x, y;
 int forx;
 int fory;
 int fors;
 for (int i = 0; i < NUM_FOREST; i++) {
// forx and fory determine the epicenter of the forest
  forx = rng(0, OMAPX - 1);
  fory = rng(0, OMAPY - 1);
// fors determinds its basic size
  fors = rng(15, 40);
  for (int j = 0; j < cities.size(); j++) {
   while (dist(forx,fory,cities[j].x,cities[j].y) - fors / 2 < cities[j].s ) {
// Set forx and fory far enough from cities
    forx = rng(0, OMAPX - 1);
    fory = rng(0, OMAPY - 1);
// Set fors to determine the size of the forest; usually won't overlap w/ cities
    fors = rng(15, 40);
    j = 0;
   }
  } 
  int swamps = SWAMPINESS;	// How big the swamp may be...
  x = forx;
  y = fory;
// Depending on the size on the forest...
  for (int j = 0; j < fors; j++) {
   int swamp_chance = 0;
   for (int k = -2; k <= 2; k++) {
    for (int l = -2; l <= 2; l++) {
     if (ter(x + k, y + l) == ot_forest_water ||
         (ter(x+k, y+l) >= ot_river_center && ter(x+k, y+l) <= ot_river_nw))
      swamp_chance += 5;
    }  
   }
   bool swampy = false;
   if (swamps > 0 && swamp_chance > 0 && !one_in(swamp_chance) &&
       (ter(x, y) == ot_forest || ter(x, y) == ot_forest_thick ||
        ter(x, y) == ot_field  || one_in(SWAMPCHANCE))) {
// ...and make a swamp.
    ter(x, y) = ot_forest_water;
    swampy = true;
    swamps--;
   } else if (swamp_chance == 0)
    swamps = SWAMPINESS;
   if (ter(x, y) == ot_field)
    ter(x, y) = ot_forest;
   else if (ter(x, y) == ot_forest)
    ter(x, y) = ot_forest_thick;

   if (swampy && (ter(x, y-1) == ot_field || ter(x, y-1) == ot_forest))
    ter(x, y-1) = ot_forest_water;
   else if (ter(x, y-1) == ot_forest)
    ter(x, y-1) = ot_forest_thick;
   else if (ter(x, y-1) == ot_field)
    ter(x, y-1) = ot_forest;

   if (swampy && (ter(x, y+1) == ot_field || ter(x, y+1) == ot_forest))
    ter(x, y+1) = ot_forest_water;
   else if (ter(x, y+1) == ot_forest)
     ter(x, y+1) = ot_forest_thick;
   else if (ter(x, y+1) == ot_field)
     ter(x, y+1) = ot_forest;

   if (swampy && (ter(x-1, y) == ot_field || ter(x-1, y) == ot_forest))
    ter(x-1, y) = ot_forest_water;
   else if (ter(x-1, y) == ot_forest)
    ter(x-1, y) = ot_forest_thick;
   else if (ter(x-1, y) == ot_field)
     ter(x-1, y) = ot_forest;

   if (swampy && (ter(x+1, y) == ot_field || ter(x+1, y) == ot_forest))
    ter(x+1, y) = ot_forest_water;
   else if (ter(x+1, y) == ot_forest)
    ter(x+1, y) = ot_forest_thick;
   else if (ter(x+1, y) == ot_field)
    ter(x+1, y) = ot_forest;
// Random walk our forest
   x += rng(-2, 2);
   if (x < 0    ) x = 0;
   if (x > OMAPX) x = OMAPX;
   y += rng(-2, 2);
   if (y < 0    ) y = 0;
   if (y > OMAPY) y = OMAPY;
  }
 }
}

void overmap::place_river(point pa, point pb)
{
 int x = pa.x, y = pa.y;
 do {
  x += rng(-1, 1);
  y += rng(-1, 1);
  if (x < 0) x = 0;
  if (x > OMAPX - 1) x = OMAPX - 2;
  if (y < 0) y = 0;
  if (y > OMAPY - 1) y = OMAPY - 1;
  for (int i = -1; i <= 1; i++) {
   for (int j = -1; j <= 1; j++) {
    if (y+i >= 0 && y+i < OMAPY && x+j >= 0 && x+j < OMAPX)
     ter(x+j, y+i) = ot_river_center;
   }
  }
  if (pb.x > x && (rng(0, int(OMAPX * 1.2) - 1) < pb.x - x ||
                   (rng(0, int(OMAPX * .2) - 1) > pb.x - x &&
                    rng(0, int(OMAPY * .2) - 1) > abs(pb.y - y))))
   x++;
  if (pb.x < x && (rng(0, int(OMAPX * 1.2) - 1) < x - pb.x ||
                   (rng(0, int(OMAPX * .2) - 1) > x - pb.x &&
                    rng(0, int(OMAPY * .2) - 1) > abs(pb.y - y))))
   x--;
  if (pb.y > y && (rng(0, int(OMAPY * 1.2) - 1) < pb.y - y ||
                   (rng(0, int(OMAPY * .2) - 1) > pb.y - y &&
                    rng(0, int(OMAPX * .2) - 1) > abs(x - pb.x))))
   y++;
  if (pb.y < y && (rng(0, int(OMAPY * 1.2) - 1) < y - pb.y ||
                   (rng(0, int(OMAPY * .2) - 1) > y - pb.y &&
                    rng(0, int(OMAPX * .2) - 1) > abs(x - pb.x))))
   y--;
  x += rng(-1, 1);
  y += rng(-1, 1);
  if (x < 0) x = 0;
  if (x > OMAPX - 1) x = OMAPX - 2;
  if (y < 0) y = 0;
  if (y > OMAPY - 1) y = OMAPY - 1;
  for (int i = -1; i <= 1; i++) {
   for (int j = -1; j <= 1; j++) {
// We don't want our riverbanks touching the edge of the map for many reasons
    if ((y+i >= 1 && y+i < OMAPY - 1 && x+j >= 1 && x+j < OMAPX - 1) ||
// UNLESS, of course, that's where the river is headed!
        (abs(pb.y - (y+i)) < 4 && abs(pb.x - (x+j)) < 4))
     ter(x+j, y+i) = ot_river_center;
   }
  }
 } while (pb.x != x || pb.y != y);
}

void overmap::place_cities(std::vector<city> &cities, int min)
{
 int NUM_CITIES = dice(2, 7) + rng(min, min + 4);
 int cx, cy, cs;
 int start_dir;

 for (int i = 0; i < NUM_CITIES; i++) {
  cx = rng(20, OMAPX - 41);
  cy = rng(20, OMAPY - 41);
  cs = rng(4, 17);
  if (ter(cx, cy) == ot_field) {
   ter(cx, cy) = ot_road_nesw;
   cities.push_back(city(cx, cy, cs));
   const auto& tmp = cities.back();
   start_dir = rng(0, 3);
   for (int j = 0; j < 4; j++)
    make_road(cx, cy, cs, (start_dir + j) % 4, tmp);
  }
 }
}

void overmap::put_buildings(int x, int y, int dir, const city& town)
{
 int ychange = dir % 2, xchange = (dir + 1) % 2;
 for (int i = -1; i <= 1; i += 2) {
  auto& terrain = ter(x + i * xchange, y + i * ychange);
  if ((ot_field == terrain) && !one_in(STREETCHANCE)) {
   const auto c_dist = dist(x, y, town.x, town.y) / town.s;
   static_assert(std::is_floating_point_v<decltype(c_dist)>);   // above truncates for integral types
   try {
     if (rng(0, 99) > 80 * c_dist) terrain = shop(((dir%2)-i)%4);
     else if (rng(0, 99) > 130 * c_dist) terrain = ot_park;
     else terrain = house(((dir%2)-i)%4);
   } catch (const std::string& e) {
     debugmsg(e.c_str());
     continue;
   }
  }
 }
}

void overmap::make_road(int cx, int cy, int cs, int dir, const city& town)
{
 int x = cx, y = cy;
 int c = cs, croad = cs;
 switch (dir) {
 case 0:
  while (c > 0 && y > 0 && (ter(x, y-1) == ot_field || c == cs)) {
   y--;
   c--;
   ter(x, y) = ot_road_ns;
   for (int i = -1; i <= 0; i++) {
    for (int j = -1; j <= 1; j++) {
     if (abs(j) != abs(i) && (ter(x+j, y+i) == ot_road_ew ||
                              ter(x+j, y+i) == ot_road_ns)) {
      ter(x, y) = ot_road_null;
      c = -1;
     }
    }
   }
   put_buildings(x, y, dir, town);
   if (c < croad - 1 && c >= 2 && ter(x - 1, y) == ot_field &&
                                  ter(x + 1, y) == ot_field) {
    croad = c;
    make_road(x, y, cs - rng(1, 3), 1, town);
    make_road(x, y, cs - rng(1, 3), 3, town);
   }
  }
  if (is_road(x, y-2))
   ter(x, y-1) = ot_road_ns;
  break;
 case 1:
  while (c > 0 && x < OMAPX-1 && (ter(x+1, y) == ot_field || c == cs)) {
   x++;
   c--;
   ter(x, y) = ot_road_ew;
   for (int i = -1; i <= 1; i++) {
    for (int j = 0; j <= 1; j++) {
     if (abs(j) != abs(i) && (ter(x+j, y+i) == ot_road_ew ||
                              ter(x+j, y+i) == ot_road_ns)) {
      ter(x, y) = ot_road_null;
      c = -1;
     }
    }
   }
   put_buildings(x, y, dir, town);
   if (c < croad-2 && c >= 3 && ter(x, y-1) == ot_field &&
                                ter(x, y+1) == ot_field) {
    croad = c;
    make_road(x, y, cs - rng(1, 3), 0, town);
    make_road(x, y, cs - rng(1, 3), 2, town);
   }
  }
  if (is_road(x-2, y))
   ter(x-1, y) = ot_road_ew;
  break;
 case 2:
  while (c > 0 && y < OMAPY-1 && (ter(x, y+1) == ot_field || c == cs)) {
   y++;
   c--;
   ter(x, y) = ot_road_ns;
   for (int i = 0; i <= 1; i++) {
    for (int j = -1; j <= 1; j++) {
     if (abs(j) != abs(i) && (ter(x+j, y+i) == ot_road_ew ||
                              ter(x+j, y+i) == ot_road_ns)) {
      ter(x, y) = ot_road_null;
      c = -1;
     }
    }
   }
   put_buildings(x, y, dir, town);
   if (c < croad-2 && ter(x-1, y) == ot_field && ter(x+1, y) == ot_field) {
    croad = c;
    make_road(x, y, cs - rng(1, 3), 1, town);
    make_road(x, y, cs - rng(1, 3), 3, town);
   }
  }
  if (is_road(x, y+2))
   ter(x, y+1) = ot_road_ns;
  break;
 case 3:
  while (c > 0 && x > 0 && (ter(x-1, y) == ot_field || c == cs)) {
   x--;
   c--;
   ter(x, y) = ot_road_ew;
   for (int i = -1; i <= 1; i++) {
    for (int j = -1; j <= 0; j++) {
     if (abs(j) != abs(i) && (ter(x+j, y+i) == ot_road_ew ||
                              ter(x+j, y+i) == ot_road_ns)) {
      ter(x, y) = ot_road_null;
      c = -1;
     }
    }
   }
   put_buildings(x, y, dir, town);
   if (c < croad - 2 && c >= 3 && ter(x, y-1) == ot_field &&
       ter(x, y+1) == ot_field) {
    croad = c;
    make_road(x, y, cs - rng(1, 3), 0, town);
    make_road(x, y, cs - rng(1, 3), 2, town);
   }
  }
  if (is_road(x+2, y))
   ter(x+1, y) = ot_road_ew;
  break;
 }
 cs -= rng(1, 3);
 if (cs >= 2 && c == 0) {
  int dir2;
  if (dir % 2 == 0)
   dir2 = rng(0, 1) * 2 + 1;
  else
   dir2 = rng(0, 1) * 2;
  make_road(x, y, cs, dir2, town);
  if (one_in(5))
   make_road(x, y, cs, (dir2 + 2) % 4, town);
 }
}

void overmap::build_lab(const city& origin)
{
 ter(origin.x, origin.y) = ot_lab;
 for (int n = 0; n <= 1; n++) {	// Do it in two passes to allow diagonals
  for (int i = 1; i <= origin.s; i++) {
   for (int lx = origin.x - i; lx <= origin.x + i; lx++) {
    for (int ly = origin.y - i; ly <= origin.y + i; ly++) {
     if ((ter(lx - 1, ly) == ot_lab || ter(lx + 1, ly) == ot_lab ||
         ter(lx, ly - 1) == ot_lab || ter(lx, ly + 1) == ot_lab) &&
         one_in(i))
      ter(lx, ly) = ot_lab;
    }
   }
  }
 }
 ter(origin.x, origin.y) = ot_lab_core;
 int numstairs = 0;
 if (origin.s > 1) {	// Build stairs going down
  while (!one_in(6)) {
   int tries = 0;
   do {
	point stair(rng(origin.x - origin.s, origin.x + origin.s), rng(origin.y - origin.s, origin.y + origin.s));
	auto& terrain = ter(stair);
	if (ot_lab == terrain) {
      terrain = ot_lab_stairs;
      numstairs++;
      break;
	}
   } while (++tries < 15);
  }
 }
 if (numstairs == 0) {	// This is the bottom of the lab;  We need a finale
  std::vector<point> valid;
  point pt;
  for (pt.x = origin.x - origin.s; pt.x <= origin.x + origin.s; pt.x++) {
      for (pt.y = origin.y - origin.s; pt.y <= origin.y + origin.s; pt.y++) {
          if (any<ot_lab, ot_lab_core>(ter(pt))) valid.push_back(pt);
      }
  }
// assert(!valid.empty());  // we set a terrain explicitly to ot_lab_core so should be fine
  ter(valid[rng(0, valid.size() - 1)]) = ot_lab_finale;
 }
 zg.push_back(mongroup(mcat_lab, (origin.x * 2), (origin.y * 2), origin.s, 400));
}

void overmap::build_anthill(const city& origin)
{
 build_tunnel(origin.x, origin.y, origin.s - rng(0, 3), 0);
 build_tunnel(origin.x, origin.y, origin.s - rng(0, 3), 1);
 build_tunnel(origin.x, origin.y, origin.s - rng(0, 3), 2);
 build_tunnel(origin.x, origin.y, origin.s - rng(0, 3), 3);
 std::vector<point> queenpoints;
 for (int i = origin.x - origin.s; i <= origin.x + origin.s; i++) {
  for (int j = origin.y - origin.s; j <= origin.y + origin.s; j++) {
   if (is_between<ot_ants_ns, ot_ants_nesw>(ter(i, j)))
    queenpoints.push_back(point(i, j));
  }
 }
 ter(queenpoints[rng(0, queenpoints.size() - 1)]) = ot_ants_queen;
}

void overmap::build_tunnel(int x, int y, int s, int dir)	// for ants nests
{
 if (s <= 0) return;
 const point origin(x, y);
 if (!is_between<ot_ants_ns, ot_ants_queen>(ter(x, y))) ter(origin) = ot_ants_ns;
 point next;
 if (1 == s) next = point(-1, -1);
 else next = origin + direction_vector((direction)(2 * dir));

 std::vector<std::pair<int,point> > valid;
 for (int d = 0; d <= 3; d++) {
     point pt = origin + direction_vector((direction)(2 * d));
     if (pt != next && !is_between<ot_ants_ns, ot_ants_queen>(ter(pt))) valid.push_back(std::pair(d,pt));
 }

 for (const auto& pt : valid) {
   if (one_in(s * 2)) {
    ter(pt.second) = one_in(2) ? ot_ants_food : ot_ants_larvae;
   } else if (one_in(5)) {
    build_tunnel(pt.second.x, pt.second.y, s - rng(0, 3), pt.first);
   }
 }
 build_tunnel(next.x, next.y, s - 1, dir);
}

void overmap::build_slimepit(const city& origin)
{
 for (int n = 1; n <= origin.s; n++) {
  for (int i = origin.x - n; i <= origin.x + n; i++) {
   for (int j = origin.y - n; j <= origin.y + n; j++) {
    if (rng(1, origin.s * 2) >= n)
     ter(i, j) = (one_in(8) ? ot_slimepit_down : ot_slimepit);
    }
   }
 }
}

void overmap::build_mine(city origin)
{
 const bool finale = (origin.s <= rng(1, 3));
 int built = 0;
 const int max_depth = (origin.s < 2) ? 2 : origin.s;
 while (built < max_depth) {
  ter(origin.x, origin.y) = ot_mine;
  std::vector<point> next;
  for (int i = -1; i <= 1; i += 2) {
   point pt(origin.x, origin.y + i);
   if (ter(pt) == ot_rock) next.push_back(pt);
   pt = point(origin.x + i, origin.y);
   if (ter(pt) == ot_rock) next.push_back(pt);
  }
  if (next.empty()) { // Dead end!  Go down!
   ter(origin.x, origin.y) = (finale ? ot_mine_finale : ot_mine_down);
   return;
  }
  point p = next[ rng(0, next.size() - 1) ];
  origin.x = p.x;
  origin.y = p.y;
  built++;
 }
 ter(origin.x, origin.y) = (finale ? ot_mine_finale : ot_mine_down);
}

void overmap::place_rifts()
{
 int num_rifts = rng(0, 2) * rng(0, 2);
 std::vector<point> riftline;
 if (!one_in(4))
  num_rifts++;
 for (int n = 0; n < num_rifts; n++) {
  int x = rng(MAX_RIFT_SIZE, OMAPX - MAX_RIFT_SIZE);
  int y = rng(MAX_RIFT_SIZE, OMAPY - MAX_RIFT_SIZE);
  int xdist = rng(MIN_RIFT_SIZE, MAX_RIFT_SIZE),
      ydist = rng(MIN_RIFT_SIZE, MAX_RIFT_SIZE);
// We use rng(0, 10) as the t-value for this Bresenham Line, because by
// repeating this twice, we can get a thick line, and a more interesting rift.
  for (int o = 0; o < 3; o++) {
   if (xdist > ydist)
    riftline = line_to(x - xdist, y - ydist+o, x + xdist, y + ydist, rng(0,10));
   else
    riftline = line_to(x - xdist+o, y - ydist, x + xdist, y + ydist, rng(0,10));
   for (int i = 0; i < riftline.size(); i++) {
    if (i == riftline.size() / 2 && !one_in(3))
     ter(riftline[i].x, riftline[i].y) = ot_hellmouth;
    else
     ter(riftline[i].x, riftline[i].y) = ot_rift;
   }
  }
 }
}

void overmap::make_hiway(int x1, int y1, int x2, int y2, oter_id base)
{
 std::vector<point> next;
 int dir = 0;
 int x = x1, y = y1;
 int xdir, ydir;
 int tmp = 0;
 bool bridge_is_okay = false;
 bool found_road = false;
 do {
  next.clear(); // Clear list of valid points
  // Add valid points -- step in the right x-direction
  if (x2 > x) next.push_back(point(x + 1, y));
  else if (x2 < x) next.push_back(point(x - 1, y));
  else next.push_back(point(-1, -1)); // X is right--don't change it!

  // Add valid points -- step in the right y-direction
  if (y2 > y) next.push_back(point(x, y + 1));
  else if (y2 < y) next.push_back(point(x, y - 1));

  for (int i = 0; i < next.size(); i++) { // Take an existing road if we can
   if (next[i].x != -1 && is_road(base, next[i].x, next[i].y)) {
    x = next[i].x;
    y = next[i].y;
    dir = i; // We are moving... whichever way that highway is moving
// If we're closer to the destination than to the origin, this highway is done!
    if (dist(x, y, x1, y1) > dist(x, y, x2, y2)) return;
    next.clear();
   } 
  }
  if (!next.empty()) { // Assuming we DIDN'T take an existing road...
   if (next[0].x == -1) { // X is correct, so we're taking the y-change
    dir = 1; // We are moving vertically
    x = next[1].x;
    y = next[1].y;
	auto& terrain = ter(x, y);
    if (is_river(terrain)) terrain = ot_bridge_ns;
    else if (!is_road(base, x, y)) terrain = base;
   } else if (next.size() == 1) { // Y must be correct, take the x-change
    if (dir == 1) ter(x, y) = base;
    dir = 0; // We are moving horizontally
    x = next[0].x;
    y = next[0].y;
	auto& terrain = ter(x, y);
	if (is_river(terrain)) terrain = ot_bridge_ew;
    else if (!is_road(base, x, y)) terrain = base;
   } else {	// More than one eligable route; pick one randomly
    if (one_in(12) &&
       !is_river(ter(next[(dir + 1) % 2].x, next[(dir + 1) % 2].y)))
     dir = (dir + 1) % 2; // Switch the direction (hori/vert) in which we move
    x = next[dir].x;
    y = next[dir].y;
    if (dir == 0) {	// Moving horizontally
     if (is_river(ter(x, y))) {
      xdir = -1;
      bridge_is_okay = true;
      if (x2 > x) xdir = 1;
      tmp = x;
      while (is_river(ter(tmp, y))) {
       if (is_road(base, tmp, y)) bridge_is_okay = false;	// Collides with another bridge!
       tmp += xdir;
      }
      if (bridge_is_okay) {
       while(is_river(ter(x, y))) {
        ter(x, y) = ot_bridge_ew;
        x += xdir;
       }
       ter(x, y) = base;
      }
     } else if (!is_road(base, x, y))
      ter(x, y) = base;
    } else {		// Moving vertically
     if (is_river(ter(x, y))) {
      ydir = -1;
      bridge_is_okay = true;
      if (y2 > y) ydir = 1;
      tmp = y;
      while (is_river(ter(x, tmp))) {
       if (is_road(base, x, tmp)) bridge_is_okay = false;	// Collides with another bridge!
       tmp += ydir;
      }
      if (bridge_is_okay) {
       while (is_river(ter(x, y))) {
        ter(x, y) = ot_bridge_ns;
        y += ydir;
       }
       ter(x, y) = base;
      }
     } else if (!is_road(base, x, y))
      ter(x, y) = base;
    }
   }
#if OVERMAP_HAS_BUILDINGS_ON_HIGHWAY
   if (one_in(50) && posz == 0)
    building_on_hiway(x, y, dir);
#endif
  }
  found_road = (
        (is_between<ot_road_null, ot_river_center>(ter(x, y - 1)) ||
         is_between<ot_road_null, ot_river_center>(ter(x, y + 1)) ||
         is_between<ot_road_null, ot_river_center>(ter(x - 1, y)) ||
         is_between<ot_road_null, ot_river_center>(ter(x + 1, y))  ) &&
      rl_dist(x, y, x1, y2) > rl_dist(x, y, x2, y2));
 } while ((x != x2 || y != y2) && !found_road);
}

#if OVERMAP_HAS_BUILDINGS_ON_HIGHWAY
void overmap::building_on_hiway(int x, int y, int dir)
{
 int xdif = dir * (1 - 2 * rng(0,1));
 int ydif = (1 - dir) * (1 - 2 * rng(0,1));
 int rot = 0;
      if (ydif ==  1)
  rot = 0;
 else if (xdif == -1)
  rot = 1;
 else if (ydif == -1)
  rot = 2;
 else if (xdif ==  1)
  rot = 3;

 switch (rng(1, 3)) {
 case 1:
  if (!is_river(ter(x + xdif, y + ydif)))
   ter(x + xdif, y + ydif) = ot_lab_stairs;
  break;
 case 2:
  if (!is_river(ter(x + xdif, y + ydif)))
   ter(x + xdif, y + ydif) = house(rot);
  break;
 case 3:
  if (!is_river(ter(x + xdif, y + ydif)))
   ter(x + xdif, y + ydif) = ot_radio_tower;
  break;
/*
 case 4:
  if (!is_river(ter(x + xdif, y + ydif)))
   ter(x + xdir, y + ydif) = ot_sewage_treatment;
  break;
*/
 }
}
#endif

void overmap::place_hiways(const std::vector<city>& cities, oter_id base)
{
 if (1 >= cities.size()) return;
 city best;
 int distance;
 for (int i = 0; i < cities.size(); i++) {
  const auto& src = cities[i];
  bool maderoad = false;
  int closest = -1;
  for (int j = i + 1; j < cities.size(); j++) {
   const auto& dest = cities[j];
   distance = dist(src.x, src.y, dest.x, dest.y);
   if (distance < closest || closest < 0) {
    closest = distance; 
    best = dest;
   }
   if (distance < TOP_HIWAY_DIST) {
    maderoad = true;
    make_hiway(src.x, src.y, dest.x, dest.y, base);
   }
  }
  if (!maderoad && closest > 0)
   make_hiway(src.x, src.y, best.x, best.y, base);
 }
}

// Polish does both good_roads and good_rivers (and any future polishing) in
// a single loop; much more efficient
void overmap::polish(oter_id min, oter_id max)
{
// Main loop--checks roads and rivers that aren't on the borders of the map
 for (int x = 0; x < OMAPX; x++) {
  for (int y = 0; y < OMAPY; y++) {
   auto& terrain = ter(x, y);
   if (terrain >= min && terrain <= max) {
    if (is_between<ot_road_null, ot_road_nesw>(terrain))
     good_road(ot_road_ns, x, y);
    else if (is_between<ot_bridge_ns, ot_bridge_ew>(terrain) &&
		     is_between<ot_bridge_ns, ot_bridge_ew>(ter(x - 1, y)) &&
		     is_between<ot_bridge_ns, ot_bridge_ew>(ter(x + 1, y)) &&
		     is_between<ot_bridge_ns, ot_bridge_ew>(ter(x, y - 1)) &&
		     is_between<ot_bridge_ns, ot_bridge_ew>(ter(x, y + 1)))
     terrain = ot_road_nesw;
    else if (is_between<ot_subway_ns, ot_subway_nesw>(terrain))
     good_road(ot_subway_ns, x, y);
    else if (is_between<ot_sewer_ns, ot_sewer_nesw>(terrain))
     good_road(ot_sewer_ns, x, y);
    else if (is_between<ot_ants_ns, ot_ants_nesw>(terrain))
     good_road(ot_ants_ns, x, y);
    else if (is_between<ot_river_center, ot_river_nw>(terrain))
     good_river(x, y);
// Sometimes a bridge will start at the edge of a river, and this looks ugly
// So, fix it by making that square normal road; bit of a kludge but it works
    else if (terrain == ot_bridge_ns &&
             (!is_river(ter(x - 1, y)) || !is_river(ter(x + 1, y))))
     terrain = ot_road_ns;
    else if (terrain == ot_bridge_ew &&
             (!is_river(ter(x, y - 1)) || !is_river(ter(x, y + 1))))
     terrain = ot_road_ew;
   }
  }
 }
// Fixes stretches of parallel roads--turns them into two-lane highways
// Note that this fixes 2x2 areas... a "tail" of 1x2 parallel roads may be left.
// This can actually be a good thing; it ensures nice connections
// Also, this leaves, say, 3x3 areas of road.  TODO: fix this?  courtyards etc?
 for (int y = 0; y < OMAPY - 1; y++) {
  for (int x = 0; x < OMAPX - 1; x++) {
   auto& terrain = ter(x, y);
   if (terrain >= min && terrain <= max) {
    if (terrain == ot_road_nes && ter(x+1, y) == ot_road_nsw &&
        ter(x, y+1) == ot_road_nes && ter(x+1, y+1) == ot_road_nsw) {
     terrain = ot_hiway_ns;
     ter(x+1, y) = ot_hiway_ns;
     ter(x, y+1) = ot_hiway_ns;
     ter(x+1, y+1) = ot_hiway_ns;
    } else if (terrain == ot_road_esw && ter(x+1, y) == ot_road_esw &&
               ter(x, y+1) == ot_road_new && ter(x+1, y+1) == ot_road_new) {
     terrain = ot_hiway_ew;
     ter(x+1, y) = ot_hiway_ew;
     ter(x, y+1) = ot_hiway_ew;
     ter(x+1, y+1) = ot_hiway_ew;
    }
   }
  }
 }
}

bool overmap::is_road(int x, int y) const
{
 const auto terrain = ter(x,y);
 if (terrain == ot_rift || terrain == ot_hellmouth)
  return true;
 if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY) {
  for (int i = 0; i < roads_out.size(); i++) {
   if (abs(roads_out[i].x - x) + abs(roads_out[i].y - y) <= 1)
    return true;
  }
 }
 if (is_between<ot_road_null, ot_bridge_ew>(terrain) ||
     is_between<ot_subway_ns, ot_subway_nesw>(terrain) ||
     is_between<ot_sewer_ns, ot_sewer_nesw>(terrain) ||
     terrain == ot_sewage_treatment_hub ||
     terrain == ot_sewage_treatment_under)
  return true;
 return false;
}

bool overmap::is_road(oter_id base, int x, int y) const
{
 oter_id min, max;
 if (is_between<ot_road_null, ot_bridge_ew>(base)) {
  min = ot_road_null;
  max = ot_bridge_ew;
 } else if (is_between<ot_subway_ns, ot_subway_nesw>(base)) {
  min = ot_subway_station;
  max = ot_subway_nesw;
 } else if (is_between<ot_sewer_ns, ot_sewer_nesw>(base)) {
  min = ot_sewer_ns;
  max = ot_sewer_nesw;
  const auto terrain = ter(x, y);
  if (terrain == ot_sewage_treatment_hub || terrain == ot_sewage_treatment_under)
   return true;
 } else if (is_between<ot_ants_ns, ot_ants_queen>(base)) {
  min = ot_ants_ns;
  max = ot_ants_queen;
 } else	{ // Didn't plan for this!
  debugmsg("Bad call to is_road, %s", oter_t::list[base].name.c_str());
  return false;
 }
 if (x < 0 || x >= OMAPX || y < 0 || y >= OMAPY) {
  for (int i = 0; i < roads_out.size(); i++) {
   if (abs(roads_out[i].x - x) + abs(roads_out[i].y - y) <= 1)
    return true;
  }
 }
 const auto terrain = ter(x, y);
 return terrain >= min && terrain <= max;
}

void overmap::good_road(oter_id base, int x, int y)
{
 int d = ot_road_ns;
 if (is_road(base, x, y-1)) {
  if (is_road(base, x+1, y)) { 
   if (is_road(base, x, y+1)) {
    if (is_road(base, x-1, y))
     ter(x, y) = oter_id(base + ot_road_nesw - d);
    else
     ter(x, y) = oter_id(base + ot_road_nes - d);
   } else {
    if (is_road(base, x-1, y))
     ter(x, y) = oter_id(base + ot_road_new - d);
    else
     ter(x, y) = oter_id(base + ot_road_ne - d);
   } 
  } else {
   if (is_road(base, x, y+1)) {
    if (is_road(base, x-1, y))
     ter(x, y) = oter_id(base + ot_road_nsw - d);
    else
     ter(x, y) = oter_id(base + ot_road_ns - d);
   } else {
    if (is_road(base, x-1, y))
     ter(x, y) = oter_id(base + ot_road_wn - d);
    else
     ter(x, y) = oter_id(base + ot_road_ns - d);
   } 
  }
 } else {
  if (is_road(base, x+1, y)) { 
   if (is_road(base, x, y+1)) {
    if (is_road(base, x-1, y))
     ter(x, y) = oter_id(base + ot_road_esw - d);
    else
     ter(x, y) = oter_id(base + ot_road_es - d);
   } else
    ter(x, y) = oter_id(base + ot_road_ew - d);
  } else {
   if (is_road(base, x, y+1)) {
    if (is_road(base, x-1, y))
     ter(x, y) = oter_id(base + ot_road_sw - d);
    else
     ter(x, y) = oter_id(base + ot_road_ns - d);
   } else {
    if (is_road(base, x-1, y))
     ter(x, y) = oter_id(base + ot_road_ew - d);
    else {// No adjoining roads/etc. Happens occasionally, esp. with sewers.
     ter(x, y) = oter_id(base + ot_road_nesw - d);
    }
   } 
  }
 }
 if (ter(x, y) == ot_road_nesw && one_in(4))
  ter(x, y) = ot_road_nesw_manhole;
}

void overmap::good_river(int x, int y)
{
 if (is_river(ter(x - 1, y))) {
  if (is_river(ter(x, y - 1))) {
   if (is_river(ter(x, y + 1))) {
    if (is_river(ter(x + 1, y))) {
// River on N, S, E, W; but we might need to take a "bite" out of the corner
     if (!is_river(ter(x - 1, y - 1)))
      ter(x, y) = ot_river_c_not_nw;
     else if (!is_river(ter(x + 1, y - 1)))
      ter(x, y) = ot_river_c_not_ne;
     else if (!is_river(ter(x - 1, y + 1)))
      ter(x, y) = ot_river_c_not_sw;
     else if (!is_river(ter(x + 1, y + 1)))
      ter(x, y) = ot_river_c_not_se;
     else
      ter(x, y) = ot_river_center;
    } else
     ter(x, y) = ot_river_east;
   } else {
    if (is_river(ter(x + 1, y)))
     ter(x, y) = ot_river_south;
    else
     ter(x, y) = ot_river_se;
   }
  } else {
   if (is_river(ter(x, y + 1))) {
    if (is_river(ter(x + 1, y)))
     ter(x, y) = ot_river_north;
    else
     ter(x, y) = ot_river_ne;
   } else {
    if (is_river(ter(x + 1, y))) // Means it's swampy
     ter(x, y) = ot_forest_water;
   }
  }
 } else {
  if (is_river(ter(x, y - 1))) {
   if (is_river(ter(x, y + 1))) {
    if (is_river(ter(x + 1, y)))
     ter(x, y) = ot_river_west;
    else // Should never happen
     ter(x, y) = ot_forest_water;
   } else {
    if (is_river(ter(x + 1, y)))
     ter(x, y) = ot_river_sw;
    else // Should never happen
     ter(x, y) = ot_forest_water;
   }
  } else {
   if (is_river(ter(x, y + 1))) {
    if (is_river(ter(x + 1, y)))
     ter(x, y) = ot_river_nw;
    else // Should never happen
     ter(x, y) = ot_forest_water;
   } else // Should never happen
    ter(x, y) = ot_forest_water;
  }
 }
}

void overmap::place_specials()
{
 int placed[NUM_OMSPECS];
 for (int i = 0; i < NUM_OMSPECS; i++)
  placed[i] = 0;

 std::vector<point> sectors;
 for (int x = 0; x < OMAPX; x += OMSPEC_FREQ) {
  for (int y = 0; y < OMAPY; y += OMSPEC_FREQ)
   sectors.push_back(point(x, y));
 }

 while (!sectors.empty()) {
  int sector_pick = rng(0, sectors.size() - 1);
  int x = sectors[sector_pick].x, y = sectors[sector_pick].y;
  sectors.erase(sectors.begin() + sector_pick);
  std::vector<omspec_id> valid;
  int tries = 0;
  point p;
  do {
   p = point(rng(x, x + OMSPEC_FREQ - 1), rng(y, y + OMSPEC_FREQ - 1));
   if (p.x >= OMAPX - 1) p.x = OMAPX - 2;
   if (p.y >= OMAPY - 1) p.y = OMAPY - 2;
   if (p.x == 0) p.x = 1;
   if (p.y == 0) p.y = 1;
   for (int i = 0; i < NUM_OMSPECS; i++) {
    overmap_special special = overmap_special::specials[i];
    int min = special.min_dist_from_city, max = special.max_dist_from_city;
    if ((placed[i] < special.max_appearances || special.max_appearances <= 0) &&
        (min == -1 || dist_from_city(p) >= min) &&
        (max == -1 || dist_from_city(p) <= max) &&
        (special.able)(this, p))
     valid.push_back( omspec_id(i) );
   }
   tries++;
  } while (valid.empty() && tries < 15); // Done looking for valid spot

  if (tries < 15) { // We found a valid spot!
// Place the MUST HAVE ones first, to try and guarantee that they appear
   std::vector<omspec_id> must_place;
   for (int i = 0; i < valid.size(); i++) {
    if (placed[i] < overmap_special::specials[ valid[i] ].min_appearances)
     must_place.push_back(valid[i]);
   }
   if (must_place.empty()) {
    int selection = rng(0, valid.size() - 1);
    overmap_special special = overmap_special::specials[ valid[selection] ];
    placed[ valid[selection] ]++;
    place_special(special, p);
   } else {
    int selection = rng(0, must_place.size() - 1);
    overmap_special special = overmap_special::specials[ must_place[selection] ];
    placed[ must_place[selection] ]++;
    place_special(special, p);
   }
  } // Done with <Found a valid spot>

 } // Done picking sectors...
}

void overmap::place_special(overmap_special special, point p)
{
 bool rotated = false;
// First, place terrain...
 ter(p.x, p.y) = special.ter;
// Next, obey any special effects the flags might have
 if (special.flags & mfb(OMS_FLAG_ROTATE_ROAD)) {
  if (is_road(p.x, p.y - 1))
   rotated = true;
  else if (is_road(p.x + 1, p.y)) {
   ter(p.x, p.y) = oter_id( int(ter(p.x, p.y)) + 1);
   rotated = true;
  } else if (is_road(p.x, p.y + 1)) {
   ter(p.x, p.y) = oter_id( int(ter(p.x, p.y)) + 2);
   rotated = true;
  } else if (is_road(p.x - 1, p.y)) {
   ter(p.x, p.y) = oter_id( int(ter(p.x, p.y)) + 3);
   rotated = true;
  }
 }

 if (!rotated && special.flags & mfb(OMS_FLAG_ROTATE_RANDOM))
  ter(p.x, p.y) = oter_id( int(ter(p.x, p.y)) + rng(0, 3) );
  
 if (special.flags & mfb(OMS_FLAG_3X3)) {
  for (int x = -1; x <= 1; x++) {
   for (int y = -1; y <= 1; y++) {
    if (x == 0 && y == 0)
     y++; // Already handled
    point np(p.x + x, p.y + y);
    ter(np.x, np.y) = special.ter;
   }
  }
 }

 if (special.flags & mfb(OMS_FLAG_3X3_SECOND)) {
  int startx = p.x - 1, starty = p.y;
  if (is_road(p.x, p.y - 1)) { // Road to north
   startx = p.x - 1;
   starty = p.y;
  } else if (is_road(p.x + 1, p.y)) { // Road to east
   startx = p.x - 2;
   starty = p.y - 1;
  } else if (is_road(p.x, p.y + 1)) { // Road to south
   startx = p.x - 1;
   starty = p.y - 2;
  } else if (is_road(p.x - 1, p.y)) { // Road to west
   startx = p.x;
   starty = p.y - 1;
  }
  if (startx != -1) {
   for (int x = startx; x < startx + 3; x++) {
    for (int y = starty; y < starty + 3; y++)
     ter(x, y) = oter_id(special.ter + 1);
   }
   ter(p.x, p.y) = special.ter;
  }
 }

 if (special.flags & mfb(OMS_FLAG_BLOB)) {
  for (int x = -2; x <= 2; x++) {
   for (int y = -2; y <= 2; y++) {
    if (x == 0 && y == 0)
     y++; // Already handled
    point np(p.x + x, p.y + y);
    if (one_in(1 + abs(x) + abs(y)) && (special.able)(this, np))
     ter(p.x + x, p.y + y) = special.ter;
   }
  }
 }

 if (special.flags & mfb(OMS_FLAG_BIG)) {
  for (int x = -3; x <= 3; x++) {
   for (int y = -3; y <= 3; y++) {
    if (x == 0 && y == 0)
     y++; // Already handled
    point np(p.x + x, p.y + y);
    if ((special.able)(this, np))
     ter(p.x + x, p.y + y) = special.ter;
     ter(p.x + x, p.y + y) = special.ter;
   }
  }
 }

 if (special.flags & mfb(OMS_FLAG_ROAD)) {
  int closest = -1, distance = 999;
  for (int i = 0; i < cities.size(); i++) {
   int dist = rl_dist(p, cities[i].x, cities[i].y);
   if (dist < distance) {
    closest = i;
    distance = dist;
   }
  }
  make_hiway(p.x, p.y, cities[closest].x, cities[closest].y, ot_road_null);
 }

 if (special.flags & mfb(OMS_FLAG_PARKING_LOT)) {
  int closest = -1, distance = 999;
  for (int i = 0; i < cities.size(); i++) {
   int dist = rl_dist(p, cities[i].x, cities[i].y);
   if (dist < distance) {
    closest = i;
    distance = dist;
   }
  }
  ter(p.x, p.y - 1) = ot_s_lot;
  make_hiway(p.x, p.y - 1, cities[closest].x, cities[closest].y, ot_road_null);
 }

// Finally, place monsters if applicable
 if (special.monsters != mcat_null) {
  if (special.monster_pop_min == 0 || special.monster_pop_max == 0 ||
      special.monster_rad_min == 0 || special.monster_rad_max == 0   ) {
   debugmsg("Overmap special %s has bad spawn: pop(%d, %d) rad(%d, %d)",
            oter_t::list[special.ter].name.c_str(), special.monster_pop_min,
            special.monster_pop_max, special.monster_rad_min,
            special.monster_rad_max);
   return;
  }
       
  int population = rng(special.monster_pop_min, special.monster_pop_max);
  int radius     = rng(special.monster_rad_min, special.monster_rad_max);
  zg.push_back(
     mongroup(special.monsters, p.x * 2, p.y * 2, radius, population));
 }
}

void overmap::place_mongroups()
{
// Cities are full of zombies
 for (int i = 0; i < cities.size(); i++) {
  if (!one_in(16) || cities[i].s > 5)
   zg.push_back(
	mongroup(mcat_zombie, (cities[i].x * 2), (cities[i].y * 2),
	         int(cities[i].s * 2.5), cities[i].s * 80));
 }

// Figure out where swamps are, and place swamp monsters
 for (int x = 3; x < OMAPX - 3; x += 7) {
  for (int y = 3; y < OMAPY - 3; y += 7) {
   int swamp_count = 0;
   for (int sx = x - 3; sx <= x + 3; sx++) {
    for (int sy = y - 3; sy <= y + 3; sy++) {
     if (ter(sx, sy) == ot_forest_water)
      swamp_count += 2;
     else if (is_river(ter(sx, sy)))
      swamp_count++;
    }
   }
   if (swamp_count >= 25) // ~25% swamp or ~50% river
    zg.push_back(mongroup(mcat_swamp, x * 2, y * 2, 3,
                          rng(swamp_count * 8, swamp_count * 25)));
  }
 }

// Place the "put me anywhere" groups
 int numgroups = rng(0, 3);
 for (int i = 0; i < numgroups; i++) {
  zg.push_back(
	mongroup(mcat_worm, rng(0, OMAPX * 2 - 1), rng(0, OMAPY * 2 - 1),
	         rng(20, 40), rng(500, 1000)));
 }

// Forest groups cover the entire map
 zg.push_back(
	mongroup(mcat_forest, 0, OMAPY, OMAPY,
                 rng(2000, 12000)));
 zg.push_back(
	mongroup(mcat_forest, 0, OMAPY * 2 - 1, OMAPY,
                 rng(2000, 12000)));
 zg.push_back(
	mongroup(mcat_forest, OMAPX, 0, OMAPX,
                 rng(2000, 12000)));
 zg.push_back(
	mongroup(mcat_forest, OMAPX * 2 - 1, 0, OMAPX,
                 rng(2000, 12000)));
}

void overmap::place_radios()
{
 for (int i = 0; i < OMAPX; i++) {
  for (int j = 0; j < OMAPY; j++) {
   if (ter(i, j) == ot_radio_tower)
    radios.push_back(radio_tower(i*2, j*2, rng(80, 200),
   "This is the emergency broadcast system.  Please proceed quickly and calmly \
to your designated evacuation point."));
  }
 }
}

void overmap::save(const std::string& name, int x, int y, int z) const
{
 std::stringstream plrfilename, terfilename;
 plrfilename << "save/" << name << ".seen." << x << "." << y << "." << z;
 terfilename << "save/o." << x << "." << y << "." << z;

 std::ofstream fout(plrfilename.str().c_str());
 for (int j = 0; j < OMAPY; j++) {	// \todo good candidate for uuencoding
  for (int i = 0; i < OMAPX; i++) {
   if (seen(i, j))
    fout << "1";
   else
    fout << "0";
  }
  fout << std::endl;
 }
 for(const auto& n : notes) fout << "N " << n << std::endl;
 fout.close();
 fout.open(terfilename.str().c_str(), std::ios_base::trunc);
 for (int j = 0; j < OMAPY; j++) {
  for (int i = 0; i < OMAPX; i++)
   fout << char(int(ter(i, j)) + 32);
 }
 fout << std::endl;

 JSON saved(JSON::object);
 saved.set("groups", JSON::encode(zg));
 saved.set("cities", JSON::encode(cities));
 saved.set("roads", JSON::encode(roads_out));
 saved.set("radios", JSON::encode(radios));
 saved.set("npcs", JSON::encode(npcs));
 fout << saved;

 fout.close();
}

void overmap::saveall()
{
    om_cache::get().save();
    game::active()->cur_om.save(game::active()->u.name);
}

std::string overmap::terrain_filename(const tripoint& pos)
{
    std::stringstream terfilename;
    terfilename << "save/o." << pos.x << "." << pos.y << "." << pos.z;
    return terfilename.str();
}

void overmap::open(game *g)	// only called from constructor
{
 std::stringstream plrfilename;
 char datatype;

 plrfilename << "save/" << g->u.name << ".seen." << pos.x << "." << pos.y << "." << pos.z;

 const auto terfilename(terrain_filename(pos));
 std::ifstream fin(terfilename.c_str());
 if (fin.is_open()) {
  for (int j = 0; j < OMAPY; j++) {
   for (int i = 0; i < OMAPX; i++) {
	const auto ter_code = fin.get() - 32;
    ter(i, j) = oter_id(ter_code);
    if (ter_code < 0 || ter_code > num_ter_types)
     debugmsg("Loaded bad ter!  %s; ter %d", terfilename.c_str(), ter_code);
   }
  }
  if ('{' != (fin >> std::ws).peek()) {
	  debugmsg("Pre-V0.2.0 format?");
	  return;
  }
	  JSON om(fin);
	  if (om.has_key("groups")) om["groups"].decode(zg);
	  if (om.has_key("cities")) om["cities"].decode(cities);
	  if (om.has_key("roads")) om["roads"].decode(roads_out);
	  if (om.has_key("radios")) om["radios"].decode(radios);
      if (om.has_key("npcs")) {
          om["npcs"].decode(npcs);
          // V0.2.2 this is the earliest we can repair the tripoint field for OM_loc-retyped npc::goal
          for (auto& _npc : npcs) {
              if (_ref<decltype(_npc.goal)>::invalid.second != _npc.goal.second && _ref<decltype(_npc.goal)>::invalid.first == _npc.goal.first) {
                  // V0.2.1- goal is point.  Assume our own location.
                  _npc.goal.first = pos;
              }
          }
      }

// Private/per-character data
  fin.close();
  fin.open(plrfilename.str().c_str());
  if (fin.is_open()) {	// Load private seen data
   for (int j = 0; j < OMAPY; j++) {
    for (int i = 0; i < OMAPX; i++) {
     seen(i, j) = (fin.get() == '1');
    }
	fin >> std::ws;
   }
   while (fin >> datatype) {	// Load private notes
    if (datatype == 'N') notes.push_back(om_note(fin));
   }
   fin.close();
  } else {
   clear_seen();
  }
 } else if (pos.z <= -1) {	// No map exists, and we are underground!
// Fetch the terrain above
     generate_sub(&om_cache::get().r_create(tripoint(pos.x, pos.y, pos.z + 1)));    // \todo? fix type of generate_sub
 } else {	// No map exists!  Prepare neighbors, and generate one.
  std::vector<const overmap*> pointers;
// Fetch north and south
  for (int i = -1; i <= 1; i+=2) {
      pointers.push_back(om_cache::get().r_get(tripoint(pos.x, pos.y + i, pos.z)));
  }
// Fetch east and west
  for (int i = -1; i <= 1; i+=2) {
      pointers.push_back(om_cache::get().r_get(tripoint(pos.x + i, pos.y, pos.z)));
  }
// pointers looks like (north, south, west, east)
  generate(g, pointers[0], pointers[3], pointers[1], pointers[2]);
 }
}


// Overmap special placement functions

bool omspec_place::water(overmap *om, point p)
{
 oter_id ter = om->ter(p.x, p.y);
 return (ter >= ot_river_center && ter <= ot_river_nw);
}

bool omspec_place::land(overmap *om, point p)
{
 oter_id ter = om->ter(p.x, p.y);
 return (ter < ot_river_center || ter > ot_river_nw);
}

bool omspec_place::forest(overmap *om, point p)
{
 oter_id ter = om->ter(p.x, p.y);
 return (ter == ot_forest || ter == ot_forest_thick || ter == ot_forest_water);
}

bool omspec_place::wilderness(overmap *om, point p)
{
 oter_id ter = om->ter(p.x, p.y);
 return (ter == ot_forest || ter == ot_forest_thick || ter == ot_forest_water ||
         ter == ot_field);
}

bool omspec_place::by_highway(overmap *om, point p)
{
 oter_id north = om->ter(p.x, p.y - 1), east = om->ter(p.x + 1, p.y),
         south = om->ter(p.x, p.y + 1), west = om->ter(p.x - 1, p.y);

 return ((north == ot_hiway_ew || north == ot_road_ew) ||
         (east  == ot_hiway_ns || east  == ot_road_ns) ||
         (south == ot_hiway_ew || south == ot_road_ew) ||
         (west  == ot_hiway_ns || west  == ot_road_ns)   );
}
