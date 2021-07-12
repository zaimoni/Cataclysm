#include "event.h"
#include "npc.h"
#include "game.h"
#include "line.h"
#include "rng.h"
#include "recent_msg.h"

#include <stdexcept>

static const char* const JSON_transcode_events[] = {
    "HELP",
    "WANTED",
    "ROBOT_ATTACK",
    "SPAWN_WYRMS",
    "AMIGARA",
    "ROOTS_DIE",
    "TEMPLE_OPEN",
    "TEMPLE_FLOOD",
    "TEMPLE_SPAWN",
    "DIM",
    "ARTIFACT_LIGHT"
};

DEFINE_JSON_ENUM_SUPPORT_TYPICAL(event_type, JSON_transcode_events)

void event::actualize() const
{
 auto g = game::active();

 switch (type) {

  case EVENT_HELP: {
   int num = (0 <= faction_id) ? rng(1, 6) : 1;
   faction* fac = (0 <= faction_id) ? faction::from_id(faction_id) : nullptr;
   if (0 <= faction_id && !fac) debugmsg("EVENT_HELP run with invalid faction_id");
   for (int i = 0; i < num; i++) {
    npc tmp;
    tmp.randomize_from_faction(g, fac);
    tmp.attitude = NPCATT_DEFEND;
	tmp.screenpos_set(g->u.pos.x - SEEX * 2 + rng(-5, 5), g->u.pos.y - SEEY * 2 + rng(-5, 5));
    g->active_npc.push_back(std::move(tmp));
   }
  } break;

  case EVENT_ROBOT_ATTACK: {
   if (rl_dist(g->lev.x, g->lev.y, map_point) <= 4) {
    const mtype *robot_type = mtype::types[mon_tripod];
    if (faction_id == 0) // The cops!
     robot_type = mtype::types[mon_copbot];
    int robx = (g->lev.x > map_point.x ? 0 - SEEX * 2 : SEEX * 4),
        roby = (g->lev.y > map_point.y ? 0 - SEEY * 2 : SEEY * 4);
    g->z.push_back(monster(robot_type, robx, roby));
   }
  } break;

  case EVENT_SPAWN_WYRMS: {
      if (g->lev.z >= 0) return;

      static constexpr const zaimoni::gdi::box<point> spawn_zone(point(0), point(SEE * MAPSIZE));
      static std::function<point()> candidate = [&]() { return g->u.pos + rng(spawn_zone); };

      static std::function<bool(const point&)> ok = [&](const point& pt) {
          return g->is_empty(pt) && 2 < rl_dist(pt, g->u.pos);
      };

      int num_wyrms = rng(1, 4);
      for (int i = 0; i < num_wyrms; i++) {
          if (auto pt = LasVegasChoice(10, candidate, ok)) {
              g->z.push_back(monster(mtype::types[mon_dark_wyrm], *pt));
          }
      }
      if (!one_in(25)) // They just keep coming!
          g->add_event(EVENT_SPAWN_WYRMS, int(messages.turn) + rng(15, 25));
  } break;

  case EVENT_AMIGARA: {
   const auto fault = find_first(map::reality_bubble_extent, [&](point pt) { return t_fault == g->m.ter(pt);});
   if (!fault) {
       debuglog("Amigara event without faultline");
#ifndef NDEBUG
       throw std::logic_error("Amigara event without faultline");
#else
       debugmsg("Amigara event without faultline");
       return;
#endif
   }
   const bool horizontal = (t_fault == g->m.ter(*fault + Direction::W) || t_fault == g->m.ter(*fault + Direction::E));
   // ? : of two lambda functions hard-errors (incompatible types)
   auto amigara_position = horizontal ? std::function<point()>([&]() {
       point ret(*fault);
       ret.x += rng(0, 16); // C:Whales 2*SEE-8
       for (int n = -1; n <= 1; n++) {
           const int try_y = fault->y + n;
           if (t_rock_floor == g->m.ter(ret.x, try_y)) ret.y = try_y;
       }
       return ret;
   }) : [&]() { // vertical fault
       point ret(*fault);
       ret.y += rng(0, 16); // C:Whales 2*SEE-8
       for (int n = -1; n <= 1; n++) {
           const int try_x = fault->x + n;
           if (t_rock_floor == g->m.ter(try_x, ret.y)) ret.x = try_x;
       }
       return ret;
   };

   const int num_horrors = rng(3, 5);
   monster horror(mtype::types[mon_amigara_horror]);
   for (int i = 0; i < num_horrors; i++) {
       const auto mon = LasVegasChoice<point>(10, amigara_position, [&](const point& pt) { return t_rock_floor == g->m.ter(pt) && g->is_empty(pt); });
       if (mon) {
           horror.spawn(*mon);
           g->z.push_back(horror);
       }
   }
  } break;

  case EVENT_ROOTS_DIE:
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
	 auto& t = g->m.ter(x,y);
     if (t_root_wall == t && one_in(3)) t = t_underbrush;
    }
   }
   break;

  case EVENT_TEMPLE_OPEN: {
   bool saw_grate = false;
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
	 if (g->m.rewrite_test<t_grate, t_stairs_down>(x,y)) {
      if (!saw_grate && g->u_see(x, y)) saw_grate = true;
     }
    }
   }
   if (saw_grate) messages.add("The nearby grates open to reveal a staircase!");
  } break;

  case EVENT_TEMPLE_FLOOD: {
   bool flooded = false;
   map copy;
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++)
     copy.ter(x, y) = g->m.ter(x, y);
   }
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
     if (g->m.ter(x, y) == t_water_sh) {
      bool deepen = false;
      for (int wx = x - 1;  wx <= x + 1 && !deepen; wx++) {
       for (int wy = y - 1;  wy <= y + 1 && !deepen; wy++) {
        if (g->m.ter(wx, wy) == t_water_dp) deepen = true;
       }
      }
      if (deepen) {
       copy.ter(x, y) = t_water_dp;
       flooded = true;
      }
     } else if (g->m.ter(x, y) == t_rock_floor) {
      bool flood = false;
      for (int wx = x - 1;  wx <= x + 1 && !flood; wx++) {
       for (int wy = y - 1;  wy <= y + 1 && !flood; wy++) {
        if (g->m.ter(wx, wy) == t_water_dp || g->m.ter(wx, wy) == t_water_sh) flood = true;
       }
      }
      if (flood) {
       copy.ter(x, y) = t_water_sh;
       flooded = true;
      }
     }
    }
   }
   if (!flooded) return; // We finished flooding the entire chamber!
// Check if we should print a message
   if (copy.ter(g->u.pos) != g->m.ter(g->u.pos)) {
    if (copy.ter(g->u.pos) == t_water_sh)
     messages.add("Water quickly floods up to your knees.");
    else { // Must be deep water!
     messages.add("Water fills nearly to the ceiling!");
     g->u.swim(g->u.GPSpos);
    }
   }
// copy is filled with correct tiles; now copy them back to g->m
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++)
     g->m.ter(x, y) = copy.ter(x, y);
   }
   g->add_event(EVENT_TEMPLE_FLOOD, int(messages.turn) + rng(2, 3));
  } break;

  case EVENT_TEMPLE_SPAWN: {
      static constexpr const mon_id temple_spawn[] = { mon_sewer_snake, mon_centipede, mon_dermatik, mon_spider_widow };
      static constexpr const zaimoni::gdi::box<point> spawn_zone(point(-5), point(5));

      static std::function<point()> candidate = [&]() { return g->u.pos + rng(spawn_zone); };

      static std::function<bool(const point&)> ok = [&](const point& pt) {
          return g->is_empty(pt) && 2 < rl_dist(pt, g->u.pos);
      };

      if (auto pt = LasVegasChoice(20, candidate, ok)) {
          g->z.push_back(monster(mtype::types[temple_spawn[rng(0, (std::end(temple_spawn) - std::begin(temple_spawn)) - 1)]], *pt));
      }
  } break;

  default:
   break; // Nothing happens for other events
 }
}

bool event::per_turn()
{
 auto g = game::active();

 switch (type) {
  case EVENT_WANTED: {
   if (g->lev.z >= 0 && one_in(100)) { // About once every 10 minutes
    point place = g->m.random_outdoor_tile();
    if (place.x == -1 && place.y == -1) return true; // We're safely indoors!
    monster eyebot(mtype::types[mon_eyebot], place);
    eyebot.faction_id = faction_id;
    g->z.push_back(eyebot);
    if (g->u_see(place)) messages.add("An eyebot swoops down nearby!");
   }
  }
   return true;

  case EVENT_SPAWN_WYRMS:
   if (g->lev.z >= 0) {
    turn--;
    return true;
   }
   if (int(messages.turn) % 3 == 0) messages.add("You hear screeches from the rock above and around you!");
   return true;

  case EVENT_AMIGARA:
   messages.add("The entire cavern shakes!");
   return true;

  case EVENT_TEMPLE_OPEN:
   messages.add("The earth rumbles.");
   return true;

  default: return true; // Nothing happens for other events
 }
}
