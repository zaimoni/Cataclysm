#include "weather.h"
#include "game.h"
#include "rng.h"
#include "recent_msg.h"

static const char* const JSON_transcode[] = {
	"CLEAR",
	"SUNNY",
	"CLOUDY",
	"DRIZZLE",
	"RAINY",
	"THUNDER",
	"LIGHTNING",
	"ACID_DRIZZLE",
	"ACID_RAIN",
	"FLURRIES",
	"SNOW",
	"SNOWSTORM"
};

DEFINE_JSON_ENUM_SUPPORT_TYPICAL(weather_type, JSON_transcode)

struct weather_effect	// NPCs not really affected
{
	static void none(game *) {};
	static void glare(game *);
	static void wet(game *);
	static void very_wet(game *);
	static void thunder(game *);
	static void lightning(game *);
	static void light_acid(game *);
	static void acid(game *);
	static void flurry(game *) {};
	static void snow(game *) {};
	static void snowstorm(game *) {};
};

/* Name, color in UI, {seasonal temperatures}, ranged penalty, sight penalty,
* minimum time (in minutes), max time (in minutes), warn player?
* Note that max time is NOT when the weather is guaranteed to stop; it is
*  simply when the weather is guaranteed to be recalculated.  Most weather
*  patterns have "stay the same" as a highly likely transition; see below
*/
const weather_datum weather_datum::data[NUM_WEATHER_TYPES] = {
	{ "NULL Weather - BUG", c_magenta,
{ 0, 0, 0, 0 }, 0, 0, 0, 0, false,
&weather_effect::none },
{ "Clear", c_cyan,
{ 55, 85, 60, 30 }, 0, 0, 30, 120, false,
&weather_effect::none },
{ "Sunny", c_ltcyan,
{ 70, 100, 70, 40 }, 0, 0, 60, 300, false,
&weather_effect::glare },
{ "Cloudy", c_ltgray,
{ 50, 75, 60, 20 }, 0, 2, 60, 300, false,
&weather_effect::none },
{ "Drizzle", c_ltblue,
{ 45, 70, 45, 35 }, 1, 3, 10, 60, true,
&weather_effect::wet },
{ "Rain", c_blue,
{ 42, 65, 40, 30 }, 3, 5, 30, 180, true,
&weather_effect::very_wet },
{ "Thunder Storm", c_dkgray,
{ 42, 70, 40, 30 }, 4, 7, 30, 120, true,
&weather_effect::thunder },
{ "Lightning Storm", c_yellow,
{ 45, 52, 42, 32 }, 4, 8, 10, 30, true,
&weather_effect::lightning },
{ "Acidic Drizzle", c_ltgreen,
{ 45, 70, 45, 35 }, 2, 3, 10, 30, true,
&weather_effect::light_acid },
{ "Acid Rain", c_green,
{ 45, 70, 45, 30 }, 4, 6, 10, 30, true,
&weather_effect::acid },
{ "Flurries", c_white,
{ 30, 30, 30, 20 }, 2, 4, 10, 60, true,
&weather_effect::flurry },
{ "Snowing", c_white,
{ 25, 25, 20, 10 }, 4, 7, 30, 360, true,
&weather_effect::snow },
{ "Snowstorm", c_white,
{ 20, 20, 20,  5 }, 6, 10, 60, 180, true,
&weather_effect::snowstorm }
};

// a more complex weather modeling system would undoubtedly use functions, so keep these names to make that easier to do.
// These are 1 in turn rates (i.e. time scale sensitive)
static constexpr const int THUNDER_CHANCE = MINUTES(5);
static constexpr const int LIGHTNING_CHANCE = MINUTES(60);

void weather_effect::glare(game *g)
{
 if (g->is_in_sunlight(g->u.GPSpos)) g->u.infect(DI_GLARE, bp_eyes, 1, 2);
}

void weather_effect::wet(game *g)
{
 if (!g->u.is_wearing(itm_coat_rain) && !g->u.has_trait(PF_FEATHERS) && g->u.GPSpos.is_outside() && one_in(2))
  g->u.add_morale(MORALE_WET, -1, -30);
// Put out fires and reduce scent
 static auto extinguish = [&](const point& delta) {
	 auto dest = g->u.GPSpos + delta;
	 auto dest_pt = g->u.pos + delta;
	 if (dest.is_outside()) {
		 auto& fd = dest.field_at();
		 if (fd.type == fd_fire) fd.age += 15;
		 auto& sc = g->scent(dest_pt);
		 if (sc > 0) sc--;
	 }
 };

 forall_do_inclusive(within_rldist<2 * SEE>, extinguish);
}

void weather_effect::very_wet(game *g)
{
 if (!g->u.is_wearing(itm_coat_rain) && !g->u.has_trait(PF_FEATHERS) && g->u.GPSpos.is_outside())
  g->u.add_morale(MORALE_WET, -1, -60);
// Put out fires and reduce scent
 static auto extinguish = [&](const point& delta) {
	 auto dest = g->u.GPSpos + delta;
	 auto dest_pt = g->u.pos + delta;
	 if (dest.is_outside()) {
		 auto& fd = dest.field_at();
		 if (fd.type == fd_fire) fd.age += 45;
		 auto& sc = g->scent(dest_pt);
		 if (sc > 0) sc--;
	 }
 };

 forall_do_inclusive(within_rldist<2 * SEE>, extinguish);
}

void weather_effect::thunder(game *g)
{
 very_wet(g);
 if (one_in(THUNDER_CHANCE)) {
  if (g->lev.z >= 0)
   messages.add("You hear a distant rumble of thunder.");
  else if (!g->u.has_trait(PF_BADHEARING) && one_in(1 - 3 * g->lev.z))
   messages.add("You hear a rumble of thunder from above.");
 }
}

void weather_effect::lightning(game *g)
{
 thunder(g);
 if (one_in(LIGHTNING_CHANCE)) {
	 static auto ok = [&](point delta) {
		 const auto dest = g->u.GPSpos + delta;
		 if (0 == dest.move_cost() && dest.is_outside()) return std::optional<GPS_loc>(dest);
		 return std::optional<GPS_loc>();
	 };

	 const auto strike = grep(within_rldist<2 * SEE>, std::function(ok));
	 if (auto ub = strike.size()) {
		 messages.add("Lightning strikes nearby!");
		 strike[rng(0, ub - 1)].explosion(10, 0, one_in(4));
	 }
 }
}

void weather_effect::light_acid(game *g)
{
 wet(g);
 if (int(messages.turn) % 10 == 0 && g->u.GPSpos.is_outside())
  messages.add("The acid rain stings, but is harmless for now...");
}

// Zaimoni, 2020-09-16
// This is *far* more severe than the Leilani Estates/2018 eruption in Hawaii,
// as far as direct damage is concerned.  It's fine for a B-movie, but
// not a reality-simulator. (On the other hand, a reality-simulator
// has to require some sort of breathing protection.  [What happens
// when the fumes dissolve in phlegm?  This is far more concentrated than an onion's
// fumes, which dissolve in water as sulfuric acid.]  A B-movie has the discretion
// to omit that requirement.)
void weather_effect::acid(game *g)
{
	static auto make_puddle = [&](const point& delta) {
		auto dest = g->u.GPSpos + delta;
		auto terrain = dest.ter();
		if (!is<diggable>(terrain) && !is<noitem>(terrain) &&
			0 < dest.move_cost() && dest.is_outside() && one_in(MINUTES(40)))
			dest.add(field(fd_acid, 1));
	};

	static std::function<void(player&)> corrode_player = [](player& u) {
		const auto g = game::active();
		u.subjective_message("The acid rain burns!");
		if (one_in(6))
			u.hit(g, bp_head, 0, 0, 1);
		if (one_in(10)) {
			u.hit(g, bp_legs, 0, 0, 1);
			u.hit(g, bp_legs, 1, 0, 1);
		}
		if (one_in(8)) {
			u.hit(g, bp_feet, 0, 0, 1);
			u.hit(g, bp_feet, 1, 0, 1);
		}
		if (one_in(6))
			u.hit(g, bp_torso, 0, 0, 1);
		if (one_in(8)) {
			u.hit(g, bp_arms, 0, 0, 1);
			u.hit(g, bp_arms, 1, 0, 1);
		}
	};

 g->forall_do(corrode_player);

 // reality-simulator wants damage to trees, if not non-living map objects, here
 if (g->lev.z >= 0) forall_do_inclusive(within_rldist<2*SEE>, make_puddle);

 static auto corrode_monster = [](monster& z) {
	 if (z.GPSpos.is_outside() && !z.has_flag(MF_ACIDPROOF)) z.hurt(1);
 };

 g->forall_do(corrode_monster);
 very_wet(g);
}
