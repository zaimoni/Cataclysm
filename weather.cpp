#include "weather.h"
#include "game.h"
#include "recent_msg.h"
#include <vector>

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

#define PLAYER_OUTSIDE (g->m.is_outside(g->u.pos.x, g->u.pos.y) && g->lev.z >= 0)
#define THUNDER_CHANCE 50
#define LIGHTNING_CHANCE 600

void weather_effect::glare(game *g)
{
 if (g->is_in_sunlight(g->u.pos)) g->u.infect(DI_GLARE, bp_eyes, 1, 2);
}

void weather_effect::wet(game *g)
{
 if (!g->u.is_wearing(itm_coat_rain) && !g->u.has_trait(PF_FEATHERS) && PLAYER_OUTSIDE && one_in(2))
  g->u.add_morale(MORALE_WET, -1, -30);
// Put out fires and reduce scent
 for (int x = g->u.pos.x - SEEX * 2; x <= g->u.pos.x + SEEX * 2; x++) {
  for (int y = g->u.pos.y - SEEY * 2; y <= g->u.pos.y + SEEY * 2; y++) {
   if (g->m.is_outside(x, y)) {
	auto& fd = g->m.field_at(x, y);
    if (fd.type == fd_fire) fd.age += 15;
	auto& sc = g->scent(x, y);
    if (sc > 0) sc--;
   }
  }
 }
}

void weather_effect::very_wet(game *g)
{
 if (!g->u.is_wearing(itm_coat_rain) && !g->u.has_trait(PF_FEATHERS) && PLAYER_OUTSIDE)
  g->u.add_morale(MORALE_WET, -1, -60);
// Put out fires and reduce scent
 for (int x = g->u.pos.x - SEEX * 2; x <= g->u.pos.x + SEEX * 2; x++) {
  for (int y = g->u.pos.y - SEEY * 2; y <= g->u.pos.y + SEEY * 2; y++) {
   if (g->m.is_outside(x, y)) {
    auto& fd = g->m.field_at(x, y);
    if (fd.type == fd_fire) fd.age += 45;
	auto& sc = g->scent(x, y);
    if (sc > 0) sc--;
   }
  }
 }
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
  std::vector<point> strike;
  for (int x = g->u.pos.x - SEEX * 2; x <= g->u.pos.x + SEEX * 2; x++) {
   for (int y = g->u.pos.y - SEEY * 2; y <= g->u.pos.y + SEEY * 2; y++) {
    if (g->m.move_cost(x, y) == 0 && g->m.is_outside(x, y))
     strike.push_back(point(x, y));
   }
  }
  point hit;
  if (strike.size() > 0) {
   hit = strike[rng(0, strike.size() - 1)];
   messages.add("Lightning strikes nearby!");
   g->explosion(hit, 10, 0, one_in(4));
  }
 }
}

void weather_effect::light_acid(game *g)
{
 wet(g);
 if (int(messages.turn) % 10 == 0 && PLAYER_OUTSIDE)
  messages.add("The acid rain stings, but is harmless for now...");
}

void weather_effect::acid(game *g)
{
 if (PLAYER_OUTSIDE) {
  messages.add("The acid rain burns!");
  if (one_in(6))
   g->u.hit(g, bp_head, 0, 0, 1);
  if (one_in(10)) {
   g->u.hit(g, bp_legs, 0, 0, 1);
   g->u.hit(g, bp_legs, 1, 0, 1);
  }
  if (one_in(8)) {
   g->u.hit(g, bp_feet, 0, 0, 1);
   g->u.hit(g, bp_feet, 1, 0, 1);
  }
  if (one_in(6))
   g->u.hit(g, bp_torso, 0, 0, 1);
  if (one_in(8)) {
   g->u.hit(g, bp_arms, 0, 0, 1);
   g->u.hit(g, bp_arms, 1, 0, 1);
  }
 }
 if (g->lev.z >= 0) {
  for (int x = g->u.pos.x - SEEX * 2; x <= g->u.pos.x + SEEX * 2; x++) {
   for (int y = g->u.pos.y - SEEY * 2; y <= g->u.pos.y + SEEY * 2; y++) {
    if (!g->m.has_flag(diggable, x, y) && !g->m.has_flag(noitem, x, y) &&
        g->m.move_cost(x, y) > 0 && g->m.is_outside(x, y) && one_in(400))
     g->m.add_field(g, x, y, fd_acid, 1);
   }
  }
 }
 for (int i = 0; i < g->z.size(); i++) {
  if (g->m.is_outside(g->z[i].pos)) {
   if (!g->z[i].has_flag(MF_ACIDPROOF))
    g->z[i].hurt(1);
  }
 }
 very_wet(g);
}
