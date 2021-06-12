#include "monattack.h"
#include "mondeath.h"
#include "monster.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "bodypart.h"
#include "recent_msg.h"
#include "stl_limits.h"
#include "stl_typetraits_late.h"

void mattack::antqueen(game *g, monster *z)
{
 std::vector<point> egg_points;
 std::vector<monster*> ants;
 z->sp_timeout = z->type->sp_freq;	// Reset timer
// Count up all adjacent tiles the contain at least one egg.
 for (int x = z->pos.x - 2; x <= z->pos.x + 2; x++) {
  for (int y = z->pos.y - 2; y <= z->pos.y + 2; y++) {
   monster* const _mon = g->mon(x, y);
   if (_mon)  { 
	   if ((_mon->type->id == mon_ant_larva || _mon->type->id == mon_ant)) ants.push_back(_mon);
	   continue;	// not empty so egg hatching check will fail
   }
   if (!g->is_empty(x, y)) continue;	// is_empty() because we can't hatch an ant under the player, a monster, etc.
   for (auto& obj : g->m.i_at(x, y)) {
    if (obj.type->id == itm_ant_egg) {
     egg_points.push_back(point(x, y));
     break;	// Done looking at this tile
    }
   }
  }
 }

 if (!ants.empty()) {
  z->moves -= 100; // It takes a while
  monster *const ant = ants[rng(0, ants.size() - 1)];
  if (g->u_see(z->pos) && g->u_see(ant->pos)) messages.add("The %s feeds an %s and it grows!", z->name().c_str(), ant->name().c_str());
  ant->poly(mtype::types[ant->type->id == mon_ant_larva ? mon_ant : mon_ant_soldier]);
 } else if (egg_points.empty()) {	// There's no eggs nearby--lay one.
  if (g->u_see(z->pos)) messages.add("The %s lays an egg!", z->name().c_str());
  g->m.add_item(z->pos, item::types[itm_ant_egg], messages.turn);
 } else { // There are eggs nearby.  Let's hatch some.
  z->moves -= 20 * egg_points.size(); // It takes a while
  if (g->u_see(z->pos)) messages.add("The %s tends nearby eggs, and they hatch!", z->name().c_str());
  for (int i = 0; i < egg_points.size(); i++) {
   int j = -1;
   for(auto& obj : g->m.i_at(egg_points[i])) {
	++j;
    if (obj.type->id == itm_ant_egg) {
     g->m.i_rem(egg_points[i], j);
     g->z.push_back(monster(mtype::types[mon_ant_larva], egg_points[i]));
	 break;	// Max one hatch per tile.
	}
   }
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
 z->moves -= 3*mobile::mp_turn; // It takes a while
 z->sp_timeout = z->type->sp_freq; // Reset timer
 g->sound(z->pos, 4, "a spitting noise.");

 static constexpr const zaimoni::gdi::box<point> spread(point(-2), point(2));

 point hit(g->u.pos + rng(spread));
 for (const auto& pt : line_to(z->pos, hit, *j)) {
     if (g->m.hit_with_acid(pt)) {
         if (g->u_see(pt)) messages.add("A glob of acid hits the %s!", g->m.tername(pt).c_str());
         return;
     }
 }
 for (int i = -3; i <= 3; i++) {
  for (int j = -3; j <= 3; j++) {
   point dest(hit.x + i, hit.y + j);
   if (g->m.move_cost(dest) > 0 &&
       g->m.sees(dest, hit, 6) &&
       one_in(abs(j)) && one_in(abs(i))) {
	 auto& f = g->m.field_at(dest);
     if (f.type == fd_acid && f.density < 3) f.density++;
     else g->m.add_field(g, dest, fd_acid, 2);
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
 static constexpr const auto spread = zaimoni::gdi::box<point>(point(-1), point(1));

 // 2/9 near-miss for each of x, y axis; theoretical probability for hit if not obstructed 49/81
 const auto tar = g->u.pos + rng(spread) + rng(spread);

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
 const bool u_see = (bool)g->u_see(z->pos);
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
      if (g->u_see(pt))
          messages.add("Bile splatters on the %s!", g->m.tername(pt).c_str());
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
 std::vector<point> corpses;
// Find all corposes that we can see within 4 tiles.
 for (int x = z->pos.x - 4; x <= z->pos.x + 4; x++) {
  for (int y = z->pos.y - 4; y <= z->pos.y + 4; y++) {
   if (g->is_empty(x, y) && g->m.sees(z->pos, x, y, -1)) {
	for(auto& obj : g->m.i_at(x, y)) {
     if (obj.type->id == itm_corpse && obj.corpse->species == species_zombie) {
      corpses.push_back(point(x, y));
	  break;
     }
    }
   }
  }
 }
 if (corpses.empty()) return;	// No nearby corpses
 z->speed = cataclysm::rational_scaled<4, 5>(z->speed - rng(0, 10));
 const bool sees_necromancer = g->u.see(*z);
 if (sees_necromancer)
     messages.add("The %s throws its arms wide...",
                  grammar::capitalize(z->desc(grammar::noun::role::subject, grammar::article::definite)).c_str());
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 z->moves -= 5 * mobile::mp_turn; // It takes a while
 int raised = 0;
 for (int i = 0; i < corpses.size(); i++) {
  int n = -1;
  for(auto& obj : g->m.i_at(corpses[i])) {
   ++n;
   if (obj.type->id == itm_corpse && obj.corpse->species == species_zombie && one_in(2)) {
    if (g->u_see(corpses[i])) raised++;
    int burnt_penalty = obj.burnt;
    monster mon(obj.corpse, corpses[i]);
    mon.speed = cataclysm::rational_scaled<4, 5>(mon.speed) - burnt_penalty / 2;
    mon.hp    = cataclysm::rational_scaled<7, 10>(mon.hp) - burnt_penalty;
    g->m.i_rem(corpses[i], n);
    g->z.push_back(mon);
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
  g->z.push_back(monster(mtype::types[mon_manhack], free[rng(0, valid.size() - 1)]));
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
     if (monster* const m_at = g->mon(dest)) {
      if (g->u_see(dest))
       messages.add("A tree bursts forth from the earth and pierces the %s!", m_at->name().c_str());
      int rn = rng(10, 30) - m_at->armor_cut();
      if (rn < 0) rn = 0;
      if (m_at->hurt(rn)) g->kill_mon(*m_at, z);
     } else if (g->u.pos == dest) {
// Player is hit by a growing tree
      body_part hit = bp_legs;
      int side = rng(1, 2);
      if (one_in(4)) hit = bp_torso;
      else if (one_in(2)) hit = bp_feet;
	  messages.add("A tree bursts forth from the earth and pierces your %s!",
                 body_part_name(hit, side));
      g->u.hit(g, hit, side, 0, rng(10, 30));
     } else if (npc* const nPC = g->nPC(dest)) {	// An NPC got hit
       body_part hit = bp_legs;
       int side = rng(1, 2);
       if (one_in(4)) hit = bp_torso;
       else if (one_in(2)) hit = bp_feet;
       if (g->u_see(dest))
        messages.add("A tree bursts forth from the earth and pierces %s's %s!",
			nPC->name.c_str(), body_part_name(hit, side));
	   nPC->hit(g, hit, side, 0, rng(10, 30));
     }
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
         if (monster* const m_at = g->mon(dest)) {
             if (g->u_see(dest))
                 messages.add("Underbrush grows, and it pierces the %s!", m_at->name().c_str());
             int rn = rng(10, 30) - m_at->armor_cut();
             if (rn < 0) rn = 0;
             if (m_at->hurt(rn)) g->kill_mon(*m_at, z);
         } else if (g->u.pos == dest) {
             body_part hit = bp_legs;
             int side = rng(1, 2);
             if (one_in(4)) hit = bp_torso;
             else if (one_in(2)) hit = bp_feet;
             messages.add("The underbrush beneath your feet grows and pierces your %s!",
                 body_part_name(hit, side));
             g->u.hit(g, hit, side, 0, rng(10, 30));
         } else if (npc* const nPC = g->nPC(dest)) {
             body_part hit = bp_legs;
             int side = rng(1, 2);
             if (one_in(4)) hit = bp_torso;
             else if (one_in(2)) hit = bp_feet;
             if (g->u_see(dest))
                 messages.add("Underbrush grows, and it pierces %s's %s!",
                     nPC->name.c_str(), body_part_name(hit, side));
             nPC->hit(g, hit, side, 0, rng(10, 30));
         } else t = t_tree_young;
     }
   }
  }
 }
}

void mattack::grow_vine(game *g, monster *z)
{
 z->sp_timeout = z->type->sp_freq;
 z->moves -= 100;
 int xshift = rng(0, 2), yshift = rng(0, 2);
 for (int x = 0; x < 3; x++) {
  for (int y = 0; y < 3; y++) {
   int xvine = z->pos.x + (x + xshift) % 3 - 1,
       yvine = z->pos.y + (y + yshift) % 3 - 1;
   if (g->is_empty(xvine, yvine)) {
    monster vine(mtype::types[mon_creeper_vine], xvine, yvine);
    vine.sp_timeout = 5;
    g->z.push_back(vine);
   }
  }
 }
}

void mattack::vine(game *g, monster *z)
{
 std::vector<point> grow;
 int vine_neighbors = 0;
 z->sp_timeout = z->type->sp_freq;
 z->moves -= mobile::mp_turn;
 // Yes, we want to count ourselves as a neighbor.
 for (int x = z->pos.x - 1; x <= z->pos.x + 1; x++) {
  for (int y = z->pos.y - 1; y <= z->pos.y + 1; y++) {
   if (g->u.pos.x == x && g->u.pos.y == y) {
    body_part bphit = random_body_part();
    int side = rng(0, 1);
    messages.add("The %s lashes your %s!", z->name().c_str(), body_part_name(bphit, side));
    g->u.hit(g, bphit, side, 4, 4);
    z->sp_timeout = z->type->sp_freq;
    z->moves -= mobile::mp_turn;
    return;
   } else if (g->is_empty(x, y)) grow.push_back(point(x, y));
   else if (monster* const m_at = g->mon(x, y)) {
    if (m_at->type->id == mon_creeper_vine) vine_neighbors++;
   }
  }
 }
 // see if we want to grow
 if (grow.empty() || vine_neighbors > 5 || one_in(7 - vine_neighbors)) return;

// Calculate distance from nearest hub, then check against that
 int dist_from_hub = INT_MAX;
 for (const auto& v : g->z) {
  if (mon_creeper_hub == v.type->id) clamp_ub(dist_from_hub, rl_dist(z->GPSpos, v.GPSpos));
 }
 if (!one_in(dist_from_hub)) return;

 monster vine(mtype::types[mon_creeper_vine], grow[rng(0, grow.size() - 1)]);
 vine.sp_timeout = 5;
 g->z.push_back(vine);
}

static int route_sap(const std::vector<point>& line)
{
    auto g = game::active();
    int dam = 5;
    for (decltype(auto) pt : line) {
        g->m.shoot(g, pt, dam, false, 0);
        if (0 == dam) {
            if (g->u_see(pt)) messages.add("A glob of sap hits the %s!", g->m.tername(pt).c_str());
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

 if (g->u_see(z->pos)) {
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
  for (int x = span.tl_c().x; x <= span.br_c().x; x++) {
   for (int y = span.tl_c().y; y <= span.br_c().y; y++) {
	auto& t = g->m.ter(x, y);
    if (g->is_empty(x, y) && one_in(4)) t = t_root_wall;
    else if (t_root_wall == t && one_in(10)) t = t_dirt;
   }
  }
// Open blank tiles as long as there's no possible route
  int tries = 0;
  while (g->m.route(g->u.pos, z->pos).empty() && tries < 20) {
   const point pt = rng(span);
   tries++;
   g->m.ter(pt) = t_dirt;
   if (rl_dist(pt, g->u.pos) > 3 && g->z.size() < 30 &&
       !g->mon(pt) && one_in(20)) { // Spawn an extra monster
    mon_id montype = mon_triffid;
    if (one_in(4)) montype = mon_creeper_hub;
    else if (one_in(3)) montype = mon_biollante;
    g->z.push_back(monster(mtype::types[montype], pt));
   }
  }
 } else { // The player is close enough for a fight!
  monster triffid(mtype::types[mon_triffid]);
  for (decltype(auto) delta : Direction::vector) {
      if (const point pt(z->pos + delta); g->is_empty(pt) && one_in(2)) {
          triffid.spawn(pt);
          g->z.push_back(triffid);
      }
  }
 }
}

void mattack::fungus(game *g, monster *z)
{
 if (g->z.size() > 100) return; // Prevent crowding the monster list.
 z->moves -= 2 * mobile::mp_turn; // It takes a while
 z->sp_timeout = z->type->sp_freq; // Reset timer
 monster spore(mtype::types[mon_spore]);
 int moncount = 0;
 g->sound(z->pos, 10, "Pouf!");
 if (g->u.see(*z)) messages.add("Spores are released from the %s!", z->name().c_str());
 for (decltype(auto) dir : Direction::vector) {
     point dest(z->pos + dir);
     monster* const m_at = g->mon(dest);
     if (m_at) ++moncount;
     if (g->m.move_cost(dest) > 0 && one_in(5)) {
         if (m_at) {	// Spores hit a monster
             // \todo? Is it correct for spores to affect digging monsters (current behavior)
             if (g->u.see(*m_at)) messages.add("The %s is covered in tiny spores!", m_at->name().c_str());
             if (!m_at->make_fungus()) g->kill_mon(*m_at, z);
             // \todo infect NPCs (both DI_SPORES and DI_FUNGUS have to become NPC-competent)
         } else if (g->u.pos == dest)
             g->u.infect(DI_SPORES, bp_mouth, 4, 30); // Spores hit the player
         else { // Spawn a spore
             spore.spawn(dest);
             g->z.push_back(spore);
         }
     }
 }
 if (moncount >= 7) z->poly(mtype::types[mon_fungaloid_dormant]);	// If we're surrounded by monsters, go dormant
}

void mattack::fungus_sprout(game *g, monster *z)
{
 for (decltype(auto) delta : Direction::vector) {
     const point pt(z->pos + delta);
     if (pt == g->u.pos) {
         messages.add("You're shoved away as a fungal wall grows!");
         g->teleport();
     } else if (const auto _npc = g->nPC(pt)) {
         if (g->u_see(pt)) messages.add("% is shoved away as a fungal wall grows!", _npc->name.c_str());
         g->teleport(_npc);
     }
     if (g->is_empty(pt)) g->z.push_back(monster(mtype::types[mon_fungal_wall], pt));
 }
}

void mattack::leap(game *g, monster *z)
{
 if (!z->see(g->u)) return;	// Only leap if we can see you!

 std::vector<point> options;
 const bool fleeing = z->is_fleeing(g->u);
 int best = fleeing ? INT_MIN : INT_MAX;

 for (int x = z->pos.x - 3; x <= z->pos.x + 3; x++) {
  for (int y = z->pos.y - 3; y <= z->pos.y + 3; y++) {
/* If we're fleeing, we want to pick those tiles with the greatest distance
 * from the player; otherwise, those tiles with the least distance from the
 * player.
 */
   if (!g->is_empty(x, y) || !g->m.sees(z->pos, x, y, g->light_level())) continue;
   const int dist = rl_dist(g->u.pos, x, y);
   if (fleeing ? dist >= best : dist <= best) {
       if (dist != best) {
           options.clear();
           best = dist;
       }
       options.push_back(point(x, y));
   }
  }
 }

 if (options.empty()) return; // Nowhere to leap!

 z->moves -= (mobile::mp_turn / 2) * 3;
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 bool seen = g->u.see(*z); // We can see them jump...
 z->screenpos_set(options[rng(0, options.size() - 1)]);
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
 int attack_roll = dice(z->type->melee_skill, 10);
 if (g->u.dodge_roll() > attack_roll) {
  messages.add("The %s tries to land on you, but you dodge.", z->name().c_str());
  z->stumble(g, false);
  return;
 }

// Can we swat the bug away?
 int dodge_roll = z->dodge_roll();
 int swat_skill = (g->u.sklevel[sk_melee] + g->u.sklevel[sk_unarmed] * 2) / 3;
 int player_swat = dice(swat_skill, 10);
 if (player_swat > dodge_roll) {
  messages.add("The %s lands on you, but you swat it off.", z->name().c_str());
  if (z->hp >= z->type->hp / 2) z->hurt(1);
  if (player_swat > dodge_roll * 1.5) z->stumble(g, false);
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
  z->moves -= (mobile::mp_turn / 2) * 3; // Attemped laying takes a while
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
  if (g->u_see(z->pos)) messages.add("The %s takes seed and becomes a young fungaloid!", z->name().c_str());
  z->poly(mtype::types[mon_fungaloid_young]);
  z->moves -= 10 * mobile::mp_turn;	// It takes a while
 }
}

void mattack::disappear(game *g, monster *z)
{
 z->hp = 0;
}

void mattack::formblob(game *g, monster *z)
{
 bool didit = false;
 int thatmon = -1;
 for (int i = -1; i <= 1; i++) {
  for (int j = -1; j <= 1; j++) {
   point test(z->pos.x + i, z->pos.y + j);
   if (g->u.pos == test) {
// If we hit the player, cover them with slime
    didit = true;
    g->u.add_disease(DI_SLIMED, rng(0, z->hp));
   } else if (monster* const m_at = g->mon(test)) {
// Hit a monster.  If it's a blob, give it our speed.  Otherwise, blobify it?
    if (z->speed > 20 && m_at->type->id == mon_blob && m_at->speed < 85) {
     didit = true;
	 m_at->speed += 5;
     z->speed -= 5;
    } else if (z->speed > 20 && m_at->type->id == mon_blob_small) {
     didit = true;
     z->speed -= 5;
	 m_at->speed += 5;
     if (m_at->speed >= 60) g->z[thatmon].poly(mtype::types[mon_blob]);
    } else if ((m_at->made_of(FLESH) || m_at->made_of(VEGGY)) &&
               rng(0, z->hp) > rng(0, g->z[thatmon].hp)) {	// Blobify!
     didit = true;
	 m_at->poly(mtype::types[mon_blob]);
	 m_at->speed = z->speed - rng(5, 25);
	 m_at->hp = g->z[thatmon].speed;
    }
   } else if (z->speed >= 85 && rng(0, 250) < z->speed) {
// If we're big enough, spawn a baby blob.
    didit = true;
    z->speed -= 15;
    monster blob(mtype::types[mon_blob_small], test);
    blob.speed = z->speed - rng(30, 60);
    blob.hp = blob.speed;
    g->z.push_back(blob);
   }
  }
  if (didit) {	// We did SOMEthing.
   if (z->type->id == mon_blob && z->speed <= 50) z->poly(mtype::types[mon_blob_small]);	// We shrank!
   z->moves = -500;
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

 z->friendly = 0;
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

void mattack::vortex(game *g, monster *z)
{
 static const int base_mon_throw_range[mtype::MS_MAX] = {5, 3, 2, 1, 0};

// Make sure that the player's butchering is interrupted!
 if (g->u.activity.type == ACT_BUTCHER && rl_dist(z->GPSpos, g->u.GPSpos) <= 2) {
  messages.add("The buffeting winds interrupt your butchering!");
  g->u.activity.type = ACT_NULL;
 }
// Moves are NOT used up by this attack, as it is "passive"
 z->sp_timeout = z->type->sp_freq;
// Before anything else, smash terrain!
 for (int x = z->pos.x - 2; x <= z->pos.x + 2; x++) {
  for (int y = z->pos.y - 2; y <= z->pos.y + 2; y++) {
   if (x == z->pos.x && y == z->pos.y) y++; // Don't throw us!
   std::string sound;
   g->m.bash(x, y, 14, sound);
   g->sound(point(x, y), 8, sound);
  }
 }
 for (int x = z->pos.x - 2; x <= z->pos.x + 2; x++) {
  for (int y = z->pos.y - 2; y <= z->pos.y + 2; y++) {
   if (x == z->pos.x && y == z->pos.y) y++; // Don't throw us!
   std::vector<point> from_monster = line_to(z->pos, x, y, 0);
   while (!g->m.i_at(x, y).empty()) {
    item thrown = g->m.i_at(x, y)[0];
    g->m.i_rem(x, y, 0);
    int distance = 5 - (thrown.weight() / 15);
    if (distance > 0) {
     int dam = thrown.weight() / double(3 + double(thrown.volume() / 6));
     std::vector<point> traj = continue_line(from_monster, distance);
     for (int i = 0; i < traj.size() && dam > 0; i++) {
      g->m.shoot(g, traj[i], dam, false, 0);
      if (monster* const m_at = g->mon(traj[i])) {
       if (m_at->hurt(dam)) g->kill_mon(*m_at, z);
       dam = 0;
      }
      if (g->m.move_cost(traj[i]) == 0) {
       dam = 0;
       i--;
      } else if (traj[i] == g->u.pos) {
       body_part hit = random_body_part();
       int side = rng(0, 1);
	   messages.add("A %s hits your %s for %d damage!", thrown.tname().c_str(), body_part_name(hit, side), dam);
       g->u.hit(g, hit, side, dam, 0);
       dam = 0;
      }
// TODO: Hit NPCs
      if (dam == 0 || i == traj.size() - 1) {
       g->m.hard_landing(traj[i], std::move(thrown));
      }
     }
    } // Done throwing item
   } // Done getting items
// Throw monsters
   if (monster* const thrown = g->mon(x, y)) {
    int distance = base_mon_throw_range[thrown->type->size];
	int damage = distance * 4;
    switch (thrown->type->mat) {
     case LIQUID:  distance += 3; damage -= 10; break;
     case VEGGY:   distance += 1; damage -=  5; break;
     case POWDER:  distance += 4; damage -= 30; break;
     case COTTON:
     case WOOL:    distance += 5; damage -= 40; break;
     case LEATHER: distance -= 1; damage +=  5; break;
     case KEVLAR:  distance -= 3; damage -= 20; break;
     case STONE:   distance -= 3; damage +=  5; break;
     case PAPER:   distance += 6; damage -= 10; break;
     case WOOD:    distance += 1; damage +=  5; break;
     case PLASTIC: distance += 1; damage +=  5; break;
     case GLASS:   distance += 2; damage += 20; break;
     case IRON:    distance -= 1; [[fallthrough]];
     case STEEL:
     case SILVER:  distance -= 3; damage -= 10; break;
    }
    if (distance > 0) {
     if (g->u.see(*thrown))
         messages.add("%s is thrown by winds!",
                      grammar::capitalize(thrown->desc(grammar::noun::role::subject, grammar::article::definite)).c_str());
     std::vector<point> traj = continue_line(from_monster, distance);
     bool hit_wall = false;
     for (int i = 0; i < traj.size() && !hit_wall; i++) {
      monster* const m_hit = g->mon(traj[i]);
      if (i > 0 && m_hit && !m_hit->has_flag(MF_DIGS)) {
       if (g->u_see(traj[i])) messages.add("The %s hits a %s!", thrown->name().c_str(), m_hit->name().c_str());
       if (m_hit->hurt(damage)) g->kill_mon(*m_hit, z);
       hit_wall = true;
       thrown->screenpos_set(traj[i - 1]);
      } else if (g->m.move_cost(traj[i]) == 0) {
       hit_wall = true;
       thrown->screenpos_set(traj[i - 1]);
      }
      int damage_copy = damage;
      g->m.shoot(g, traj[i], damage_copy, false, 0);
      if (damage_copy < damage) thrown->hurt(damage - damage_copy);
     }
     if (hit_wall) damage *= 2;
     else thrown->screenpos_set(traj.back());
     if (thrown->hurt(damage)) g->kill_mon(*thrown, z);
    } // if (distance > 0)
   } // if (thrown)

   if (g->u.pos.x == x && g->u.pos.y == y) { // Throw... the player?! D:
    std::vector<point> traj = continue_line(from_monster, rng(2, 3));
    messages.add("You're thrown by winds!");
    bool hit_wall = false;
    int damage = rng(5, 10);
    for (int i = 0; i < traj.size() && !hit_wall; i++) {
	 monster* const m_hit = g->mon(traj[i]);
     if (i > 0 && m_hit && !m_hit->has_flag(MF_DIGS)) {
      if (g->u_see(traj[i])) messages.add("You hit a %s!", m_hit->name().c_str());
      if (m_hit->hurt(damage)) g->kill_mon(*m_hit, &g->u); // We get the kill :)
      hit_wall = true;
      g->u.screenpos_set(traj[i - 1]);
     } else if (g->m.move_cost(traj[i]) == 0) {
      messages.add("You slam into a %s", g->m.tername(traj[i]).c_str());
      hit_wall = true;
      g->u.screenpos_set(traj[i - 1]);
     }
     int damage_copy = damage;
     g->m.shoot(g, traj[i], damage_copy, false, 0);
     if (damage_copy < damage) g->u.hit(g, bp_torso, 0, damage - damage_copy, 0);
    }
    if (hit_wall) damage *= 2;
    else {
     g->u.screenpos_set(traj.back());
    }
    g->u.hit(g, bp_torso, 0, damage, 0);
    g->update_map(g->u.pos.x, g->u.pos.y);
   } // Done with checking for player
  }
 } // Done with loop!
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
 if (g->u_see(z->pos)) {
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
 g->add_event(EVENT_ROBOT_ATTACK, int(messages.turn) + rng(15, 30), z->faction_id, g->lev.x, g->lev.y);
}

void mattack::tazer(game *g, monster *z)
{
 if (rl_dist(z->GPSpos, g->u.GPSpos) > 2 || !z->see(g->u)) return;	// Out of range
 z->sp_timeout = z->type->sp_freq; // Reset timer
 z->moves -= 2*mobile::mp_turn; // It takes a while
 messages.add("%s shocks you!",
              grammar::capitalize(z->desc(grammar::noun::role::subject, grammar::article::definite)).c_str());
 int shock = rng(1, 5);
 g->u.hurt(g, bp_torso, 0, shock * rng(1, 3));
 g->u.moves -= shock * (mobile::mp_turn/5);
}

void mattack::smg(game *g, monster *z)
{
 if (!z->is_enemy()) { // Attacking monsters, not the player!
  int fire_t;
  monster* target = nullptr;
  point target_pos;
  int closest = 19;
  for(auto& _mon : g->z) {
   if (!_mon.is_enemy(z)) continue;
   if (int dist = rl_dist(z->GPSpos, _mon.GPSpos); dist < closest) {
       if (const auto pos = _mon.screen_pos()) { // \todo would be nice to attack off-reality bubble
           if (const auto t = g->m.sees(z->pos, *pos, 18)) {
               target = &_mon;
               closest = dist;
               fire_t = *t;
               target_pos = *pos;
           }
       }
   }
  }
  if (!target) return; // Couldn't find any targets!

  z->sp_timeout = z->type->sp_freq;	// Reset timer
  z->moves -= (mobile::mp_turn / 2) * 3; // It takes a while
  if (g->u_see(z->pos)) messages.add("The %s fires its smg!", z->name().c_str());
  player tmp(player::get_proxy("The " + z->name(), z->pos, *static_cast<it_gun*>(item::types[itm_smg_9mm]), 0, 10));
  std::vector<point> traj = line_to(z->pos, target_pos, fire_t);
  g->fire(tmp, target_pos, traj, true);

  return;
 }
 
// Not friendly; hence, firing at the player
 if (24 < rl_dist(z->GPSpos, g->u.GPSpos)) return; // Out of range
 const auto t = z->see(g->u);
 if (!t) return; // Unseen
 z->sp_timeout = z->type->sp_freq;	// Reset timer

 if (!z->has_effect(ME_TARGETED)) {
  g->sound(z->pos, 6, "beep-beep-beep!");
  z->add_effect(ME_TARGETED, 8);
  z->moves -= mobile::mp_turn;
  return;
 }
 z->moves -= (mobile::mp_turn/2)*3; // It takes a while

 if (g->u_see(z->pos)) messages.add("The %s fires its smg!", z->name().c_str());
// Set up a temporary player to fire this gun
 player tmp(player::get_proxy("The " + z->name(), z->pos, *static_cast<it_gun*>(item::types[itm_smg_9mm]), 0, 10));
 std::vector<point> traj = line_to(z->pos, g->u.pos, *t);
 g->fire(tmp, g->u.pos, traj, true);
 z->add_effect(ME_TARGETED, 3);
}

void mattack::flamethrower(game *g, monster *z)
{
 if (5 < Linf_dist(g->u.pos - z->pos)) return;	// Out of range
 const auto t = z->see(g->u);
 if (!t) return; // Unseen
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 z->moves -= 5 * mobile::mp_turn;	// It takes a while
 for (const auto& pt : line_to(z->pos, g->u.pos, *t)) g->m.add_field(g, pt, fd_fire, 1);
 g->u.add_disease(DI_ONFIRE, TURNS(8));
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
 tazer(g, z);
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
  case 1: tazer(g, z);        break;
  case 2: flamethrower(g, z); break;
  case 3: smg(g, z);          break;
 }
}

void mattack::ratking(game *g, monster *z)
{
 if (rl_dist(z->GPSpos, g->u.GPSpos) > 10) return;
 z->sp_timeout = z->type->sp_freq;	// Reset timer

 static constexpr const char* rat_chat[] = {  // What do we say?
  "\"YOU... ARE FILTH...\"",
  "\"VERMIN... YOU ARE VERMIN...\"",
  "\"LEAVE NOW...\"",
  "\"WE... WILL FEAST... UPON YOU...\"",
  "\"FOUL INTERLOPER...\""
 };

 messages.add(rat_chat[rng(0, std::end(rat_chat)-std::begin(rat_chat))]);
 g->u.add_disease(DI_RAT, MINUTES(2));
}

void mattack::generator(game *g, monster *z)
{
 g->sound(z->pos, 100, "");
 if (int(messages.turn) % 10 == 0 && z->hp < z->type->hp) z->hp++;
}

void mattack::upgrade(game* g, monster* z)
{
    assert(z && mon_zombie != z->type->id);
    std::vector<monster*> targets;
    for (decltype(auto) _mon : g->z) {
        if (mon_zombie == _mon.type->id && 5 >= rl_dist(z->GPSpos, _mon.GPSpos))
            targets.push_back(&_mon);
    }
    if (targets.empty()) return;

    z->sp_timeout = z->type->sp_freq; // Reset timer
    z->moves -= (mobile::mp_turn / 2) * 3; // It takes a while

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

    if (g->u_see(z->pos)) messages.add("The black mist around the %s grows...", z->name().c_str());
    if (g->u_see(target->pos)) messages.add("...a zombie becomes a %s!", target->name().c_str());
}

void mattack::breathe(game *g, monster *z)
{
 z->sp_timeout = z->type->sp_freq; // Reset timer
 z->moves -= mobile::mp_turn; // It takes a while

 bool able = (z->type->id == mon_breather_hub);
 if (!able) {
  for (int x = z->pos.x - 3; x <= z->pos.x + 3 && !able; x++) {
   for (int y = z->pos.y - 3; y <= z->pos.y + 3 && !able; y++) {
    if (monster* const m_at = g->mon(x, y)) {
	  if (m_at->type->id == mon_breather_hub) able = true;
	}
   }
  }
 }
 if (!able) return;

 std::vector<point> valid;
 for (decltype(auto) delta : Direction::vector) {
     const point test(z->pos + delta);
     if (g->is_empty(test)) valid.push_back(test);
 }

 if (!valid.empty()) {
  monster spawned(mtype::types[mon_breather], valid[rng(0, valid.size() - 1)]);
  spawned.sp_timeout = 12;
  g->z.push_back(spawned);
 }
}
