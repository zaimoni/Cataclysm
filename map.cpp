#include "map.h"
#include "submap.h"
#include "output.h"
#include "rng.h"
#include "game.h"
#include "line.h"
#include "mapbuffer.h"
#include "posix_time.h"
#include "json.h"
#include "recent_msg.h"
#include "om_cache.hpp"
#include "stl_limits.h"
#include "inline_stack.hpp"
#include <fstream>
#include <iostream>
#include <cmath>
#include <stdlib.h>

map_extra map::_force_map_extra = mx_null;
point map::_force_map_extra_pos = point(-1, -1);

using namespace cataclysm;

#if NONINLINE_EXPLICIT_INSTANTIATION
template<> ter_id discard<ter_id>::x = t_null;
template<> trap_id discard<trap_id>::x = tr_null;
template<> field discard<field>::x = field();
template<> std::vector<item> discard<std::vector<item> >::x(0);
#endif

enum astar_list {
 ASL_NONE,
 ASL_OPEN,
 ASL_CLOSED
};

std::optional<reality_bubble_loc> map::to(const point& pt) const
{
    if (!inbounds(pt.x, pt.y)) return std::nullopt;
    return reality_bubble_loc((pt.x / SEE) + (pt.y / SEE) * my_MAPSIZE, pt % SEE);
}

std::optional<reality_bubble_loc> map::to(const GPS_loc& pt)
{
    return game::active()->toSubmap(pt);
}

submap* map::chunk(const GPS_loc& pt)
{
    if (const auto pos = to(pt)) return grid[pos->first]; // check internal cache first
    return MAPBUFFER.lookup_submap(pt.first);
}

bool map::find_stairs(const point& pt, const int movez, point& pos) const
{
    int best = 999;
    bool ok = false;
    for (int i = pt.x - SEEX * 2; i <= pt.x + SEEX * 2; i++) {
        for (int j = pt.y - SEEY * 2; j <= pt.y + SEEY * 2; j++) {
            const int dist = rl_dist(pt, i, j);
            if (dist <= best &&
                ((movez == -1 && has_flag(goes_up, i, j)) ||
                (movez == 1 && (has_flag(goes_down, i, j) || ter(i, j) == t_manhole_cover)))) {
                pos = point(i, j);
                best = dist;
                ok = true;
            }
        }
    }
    return ok;
}

bool map::find_terrain(const point& pt, const ter_id dest, point& pos) const
{
    int best = 999;
    bool ok = false;
    for (int x = 0; x < SEEX * MAPSIZE; x++) {
        for (int y = 0; y < SEEY * MAPSIZE; y++) {
            const int dist = rl_dist(pt, x, y);
            if (dist <= best && dest == ter(x, y)) {
                pos = point(x, y);
                ok = true;
                best = dist;
            }
        }
    }
    return ok;
}

std::vector<point> map::grep(const point& tl, const point& br, std::function<bool(point)> test)
{
    std::vector<point> ret;
    if (tl.x <= br.x && tl.y <= br.y) {
        point pt;
        for (pt.x = tl.x; pt.x <= br.x; pt.x++) {
            for (pt.y = tl.y; pt.y <= br.y; pt.y++) {
                if (test(pt)) ret.push_back(pt);
            }
        }
    }
    return ret;
}

std::optional<std::pair<const vehicle*, int>> map::veh_at(const reality_bubble_loc& src) const
{
    // must check 3x3 map chunks, as vehicle part may span to neighbour chunk
    // we presume that vehicles don't intersect (they shouldn't by any means)
    const auto nonant_ub = my_MAPSIZE * my_MAPSIZE;
    for (int mx = -1; mx <= 1; mx++) {
        for (int my = -1; my <= 1; my++) {
            int nonant1 = src.first + mx + my * my_MAPSIZE;
            if (nonant1 < 0 || nonant1 >= nonant_ub) continue; // out of grid
            for (auto& veh : grid[nonant1]->vehicles) { // profiler likes this; burns less CPU than testing for empty std::vector
                const auto veh_loc = veh.bubble_pos();
                assert(veh_loc);
                assert(veh_loc->first == nonant1);
                int part = veh.part_at(src.second.x - (veh_loc->second.x + mx * SEEX), src.second.y - (veh_loc->second.y + my * SEEY));
                if (0 <= part) return std::pair(&veh, part);
            }
        }
    }
    return std::nullopt;
}

std::optional<std::pair<vehicle*, int>> map::veh_at(const reality_bubble_loc& src)
{
    // must check 3x3 map chunks, as vehicle part may span to neighbour chunk
    // we presume that vehicles don't intersect (they shouldn't by any means)
    const auto nonant_ub = my_MAPSIZE * my_MAPSIZE;
    for (int mx = -1; mx <= 1; mx++) {
        for (int my = -1; my <= 1; my++) {
            int nonant1 = src.first + mx + my * my_MAPSIZE;
            if (nonant1 < 0 || nonant1 >= nonant_ub) continue; // out of grid
            for (auto& veh : grid[nonant1]->vehicles) { // profiler likes this; burns less CPU than testing for empty std::vector
                const auto veh_loc = veh.bubble_pos();
                assert(veh_loc);
                assert(veh_loc->first == nonant1);
                int part = veh.part_at(src.second.x - (veh_loc->second.x + mx * SEEX), src.second.y - (veh_loc->second.y + my * SEEY));
                if (0 <= part) return std::pair(&veh, part);
            }
        }
    }
    return std::nullopt;
}

// \todo if map::veh_at goes dead code then relocate (overmap.cpp? new GPS_loc.cpp?)
std::optional<std::pair<vehicle*, int>> GPS_loc::veh_at() const
{
    // must check 3x3 map chunks, as vehicle part may span to neighbour chunk
    // we presume that vehicles don't intersect (they shouldn't by any means)
    submap* const local_map[3][3] = { {MAPBUFFER.lookup_submap(first + Direction::NW), MAPBUFFER.lookup_submap(first + Direction::N), MAPBUFFER.lookup_submap(first + Direction::NE)},
        {MAPBUFFER.lookup_submap(first + Direction::W), MAPBUFFER.lookup_submap(first), MAPBUFFER.lookup_submap(first + Direction::E)},
        {MAPBUFFER.lookup_submap(first + Direction::SW), MAPBUFFER.lookup_submap(first + Direction::S), MAPBUFFER.lookup_submap(first + Direction::SE)} };

    for (int mx = 0; mx <= 2; mx++) {
        for (int my = 0; my <= 2; my++) {
            if (!local_map[mx][my]) continue;   // submap not generated yet
            for (decltype(auto) veh : local_map[mx][my]->vehicles) {
                const auto delta = *this - veh.GPSpos;
                if (const point* const pt = std::get_if<point>(&delta)) { // gross invariant failure: vehicles should have GPSpos tripoint of their submap
                    int part = veh.part_at(pt->x, pt->y);
                    if (part >= 0) return std::pair(&veh, part);
                }
            }
        }
    }

    return std::nullopt;
}

std::optional<std::pair<const vehicle*, int>> map::_veh_at(const point& src) const
{
    if (auto pos = to(src)) return veh_at(*pos);
    return std::nullopt;
}

std::optional<std::pair<vehicle*, int>> map::_veh_at(const point& src)
{
    if (auto pos = to(src)) return veh_at(*pos);
    return std::nullopt;
}

vehicle* map::veh_near(const point& pt)
{
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (auto veh = _veh_at(pt.x + dx, pt.y + dy)) return veh->first;
        }
    }
    return nullptr;
}

std::optional<std::vector<std::pair<point, vehicle*> > > map::all_veh_near(const point& pt)
{
    std::vector<std::pair<point, vehicle*> > ret;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            const point pos(pt.x + dx, pt.y + dy);
            if (auto veh = _veh_at(pos)) ret.push_back({ pos, veh->first });
        }
    }
    if (!ret.empty()) return ret;
    return std::nullopt;
}

bool map::try_board_vehicle(game* g, int x, int y, player& p)
{
    // p.in_vehicle theoretically could be true, it just usually isn't because boarding a vehicle from another vehicle isn't easy to set up
    const bool is_current_player = &p == &g->u;

    const auto v = _veh_at(x, y);
    if (!v) return false;
    vehicle* const veh = v->first; // backward compatibility
    const int seat_part = veh->part_with_feature(v->second, vpf_seat);
    if (0 > seat_part) return false;
    if (veh->parts[seat_part].passenger) {
        if (player* psg = veh->get_passenger(seat_part)) {
            if (is_current_player) messages.add("Not boarding %s; %s already there.", veh->name.c_str(), psg->name.c_str());
            return false;
        }
        veh->parts[seat_part].passenger = false; // invariant violation.  Enforced at map::displace_vehicle
    }

    if (is_current_player && !query_yn("Board vehicle?")) return false; // XXX \todo what if a PC, but not PC's turn (multi-PC case)

    veh->parts[seat_part].passenger = 1;
    p.screenpos_set(point(x, y));   // \todo this is where vehicle-vehicle transfer would trigger unboarding the old vehicle
    p.in_vehicle = true;
    if (is_current_player && game::update_map_would_scroll(point(x, y)))
        g->update_map(x, y);
    return true;
}

bool map::displace_vehicle(vehicle* const veh, const point& delta, bool test)
{
 point src = veh->screen_pos().value();
 const point dest = src + delta;

 if (!inbounds(src.x, src.y)) { // possibly dead code
  debuglog("map::displace_vehicle: coords out of bounds %d,%d->%d,%d", src.x, src.y, dest.x, dest.y);
  return false;
 }

 int src_na = int(src.x / SEEX) + int(src.y / SEEY) * my_MAPSIZE;
 src %= SEE;

 int dst_na = int(dest.x / SEEX) + int(dest.y / SEEY) * my_MAPSIZE;

 if (test) return src_na != dst_na;

 // first, let's find our position in current vehicles vector
 int our_i = -1;
 for (int i = 0; i < grid[src_na]->vehicles.size(); i++) {
     if (veh == &(grid[src_na]->vehicles[i])) {
          our_i = i;
          break;
     }
 }
 if (our_i < 0) {
  debuglog("displace_vehicle our_i=%d", our_i);
  return false;
 }
 // move the vehicle
 // don't let it go off grid
 if (!inbounds(dest.x, dest.y)) veh->stop();

 int rec = abs(veh->velocity) / 5 / 100;

 // record every passenger inside
 auto passengers = veh->passengers();

 bool need_update = false;
 point upd;
 const auto g = game::active();

 // move passengers
 for (const auto& pass : passengers) {
  assert(pass.second);
  player *psg = pass.second;
  int p = pass.first;
  int trec = rec - psg->sklevel[sk_driving];	// dead compensation \todo how does C:DDA handle this?
  if (trec < 0) trec = 0;
  // add recoil
  psg->driving_recoil = rec;
  // displace passenger taking in account vehicle movement (dx, dy)
  // and turning: precalc_dx/dy [0] contains previous frame direction,
  // and precalc_dx/dy[1] should contain next direction
  psg->screenpos_add(delta + veh->parts[p].precalc_d[1] - veh->parts[p].precalc_d[0]);
  if (psg == &g->u) { // if passemger is you, we need to update the map
   need_update = true;
   upd = psg->pos;
  }
 }
 for (auto& part : veh->parts) part.precalc_d[0] = part.precalc_d[1];

 veh->GPSpos = g->toGPS(dest);
 if (src_na != dst_na) {
  grid[dst_na]->vehicles.push_back(std::move(*veh));
  EraseAt(grid[src_na]->vehicles, our_i);
 }

 bool was_update = false;
 if (need_update && game::update_map_would_scroll(upd)) {
  assert(MAPSIZE == my_MAPSIZE);	// critical fail if this doesn't hold, but map doesn't provide an accessor for testing this
// map will shift
  g->update_map(upd.x, upd.y);	// player coords were already updated so don't re-update them
  was_update = true;
 }
 return (src_na != dst_na) || was_update;
}

void map::vehmove(game *g)
{
 static int sm_stack[MAPSIZE*MAPSIZE];  // requires single-threaded or locking to be safe
 size_t ub = 0;

 // give vehicles movement points
 for (int i = 0; i < my_MAPSIZE; i++) {
     for (int j = 0; j < my_MAPSIZE; j++) {
         int sm = i + j * my_MAPSIZE;
         if (!grid[sm]->vehicles.empty()) {
             sm_stack[ub++] = sm;
             for (auto& veh : grid[sm]->vehicles) veh.gain_moves(abs(veh.velocity)); // velocity is ability to make more one-tile steps per turn
         }
     }
 }
 if (0 >= ub) return;
// move vehicles
 bool sm_change;
 int count = 0;
 do {
  sm_change = false;
  for (int scan = 0; scan < ub; scan++) {
      const int sm = sm_stack[scan];
      const int i = sm % my_MAPSIZE;
      const int j = sm / my_MAPSIZE;

    for (int v = 0; v < grid[sm]->vehicles.size(); v++) {
     vehicle *veh = &(grid[sm]->vehicles[v]);
     bool pl_ctrl = veh->player_in_control(g->u);
     while (!sm_change && veh->moves > 0 && veh->velocity != 0) {
      const int mv_cost_terrain = grid[sm]->move_cost_ter_only(veh->GPSpos.second);
      if (grid[sm]->has_flag_ter_only<swimmable>(veh->GPSpos.second) && 0 == mv_cost_terrain) { // deep water
       if (pl_ctrl) messages.add("Your %s sank.", veh->name.c_str());
       veh->unboard_all ();
// destroy vehicle (sank to nowhere)
       EraseAt(grid[sm]->vehicles, v);
       v--;
       break;
      }
// one-tile step take some of movement
      veh->moves -= (5 * mobile::mp_turn) * mv_cost_terrain;

      auto pt = veh->screen_pos();
      assert(pt);
      assert(pt->x == i * SEEX + veh->GPSpos.second.x);
      assert(pt->y == j * SEEY + veh->GPSpos.second.y);

      if (!veh->valid_wheel_config()) { // not enough wheels
       veh->velocity += veh->velocity < 0 ? 20 * vehicle::mph_1 : -20 * vehicle::mph_1;
       for (int ep = 0; ep < veh->external_parts.size(); ep++) {
        int p = veh->external_parts[ep];
		const point origin(*pt + veh->parts[p].precalc_d[0]);
        ter_id &pter = ter(origin);
        if (pter == t_dirt || pter == t_grass) pter = t_dirtmound;
       }
      } // !veh->valid_wheel_config()

      if (veh->skidding && one_in(4)) // might turn uncontrollably while skidding
       veh->move.init (veh->move.dir() + (one_in(2) ? -15 * rng(1, 3) : 15 * rng(1, 3)));
      else if (pl_ctrl && rng(0, 4) > g->u.sklevel[sk_driving] && one_in(20)) {
       messages.add("You fumble with the %s's controls.", veh->name.c_str());
       veh->turn (one_in(2) ? -15 : 15);
      }
 // eventually send it skidding if no control
      if (!veh->any_boarded_parts() && one_in (10))	// XXX \todo should be out of control? check C:DDA
       veh->skidding = true;
      tileray mdir(veh->physical_facing());
      mdir.advance (veh->velocity < 0? -1 : 1);
      const point delta(mdir.dx(), mdir.dy()); // where do we go
      bool can_move = true;
// calculate parts' mount points @ next turn (put them into precalc[1])
      veh->precalc_mounts(1, veh->skidding ? veh->turn_dir : mdir.dir());

      int imp = 0;
// find collisions
      for (int ep = 0; ep < veh->external_parts.size(); ep++) {
       int p = veh->external_parts[ep];
// coords of where part will go due to movement (dx/dy)
// and turning (precalc_dx/dy [1])
	   const point ds(*pt + delta + veh->parts[p].precalc_d[1]);
       if (can_move) imp += veh->part_collision(pt->x, pt->y, p, ds);
       if (veh->velocity == 0) can_move = false;
       if (!can_move) break;
      }

      int coll_turn = 0;
      if (imp > 0) {
       if (imp > 100) veh->damage_all(imp / 20, imp / 10, vehicle::damage_type::bash);// shake veh because of collision
	   int vel2 = imp * vehicle::k_mvel / (veh->total_mass() / 8);
	   auto passengers = veh->passengers();
	   for (const auto& pass : passengers) {
		   assert(pass.second);
		   int ps = pass.first;
		   player* psg = pass.second;
		   int throw_roll = rng(vel2 / 100, vel2 / 100 * 2);
		   int psblt = veh->part_with_feature(ps, vpf_seatbelt);
		   int sb_bonus = psblt >= 0 ? veh->part_info(psblt).bonus : 0;
		   bool throw_it = throw_roll > (psg->str_cur + sb_bonus) * 3;
		   std::string psgname;
		   const char* psgverb;
		   if (psg == &g->u) {
			   psgname = "You";
			   psgverb = "were";
		   } else {
			   psgname = psg->name;
			   psgverb = "was";
		   }
		   if (throw_it) {
			   if (psgname.length())
				   messages.add("%s %s hurled from the %s's seat by the power of impact!",
					   psgname.c_str(), psgverb, veh->name.c_str());
               veh->unboard(ps);
			   g->fling_player_or_monster(psg, nullptr, mdir.dir() + rng(0, 60) - 30,
				   (vel2 / 100 - sb_bonus < 10 ? 10 : vel2 / 100 - sb_bonus));
		   } else if (veh->part_with_feature(ps, vpf_controls) >= 0) {
			   int lose_ctrl_roll = rng(0, imp);
			   if (lose_ctrl_roll > psg->dex_cur * 2 + psg->sklevel[sk_driving] * 3) {
				   if (psgname.length())
					   messages.add("%s lose%s control of the %s.", psgname.c_str(),
					   (psg == &g->u ? "" : "s"), veh->name.c_str());
				   int turn_amount = (rng(1, 3) * sqrt(vel2) / 2) / 15;
				   if (turn_amount < 1) turn_amount = 1;
				   turn_amount *= 15;
				   if (turn_amount > 120) turn_amount = 120;
				   //veh->skidding = true;
				   //veh->turn (one_in (2)? turn_amount : -turn_amount);
				   coll_turn = one_in(2) ? turn_amount : -turn_amount;
			   }
		   }
	   }
      }
// now we're gonna handle traps we're standing on (if we're still moving).
// this is done here before displacement because
// after displacement veh reference would be invalid.
// damn references!
      if (can_move) {
       for (int ep = 0; ep < veh->external_parts.size(); ep++) {
        int p = veh->external_parts[ep];
		const point origin(*pt + veh->parts[p].precalc_d[0]);
        if (veh->part_flag(p, vpf_wheel) && one_in(2))
         if (displace_water(origin) && pl_ctrl)
          messages.add("You hear a splash!");
        veh->handle_trap(origin, p);
       }
      }

      if (veh->last_turn < 0) veh->last_turn += 1;
      else if (veh->last_turn > 0) veh->last_turn -= 1;

      int slowdown = veh->skidding? 2*vehicle::mph_1 : 2 * vehicle::mph_1 / 10; // mph lost per tile when coasting
      float kslw = (0.1 + veh->k_dynamics()) / ((0.1) + veh->k_mass());
      slowdown = (int) (slowdown * kslw);
      if (veh->velocity < 0)
       veh->velocity += slowdown;
      else
       veh->velocity -= slowdown;
      if (abs(veh->velocity) < vehicle::mph_1) veh->stop();

      if (pl_ctrl) {
       timespec ts = { 0, 50000000 };   // Timespec for the animation
       nanosleep(&ts, nullptr);
      }

      if (can_move) {
       veh->physical_facing(mdir); // accept new direction
       if (coll_turn) {
        veh->skidding = true;
        veh->turn (coll_turn);
       }
// accept new position
// if submap changed, we need to process grid from the beginning.
       sm_change = displace_vehicle(veh, delta);
      } else // can_move
       veh->stop();
// redraw scene
      g->draw();
      if (sm_change) break;
     } // while (veh->moves
     if (sm_change) break;
    } //for v
    if (sm_change) break;
  } // for scan
  count++;
  if (10 < ++count) break;  // infinite loop interceptor
 } while (sm_change);
}

bool map::displace_water(const point& pt)
{
    if (0 < move_cost_ter_only(pt.x, pt.y) && has_flag(swimmable, pt)) // shallow water
    { // displace it
        inline_stack<point, std::end(Direction::vector) - std::begin(Direction::vector)> can_displace_to;

        for (decltype(auto) dir : Direction::vector) {
            auto dest = pt + dir;
            if (0 == move_cost_ter_only(dest.x, dest.y)) continue;
            ter_id ter0 = ter(dest);
            if (ter0 == t_water_sh || ter0 == t_water_dp) continue;
            // C:Z water is not acid.  Until we can record the terrain under shallow water, disallow acidic displacement
            if (ter0 != t_dirt) continue;
            can_displace_to.push(dest);
        }

        if (auto ub = can_displace_to.size()) {
            ter(can_displace_to[rng(0, ub-1)]) = t_water_sh;
            ter(pt) = t_dirt;
            return true;
        }

    }
    return false;
}

ter_id& GPS_loc::ter()
{
    if (submap* const sm = game::active()->m.chunk(*this)) return sm->ter[second.x][second.y];
    return (discard<ter_id>::x = t_null); // Out-of-bounds - null terrain
}

ter_id GPS_loc::ter() const
{
    if (const submap* const sm = game::active()->m.chunk(*this)) return sm->ter[second.x][second.y];
    return t_null; // Out-of-bounds - null terrain
}

ter_id& map::ter(int x, int y)
{
    if (const auto pos = to(x, y)) return grid[pos->first]->ter[pos->second.x][pos->second.y];
    return (discard<ter_id>::x = t_null); // Out-of-bounds - null terrain
}

void map::_translate(ter_id from, ter_id to)
{
	for (int x = 0; x < SEEX * my_MAPSIZE; x++) {
		for (int y = 0; y < SEEY * my_MAPSIZE; y++) {
			auto& t = ter(x, y);
			if (from == t) t = to;
		}
	}
}

std::string map::features(const point& pt) const
{
// This is used in an info window that is 46 characters wide, and is expected
// to take up one line.  So, make sure it does that.
 std::string ret;
 if (has_flag(bashable, pt))
  ret += "Smashable. ";	// 11 chars (running total)
 if (has_flag(diggable, pt))
  ret += "Diggable. ";	// 21 chars
 if (has_flag(rough, pt))
  ret += "Rough. ";	// 28 chars
 if (has_flag(sharp, pt))
  ret += "Sharp. ";	// 35 chars
 return ret;
}

int map::move_cost(const reality_bubble_loc& pos) const
{
 if (const auto v = veh_at(pos)) {
     const vehicle* const veh = v->first; // backward compatibility
     int dpart = veh->part_with_feature(v->second, vpf_obstacle);
     if (dpart >= 0 &&
         (!veh->part_flag(dpart, vpf_openable) || !veh->parts[dpart].open))
         return 0;
     else
         return 8;
 }

 return ter_t::list[ter(pos)].movecost;
}

int map::move_cost(int x, int y) const
{
 if (const auto v = _veh_at(x, y)) {
     const vehicle* const veh = v->first;
     int dpart = veh->part_with_feature(v->second, vpf_obstacle);
     if (dpart >= 0 &&
         (!veh->part_flag(dpart, vpf_openable) || !veh->parts[dpart].open))
         return 0;
     else
         return 8;
 }
 return ter_t::list[ter(x, y)].movecost;
}

int GPS_loc::move_cost() const
{
    if (const auto v = veh_at()) {
        const vehicle* const veh = v->first;
        int dpart = veh->part_with_feature(v->second, vpf_obstacle);
        if (dpart >= 0 &&
            (!veh->part_flag(dpart, vpf_openable) || !veh->parts[dpart].open))
            return 0;
        else
            return 8;
    }
    return ter_t::list[ter()].movecost;
}


int map::move_cost_ter_only(int x, int y) const
{
 return ter_t::list[ter(x, y)].movecost;
}

bool GPS_loc::is_transparent() const
{
    // Control statement is a problem. Normally returning false on an out-of-bounds
    // is how we stop rays from going on forever.  Instead we'll have to include
    // this check in the ray loop.
    bool tertr;
    if (const auto v = veh_at()) {
        const vehicle* const veh = v->first; // backward compatibility
        tertr = !veh->part_flag(v->second, vpf_opaque) || veh->parts[v->second].hp <= 0;
        if (!tertr) {
            int dpart = veh->part_with_feature(v->second, vpf_openable);
            if (dpart >= 0 && veh->parts[dpart].open)
                tertr = true; // open opaque door
        }
    }
    else
        tertr = is<transparent>(ter());
    const auto& fd = field_at();
    return tertr && (fd.type == 0 || field::list[fd.type].transparent[fd.density - 1]);	// Fields may obscure the view, too
}

bool map::trans(const reality_bubble_loc& pos) const
{
    // Control statement is a problem. Normally returning false on an out-of-bounds
    // is how we stop rays from going on forever.  Instead we'll have to include
    // this check in the ray loop.
    bool tertr;
    if (const auto v = veh_at(pos)) {
        const vehicle* const veh = v->first; // backward compatibility
        tertr = !veh->part_flag(v->second, vpf_opaque) || veh->parts[v->second].hp <= 0;
        if (!tertr) {
            int dpart = veh->part_with_feature(v->second, vpf_openable);
            if (dpart >= 0 && veh->parts[dpart].open)
                tertr = true; // open opaque door
        }
    }
    else
        tertr = is<transparent>(ter(pos));
    const auto& fd = grid[pos.first]->field_at(pos.second);
    return tertr && (fd.type == 0 || field::list[fd.type].transparent[fd.density - 1]);	// Fields may obscure the view, too
}

bool map::trans(const point& pt) const
{
 const auto pos = to(pt);
 return pos ? trans(*pos) : true;
}

bool map::has_flag(t_flag flag, const reality_bubble_loc& pos) const
{
    if (flag == bashable) {
        if (const auto v = veh_at(pos)) {
            const vehicle* const veh = v->first; // backward compatibility
            if (veh->parts[v->second].hp > 0 && // if there's a vehicle part here...
                veh->part_with_feature(v->second, vpf_obstacle) >= 0) {// & it is obstacle...
                int p = veh->part_with_feature(v->second, vpf_openable);
                if (p < 0 || !veh->parts[p].open) // and not open door
                    return true;
            }
        }
    }
    return ter_t::list[ter(pos)].flags & mfb(flag);
}

bool map::has_flag(t_flag flag, int x, int y) const
{
 if (flag == bashable) {
     if (const auto v = _veh_at(x, y)) {
         const vehicle* const veh = v->first; // backward compatibility
         const int vpart = v->second;
         if (veh->parts[vpart].hp > 0 && // if there's a vehicle part here...
             veh->part_with_feature(vpart, vpf_obstacle) >= 0) {// & it is obstacle...
             int p = veh->part_with_feature(vpart, vpf_openable);
             if (p < 0 || !veh->parts[p].open) // and not open door
                 return true;
         }
     }
 }
 return ter_t::list[ter(x, y)].flags & mfb(flag);
}

bool map::is_destructable(int x, int y) const
{
 return (has_flag(bashable, x, y) ||
         (move_cost(x, y) == 0 && !has_flag(liquid, x, y)));
}

bool map::is_outside(int x, int y) const
{
 // with proper z-levels, we would say "outside is when there are no floors above us, and arguably properly enclosed by walls/doors/etc.
 if (any<t_floor, t_floor_wax>(ter(x, y))) return false;
 for(int dir = direction::NORTH; dir <= direction::NORTHWEST; dir++) {
     point pt = point(x, y) + direction_vector((direction)dir);
     if (any<t_floor, t_floor_wax>(ter(pt))) return false;
 }
 if (const auto veh = _veh_at(x, y)) {
     if (veh->first->is_inside(veh->second)) return false;
 }
 return true;
}

// \todo if map::is_outside goes dead code then relocate (overmap.cpp? new GPS_loc.cpp?)
bool GPS_loc::is_outside() const
{
    // With proper z-levels, we would say "outside is when there are no floors above us, and arguably properly enclosed by walls/doors/etc.".
    if (0 > first.z) return false;
    if (const submap* const sm = MAPBUFFER.lookup_submap(first)) {
        if (any<t_floor, t_floor_wax>(sm->ter[second.x][second.y])) return false;
        for (decltype(auto) delta : Direction::vector) {
            const GPS_loc loc(*this + delta);
            if (loc.first == first) {
                if (any<t_floor, t_floor_wax>(sm->ter[loc.second.x][loc.second.y])) return false;
            } else if (const submap* const sm2 = MAPBUFFER.lookup_submap(loc.first)) {
                if (any<t_floor, t_floor_wax>(sm2->ter[loc.second.x][loc.second.y])) return false;
            }
            // Just discard the test if the submap wasn't generated yet.  We'll still have some context.
        }
        if (const auto veh = veh_at()) {
            if (veh->first->is_inside(veh->second)) return false;
        }
        return true; // No guessing, we're outside.
    }
    return true;    // If we haven't generated the submap yet, just assume we're outside.
    // \todo This could be improved by using overmap terrain, e.g. houses are likely inside.
}

bool map::bash(int x, int y, int str, int *res)
{
	std::string discard;
	return bash(x, y, str, discard, res);
}

bool map::bash(int x, int y, int str, std::string &sound, int *res)
{
 sound = "";
 bool smashed_web = false;
 if (field_at(x, y).type == fd_web) {
  smashed_web = true;
  remove_field(x, y);
 }

 {
 auto& stack = i_at(x, y);
 for (int i = 0; i < stack.size(); i++) {	// Destroy glass items (maybe)
  item& it = stack[i];
  if (it.made_of(GLASS) && one_in(2)) {
   if (sound == "")
    sound = "A " + it.tname() + " shatters!  ";
   else
    sound = "Some items shatter!  ";
   for(decltype(auto) obj : it.contents) stack.push_back(obj);  // count-forward loop enables chain breakage
   EraseAt(stack, i--); // inline i_rem(x, y, i); i--;
  }
 }
 }

 int result = -1;
 if (const auto veh = _veh_at(x, y)) {
  veh->first->damage(veh->second, str);
  result = str;
  sound += "crash!";
  return true;
 }

 switch (ter(x, y)) {
 case t_wall_wood:
  result = rng(0, 120);
  if (res) *res = result;
  if (str >= result && str >= rng(0, 120)) {
   sound += "crunch!";
   ter(x, y) = t_wall_wood_chipped;
   const int num_boards = rng(0, 2);
   for (int i = 0; i < num_boards; i++) add_item(x, y, item::types[itm_2x4], 0);
   return true;
  } else {
   sound += "whump!";
   return true;
  }
  break;

 case t_wall_wood_chipped:
  result = rng(0, 100);
  if (res) *res = result;
  if (str >= result && str >= rng(0, 100)) {
   sound += "crunch!";
   ter(x, y) = t_wall_wood_broken;
   const int num_boards = rng(3, 8);
   for (int i = 0; i < num_boards; i++) add_item(x, y, item::types[itm_2x4], 0);
   return true;
  } else {
   sound += "whump!";
   return true;
  }
  break;

 case t_wall_wood_broken:
  result = rng(0, 80);
  if (res) *res = result;
  if (str >= result && str >= rng(0, 80)) {
   sound += "crash!";
   ter(x, y) = t_dirt;
   const int num_boards = rng(4, 10);
   for (int i = 0; i < num_boards; i++) add_item(x, y, item::types[itm_2x4], 0);
   return true;
  } else {
   sound += "whump!";
   return true;
  }
  break;

 case t_door_c:
 case t_door_locked:
 case t_door_locked_alarm:
  result = rng(0, 40);
  if (res) *res = result;
  if (str >= result) {
   sound += "smash!";
   ter(x, y) = t_door_b;
   return true;
  } else {
   sound += "whump!";
   return true;
  }
  break;

 case t_door_b:
  result = rng(0, 30);
  if (res) *res = result;
  if (str >= result) {
   sound += "crash!";
   ter(x, y) = t_door_frame;
   const int num_boards = rng(2, 6);
   for (int i = 0; i < num_boards; i++) add_item(x, y, item::types[itm_2x4], 0);
   return true;
  } else {
   sound += "wham!";
   return true;
  }
  break;

 case t_window:
 case t_window_alarm:
  result = rng(0, 6);
  if (res) *res = result;
  if (str >= result) {
   sound += "glass breaking!";
   ter(x, y) = t_window_frame;
   return true;
  } else {
   sound += "whack!";
   return true;
  }
  break;

 case t_door_boarded:
  result = dice(3, 50);
  if (res) *res = result;
  if (str >= result) {
   sound += "crash!";
   ter(x, y) = t_door_frame;
   const int num_boards = rng(0, 2);
   for (int i = 0; i < num_boards; i++) add_item(x, y, item::types[itm_2x4], 0);
   return true;
  } else {
   sound += "wham!";
   return true;
  }
  break;

 case t_window_boarded:
  result = dice(3, 30);
  if (res) *res = result;
  if (str >= result) {
   sound += "crash!";
   ter(x, y) = t_window_frame;
   const int num_boards = rng(0, 2) * rng(0, 1);
   for (int i = 0; i < num_boards; i++) add_item(x, y, item::types[itm_2x4], 0);
   return true;
  } else {
   sound += "wham!";
   return true;
  }
  break;

 case t_paper:
  result = dice(2, 6) - 2;
  if (res) *res = result;
  if (str >= result) {
   sound += "rrrrip!";
   ter(x, y) = t_dirt;
   return true;
  } else {
   sound += "slap!";
   return true;
  }
  break;

 case t_toilet:
  result = dice(8, 4) - 8;
  if (res) *res = result;
  if (str >= result) {
   sound += "porcelain breaking!";
   ter(x, y) = t_rubble;
   return true;
  } else {
   sound += "whunk!";
   return true;
  }
  break;

 case t_dresser:
 case t_bookcase:
  result = dice(3, 45);
  if (res) *res = result;
  if (str >= result) {
   sound += "smash!";
   ter(x, y) = t_floor;
   const int num_boards = rng(4, 12);
   for (int i = 0; i < num_boards; i++) add_item(x, y, item::types[itm_2x4], 0);
   return true;
  } else {
   sound += "whump.";
   return true;
  }
  break;

 case t_wall_glass_h:
 case t_wall_glass_v:
 case t_wall_glass_h_alarm:
 case t_wall_glass_v_alarm:
 case t_door_glass_c:
  result = rng(0, 20);
  if (res) *res = result;
  if (str >= result) {
   sound += "glass breaking!";
   ter(x, y) = t_floor;
   return true;
  } else {
   sound += "whack!";
   return true;
  }
  break;

 case t_reinforced_glass_h:
 case t_reinforced_glass_v:
  result = rng(60, 100);
  if (res) *res = result;
  if (str >= result) {
   sound += "glass breaking!";
   ter(x, y) = t_floor;
   return true;
  } else {
   sound += "whack!";
   return true;
  }
  break;

 case t_tree_young:
  result = rng(0, 50);
  if (res) *res = result;
  if (str >= result) {
   sound += "crunch!";
   ter(x, y) = t_underbrush;
   const int num_sticks = rng(0, 3);
   for (int i = 0; i < num_sticks; i++) add_item(x, y, item::types[itm_stick], 0);
   return true;
  } else {
   sound += "whack!";
   return true;
  }
  break;

 case t_underbrush:
  result = rng(0, 30);
  if (res) *res = result;
  if (str >= result && !one_in(4)) {
   sound += "crunch.";
   ter(x, y) = t_dirt;
   return true;
  } else {
   sound += "brush.";
   return true;
  }
  break;

 case t_shrub:
  if (str >= rng(0, 30) && str >= rng(0, 30) && str >= rng(0, 30) && one_in(2)){
   sound += "crunch.";
   ter(x, y) = t_underbrush;
   return true;
  } else {
   sound += "brush.";
   return true;
  }
  break;

 case t_marloss:
  result = rng(0, 40);
  if (res) *res = result;
  if (str >= result) {
   sound += "crunch!";
   ter(x, y) = t_fungus;
   return true;
  } else {
   sound += "whack!";
   return true;
  }
  break;

 case t_vat:
  result = dice(2, 20);
  if (res) *res = result;
  if (str >= result) {
   sound += "ker-rash!";
   ter(x, y) = t_floor;
   return true;
  } else {
   sound += "plunk.";
   return true;
  }
 case t_crate_c:
 case t_crate_o:
  result = dice(4, 20);
  if (res) *res = result;
  if (str >= result) {
   sound += "smash";
   ter(x, y) = t_dirt;
   const int num_boards = rng(1, 5);
   for (int i = 0; i < num_boards; i++) add_item(x, y, item::types[itm_2x4], 0);
   return true;
  } else {
   sound += "wham!";
   return true;
  }
 }
 if (res) *res = result;
 if (move_cost(x, y) == 0) {
  sound += "thump!";
  return true;
 }
 return smashed_web;// If we kick empty space, the action is canceled
}

// creatures call map::destroy only if the terrain is NOT bashable.  Map generation usually pre-emptively bashes.
void map::destroy(game *g, int x, int y, bool makesound)
{
 auto& terrain = ter(x, y);

 // contrary to what one would expect, vehicle destruction not directly processed here
 if (!is_destructible(terrain)) return;

 // \todo V0.3.0 bash bashable terrain (seems to give more detailed results)?

 switch(terrain) {
 case t_gas_pump:
  if (makesound && one_in(3)) g->explosion(x, y, 40, 0, true);
  else {
   for (int i = x - 2; i <= x + 2; i++) {
    for (int j = y - 2; j <= y + 2; j++) {
     if (move_cost(i, j) > 0 && one_in(3)) add_item(i, j, item::types[itm_gasoline], 0);
     if (move_cost(i, j) > 0 && one_in(6)) add_item(i, j, item::types[itm_steel_chunk], 0);
    }
   }
  }
  terrain = t_rubble;
  break;

 case t_door_c:
 case t_door_b:
 case t_door_locked:
 case t_door_boarded:
  terrain = t_door_frame;
  for (int i = x - 2; i <= x + 2; i++) {
   for (int j = y - 2; j <= y + 2; j++) {
    if (move_cost(i, j) > 0 && one_in(6)) add_item(i, j, item::types[itm_2x4], 0);
   }
  }
  break;

 case t_wall_v:
 case t_wall_h:
  for (int i = x - 2; i <= x + 2; i++) {
   for (int j = y - 2; j <= y + 2; j++) {
    if (move_cost(i, j) > 0 && one_in(5)) add_item(i, j, item::types[itm_rock], 0);
    if (move_cost(i, j) > 0 && one_in(4)) add_item(i, j, item::types[itm_2x4], 0);
   }
  }
  terrain = t_rubble;
  break;

 default:
  if (makesound && has_flag(explodes, x, y) && one_in(2)) g->explosion(x, y, 40, 0, true);
  terrain = t_rubble;
 }

 if (makesound) g->sound(point(x, y), 40, "SMASH!!");
}

void map::shoot(game *g, const point& pt, int &dam, bool hit_items, unsigned flags)
{
 if (dam < 0) return;

 if (has_flag(alarmed, pt) && !event::queued(EVENT_WANTED)) {
  g->sound(g->u.pos, 30, "An alarm sounds!");   // \todo should alarm at destination, not PC
  event::add(event(EVENT_WANTED, int(messages.turn) + MINUTES(30), 0, g->lev.x, g->lev.y));
 }

 if (const auto veh = _veh_at(pt)) {
     bool inc = flags & (mfb(IF_AMMO_INCENDIARY) | mfb(IF_AMMO_FLAME));
     dam = veh->first->damage(veh->second, dam, inc ? vehicle::damage_type::incendiary : vehicle::damage_type::pierce, hit_items);
 }

 switch (ter_id& terrain = ter(pt)) {
 case t_wall_wood_broken:
 case t_door_b:
  if (hit_items || one_in(8)) {	// 1 in 8 chance of hitting the door
   dam -= rng(20, 40);
   if (dam > 0) terrain = t_dirt;
  } else
   dam -= rng(0, 1);
  break;


 case t_door_c:
 case t_door_locked:
 case t_door_locked_alarm:
  dam -= rng(15, 30);
  if (dam > 0) terrain = t_door_b;
  break;

 case t_door_boarded:
  dam -= rng(15, 35);
  if (dam > 0) terrain = t_door_b;
  break;

 case t_window:
 case t_window_alarm:
  dam -= rng(0, 5);
  terrain = t_window_frame;
  break;

 case t_window_boarded:
  dam -= rng(10, 30);
  if (dam > 0) terrain = t_window_frame;
  break;

 case t_wall_glass_h:
 case t_wall_glass_v:
 case t_wall_glass_h_alarm:
 case t_wall_glass_v_alarm:
  dam -= rng(0, 8);
  terrain = t_floor;
  break;

 case t_paper:
  dam -= rng(4, 16);
  if (dam > 0) terrain = t_dirt;
  if (flags & mfb(IF_AMMO_INCENDIARY)) add_field(g, pt, fd_fire, 1);
  break;

 case t_gas_pump:
  if (hit_items || one_in(3)) {
   if (dam > 15) {
    if (flags & mfb(IF_AMMO_INCENDIARY) || flags & mfb(IF_AMMO_FLAME))
     g->explosion(pt, 40, 0, true);
    else {
     for (int i = pt.x - 2; i <= pt.x + 2; i++) {
      for (int j = pt.y - 2; j <= pt.y + 2; j++) {
       if (move_cost(i, j) > 0 && one_in(3))
        add_item(i, j, item::types[itm_gasoline], 0);
      }
     }
    }
    terrain = t_gas_pump_smashed;
   }
   dam -= 60;
  }
  break;

 case t_vat:
  if (dam >= 10) {
   g->sound(pt, 15, "ke-rash!");
   terrain = t_floor;
  } else
   dam = 0;
  break;

 default:
  if (move_cost(pt) == 0 && !trans(pt))
   dam = 0;	// TODO: Bullets can go through some walls?
  else
   dam -= (rng(0, 1) * rng(0, 1) * rng(0, 1));
 }

 if (flags & mfb(IF_AMMO_TRAIL) && !one_in(4)) add_field(g, pt, fd_smoke, rng(1, 2));
 if (dam < 0) dam = 0; // Set damage to 0 if it's less

// Check fields?
 field& fieldhit = field_at(pt);
 switch (fieldhit.type) {
  case fd_web:
   if (flags & (mfb(IF_AMMO_INCENDIARY) | mfb(IF_AMMO_FLAME)))
    add_field(g, pt, fd_fire, fieldhit.density - 1);
   else if (dam > 5 + fieldhit.density * 5 && one_in(5 - fieldhit.density)) {
    dam -= rng(1, 2 + fieldhit.density * 2);
    remove_field(pt);
   }
   break;
 }

// Now, destroy items on that tile.
 const auto pos = to(pt);
 if (!pos || (!hit_items && 2 == move_cost(*pos))) return; // Items on floor-type spaces won't be shot up.

 {
 auto& stack = i_at(*pos);
 for (int i = 0; i < stack.size(); i++) {	// forward-increment so that containers do not grant invulnerability to their contents
  bool destroyed = false;
  auto& it = stack[i];
  switch (it.type->m1) {
   case GLASS:
   case PAPER:
    if (dam > rng(2, 8) && one_in(it.volume())) destroyed = true;
    break;
   case PLASTIC:
    if (dam > rng(2, 10) && one_in(it.volume() * 3)) destroyed = true;
    break;
   case VEGGY:
   case FLESH:
    if (dam > rng(10, 40)) destroyed = true;
    break;
   case COTTON:
   case WOOL:
    if (5 <= ++it.damage) destroyed = true;
    break;
  }
  if (destroyed) {
   for(auto& obj : it.contents) stack.push_back(obj);
   i_rem(pt, i);
   i--;
  }
 }
 }
}

void GPS_loc::shoot(int& dam, bool hit_items, unsigned flags)
{
    const auto g = game::active();
    if (auto pos = g->toScreen(*this)) g->m.shoot(g,*pos, dam, hit_items, flags);
}

bool map::hit_with_acid(const point& pt)
{
 if (0 != move_cost(pt)) return false; // Didn't hit the tile!

 switch(auto& t = ter(pt)) {
  case t_wall_glass_v:
  case t_wall_glass_h:
  case t_wall_glass_v_alarm:
  case t_wall_glass_h_alarm:
  case t_vat:
   t = t_floor;
   return true;

  case t_door_c:
  case t_door_locked:
  case t_door_locked_alarm:
   if (one_in(3)) t = t_door_b;
   return true;

  case t_door_b:
   if (one_in(4)) t = t_door_frame;
   else return false;
   return true;

  case t_window:
  case t_window_alarm:
   t = t_window_empty;
   return true;

  case t_wax:
   t = t_floor_wax;
   return true;

  case t_toilet:
  case t_gas_pump:
  case t_gas_pump_smashed:
   return false;

  case t_card_science:
  case t_card_military:
   t = t_card_reader_broken;
   return true;

  default: return true;	 // would like to assert but the uploads are debug builds as of 2019-02-18
 }
}

void map::marlossify(int x, int y)
{
 int type = rng(1, 9);
 switch (type) {
  case 1:
  case 2:
  case 3:
  case 4: ter(x, y) = t_fungus;      break;
  case 5:
  case 6:
  case 7: ter(x, y) = t_marloss;     break;
  case 8: ter(x, y) = t_tree_fungal; break;
  case 9: ter(x, y) = t_slime;       break;
 }
}

bool map::open_door(int x, int y, bool inside)
{
 auto& t = ter(x, y);
 switch (t) {
 case t_door_c:
	 t = t_door_o;
	 return true;
 case t_door_metal_c:
	 t = t_door_metal_o;
	 return true;
 case t_door_glass_c:
	 t = t_door_glass_o;
	 return true;
 case t_door_locked:
 case t_door_locked_alarm:
	 if (inside) {
		 t = t_door_o;
		 return true;
	 }
     [[fallthrough]];
 default: return false;
 }
}

bool map::close_door(const point& pt)
{
 auto& t = ter(pt);
 switch(t) {
 case t_door_o:
	 t = t_door_c;
	 return true;
 case t_door_metal_o:
	 t = t_door_metal_c;
	 return true;
 case t_door_glass_o:
	 t = t_door_glass_c;
	 return true;
 default: return false;
 }
}

std::optional<item> map::water_from(const point& pt) const
{
 // 2021-04-24: technically want to re-implement the toilet
 // as a map feature (C:DDA/C:BN) or map object (Rogue Survivor Revived).
 // Fully extracting this to take terrain as a parameter, is not future-correct.
 const ter_id terrain = ter(pt);

 if (!is<swimmable>(terrain) && t_toilet != terrain) return std::nullopt;

 for (decltype(auto) x : ter_t::water_from_terrain) {
     if (terrain != x.first) continue;
     item ret(item::types[itm_water], 0);
     if (x.second.first.numerator() >= rng(1, x.second.first.denominator())) ret.poison = rng(x.second.second.first, x.second.second.second);
     return ret;
 };
 return std::nullopt;
}

void map::i_rem(int x, int y, int index)
{
 auto& stack = i_at(x, y);
 if (index > stack.size() - 1) return;
 EraseAt(stack, index);
}

void map::i_clear(int x, int y)
{
 i_at(x, y).clear();
}

std::optional<std::pair<point, int> > map::find_item(item* it) const
{
    point ret;
    for (ret.x = 0; ret.x < SEEX * my_MAPSIZE; ret.x++) {
        for (ret.y = 0; ret.y < SEEY * my_MAPSIZE; ret.y++) {
            int i = -1;
            for (auto& obj : i_at(ret)) {
                ++i;
                if (it == &obj) return std::pair<point, int>(ret, i);
            }
        }
    }
    return std::nullopt;
}

void map::add_item(int x, int y, const itype* type, int birthday)
{
    if (const auto it = submap::for_drop(ter(x, y), type, birthday)) add_item(x, y, std::move(*it));
}

static bool item_location_ok(const map& m, const reality_bubble_loc& pos)
{
    return !m.has_flag(noitem, pos) && m.i_at(pos).size() < 26;
}

static bool item_location_alternate(const map& m, point origin, reality_bubble_loc& dest)
{
    std::vector<reality_bubble_loc> okay;
    for (decltype(auto) delta : Direction::vector) {
        const auto pos = m.to(origin + delta);
        if (pos && m.move_cost(*pos) > 0 && item_location_ok(m, *pos)) okay.push_back(*pos);
    }
    if (okay.empty()) {
        for (int i = -2; i <= 2; i++) {
            for (int j = -2; j <= 2; j++) {
                if (-2 != i && 2 != i && -2 != j && 2 != j) continue;
                const auto pos = m.to(origin + point(i, j));
                if (pos && m.move_cost(*pos) > 0 && item_location_ok(m, *pos)) okay.push_back(*pos);
            }
        }
    }
    // do not recurse.
    if (auto ub = okay.size()) {
        dest = okay[rng(0, ub - 1)];
    }
    return !okay.empty();
}

void map::add_item(int x, int y, const item& new_item)
{
 if (new_item.is_style()) return;

 auto pos_in = to(x, y);
 if (!pos_in) return;
 if (new_item.made_of(LIQUID) && has_flag(swimmable, *pos_in)) return;
 if (!item_location_ok(*this, *pos_in)) {
     if (item_location_alternate(*this, point(x, y), *pos_in)) {
         if (new_item.made_of(LIQUID) && has_flag(swimmable, *pos_in)) return;
     } // STILL?	\todo decide what gets lost
 }

 grid[pos_in->first]->items_at(pos_in->second).push_back(new_item);
 if (new_item.active) grid[pos_in->first]->active_item_count++;
}

// \todo: make these bypass map class
void GPS_loc::add(const item& new_item)
{
    const auto g = game::active();
    if (auto pos = g->toScreen(*this)) g->m.add_item(*pos, new_item);
}

void GPS_loc::add(item&& new_item)
{
    const auto g = game::active();
    if (auto pos = g->toScreen(*this)) g->m.add_item(*pos, std::move(new_item));
}

void map::add_item(int x, int y, item&& new_item)
{
    if (new_item.is_style()) return;

    auto pos_in = to(x, y);
    if (!pos_in) return;
    if (new_item.made_of(LIQUID) && has_flag(swimmable, *pos_in)) return;
    if (!item_location_ok(*this, *pos_in)) {
        if (item_location_alternate(*this, point(x, y), *pos_in)) {
            if (new_item.made_of(LIQUID) && has_flag(swimmable, *pos_in)) return;
        } // STILL?	\todo decide what gets lost
    }

    if (new_item.active) grid[pos_in->first]->active_item_count++;
    grid[pos_in->first]->items_at(pos_in->second).push_back(std::move(new_item));
}

bool map::hard_landing(const point& pt, item&& thrown, player* p)
{
    const auto g = game::active();
    if (thrown.made_of(GLASS)) {
        bool break_this = !p;
        if (!break_this && !thrown.active) {
            const int vol = thrown.volume();
            if (rng(0, vol + 8) - rng(0, p->str_cur) < vol) break_this = true;
        }
        if (break_this) {
            if (g->u.see(pt)) messages.add("The %s shatters!", thrown.tname().c_str());
            for (auto& obj : thrown.contents) add_item(pt, std::move(obj));
            g->sound(pt, 16, "glass breaking!");
            return true;
        }
    }
    add_item(pt, std::move(thrown));
    return false;
}

bool GPS_loc::hard_landing(item&& thrown, player* p)
{
    if (thrown.made_of(GLASS)) {
        bool break_this = !p;
        if (!break_this && !thrown.active) {
            const int vol = thrown.volume();
            if (rng(0, vol + 8) - rng(0, p->str_cur) < vol) break_this = true;
        }
        if (break_this) {
            if (game::active()->u.see(*this)) messages.add("The %s shatters!", thrown.tname().c_str());
            for (auto& obj : thrown.contents) add(std::move(obj));
            sound(16, "glass breaking!");
            return true;
        }
    }
    add(std::move(thrown));
    return false;
}

void submap::process_active_items()
{
    if (0 >= active_item_count) return;
    const auto g = game::active();
    for (int i = 0; i < SEEX; i++) {
        for (int j = 0; j < SEEY; j++) {
            std::vector<item>& items = itm[i][j];
            int n = items.size();
            while (0 < n) {
                if (decltype(auto) it = items[--n]; it.active) {
                    switch (int code = g->u.use_active(it))   // XXX \todo allow modeling active item effects w/o player
                    { // ignore artifacts/code -2
                    case -1:   // discharge charger gun
                        it.active = false;
                        it.charges = 0;
                        active_item_count--;
                        break;
                    case 1:
                        EraseAt(items, n);  // reference invalidated
                        active_item_count--;
                        break;
                    case 0:
                        if (!it.active) active_item_count--;
                        break;
                    }
                }
            }
        }
    }
}

void map::process_active_items()
{
    for(submap* const gr : grid) gr->process_active_items();
}

void map::use_amount(point origin, int range, const itype_id type, int quantity, bool use_container)
{
 for (int radius = 0; radius <= range && quantity > 0; radius++) {
  for (int x = origin.x - radius; x <= origin.x + radius; x++) {
   for (int y = origin.y - radius; y <= origin.y + radius; y++) {
    if (rl_dist(origin, x, y) <= radius) {
     for (int n = 0; n < i_at(x, y).size() && quantity > 0; n++) {
      item* curit = &(i_at(x, y)[n]);
	  bool used_contents = false;
	  if (use_container) {
       for (int m = 0; m < curit->contents.size() && quantity > 0; m++) {
		if (type != curit->contents[m].type->id) continue;
		quantity--;
		EraseAt(curit->contents, m);
        m--;
		used_contents = curit->contents.empty();
       }
	  }
      if (used_contents) {
       i_rem(x, y, n);
       n--;
      } else if (curit->type->id == type && quantity > 0) {
       quantity--;
       i_rem(x, y, n);
       n--;
      }
     }
    }
   }
  }
 }
}

void map::use_charges(point origin, int range, const itype_id type, int quantity)
{
 // XXX cubic rather than quartic \todo fix
 for (int radius = 0; radius <= range && quantity > 0; radius++) {
  for (int x = origin.x - radius; x <= origin.x + radius; x++) {
   for (int y = origin.y - radius; y <= origin.y + radius; y++) {
       if (radius < rl_dist(origin, x, y)) continue;   // invariant?  meaningful for trigonmetric distance
       auto n = i_at(x, y).size();
       while (0 < n) {
           auto& curit = i_at(x, y)[--n];
           if (auto code = curit.use_charges(type, quantity)) {
               if (0 > code) {
                   i_rem(x, y, n);
                   if (0 < quantity) continue;
               }
               return;
           }
       }
   }
  }
 }
}

trap_id& GPS_loc::trap_at()
{
    if (submap* const sm = game::active()->m.chunk(*this)) {
        if (const auto terrain_trap = ter_t::list[sm->ter[second.x][second.y]].trap) return (discard<trap_id>::x = terrain_trap);
        return sm->trp[second.x][second.y];
    }
    return (discard<trap_id>::x = tr_null);	// Out-of-bounds, return our null trap
}

trap_id GPS_loc::trap_at() const
{
    if (const submap* const sm = game::active()->m.chunk(*this)) {
        if (const auto terrain_trap = ter_t::list[sm->ter[second.x][second.y]].trap) return terrain_trap;
        return sm->trp[second.x][second.y];
    }
    return tr_null;	// Out-of-bounds, return our null trap
}

field& GPS_loc::field_at()
{
    if (submap* const sm = game::active()->m.chunk(*this)) {
        return sm->field_at(second);
    }
    return (discard<field>::x = field());	// Out-of-bounds, return null field
}

std::vector<item>& GPS_loc::items_at()
{
    if (submap* const sm = game::active()->m.chunk(*this)) {
        return sm->items_at(second);
    }
    return (discard<std::vector<item> >::x = std::vector<item>());	// Out-of-bounds, return null field
}

trap_id& map::tr_at(int x, int y)
{
    if (const auto pos = to(x, y)) {
        if (const auto terrain_trap = ter_t::list[grid[pos->first]->ter[pos->second.x][pos->second.y]].trap) return (discard<trap_id>::x = terrain_trap);
        return grid[pos->first]->trp[pos->second.x][pos->second.y];
    }
    return (discard<trap_id>::x = tr_null);	// Out-of-bounds, return our null trap
}

void map::add_trap(int x, int y, trap_id t)
{
    if (const auto pos = to(x, y)) grid[pos->first]->trp[pos->second.x][pos->second.y] = t;
}

std::optional<item> submap::for_drop(ter_id dest, const itype* type, int birthday)
{
    if (type->is_style()) return std::nullopt;
    // ignore corpse special case here when inlining item::made_of
    if ((type->m1 == LIQUID || type->m2 == LIQUID)
        && is<swimmable>(dest))
        return std::nullopt;
    return item(type, birthday).in_its_container();
}

field* submap::add(const point& p, field&& src)
{
    auto& fd = field_at(p);
    if (fd.type == fd_web && src.type == fd_fire) src.density++;
    else if (!fd.is_null()) return nullptr; // Blood & bile are null too
    if (3 < src.density) src.density = 3;
    if (0 >= src.density) return nullptr;
    if (fd.type == fd_null) field_count++;
    return &(fd = std::move(src));
}

bool map::add_field(game *g, int x, int y, field_id t, unsigned char density, unsigned int age)
{
    const auto pos = to(x, y);
    if (!pos) return false; // wasn't in bounds
    if (decltype(auto) fd = grid[pos->first]->add(pos->second, field(t, density, age))) {
        if (g && fd->is_dangerous()) {
            if (const auto _pc = g->survivor(x, y)) _pc->cancel_activity_query("You're in a %s!", field::list[t].name[fd->density - 1].c_str());
        }
        return true;
    }
    return false;
}

// return value is per waterfall/SSADM software lifecycle
bool GPS_loc::add(field&& src)
{   // intentionally no-op if submap doesn't exist
    if (submap* const sm = MAPBUFFER.lookup_submap(first)) {
        if (src.is_dangerous()) {
            if (const auto _pc = game::active()->survivor(*this)) _pc->cancel_activity_query("You're in a %s!", src.name().c_str());
        }
        sm->add(second, std::move(src));
        return true;
    }
    return false;
}


void submap::remove_field(const point& p) {
    if (fld[p.x][p.y].type != fd_null) field_count--;
    fld[p.x][p.y] = field();
}

computer* map::computer_at(const point& pt)
{
 if (auto pos = to(pt)) {
     auto& c = grid[pos->first]->comp;
     if (!c.name.empty()) return &c;
 }
 return nullptr;
}

void map::debug()
{
 mvprintw(0, 0, "MAP DEBUG");
 getch();
 for (int i = 0; i <= SEEX * 2; i++) {
  for (int j = 0; j <= SEEY * 2; j++) {
   const auto& stack = i_at(i, j);
   if (!stack.empty()) {
    mvprintw(1, 0, "%d, %d: %d items", i, j, stack.size());
    mvprintw(2, 0, "%c, %d", stack.back().symbol(), stack.back().color());
    getch();
   }
  }
 }
 getch();
}

void map::draw(game *g, WINDOW* w, point center)
{
 int light = g->u.sight_range();
 forall_do_inclusive(view_center_extent(), [&](point offset) {
        const point real(center + offset);
        const int dist = rl_dist(g->u.pos, real);
        const point draw_at(offset + point(VIEW_CENTER));
        if (dist > light) {
            mvwputch(w, draw_at.y, draw_at.x, (g->u.has_disease(DI_BOOMERED) ? c_magenta : c_dkgray), '#');
        } else if (dist <= g->u.clairvoyance() || sees(g->u.pos, real, light))
            drawsq(w, g->u, real.x, real.y, false, true, center);
        else
            mvwputch(w, draw_at.y, draw_at.x, c_black, '#');
     });
 const point at(g->u.pos - center + point(VIEW_CENTER));
 if (at.x >= 0 && at.x < VIEW && at.y >= 0 && at.y < VIEW)
  mvwputch(w, at.y, at.x, g->u.color(), '@');
}

void map::drawsq(WINDOW* w, const player& u, int x, int y, bool invert, bool show_items, std::optional<point> viewpoint) const
{
    const auto pos = to(x, y);
    if (!pos) return;	// Out of bounds
    if (!viewpoint) viewpoint = u.pos;
    const int k = x + VIEW_CENTER - viewpoint->x;
    const int j = y + VIEW_CENTER - viewpoint->y;
    nc_color tercol;
    const auto terrain = ter(*pos);
    long sym = ter_t::list[terrain].sym;
    bool hi = false;
    bool normal_tercol = false;
    bool drew_field = false;
    mvwputch(w, j, k, c_black, ' ');	// actively clear
    if (u.has_disease(DI_BOOMERED))
        tercol = c_magenta;
    else if ((u.is_wearing(itm_goggles_nv) && u.has_active_item(itm_UPS_on)) ||
        u.has_active_bionic(bio_night_vision))
        tercol = c_ltgreen;
    else {
        normal_tercol = true;
        tercol = ter_t::list[terrain].color;
    }
    // background tile should show no matter what
    if (ter_t::tiles.count(terrain)) {
        if (mvwaddbgtile(w, j, k, ter_t::tiles[terrain].c_str())) sym = 0;
    }

    if (0 == move_cost(*pos) && has_flag(swimmable, x, y) && !u.underwater)
        show_items = false;	// Can only see underwater items if WE are underwater
      // If there's a trap here, and we have sufficient perception, draw that instead
    const auto tr_id = tr_at(*pos);
    if (tr_id != tr_null) {
        const trap* const tr = trap::traps[tr_id];
        if (u.per_cur - u.encumb(bp_eyes) >= tr->visibility) {
            tercol = tr->color;
            if (tr->sym == '%') {
                switch (rng(1, 5)) {
                case 1: sym = '*'; break;
                case 2: sym = '0'; break;
                case 3: sym = '8'; break;
                case 4: sym = '&'; break;
                case 5: sym = '+'; break;
                }
            } else sym = tr->sym;
        }
    }
    // If there's a field here, draw that instead (unless its symbol is %)
    const auto& fd = grid[pos->first]->field_at(pos->second);
    if (fd.type != fd_null && field::list[fd.type].sym != '&') {
        tercol = field::list[fd.type].color[fd.density - 1];
        drew_field = true;
        if (field::list[fd.type].sym == '*') {
            switch (rng(1, 5)) {
            case 1: sym = '*'; break;
            case 2: sym = '0'; break;
            case 3: sym = '8'; break;
            case 4: sym = '&'; break;
            case 5: sym = '+'; break;
            }
        } else if (field::list[fd.type].sym != '%') {
            sym = field::list[fd.type].sym;
            drew_field = false;
        }
    }
    // If there's items here, draw those instead
    if (show_items && !drew_field) {
        const auto& stack = i_at(x, y);
        if (!stack.empty()) {
            if ((ter_t::list[ter(x, y)].sym != '.')) hi = true;
            else {
                auto& top = stack.back();
                tercol = top.color();
                sym = top.symbol();
                if (stack.size() > 1) invert = !invert;
            }
        }
    }

    if (const auto v = veh_at(*pos)) {
        const vehicle* const veh = v->first; // backward compatibility
        sym = special_symbol(veh->face.dir_symbol(veh->part_sym(v->second)));
        if (normal_tercol) tercol = veh->part_color(v->second);
    }

    if (sym) {
        if (invert) mvwputch_inv(w, j, k, tercol, sym);
        else if (hi) mvwputch_hi(w, j, k, tercol, sym);
        else mvwputch(w, j, k, tercol, sym);
    }
}

void map::drawsq(WINDOW* w, const player& u, GPS_loc dest, bool invert, bool show_items, std::optional<GPS_loc> viewpoint)
{
    if (!viewpoint) viewpoint = u.GPSpos;
    const auto delta = dest - *viewpoint;
    if (std::get_if<tripoint>(&delta)) return;  // we don't handle cross-layer view
    decltype(auto) delta_pt = std::get<point>(delta);
    zaimoni::gdi::box<point> viewport(point(-VIEW_CENTER), point(VIEW_CENTER));
    if (!viewport.contains(delta_pt)) return; // not within viewport

    const auto draw_at = delta_pt + point(VIEW_CENTER);

    nc_color tercol;
    const auto terrain = dest.ter();
    long sym = ter_t::list[terrain].sym;
    bool hi = false;
    bool normal_tercol = false;
    bool drew_field = false;
    mvwputch(w, draw_at.y, draw_at.x, c_black, ' ');	// actively clear
    if (u.has_disease(DI_BOOMERED))
        tercol = c_magenta;
    else if ((u.is_wearing(itm_goggles_nv) && u.has_active_item(itm_UPS_on)) ||
        u.has_active_bionic(bio_night_vision))
        tercol = c_ltgreen;
    else {
        normal_tercol = true;
        tercol = ter_t::list[terrain].color;
    }
    // background tile should show no matter what
    if (ter_t::tiles.count(terrain)) {
        if (mvwaddbgtile(w, draw_at.y, draw_at.x, ter_t::tiles[terrain].c_str())) sym = 0;
    }

    if (0 == dest.move_cost() && is<swimmable>(terrain) && !u.underwater)
        show_items = false;	// Can only see underwater items if WE are underwater

    static constexpr const char random_glyph[] = { '*', '0', '8', '&', '+' };

      // If there's a trap here, and we have sufficient perception, draw that instead
    if (const auto tr_id = dest.trap_at()) {
        const trap* const tr = trap::traps[tr_id];
        if (u.per_cur - u.encumb(bp_eyes) >= tr->visibility) {
            tercol = tr->color;
            if (tr->sym == '%') sym = random_glyph[rng(0, (std::end(random_glyph) - std::begin(random_glyph)) - 1)];
            else sym = tr->sym;
        }
    }
    // If there's a field here, draw that instead (unless its symbol is %)
    const auto& fd = dest.field_at();
    if (fd.type != fd_null && field::list[fd.type].sym != '&') {
        tercol = field::list[fd.type].color[fd.density - 1];
        drew_field = true;
        if (field::list[fd.type].sym == '*') {
            sym = random_glyph[rng(0, (std::end(random_glyph) - std::begin(random_glyph)) - 1)];
        } else if (field::list[fd.type].sym != '%') {
            sym = field::list[fd.type].sym;
            drew_field = false;
        }
    }
    // If there's items here, draw those instead
    if (show_items && !drew_field) {
        const auto& stack = dest.items_at();
        if (!stack.empty()) {
            if ((ter_t::list[terrain].sym != '.')) hi = true;
            else {
                auto& top = stack.back();
                tercol = top.color();
                sym = top.symbol();
                if (stack.size() > 1) invert = !invert;
            }
        }
    }

    if (const auto v = dest.veh_at()) {
        const vehicle* const veh = v->first; // backward compatibility
        sym = special_symbol(veh->face.dir_symbol(veh->part_sym(v->second)));
        if (normal_tercol) tercol = veh->part_color(v->second);
    }

    if (sym) {
        if (invert) mvwputch_inv(w, draw_at.y, draw_at.x, tercol, sym);
        else if (hi) mvwputch_hi(w, draw_at.y, draw_at.x, tercol, sym);
        else mvwputch(w, draw_at.y, draw_at.x, tercol, sym);
    }
}

/*
based off code by Steve Register [arns@arns.freeservers.com]
http://roguebasin.roguelikedevelopment.org/index.php?title=Simple_Line_of_Sight
*/
std::optional<int> map::_BresenhamLine(int Fx, int Fy, int Tx, int Ty, int range, std::function<bool(reality_bubble_loc)> test) const
{
    int dx = Tx - Fx;
    int dy = Ty - Fy;

    if (range >= 0 && (abs(dx) > range || abs(dy) > range)) return std::nullopt;	// Out of range!

    int ax = abs(dx) << 1;
    int ay = abs(dy) << 1;
    int sx = signum(dx);
    int sy = signum(dy);
    int x = Fx;
    int y = Fy;
    int t = 0;
    int st;

    decltype(to(x,y)) pos;

    if (ax > ay) { // Mostly-horizontal line
        st = signum(ay - (ax >> 1));
        // Doing it "backwards" prioritizes straight lines before diagonal.
        // This will help avoid creating a string of zombies behind you and will
        // promote "mobbing" behavior (zombies surround you to beat on you)
        for (int tc = abs(ay - (ax >> 1)) * 2 + 1; tc >= -1; tc--) {
            t = tc * st;
            x = Fx;
            y = Fy;
            do {
                if (t > 0) {
                    y += sy;
                    t -= ax;
                }
                x += sx;
                t += ay;
                if (x == Tx && y == Ty) {
                    tc *= st;
                    return tc;
                }
            } while ((pos = to(x, y)) && test(*pos));
        }
        return std::nullopt;
    } else { // Same as above, for mostly-vertical lines
        st = signum(ax - (ay >> 1));
        for (int tc = abs(ax - (ay >> 1)) * 2 + 1; tc >= -1; tc--) {
            t = tc * st;
            x = Fx;
            y = Fy;
            do {
                if (t > 0) {
                    x += sx;
                    t -= ay;
                }
                y += sy;
                t += ax;
                if (x == Tx && y == Ty) {
                    tc *= st;
                    return tc;
                }
            } while ((pos = to(x, y)) && test(*pos));
        }
        return std::nullopt;
    }
    return std::nullopt; // Shouldn't ever be reached, but there it is.
}

std::optional<int> map::sees(int Fx, int Fy, int Tx, int Ty, int range) const
{
  return _BresenhamLine(Fx, Fy, Tx, Ty, range, [&](reality_bubble_loc pos) { return trans(pos); });
}

std::optional<int> map::clear_path(int Fx, int Fy, int Tx, int Ty, int range, int cost_min, int cost_max) const
{
    return _BresenhamLine(Fx, Fy, Tx, Ty, range, [&](reality_bubble_loc pos){ return is_between(cost_min, move_cost(pos), cost_max);});
}

// stub to enable building
template<bool want_path=false>
static std::conditional_t<want_path, std::optional<std::vector<GPS_loc> >, bool> _BresenhamLine(GPS_loc origin, tripoint delta, int range, std::function<bool(GPS_loc)> test)
{
#if 0
    int dx = Tx - Fx;
    int dy = Ty - Fy;

    if (range >= 0 && (abs(dx) > range || abs(dy) > range)) return std::nullopt;	// Out of range!

    int ax = abs(dx) << 1;
    int ay = abs(dy) << 1;
    int sx = signum(dx);
    int sy = signum(dy);
    int x = Fx;
    int y = Fy;
    int t = 0;
    int st;

    GPS_loc loc;
    std::vector<GPS_loc> ret;
    if (0 < range) ret.reserve((size_t)range + 1);

    if (ax > ay) { // Mostly-horizontal line
        st = signum(ay - (ax >> 1));
        // Doing it "backwards" prioritizes straight lines before diagonal.
        // This will help avoid creating a string of zombies behind you and will
        // promote "mobbing" behavior (zombies surround you to beat on you)
        for (int tc = abs(ay - (ax >> 1)) * 2 + 1; tc >= -1; tc--) {
            t = tc * st;
            GPS_loc loc(origin);
            decltype(delta) delta_loc(0);
            do {
                if (t > 0) {
                    y += sy;
                    t -= ax;
                }
                x += sx;
                t += ay;
                if (delta == delta_loc) {
                    tc *= st;
                    return tc;
                }
            } while (test(loc));
        }
        return std::nullopt;
    }
    else { // Same as above, for mostly-vertical lines
        st = signum(ax - (ay >> 1));
        for (int tc = abs(ax - (ay >> 1)) * 2 + 1; tc >= -1; tc--) {
            t = tc * st;
            GPS_loc loc(origin);
            decltype(delta) delta_loc(0);
            do {
                if (t > 0) {
                    x += sx;
                    t -= ay;
                }
                y += sy;
                t += ax;
                if (delta == delta_loc) {
                    tc *= st;
                    return tc;
                }
            } while (test(loc));
        }
        return std::nullopt;
    }
#endif
    // Shouldn't ever be reached, but there it is.
    if constexpr (want_path) return std::nullopt;
    else return false;
}

template<bool want_path = false>
static std::conditional_t<want_path, std::optional<std::vector<GPS_loc> >, bool> _BresenhamLine(GPS_loc origin, const point delta, int range, std::function<bool(GPS_loc)> test)
{
    if (0 <= range && Linf_dist(delta) > range) {	// Out of range!
        if constexpr (want_path) return std::nullopt;
        else return false;
    }

    int ax = abs(delta.x) << 1;
    int ay = abs(delta.y) << 1;
    point dir_x(signum(delta.x), 0);
    point dir_y(0, signum(delta.y));
/*  int sx = signum(delta.x);
    int sy = signum(delta.y); */
/*  int x = Fx;
    int y = Fy; */
    int t = 0;
    int st;

    GPS_loc loc;
    std::vector<GPS_loc> ret;
    if constexpr (want_path) {
        if (0 < range) ret.reserve((size_t)range + 1);
    }

    if (ax > ay) { // Mostly-horizontal line
        st = signum(ay - (ax >> 1));
        // Doing it "backwards" prioritizes straight lines before diagonal.
        // This will help avoid creating a string of zombies behind you and will
        // promote "mobbing" behavior (zombies surround you to beat on you)
        for (int tc = abs(ay - (ax >> 1)) * 2 + 1; tc >= -1; tc--) {
            if constexpr (want_path) ret.clear();
            t = tc * st;
            GPS_loc loc(origin);
//          if constexpr (want_path) ret.push_back(loc); // C:Whales does not include origin in path
            decltype(delta) delta_loc(0);
            do {
                if (t > 0) {
                    loc += dir_y;
                    t -= ax;
                }
                loc += dir_x;
                t += ay;
                if constexpr (want_path) ret.push_back(loc);
                if (delta == delta_loc) {
                    if constexpr (want_path) return ret;
                    else return true;
                }
            } while (test(loc));
        }
        if constexpr (want_path) return std::nullopt;
        else return false;
    } else { // Same as above, for mostly-vertical lines
        st = signum(ax - (ay >> 1));
        for (int tc = abs(ax - (ay >> 1)) * 2 + 1; tc >= -1; tc--) {
            if constexpr (want_path) ret.clear();
            t = tc * st;
            GPS_loc loc(origin);
            if constexpr (want_path) ret.push_back(loc);
            decltype(delta) delta_loc(0);
            do {
                if (t > 0) {
                    loc += dir_x;
                    t -= ay;
                }
                loc += dir_y;
                t += ax;
                if constexpr (want_path) ret.push_back(loc);
                if (delta == delta_loc) {
                    if constexpr (want_path) return ret;
                    else return true;
                }
            } while (test(loc));
        }
        if constexpr (want_path) return std::nullopt;
        else return false;
    }
    // Shouldn't ever be reached, but there it is.
    if constexpr (want_path) return std::nullopt;
    else return false;
}

template<bool want_path = false>
static auto _BresenhamLine(GPS_loc origin, GPS_loc dest, int range, std::function<bool(GPS_loc)> test)
{
    auto delta = dest - origin;
    // \todo convert to std::visit so we have compile-time checking that all cases are handled
    if (auto pt = std::get_if<point>(&delta)) return _BresenhamLine<want_path>(origin, *pt, range, test);
    return _BresenhamLine<want_path>(origin, std::get<tripoint>(delta), range, test);
}

std::optional<std::vector<GPS_loc> > GPS_loc::sees(const GPS_loc& dest, int range) const
{
    return _BresenhamLine<true>(*this, dest, range, [&](GPS_loc loc) { return loc.is_transparent(); });
}

bool GPS_loc::can_see(const GPS_loc& dest, int range) const
{
    return _BresenhamLine(*this, dest, range, [&](GPS_loc loc) { return loc.is_transparent(); });
}

// Bash defaults to true.
std::vector<point> map::route(int Fx, int Fy, int Tx, int Ty, bool bash) const
{
/* TODO: If the origin or destination is out of bound, figure out the closest
 * in-bounds point and go to that, then to the real origin/destination.
 */

 if (!inbounds(Fx, Fy) || !inbounds(Tx, Ty)) {
  if (const auto linet = sees(Fx, Fy, Tx, Ty, -1))
   return line_to(Fx, Fy, Tx, Ty, *linet);
  else {
   return std::vector<point>();
  }
 }
// First, check for a simple straight line on flat ground
 if (const auto linet = clear_path(Fx, Fy, Tx, Ty, -1, 2, 2)) return line_to(Fx, Fy, Tx, Ty, *linet);

 std::vector<point> open;
 // Zaimoni: we're taking a memory management hit converting from C99 stack-allocated arrays to std::vector here.
 std::vector<std::vector<astar_list> > list(SEEX * my_MAPSIZE, std::vector<astar_list>(SEEY * my_MAPSIZE, ASL_NONE)); // Init to not being on any list
 std::vector<std::vector<int> > score(SEEX * my_MAPSIZE, std::vector<int>(SEEY * my_MAPSIZE, 0));	// No score!
 std::vector<std::vector<int> > gscore(SEEX * my_MAPSIZE, std::vector<int>(SEEY * my_MAPSIZE, 0));	// No score!
 std::vector<std::vector<point> > parent(SEEX * my_MAPSIZE, std::vector<point>(SEEY * my_MAPSIZE, point(-1, -1)));

 int startx = Fx - 4, endx = Tx + 4, starty = Fy - 4, endy = Ty + 4;
 if (Tx < Fx) {
  startx = Tx - 4;
  endx = Fx + 4;
 }
 if (Ty < Fy) {
  starty = Ty - 4;
  endy = Fy + 4;
 }
 if (startx < 0)
  startx = 0;
 if (starty < 0)
  starty = 0;
 if (endx > SEEX * my_MAPSIZE - 1)
  endx = SEEX * my_MAPSIZE - 1;
 if (endy > SEEY * my_MAPSIZE - 1)
  endy = SEEY * my_MAPSIZE - 1;

 list[Fx][Fy] = ASL_OPEN;
 open.push_back(point(Fx, Fy));

 bool done = false;

 do {
  int best = INT_MAX;
  int index = -1;
  for (int i = 0; i < open.size(); i++) {
   if (score[open[i].x][open[i].y] < best) {
    best = score[open[i].x][open[i].y];
    index = i;
   }
  }
  for (decltype(auto) dir_vec : Direction::vector) {
      const point dest = open[index] + dir_vec;
      if (dest.x == Tx && dest.y == Ty) {
          done = true;
          parent[dest.x][dest.y] = open[index];
      } else if (dest.x >= startx && dest.x <= endx && dest.y >= starty && dest.y <= endy) {
          const int mv_cost = move_cost(dest);
          const bool can_destroy = bash && has_flag(bashable, dest);
          if (0 < mv_cost || can_destroy) {
              decltype(auto) gcost = [&]() {
                  int new_g = gscore[open[index].x][open[index].y] + mv_cost;
                  if (ter(dest) == t_door_c) new_g += 4;	// A turn to open it and a turn to move there
                  else if (0 == mv_cost && can_destroy) new_g += 18;	// Worst case scenario with damage penalty
                  return new_g;
              };

              switch(auto& asl_mode = list[dest.x][dest.y]) {
              case ASL_NONE: // Not listed, so make it open
                  asl_mode = ASL_OPEN;
                  open.push_back(dest);
                  parent[dest.x][dest.y] = open[index];
                  gscore[dest.x][dest.y] = gcost();
                  score[dest.x][dest.y] = gscore[dest.x][dest.y] + 2 * rl_dist(dest, Tx, Ty);
                  break;
              case ASL_OPEN: // It's open, but make it our child
                  if (int newg = gcost(); newg < gscore[dest.x][dest.y]) {
                      gscore[dest.x][dest.y] = newg;
                      parent[dest.x][dest.y] = open[index];
                      score[dest.x][dest.y] = gscore[dest.x][dest.y] + 2 * rl_dist(dest, Tx, Ty);
                  }
                  break;
              }
          }
      }
  }

  list[open[index].x][open[index].y] = ASL_CLOSED;
  open.erase(open.begin() + index);
 } while (!done && !open.empty());

 std::vector<point> tmp;
 std::vector<point> ret;
 if (done) {
  point cur(Tx, Ty);
  while (cur.x != Fx || cur.y != Fy) {
   tmp.push_back(cur);
   assert(1 == rl_dist(cur, parent[cur.x][cur.y]));
   cur = parent[cur.x][cur.y];
  }
  while (!tmp.empty()) {
      ret.push_back(tmp.back());
      tmp.pop_back();
  }
 }
 return ret;
}

void map::save(const tripoint& om_pos, unsigned int turn, const point& world)
{
 for (int gridx = 0; gridx < my_MAPSIZE; gridx++) {
  for (int gridy = 0; gridy < my_MAPSIZE; gridy++)
   saven(om_pos, turn, world, gridx, gridy);
 }
}

void map::load(game *g, const point& world)
{
 for (int gridx = 0; gridx < my_MAPSIZE; gridx++) {
  for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
   if (!loadn(g, world, gridx, gridy)) loadn(g, world, gridx, gridy);
   const int i = gridx + gridy * my_MAPSIZE;
   // null pointer is a hard crash...just exit, now
   if (!grid[i]) {
     debuglog("grid %d (%d, %d) is null! mapbuffer size = %d", i, i % my_MAPSIZE, i / my_MAPSIZE, MAPBUFFER.size());
	 debugmsg("grid %d (%d, %d) is null! mapbuffer size = %d", i, i % my_MAPSIZE, i / my_MAPSIZE, MAPBUFFER.size()); // historical UI
     exit(EXIT_FAILURE);
   }
  }
 }
}

void map::load(const tripoint& GPS)
{
    for (int gridx = 0; gridx < my_MAPSIZE; gridx++) {
        for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
            if (!loadn(GPS, gridx, gridy)) loadn(GPS, gridx, gridy);
            const int i = gridx + gridy * my_MAPSIZE;
            // null pointer is a hard crash...just exit, now
            if (!grid[i]) {
                debuglog("grid %d (%d, %d) is null! mapbuffer size = %d", i, i % my_MAPSIZE, i / my_MAPSIZE, MAPBUFFER.size());
                debugmsg("grid %d (%d, %d) is null! mapbuffer size = %d", i, i % my_MAPSIZE, i / my_MAPSIZE, MAPBUFFER.size()); // historical UI
                exit(EXIT_FAILURE);
            }
        }
    }
}

void map::load(const OM_loc<1>& GPS)
{
    // \todo extract this to to OM_loc::toGPS; needs to use exceptions for error reporting?
    if (INT_MAX / (2 * OMAP) < GPS.first.x || INT_MIN / (2 * OMAP) > GPS.first.x) return;   // does not represent
    if (INT_MAX / (2 * OMAP) < GPS.first.y || INT_MIN / (2 * OMAP) > GPS.first.y) return;   // does not represent
    GPS_loc tmp(tripoint(GPS.first.x*(2*OMAP), GPS.first.y * (2 * OMAP), GPS.first.z), point(0, 0));
    if (0 < GPS.second.x && 0 < tmp.first.x && INT_MAX - tmp.first.x < GPS.second.x) return; // does not represent
    else if (0 > GPS.second.x && 0 > tmp.first.x&& INT_MIN - tmp.first.x > GPS.second.x) return; // does not represent
    if (0 < GPS.second.y && INT_MAX - tmp.first.y < GPS.second.y) return; // does not represent
    else if (0 > GPS.second.y && 0 > tmp.first.y && INT_MIN - tmp.first.y > GPS.second.y) return; // does not represent
    tmp.first.x += GPS.second.x;
    tmp.first.y += GPS.second.y;
    // end extraction of OM_loc::toGPS
    load(tmp.first);
}

void map::shift(game *g, const point& world, const point& delta)
{
 const point dest(world+delta);
// Special case of 0-shift; refresh the map
 if (delta.x == 0 && delta.y == 0) {
#if SHOULD_BE_NOOP
  for (int gridx = 0; gridx < my_MAPSIZE; gridx++) {
   for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
    if (!loadn(g, dest, gridx, gridy))
     loadn(g, dest, gridx, gridy);
   }
  }
#endif
  return;
 }

// if player is driving vehicle, (s)he must be shifted with vehicle too
 if (g->u.in_vehicle) g->u.pos -= SEE*delta;

// Shift the map sx submaps to the right and sy submaps down.
// sx and sy should never be bigger than +/-1.
// wx and wy are our position in the world, for saving/loading purposes.
 if (delta.x >= 0) {
  for (int gridx = 0; gridx < my_MAPSIZE; gridx++) {
   if (delta.y >= 0) {
    for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
     if (gridx + delta.x < my_MAPSIZE && gridy + delta.y < my_MAPSIZE)
      copy_grid(gridx + gridy * my_MAPSIZE,
                gridx + delta.x + (gridy + delta.y) * my_MAPSIZE);
     else if (!loadn(g, dest, gridx, gridy))
      loadn(g, dest, gridx, gridy);
    }
   } else { // sy < 0; work through it backwards
    for (int gridy = my_MAPSIZE - 1; gridy >= 0; gridy--) {
     if (gridx + delta.x < my_MAPSIZE && gridy + delta.y >= 0)
      copy_grid(gridx + gridy * my_MAPSIZE,
                gridx + delta.x + (gridy + delta.y) * my_MAPSIZE);
     else if (!loadn(g, dest, gridx, gridy))
      loadn(g, dest, gridx, gridy);
    }
   }
  }
 } else { // sx < 0; work through it backwards
  for (int gridx = my_MAPSIZE - 1; gridx >= 0; gridx--) {
   if (delta.y >= 0) {
    for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
     if (gridx + delta.x >= 0 && gridy + delta.y < my_MAPSIZE)
      copy_grid(gridx + gridy * my_MAPSIZE,
                gridx + delta.x + (gridy + delta.y) * my_MAPSIZE);
     else if (!loadn(g, dest, gridx, gridy))
      loadn(g, dest, gridx, gridy);
    }
   } else { // sy < 0; work through it backwards
    for (int gridy = my_MAPSIZE - 1; gridy >= 0; gridy--) {
     if (gridx + delta.x >= 0 && gridy + delta.y >= 0)
      copy_grid(gridx + gridy * my_MAPSIZE,
                gridx + delta.x + (gridy + delta.y) * my_MAPSIZE);
     else if (!loadn(g, dest, gridx, gridy))
      loadn(g, dest, gridx, gridy);
    }
   }
  }
 }
}

// saven saves a single nonant.  worldx and worldy are used for the file
// name and specifies where in the world this nonant is.  gridx and gridy are
// the offset from the top left nonant:
// 0,0 1,0 2,0
// 0,1 1,1 2,1
// 0,2 1,2 2,2
void map::saven(const tripoint& om_pos, unsigned int turn, const point& world, int gridx, int gridy)
{
 const int n = gridx + gridy * my_MAPSIZE;

 if (grid[n]->ter[0][0] == t_null) return;
 int abs_x = om_pos.x * OMAPX * 2 + world.x + gridx,
     abs_y = om_pos.y * OMAPY * 2 + world.y + gridy;

 MAPBUFFER.add_submap(abs_x, abs_y, om_pos.z, grid[n]);
}

// worldx & worldy specify where in the world this is;
// gridx & gridy specify which nonant:
// 0,0  1,0  2,0
// 0,1  1,1  2,1
// 0,2  1,2  2,2 etc
bool map::loadn(game *g, const point& world, int gridx, int gridy)
{
 int absx = g->cur_om.pos.x * OMAPX * 2 + world.x + gridx,
     absy = g->cur_om.pos.y * OMAPY * 2 + world.y + gridy,
     gridn = gridx + gridy * my_MAPSIZE;
 if (submap * const tmpsub = MAPBUFFER.lookup_submap(absx, absy, g->cur_om.pos.z)) {
  grid[gridn] = tmpsub;
 } else { // It doesn't exist; we must generate it!
  map tmp_map;
// overx, overy is where in the overmap we need to pull data from
// Each overmap square is two nonants; to prevent overlap, generate only at
//  squares divisible by 2.
  int newmapx = world.x + gridx;
  int newmapy = world.y + gridy;
  if (newmapx % 2) newmapx--;
  if (newmapy % 2) newmapy--;
  tmp_map.generate(g, &(g->cur_om), newmapx, newmapy);
  return false;
 }
 return true;
}

bool map::loadn(const tripoint& GPS, int gridx, int gridy)
{
    const int gridn = gridx + gridy * my_MAPSIZE;
    if (submap* const tmpsub = MAPBUFFER.lookup_submap(GPS.x+gridx, GPS.y + gridy, GPS.z)) {
        grid[gridn] = tmpsub;
    } else { // It doesn't exist; we must generate it!
        map tmp_map;
        // overx, overy is where in the overmap we need to pull data from
        // Each overmap square is two nonants; to prevent overlap, generate only at
        //  squares divisible by 2.
        GPS_loc target(tripoint(GPS.x + gridx, GPS.y + gridy, GPS.z), point(0, 0));
        OM_loc _target = overmap::toOvermap(target);
        auto g = game::active();

        tmp_map.generate(g, &om_cache::get().create(_target.first), _target.second.x * 2, _target.second.y * 2);
        return false;
    }
    return true;
}

void map::copy_grid(int to, int from)
{
 grid[to] = grid[from];
}

void map::spawn_monsters(game *g)
{
 point scan;

 for (scan.x = 0; scan.x < my_MAPSIZE; scan.x++) {
  for (scan.y = 0; scan.y < my_MAPSIZE; scan.y++) {
   int n = scan.x + scan.y * my_MAPSIZE;
   for (const auto& spawn_pt : grid[n]->spawns) {
    for (int j = 0; j < spawn_pt.count; j++) {
     monster tmp(mtype::types[spawn_pt.type]);
     tmp.spawnmap.x = g->lev.x + scan.x;
     tmp.spawnmap.y = g->lev.y + scan.y;
     tmp.faction_id = spawn_pt.faction_id;
     tmp.mission_id = spawn_pt.mission_id;
     if (!spawn_pt._name.empty()) tmp.unique_name = spawn_pt._name;
     if (spawn_pt.friendly) tmp.friendly = -1;

     std::function<point()> nominate_spawn_pos = [&]() {
         point m = (spawn_pt.pos + rng(within_rldist<3>)) % SEE;
         if (m.x < 0) m.x += SEE;
         if (m.y < 0) m.y += SEE;
         return m + scan * SEE;
     };

     std::function<bool(const point&)> spawn_ok = [&](const point& pt) { return g->is_empty(pt) && tmp.can_move_to(g->m, pt); };

     if (decltype(auto) dest = LasVegasChoice(10, nominate_spawn_pos, spawn_ok)) {
         tmp.spawnpos = *dest;
         tmp.spawn(*dest);
         g->z.push_back(std::move(tmp));
     }
    }
   }
   grid[n]->spawns.clear();
  }
 }
}

void map::clear_spawns()
{
    for (submap* const gr : grid) gr->spawns.clear();
}

void map::clear_traps()
{
    for (submap* const gr : grid) memset(gr->trp, 0, sizeof(gr->trp)); // tr_null defined as 0
}


bool map::inbounds(int x, int y) const
{
 return (x >= 0 && x < SEEX * my_MAPSIZE && y >= 0 && y < SEEY * my_MAPSIZE);
}
