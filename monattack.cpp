#include "monattack.h"
#include "mondeath.h"
#include "monster.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "monattack_spores.hpp"
#include "bodypart.h"
#include "recent_msg.h"
#include "stl_limits.h"
#include "stl_typetraits_late.h"
#include "inline_stack.hpp"
#include "Zaimoni.STL/GDI/box.hpp"
#include "fragment.inc/rng_box.hpp"

void mattack::antqueen(monster& z)
{
 const auto g = game::active();
 std::vector<std::pair<GPS_loc, int> > egg_points;
 std::vector<monster*> ants;
 z.sp_timeout = z.type->sp_freq;	// Reset timer

 static auto survey = [&](const point& delta) {
     auto dest = z.GPSpos + delta;
     if (const auto _mon = g->mon(dest)) {
         if ((mon_ant_larva == _mon->type->id || mon_ant == _mon->type->id)) ants.push_back(_mon);
         return; // not empty so egg hatching check will fail
     }
     if (!dest.is_empty()) return;	// is_empty() because we can't hatch an ant under the player, a monster, etc.
     int i = -1;
     for (auto& obj : dest.items_at()) {
         ++i;
         if (itm_ant_egg == obj.type->id) {
             egg_points.push_back(std::pair(dest, i));
             break;	// Done looking at this tile
         }
     }
 };

// Count up all adjacent tiles the contain at least one egg.
 forall_do_inclusive(within_rldist<2>, survey);

 if (!ants.empty()) {
  z.moves -= mobile::mp_turn; // It takes a while
  monster *const ant = ants[rng(0, ants.size() - 1)];
  if (g->u.see(z) && g->u.see(*ant)) messages.add("The %s feeds an %s and it grows!", z.name().c_str(), ant->name().c_str());
  ant->poly(mtype::types[ant->type->id == mon_ant_larva ? mon_ant : mon_ant_soldier]);
 } else if (egg_points.empty()) {	// There's no eggs nearby--lay one.
  if (g->u.see(z)) messages.add("The %s lays an egg!", z.name().c_str());
  z.GPSpos.add(item(item::types[itm_ant_egg], messages.turn));
 } else { // There are eggs nearby.  Let's hatch some.
  z.moves -= (mobile::mp_turn/5) * egg_points.size(); // It takes a while
  if (g->u.see(z)) messages.add("The %s tends nearby eggs, and they hatch!", z.name().c_str());
  for(decltype(auto) egg_point : egg_points) {
     EraseAt(egg_point.first.items_at(), egg_point.second);
     g->spawn(monster(mtype::types[mon_ant_larva], egg_point.first));
  }
 }
}

void mattack::shriek(game *g, monster *z)
{
 if (rl_dist(z->GPSpos, g->u.GPSpos) > 4 || !z->see(g->u)) return;	// Out of range
 z->moves -= (mobile::mp_turn/5)*12; // It takes a while
 z->sp_timeout = z->type->sp_freq; // Reset timer
 g->sound(z->pos, 50, "a terrible shriek!");
}

void mattack::acid(game *g, monster *z)
{
 if (rl_dist(z->GPSpos, g->u.GPSpos) > 10) return; // Out of range
 const auto j = z->see(g->u);
 if (!j) return; // Unseen
 auto hit(g->u.GPSpos + rng(within_rldist<2>));
 decltype(auto) LoF = z->GPSpos.sees(hit, 12);
 if (!LoF) return;  // deviation has no path at all

 z->moves -= 3*mobile::mp_turn; // It takes a while
 z->sp_timeout = z->type->sp_freq; // Reset timer
 z->GPSpos.sound(4, "a spitting noise.");

 for (decltype(auto) pt : *LoF) {
     ter_id terrain = pt.ter();
     if (pt.hit_with_acid()) {
         if (g->u.see(pt)) messages.add("A glob of acid hits the %s!", name_of(terrain).c_str());
         return;
     }
 }
 for (int i = -3; i <= 3; i++) {
    for (int j = -3; j <= 3; j++) {
        auto dest(hit + point(i, j));
        if (0 < dest.move_cost() && dest.can_see(hit, 6) && one_in(abs(j)) && one_in(abs(i))) {
	        auto& f = dest.field_at();
            if (f.type == fd_acid && f.density < 3) f.density++;
            else dest.add(field(fd_acid, 2));
        }
    }
 }
}

void mattack::shockstorm(game *g, monster *z)
{
 if (rl_dist(z->GPSpos, g->u.GPSpos) > 12 || !z->see(g->u)) return; // Can't see you, no attack
 z->moves -= mobile::mp_turn / 2; // It takes a while
 z->sp_timeout = z->type->sp_freq; // Reset timer
 messages.add("A bolt of electricity arcs towards you!");

 // 2/9 near-miss for each of x, y axis; theoretical probability for hit if not obstructed 49/81
 const auto tar = g->u.pos + rng(within_rldist<1>) + rng(within_rldist<1>);

 const auto bolt = line_to(z->pos, tar, g->m.sees(z->pos, tar, -1));
 for (decltype(auto) pt : bolt) { // Fill the LOS with electricity
     if (!one_in(4)) g->m.add_field(g, pt, fd_electricity, rng(1, 3));
 }

 // 3x3 cloud of electricity at the square hit
 for (int i = tar.x - 1; i <= tar.x + 1; i++) {
  for (int j = tar.y - 1; j <= tar.y + 1; j++) {
   if (!one_in(6)) g->m.add_field(g, i, j, fd_electricity, rng(1, 3));
  }
 }
}

void mattack::boomer(game *g, monster *z)
{
 if (rl_dist(z->GPSpos, g->u.GPSpos) > 3) return;	// Out of range
 const auto j = z->see(g->u);
 if (!j) return; // Unseen
 z->sp_timeout = z->type->sp_freq; // Reset timer
 z->moves -= (mobile::mp_turn/2)*5; // It takes a while
 const bool u_see = (bool)g->u.see(z->pos);
 if (u_see)
     messages.add("The %s spews bile!",
                  grammar::capitalize(z->desc(grammar::noun::role::subject, grammar::article::definite)).c_str());
 for (const auto& pt : line_to(z->pos, g->u.pos, *j)) {
  auto& fd = g->m.field_at(pt);
  if (fd.type == fd_blood) {
   fd.type = fd_bile;
   fd.density = 1;
  } else if (fd.type == fd_bile) {
      if (fd.density < 3) fd.density++;
  } else
      g->m.add_field(g, pt, fd_bile, 1);

  // If bile hit a solid tile, return.
  if (0 == g->m.move_cost(pt)) {
      g->m.add_field(g, pt, fd_bile, 3);
      if (g->u.see(pt))
          messages.add("Bile splatters on the %s!", name_of(g->m.ter(pt)).c_str());
      return;
  }
 }

 const int my_dodge = g->u.dodge();
 if (rng(0, 10) > my_dodge || one_in(my_dodge))
  g->u.infect(DI_BOOMERED, bp_eyes, 3, 12);
 else if (u_see)
  messages.add("You dodge it!");
}

void mattack::resurrect(game *g, monster *z)
{
 if (z->speed < z->type->speed / 2) return;	// We can only resurrect so many times!

// Find all corpses that we can see within 4 tiles.
 static std::function<std::optional<point>(point)> ok = [&](point pt) {
     auto pos(pt + z->pos);
     if (!g->is_empty(pos) || !g->m.sees(z->pos, pos, -1)) return std::optional<point>();
     for (auto& obj : g->m.i_at(pos)) {
         if (obj.type->id == itm_corpse && obj.corpse->species == species_zombie) return std::optional<point>(pos);
     }
     return std::optional<point>();
 };

 // Find all corpses that we can see within 4 tiles.
 const auto corpse_locations = grep(within_rldist<4>, ok);
 if (corpse_locations.empty()) return;	// No nearby corpses

 z->speed = cataclysm::rational_scaled<4, 5>(z->speed - rng(0, 10));
 const bool sees_necromancer = g->u.see(*z);
 if (sees_necromancer)
     messages.add("The %s throws its arms wide...",
                  grammar::capitalize(z->desc(grammar::noun::role::subject, grammar::article::definite)).c_str());
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 z->moves -= 5 * mobile::mp_turn; // It takes a while
 int raised = 0;
 for (decltype(auto) pt : corpse_locations) {
     decltype(auto) stack = g->m.i_at(pt);
     int n = -1;
     for (auto& obj : stack) {
         ++n;
         if (obj.type->id == itm_corpse && obj.corpse->species == species_zombie && one_in(2)) {
             if (g->u.see(pt)) raised++;
             int burnt_penalty = obj.burnt;
             monster mon(obj.corpse, pt);
             mon.speed = cataclysm::rational_scaled<4, 5>(mon.speed) - burnt_penalty / 2;
             mon.hp = cataclysm::rational_scaled<7, 10>(mon.hp) - burnt_penalty;
             EraseAt(stack, n); // invalidates iterator
             g->spawn(std::move(mon));
             break;
         }
     }
 }

 if (raised > 0) {
  if (raised == 1) messages.add("A nearby corpse rises from the dead!");
  else if (raised < 4) messages.add("A few corpses rise from the dead!");
  else messages.add("Several corpses rise from the dead!");
 } else if (sees_necromancer)
  messages.add("...but nothing seems to happen.");
}

void mattack::science(game *g, monster *z)	// I said SCIENCE again!
{
 const int dist = rl_dist(z->GPSpos, g->u.GPSpos);
 if (dist > 5) return;	// Out of range
 const auto t = z->see(g->u);
 if (!t) return; // Unseen
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 std::vector<point> line = line_to(z->pos, g->u.pos, *t);
 std::vector<point> free;
 for (decltype(auto) delta : Direction::vector) {
   if (const point pt(z->pos + delta); g->is_empty(pt)) free.push_back(pt);
 }
 std::vector<int> valid;// List of available attacks
 if (dist == 1) valid.push_back(1);	// Shock
 if (dist <= 2) valid.push_back(2);	// Radiation
 if (!free.empty()) {   // \todo these two arguably should use "inventory"
  valid.push_back(3);	// Manhack
  valid.push_back(4);	// Acid pool
 }
 valid.push_back(5);	// Flavor text
 switch (valid[rng(0, valid.size() - 1)]) {	// What kind of attack?
 case 1:	// Shock the player
  messages.add("The %s shocks you!", z->name().c_str());
  z->moves -= (mobile::mp_turn / 2) * 3;
  g->u.hurtall(rng(1, 2));
  if (one_in(6) && !one_in(30 - g->u.str_cur)) {
   messages.add("You're paralyzed!");
   g->u.moves -= 3 * mobile::mp_turn;
  }
  break;
 case 2:	// Radioactive beam
  messages.add("The %s opens its mouth and a beam shoots towards you!", z->name().c_str());
  z->moves -= 4 * mobile::mp_turn;
  if (g->u.dodge() > rng(1, 16))
   messages.add("You dodge the beam!");
  else if (one_in(6))
   g->u.mutate();
  else {
   messages.add("You get pins and needles all over.");
   g->u.radiation += rng(20, 50);
  }
  break;
 case 3:	// Spawn a manhack
  messages.add("The %s opens its coat, and a manhack flies out!", z->name().c_str());
  z->moves -= 2 * mobile::mp_turn;
  g->spawn(monster(mtype::types[mon_manhack], free[rng(0, valid.size() - 1)]));
  break;
 case 4:	// Acid pool
  messages.add("The %s drops a flask of acid!", z->name().c_str());
  z->moves -= mobile::mp_turn;
  for(const auto& pt : free) g->m.add_field(g, pt, fd_acid, 3);
  break;
 case 5:	// Flavor text
  {
  static constexpr const char* const no_op[] = {
    "The %s gesticulates wildly!",
    "The %s coughs up a strange dust.",
    "The %s moans softly.",
    "The %s's skin crackles with electricity." // extra processing, but still mostly harmless
  };

  const auto index = rng(0, std::end(no_op) - std::begin(no_op) - 1);
  messages.add(no_op[index], z->name().c_str());
  if (3 == index) {
      static_assert(3 == std::end(no_op) - std::begin(no_op) - 1);  // \todo need a policy decision when adding to no-op messages
      z->moves -= (mobile::mp_turn / 5) * 4;
  }
  }
  break;
 }
}

struct grow_tree
{
    game* const g;
    const monster& z;

    grow_tree(const monster& z) noexcept : g(game::active()), z(z) {}
    grow_tree(const grow_tree&) = delete;
    grow_tree(grow_tree&&) = delete;
    grow_tree& operator=(const grow_tree&) = delete;
    grow_tree& operator=(grow_tree&&) = delete;
    ~grow_tree() = default;

    void operator()(monster* target) {
        if (g->u.see(*target)) messages.add("A tree bursts forth from the earth and pierces the %s!", target->name().c_str());
        int rn = rng(10, 30) - target->armor_cut();
        if (rn < 0) rn = 0;
        if (target->hurt(rn)) g->kill_mon(*target, &z);
    }
    void operator()(npc* target) {
        body_part hit = bp_legs;
        int side = rng(1, 2);
        if (one_in(4)) hit = bp_torso;
        else if (one_in(2)) hit = bp_feet;
        if (g->u.see(*target))
            messages.add("A tree bursts forth from the earth and pierces %s's %s!", target->name.c_str(), body_part_name(hit, side));
        target->hit(g, hit, side, 0, rng(10, 30));
    }
    void operator()(pc* target) {
        body_part hit = bp_legs;
        int side = rng(1, 2);
        if (one_in(4)) hit = bp_torso;
        else if (one_in(2)) hit = bp_feet;
        messages.add("A tree bursts forth from the earth and pierces your %s!", body_part_name(hit, side));
        target->hit(g, hit, side, 0, rng(10, 30));
    }
};

struct grow_underbrush
{
    game* const g;
    const monster& z;

    grow_underbrush(const monster& z) noexcept : g(game::active()), z(z) {}
    grow_underbrush(const grow_underbrush&) = delete;
    grow_underbrush(grow_underbrush&&) = delete;
    grow_underbrush& operator=(const grow_underbrush&) = delete;
    grow_underbrush& operator=(grow_underbrush&&) = delete;
    ~grow_underbrush() = default;

    void operator()(monster* target) {
        if (g->u.see(*target)) messages.add("Underbrush grows, and it pierces the %s!", target->name().c_str());
        int rn = rng(10, 30) - target->armor_cut();
        if (rn < 0) rn = 0;
        if (target->hurt(rn)) g->kill_mon(*target, &z);
    }
    void operator()(npc* target) {
        body_part hit = bp_legs;
        int side = rng(1, 2);
        if (one_in(4)) hit = bp_torso;
        else if (one_in(2)) hit = bp_feet;
        if (g->u.see(*target))
            messages.add("Underbrush grows, and it pierces %s's %s!", target->name.c_str(), body_part_name(hit, side));
        target->hit(g, hit, side, 0, rng(10, 30));
    }
    void operator()(pc* target) {
        body_part hit = bp_legs;
        int side = rng(1, 2);
        if (one_in(4)) hit = bp_torso;
        else if (one_in(2)) hit = bp_feet;
        messages.add("The underbrush beneath your feet grows and pierces your %s!", body_part_name(hit, side));
        target->hit(g, hit, side, 0, rng(10, 30));
    }
};

void mattack::growplants(game *g, monster *z)
{
 for (int i = -3; i <= 3; i++) {
  for (int j = -3; j <= 3; j++) {
   if (i == 0 && j == 0) j++;
   point dest(z->pos.x + i, z->pos.y + j);
   auto& t = g->m.ter(dest);
   if (!g->m.has_flag(diggable, dest) && one_in(4))
    t = t_dirt;
   else if (one_in(3) && g->m.is_destructable(dest))
    t = t_dirtmound; // Destroy walls, &c
   else if (one_in(4)) {	// 1 in 4 chance to grow a tree
     if (auto _mob = g->mob_at(dest)) std::visit(grow_tree(*z), *_mob);
	 t = t_tree_young;
    } else if (one_in(3)) // If no tree, perhaps underbrush
     t = t_underbrush;
  }
 }

 if (one_in(5)) { // 1 in 5 chance of making existing vegetation grow larger
  for (int i = -5; i <= 5; i++) {
   for (int j = -5; j <= 5; j++) {
	if (0 == i && 0 == j) j++;
	point dest(z->pos.x + i, z->pos.y + j);
	auto& t = g->m.ter(dest);
     if (t_tree_young == t) t = t_tree; // Young tree => tree
     else if (t_underbrush == t) { // Underbrush => young tree
         if (auto _mob = g->mob_at(dest)) std::visit(grow_underbrush(*z), *_mob);
         else t = t_tree_young;
     }
   }
  }
 }
}

void mattack::grow_vine(monster& z)
{
 z.sp_timeout = z.type->sp_freq;
 z.moves -= mobile::mp_turn;

 point shift = rng(within_rldist<1>);

 static auto grow = [&](const point& delta) {
     auto delta2(shift + delta);

     if (-1 > delta2.x) delta2.x += 3;
     else if (1 < delta2.x) delta2.x -= 3;
     if (-1 > delta2.y) delta2.y += 3;
     else if (1 < delta2.y) delta2.y -= 3;

     auto dest = z.GPSpos + delta2;
     if (dest.is_empty()) {
         monster vine(mtype::types[mon_creeper_vine], dest);
         vine.sp_timeout = 5;
         game::active()->spawn(std::move(vine));
     }
 };

 forall_do_inclusive(within_rldist<1>, grow);
}

struct vine_attack {
    monster& z;
    game* g;
    int neighbors;
    bool hit_anyone;

    vine_attack(monster& z) noexcept : z(z), g(game::active()), neighbors(0), hit_anyone(false) {}
    vine_attack(const vine_attack& src) = delete;
    vine_attack(vine_attack&& src) = delete;
    vine_attack& operator=(const vine_attack& src) = delete;
    vine_attack& operator=(vine_attack&& src) = delete;
    ~vine_attack() = default;

    // XXX no attempt to account for enemy status
    void operator()(player* target) {
        body_part bphit = random_body_part();
        int side = rng(0, 1);
        auto msg = grammar::SVO_sentence(z, "lash", body_part_name(bphit, side), "!");
        target->if_visible_message(msg.c_str());
        target->hit(g, bphit, side, 4, 4);
        hit_anyone = true;
    }

    void operator()(monster* target) {
        if (mon_creeper_vine == target->type->id) neighbors++;
    }
};

void mattack::vine(monster& z)
{
 const auto g = game::active();
 std::vector<GPS_loc> grow;
 int vine_neighbors = 0;
 z.sp_timeout = z.type->sp_freq;
 z.moves -= mobile::mp_turn;

 vine_attack attack(z);
 // Yes, we want to count ourselves as a neighbor.
 static auto hit_by_vine = [&](const point& delta) {
     auto dest = z.GPSpos + delta;
     if (auto _mob = g->mob_at(dest)) std::visit(attack, *_mob);
     else if (dest.is_empty()) grow.push_back(dest);
 };

 // C:Z changes:
 // * can attack more than one PC/NPC (and can attack NPCs at all)
 // * do not double-reset the special attack timeout
 forall_do_inclusive(within_rldist<1>, hit_by_vine);
 // see if we want to grow
 if (attack.hit_anyone) return;
 if (grow.empty() || vine_neighbors > 5 || one_in(7 - vine_neighbors)) return;

// Calculate distance from nearest hub, then check against that
 int dist_from_hub = INT_MAX;
 g->forall_do([&, gps=z.GPSpos](const monster& v) {
     if (mon_creeper_hub == v.type->id) clamp_ub(dist_from_hub, rl_dist(gps, v.GPSpos));
 });
 if (!one_in(dist_from_hub)) return;

 monster vine(mtype::types[mon_creeper_vine], grow[rng(0, grow.size() - 1)]);
 vine.sp_timeout = 5;
 g->spawn(std::move(vine));
}

static int route_sap(const std::vector<point>& line)
{
    auto g = game::active();
    int dam = 5;
    for (decltype(auto) pt : line) {
        g->m.shoot(g, pt, dam, false, 0);
        if (0 == dam) {
            if (g->u.see(pt)) messages.add("A glob of sap hits the %s!", name_of(g->m.ter(pt)).c_str());
            g->m.add_field(g, pt, fd_sap, (dam >= 4 ? 3 : 2));
            return 0;
        }
        // \todo maybe we should hit monsters/NPCs/players here?
    }
    return dam;
}

void mattack::spit_sap(game *g, monster *z)
{
// TODO: Friendly biollantes?
 const int dist = rl_dist(z->GPSpos, g->u.GPSpos);
 if (dist > 12 || !z->see(g->u)) return;

 z->moves -= (mobile::mp_turn / 2) * 3;
 z->sp_timeout = z->type->sp_freq;

 int deviation = rng(1, 10);
 double missed_by = (.0325 * deviation * dist); // cf. calculate_missed_by/ranged.cpp

 if (g->u.see(z->pos)) {
     const auto z_name = grammar::capitalize(z->desc(grammar::noun::role::subject, grammar::article::definite));
     if (missed_by > 1.) messages.add("%s spits sap, but misses you.", z_name.c_str());
     else messages.add("%s spits sap!", z_name.c_str());
 }

 if (missed_by > 1.) {
  zaimoni::gdi::box<point> error(point(-missed_by), point(missed_by));
  point hit = g->u.pos + rng(error);

  if (int dam = route_sap(line_to(z->pos, hit, 0))) g->m.add_field(g, hit, fd_sap, (dam >= 4 ? 3 : 2));
  return;
 }

 if (int dam = route_sap(line_to(z->pos, g->u.pos, 0))) {
     messages.add("A glob of sap hits you!");
     g->u.hit(g, bp_torso, 0, dam, 0);
     g->u.add_disease(DI_SAP, dam);
 }
}

void mattack::triffid_heartbeat(game *g, monster *z)
{
 g->sound(z->pos, 14, "thu-THUMP.");
 z->moves -= 3 * mobile::mp_turn;
 z->sp_timeout = z->type->sp_freq;
 if (!map::in_bounds(z->pos)) return;
 if (rl_dist(z->GPSpos, g->u.GPSpos) > 5 && !g->m.route(g->u.pos, z->pos).empty()) {
  messages.add("The root walls creak around you.");

  // overmap generation is as follows:
  // ground level: triffid grove
  // -1 z: triffid roots
  // -2 z: triffid finale.
  // i.e., this is happening underground; we are immobile and have a hard-coded relative location of 21,21
  // and mapgen gives a hard guarantee the player is approaching from the northwest
  const zaimoni::gdi::box<point> span(g->u.pos, z->pos + 3*Direction::NW);

  static auto grow_wall = [&](const point& dest) {
      auto& t = g->m.ter(dest);
      if (g->is_empty(dest) && one_in(4)) t = t_root_wall;
      else if (t_root_wall == t && one_in(10)) t = t_dirt;
  };

  forall_do_inclusive(span, grow_wall);

// Open blank tiles as long as there's no possible route
  int tries = 0;
  while (g->m.route(g->u.pos, z->pos).empty() && tries < 20) {
   const point pt = rng(span);
   tries++;
   g->m.ter(pt) = t_dirt;
   if (rl_dist(pt, g->u.pos) > 3 && 30 > g->mon_count() &&
       !g->mon(pt) && one_in(20)) { // Spawn an extra monster
    mon_id montype = mon_triffid;
    if (one_in(4)) montype = mon_creeper_hub;
    else if (one_in(3)) montype = mon_biollante;
    g->spawn(monster(mtype::types[montype], pt));
   }
  }
 } else { // The player is close enough for a fight!
  monster triffid(mtype::types[mon_triffid]);
  for (decltype(auto) delta : Direction::vector) {
      if (const auto loc(z->GPSpos + delta); loc.is_empty() && one_in(2)) {
          triffid.spawn(loc);
          g->spawn(triffid);
      }
  }
 }
}

void mattack::fungus(game *g, monster *z)
{
 if (100 < g->mon_count()) return; // Prevent crowding the monster list.
 z->moves -= 2 * mobile::mp_turn; // It takes a while
 z->sp_timeout = z->type->sp_freq; // Reset timer

 int moncount = 0;
 int blocked = 0;
 z->GPSpos.sound(10, "Pouf!");
 if (g->u.see(*z)) messages.add("Spores are released from the %s!", z->name().c_str());
 for (decltype(auto) dir : Direction::vector) {
     const auto dest(z->GPSpos + dir);
     if (0 < dest.move_cost()) {
         if (one_in(5)) {
             if (spray_spores(dest, z)) ++moncount;
         }
     } else ++blocked; // C:Z : if the tile is *blocked*, it might as well have a monster for purposes of dormancy
 }
 if (7 <= moncount) z->poly(mtype::types[mon_fungaloid_dormant]);	// If we're surrounded by monsters, go dormant
 else if (8 <= moncount+blocked) z->poly(mtype::types[mon_fungaloid_dormant]);
}

struct fungus_displace {
    fungus_displace() = default;
    fungus_displace(const fungus_displace&) = delete;
    fungus_displace(fungus_displace&&) = delete;
    fungus_displace& operator=(const fungus_displace&) = delete;
    fungus_displace& operator=(fungus_displace&&) = delete;
    ~fungus_displace() = default;

    void operator()(monster* target) {} // C:Whales: no-op
    void operator()(npc* target) {
        const auto g = game::active();
        if (g->u.see(*target)) messages.add("% is shoved away as a fungal wall grows!", target->name.c_str());
        g->teleport(target);
    }
    void operator()(pc* target) {
        messages.add("You're shoved away as a fungal wall grows!");
        game::active()->teleport();
    }
};

void mattack::fungus_sprout(monster& z)
{
    const auto g = game::active();
    for (decltype(auto) delta : Direction::vector) {
        auto loc(z.GPSpos + delta);
        if (auto _mob = g->mob_at(loc)) std::visit(fungus_displace(), *_mob);
        if (loc.is_empty()) g->spawn(monster(mtype::types[mon_fungal_wall], loc));
    }
}

void mattack::leap(game *g, monster *z)
{
 if (!z->see(g->u)) return;	// Only leap if we can see you!

 std::vector<GPS_loc> options;
 const bool fleeing = z->is_fleeing(g->u);
 int best = fleeing ? INT_MIN : INT_MAX;

 static auto jump_to = [&](const point delta) {
     auto dest = z->GPSpos + delta;
     if (0 >= dest.move_cost()) return;
     if (g->mob_at(dest)) return;  // these two simulate is_empty(point)
     if (!z->GPSpos.can_see(dest, game::light_level(dest))) return;
     const int dist = rl_dist(g->u.GPSpos, dest);
     if (fleeing ? dist >= best : dist <= best) {
         if (dist != best) {
             options.clear();
             best = dist;
         }
         options.push_back(dest);
     }
 };

 forall_do_inclusive(within_rldist<3>, jump_to);
 if (options.empty()) return; // Nowhere to leap!

 z->moves -= (mobile::mp_turn / 2) * 3;
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 bool seen = g->u.see(*z); // We can see them jump...
 z->set_screenpos(options[rng(0, options.size() - 1)]);
 seen |= g->u.see(*z); // ... or we can see them land
 if (seen)
     messages.add("%s leaps!",
                  grammar::capitalize(z->desc(grammar::noun::role::subject, grammar::article::definite)).c_str());
}

void mattack::dermatik(game *g, monster *z)
{
 if (rl_dist(z->GPSpos, g->u.GPSpos) > 1 || g->u.has_disease(DI_DERMATIK))
  return; // Too far to implant, or the player's already incubating bugs

 z->sp_timeout = z->type->sp_freq;	// Reset timer

// Can we dodge the attack?
 int attack_roll = dice(z->melee_skill(), 10);
 if (g->u.dodge_roll() > attack_roll) {
  messages.add("The %s tries to land on you, but you dodge.", z->name().c_str());
  z->stumble(g);
  return;
 }

// Can we swat the bug away?
 int dodge_roll = z->dodge_roll();
 int swat_skill = (g->u.sklevel[sk_melee] + g->u.sklevel[sk_unarmed] * 2) / 3;
 int player_swat = dice(swat_skill, 10);
 if (player_swat > dodge_roll) {
  messages.add("The %s lands on you, but you swat it off.", z->name().c_str());
  if (z->hp >= z->type->hp / 2) z->hurt(1);
  if (player_swat > dodge_roll * 1.5) z->stumble(g);
  return;
 }

// Can the bug penetrate our armor?
 body_part targeted = bp_head;
 if (!one_in(4))
  targeted = bp_torso;
 else if (one_in(2))
  targeted = bp_legs;
 else if (one_in(5))
  targeted = bp_hands;
 else if (one_in(5))
  targeted = bp_feet;
 if (g->u.armor_cut(targeted) >= 2) {
  messages.add("The %s lands on your %s, but can't penetrate your armor.",
             z->name().c_str(), body_part_name(targeted, rng(0, 1)));
  z->moves -= (mobile::mp_turn / 2) * 3; // Attempted laying takes a while
  return;
 }

// Success!
 z->moves -= 5 * mobile::mp_turn; // Successful laying takes a long time
 messages.add("The %s sinks its ovipositor into you!", z->name().c_str());
 g->u.add_disease(DI_DERMATIK, -1); // -1 = infinite
}

void mattack::plant(game *g, monster *z)
{
// Spores taking seed and growing into a fungaloid
 if (g->m.has_flag(diggable, z->pos)) {
  if (g->u.see(z->pos)) messages.add("The %s takes seed and becomes a young fungaloid!", z->name().c_str());
  z->poly(mtype::types[mon_fungaloid_young]);
  z->moves -= 10 * mobile::mp_turn;	// It takes a while
 }
}

void mattack::disappear(monster& z) { z.hp = 0; }

struct blob_coat {
    monster& z;

    blob_coat(monster& z) noexcept : z(z) {}
    blob_coat(const blob_coat& src) = delete;
    blob_coat(blob_coat&& src) = delete;
    blob_coat& operator=(const blob_coat& src) = delete;
    blob_coat& operator=(blob_coat&& src) = delete;
    ~blob_coat() = default;

    bool operator()(monster* target) {
        return target->hit_by_blob(&z, rng(0, z.hp) > rng(0, target->hp));
    };

    bool operator()(player* target) {
        target->add_disease(DI_SLIMED, rng(0, z.hp));
        return true;
    };
};

void mattack::formblob(game *g, monster *z)
{
 for (decltype(auto) dir : Direction::vector) {
     bool didit = false;
     auto loc = z->GPSpos + dir;
     if (auto mob = g->mob_at(loc)) {
         didit = std::visit(blob_coat(*z), *mob);
     } else if (z->speed >= 85 && rng(0, 250) < z->speed) {
         // If we're big enough, spawn a baby blob.
         didit = true;
         z->speed -= 3 * (mobile::mp_turn / 20);
         monster blob(mtype::types[mon_blob_small], loc);
         blob.speed = z->speed - rng(30, 60);
         blob.hp = blob.speed;
         g->spawn(std::move(blob));
     }
     if (didit) {	// We did SOMEthing.
         if (z->type->id == mon_blob && z->speed <= 50) z->poly(mtype::types[mon_blob_small]);	// We shrank!
         z->moves -= 5 * mobile::mp_turn;
         z->sp_timeout = z->type->sp_freq;	// Reset timer
         return;
     }
 }
}

void mattack::dogthing(game *g, monster *z)	// XXX only happens when PC can see it?
{
 if (!g->u.see(*z) || !one_in(3)) return;

 messages.add("The %s's head explodes in a mass of roiling tentacles!",
               grammar::capitalize(z->desc(grammar::noun::role::possessive, grammar::article::definite)).c_str());

 for (int x = z->pos.x - 2; x <= z->pos.x + 2; x++) {
  for (int y = z->pos.y - 2; y <= z->pos.y + 2; y++) {
   if (rng(0, 2) >= rl_dist(z->pos, x, y))
    g->m.add_field(g, x, y, fd_blood, 2);
  }
 }

 z->make_threat(g->u);
 z->poly(mtype::types[mon_headless_dog_thing]);
}

void mattack::tentacle(game *g, monster *z)
{
 const auto t = z->see(g->u);
 if (!t) return;

 messages.add("%s lashes its tentacle at you!",
               grammar::capitalize(z->desc(grammar::noun::role::subject, grammar::article::definite)).c_str());
 z->moves -= mobile::mp_turn;
 z->sp_timeout = z->type->sp_freq;	// Reset timer

 for (const auto& pt : line_to(z->pos, g->u.pos, *t)) {
     int tmpdam = 20;
     g->m.shoot(g, pt, tmpdam, true, 0);
 }

 const int evasion = g->u.dodge();
 if (rng(0, 20) > evasion || one_in(evasion)) {
  messages.add("You dodge it!");
  return;
 }
 body_part hit = random_body_part();
 int dam = rng(10, 20), side = rng(0, 1);
 messages.add("Your %s is hit for %d damage!", body_part_name(hit, side), dam);
 g->u.hit(g, hit, side, dam, 0);
}

namespace {

    struct hit_by_item {
        item& obj;
        int dam;
        const monster* killed_by_mon;
        player* killed_by_player;

        // Waterfall/SSADM lifecycle for these constructors
        hit_by_item(item& obj, int dam) noexcept : obj(obj), dam(dam), killed_by_mon(nullptr), killed_by_player(nullptr) {};
        hit_by_item(item& obj, int dam, const monster* killed_by_mon) noexcept : obj(obj), dam(dam), killed_by_mon(killed_by_mon), killed_by_player(nullptr) {};
        hit_by_item(item& obj, int dam, player* killed_by_player) noexcept : obj(obj), dam(dam), killed_by_mon(nullptr), killed_by_player(killed_by_player) {};
        hit_by_item(const hit_by_item& src) = delete;
        hit_by_item(hit_by_item&& src) = delete;
        hit_by_item& operator=(const hit_by_item& src) = delete;
        hit_by_item& operator=(hit_by_item&& src) = delete;
        ~hit_by_item() = default;

        void operator()(monster* target) {
            if (target->hurt(dam)) {
                const auto g = game::active();
                if (killed_by_mon) g->kill_mon(*target, killed_by_mon);
                else if (killed_by_player) g->kill_mon(*target, killed_by_player);
                else g->kill_mon(*target);
            }
        }

        void operator()(player* target) {
            body_part hit = random_body_part();
            int side = rng(0, 1);
            static auto hit_by = [&]() {
                return std::string("A ") + obj.tname() + " hits " + target->possessive() + " " + body_part_name(hit, side) + " for " + std::to_string(dam) + " damage!";
            };
            target->if_visible_message(hit_by);
            target->hit(game::active(), hit, side, dam, 0);
        }
    };

}

struct thrown_by_vortex
{
    game* const g;
    const std::vector<point>& from_monster;
    const monster& z;

    thrown_by_vortex(const std::vector<point>& from_monster, const monster& z) noexcept : g(game::active()), from_monster(from_monster), z(z) {}
    thrown_by_vortex(const thrown_by_vortex& src) = delete;
    thrown_by_vortex(thrown_by_vortex&& src) = delete;
    thrown_by_vortex& operator=(const thrown_by_vortex& src) = delete;
    thrown_by_vortex& operator=(thrown_by_vortex&& src) = delete;
    ~thrown_by_vortex() = default;

    void operator()(monster* thrown) {
        static const int base_mon_throw_range[mtype::MS_MAX] = { 5, 3, 2, 1, 0 };

        int distance = base_mon_throw_range[thrown->type->size];
        int damage = distance * 4;
        switch (thrown->type->mat) {
        case LIQUID:  distance += 3; damage -= 10; break;
        case VEGGY:   distance += 1; damage -= 5; break;
        case POWDER:  distance += 4; damage -= 30; break;
        case COTTON:
        case WOOL:    distance += 5; damage -= 40; break;
        case LEATHER: distance -= 1; damage += 5; break;
        case KEVLAR:  distance -= 3; damage -= 20; break;
        case STONE:   distance -= 3; damage += 5; break;
        case PAPER:   distance += 6; damage -= 10; break;
        case WOOD:    distance += 1; damage += 5; break;
        case PLASTIC: distance += 1; damage += 5; break;
        case GLASS:   distance += 2; damage += 20; break;
        case IRON:    distance -= 1; [[fallthrough]];
        case STEEL:
        case SILVER:  distance -= 3; damage -= 10; break;
        }
        if (distance > 0) {
            {
            auto msg = grammar::capitalize(thrown->desc(grammar::noun::role::subject, grammar::article::definite)) + " is thrown by winds!";
            thrown->if_visible_message(msg.c_str());
            }
            std::vector<point> traj = continue_line(from_monster, distance);
            bool hit_wall = false;
            for (int i = 0; i < traj.size() && !hit_wall; i++) {
                monster* const m_hit = g->mon(traj[i]);
                if (i > 0 && is_not<MF_DIGS>(m_hit)) {
                    auto msg = grammar::SVO_sentence(*thrown, "hit", m_hit->desc(grammar::noun::role::direct_object, grammar::article::indefinite), "!");
                    if (g->u.see(traj[i])) messages.add(msg);
                    if (m_hit->hurt(damage)) g->kill_mon(*m_hit, &z);
                    hit_wall = true;
                    thrown->screenpos_set(traj[i - 1]);
                }
                else if (g->m.move_cost(traj[i]) == 0) {
                    hit_wall = true;
                    thrown->screenpos_set(traj[i - 1]);
                }
                int damage_copy = damage;
                g->m.shoot(g, traj[i], damage_copy, false, 0);
                if (damage_copy < damage) thrown->hurt(damage - damage_copy);
            }
            if (hit_wall) damage *= 2;
            else thrown->screenpos_set(traj.back());
            if (thrown->hurt(damage)) g->kill_mon(*thrown, &z);
        } // if (distance > 0)
    }

    void operator()(npc* target) {
        std::vector<point> traj = continue_line(from_monster, rng(2, 3));
        {
        auto msg = grammar::capitalize(target->desc(grammar::noun::role::subject, grammar::article::definite)) + " is thrown by winds!";
        target->if_visible_message(msg.c_str());
        }
        bool hit_wall = false;
        int damage = rng(5, 10);
        for (int i = 0; i < traj.size() && !hit_wall; i++) {
            monster* const m_hit = g->mon(traj[i]);
            if (i > 0 && is_not<MF_DIGS>(m_hit)) {
                auto msg = grammar::SVO_sentence(*target, "hit", m_hit->desc(grammar::noun::role::direct_object, grammar::article::indefinite), "!");
                if (g->u.see(traj[i])) messages.add(msg);
                if (m_hit->hurt(damage)) g->kill_mon(*m_hit, target); // We get the kill :)
                hit_wall = true;
                target->screenpos_set(traj[i - 1]);
            }
            else if (g->m.move_cost(traj[i]) == 0) {
                messages.add("You slam into a %s", name_of(g->m.ter(traj[i])).c_str());
                hit_wall = true;
                target->screenpos_set(traj[i - 1]);
            }
            int damage_copy = damage;
            g->m.shoot(g, traj[i], damage_copy, false, 0);
            if (damage_copy < damage) target->hit(g, bp_torso, 0, damage - damage_copy, 0);
        }

        if (hit_wall) damage *= 2;
        else target->screenpos_set(traj.back());

        target->hit(g, bp_torso, 0, damage, 0);
    }

    void operator()(pc* target) {
        std::vector<point> traj = continue_line(from_monster, rng(2, 3));
        messages.add("You're thrown by winds!");
        bool hit_wall = false;
        int damage = rng(5, 10);
        for (int i = 0; i < traj.size() && !hit_wall; i++) {
            monster* const m_hit = g->mon(traj[i]);
            if (i > 0 && is_not<MF_DIGS>(m_hit)) {
                messages.add("You hit a %s!", m_hit->name().c_str());
                if (m_hit->hurt(damage)) g->kill_mon(*m_hit, target); // We get the kill :)
                hit_wall = true;
                target->screenpos_set(traj[i - 1]);
            }
            else if (g->m.move_cost(traj[i]) == 0) {
                messages.add("You slam into a %s", name_of(g->m.ter(traj[i])).c_str());
                hit_wall = true;
                target->screenpos_set(traj[i - 1]);
            }
            int damage_copy = damage;
            g->m.shoot(g, traj[i], damage_copy, false, 0);
            if (damage_copy < damage) target->hit(g, bp_torso, 0, damage - damage_copy, 0);
        }

        if (hit_wall) damage *= 2;
        else target->screenpos_set(traj.back());

        target->hit(g, bp_torso, 0, damage, 0);
        g->update_map(target->pos.x, target->pos.y);
    }
};

void mattack::vortex(game *g, monster *z)
{
// Make sure that the player's butchering is interrupted!
 if (g->u.activity.type == ACT_BUTCHER && rl_dist(z->GPSpos, g->u.GPSpos) <= 2) {
  messages.add("The buffeting winds interrupt your butchering!");
  g->u.activity.type = ACT_NULL;
 }
// Moves are NOT used up by this attack, as it is "passive"
 z->sp_timeout = z->type->sp_freq;
// Before anything else, smash terrain!
 auto wind_damage = [&](point delta) {
     if (point(0) == delta) return;
     std::string sound;
     auto loc = z->GPSpos + delta;
     loc.bash(14, sound);
     loc.sound(8, sound);
 };

 forall_do_inclusive(within_rldist<2>, wind_damage);

 auto throwing = [&](point delta) {
     if (point(0) == delta) return;
     auto dest_loc = z->GPSpos + delta;
     auto dest = z->pos + delta;
     std::vector<point> from_monster = line_to(z->pos, dest, 0);
     decltype(auto) items = dest_loc.items_at();
     while (!items.empty()) {
         item thrown(std::move(items[0]));
         EraseAt(items, 0);
         int distance = 5 - (thrown.weight() / 15);
         if (distance > 0) {
             int dam = thrown.weight() / double(3 + double(thrown.volume() / 6));
             std::vector<point> traj = continue_line(from_monster, distance);
             for (int i = 0; i < traj.size() && dam > 0; i++) {
                 g->m.shoot(g, traj[i], dam, false, 0);
                 if (auto mob = g->mob_at(traj[i])) {
                     std::visit(hit_by_item(thrown, dam, z), *mob);
                     dam = 0;
                 };

                 if (g->m.move_cost(traj[i]) == 0) {
                     dam = 0;
                     i--;
                 }

                 if (dam == 0 || i == traj.size() - 1) {
                     g->m.hard_landing(traj[i], std::move(thrown));
                 }
             }
         } // Done throwing item
     } // Done getting items

     if (auto _mob = g->mob_at(dest_loc)) {
         std::visit(thrown_by_vortex(from_monster, *z), *_mob);
     }
 };

 forall_do_inclusive(within_rldist<2>, throwing);
}

void mattack::gene_sting(game *g, monster *z)
{
 if (rl_dist(z->GPSpos, g->u.GPSpos) > 7 || !z->see(g->u)) return;	// Not within range and/or sight

 z->moves -= (mobile::mp_turn/2)*3;
 z->sp_timeout = z->type->sp_freq;
 messages.add("%s shoots a dart into you!",
              grammar::capitalize(z->desc(grammar::noun::role::subject, grammar::article::definite)).c_str());
 g->u.mutate();
}

void mattack::stare(game *g, monster *z)
{
 z->moves -= 2*mobile::mp_turn;
 z->sp_timeout = z->type->sp_freq;
 if (z->see(g->u)) {
  messages.add("%s stares at you, and you shudder.",
                grammar::capitalize(z->desc(grammar::noun::role::subject, grammar::article::definite)).c_str());
  g->u.add_disease(DI_TELEGLOW, MINUTES(80));
 } else {
  messages.add("A piercing beam of light bursts forth!");
  std::vector<point> sight = line_to(z->pos, g->u.pos, 0);
  for (const point& view : sight) {
   auto& t = g->m.ter(view);
   if (any<t_reinforced_glass_v, t_reinforced_glass_h>(t)) break;
   if (g->m.is_destructable(view)) t = t_rubble;
  }
 }
}

void mattack::fear_paralyze(game *g, monster *z)
{
 if (g->u.see(z->pos)) {
  z->sp_timeout = z->type->sp_freq;	// Reset timer
  if (g->u.has_artifact_with(AEP_PSYSHIELD)) {
   messages.add("The %s probes your mind, but is rebuffed!", z->name().c_str());
  } else if (rng(1, 20) > g->u.int_cur) {
   messages.add("The terrifying visage of the %s paralyzes you.", z->name().c_str());
   g->u.moves -= mobile::mp_turn;
  } else
   messages.add("You manage to avoid staring at the horrendous %s.", z->name().c_str());
 }
}

void mattack::photograph(game *g, monster *z)
{
 if (z->faction_id == -1 ||
     rl_dist(z->GPSpos, g->u.GPSpos) > 6 ||
     !z->see(g->u))
  return;
 z->sp_timeout = z->type->sp_freq;
 z->moves -= (mobile::mp_turn/2)*3;
 messages.add("%s takes your picture!",
              grammar::capitalize(z->desc(grammar::noun::role::subject, grammar::article::definite)).c_str());
// TODO: Make the player known to the faction
 event::add(event(EVENT_ROBOT_ATTACK, int(messages.turn) + rng(15, 30), z->faction_id, g->lev.x, g->lev.y));
}

struct is_seen_by {
    const monster& z;

    is_seen_by(const monster& z) noexcept : z(z) {}
    is_seen_by(const is_seen_by& src) = delete;
    is_seen_by(is_seen_by&& src) = delete;
    is_seen_by& operator=(const is_seen_by& src) = delete;
    is_seen_by& operator=(is_seen_by&& src) = delete;
    ~is_seen_by() = default;

    auto operator()(const monster* target) const { return true; }   // stub \todo implement
        // note that Mi-go might be expected to have technological communication i.e. be "aware" of other mi-go without line of sight
    auto operator()(const player* target) const { return (bool)z.see(*target); }
};

static auto mobs_seen_by(const monster& z, const zaimoni::gdi::box<point>& scan)
{
    std::pair<std::vector<std::variant<monster*, npc*, pc*> >, std::vector<std::variant<monster*, npc*, pc*> > > ret;
    const auto g = game::active();

    static auto classify = [&](const point& pos) {
        if (point(0) == pos) return;
        auto loc = z.GPSpos + pos;
        if (auto mob = g->mob_at(loc)) {
            if (!std::visit(is_seen_by(z), *mob)) return;
            if (std::visit(monster::is_enemy_of(z), *mob)) ret.first.push_back(*mob);
            else ret.second.push_back(*mob);
        }
    };

    forall_do_inclusive(scan, classify);
    return ret;
}

struct can_tase {
    monster& z;

    can_tase(monster& z) noexcept : z(z) {}
    can_tase(const can_tase& src) = delete;
    can_tase(can_tase&& src) = delete;
    can_tase& operator=(const can_tase& src) = delete;
    can_tase& operator=(can_tase&& src) = delete;
    ~can_tase() = default;

    // following is highly asymmetric
    bool operator()(const monster* target) const {
        return !target->has_flag(MF_ELECTRIC); // we assume immune to tasers if electrified
    }
    bool operator()(const player* target) const {
        return bool(z.see(*target));
    }
};

void mattack::tazer(monster& z)
{
    if (!z.can_see()) return;  // everything automatically out of range?

    const auto g = game::active();
    inline_stack<std::remove_reference_t<decltype(*(g->mob_at(z.GPSpos)))>, std::end(Direction::vector)-std::begin(Direction::vector)> threats;

    for (decltype(auto) dir : Direction::vector) {
        auto dest = z.GPSpos + dir;
        if (auto mob = g->mob_at(dest)) {
            if (!std::visit(monster::is_enemy_of(z), *mob)) continue;
            if (!std::visit(can_tase(z), *mob)) continue;
            threats.push(*mob);
        }
    }
    if (threats.empty()) return;    // no hostile, taseable targets in range
    mobile& target = std::visit(to_ref<mobile>(), threats[rng(0, threats.size() - 1)]);

    z.sp_timeout = z.type->sp_freq; // Reset timer
    z.moves -= 2 * mobile::mp_turn; // It takes a while

    static auto shocked = [&]() {
        return SVO_sentence(z, "shock", target.desc(grammar::noun::role::direct_object, grammar::article::definite), "!");
    };

    z.if_visible_message(shocked);
    int shock = rng(1, 5);
    target.hurt(shock * rng(1, 3));    // \todo B-movie: bypass metallic armor?  Or be immune if in self-insulated metallic armor?
    target.moves -= shock * (mobile::mp_turn / 5);
}

void mattack::smg(monster& z)
{
    const auto g = game::active();
    std::vector<std::pair<std::variant<monster*, npc*, pc*>, std::vector<GPS_loc> > > targets;
    {
    std::vector<GPS_loc> friendly_fire;

    {
    auto in_range = g->mobs_in_range(z.GPSpos, 24);    // use target-PC range 24. rather than target-monster range of 19
    if (!in_range) return;  // couldn't find any targets, even clairvoyantly

    for (decltype(auto) x : *in_range) {
        if (std::visit(monster::is_enemy_of(z), x)) {
            if (auto path = z.GPSpos.sees(std::visit(to_ref<mobile>(), x).GPSpos, -1)) targets.push_back(std::pair(x, std::move(*path)));
        } else {
            friendly_fire.push_back(std::visit(to_ref<mobile>(), x).GPSpos);
        }
    };
    }   // end scope of in_range
    if (targets.empty()) return;  // no hostile targets in Line Of Sight
    if (!friendly_fire.empty()) {
        auto ub = targets.size();
        do {
            bool remove = false;
            --ub;
            for (decltype(auto) x : targets[ub].second) {
                for (decltype(auto) loc : friendly_fire) {
                    if (loc == std::visit(to_ref<mobile>(), targets[ub].first).GPSpos) {
                        remove = true;
                        break;
                    }
                }
                if (remove) break;
            };
            if (remove) EraseAt(targets, ub);
            while (targets[ub].second.size() != targets.back().second.size()) {
                if (targets[ub].second.size() < targets.back().second.size()) EraseAt(targets, targets.size() - 1);
                else EraseAt(targets, ub);
            };
        } while(0<ub);
        if (targets.empty()) return;  // no usable hostile targets in Line Of Sight
    }
    }   // end scope of friendly_fire

    z.sp_timeout = z.type->sp_freq;	// Reset timer

    // XXX Players are Special (arguably a game balance issue)
    if (std::get_if<pc*>(&(targets[0]).first)) {
        if (!z.has_effect(ME_TARGETED)) {
            g->sound(z.pos, 6, "beep-beep-beep!");
            z.add_effect(ME_TARGETED, 8);
            z.moves -= mobile::mp_turn;
            return;
        }
    };

    z.moves -= (mobile::mp_turn / 2) * 3; // It takes a while
    if (g->u.see(z.GPSpos)) messages.add("The %s fires its smg!", z.name().c_str());
    auto tmp(npc::get_proxy("The " + z.name(), z.pos, *static_cast<it_gun*>(item::types[itm_smg_9mm]), 0, 10));
    g->fire(tmp, targets[0].second, true);

    // XXX Players are Special (arguably a game balance issue)
    if (std::get_if<pc*>(&(targets[0]).first)) {
        z.add_effect(ME_TARGETED, 3);
    };
}

void mattack::flamethrower(monster& z)
{
    auto mobs_seen = mobs_seen_by(z, within_rldist<5>);
    if (mobs_seen.first.empty()) return;    // no enemies in range

    // \todo screen out immune targets
    // \todo avoid friendly fire

    mobile& target = std::visit(to_ref<mobile>(), mobs_seen.first[rng(0, mobs_seen.first.size() - 1)]);

    z.sp_timeout = z.type->sp_freq;	// Reset timer
    z.moves -= 5 * mobile::mp_turn;	// It takes a while
    for (auto tmp = *z.GPSpos.sees(target.GPSpos, 5); auto& loc : tmp) loc.add(field(fd_fire, 1));
    target.add(mobile::effect::ONFIRE, TURNS(8));
}

void mattack::copbot(game *g, monster *z)
{
 const bool sees_u = (bool)z->see(g->u);

 static auto speech = [&]() {
     if (one_in(3)) {
         if (sees_u) {
             if (g->u.unarmed_attack()) return "a robotic voice booms, \"Citizen, Halt!\"";
             else return "a robotic voice booms, \"Please put down your weapon.\"";
         } else
             return "a robotic voice booms, \"Come out with your hands up!\"";
     } else
         return "a police siren, whoop WHOOP";
 };

 z->sp_timeout = z->type->sp_freq;	// Reset timer
 if (rl_dist(z->GPSpos, g->u.GPSpos) > 2 || !sees_u) {
  g->sound(z->pos, 18, speech());
  return;
 }
 tazer(*z);
}

void mattack::multi_robot(game *g, monster *z)
{
 int mode = 0;
 if (!z->see(g->u)) return;	// Can't see you!
 {
 const auto range = rl_dist(z->GPSpos, g->u.GPSpos);
 if (range == 1 && one_in(2)) mode = 1;
 else if (range <= 5) mode = 2;
 else if (range <= 12) mode = 3;
 }

 if (mode == 0) return;	// No attacks were valid!

 switch (mode) {
  case 1: tazer(*z);        break;
  case 2: flamethrower(*z); break;
  case 3: smg(*z);          break;
 }
}

// This attack curses the player with a disease that rapidly mutates.
// extending this to NPCs, requires NPC support for mutations, etc.
void mattack::ratking(monster& z)
{
    const auto g = game::active();
    if (rl_dist(z.GPSpos, g->u.GPSpos) > 10) return;
    z.sp_timeout = z.type->sp_freq;	// Reset timer

 static constexpr const char* rat_chat[] = {  // What do we say?
  "\"YOU... ARE FILTH...\"",
  "\"VERMIN... YOU ARE VERMIN...\"",
  "\"LEAVE NOW...\"",
  "\"WE... WILL FEAST... UPON YOU...\"",
  "\"FOUL INTERLOPER...\""
 };

 messages.add(rat_chat[rng(0, (std::end(rat_chat)-std::begin(rat_chat))-1)]);
 g->u.add_disease(DI_RAT, MINUTES(2));
}

void mattack::generator(monster& z)
{
 z.GPSpos.sound(100, "");
 if (int(messages.turn) % 10 == 0 && z.hp < z.type->hp) z.hp++; // self-heal (for defense scenario)
}

void mattack::upgrade(monster& z)
{
    assert(mon_zombie != z.type->id);
    const auto g = game::active();
    std::vector<monster*> targets;

    g->forall_do([&, gps=z.GPSpos](monster& _mon) mutable {
        if (mon_zombie == _mon.type->id && 5 >= rl_dist(gps, _mon.GPSpos)) targets.push_back(&_mon);
    });
    if (targets.empty()) return;

    z.sp_timeout = z.type->sp_freq; // Reset timer
    z.moves -= (mobile::mp_turn / 2) * 3; // It takes a while

    monster* const target = targets[rng(0, targets.size() - 1)];

    static constexpr const std::pair<mon_id, int> poly_to[] = {
        {mon_zombie_shrieker, 1},
        {mon_zombie_spitter, 2},
        {mon_zombie_electric, 2},
        {mon_zombie_fast, 3},
        {mon_zombie_brute, 1},
        {mon_boomer, 1}
    };

    target->poly(mtype::types[use_rarity_table(rng(0, force_consteval<rarity_table_nonstrict_ub(std::begin(poly_to), std::end(poly_to))>), std::begin(poly_to), std::end(poly_to))]);

    if (g->u.see(z.GPSpos)) messages.add("The black mist around the %s grows...", z.name().c_str());
    if (g->u.see(target->GPSpos)) messages.add("...a zombie becomes a %s!", target->name().c_str());
}

void mattack::breathe(monster& z)
{
 z.sp_timeout = z.type->sp_freq; // Reset timer
 z.moves -= mobile::mp_turn; // It takes a while

 bool able = (z.type->id == mon_breather_hub);
 if (!able) {
     static auto has_hub = [&](const point delta) {
         auto loc = z.GPSpos + delta;
         if (const auto m_at = game::active()->mon(loc)) {
             if (mon_breather_hub == m_at->type->id) return true;
         }
         return false;
     };

     if (find_first(within_rldist<3>, has_hub)) able = true;
 }
 if (!able) return;

 std::vector<GPS_loc> valid;
 for (decltype(auto) delta : Direction::vector) {
     const auto test(z.GPSpos + delta);
     if (test.is_empty()) valid.push_back(test);
 }

 if (!valid.empty()) {
  monster spawned(mtype::types[mon_breather], valid[rng(0, valid.size() - 1)]);
  spawned.sp_timeout = 12;
  game::active()->spawn(std::move(spawned));
 }
}
