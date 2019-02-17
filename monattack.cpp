#include "mondeath.h"
#include "monster.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "bodypart.h"
#include "recent_msg.h"

void mattack::antqueen(game *g, monster *z)
{
 std::vector<point> egg_points;
 std::vector<int> ants;
 z->sp_timeout = z->type->sp_freq;	// Reset timer
// Count up all adjacent tiles the contain at least one egg.
 for (int x = z->pos.x - 2; x <= z->pos.x + 2; x++) {
  for (int y = z->pos.y - 2; y <= z->pos.y + 2; y++) {
   for (int i = 0; i < g->m.i_at(x, y).size(); i++) {
// is_empty() because we can't hatch an ant under the player, a monster, etc.
    if (g->m.i_at(x, y)[i].type->id == itm_ant_egg && g->is_empty(x, y)) {
     egg_points.push_back(point(x, y));
     i = g->m.i_at(x, y).size();	// Done looking at this tile
    }
    int mondex = g->mon_at(x, y);
    if (mondex != -1 && (g->z[mondex].type->id == mon_ant_larva ||
                         g->z[mondex].type->id == mon_ant         ))
     ants.push_back(mondex);
   }
  }
 }

 if (!ants.empty()) {
  z->moves -= 100; // It takes a while
  int mondex = ants[ rng(0, ants.size() - 1) ];
  monster *const ant = &(g->z[mondex]);
  if (g->u_see(z->pos.x, z->pos.y) && g->u_see(ant->pos.x, ant->pos.y))
   messages.add("The %s feeds an %s and it grows!", z->name().c_str(), ant->name().c_str());
  ant->poly(mtype::types[ant->type->id == mon_ant_larva ? mon_ant : mon_ant_soldier]);
 } else if (egg_points.empty()) {	// There's no eggs nearby--lay one.
  if (g->u_see(z->pos.x, z->pos.y))
   messages.add("The %s lays an egg!", z->name().c_str());
  g->m.add_item(z->pos.x, z->pos.y, item::types[itm_ant_egg], messages.turn);
 } else { // There are eggs nearby.  Let's hatch some.
  z->moves -= 20 * egg_points.size(); // It takes a while
  if (g->u_see(z->pos.x, z->pos.y))
   messages.add("The %s tends nearby eggs, and they hatch!", z->name().c_str());
  for (int i = 0; i < egg_points.size(); i++) {
   int x = egg_points[i].x, y = egg_points[i].y;
   for (int j = 0; j < g->m.i_at(x, y).size(); j++) {
    if (g->m.i_at(x, y)[j].type->id == itm_ant_egg) {
     g->m.i_rem(x, y, j);
     j = g->m.i_at(x, y).size();	// Max one hatch per tile.
     g->z.push_back(monster(mtype::types[mon_ant_larva], x, y));
    }
   }
  }
 }
}

void mattack::shriek(game *g, monster *z)
{
 if (rl_dist(z->pos.x, z->pos.y, g->u.posx, g->u.posy) > 4 || !g->sees_u(z->pos.x, z->pos.y)) return;	// Out of range
 z->moves = -240;			// It takes a while
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 g->sound(z->pos, 50, "a terrible shriek!");
}

void mattack::acid(game *g, monster *z)
{
 int junk;
 if (rl_dist(z->pos.x, z->pos.y, g->u.posx, g->u.posy) > 10 || !g->sees_u(z->pos.x, z->pos.y, junk)) return;	// Out of range
 z->moves = -300;			// It takes a while
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 g->sound(z->pos, 4, "a spitting noise.");
 int hitx = g->u.posx + rng(-2, 2), hity = g->u.posy + rng(-2, 2);
 std::vector<point> line = line_to(z->pos.x, z->pos.y, hitx, hity, junk);
 for (int i = 0; i < line.size(); i++) {
  if (g->m.hit_with_acid(g, line[i].x, line[i].y)) {
   if (g->u_see(line[i].x, line[i].y))
    messages.add("A glob of acid hits the %s!", g->m.tername(line[i].x, line[i].y).c_str());
   return;
  }
 }
 for (int i = -3; i <= 3; i++) {
  for (int j = -3; j <= 3; j++) {
   if (g->m.move_cost(hitx + i, hity +j) > 0 &&
       g->m.sees(hitx + i, hity + j, hitx, hity, 6) &&
       ((one_in(abs(j)) && one_in(abs(i))) || (i == 0 && j == 0))) {
    if (g->m.field_at(hitx + i, hity + j).type == fd_acid &&
        g->m.field_at(hitx + i, hity + j).density < 3)
     g->m.field_at(hitx + i, hity + j).density++;
    else
     g->m.add_field(g, hitx + i, hity + j, fd_acid, 2);
   }
  }
 }
}

void mattack::shockstorm(game *g, monster *z)
{
 if (!g->sees_u(z->pos.x, z->pos.y) || rl_dist(z->pos.x, z->pos.y, g->u.posx, g->u.posy) > 12) return;	// Can't see you, no attack
 z->moves = -50;			// It takes a while
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 messages.add("A bolt of electricity arcs towards you!");
 int tarx = g->u.posx + rng(-1, 1) + rng(-1, 1),// 3 in 9 chance of direct hit,
     tary = g->u.posy + rng(-1, 1) + rng(-1, 1);// 4 in 9 chance of near hit
 int t;
 std::vector<point> bolt = line_to(z->pos.x, z->pos.y, tarx, tary, (g->m.sees(z->pos.x, z->pos.y, tarx, tary, -1, t) ? t : 0));
 for (int i = 0; i < bolt.size(); i++) { // Fill the LOS with electricity
  if (!one_in(4))
   g->m.add_field(g, bolt[i].x, bolt[i].y, fd_electricity, rng(1, 3));
 }
// 3x3 cloud of electricity at the square hit
 for (int i = tarx - 1; i <= tarx + 1; i++) {
  for (int j = tary - 1; j <= tary + 1; j++) {
   if (!one_in(6))
    g->m.add_field(g, i, j, fd_electricity, rng(1, 3));
  }
 }
}

void mattack::boomer(game *g, monster *z)
{
 int j;
 if (rl_dist(z->pos.x, z->pos.y, g->u.posx, g->u.posy) > 3 || !g->sees_u(z->pos.x, z->pos.y, j)) return;	// Out of range
 std::vector<point> line = line_to(z->pos.x, z->pos.y, g->u.posx, g->u.posy, j);
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 z->moves = -250;			// It takes a while
 bool u_see = g->u_see(z->pos.x, z->pos.y);
 if (u_see) messages.add("The %s spews bile!", z->name().c_str());
 for (int i = 0; i < line.size(); i++) {
  if (g->m.field_at(line[i].x, line[i].y).type == fd_blood) {
   g->m.field_at(line[i].x, line[i].y).type = fd_bile;
   g->m.field_at(line[i].x, line[i].y).density = 1;
  } else if (g->m.field_at(line[i].x, line[i].y).type == fd_bile &&
             g->m.field_at(line[i].x, line[i].y).density < 3)
   g->m.field_at(line[i].x, line[i].y).density++;
  else
   g->m.add_field(g, line[i].x, line[i].y, fd_bile, 1);
// If bile hit a solid tile, return.
  if (g->m.move_cost(line[i].x, line[i].y) == 0) {
   g->m.add_field(g, line[i].x, line[i].y, fd_bile, 3);
   if (g->u_see(line[i].x, line[i].y))
    messages.add("Bile splatters on the %s!", g->m.tername(line[i].x, line[i].y).c_str());
   return;
  }
 }
 if (rng(0, 10) > g->u.dodge(g) || one_in(g->u.dodge(g)))
  g->u.infect(DI_BOOMERED, bp_eyes, 3, 12, g);
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
   if (g->is_empty(x, y) && g->m.sees(z->pos.x, z->pos.y, x, y, -1)) {
    for (int i = 0; i < g->m.i_at(x, y).size(); i++) {
     if (g->m.i_at(x, y)[i].type->id == itm_corpse &&
         g->m.i_at(x, y)[i].corpse->species == species_zombie) {
      corpses.push_back(point(x, y));
      i = g->m.i_at(x, y).size();
     }
    }
   }
  }
 }
 if (corpses.empty()) return;	// No nearby corpses
 z->speed = (z->speed - rng(0, 10)) * .8;
 const bool sees_necromancer = g->u_see(z);
 if (sees_necromancer) messages.add("The %s throws its arms wide...", z->name().c_str());
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 z->moves = -500;			// It takes a while
 int raised = 0;
 for (int i = 0; i < corpses.size(); i++) {
  int x = corpses[i].x, y = corpses[i].y;
  for (int n = 0; n < g->m.i_at(x, y).size(); n++) {
   if (g->m.i_at(x, y)[n].type->id == itm_corpse && one_in(2)) {
    if (g->u_see(x, y)) raised++;
    int burnt_penalty = g->m.i_at(x, y)[n].burnt;
    monster mon(g->m.i_at(x, y)[n].corpse, x, y);
    mon.speed = int(mon.speed * .8) - burnt_penalty / 2;
    mon.hp    = int(mon.hp    * .7) - burnt_penalty;
    g->m.i_rem(x, y, n);
    n = g->m.i_at(x, y).size();	// Only one body raised per tile
    g->z.push_back(mon);
   }
  }
 }
 if (raised > 0) {
  if (raised == 1)
   messages.add("A nearby corpse rises from the dead!");
  else if (raised < 4)
   messages.add("A few corpses rise from the dead!");
  else
   messages.add("Several corpses rise from the dead!");
 } else if (sees_necromancer)
  messages.add("...but nothing seems to happen.");
}

void mattack::science(game *g, monster *z)	// I said SCIENCE again!
{
 int t, dist = rl_dist(z->pos.x, z->pos.y, g->u.posx, g->u.posy);
 if (dist > 5 || !g->sees_u(z->pos.x, z->pos.y, t)) return;	// Out of range
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 std::vector<point> line = line_to(z->pos.x, z->pos.y, g->u.posx, g->u.posy, t);
 std::vector<point> free;
 for (int x = z->pos.x - 1; x <= z->pos.x + 1; x++) {
  for (int y = z->pos.y - 1; y <= z->pos.y + 1; y++) {
   if (g->is_empty(x, y))
    free.push_back(point(x, y));
  }
 }
 std::vector<int> valid;// List of available attacks
 int index;
 if (dist == 1) valid.push_back(1);	// Shock
 if (dist <= 2) valid.push_back(2);	// Radiation
 if (!free.empty()) {
  valid.push_back(3);	// Manhack
  valid.push_back(4);	// Acid pool
 }
 valid.push_back(5);	// Flavor text
 switch (valid[rng(0, valid.size() - 1)]) {	// What kind of attack?
 case 1:	// Shock the player
  messages.add("The %s shocks you!", z->name().c_str());
  z->moves -= 150;
  g->u.hurtall(rng(1, 2));
  if (one_in(6) && !one_in(30 - g->u.str_cur)) {
   messages.add("You're paralyzed!");
   g->u.moves -= 300;
  }
  break;
 case 2:	// Radioactive beam
  messages.add("The %s opens it's mouth and a beam shoots towards you!", z->name().c_str());
  z->moves -= 400;
  if (g->u.dodge(g) > rng(1, 16))
   messages.add("You dodge the beam!");
  else if (one_in(6))
   g->u.mutate(g);
  else {
   messages.add("You get pins and needles all over.");
   g->u.radiation += rng(20, 50);
  }
  break;
 case 3:	// Spawn a manhack
  {
  messages.add("The %s opens its coat, and a manhack flies out!", z->name().c_str());
  z->moves -= 200;
  index = rng(0, valid.size() - 1);
  g->z.push_back(monster(mtype::types[mon_manhack], free[index].x, free[index].y));
  }
  break;
 case 4:	// Acid pool
  messages.add("The %s drops a flask of acid!", z->name().c_str());
  z->moves -= 100;
  for(const auto& pt : free) g->m.add_field(g, pt.x, pt.y, fd_acid, 3);
  break;
 case 5:	// Flavor text
  switch (rng(1, 4)) {
  case 1:
   messages.add("The %s gesticulates wildly!", z->name().c_str());
   break;
  case 2:
   messages.add("The %s coughs up a strange dust.", z->name().c_str());
   break;
  case 3:
   messages.add("The %s moans softly.", z->name().c_str());
   break;
  case 4:
   messages.add("The %s's skin crackles with electricity.", z->name().c_str());
   z->moves -= 80;
   break;
  }
  break;
 }
}

void mattack::growplants(game *g, monster *z)
{
 for (int i = -3; i <= 3; i++) {
  for (int j = -3; j <= 3; j++) {
   if (i == 0 && j == 0) j++;
   if (!g->m.has_flag(diggable, z->pos.x + i, z->pos.y + j) && one_in(4))
    g->m.ter(z->pos.x + i, z->pos.y + j) = t_dirt;
   else if (one_in(3) && g->m.is_destructable(z->pos.x + i, z->pos.y + j))
    g->m.ter(z->pos.x + i, z->pos.y + j) = t_dirtmound; // Destroy walls, &c
   else {
    if (one_in(4)) {	// 1 in 4 chance to grow a tree
     if (monster* const m_at = g->mon(z->pos.x + i, z->pos.y + j)) {
      if (g->u_see(z->pos.x + i, z->pos.y + j))
       messages.add("A tree bursts forth from the earth and pierces the %s!", m_at->name().c_str());
      int rn = rng(10, 30) - m_at->armor_cut();
      if (rn < 0) rn = 0;
      if (m_at->hurt(rn)) g->kill_mon(*m_at, (z->friendly != 0));
     } else if (g->u.posx == z->pos.x + i && g->u.posy == z->pos.y + j) {
// Player is hit by a growing tree
      body_part hit = bp_legs;
      int side = rng(1, 2);
      if (one_in(4)) hit = bp_torso;
      else if (one_in(2)) hit = bp_feet;
	  messages.add("A tree bursts forth from the earth and pierces your %s!",
                 body_part_name(hit, side).c_str());
      g->u.hit(g, hit, side, 0, rng(10, 30));
     } else if (npc* const nPC = g->nPC(z->pos.x + i, z->pos.y + j)) {	// An NPC got hit
       body_part hit = bp_legs;
       int side = rng(1, 2);
       if (one_in(4)) hit = bp_torso;
       else if (one_in(2)) hit = bp_feet;
       if (g->u_see(z->pos.x + i, z->pos.y + j))
        messages.add("A tree bursts forth from the earth and pierces %s's %s!",
			nPC->name.c_str(), body_part_name(hit, side).c_str());
	   nPC->hit(g, hit, side, 0, rng(10, 30));
     }
     g->m.ter(z->pos.x + i, z->pos.y + j) = t_tree_young;
    } else if (one_in(3)) // If no tree, perhaps underbrush
     g->m.ter(z->pos.x + i, z->pos.y + j) = t_underbrush;
   }
  }
 }

 if (one_in(5)) { // 1 in 5 chance of making exisiting vegetation grow larger
  for (int i = -5; i <= 5; i++) {
   for (int j = -5; j <= 5; j++) {
    if (i != 0 || j != 0) {
     if (g->m.ter(z->pos.x + i, z->pos.y + j) == t_tree_young)
      g->m.ter(z->pos.x + i, z->pos.y + j) = t_tree; // Young tree => tree
     else if (g->m.ter(z->pos.x + i, z->pos.y + j) == t_underbrush) {
// Underbrush => young tree
	  monster* const m_at = g->mon(z->pos.x + i, z->pos.y + j);
      if (m_at) {
       if (g->u_see(z->pos.x + i, z->pos.y + j))
        messages.add("Underbrush forms into a tree, and it pierces the %s!", m_at->name().c_str());
       int rn = rng(10, 30) - m_at->armor_cut();
       if (rn < 0) rn = 0;
       if (m_at->hurt(rn)) g->kill_mon(*m_at, (z->friendly != 0));
      } else if (g->u.posx == z->pos.x + i && g->u.posy == z->pos.y + j) {
       body_part hit = bp_legs;
       int side = rng(1, 2);
       if (one_in(4)) hit = bp_torso;
       else if (one_in(2)) hit = bp_feet;
	   messages.add("The underbrush beneath your feet grows and pierces your %s!",
                  body_part_name(hit, side).c_str());
       g->u.hit(g, hit, side, 0, rng(10, 30));
      } else {
       if (npc* const nPC = g->nPC(z->pos.x + i, z->pos.y + j)) {
        body_part hit = bp_legs;
        int side = rng(1, 2);
        if (one_in(4)) hit = bp_torso;
        else if (one_in(2)) hit = bp_feet;
        if (g->u_see(z->pos.x + i, z->pos.y + j))
         messages.add("Underbrush grows into a tree, and it pierces %s's %s!",
			 nPC->name.c_str(), body_part_name(hit, side).c_str());
		nPC->hit(g, hit, side, 0, rng(10, 30));
       }
      }
     }
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
 bool hit_u = false;
 std::vector<point> grow;
 int vine_neighbors = 0;
 z->sp_timeout = z->type->sp_freq;
 z->moves -= 100;
 for (int x = z->pos.x - 1; x <= z->pos.x + 1; x++) {
  for (int y = z->pos.y - 1; y <= z->pos.y + 1; y++) {
   if (g->u.posx == x && g->u.posy == y) {
    body_part bphit = random_body_part();
    int side = rng(0, 1);
    messages.add("The %s lashes your %s!", z->name().c_str(), body_part_name(bphit, side).c_str());
    g->u.hit(g, bphit, side, 4, 4);
    z->sp_timeout = z->type->sp_freq;
    z->moves -= 100;
    return;
   } else if (g->is_empty(x, y)) grow.push_back(point(x, y));
   else if (monster* const m_at = g->mon(x, y)) {
    if (m_at->type->id == mon_creeper_vine) vine_neighbors++;
   }
  }
 }
// Calculate distance from nearest hub
 int dist_from_hub = 999;
 for (int i = 0; i < g->z.size(); i++) {
  if (g->z[i].type->id == mon_creeper_hub) {
   int dist = rl_dist(z->pos, g->z[i].pos);
   if (dist < dist_from_hub) dist_from_hub = dist;
  }
 }
 if (grow.empty() || vine_neighbors > 5 || one_in(7 - vine_neighbors) || !one_in(dist_from_hub)) return;
 int index = rng(0, grow.size() - 1);
 monster vine(mtype::types[mon_creeper_vine], grow[index].x, grow[index].y);
 vine.sp_timeout = 5;
 g->z.push_back(vine);
}

void mattack::spit_sap(game *g, monster *z)
{
// TODO: Friendly biollantes?
 if (rl_dist(z->pos.x, z->pos.y, g->u.posx, g->u.posy) > 12 || !g->sees_u(z->pos.x, z->pos.y)) return;

 z->moves -= 150;
 z->sp_timeout = z->type->sp_freq;

 int dist = rl_dist(z->pos.x, z->pos.y, g->u.posx, g->u.posy);
 int deviation = rng(1, 10);
 double missed_by = (.0325 * deviation * dist);

 if (missed_by > 1.) {
  if (g->u_see(z->pos.x, z->pos.y))
   messages.add("The %s spits sap, but misses you.", z->name().c_str());

  int hitx = g->u.posx + rng(0 - int(missed_by), int(missed_by)),
      hity = g->u.posy + rng(0 - int(missed_by), int(missed_by));
  std::vector<point> line = line_to(z->pos.x, z->pos.y, hitx, hity, 0);
  int dam = 5;
  for (int i = 0; i < line.size() && dam > 0; i++) {
   g->m.shoot(g, line[i].x, line[i].y, dam, false, 0);
   if (dam == 0 && g->u_see(line[i].x, line[i].y)) {
    messages.add("A glob of sap hits the %s!", g->m.tername(line[i].x, line[i].y).c_str());
    return;
   }
  }
  g->m.add_field(g, hitx, hity, fd_sap, (dam >= 4 ? 3 : 2));
  return;
 }

 int t = 0;
 if (g->u_see(z->pos.x, z->pos.y, t)) messages.add("The %s spits sap!", z->name().c_str());
 std::vector<point> line = line_to(z->pos.x, z->pos.y, g->u.posx, g->u.posy, t);
 int dam = 5;
 for (int i = 0; i < line.size() && dam > 0; i++) {
  g->m.shoot(g, line[i].x, line[i].y, dam, false, 0);
  if (dam == 0 && g->u_see(line[i].x, line[i].y)) {
   messages.add("A glob of sap hits the %s!", g->m.tername(line[i].x, line[i].y).c_str());
   return;
  }
 }
 if (dam <= 0) return;
 messages.add("A glob of sap hits you!");
 g->u.hit(g, bp_torso, 0, dam, 0);
 g->u.add_disease(DI_SAP, dam, g);
}

void mattack::triffid_heartbeat(game *g, monster *z)
{
 g->sound(z->pos, 14, "thu-THUMP.");
 z->moves -= 300;
 z->sp_timeout = z->type->sp_freq;
 if ((z->pos.x < 0 || z->pos.x >= SEEX * MAPSIZE) &&
     (z->pos.y < 0 || z->pos.y >= SEEY * MAPSIZE)   )
  return;
 if (rl_dist(z->pos.x, z->pos.y, g->u.posx, g->u.posy) > 5 &&
     !g->m.route(g->u.posx, g->u.posy, z->pos.x, z->pos.y).empty()) {
  messages.add("The root walls creak around you.");
  for (int x = g->u.posx; x <= z->pos.x - 3; x++) {
   for (int y = g->u.posy; y <= z->pos.y - 3; y++) {
    if (g->is_empty(x, y) && one_in(4))
     g->m.ter(x, y) = t_root_wall;
    else if (g->m.ter(x, y) == t_root_wall && one_in(10))
     g->m.ter(x, y) = t_dirt;
   }
  }
// Open blank tiles as long as there's no possible route
  int tries = 0;
  while (g->m.route(g->u.posx, g->u.posy, z->pos.x, z->pos.y).empty() && tries < 20) {
   int x = rng(g->u.posx, z->pos.x - 3), y = rng(g->u.posy, z->pos.y - 3);
   tries++;
   g->m.ter(x, y) = t_dirt;
   if (rl_dist(x, y, g->u.posx, g->u.posy > 3 && g->z.size() < 30 &&
       g->mon_at(x, y) == -1 && one_in(20))) { // Spawn an extra monster
    mon_id montype = mon_triffid;
    if (one_in(4)) montype = mon_creeper_hub;
    else if (one_in(3)) montype = mon_biollante;
    monster plant(mtype::types[montype], x, y);
    g->z.push_back(plant);
   }
  }

 } else { // The player is close enough for a fight!

  monster triffid(mtype::types[mon_triffid]);
  for (int x = z->pos.x - 1; x <= z->pos.x + 1; x++) {
   for (int y = z->pos.y - 1; y <= z->pos.y + 1; y++) {
    if (g->is_empty(x, y) && one_in(2)) {
     triffid.spawn(x, y);
     g->z.push_back(triffid);
    }
   }
  }

 }
}

void mattack::fungus(game *g, monster *z)
{
 if (g->z.size() > 100) return; // Prevent crowding the monster list.
// TODO: Infect NPCs?
 z->moves = -200;			// It takes a while
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 monster spore(mtype::types[mon_spore]);
 int sporex, sporey;
 int moncount = 0;
 g->sound(z->pos, 10, "Pouf!");
 if (g->u_see(z->pos.x, z->pos.y)) messages.add("Spores are released from the %s!", z->name().c_str());
 for (int i = -1; i <= 1; i++) {
  for (int j = -1; j <= 1; j++) {
   if (i == 0 && j == 0) j++;	// No need to check 0, 0
   sporex = z->pos.x + i;
   sporey = z->pos.y + j;
   if (g->m.move_cost(sporex, sporey) > 0 && one_in(5)) {
	monster* const m_at = g->mon(sporex, sporey);
    if (m_at) {	// Spores hit a monster
     if (g->u_see(sporex, sporey)) messages.add("The %s is covered in tiny spores!", m_at->name().c_str());
     if (!m_at->make_fungus(g)) g->kill_mon(*m_at, (z->friendly != 0));
    } else if (g->u.posx == sporex && g->u.posy == sporey)
     g->u.infect(DI_SPORES, bp_mouth, 4, 30, g); // Spores hit the player
    else { // Spawn a spore
     spore.spawn(sporex, sporey);
     g->z.push_back(spore);
    }
   }
  }
 }
 if (moncount >= 7) z->poly(mtype::types[mon_fungaloid_dormant]);	// If we're surrounded by monsters, go dormant
}

void mattack::fungus_sprout(game *g, monster *z)
{
 for (int x = z->pos.x - 1; x <= z->pos.x + 1; x++) {
  for (int y = z->pos.y - 1; y <= z->pos.y + 1; y++) {
   if (g->u.posx == x && g->u.posy == y) {
    messages.add("You're shoved away as a fungal wall grows!");
    g->teleport();
   }
   if (g->is_empty(x, y)) g->z.push_back(monster(mtype::types[mon_fungal_wall], x, y));
  }
 }
}

void mattack::leap(game *g, monster *z)
{
 if (!g->sees_u(z->pos.x, z->pos.y)) return;	// Only leap if we can see you!

 std::vector<point> options;
 int best = 0;
 bool fleeing = z->is_fleeing(g->u);

 for (int x = z->pos.x - 3; x <= z->pos.x + 3; x++) {
  for (int y = z->pos.y - 3; y <= z->pos.y + 3; y++) {
/* If we're fleeing, we want to pick those tiles with the greatest distance
 * from the player; otherwise, those tiles with the least distance from the
 * player.
 */
   if (g->is_empty(x, y) && g->m.sees(z->pos.x, z->pos.y, x, y, g->light_level()) &&
       (( fleeing && rl_dist(g->u.posx, g->u.posy, x, y) >= best) ||
        (!fleeing && rl_dist(g->u.posx, g->u.posy, x, y) <= best)   )) {
    options.push_back( point(x, y) );
    best = rl_dist(g->u.posx, g->u.posy, x, y);
   }
  }
 }

// Go back and remove all options that aren't tied for best
 for (int i = 0; i < options.size() && options.size() > 1; i++) {
  point p = options[i];
  if (rl_dist(g->u.posx, g->u.posy, options[i].x, options[i].y) != best) {
   options.erase(options.begin() + i);
   i--;
  }
 }

 if (options.size() == 0) return; // Nowhere to leap!

 z->moves -= 150;
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 point chosen = options[rng(0, options.size() - 1)];
 bool seen = g->u_see(z); // We can see them jump...
 z->pos = chosen;
 seen |= g->u_see(z); // ... or we can see them land
 if (seen) messages.add("The %s leaps!", z->name().c_str());
}

void mattack::dermatik(game *g, monster *z)
{
 if (rl_dist(z->pos.x, z->pos.y, g->u.posx, g->u.posy) > 1 || g->u.has_disease(DI_DERMATIK))
  return; // Too far to implant, or the player's already incubating bugs

 z->sp_timeout = z->type->sp_freq;	// Reset timer

// Can we dodge the attack?
 int attack_roll = dice(z->type->melee_skill, 10);
 int player_dodge = g->u.dodge_roll(g);
 if (player_dodge > attack_roll) {
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
             z->name().c_str(), body_part_name(targeted, rng(0, 1)).c_str());
  z->moves -= 150; // Attemped laying takes a while
  return;
 }

// Success!
 z->moves -= 500; // Successful laying takes a long time
 messages.add("The %s sinks its ovipositor into you!", z->name().c_str());
 g->u.add_disease(DI_DERMATIK, -1, g); // -1 = infinite
}

void mattack::plant(game *g, monster *z)
{
// Spores taking seed and growing into a fungaloid
 if (g->m.has_flag(diggable, z->pos.x, z->pos.y)) {
  if (g->u_see(z->pos.x, z->pos.y))
   messages.add("The %s takes seed and becomes a young fungaloid!", z->name().c_str());
  z->poly(mtype::types[mon_fungaloid_young]);
  z->moves = -1000;	// It takes a while
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
   if (g->u.posx == test.x && g->u.posy == test.y) {
// If we hit the player, cover them with slime
    didit = true;
    g->u.add_disease(DI_SLIMED, rng(0, z->hp), g);
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
    monster blob(mtype::types[mon_blob_small], test.x, test.y);
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
 if (!g->u_see(z) || !one_in(3)) return;

 messages.add("The %s's head explodes in a mass of roiling tentacles!", z->name().c_str());

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
 int t;
 if (!g->sees_u(z->pos.x, z->pos.y, t)) return;

 messages.add("The %s lashes its tentacle at you!", z->name().c_str());
 z->moves -= 100;
 z->sp_timeout = z->type->sp_freq;	// Reset timer

 std::vector<point> line = line_to(z->pos.x, z->pos.y, g->u.posx, g->u.posy, t);
 for (int i = 0; i < line.size(); i++) {
  int tmpdam = 20;
  g->m.shoot(g, line[i].x, line[i].y, tmpdam, true, 0);
 }

 if (rng(0, 20) > g->u.dodge(g) || one_in(g->u.dodge(g))) {
  messages.add("You dodge it!");
  return;
 }
 body_part hit = random_body_part();
 int dam = rng(10, 20), side = rng(0, 1);
 messages.add("Your %s is hit for %d damage!", body_part_name(hit, side).c_str(), dam);
 g->u.hit(g, hit, side, dam, 0);
}

void mattack::vortex(game *g, monster *z)
{
// Make sure that the player's butchering is interrupted!
 if (g->u.activity.type == ACT_BUTCHER && rl_dist(z->pos.x, z->pos.y, g->u.posx, g->u.posy) <= 2) {
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
   g->sound(x, y, 8, sound);
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
      g->m.shoot(g, traj[i].x, traj[i].y, dam, false, 0);
	  monster* const m_at = g->mon(traj[i]);
      if (m_at) {
       if (m_at->hurt(dam)) g->kill_mon(*m_at, (z->friendly != 0));
       dam = 0;
      }
      if (g->m.move_cost(traj[i].x, traj[i].y) == 0) {
       dam = 0;
       i--;
      } else if (traj[i].x == g->u.posx && traj[i].y == g->u.posy) {
       body_part hit = random_body_part();
       int side = rng(0, 1);
	   messages.add("A %s hits your %s for %d damage!", thrown.tname().c_str(),
                  body_part_name(hit, side).c_str(), dam);
       g->u.hit(g, hit, side, dam, 0);
       dam = 0;
      }
// TODO: Hit NPCs
      if (dam == 0 || i == traj.size() - 1) {
       if (thrown.made_of(GLASS)) {
        if (g->u_see(traj[i].x, traj[i].y))
         messages.add("The %s shatters!", thrown.tname().c_str());
        for (int n = 0; n < thrown.contents.size(); n++)
         g->m.add_item(traj[i].x, traj[i].y, thrown.contents[n]);
        g->sound(traj[i], 16, "glass breaking!");
       } else
        g->m.add_item(traj[i].x, traj[i].y, thrown);
      }
     }
    } // Done throwing item
   } // Done getting items
// Throw monsters
   monster* const thrown = g->mon(x,y);
   if (thrown) {
    int distance = 0, damage = 0;
    switch (thrown->type->size) {
     case MS_TINY:   distance = 5; break;
     case MS_SMALL:  distance = 3; break;
     case MS_MEDIUM: distance = 2; break;
     case MS_LARGE:  distance = 1; break;
     case MS_HUGE:   distance = 0; break;
    }
    damage = distance * 4;
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
     case IRON:    distance -= 1; // fall through
     case STEEL:
     case SILVER:  distance -= 3; damage -= 10; break;
    }
    if (distance > 0) {
     if (g->u_see(thrown)) messages.add("The %s is thrown by winds!", thrown->name().c_str());
     std::vector<point> traj = continue_line(from_monster, distance);
     bool hit_wall = false;
     for (int i = 0; i < traj.size() && !hit_wall; i++) {
      monster* const m_hit = g->mon(traj[i]);
      if (i > 0 && m_hit && !m_hit->has_flag(MF_DIGS)) {
       if (g->u_see(traj[i].x, traj[i].y)) messages.add("The %s hits a %s!", thrown->name().c_str(), m_hit->name().c_str());
       if (m_hit->hurt(damage)) g->kill_mon(*m_hit, (z->friendly != 0));
       hit_wall = true;
       thrown->pos = traj[i - 1];
      } else if (g->m.move_cost(traj[i].x, traj[i].y) == 0) {
       hit_wall = true;
       thrown->pos = traj[i - 1];
      }
      int damage_copy = damage;
      g->m.shoot(g, traj[i].x, traj[i].y, damage_copy, false, 0);
      if (damage_copy < damage) thrown->hurt(damage - damage_copy);
     }
     if (hit_wall) damage *= 2;
     else  thrown->pos = traj[traj.size() - 1];
     if (thrown->hurt(damage)) g->kill_mon(*thrown, (z->friendly != 0));
    } // if (distance > 0)
   } // if (thrown)

   if (g->u.posx == x && g->u.posy == y) { // Throw... the player?! D:
    std::vector<point> traj = continue_line(from_monster, rng(2, 3));
    messages.add("You're thrown by winds!");
    bool hit_wall = false;
    int damage = rng(5, 10);
    for (int i = 0; i < traj.size() && !hit_wall; i++) {
	 monster* const m_hit = g->mon(traj[i]);
     if (i > 0 && m_hit && !m_hit->has_flag(MF_DIGS)) {
      if (g->u_see(traj[i].x, traj[i].y)) messages.add("You hit a %s!", m_hit->name().c_str());
      if (m_hit->hurt(damage)) g->kill_mon(*m_hit, true); // We get the kill :)
      hit_wall = true;
      g->u.posx = traj[i - 1].x;
      g->u.posy = traj[i - 1].y;
     } else if (g->m.move_cost(traj[i].x, traj[i].y) == 0) {
      messages.add("You slam into a %s", g->m.tername(traj[i].x, traj[i].y).c_str());
      hit_wall = true;
      g->u.posx = traj[i - 1].x;
      g->u.posy = traj[i - 1].y;
     }
     int damage_copy = damage;
     g->m.shoot(g, traj[i].x, traj[i].y, damage_copy, false, 0);
     if (damage_copy < damage)
      g->u.hit(g, bp_torso, 0, damage - damage_copy, 0);
    }
    if (hit_wall) damage *= 2;
    else {
     g->u.posx = traj[traj.size() - 1].x;
     g->u.posy = traj[traj.size() - 1].y;
    }
    g->u.hit(g, bp_torso, 0, damage, 0);
    g->update_map(g->u.posx, g->u.posy);
   } // Done with checking for player
  }
 } // Done with loop!
}

void mattack::gene_sting(game *g, monster *z)
{
 if (rl_dist(z->pos.x, z->pos.y, g->u.posx, g->u.posy) > 7 || !g->sees_u(z->pos.x, z->pos.y)) return;	// Not within range and/or sight

 z->moves -= 150;
 z->sp_timeout = z->type->sp_freq;
 messages.add("The %s shoots a dart into you!", z->name().c_str());
 g->u.mutate(g);
}

void mattack::stare(game *g, monster *z)
{
 z->moves -= 200;
 z->sp_timeout = z->type->sp_freq;
 if (g->sees_u(z->pos.x, z->pos.y)) {
  messages.add("The %s stares at you, and you shudder.", z->name().c_str());
  g->u.add_disease(DI_TELEGLOW, 800, g);
 } else {
  messages.add("A piercing beam of light bursts forth!");
  std::vector<point> sight = line_to(z->pos.x, z->pos.y, g->u.posx, g->u.posy, 0);
  for (int i = 0; i < sight.size(); i++) {
   if (g->m.ter(sight[i].x, sight[i].y) == t_reinforced_glass_h ||
       g->m.ter(sight[i].x, sight[i].y) == t_reinforced_glass_v)
    i = sight.size();
   else if (g->m.is_destructable(sight[i].x, sight[i].y))
    g->m.ter(sight[i].x, sight[i].y) = t_rubble;
  }
 }
}

void mattack::fear_paralyze(game *g, monster *z)
{
 if (g->u_see(z->pos.x, z->pos.y)) {
  z->sp_timeout = z->type->sp_freq;	// Reset timer
  if (g->u.has_artifact_with(AEP_PSYSHIELD)) {
   messages.add("The %s probes your mind, but is rebuffed!", z->name().c_str());
  } else if (rng(1, 20) > g->u.int_cur) {
   messages.add("The terrifying visage of the %s paralyzes you.", z->name().c_str());
   g->u.moves -= 100;
  } else
   messages.add("You manage to avoid staring at the horrendous %s.", z->name().c_str());
 }
}

void mattack::photograph(game *g, monster *z)
{
 if (z->faction_id == -1 ||
     rl_dist(z->pos.x, z->pos.y, g->u.posx, g->u.posy) > 6 ||
     !g->sees_u(z->pos.x, z->pos.y))
  return;
 z->sp_timeout = z->type->sp_freq;
 z->moves -= 150;
 messages.add("The %s takes your picture!", z->name().c_str());
// TODO: Make the player known to the faction
 g->add_event(EVENT_ROBOT_ATTACK, int(messages.turn) + rng(15, 30), z->faction_id, g->lev.x, g->lev.y);
}

void mattack::tazer(game *g, monster *z)
{
 if (rl_dist(z->pos.x, z->pos.y, g->u.posx, g->u.posy) > 2 || !g->sees_u(z->pos.x, z->pos.y)) return;	// Out of range
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 z->moves = -200;			// It takes a while
 messages.add("The %s shocks you!", z->name().c_str());
 int shock = rng(1, 5);
 g->u.hurt(g, bp_torso, 0, shock * rng(1, 3));
 g->u.moves -= shock * 20;
}

void mattack::smg(game *g, monster *z)
{
 int t, fire_t;
 if (z->friendly != 0) { // Attacking monsters, not the player!
  monster* target = NULL;
  int closest = 19;
  for (int i = 0; i < g->z.size(); i++) {
   int dist = rl_dist(z->pos, g->z[i].pos);
   if (g->z[i].friendly == 0 && dist < closest && 
       g->m.sees(z->pos.x, z->pos.y, g->z[i].pos.x, g->z[i].pos.y, 18, t)) {
    target = &(g->z[i]);
    closest = dist;
    fire_t = t;
   }
  }
  z->sp_timeout = z->type->sp_freq;	// Reset timer
  if (target == NULL) return; // Couldn't find any targets!
  z->moves = -150;			// It takes a while
  if (g->u_see(z->pos.x, z->pos.y)) messages.add("The %s fires its smg!", z->name().c_str());
  player tmp;
  tmp.name = "The " + z->name();
  tmp.sklevel[sk_smg] = 1;
  tmp.sklevel[sk_gun] = 0;
  tmp.recoil = 0;
  tmp.posx = z->pos.x;
  tmp.posy = z->pos.y;
  tmp.str_cur = 16;
  tmp.dex_cur =  6;
  tmp.per_cur =  8;
  tmp.weapon = item(item::types[itm_smg_9mm], 0);
  tmp.weapon.curammo = dynamic_cast<const it_ammo*>(item::types[itm_9mm]);
  tmp.weapon.charges = 10;
  std::vector<point> traj = line_to(z->pos.x, z->pos.y, target->pos.x, target->pos.y, fire_t);
  g->fire(tmp, target->pos.x, target->pos.y, traj, true);

  return;
 }
 
// Not friendly; hence, firing at the player
 if (rl_dist(z->pos.x, z->pos.y, g->u.posx, g->u.posy) > 24 || !g->sees_u(z->pos.x, z->pos.y, t)) return;
 z->sp_timeout = z->type->sp_freq;	// Reset timer

 if (!z->has_effect(ME_TARGETED)) {
  g->sound(z->pos, 6, "beep-beep-beep!");
  z->add_effect(ME_TARGETED, 8);
  z->moves -= 100;
  return;
 }
 z->moves = -150;			// It takes a while

 if (g->u_see(z->pos.x, z->pos.y)) messages.add("The %s fires its smg!", z->name().c_str());
// Set up a temporary player to fire this gun
 player tmp;
 tmp.name = "The " + z->name();
 tmp.sklevel[sk_smg] = 1;
 tmp.sklevel[sk_gun] = 0;
 tmp.recoil = 0;
 tmp.posx = z->pos.x;
 tmp.posy = z->pos.y;
 tmp.str_cur = 16;
 tmp.dex_cur =  6;
 tmp.per_cur =  8;
 tmp.weapon = item(item::types[itm_smg_9mm], 0);
 tmp.weapon.curammo = dynamic_cast<const it_ammo*>(item::types[itm_9mm]);
 tmp.weapon.charges = 10;
 std::vector<point> traj = line_to(z->pos.x, z->pos.y, g->u.posx, g->u.posy, t);
 g->fire(tmp, g->u.posx, g->u.posy, traj, true);
 z->add_effect(ME_TARGETED, 3);
}

void mattack::flamethrower(game *g, monster *z)
{
 int t;
 if (abs(g->u.posx - z->pos.x) > 5 || abs(g->u.posy - z->pos.y) > 5 || !g->sees_u(z->pos.x, z->pos.y, t)) return;	// Out of range
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 z->moves = -500;			// It takes a while
 std::vector<point> traj = line_to(z->pos.x, z->pos.y, g->u.posx, g->u.posy, t);
 for (int i = 0; i < traj.size(); i++)
  g->m.add_field(g, traj[i].x, traj[i].y, fd_fire, 1);
 g->u.add_disease(DI_ONFIRE, 8, g);
}

void mattack::copbot(game *g, monster *z)
{
 const bool sees_u = g->sees_u(z->pos.x, z->pos.y);
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 if (rl_dist(z->pos.x, z->pos.y, g->u.posx, g->u.posy) > 2 || !sees_u) {
  if (one_in(3)) {
   if (sees_u) {
    if (g->u.unarmed_attack())
     g->sound(z->pos, 18, "a robotic voice boom, \"Citizen, Halt!\"");
    else
     g->sound(z->pos, 18, "a robotic voice boom, \"Please put down your weapon.\"");
   } else
    g->sound(z->pos, 18, "a robotic voice boom, \"Come out with your hands up!\"");
  } else
   g->sound(z->pos, 18, "a police siren, whoop WHOOP");
  return;
 }
 tazer(g, z);
}

void mattack::multi_robot(game *g, monster *z)
{
 int mode = 0;
 if (!g->sees_u(z->pos.x, z->pos.y)) return;	// Can't see you!
 {
 const auto range = rl_dist(z->pos.x, z->pos.y, g->u.posx, g->u.posy);
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
 if (rl_dist(z->pos.x, z->pos.y, g->u.posx, g->u.posy) > 10) return;
 z->sp_timeout = z->type->sp_freq;	// Reset timer

 switch (rng(1, 5)) { // What do we say?
  case 1: messages.add("\"YOU... ARE FILTH...\""); break;
  case 2: messages.add("\"VERMIN... YOU ARE VERMIN...\""); break;
  case 3: messages.add("\"LEAVE NOW...\""); break;
  case 4: messages.add("\"WE... WILL FEAST... UPON YOU...\""); break;
  case 5: messages.add("\"FOUL INTERLOPER...\""); break;
 }

 g->u.add_disease(DI_RAT, 20, g);
}

void mattack::generator(game *g, monster *z)
{
 g->sound(z->pos, 100, "");
 if (int(messages.turn) % 10 == 0 && z->hp < z->type->hp) z->hp++;
}

void mattack::upgrade(game *g, monster *z)
{
 std::vector<int> targets;
 for (int i = 0; i < g->z.size(); i++) {
  if (g->z[i].type->id == mon_zombie && rl_dist(z->pos, g->z[i].pos) <= 5)
   targets.push_back(i);
 }
 if (targets.empty()) return;
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 z->moves -= 150;			// It takes a while

 monster *target = &( g->z[ targets[ rng(0, targets.size()-1) ] ] );

 mon_id newtype = mon_zombie;
 
 switch( rng(1, 10) ) {
  case  1: newtype = mon_zombie_shrieker;
           break;
  case  2:
  case  3: newtype = mon_zombie_spitter;
           break;
  case  4:
  case  5: newtype = mon_zombie_electric;
           break;
  case  6:
  case  7:
  case  8: newtype = mon_zombie_fast;
           break;
  case  9: newtype = mon_zombie_brute;
           break;
  case 10: newtype = mon_boomer;
           break;
 }

 target->poly(mtype::types[newtype]);
 if (g->u_see(z->pos.x, z->pos.y))
  messages.add("The black mist around the %s grows...", z->name().c_str());
 if (g->u_see(target->pos.x, target->pos.y))
  messages.add("...a zombie becomes a %s!", target->name().c_str());
}

void mattack::breathe(game *g, monster *z)
{
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 z->moves -= 100;			// It takes a while

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
 for (int x = z->pos.x - 1; x <= z->pos.x + 1; x++) {
  for (int y = z->pos.y - 1; y <= z->pos.y + 1; y++) {
   if (g->is_empty(x, y)) valid.push_back( point(x, y) );
  }
 }

 if (!valid.empty()) {
  point place = valid[ rng(0, valid.size() - 1) ];
  monster spawned(mtype::types[mon_breather], place.x, place.y);
  spawned.sp_timeout = 12;
  g->z.push_back(spawned);
 }
}
