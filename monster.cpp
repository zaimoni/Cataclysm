#include "monster.h"
#include "game.h"
#include "recent_msg.h"
#include "saveload.h"

#include <sstream>
#include <fstream>
#include <stdlib.h>

#define SGN(a) (((a)<0) ? -1 : 1)
#define SQR(a) ((a)*(a))

monster::monster()
: pos(20, 10), wand(-1, -1), spawnmap(-1, -1), spawnpos(-1,-1)
{
 wandf = 0;
 type = NULL;
 hp = 60;
 moves = 0;
 sp_timeout = 0;
 friendly = 0;
 anger = 0;
 morale = 2;
 faction_id = -1;
 mission_id = -1;
 dead = false;
 made_footstep = false;
 unique_name = "";
}

monster::monster(const mtype *t)
: pos(20, 10), wand(-1, -1), spawnmap(-1, -1), spawnpos(-1,-1)
{
 wandf = 0;
 type = t;
 moves = type->speed;
 speed = type->speed;
 hp = type->hp;
 sp_timeout = rng(0, type->sp_freq);
 friendly = 0;
 anger = t->agro;
 morale = t->morale;
 faction_id = -1;
 mission_id = -1;
 dead = false;
 made_footstep = false;
 unique_name = "";
}

monster::monster(const mtype *t, int x, int y)
: pos(x, y), wand(-1, -1), spawnmap(-1, -1), spawnpos(-1,-1)
{
 wandf = 0;
 type = t;
 moves = type->speed;
 speed = type->speed;
 hp = type->hp;
 sp_timeout = type->sp_freq;
 friendly = 0;
 anger = type->agro;
 morale = type->morale;
 faction_id = -1;
 mission_id = -1;
 dead = false;
 made_footstep = false;
 unique_name = "";
}

void monster::poly(const mtype *t)
{
 double hp_percentage = double(hp) / double(type->hp);
 type = t;
 moves = 0;
 speed = type->speed;
 anger = type->agro;
 morale = type->morale;
 hp = int(hp_percentage * type->hp);
 sp_timeout = type->sp_freq;
}

// definitely could be a complex implementation, so retain out-of-line definition
void monster::spawn(int x, int y)
{
 pos = point(x,y);
}

void monster::spawn(const point& pt)
{
	pos = pt;
}

std::string monster::name() const
{
 if (!type) {
  debugmsg ("monster::name empty type!");
  return std::string();
 }
 if (unique_name != "")
  return type->name + ": " + unique_name;
 return type->name;
}

std::string monster::name_with_armor() const
{
 std::string ret = type->name;
 if (type->species == species_insect)
  ret += "'s carapace";
 else {
  switch (type->mat) {
   case VEGGY: ret += "'s thick bark";    break;
   case FLESH: ret += "'s thick hide";    break;
   case IRON:
   case STEEL: ret += "'s armor plating"; break;
  }
 }
 return ret;
}

void monster::print_info(const player& u, WINDOW* w) const
{
// First line of w is the border; the next two are terrain info, and after that
// is a blank line. w is 13 characters tall, and we can't use the last one
// because it's a border as well; so we have lines 4 through 11.
// w is also 48 characters wide - 2 characters for border = 46 characters for us
 mvwprintz(w, 6, 1, c_white, "%s ", type->name.c_str());
 switch (attitude(&u)) {
  case MATT_FRIEND:
   wprintz(w, h_white, "Friendly! ");
   break;
  case MATT_FLEE:
   wprintz(w, c_green, "Fleeing! ");
   break;
  case MATT_IGNORE:
   wprintz(w, c_ltgray, "Ignoring ");
   break;
  case MATT_FOLLOW:
   wprintz(w, c_yellow, "Tracking ");
   break;
  case MATT_ATTACK:
   wprintz(w, c_red, "Hostile! ");
   break;
  default:
   wprintz(w, h_red, "BUG: Behavior unnamed ");
   break;
 }
 if (has_effect(ME_DOWNED))
  wprintz(w, h_white, "On ground");
 else if (has_effect(ME_STUNNED))
  wprintz(w, h_white, "Stunned");
 else if (has_effect(ME_BEARTRAP))
  wprintz(w, h_white, "Trapped");
 std::string damage_info;
 nc_color col;
 if (hp == type->hp) {
  damage_info = "It is uninjured";
  col = c_green;
 } else if (hp >= type->hp * .8) {
  damage_info = "It is lightly injured";
  col = c_ltgreen;
 } else if (hp >= type->hp * .6) {
  damage_info = "It is moderately injured";
  col = c_yellow;
 } else if (hp >= type->hp * .3) {
  damage_info = "It is heavily injured";
  col = c_yellow;
 } else if (hp >= type->hp * .1) {
  damage_info = "It is severly injured";
  col = c_ltred;
 } else {
  damage_info = "it is nearly dead";
  col = c_red;
 }
 mvwprintz(w, 7, 1, col, damage_info.c_str());

 std::string tmp = type->description;
 size_t pos;
 int line = 8;
 do {
  pos = tmp.find_first_of('\n');
  mvwprintz(w, line, 1, c_white, tmp.substr(0, pos).c_str());
  tmp = tmp.substr(pos + 1);
  line++;
 } while (pos != std::string::npos && line < 12);
}

void monster::draw(WINDOW *w, const point& pl, bool inv) const
{
 point pt = pos - pl + point(SEE);
 nc_color color = type->color;
 char sym = type->sym;

 if (mtype::tiles.count(type->id)) {
	 if (mvwaddfgtile(w, pt.y, pt.x, mtype::tiles[type->id].c_str())) sym = ' ';	// we still want any background color effects
 }

 if (friendly != 0 && !inv)
  mvwputch_hi(w, pt.y, pt.x, color, sym);
 else if (inv)
  mvwputch_inv(w, pt.y, pt.x, color, sym);
 else {
  color = color_with_effects();
  mvwputch(w, pt.y, pt.x, color, sym);
 }
}

nc_color monster::color_with_effects() const
{
 nc_color ret = type->color;
 if (has_effect(ME_BEARTRAP) || has_effect(ME_STUNNED) || has_effect(ME_DOWNED))
  ret = hilite(ret);
 if (has_effect(ME_ONFIRE))
  ret = red_background(ret);
 return ret;
}

bool monster::can_see() const
{
 return has_flag(MF_SEES) && !has_effect(ME_BLIND);
}

bool monster::can_hear() const
{
 return has_flag(MF_HEARS) && !has_effect(ME_DEAF);
}

// this legitimately could be a more complicated implementation
bool monster::made_of(material m) const
{
 return type->mat == m;
}
 
void monster::load_info(std::string data)
{
 std::stringstream dump;
 int idtmp, plansize;
 dump << data;
 dump >> idtmp >> pos >> wand >> wandf >> moves >> speed >> hp >>  sp_timeout >>
	     plansize >> friendly >> faction_id >> mission_id >> dead >> anger >> 
	     morale;
 type = mtype::types[idtmp];
 point ptmp;
 for (int i = 0; i < plansize; i++) {
  dump >> ptmp;
  plans.push_back(ptmp);
 }
}

std::string monster::save_info() const
{
 std::stringstream pack;
 pack << int(type->id) << " " << pos << " " << wand << " " <<  wandf << " " <<
	     moves << " " << speed << " " << hp << " " <<  sp_timeout << " " << 
	     plans.size() << " " << friendly << " " << faction_id << " " << 
	     mission_id << " " << dead << " " << anger << " " << morale;
 for (int i = 0; i < plans.size(); i++) {
  pack << " " << plans[i];
 }
 return pack.str();
}

void monster::debug(player &u)
{
 char buff[2];
 debugmsg("%s has %d steps planned.", name().c_str(), plans.size());
 debugmsg("%s Moves %d Speed %d HP %d",name().c_str(), moves, speed, hp);
 for (int i = 0; i < plans.size(); i++) {
  sprintf(buff, "%d", i);
  if (i < 10) mvaddch(plans[i].y - SEEY + u.pos.y, plans[i].x - SEEX + u.pos.x, buff[0]);
  else mvaddch(plans[i].y - SEEY + u.pos.y, plans[i].x - SEEX + u.pos.x, buff[1]);
 }
 getch();
}

void monster::shift(int sx, int sy)
{
 pos.x -= sx * SEEX;
 pos.y -= sy * SEEY;
 for (int i = 0; i < plans.size(); i++) {
  plans[i].x -= sx * SEEX;
  plans[i].y -= sy * SEEY;
 }
}

bool monster::is_fleeing(const player &u) const
{
 if (has_effect(ME_RUN)) return true;
 monster_attitude att = attitude(&u);
 return (att == MATT_FLEE ||
         (att == MATT_FOLLOW && rl_dist(pos, u.pos) <= 4));
}

monster_attitude monster::attitude(const player *u) const
{
 if (friendly != 0) return MATT_FRIEND;
 if (has_effect(ME_RUN)) return MATT_FLEE;

 int effective_anger  = anger;
 int effective_morale = morale;

 if (u != NULL) {

  if (((type->species == species_mammal && u->has_trait(PF_PHEROMONE_MAMMAL)) ||
       (type->species == species_insect && u->has_trait(PF_PHEROMONE_INSECT)))&&
      effective_anger >= 10)
   effective_anger -= 20;

  if (u->has_trait(PF_TERRIFYING)) effective_morale -= 10;

  if (u->has_trait(PF_ANIMALEMPATH) && has_flag(MF_ANIMAL)) {
   if (effective_anger >= 10) effective_anger -= 10;
   if (effective_anger < 10) effective_morale += 5;
  }

 }

 if (effective_morale < 0) return (effective_morale + effective_anger > 0) ? MATT_FOLLOW : MATT_FLEE;
 if (effective_anger < 0) return MATT_IGNORE;
 if (effective_anger < 10) return MATT_FOLLOW;
 return MATT_ATTACK;
}

void monster::process_triggers(const game *g)
{
 anger += trigger_sum(g, type->anger);
 anger -= trigger_sum(g, type->placate);
 if (morale < 0) {
  if (morale < type->morale && one_in(20))
  morale++;
 } else
  morale -= trigger_sum(g, type->fear);
}

// This Adjustes anger/morale levels given a single trigger.
void monster::process_trigger(monster_trigger trig, int amount)
{
 for (const auto trigger : type->anger) if (trigger == trig) anger += amount;
 for (const auto trigger : type->placate) if (trigger == trig) anger -= amount;
 for (const auto trigger : type->fear) if (trigger == trig) morale -= amount;
}

int monster::trigger_sum(const game *g, const std::vector<monster_trigger>& triggers) const
{
 int ret = 0;
 bool check_terrain = false, check_meat = false, check_fire = false;
 for (const auto trigger : triggers) {

  switch (trigger) {
  case MTRIG_TIME:
   if (one_in(20)) ret++;
   break;

  case MTRIG_MEAT:
   check_terrain = true;
   check_meat = true;
   break;

  case MTRIG_PLAYER_CLOSE:
   if (rl_dist(pos, g->u.pos) <= 5) ret += 5;
   for (int i = 0; i < g->active_npc.size(); i++) {
    if (rl_dist(pos, g->active_npc[i].pos) <= 5)
     ret += 5;
   }
   break;

  case MTRIG_FIRE:
   check_terrain = true;
   check_fire = true;
   break;

  case MTRIG_PLAYER_WEAK:
   if (g->u.hp_percentage() <= 70)
    ret += 10 - int(g->u.hp_percentage() / 10);
   break;

  default:
   break; // The rest are handled when the impetus occurs
  }
 }

 if (check_terrain) {
  for (int x = pos.x - 3; x <= pos.x + 3; x++) {
   for (int y = pos.y - 3; y <= pos.y + 3; y++) {
    if (check_meat) {
	 for(const auto& obj : g->m.i_at(x, y)) {
      if (obj.type->id == itm_corpse ||
		  obj.type->id == itm_meat ||
		  obj.type->id == itm_meat_tainted) {
       ret += 3;
       check_meat = false;
      }
     }
    }
    if (check_fire) {
	 const auto fd = g->m.field_at(x, y);
     if (fd.type == fd_fire) ret += 5 * fd.density;
    }
   }
  }
 }

 return ret;
}

int monster::hit(game *g, player &p, body_part &bp_hit)
{
 static const int base_bodypart_hitrange[mtype::MS_MAX] = {3, 12, 20, 28, 35};

 int numdice = type->melee_skill;
 if (dice(numdice, 10) <= dice(p.dodge(g), 10) && !one_in(20)) {
  if (numdice > p.sklevel[sk_dodge])
   p.practice(sk_dodge, 10);
  return 0;	// We missed!
 }
 p.practice(sk_dodge, 5);
 int ret = 0;
 int highest_hit = base_bodypart_hitrange[type->size];

 if (has_flag(MF_DIGS)) highest_hit -= 8;
 if (has_flag(MF_FLIES)) highest_hit += 20;
 if (highest_hit <= 1) highest_hit = 2;
 if (highest_hit > 20) highest_hit = 20;

 int bp_rand = rng(0, highest_hit - 1);
 if (bp_rand <=  2)
  bp_hit = bp_legs;
 else if (bp_rand <= 10)
  bp_hit = bp_torso;
 else if (bp_rand <= 14)
  bp_hit = bp_arms;
 else if (bp_rand <= 16)
  bp_hit = bp_mouth;
 else if (bp_rand == 17)
  bp_hit = bp_eyes;
 else
  bp_hit = bp_head;
 ret += dice(type->melee_dice, type->melee_sides);
 return ret;
}

void monster::hit_monster(game *g, monster& target)
{
 static const int dodge_bonus[mtype::MS_MAX] = {6, 3, 0, -2, -4};

 int numdice = type->melee_skill;
 int dodgedice = target.dodge() * 2;	// decidedly more agile than a typical dodge roll
 dodgedice += dodge_bonus[target.type->size];

 if (dice(numdice, 10) <= dice(dodgedice, 10)) {
  if (g->u_see(this)) messages.add("The %s misses the %s!", name().c_str(), target.name().c_str());
  return;
 }
 if (g->u_see(this)) messages.add("The %s hits the %s!", name().c_str(), target.name().c_str());
 int damage = dice(type->melee_dice, type->melee_sides);
 if (target.hurt(damage)) g->kill_mon(target, (friendly != 0));
}
 

bool monster::hurt(int dam)
{
 hp -= dam;
 if (hp < 1) return true;
 if (dam > 0) process_trigger(MTRIG_HURT, 1 + int(dam / 3));
 return false;
}

int monster::armor_cut() const
{
 return int(type->armor_cut);	// \todo worn armor
}

int monster::armor_bash() const
{
 return int(type->armor_bash);	// \todo worn armor
}

int monster::dodge() const
{
 if (has_effect(ME_DOWNED)) return 0;
 int ret = type->sk_dodge;
 if (has_effect(ME_BEARTRAP)) ret /= 2;
 if (moves <= 0 - 100 - type->speed) ret = rng(0, ret);
 return ret;
}

int monster::dodge_roll() const
{
 static const int dodge_bonus[mtype::MS_MAX] = {6, 3, 0, -2, -4};

 const int numdice = dodge()+dodge_bonus[type->size] + (speed / 80);
 return dice(numdice, 10);
}

int monster::fall_damage() const
{
 if (has_flag(MF_FLIES)) return 0;
 switch (type->size) {
  case MS_TINY:   return rng(0, 4);  break;
  case MS_SMALL:  return rng(0, 6);  break;
  case MS_MEDIUM: return dice(2, 4); break;
  case MS_LARGE:  return dice(2, 6); break;
  case MS_HUGE:   return dice(3, 5); break;
 }

 return 0;
}

void monster::die(game *g)
{
 if (!dead) dead = true;
// Drop goodies
 int total_chance = 0, total_it_chance, cur_chance, selected_location,
     selected_item;
 bool animal_done = false;
 std::vector<items_location_and_chance> it = mtype::items[type->id];
 std::vector<itype_id> mapit;
 if (type->item_chance != 0 && it.size() == 0)
  debugmsg("Type %s has item_chance %d but no items assigned!",
           type->name.c_str(), type->item_chance);
 else {
  for (int i = 0; i < it.size(); i++)
   total_chance += it[i].chance;

  while (rng(0, 99) < abs(type->item_chance) && !animal_done) {
   cur_chance = rng(1, total_chance);
   selected_location = -1;
   while (cur_chance > 0) {
    selected_location++;
    cur_chance -= it[selected_location].chance;
   }
   total_it_chance = 0;
   mapit = map::items[it[selected_location].loc];
   for (int i = 0; i < mapit.size(); i++)
    total_it_chance += item::types[mapit[i]]->rarity;
   cur_chance = rng(1, total_it_chance);
   selected_item = -1;
   while (cur_chance > 0) {
    selected_item++;
    cur_chance -= item::types[mapit[selected_item]]->rarity;
   }
   g->m.add_item(pos, item::types[mapit[selected_item]], 0);
   if (type->item_chance < 0) animal_done = true;	// Only drop ONE item.
  }
 } // Done dropping items

// If we're a queen, make nearby groups of our type start to die out
 if (has_flag(MF_QUEEN)) {
  for(mongroup* const mon_gr : g->cur_om.monsters_at(g->lev.x, g->lev.y)) {
   for(const auto tmp_id : mongroup::moncats[mon_gr->type]) {
    if (tmp_id == type->id) {
	 mon_gr->dying = true;
	 break;
	}
   }
  }
// Do it for overmap above/below too
  overmap tmp(g, g->cur_om.pos.x, g->cur_om.pos.y,(g->cur_om.pos.z == 0 ? -1 : 0));

  for(mongroup* const mon_gr : tmp.monsters_at(g->lev.x, g->lev.y)) {
   for(const auto tmp_id : mongroup::moncats[mon_gr->type]) {
    if (tmp_id == type->id) {
	 mon_gr->dying = true;
	 break;
	}
   }
  }
 }
// If we're a mission monster, update the mission
 if (mission_id != -1) {
  const mission_type *misstype = g->find_mission_type(mission_id);
  if (misstype) {
    if (misstype->goal == MGOAL_FIND_MONSTER) g->fail_mission(mission_id);
    if (misstype->goal == MGOAL_KILL_MONSTER) g->mission_step_complete(mission_id, 1);
  } else mission_id = -1;	// active reset
 }
// Also, perform our death function
 (type->dies)(g, this);
// If our species fears seeing one of our own die, process that
 int anger_adjust = 0, morale_adjust = 0;
 for (const auto tr : type->anger) if (tr == MTRIG_FRIEND_DIED) anger_adjust += 15;
 for (const auto tr : type->placate) if (tr == MTRIG_FRIEND_DIED) anger_adjust -= 15;
 for (const auto tr : type->fear) if (tr == MTRIG_FRIEND_DIED) morale_adjust -= 15;
 if (anger_adjust != 0 || morale_adjust != 0) {
  int light = g->light_level();
  for (auto& critter : g->z) {
   if (critter.type->species != type->species) continue;
   if (!g->m.sees(critter.pos, pos, light)) continue;
   critter.morale += morale_adjust;
   critter.anger += anger_adjust;
  }
 }
}

void monster::add_effect(monster_effect_type effect, int duration)
{
 for (auto& e : effects) {
  if (e.type == effect) {
   e.duration += duration;
   return;
  }
 }
 effects.push_back(monster_effect(effect, duration));
}

bool monster::has_effect(monster_effect_type effect) const
{
 for (const auto& e : effects) if (e.type == effect) return true;
 return false;
}

void monster::rem_effect(monster_effect_type effect)
{
 for (int i = 0; i < effects.size(); i++) {
  if (effects[i].type == effect) {
   effects.erase(effects.begin() + i);
   i--;	// \todo micro-optimize: return here since add effect guarantees only one effect of any given type?
  }
 }
}

void monster::process_effects()
{
 for (int i = 0; i < effects.size(); i++) {
  switch (effects[i].type) {
  case ME_POISONED:
   speed -= rng(0, 3);
   hurt(rng(1, 3));
   break;

  case ME_ONFIRE:
   if (made_of(FLESH))
    hurt(rng(3, 8));
   if (made_of(VEGGY))
    hurt(rng(10, 20));
   if (made_of(PAPER) || made_of(POWDER) || made_of(WOOD) || made_of(COTTON) ||
       made_of(WOOL))
    hurt(rng(15, 40));
   break;

  }
  if (effects[i].duration > 0) {
   effects[i].duration--;
   if (game::debugmon) debugmsg("Duration %d", effects[i].duration);
  }
  if (effects[i].duration == 0) {
   if (game::debugmon) debugmsg("Deleting");
   effects.erase(effects.begin() + i);
   i--;
  }
 }
}

bool monster::make_fungus()
{
 if (species_hallu == type->species) return true;
 if (species_fungus == type->species) return true;	// No friendly-fungalizing ;-)
 if (!made_of(FLESH) && !made_of(VEGGY)) return true;	// No fungalizing robots or weird stuff

 // we have significant coverage holes below
 switch (mon_id(type->id)) {
 case mon_ant:
 case mon_ant_soldier:
 case mon_ant_queen:
 case mon_fly:
 case mon_bee:
 case mon_dermatik:
  poly(mtype::types[mon_ant_fungus]);	// description is actually "fungal insect" so tolerable (more detail would be useful)
  return true;
 case mon_zombie_master:	// have enough Goo to resist conversion
 case mon_zombie_necro:
  return true;
 case mon_zombie:
 case mon_zombie_shrieker:
 case mon_zombie_electric:
 case mon_zombie_spitter:
 case mon_zombie_fast:
 case mon_zombie_brute:
 case mon_zombie_hulk:
  poly(mtype::types[mon_zombie_fungus]);
  return true;
 case mon_boomer:
  poly(mtype::types[mon_boomer_fungus]);
  return true;
 case mon_triffid:
 case mon_triffid_young:
 case mon_triffid_queen:
  poly(mtype::types[mon_fungaloid]);
  return true;
 default:
  return true;
 }
 return false;
}

void monster::make_friendly()
{
 plans.clear();
 friendly = rng(5, 30) + rng(0, 20);
}

void monster::add_item(const item& it)
{
 inv.push_back(it);
}
