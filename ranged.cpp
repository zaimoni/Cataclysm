#include "game.h"
#include "keypress.h"
#include "output.h"
#include "line.h"
#include "skill.h"
#include "rng.h"
#include "item.h"
#include "options.h"
#include "posix_time.h"
#include "recent_msg.h"

#include <math.h>
#include <vector>

int time_to_fire(player &p, const it_gun* firing);
int recoil_add(player &p);
void make_gun_sound_effect(game *g, player &p, bool burst);
int calculate_range(player &p, int tarx, int tary);
double calculate_missed_by(player &p, int trange);
void shoot_monster(game *g, player &p, monster &mon, int &dam, double goodhit);
void shoot_player(game *g, player &p, player *h, int &dam, double goodhit);

void splatter(game *g, std::vector<point> trajectory, int dam,
              monster* mon = NULL);

void ammo_effects(game *g, point pt, long flags)
{
	if (flags & mfb(IF_AMMO_EXPLOSIVE)) g->explosion(pt, 24, 0, false);
	if (flags & mfb(IF_AMMO_FRAG)) g->explosion(pt, 12, 28, false);
	if (flags & mfb(IF_AMMO_NAPALM)) g->explosion(pt, 18, 0, true);
	if (flags & mfb(IF_AMMO_EXPLOSIVE_BIG)) g->explosion(pt, 40, 0, false);

	if (flags & mfb(IF_AMMO_TEARGAS)) {
		for (int i = -2; i <= 2; i++) {
			for (int j = -2; j <= 2; j++)
				g->m.add_field(g, pt.x + i, pt.y + j, fd_tear_gas, 3);
		}
	}

	if (flags & mfb(IF_AMMO_SMOKE)) {
		for (int i = -1; i <= 1; i++) {
			for (int j = -1; j <= 1; j++)
				g->m.add_field(g, pt.x + i, pt.y + j, fd_smoke, 3);
		}
	}

	if (flags & mfb(IF_AMMO_FLASHBANG)) g->flashbang(pt);

	if (flags & mfb(IF_AMMO_FLAME)) {
		if (g->m.add_field(g, pt.x, pt.y, fd_fire, 1)) g->m.field_at(pt).age = 800;
	}
}

void game::fire(player &p, int tarx, int tary, std::vector<point> &trajectory, bool burst)
{
 item ammotmp;
 if (p.weapon.has_flag(IF_CHARGE)) { // It's a charger gun, so make up a type
// Charges maxes out at 8.
  it_ammo* const tmpammo = dynamic_cast<it_ammo*>(item::types[itm_charge_shot]);	// XXX should be copy-construction \todo fix
  tmpammo->damage = p.weapon.charges * p.weapon.charges;
  tmpammo->pierce = (p.weapon.charges >= 4 ? (p.weapon.charges - 3) * 2.5 : 0);
  tmpammo->range = 5 + p.weapon.charges * 5;
  if (p.weapon.charges <= 4)
   tmpammo->accuracy = 14 - p.weapon.charges * 2;
  else // 5, 12, 21, 32
   tmpammo->accuracy = p.weapon.charges * (p.weapon.charges - 4);
  tmpammo->recoil = tmpammo->accuracy * .8;
  tmpammo->item_flags = 0;
  if (p.weapon.charges == 8)
   tmpammo->item_flags |= mfb(IF_AMMO_EXPLOSIVE_BIG);
  else if (p.weapon.charges >= 6)
   tmpammo->item_flags |= mfb(IF_AMMO_EXPLOSIVE);
  if (p.weapon.charges >= 5)
   tmpammo->item_flags |= mfb(IF_AMMO_FLAME);
  else if (p.weapon.charges >= 4)
   tmpammo->item_flags |= mfb(IF_AMMO_INCENDIARY);

  ammotmp = item(tmpammo, 0);
  p.weapon.curammo = tmpammo;
  p.weapon.active = false;
  p.weapon.charges = 0;

 } else // Just a normal gun. If we're here, we know curammo is valid.
  ammotmp = item(p.weapon.curammo, 0);

 ammotmp.charges = 1;
 if (!p.weapon.is_gun()) {
  debugmsg("%s tried to fire a non-gun (%s).", p.name.c_str(),
                                               p.weapon.tname().c_str());
  return;
 }
 unsigned int flags = p.weapon.curammo->item_flags;
// Bolts and arrows are silent
 const bool is_bolt = (p.weapon.curammo->type == AT_BOLT || p.weapon.curammo->type == AT_ARROW);
// TODO: Move this check to game::plfire
 if ((p.weapon.has_flag(IF_STR8_DRAW)  && p.str_cur <  4) ||
     (p.weapon.has_flag(IF_STR10_DRAW) && p.str_cur <  5)   ) {
  messages.add("You're not strong enough to draw the bow!");
  return;
 }

 int x = p.pos.x, y = p.pos.y;
 const it_gun* const firing = dynamic_cast<const it_gun*>(p.weapon.type);
 if (p.has_trait(PF_TRIGGERHAPPY) && one_in(30)) burst = true;
 if (burst && p.weapon.burst_size() < 2) burst = false; // Can't burst fire a semi-auto

 bool u_see_shooter = u_see(p.pos);
// Use different amounts of time depending on the type of gun and our skill
 p.moves -= time_to_fire(p, firing);
// Decide how many shots to fire
 int num_shots = 1;
 if (burst) num_shots = p.weapon.burst_size();
 if (num_shots > p.weapon.charges && !p.weapon.has_flag(IF_CHARGE)) num_shots = p.weapon.charges;

 if (num_shots == 0) debugmsg("game::fire() - num_shots = 0!");
 
 // Make a sound at our location - Zombies will chase it
 make_gun_sound_effect(this, p, burst);
// Set up a timespec for use in the nanosleep function below
 timespec ts;
 ts.tv_sec = 0;
 ts.tv_nsec = BULLET_SPEED;

 bool missed = false;
 int tart;
 for (int curshot = 0; curshot < num_shots; curshot++) {
// Burst-fire weapons allow us to pick a new target after killing the first
  monster* m_at = mon(tarx,tary);	// code below assumes kill processing is not "immediate"
  if (curshot > 0 && (!m_at || m_at->hp <= 0)) {
   std::vector<point> new_targets;
   for (int radius = 1; radius <= 2 + p.sklevel[sk_gun] && new_targets.empty(); radius++) {
    for (int diff = 0 - radius; diff <= radius; diff++) {
     m_at = mon(tarx + diff, tary - radius);
     if (m_at && 0 < m_at->hp && m_at->friendly == 0)
      new_targets.push_back( point(tarx + diff, tary - radius) );

	 m_at = mon(tarx + diff, tary + radius);
     if (m_at && 0 < m_at->hp && m_at->friendly == 0)
      new_targets.push_back( point(tarx + diff, tary + radius) );

     if (diff != 0 - radius && diff != radius) { // Corners were already checked
      m_at = mon(tarx - radius, tary + diff);
      if (m_at && 0 < m_at->hp && m_at->friendly == 0)
       new_targets.push_back( point(tarx - radius, tary + diff) );

	  m_at = mon(tarx + radius, tary + diff);
      if (m_at && 0 < m_at->hp && m_at->friendly == 0)
       new_targets.push_back( point(tarx + radius, tary + diff) );
     }
    }
   }
   if (!new_targets.empty()) {
    int target_picked = rng(0, new_targets.size() - 1);
    tarx = new_targets[target_picked].x;
    tary = new_targets[target_picked].y;
	trajectory = line_to(p.pos, tarx, tary, (m.sees(p.pos, tarx, tary, 0, tart) ? tart : 0));
   } else if ((!p.has_trait(PF_TRIGGERHAPPY) || one_in(3)) &&
              (p.sklevel[sk_gun] >= 7 || one_in(7 - p.sklevel[sk_gun])))
    return; // No targets, so return
  }
// Use up a round (or 100)
  if (p.weapon.has_flag(IF_FIRE_100))
   p.weapon.charges -= 100;
  else
   p.weapon.charges--;

  int trange = calculate_range(p, tarx, tary);
  double missed_by = calculate_missed_by(p, trange);
// Calculate a penalty based on the monster's speed
  double monster_speed_penalty = 1.;
  {
  monster* const m_at = mon(tarx, tary);
  if (m_at) {
   monster_speed_penalty = double(m_at->speed) / 80.;
   if (monster_speed_penalty < 1.) monster_speed_penalty = 1.;
  }
  }

  if (curshot > 0) {
   if (recoil_add(p) % 2 == 1) p.recoil++;
   p.recoil += recoil_add(p) / 2;
  } else
   p.recoil += recoil_add(p);

  if (missed_by >= 1.) {
// We missed D:
// Shoot a random nearby space?
   const int delta = int(sqrt(double(missed_by)));
   tarx += rng(-delta, delta);
   tary += rng(-delta, delta);
   trajectory = line_to(p.pos, tarx, tary, (m.sees(p.pos, x, y, -1, tart) ? tart : 0));
   missed = true;
   if (!burst) {
    if (&p == &u) messages.add("You miss!");
    else if (u_see_shooter) messages.add("%s misses!", p.name.c_str());
   }
  } else if (missed_by >= .7 / monster_speed_penalty) {
// Hit the space, but not necessarily the monster there
   missed = true;
   if (!burst) {
    if (&p == &u) messages.add("You barely miss!");
    else if (u_see_shooter) messages.add("%s barely misses!", p.name.c_str());
   }
  }

  int dam = p.weapon.gun_damage();
  for (int i = 0; i < trajectory.size() && (dam > 0 || (flags & IF_AMMO_FLAME)); i++) {
   if (i > 0)
    m.drawsq(w_terrain, u, trajectory[i-1].x, trajectory[i-1].y, false, true);
// Drawing the bullet uses player u, and not player p, because it's drawn
// relative to YOUR position, which may not be the gunman's position.
   if (u_see(trajectory[i])) {
    char bullet = (flags & mfb(IF_AMMO_FLAME)) ? '#' : '*';
    mvwputch(w_terrain, trajectory[i].y + SEEY - u.pos.y,
                        trajectory[i].x + SEEX - u.pos.x, c_red, bullet);
    wrefresh(w_terrain);
    if (&p == &u) nanosleep(&ts, NULL);
   }
   
   if (dam <= 0) { // Ran out of momentum.
    ammo_effects(this, trajectory[i], flags);
    if (is_bolt &&
        ((p.weapon.curammo->m1 == WOOD && !one_in(4)) ||
         (p.weapon.curammo->m1 != WOOD && !one_in(15))))
     m.add_item(trajectory[i], ammotmp);
    if (p.weapon.charges == 0) p.weapon.curammo = NULL;
    return;
   }

   int tx = trajectory[i].x, ty = trajectory[i].y;
// If there's a monster in the path of our bullet, and either our aim was true,
//  OR it's not the monster we were aiming at and we were lucky enough to hit it
   monster* const m_at = mon(trajectory[i]);
   player* const h = (u.pos == trajectory[i]) ? &u : nPC(tx, ty);	// XXX would prefer not to pay CPU here when monster case processes
// If we shot us a monster...
   if (m_at && (!m_at->has_flag(MF_DIGS) ||
       rl_dist(p.pos, m_at->pos) <= 1) &&
       ((!missed && i == trajectory.size() - 1) ||
        one_in((5 - int(m_at->type->size))))) {

    double goodhit = missed_by;
    if (i < trajectory.size() - 1) // Unintentional hit
     goodhit = double(rand() / (RAND_MAX + 1.0)) / 2;

// Penalize for the monster's speed
    if (m_at->speed > 80) goodhit *= double( double(m_at->speed) / 80.);
    
    std::vector<point> blood_traj = trajectory;
    blood_traj.insert(blood_traj.begin(), p.pos);
    splatter(this, blood_traj, dam, m_at);
    shoot_monster(this, p, *m_at, dam, goodhit);

   } else if ((!missed || one_in(3)) && h)  {
    double goodhit = missed_by;
    if (i < trajectory.size() - 1) // Unintentional hit
     goodhit = double(rand() / (RAND_MAX + 1.0)) / 2;

    std::vector<point> blood_traj = trajectory;
    blood_traj.insert(blood_traj.begin(), p.pos);
    splatter(this, blood_traj, dam);
    shoot_player(this, p, h, dam, goodhit);

   } else
    m.shoot(this, tx, ty, dam, i == trajectory.size() - 1, flags);
  } // Done with the trajectory!

  point last(trajectory.back());
  ammo_effects(this, last, flags);

  if (m.move_cost(last) == 0) last = trajectory[trajectory.size() - 2];
  if (is_bolt &&
      ((p.weapon.curammo->m1 == WOOD && !one_in(5)) ||	// leave this verbose in case we want additional complexity
       (p.weapon.curammo->m1 != WOOD && !one_in(15))  ))
    m.add_item(last, ammotmp);
 }

 if (p.weapon.charges == 0) p.weapon.curammo = NULL;
}


void game::throw_item(player &p, int tarx, int tary, item &thrown,
                      std::vector<point> &trajectory)
{
 int deviation = 0;
 int trange = 1.5 * rl_dist(p.pos, tarx, tary);

// Throwing attempts below "Basic Competency" level are extra-bad
 if (p.sklevel[sk_throw] < 3)
  deviation += rng(0, 8 - p.sklevel[sk_throw]);

 if (p.sklevel[sk_throw] < 8)
  deviation += rng(0, 8 - p.sklevel[sk_throw]);
 else
  deviation -= p.sklevel[sk_throw] - 6;

 deviation += p.throw_dex_mod();

 if (p.per_cur < 6)
  deviation += rng(0, 8 - p.per_cur);
 else if (p.per_cur > 8)
  deviation -= p.per_cur - 8;

 deviation += rng(0, p.encumb(bp_hands) * 2 + p.encumb(bp_eyes) + 1);
 if (thrown.volume() > 5)
  deviation += rng(0, 1 + (thrown.volume() - 5) / 4);
 if (thrown.volume() == 0)
  deviation += rng(0, 3);

 deviation += rng(0, 1 + abs(p.str_cur - thrown.weight()));

 double missed_by = .01 * deviation * trange;
 bool missed = false;
 int tart;

 if (missed_by >= 1) {
// We missed D:
// Shoot a random nearby space?
  if (missed_by > 9) missed_by = 9;
  const int delta = int(sqrt(double(missed_by)));
  tarx += rng(-delta, delta);
  tary += rng(-delta, delta);
  trajectory = line_to(p.pos, tarx, tary, (m.sees(p.pos, tarx, tary, -1, tart) ? tart : 0));
  missed = true;
  if (!p.is_npc()) messages.add("You miss!");
 } else if (missed_by >= .6) {
// Hit the space, but not necessarily the monster there
  missed = true;
  if (!p.is_npc()) messages.add("You barely miss!");
 }

 std::string message;
 int dam = (thrown.weight() / 4 + thrown.type->melee_dam / 2 + p.str_cur / 2) /
            double(2 + double(thrown.volume() / 4));
 if (dam > thrown.weight() * 3)
  dam = thrown.weight() * 3;

 int i = 0, tx = 0, ty = 0;
 for (i = 0; i < trajectory.size() && dam > -10; i++) {
  message = "";
  double goodhit = missed_by;
  tx = trajectory[i].x;
  ty = trajectory[i].y;
  monster* const m_at = mon(trajectory[i]);
// If there's a monster in the path of our item, and either our aim was true,
//  OR it's not the monster we were aiming at and we were lucky enough to hit it
  if (m_at &&
      (!missed || one_in(7 - int(m_at->type->size)))) {
   if (0 < thrown.type->melee_cut && rng(0, 100) < 20 + p.sklevel[sk_throw] * 12) {
    if (!p.is_npc()) {
     message += " You cut the ";
     message += m_at->name();
     message += "!";
    }
	const auto c_armor = m_at->armor_cut();
    if (thrown.type->melee_cut > c_armor) dam += (thrown.type->melee_cut - c_armor);
   }
   if (thrown.made_of(GLASS) && !thrown.active && // active = molotov, etc.
       rng(0, thrown.volume() + 8) - rng(0, p.str_cur) < thrown.volume()) {
    if (u_see(tx, ty)) messages.add("The %s shatters!", thrown.tname().c_str());
    for (int i = 0; i < thrown.contents.size(); i++)
     m.add_item(tx, ty, thrown.contents[i]);
    sound(tx, ty, 16, "glass breaking!");
    const int glassdam = rng(0, thrown.volume() * 2);
	const auto c_armor = m_at->armor_cut();
	if (glassdam > c_armor) dam += (glassdam - c_armor);
   } else
    m.add_item(tx, ty, thrown);
   if (i < trajectory.size() - 1)
    goodhit = double(double(rand() / RAND_MAX) / 2);
   if (goodhit < .1 && !m_at->has_flag(MF_NOHEAD)) {
    message = "Headshot!";
    dam = rng(dam, dam * 3);
    p.practice(sk_throw, 5);
   } else if (goodhit < .2) {
    message = "Critical!";
    dam = rng(dam, dam * 2);
    p.practice(sk_throw, 2);
   } else if (goodhit < .4)
    dam = rng(int(dam / 2), int(dam * 1.5));
   else if (goodhit < .5) {
    message = "Grazing hit.";
    dam = rng(0, dam);
   }
   if (!p.is_npc())
    messages.add("%s You hit the %s for %d damage.", message.c_str(), m_at->name().c_str(), dam);
   else if (u_see(tx, ty))
    messages.add("%s hits the %s for %d damage.", message.c_str(), m_at->name().c_str(), dam);
   if (m_at->hurt(dam)) kill_mon(*m_at, !p.is_npc());
   return;
  } else // No monster hit, but the terrain might be.
   m.shoot(this, tx, ty, dam, false, 0);
  if (m.move_cost(tx, ty) == 0) {
   if (i > 0) {
    tx = trajectory[i - 1].x;
    ty = trajectory[i - 1].y;
   } else {
    tx = u.pos.x;
    ty = u.pos.y;
   }
   i = trajectory.size();
  }
 }
 if (m.move_cost(tx, ty) == 0) {
  if (i > 1) {
   tx = trajectory[i - 2].x;
   ty = trajectory[i - 2].y;
  } else {
   tx = u.pos.x;
   ty = u.pos.y;
  }
 }
 if (thrown.made_of(GLASS) && !thrown.active && // active means molotov, etc
     rng(0, thrown.volume() + 8) - rng(0, p.str_cur) < thrown.volume()) {
  if (u_see(tx, ty)) messages.add("The %s shatters!", thrown.tname().c_str());
  for (int i = 0; i < thrown.contents.size(); i++)
   m.add_item(tx, ty, thrown.contents[i]);
  sound(tx, ty, 16, "glass breaking!");
 } else {
  sound(tx, ty, 8, "thud.");
  m.add_item(tx, ty, thrown);
 }
}

std::vector<point> game::target(int &x, int &y, int lowx, int lowy, int hix,
                                int hiy, std::vector <monster> t, int &target,
                                item *relevent)
{
 std::vector<point> ret;
 const int sight_dist = u.sight_range(light_level());

// First, decide on a target among the monsters, if there are any in range
 if (t.size() > 0) {
// Check for previous target
  if (target == -1) {
// If no previous target, target the closest there is
   double closest = -1;
   double dist;
   for (int i = 0; i < t.size(); i++) {
    dist = rl_dist(t[i].pos, u.pos);
    if (closest < 0 || dist < closest) {
     closest = dist;
     target = i;
    }
   }
  }
  x = t[target].pos.x;
  y = t[target].pos.y;
 } else
  target = -1;	// No monsters in range, don't use target, reset to -1

 WINDOW* w_target = newwin(13, 48, 12, SEEX * 2 + 8);
 wborder(w_target, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                 LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 if (!relevent) // currently targetting vehicle to refill with fuel
  mvwprintz(w_target, 1, 1, c_red, "Select a vehicle");
 else
 if (relevent == &u.weapon && relevent->is_gun())
  mvwprintz(w_target, 1, 1, c_red, "Firing %s (%d)", // - %s (%d)",
            u.weapon.tname().c_str(),// u.weapon.curammo->name.c_str(),
            u.weapon.charges);
 else
  mvwprintz(w_target, 1, 1, c_red, "Throwing %s", relevent->tname().c_str());
 mvwprintz(w_target, 2, 1, c_white,
           "Move cursor to target with directional keys.");
 if (relevent) {
  mvwprintz(w_target, 3, 1, c_white,
            "'<' '>' Cycle targets; 'f' or '.' to fire.");
  mvwprintz(w_target, 4, 1, c_white, 
            "'0' target self; '*' toggle snap-to-target");
 }

 wrefresh(w_target);
 char ch;
 bool snap_to_target = OPTIONS[OPT_SNAP_TO_TARGET];
// The main loop.
 do {
  point center;
  if (snap_to_target)
   center = point(x, y);
  else
   center = u.pos;
// Clear the target window.
  for (int i = 5; i < 12; i++) {
   for (int j = 1; j < 46; j++)
    mvwputch(w_target, i, j, c_white, ' ');
  }
  m.draw(this, w_terrain, center);
// Draw the Monsters
  for (int i = 0; i < z.size(); i++) {
   if (u_see(&(z[i])) && z[i].pos.x >= lowx && z[i].pos.y >= lowy &&
                               z[i].pos.x <=  hix && z[i].pos.y <=  hiy)
    z[i].draw(w_terrain, center.x, center.y, false);
  }
// Draw the NPCs
  for (int i = 0; i < active_npc.size(); i++) {
   if (u_see(active_npc[i].pos))
    active_npc[i].draw(w_terrain, center.x, center.y, false);
  }
  if (x != u.pos.x || y != u.pos.y) {
// Calculate the return vector (and draw it too)
/*
   for (int i = 0; i < ret.size(); i++)
    m.drawsq(w_terrain, u, ret[i].x, ret[i].y, false, true, center.x, center.y);
*/
// Draw the player
   int atx = SEEX + u.pos.x - center.x, aty = SEEY + u.pos.y - center.y;
   if (atx >= 0 && atx < SEEX * 2 + 1 && aty >= 0 && aty < SEEY * 2 + 1)
    mvwputch(w_terrain, aty, atx, u.color(), '@');

   int tart;
   if (m.sees(u.pos, x, y, -1, tart)) {// Selects a valid line-of-sight
    ret = line_to(u.pos, x, y, tart); // Sets the vector to that LOS
// Draw the trajectory
    for (int i = 0; i < ret.size(); i++) {
     if (abs(ret[i].x - u.pos.x) <= sight_dist &&
         abs(ret[i].y - u.pos.y) <= sight_dist   ) {
      monster* const m_at = mon(ret[i]);
// NPCs and monsters get drawn with inverted colors
      if (m_at && u_see(m_at)) m_at->draw(w_terrain, center.x, center.y, true);
      else if (npc* const _npc = nPC(ret[i]))
       _npc->draw(w_terrain, center.x, center.y, true);
      else
       m.drawsq(w_terrain, u, ret[i].x, ret[i].y, true,true,center.x, center.y);
     }
    }
   }

   if (!relevent) { // currently targetting vehicle to refill with fuel
    vehicle *veh = m.veh_at(x, y);
    if (veh)
     mvwprintw(w_target, 5, 1, "There is a %s", veh->name.c_str());
   } else
    mvwprintw(w_target, 5, 1, "Range: %d", rl_dist(u.pos, x, y));

   monster* const m_at = mon(x, y);
   if (!m_at) {
    mvwprintw(w_status, 0, 9, "                             ");
    if (snap_to_target)
     mvwputch(w_terrain, SEEY, SEEX, c_red, '*');
    else
     mvwputch(w_terrain, y + SEEY - u.pos.y, x + SEEX - u.pos.x, c_red, '*');
   } else if (u_see(m_at)) m_at->print_info(this, w_target);
  }
  wrefresh(w_target);
  wrefresh(w_terrain);
  wrefresh(w_status);
  refresh();
  ch = input();
  point tar(get_direction(ch));
  if (tar.x != -2 && ch != '.') {	// Direction character pressed
   monster* const m_at = mon(x,y);
   if (m_at && u_see(m_at))
    m_at->draw(w_terrain, center.x, center.y, false);
   else if (npc* const _npc = nPC(x,y))
	_npc->draw(w_terrain, center.x, center.y, false);
   else if (m.sees(u.pos, x, y, -1))
    m.drawsq(w_terrain, u, x, y, false, true, center.x, center.y);
   else
    mvwputch(w_terrain, SEEY, SEEX, c_black, 'X');
   x += tar.x;
   y += tar.y;
   if (x < lowx) x = lowx;
   else if (x > hix) x = hix;
   if (y < lowy) y = lowy;
   else if (y > hiy) y = hiy;
  } else if ((ch == '<') && (target != -1)) {
   target--;
   if (target == -1) target = t.size() - 1;
   x = t[target].pos.x;
   y = t[target].pos.y;
  } else if ((ch == '>') && (target != -1)) {
   target++;
   if (target == t.size()) target = 0;
   x = t[target].pos.x;
   y = t[target].pos.y;
  } else if (ch == '.' || ch == 'f' || ch == 'F' || ch == '\n') {
   for (int i = 0; i < t.size(); i++) {
    if (t[i].pos.x == x && t[i].pos.y == y)
     target = i;
   }
   return ret;
  } else if (ch == '0') {
   x = u.pos.x;
   y = u.pos.y;
  } else if (ch == '*')
   snap_to_target = !snap_to_target;
  else if (ch == KEY_ESCAPE || ch == 'q') { // return empty vector (cancel)
   ret.clear();
   return ret;
  }
 } while (true);
}

void game::hit_monster_with_flags(monster &z, unsigned int flags)
{
 if (flags & mfb(IF_AMMO_FLAME)) {

  if (z.made_of(VEGGY) || z.made_of(COTTON) || z.made_of(WOOL) ||
      z.made_of(PAPER) || z.made_of(WOOD))
   z.add_effect(ME_ONFIRE, rng(8, 20));
  else if (z.made_of(FLESH))
   z.add_effect(ME_ONFIRE, rng(5, 10));
  
 } else if (flags & mfb(IF_AMMO_INCENDIARY)) {

  if (z.made_of(VEGGY) || z.made_of(COTTON) || z.made_of(WOOL) ||
      z.made_of(PAPER) || z.made_of(WOOD))
   z.add_effect(ME_ONFIRE, rng(2, 6));
  else if (z.made_of(FLESH) && one_in(4))
   z.add_effect(ME_ONFIRE, rng(1, 4));

 }
}

int time_to_fire(player &p, const it_gun* const firing)
{
 int time = 0;

 switch (firing->skill_used) {
 case sk_pistol: return (6 < p.sklevel[sk_pistol]) ? 10 : (80 - 10 * p.sklevel[sk_pistol]);
 case sk_shotgun: return (3 < p.sklevel[sk_shotgun]) ? 70 : (150 - 25 * p.sklevel[sk_shotgun]);
 case sk_smg: return (5 < p.sklevel[sk_smg]) ? 20 : (80 - 10 * p.sklevel[sk_smg]);
 case sk_rifle: return (8 < p.sklevel[sk_rifle]) ? 30 : (150 - 15 * p.sklevel[sk_rifle]);
 case sk_archery: return (8 < p.sklevel[sk_archery]) ? 20 : (220 - 25 * p.sklevel[sk_archery]);
 case sk_launcher: return (8 < p.sklevel[sk_launcher]) ? 30 : (200 - 20 * p.sklevel[sk_launcher]);

 default:
  debugmsg("Why is shooting %s using %s skill?", (firing->name).c_str(),
		skill_name(firing->skill_used).c_str());
  return 0;
 }

 return time;
}

void make_gun_sound_effect(game *g, player &p, bool burst)
{
 std::string gunsound;
 int noise = p.weapon.noise();
 if (noise < 5) {
  gunsound = burst ? "Brrrip!" : "plink!";
 } else if (noise < 25) {
  gunsound = burst ? "Brrrap!" : "bang!";
 } else if (noise < 60) {
  gunsound = burst ? "P-p-p-pow!" : "blam!";
 } else {
  gunsound = burst ? "Kaboom!!" : "kerblam!";
 }
 if (p.weapon.curammo->type == AT_FUSION || p.weapon.curammo->type == AT_BATT || p.weapon.curammo->type == AT_PLUT)
  g->sound(p.pos, 8, "Fzzt!");
 else if (p.weapon.curammo->type == AT_40MM)
  g->sound(p.pos, 8, "Thunk!");
 else if (p.weapon.curammo->type == AT_GAS)
  g->sound(p.pos, 4, "Fwoosh!");
 else if (p.weapon.curammo->type != AT_BOLT && p.weapon.curammo->type != AT_ARROW)
  g->sound(p.pos, noise, gunsound);
}

int calculate_range(player &p, int tarx, int tary)
{
 int trange = rl_dist(p.pos, tarx, tary);
 const it_gun* const firing = dynamic_cast<const it_gun*>(p.weapon.type);
 if (trange < int(firing->volume / 3) && firing->ammo != AT_SHOT)
  trange = int(firing->volume / 3);
 else if (p.has_bionic(bio_targeting)) {
  trange = int(trange * ((LONG_RANGE < trange) ? .65 : .8));
 }

 if (firing->skill_used == sk_rifle && trange > LONG_RANGE)
  trange = LONG_RANGE + .6 * (trange - LONG_RANGE);

 return trange;
}

double calculate_missed_by(player &p, int trange)	// XXX real-world deviation is normal distribution with arithmetic mean zero \todo fix
{
  const it_gun* const firing = dynamic_cast<const it_gun*>(p.weapon.type);
// Calculate deviation from intended target (assuming we shoot for the head)
  double deviation = 0.; // Measured in quarter-degrees
// Up to 1.5 degrees for each skill point < 4; up to 1.25 for each point > 4
  if (p.sklevel[firing->skill_used] < 4)
   deviation += rng(0, 6 * (4 - p.sklevel[firing->skill_used]));
  else if (p.sklevel[firing->skill_used] > 4)
   deviation -= rng(0, 5 * (p.sklevel[firing->skill_used] - 4));

  if (p.sklevel[sk_gun] < 3)
   deviation += rng(0, 3 * (3 - p.sklevel[sk_gun]));
  else
   deviation -= rng(0, 2 * (p.sklevel[sk_gun] - 3));

  deviation += p.ranged_dex_mod();
  deviation += p.ranged_per_mod();

  deviation += rng(0, 2 * p.encumb(bp_arms)) + rng(0, 4 * p.encumb(bp_eyes));

  deviation += rng(0, p.weapon.curammo->accuracy);
  deviation += rng(0, p.weapon.accuracy());
  int adj_recoil = p.recoil + p.driving_recoil;
  deviation += rng(int(adj_recoil / 4), adj_recoil);

// .013 * trange is a computationally cheap version of finding the tangent.
// (note that .00325 * 4 = .013; .00325 is used because deviation is a number
//  of quarter-degrees)
// It's also generous; missed_by will be rather short.
  return (.00325 * deviation * trange);
}

int recoil_add(player &p)
{
 const it_gun* const firing = dynamic_cast<const it_gun*>(p.weapon.type);
 int ret = p.weapon.recoil();
 ret -= rng(p.str_cur / 2, p.str_cur);
 ret -= rng(0, p.sklevel[firing->skill_used] / 2);
 return (0 < ret) ? ret : 0;
}

void shoot_monster(game *g, player &p, monster &mon, int &dam, double goodhit)
{
 const it_gun* const firing = dynamic_cast<const it_gun*>(p.weapon.type);
 std::string message;
 const bool u_see_mon = g->u_see(&(mon));
 if (mon.has_flag(MF_HARDTOSHOOT) && !one_in(4) &&
     !p.weapon.curammo->m1 == LIQUID && 
     p.weapon.curammo->accuracy >= 4) { // Buckshot hits anyway
  if (u_see_mon)
   messages.add("The shot passes through the %s without hitting.", mon.name().c_str());
  goodhit = 1;
 } else { // Not HARDTOSHOOT
// Armor blocks BEFORE any critical effects.
  int zarm = mon.armor_cut();
  zarm -= p.weapon.curammo->pierce;
  if (p.weapon.curammo->m1 == LIQUID) zarm = 0;
  else if (p.weapon.curammo->accuracy < 4) // Shot doesn't penetrate armor well
   zarm *= rng(2, 4);
  if (zarm > 0) dam -= zarm;
  if (dam <= 0) {
   if (u_see_mon)
    messages.add("The shot reflects off the %s!", mon.name_with_armor().c_str());
   dam = 0;
   goodhit = 1;
  }
  if (goodhit < .1 && !mon.has_flag(MF_NOHEAD)) {
   message = "Headshot!";
   dam = rng(5 * dam, 8 * dam);
   p.practice(firing->skill_used, 5);
  } else if (goodhit < .2) {
   message = "Critical!";
   dam = rng(dam * 2, dam * 3);
   p.practice(firing->skill_used, 2);
  } else if (goodhit < .4) {
   dam = rng(int(dam * .9), int(dam * 1.5));
   p.practice(firing->skill_used, rng(0, 2));
  } else if (goodhit <= .7) {
   message = "Grazing hit.";
   dam = rng(0, dam);
  } else dam = 0;

// Find the zombie at (x, y) and hurt them, MAYBE kill them!
  if (dam > 0) {
   mon.moves -= dam * 5;
   if (&p == &(g->u) && u_see_mon)
    messages.add("%s You hit the %s for %d damage.", message.c_str(), mon.name().c_str(), dam);
   else if (u_see_mon)
    messages.add("%s %s shoots the %s.", message.c_str(), p.name.c_str(), mon.name().c_str());
   if (mon.hurt(dam)) g->kill_mon(mon, (&p == &(g->u)));
   else if (p.weapon.curammo->item_flags != 0)
    g->hit_monster_with_flags(mon, p.weapon.curammo->item_flags);
   dam = 0;
  }
 }
}

void shoot_player(game *g, player &p, player *h, int &dam, double goodhit)
{
 const it_gun* const firing = dynamic_cast<const it_gun*>(p.weapon.type);
 body_part hit;
 int side = rng(0, 1);
 if (goodhit < .05) {
  hit = bp_eyes;
  dam = rng(3 * dam, 5 * dam);
  p.practice(firing->skill_used, 5);
 } else if (goodhit < .1) {
  if (one_in(6)) hit = bp_eyes;
  else if (one_in(4)) hit = bp_mouth;
  else hit = bp_head;
  dam = rng(2 * dam, 5 * dam);
  p.practice(firing->skill_used, 5);
 } else if (goodhit < .2) {
  hit = bp_torso;
  dam = rng(dam, 2 * dam);
  p.practice(firing->skill_used, 2);
 } else if (goodhit < .4) {
  if (one_in(3)) hit = bp_torso;
  else if (one_in(2)) hit = bp_arms;
  else hit = bp_legs;
  dam = rng(int(dam * .9), int(dam * 1.5));
  p.practice(firing->skill_used, rng(0, 1));
 } else if (goodhit < .5) {
  hit = one_in(2) ? bp_arms : bp_legs;
  dam = rng(dam / 2, dam);
 } else dam = 0;

 if (dam > 0) {
  h->moves -= rng(0, dam);
  if (h == &(g->u))
   messages.add("%s shoots your %s for %d damage!", p.name.c_str(), body_part_name(hit, side).c_str(), dam);
  else {
   if (&p == &(g->u))
    messages.add("You shoot %s's %s.", h->name.c_str(), body_part_name(hit, side).c_str());
   else if (g->u_see(h->pos))
    messages.add("%s shoots %s's %s.",
               (g->u_see(p.pos) ? p.name.c_str() : "Someone"),
               h->name.c_str(), body_part_name(hit, side).c_str());
  }
  h->hit(g, hit, side, 0, dam);
/*
  if (h != &(g->u)) {
   int npcdex = g->npc_at(h->posx, h->posy);
   if (g->active_npc[npcdex].hp_cur[hp_head]  <= 0 ||
       g->active_npc[npcdex].hp_cur[hp_torso] <= 0   ) {
    g->active_npc[npcdex].die(g, !p.is_npc());
    g->active_npc.erase(g->active_npc.begin() + npcdex);
   }
  }
*/
 }
}

void splatter(game *g, std::vector<point> trajectory, int dam, monster* mon)
{
 field_id blood = fd_blood;
 if (mon != NULL) {
  if (!mon->made_of(FLESH)) return;
  if (mon->type->dies == &mdeath::boomer) blood = fd_bile;
  else if (mon->type->dies == &mdeath::acid) blood = fd_acid;
 }

 int distance = 1;
 if (dam > 50) distance = 3;
 else if (dam > 20) distance = 2;

 std::vector<point> spurt = continue_line(trajectory, distance);

 for (auto& tar : spurt) {
	 auto& fd = g->m.field_at(tar);
	 if (blood == fd.type) {
		 if (3 > fd.density) fd.density++;
	 } else g->m.add_field(g, tar.x, tar.y, blood, 1);
 }
}
