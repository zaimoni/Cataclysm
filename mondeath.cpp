#include "mondeath.h"
#include "monster.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "recent_msg.h"

void mdeath::normal(game *g, monster *z)
{
 if (g->u.see(*z)) messages.add("It dies!");
 if (z->made_of(FLESH) && z->has_flag(MF_WARM)) {
  auto& f = g->m.field_at(z->pos);
  if (f.type == fd_blood) {
	  if (f.density < 3) f.density++;
  } else g->m.add_field(g, z->pos, fd_blood, 1);
 }
// Drop a dang ol' corpse
// If their hp is less than -50, we destroyed them so badly no corpse was left
 if ((z->hp >= -50 || z->hp >= 0 - 2 * z->type->hp) && (z->made_of(FLESH) || z->made_of(VEGGY))) {
  g->m.add_item(z->pos, item(messages.turn, z->type->id));
 }
}

void mdeath::acid(game *g, monster *z)
{
 if (g->u.see(*z))
     messages.add("%s corpse melts into a pool of acid.",
                   grammar::capitalize(z->desc(grammar::noun::role::possessive, grammar::article::definite)).c_str());
 g->m.add_field(g, z->pos, fd_acid, 3);
}

void mdeath::boomer(game *g, monster *z)
{
 g->sound(z->pos, 24, "a boomer explode!");
 for (int i = -1; i <= 1; i++) {
  for (int j = -1; j <= 1; j++) {
   const point dest(z->pos.x + i, z->pos.y + j);
   g->m.bash(dest, 10);	// no need to spam player with sounds
   {
   auto& f = g->m.field_at(dest);
   if (f.type == fd_bile) {
	   if (f.density < 3) f.density++;
   } else g->m.add_field(g, dest, fd_bile, 1);
   }
   if (monster* const m_at = g->mon(dest)) {
	m_at->stumble(g, false);
	m_at->moves -= (mobile::mp_turn / 2) * 5;
   }
  }
 }
 if (rl_dist(z->GPSpos, g->u.GPSpos) == 1)
  g->u.infect(DI_BOOMERED, bp_eyes, 2, 24);	// V 0.2.1 npcs
}

void mdeath::kill_vines(game *g, monster *z)
{
 std::vector<monster*> vines;
 std::vector<const monster*> hubs;
 for (decltype(auto) _mon : g->z) {
     switch (_mon.type->id)
     {
     case mon_creeper_hub:
         if (_mon.GPSpos != z->GPSpos) hubs.push_back(&_mon);
         break;
     case mon_creeper_vine:
         vines.push_back(&_mon);
         break;
     }
 }

 for (auto vine : vines) {
  int dist = rl_dist(vine->GPSpos, z->GPSpos);
  bool closer_hub = false;
  for (auto hub : hubs) {
   if (rl_dist(vine->GPSpos, hub->GPSpos) < dist) {
    closer_hub = true;
	break;
   }
  }
  if (!closer_hub) vine->hp = 0;
 }
}

void mdeath::vine_cut(game *g, monster *z)
{
 std::vector<monster*> vines;
 for (decltype(auto) dir : Direction::vector) {
     if (monster* const m_at = g->mon(z->pos + dir)) {
         if (mon_creeper_vine == m_at->type->id) vines.push_back(m_at);
     }
 }

 for(auto vine : vines) {
  bool found_neighbor = false;
  for (decltype(auto) dir : Direction::vector) {
      if (const monster* const m_at = g->mon(vine->pos + dir)) {
          if (m_at->type->id == mon_creeper_hub || m_at->type->id == mon_creeper_vine) {
              found_neighbor = true;
              break;
          }
      }
  }
  if (!found_neighbor) vine->hp = 0;
 }
}

void mdeath::triffid_heart(game *g, monster *z)
{
 messages.add("The root walls begin to crumble around you.");
 g->add_event(EVENT_ROOTS_DIE, int(messages.turn) + MINUTES(10));
}

void mdeath::fungus(game *g, monster *z)
{
 monster spore(mtype::types[mon_spore]);
 g->sound(z->pos, 10, "Pouf!");
 for (int i = -1; i <= 1; i++) {
  for (int j = -1; j <= 1; j++) {
   point dest(z->pos.x + i, z->pos.y + j);
   if (g->m.move_cost(dest) > 0 && one_in(5)) {
	monster* const m_at = g->mon(dest);
    if (m_at) {	// Spores hit a monster
     if (g->u_see(dest)) messages.add("The %s is covered in tiny spores!", m_at->name().c_str());
     if (!m_at->make_fungus()) g->kill_mon(*m_at, z);
    } else if (g->u.pos == dest)	// V 0.2.1 \todo infect npcs
     g->u.infect(DI_SPORES, bp_mouth, 4, 30);
    else {
     spore.spawn(dest);
     g->z.push_back(spore);
    }
   }
  }
 }
}

void mdeath::fungusawake(game *g, monster *z)
{
 g->z.push_back(monster(mtype::types[mon_fungaloid], z->pos));
}

void mdeath::disintegrate(game *g, monster *z)
{
 if (g->u.see(*z)) messages.add("It disintegrates!");
}

void mdeath::worm(game *g, monster *z)
{
 if (g->u.see(*z))
     messages.add("The %s splits in two!",
                   grammar::capitalize(z->desc(grammar::noun::role::subject, grammar::article::definite)).c_str());

 static std::function<std::optional<point>(point)> dest_clear = [&](point delta) {
     decltype(auto) test = z->pos + delta;
     return g->m.has_flag(diggable, test) && !g->mon(test) && !g->survivor(test) ? std::optional<point>(test) : std::nullopt;
 };

 auto wormspots(grep(within_rldist<1>, dest_clear));

 monster worm(mtype::types[mon_halfworm]);
 for (int worms = 0; worms < 2 && wormspots.size() > 0; worms++) {
  const int rn = rng(0, wormspots.size() - 1);
  worm.spawn(wormspots[rn]);
  g->z.push_back(worm);
  wormspots.erase(wormspots.begin() + rn);
 }
}

void mdeath::disappear(game *g, monster *z)
{
 messages.add("The %s disappears!  Was it in your head?", z->name().c_str());
}

void mdeath::guilt(game *g, monster *z)
{
 if (g->u.has_trait(PF_HEARTLESS)) return;	// We don't give a shit!
 if (rl_dist(z->GPSpos, g->u.GPSpos) > 1) return;	// Too far away, we can deal with it
 if (z->hp >= 0) return;	// It probably didn't die from damage
 messages.add("You feel terrible for killing %s!", z->name().c_str());
 g->u.add_morale(MORALE_KILLED_MONSTER, -50, -250);
}

void mdeath::blobsplit(game *g, monster *z)
{
 const int speed = z->speed - rng(30, 50);
 std::optional<std::string> z_name;
 const bool seen = g->u.see(*z);
 if (seen) z_name = grammar::capitalize(z->desc(grammar::noun::role::subject, grammar::article::definite));
 if (speed <= 0) {
  if (seen) messages.add("The %s splatters into tiny, dead pieces.", z_name->c_str());
  return;
 }
 monster blob(mtype::types[(speed < 50 ? mon_blob_small : mon_blob)]);
 blob.speed = speed;
 blob.friendly = z->friendly; // If we're tame, our kids are too
 if (seen) messages.add("The %s splits!", z_name->c_str());
 blob.hp = blob.speed;

 static std::function<std::optional<point>(point)> dest_clear = [&](point delta) {
     decltype(auto) test = z->pos + delta;
     return g->m.move_cost(test) > 0 && !g->mon(test) && !g->survivor(test) ? std::optional<point>(test) : std::nullopt;
 };

 auto valid(grep(within_rldist<1>, dest_clear));

 for (int s = 0; s < 2 && valid.size() > 0; s++) {
  const int rn = rng(0, valid.size() - 1);
  blob.spawn(valid[rn]);
  g->z.push_back(blob);
  valid.erase(valid.begin() + rn);
 }
}

void mdeath::melt(game *g, monster *z)
{
    if (g->u.see(*z))
        messages.add("%s melts away!",
                  grammar::capitalize(z->desc(grammar::noun::role::subject, grammar::article::definite)).c_str());
}

void mdeath::amigara(game *g, monster *z)
{
 if (g->u.has_disease(DI_AMIGARA)) {
  int count = 0;
  for(const auto& _mon : g->z) {
   if (_mon.type->id == mon_amigara_horror) count++;
  }
  if (count <= 1) { // We're the last!
   g->u.rem_disease(DI_AMIGARA);
   messages.add("Your obsession with the fault fades away...");
   g->m.add_item(z->pos, item(g->new_artifact(), messages.turn));
  }
 }
 normal(g, z);
}

void mdeath::thing(game *g, monster *z)
{
 g->z.push_back(monster(mtype::types[mon_thing], z->pos));
}

void mdeath::explode(game *g, monster *z)
{
 static const int explosion_size[mtype::MS_MAX] = {4, 8, 14, 20, 26 };

 g->explosion(z->pos, explosion_size[z->type->size], 0, false);
}

void mdeath::ratking(game *g, monster *z)
{
 g->u.rem_disease(DI_RAT);
}

void mdeath::gameover(game *g, monster *z)
{
 messages.add("Your %s is destroyed!  GAME OVER!", z->name().c_str());
 g->u.hp_cur[hp_torso] = 0;
}

void mdeath::kill_breathers(game *g, monster *z)
{
 for (int i = 0; i < g->z.size(); i++) {
  if (g->z[i].type->id == mon_breather_hub || g->z[i].type->id == mon_breather)
   g->z[i].dead = true;
 }
}
