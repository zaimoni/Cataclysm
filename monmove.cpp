// Monster movement code; essentially, the AI

#include "monster.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "recent_msg.h"

#include <stdlib.h>

#define MONSTER_FOLLOW_DIST 8

bool monster::can_move_to(const map &m, int x, int y) const
{
 if (m.move_cost(x, y) == 0 &&
     (!has_flag(MF_DESTROYS) || !m.is_destructable(x, y)) &&
     ((!has_flag(MF_AQUATIC) && !has_flag(MF_SWIMS)) ||
      !m.has_flag(swimmable, x, y)))
  return false;
 if (has_flag(MF_DIGS) && !m.has_flag(diggable, x, y)) return false;
 if (has_flag(MF_AQUATIC) && !m.has_flag(swimmable, x, y)) return false;
 return true;
}

bool monster::can_enter(const map& m, const point& pt) const
{
    if (can_move_to(m, pt)) return true;
    return has_flag(MF_BASHES) && m.has_flag(bashable, pt);
}

// Resets plans (list of squares to visit) and builds it as a straight line
// to the destination (x,y). t is used to choose which eligible line to use.
// Currently, this assumes we can see (x,y), so shouldn't be used in any other
// circumstance (or else the monster will "phase" through solid terrain!)
void monster::set_dest(const point& pt, int t)
{ 
 plans = line_to(pos, pt, t);
}

// Move towards (x,y) for f more turns--generally if we hear a sound there
// "Stupid" movement; "if (wandx < posx) posx--;" etc.
void monster::wander_to(const point& pt, int f)
{
 if (has_flag(MF_GOODHEARING)) f *= 6;
 wand.set(pt, f);
}

void monster::plan(game *g)
{
 int sightrange = g->light_level();
 int dist = INT_MAX;
 int stc;
 if (is_friend()) {	// Target monsters, not the player!
  const monster* closest_mon = nullptr;
  if (!has_effect(ME_DOCILE)) {
   for (const auto& _mon : g->z) {
    if (!is_enemy(&_mon)) continue;
    const int test = rl_dist(GPSpos, _mon.GPSpos);
    if (test < dist) {
        if (const auto tc = g->m.sees(pos, _mon.pos, sightrange)) {
            closest_mon = &_mon;
            dist = test;
            stc = *tc;
        }
    }
   }
  }

  if (closest_mon) set_dest(closest_mon->pos, stc);
  else if (0 < friendly) {
      if (one_in(3)) friendly--;	// Grow restless with no targets
  } else if (0 > friendly) {
      if (const auto tc = see(g->u)) {
          if (rl_dist(GPSpos, g->u.GPSpos) > 2)
              set_dest(g->u.pos, *tc);
          else
              plans.clear();
      }
  }
 }

 bool fleeing = false;
 int closest = -1;
 if (can_see()) {
     auto tc = see(g->u);
     if (tc) {
         if (is_fleeing(g->u)) {
             fleeing = true;
             wand.set(2 * pos - g->u.pos, 40);
             dist = rl_dist(GPSpos, g->u.GPSpos);
         } else { // If we can see, and we can see a character, start moving towards them
             dist = rl_dist(GPSpos, g->u.GPSpos);
             closest = -2;
             stc = *tc;
         }
     }
	 // check NPCs
     int i = -1;
     for (const auto& _npc : g->active_npc) {
         ++i;
         int medist = rl_dist(GPSpos, _npc.GPSpos);
         if ((medist < dist || (!fleeing && is_fleeing(_npc)))) {
             tc = see(_npc);
             if (tc) {
                 dist = medist;
                 if (is_fleeing(_npc)) {
                     fleeing = true;
                     wand.set(2 * pos - _npc.pos, 40);
                 } else /* if (g->m.sees(pos, _npc.pos, sightrange, tc)) */ {
                     closest = i;
                     stc = *tc;
                 }
             }
         }
     }
 }

 if (!fleeing) fleeing = attitude() == MATT_FLEE; // inlining partial definition of is_fleeing(g->u)
 if (!fleeing) {
  const monster* closest_mon = nullptr;
  for (const auto& _mon : g->z) {
   int mondist = rl_dist(GPSpos, _mon.GPSpos);
   if (is_enemy(&_mon) && mondist < dist && can_see()) {
       if (const auto tc = g->m.sees(pos, _mon.pos, sightrange)) {
           dist = mondist;
           if (fleeing) wand.set(2 * pos - _mon.pos, 40);
           else {
               closest_mon = &_mon;
               stc = *tc;
           }
       }
   }
  }

  if (closest == -2)
   set_dest(g->u.pos, stc);
  else if (closest_mon)
   set_dest(closest_mon->pos, stc);
  else if (closest >= 0)
   set_dest(g->active_npc[closest].pos, stc);
 }
}

class melee_target
{
    monster& viewpoint;
public:
    static bool can_construct(const monster& origin) { return 0 < origin.type->melee_dice; }
    melee_target(monster& origin) noexcept : viewpoint(origin) {}; // \todo? enforce suggested invariant, above

    bool operator()(monster* target) {
        if (!target) return false;
        if (species_hallu == target->type->species) {
            game::active()->kill_mon(*target);
            return false;   // *We* did not interact w/the hallucination
        }
        if (viewpoint.is_enemy(target)) {
            viewpoint.hit_monster(game::active(), *target);
            return true;
        }
        return false;
    }
    bool operator()(player* target) {
        if (!target) return false;
        if (viewpoint.is_enemy(target) && !viewpoint.is_fleeing(*target)) {
            viewpoint.hit_player(game::active(), *target);
            return true;
        }
        return false;
    }
};

// General movement.
// Currently, priority goes:
// 1) Special Attack
// 2) Sight-based tracking
// 3) Scent-based tracking
// 4) Sound-based tracking
void monster::move(game *g)
{
// We decrement wandf no matter what.  We'll save our wander_to plans until
// after we finish out set_dest plans, UNLESS they time out first.
 wand.tick();

// First, use the special attack, if we can!
 if (sp_timeout > 0) sp_timeout--;
 if (sp_timeout == 0 && (friendly == 0 || has_flag(MF_FRIENDLY_SPECIAL))) {
     type->do_special_attack(*this);
 }
 if (moves < 0) return;
 if (has_flag(MF_IMMOBILE)) {
  moves = 0;
  return;
 }
 if (has_effect(ME_STUNNED)) {
  stumble(g, false);
  moves = 0;
  return;
 }
 if (has_effect(ME_DOWNED)) {
  moves = 0;
  return;
 }

 std::optional<GPS_loc> next_loc = std::nullopt;

 static auto update_next_loc = [&](const GPS_loc& dest) {
     next_loc = dest;
     if (const auto mob_plan = next_loc ? g->mob_at(*next_loc) : std::nullopt) {
         // can't reuse mob_plan because of dead hallucination path
         if (can_enter(g->m, plans[0]) && melee_target::can_construct(*this) && std::visit(melee_target(*this), *mob_plan)) {
             // we have  melee'ed a hostile target
             moves -= mobile::mp_turn;
             if (0 < friendly) friendly--;
             return true;
         }
     }
     return false;
 };

 if (!plans.empty() && update_next_loc(g->toGPS(plans[0]))) return;

 if (friendly != 0) {
  if (friendly > 0) friendly--;
  friendly_move(g);
  return;
 }

 moves -= mobile::mp_turn;

 monster_attitude current_attitude = attitude(0 == friendly ? &(g->u) : nullptr);
// If our plans end in a player, set our attitude to consider that player
 if (!plans.empty()) {
  if (const auto _survivor = g->survivor(plans.back())) current_attitude = attitude(_survivor);
 }

 if (current_attitude == MATT_IGNORE ||
     (current_attitude == MATT_FOLLOW && plans.size() <= MONSTER_FOLLOW_DIST)) {
  stumble(g, false);
  return;
 }

 bool moved = false;
 point next;
 monster* const m_plan = plans.empty() ? nullptr : g->mon(plans[0]);

 if (!plans.empty() && !is_fleeing(g->u) &&
     (!m_plan || is_enemy(m_plan)) && can_sound_move_to(plans[0])){
  // CONCRETE PLANS - Most likely based on sight
  next = plans[0];
  moved = true;
 } else if (has_flag(MF_SMELLS)) {
// No sight... or our plans are invalid (e.g. moving through a transparent, but
//  solid, square of terrain).  Fall back to smell if we have it.
  if (const auto dest = scent_move()) {
   if (update_next_loc(g->toGPS(*dest))) return;
   next = *dest;
   moved = true;
  }
 }
 if (wand.live() && !moved) { // No LOS, no scent, so as a fall-back follow sound
  point tmp = sound_move();
  if (tmp != pos) {
   if (update_next_loc(g->toGPS(tmp))) return;
   next = tmp;
   moved = true;
  }
 }

// Finished logic section.  By this point, we should have chosen a square to
//  move to (moved = true).
 if (moved) {	// Actual effects of moving to the square we've chosen
  // \todo start C:DDA refactor target monster::attack_at
  if (auto _mob = g->mob_at(next)) {
      if (auto _mon = std::get_if<monster*>(&(*_mob))) {
          if (species_hallu == (*_mon)->type->species) {
              debuglog("should not be executing: monster::move/_mon && species_hallu == (*_mon)->type->species");
              return;
          }
      }
      if (0 < type->melee_dice) {
          if (std::visit(monster::is_enemy_of(*this), *_mob)) {
              debuglog("should not be executing: _mob && 0 < type->melee_dice && std::visit(monster::is_enemy_of(*this), *_mob)");
              return;
          }
          debuglog("should not be executing: _mob && 0 < type->melee_dice");
          return;
      }
  }
  // end C:DDA refactor target monster::attack_at
  // \todo C:DDA refactor target monster::bash_at
  else if ((!can_move_to(g->m, next) || one_in(3)) &&
             g->m.has_flag(bashable, next) && has_flag(MF_BASHES)) {
   std::string bashsound = "NOBASH"; // If we hear "NOBASH" it's time to debug!
   int bashskill = int(type->melee_dice * type->melee_sides);
   g->m.bash(next, bashskill, bashsound);
   g->sound(next, 18, bashsound);
  } else if (g->m.move_cost(next) == 0 && has_flag(MF_DESTROYS)) {
   g->m.destroy(g, next, true);
   moves -= (mobile::mp_turn / 2) * 5;
  // end C:DDA refactor target monster::bash_at
  } else if (can_move_to(g->m, next) && g->is_empty(next))
   move_to(g, next);
  else
   moves -= mobile::mp_turn;
 }

// If we're close to our target, we get focused and don't stumble
 if ((has_flag(MF_STUMBLES) && (plans.size() > 3 || plans.size() == 0)) || !moved)
  stumble(g, moved);
}

// footsteps will determine how loud a monster's normal movement is
// and create a sound in the monsters location when they move
void monster::footsteps(game *g, const point& pt)
{
 if (made_footstep) return;
 if (has_flag(MF_FLIES)) return; // Flying monsters don't have footsteps!
 made_footstep = true;
 int volume = 6; // same as player's footsteps
 if (has_flag(MF_DIGS)) volume = 10;
 switch (type->size) {
  case MS_TINY: return; // No sound for the tinies
  case MS_SMALL:
   volume /= 3;
   break;
  case MS_MEDIUM: break;
  case MS_LARGE:
   volume *= 3;
   volume /= 2;
   break;
  case MS_HUGE:
   volume *= 2;
   break;
  default: break;
 }
 g->u.add_footstep(pt, volume);	// \todo V 0.2.1+ would be interesting, but very expensive in CPU, to put npcs on the same infrastructure (moves footsteps storage to player object)
 return;
}

void monster::friendly_move(game *g)
{
 moves -= mobile::mp_turn;
 if (plans.empty() || !can_enter(g->m, plans[0])) {
     stumble(g, false);
     return;
 }
  const point next(plans[0]);
  EraseAt(plans, 0);
  const auto _mob = g->mob_at(next);
  if (_mob && std::visit(monster::is_enemy_of(*this), *_mob) && 0 < type->melee_dice) {
#ifndef NDEBUG
      throw std::logic_error("should not be executing: _mob && std::visit(monster::is_enemy_of(*this), *_mob) && 0 < type->melee_dice");
#else
      debuglog("should not be executing: _mob && std::visit(monster::is_enemy_of(*this), *_mob) && 0 < type->melee_dice");
      return;
#endif
  }
  if (!_mob && can_move_to(g->m, next)) move_to(g, next);
  else if ((!can_move_to(g->m, next) || one_in(3)) &&
      g->m.has_flag(bashable, next) && has_flag(MF_BASHES)) {
      std::string bashsound = "NOBASH"; // If we hear "NOBASH" it's time to debug!
      int bashskill = int(type->melee_dice * type->melee_sides);
      g->m.bash(next, bashskill, bashsound);
      g->sound(next, 18, bashsound);
  }
  else if (g->m.move_cost(next) == 0 && has_flag(MF_DESTROYS)) {
      g->m.destroy(g, next, true);
      moves -= (mobile::mp_turn / 2) * 5;
  }
}

std::optional<point> monster::scent_move()
{
 const auto g = game::active_const();
 const bool flee = is_fleeing(g->u);
 int smell_threshold = flee ? INT_MAX : 1; // Squares with smell 0 are not eligible targets
 plans.clear();
 std::vector<point> smoves;
 for (decltype(auto) dir : Direction::vector) {
     point test(pos + dir);
     auto _mob = g->mob_at(test);
     if (  (!_mob || std::visit(is_enemy_of(*this), *_mob))
         && can_sound_move_to(test)) {
         const auto smell = g->scent(test);
         if (flee) {
             if (smell > smell_threshold) continue;
             if (smell < smell_threshold) {
                 smoves.clear();
                 smell_threshold = smell;
             }
         } else {
             if (smell < smell_threshold) continue;
             if (smell > smell_threshold) {
                 smoves.clear();
                 smell_threshold = smell;
             }
        }
       smoves.push_back(test);
     }
 }
 if (!smoves.empty()) return smoves[rng(0, smoves.size() - 1)];
 return std::nullopt;
}

bool monster::can_sound_move_to(const point& pt) const
{
    const auto g = game::active_const();
    if (can_enter(g->m, pt)) return true;
    if (auto _mob = g->mob_at(pt)) return std::visit(is_enemy_of(*this), *_mob); // melee attack
    return false;
}

point monster::sound_move()
{
 plans.clear();
 const bool xbest = (abs(wand.x.y - pos.y) <= abs(wand.x.x - pos.x));	// which is more important
 point next(pos);
 int x = pos.x, x2 = pos.x - 1, x3 = pos.x + 1;
 int y = pos.y, y2 = pos.y - 1, y3 = pos.y + 1;
 if (wand.x.x < pos.x) { x--; x2++;          }
 else if (wand.x.x > pos.x) { x++; x2++; x3 -= 2; }
 if (wand.x.y < pos.y) { y--; y2++;          }
 else if (wand.x.y > pos.y) { y++; y2++; y3 -= 2; }

 static auto _can_sound_move_to = [&](const auto& pt) {
     if (can_sound_move_to(pt)) {
         next = pt;
         return true;
     }
     return false;
 };

 if (!_can_sound_move_to(point(x, y))) {
	 if (xbest) {
		    _can_sound_move_to(point(x, y2)) || _can_sound_move_to(point(x2, y))
	     || _can_sound_move_to(point(x, y3)) || _can_sound_move_to(point(x3, y));
	 } else {
           _can_sound_move_to(point(x2, y)) || _can_sound_move_to(point(x, y2))
	    || _can_sound_move_to(point(x3, y)) || _can_sound_move_to(point(x, y3));
	 }
 }

 return next;
}

void monster::hit_player(game *g, player &p, bool can_grab)
{
 if (type->melee_dice == 0) return; // We don't attack, so just return
 add_effect(ME_HIT_BY_PLAYER, 3); // Make us a valid target for a few turns
 if (has_flag(MF_HIT_AND_RUN)) add_effect(ME_RUN, 4);
 bool is_npc = p.is_npc();
 bool u_see = (!is_npc || g->u.see(p.pos));
 std::string you  = (is_npc ? p.name : "you");
 std::string You  = (is_npc ? p.name : "You");
 std::string your = (is_npc ? p.name + "'s" : "your");
 std::string Your = (is_npc ? p.name + "'s" : "Your");
 body_part bphit;
 int side = rng(0, 1);
 int dam = hit(g, p, bphit), cut = type->melee_cut, stab = 0;
 technique_id tech = p.pick_defensive_technique(this, nullptr);
 p.perform_defensive_technique(tech, g, this, nullptr, bphit, side, dam, cut, stab);
 if (dam == 0 && u_see) messages.add("The %s misses %s.", name().c_str(), you.c_str());
 else if (dam > 0) {
  if (u_see && tech != TEC_BLOCK)
   messages.add("The %s hits %s %s.", name().c_str(), your.c_str(), body_part_name(bphit, side));
// Attempt defensive moves

  if (!is_npc) {
   if (g->u.activity.type == ACT_RELOAD) messages.add("You stop reloading.");
   else if (g->u.activity.type == ACT_READ) messages.add("You stop reading.");
   else if (g->u.activity.type == ACT_CRAFT) messages.add("You stop crafting.");
   g->u.activity.type = ACT_NULL;
  }
  if (p.has_active_bionic(bio_ods)) {
   if (u_see) messages.add("%s offensive defense system shocks it!", Your.c_str());
   hurt(rng(10, 40));
  }
  if (p.encumb(bphit) == 0 && (p.has_trait(PF_SPINES) || p.has_trait(PF_QUILLS))) {
   int spine = rng(1, (p.has_trait(PF_QUILLS) ? 20 : 8));
   messages.add("%s %s puncture it!", Your.c_str(), (g->u.has_trait(PF_QUILLS) ? "quills" : "spines"));
   hurt(spine);
  }

  if (dam + cut <= 0) return; // Defensive technique canceled damage.

  p.hit(g, bphit, side, dam, cut);
  if (has_flag(MF_VENOM)) {
   if (!is_npc) messages.add("You're poisoned!");
   p.add_disease(DI_POISON, MINUTES(3));
  } else if (has_flag(MF_BADVENOM)) {
   if (!is_npc) messages.add("You feel poison flood your body, wracking you with pain...");
   p.add_disease(DI_BADPOISON, MINUTES(4));
  }
  if (can_grab && has_flag(MF_GRABS) && dice(type->melee_dice, 10) > dice(p.dodge(), 10)) {
   if (!is_npc) messages.add("The %s grabs you!", name().c_str());
   if (p.weapon.has_technique(TEC_BREAK, &p) &&
       dice(p.melee_skill(), 12) > dice(type->melee_dice, 10)){
    if (!is_npc) messages.add("You break the grab!");
   } else
    hit_player(g, p, false);
  }
     
  if (tech == TEC_COUNTER && !is_npc) {
   messages.add("Counter-attack!");
   hurt( p.hit_mon(g, this) );
  }
 } // if dam > 0
 if (is_npc) {
  if (p.hp_cur[hp_head] <= 0 || p.hp_cur[hp_torso] <= 0) {
   dynamic_cast<npc*>(&p)->die(g);
   plans.clear();
  }
 }
// Adjust anger/morale of same-species monsters, if appropriate
// we do not use monster::process_trigger here as we're bulk-updating
 int anger_adjust = 0, morale_adjust = 0;
 if (type->anger & mfb(MTRIG_FRIEND_ATTACKED)) anger_adjust += 15;
 if (type->placate & mfb(MTRIG_FRIEND_ATTACKED)) anger_adjust -= 15;
 if (type->fear & mfb(MTRIG_FRIEND_ATTACKED)) morale_adjust -= 15;

 // No FOV/hearing/... in-communication check (inherited from C:Whales)
 if (anger_adjust != 0 || morale_adjust != 0) {
  for(auto& _mon : g->z) {
   if (type->species != _mon.type->species) continue;
   _mon.morale += morale_adjust;
   _mon.anger += anger_adjust;
  }
 }
}

void monster::move_to(game *g, const point& pt)
{
    if (const auto mob_plan = g->mob_at(g->toGPS(pt))) {
        // can't reuse mob_plan because of dead hallucination path
        if (melee_target::can_construct(*this) && std::visit(melee_target(*this), *mob_plan)) {
            // we have  melee'ed a hostile target
            // unlike our exemplar, move points have already been deducted
            return;
        }
        // \todo other interesting behaviors (side-step, etc.)
        return;
    }

    if (has_effect(ME_BEARTRAP)) {
        moves = 0;
        return;
    }
    if (!plans.empty()) EraseAt(plans, 0);
    const unsigned int not_landbound = has_flag(MF_DIGS) + 2 * has_flag(MF_FLIES);
    const bool is_swimming = (has_flag(MF_SWIMS) && g->m.has_flag(swimmable, pt));
    if (is_swimming) moves += mobile::mp_turn / 2;
    else if (!not_landbound) moves -= (g->m.move_cost(pt) - 2) * (mobile::mp_turn / 2);
    screenpos_set(pt);
    footsteps(g, pt);
    if (!not_landbound) {
        if (const auto tr_at = GPSpos.trap_at()) { // Monster stepped on a trap!
            const trap* const tr = trap::traps[tr_at];
            if (dice(3, sk_dodge + 1) < dice(3, tr->avoidance)) tr->trigger(*this);
        }
    }
    // Diggers turn the dirt into dirtmound
    if (1 == not_landbound % 2) GPSpos.ter() = t_dirtmound;
    // Acid trail monsters leave... a trail of acid
    if (has_flag(MF_ACIDTRAIL)) g->m.add_field(g, pos, fd_acid, 1);
}

/* Random walking even when we've moved
 * To simulate zombie stumbling and ineffective movement
 * Note that this is sub-optimal; stumbling may INCREASE a zombie's speed.
 * Most of the time (out in the open) this effect is insignificant compared to
 * the negative effects, but in a hallway it's perfectly even
 */
// to fix this, we'd need both the previous and current location
void monster::stumble(game *g, bool moved)
{
 std::vector<point> valid_stumbles;
 for (int i = -1; i <= 1; i++) {
  for (int j = -1; j <= 1; j++) {
   const point dest(pos.x + i, pos.y + j);
   if (can_move_to(g->m, dest) && g->u.pos != dest &&
       (!g->mon(dest) || (i == 0 && j == 0))) {
    valid_stumbles.push_back(dest);
   }
  }
 }
 const size_t ub = valid_stumbles.size();
 if (0 < ub && (one_in(8) || (!moved && one_in(3)))) {
  screenpos_set(valid_stumbles[rng(0, ub - 1)]);
  if (!has_flag(MF_DIGS) || !has_flag(MF_FLIES))
   moves -= (g->m.move_cost(pos) - 2) * (mobile::mp_turn / 2);
// Here we have to fix our plans[] list, trying to get back to the last point
// Otherwise the stumble will basically have no effect!
  if (!plans.empty()) {
   if (const auto tc = g->m.sees(pos, plans[0], -1)) {
// Copy out old plans...
    const std::vector<point> plans2(std::move(plans));
// Set plans to a route between where we are now, and where we were
    set_dest(plans2[0], *tc);
// Append old plans to the new plans
    for (decltype(auto) pt : plans2) plans.push_back(pt);
   } else
    plans.clear();
  }
 }
}

/* will_reach() is used for determining whether we'll get to stairs (and 
 * potentially other locations of interest).  It is generally permissive.
 * TODO: Pathfinding;
         Make sure that non-smashing monsters won't "teleport" through windows
         Injure monsters if they're gonna be walking through pits or whatevs
 */
bool monster::will_reach(const game *g, const point& pt) const
{
 monster_attitude att = attitude(&(g->u));
 if (att != MATT_FOLLOW && att != MATT_ATTACK && att != MATT_FRIEND) return false;
 if (has_flag(MF_DIGS)) return false;
 if (has_flag(MF_IMMOBILE) && pos != pt) return false;

 if (has_flag(MF_SMELLS)) {
	 const auto scent = g->scent(pos);
	 if ( 0 < scent && g->scent(pt) > scent) return true;
 }

 if (can_hear() && wand.live() && rl_dist(wand.x, pt) <= 2 && rl_dist(pos, wand.x) <= wand.remaining)
  return true;

 if (can_see() && g->m.sees(pos, pt, g->light_level())) return true;

 return false;
}

int monster::turns_to_reach(const map& m, const point& pt) const
{
 std::vector<point> path = m.route(pos, pt, has_flag(MF_BASHES));
 if (path.size() == 0) return 999;

 double turns = 0.;
 for(const auto& step : path) {
  const auto cost = m.move_cost(step);
  turns += (0 >= cost) ? 5	// We have to bash through
		 : double(50 * cost) / speed;
 }
 return int(turns + .9); // Round up
}
