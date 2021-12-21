#include "monster.h"
#include "game.h"
#include "line.h"
#include "rng.h"
#include "recent_msg.h"
#include "om_cache.hpp"
#include "saveload.h"
#include "stl_limits.h"

#include <fstream>
#include <stdlib.h>

static const char* const JSON_transcode_meffects[] = {
	"BEARTRAP",
	"POISONED",
	"ONFIRE",
	"STUNNED",
	"DOWNED",
	"BLIND",
	"DEAF",
	"TARGETED",
	"DOCILE",
	"HIT_BY_PLAYER",
	"RUN"
};

DEFINE_JSON_ENUM_SUPPORT_HARDCODED_NONZERO(monster_effect_type, JSON_transcode_meffects)

monster::monster() noexcept
: mobile(overmap::toGPS(point(20, 10)), 0), pos(20, 10), wand(point(-1, -1), 0), spawnmap(-1, -1), spawnpos(-1,-1), speed(0), hp(60),
  sp_timeout(0), friendly(0), anger(0), morale(2), faction_id(-1), mission_id(-1),type(nullptr),dead(false),made_footstep(false)
{
}

monster::monster(const mtype *t) noexcept : monster(t, point(20, 10)) {}

monster::monster(const mtype *t, point origin) noexcept
: mobile(overmap::toGPS(origin), t->speed), pos(origin), wand(point(-1, -1),0), spawnmap(-1, -1), spawnpos(-1, -1), speed(t->speed), hp(t->hp),
  sp_timeout(t->sp_freq), friendly(0), anger(t->agro), morale(t->morale), faction_id(-1), mission_id(-1), type(t), dead(false), made_footstep(false)
{
}

DEFINE_ACID_ASSIGN_W_MOVE(monster)

void monster::screenpos_set(point pt) { GPSpos = overmap::toGPS(pos = pt); }
void monster::screenpos_set(int x, int y) { GPSpos = overmap::toGPS(pos = point(x, y)); }
void monster::screenpos_add(point delta) { GPSpos = overmap::toGPS(pos += delta); }

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
void monster::spawn(int x, int y) { screenpos_set(x, y); }
void monster::spawn(const point& pt) { screenpos_set(pt); }

// yes, could intentionally inline function body here
void monster::spawn(const GPS_loc& loc) { set_screenpos(loc); }

std::string monster::name() const
{
 if (!type) {
  debugmsg ("monster::name empty type!");
  return std::string();
 }
 if (!unique_name.empty())
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

std::string monster::subject() const
{
    return name();
}

std::string monster::direct_object() const
{
    return name();
}

std::string monster::indirect_object() const
{
    return name();
}

std::string monster::possessive() const
{
    auto ret(name());
    regular_possessive(ret);
    return ret;
}

static std::pair<nc_color, const char*> behavior_text(monster_attitude src)
{
    switch (src) {
    case MATT_FRIEND: return std::pair(h_white, " Friendly! ");
    case MATT_FLEE: return std::pair(c_green, " Fleeing! ");
    case MATT_IGNORE: return std::pair(c_ltgray, " Ignoring ");
    case MATT_FOLLOW: return std::pair(c_yellow, " Tracking ");
    case MATT_ATTACK: return std::pair(c_red, " Hostile! ");
    default: return std::pair(h_red, " BUG: Behavior unnamed ");
    }
}

static const char* mobility_text(const monster& _mon) {
    if (_mon.has_effect(ME_DOWNED)) return "On ground";
    if (_mon.has_effect(ME_STUNNED)) return "Stunned";
    if (_mon.has_effect(ME_BEARTRAP)) return "Trapped";
    return nullptr;
}

static std::pair<nc_color, const char*> health_text(int hp, int max_hp)
{
    if (hp >= max_hp) return std::pair(c_green, "It is uninjured");
    if (hp >= cataclysm::rational_scaled<4, 5>(max_hp)) return std::pair(c_ltgreen, "It is lightly injured");
    if (hp >= cataclysm::rational_scaled<3, 5>(max_hp)) return std::pair(c_yellow, "It is moderately injured");
    if (hp >= cataclysm::rational_scaled<3, 10>(max_hp)) return std::pair(c_yellow, "It is heavily injured");
    if (hp >= max_hp/10) return std::pair(c_ltred, "It is severly injured");
    return std::pair(c_red, "it is nearly dead");
}

void monster::print_info(const player& u, WINDOW* w) const
{
// First line of w is the border; the next two are terrain info, and after that
// is a blank line. w is historically 13 characters tall, and we can't use the last one
// because it's a border as well; so we have lines 4 through 11.
// w is also historically 48 characters wide - 2 characters for border = 46 characters for us
 mvwaddstrz(w, 6, 1, c_white, type->name.c_str());
 waddstrz(w, behavior_text(attitude(&u)));
 waddstrz(w, h_white, mobility_text(*this));
 mvwaddstrz(w, 7, 1, health_text(hp, type->hp));

 const int line_ub = getmaxy(w) - 1;
 std::string tmp = type->description;
 size_t pos;
 int line = 8;
 do {
  pos = tmp.find_first_of('\n');
  mvwaddstrz(w, line++, 1, c_white, tmp.substr(0, pos).c_str());
  tmp = tmp.substr(pos + 1);
 } while (pos != std::string::npos && line < line_ub);
}

void monster::draw(WINDOW *w, const point& pl, bool inv) const
{
 point pt = pos - pl + point(SEE);
 nc_color color = type->color;
 char sym = type->sym;

 if (mtype::tiles.count(type->id)) {
//	 if (mvwaddfgtile(w, pt.y, pt.x, mtype::tiles[type->id].c_str())) sym = ' ';	// we still want any background color effects
     mvwaddfgtile(w, pt.y, pt.x, mtype::tiles[type->id].c_str());
 }

 if (is_friend() && !inv) // viewpoint is PC
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

bool monster::ignitable() const
{
    return !made_of(LIQUID) && !made_of(STONE) && !made_of(KEVLAR) && !made_of(STEEL) && !has_flag(MF_FIREY);
}

std::optional<int> monster::see(const player& u) const
{
    if (u.has_active_bionic(bio_cloak) || u.has_artifact_with(AEP_INVISIBLE)) return std::nullopt;
    const auto g = game::active();
    return g->m.sees(pos, u.pos, g->light_level());
}

void monster::debug(player &u)
{
 char buff[16];
 debugmsg("%s has %d steps planned.", name().c_str(), plans.size());
 debugmsg("%s Moves %d Speed %d HP %d",name().c_str(), moves, speed, hp);
 for (int i = 0; i < plans.size(); i++) {
  sprintf(buff, "%d", i);
  const point pt(plans[i] - point(VIEW_CENTER) + u.pos);
  mvaddch(pt.y, pt.x, buff[(10 > i) ? 0 : 1]);
 }
 getch();
}

void monster::shift(const point& delta)
{
 const point delta_block(delta*SEE);
 screenpos_add(-delta_block);
 for (auto& pt : plans) pt -= delta_block;
}

bool monster::is_fleeing(const player &u) const
{
 if (has_effect(ME_RUN)) return true;
 monster_attitude att = attitude(&u);
 return (att == MATT_FLEE ||
         (att == MATT_FOLLOW && rl_dist(GPSpos, u.GPSpos) <= 4));
}

monster_attitude monster::attitude(const player *u) const
{
 if (is_friend(u)) return MATT_FRIEND;
 if (has_effect(ME_RUN)) return MATT_FLEE;

 int effective_anger  = anger;
 int effective_morale = morale;

 if (u != nullptr) {

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

// This adjusts anger/morale levels given a single trigger.
void monster::process_trigger(monster_trigger trig, int amount)
{
 if (type->anger & mfb(trig)) anger += amount;
 if (type->placate & mfb(trig)) anger -= amount;
 if (type->fear & mfb(trig)) morale -= amount;
}

int monster::trigger_sum(const game* g, typename cataclysm::bitmap<N_MONSTER_TRIGGERS>::type triggers) const
{
    int ret = 0;
    bool check_meat = (triggers & mfb(MTRIG_MEAT)), check_fire = (triggers & mfb(MTRIG_FIRE));

    if (triggers & mfb(MTRIG_TIME)) {
        if (one_in(20)) ret++;
    }

    if (triggers & mfb(MTRIG_PLAYER_CLOSE)) {
        if (rl_dist(GPSpos, g->u.GPSpos) <= 5) ret += 5;
        for (const auto& _npc : g->active_npc) if (5 >= rl_dist(GPSpos, _npc.GPSpos)) ret += 5;
    }

    if (triggers & mfb(MTRIG_PLAYER_WEAK)) {	// \todo why not NPCs?
        if (const int hp_percent = g->u.hp_percentage(); 70 >= hp_percent) ret += hp_percent / 10;
    }

    const bool check_terrain = check_meat || check_fire;
    if (check_terrain) {
        for (int x = pos.x - 3; x <= pos.x + 3; x++) {
            for (int y = pos.y - 3; y <= pos.y + 3; y++) {
                if (check_meat) {
                    for (const auto& obj : g->m.i_at(x, y)) {
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

 int numdice = melee_skill();
 if (!one_in(20) && dice(numdice, 10) <= dice(p.dodge(), 10)) {
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

 int numdice = melee_skill();
 int dodgedice = target.dodge() * 2;	// decidedly more agile than a typical dodge roll
 dodgedice += dodge_bonus[target.type->size];

 std::optional<std::string> my_name;
 std::optional<std::string> target_name;
 const bool seen = g->u.see(*this);
 if (seen) {
     my_name = grammar::capitalize(desc(grammar::noun::role::subject, grammar::article::definite));
     target_name = target.desc(grammar::noun::role::direct_object, grammar::article::definite);
 }

 if (dice(numdice, 10) <= dice(dodgedice, 10)) {
  if (seen) messages.add("%s misses %s!", my_name->c_str(), target_name->c_str());
  return;
 }
 if (seen) messages.add("%s hits %s!", my_name->c_str(), target_name->c_str());
 int damage = dice(type->melee_dice, type->melee_sides);
 if (target.hurt(damage)) g->kill_mon(target, this);
}

bool monster::hit_by_blob(monster* origin, bool force)
{
    // actual validity test is whether the attack handler is mattack::formblob
    assert(!origin || mon_blob == origin->type->id || mon_blob_small == origin->type->id);
    // origin's viewpoint: Hit a monster.  If it's a blob, give it our speed.  Otherwise, blobify it?
    switch (type->id) {
    case mon_blob:
        if (85 > speed) {
            if (origin) {
                if (20 < origin->speed) {
                    speed += 5;
                    origin->speed -= 5;
                    return true;
                }
            } else {
                speed += 15;
                hp = speed;
                return true;
            };
        };
        break;
    case mon_blob_small:
        // We expect our speed to be less than 85.
        if (origin) {
            if (20 < origin->speed) {
                speed += 5;
                origin->speed -= 5;
                if (60 <= speed) poly(mtype::types[mon_blob]);
                return true;
            }
        } else {
            speed += 15;
            hp = speed;
            if (60 <= speed) poly(mtype::types[mon_blob]);
            return true;
        };
        break;
    default:
        if (force && (made_of(FLESH) || made_of(VEGGY))) {	// Blobify!
            static auto enveloped = [&]() {
                return std::string("The goo envelops ")+ desc(grammar::noun::role::direct_object, grammar::article::indefinite)  +"!";
            };

            if_visible_message(enveloped);
            poly(mtype::types[mon_blob]);
            speed = (origin ? origin : this)->speed - rng(5, 25);
            hp = speed;
            return true;
        };
        break;
    }
    return false;
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

static void leaderless_hive(int id, const std::vector<mongroup*>& m_gr)
{
    for (const auto mon_gr : m_gr) {
        for (const auto tmp_id : mongroup::moncats[mon_gr->type]) {
            if (tmp_id == id) {
                mon_gr->dying = true;
                break;
            }
        }
    }
}

void monster::die(game *g)
{
 if (!dead) dead = true;
// Drop goodies
 if (type->item_chance) {
  const auto& it = mtype::items[type->id];
  assert(!it.empty()); // should be caught in mtype::init_items
  int total_chance = 0;
  for (decltype(auto) x : it) total_chance += x.chance;

  bool animal_done = false;
  while (rng(0, 99) < abs(type->item_chance) && !animal_done) {
   int cur_chance = rng(1, total_chance);
   int selected_location = -1;
   while (cur_chance > 0) {
    selected_location++;
    cur_chance -= it[selected_location].chance;
   }
   int total_it_chance = 0;
   const auto& mapit = map::items[it[selected_location].loc];
   assert(!mapit.empty());
   for (auto x : mapit) total_it_chance += item::types[x]->rarity;
   cur_chance = rng(1, total_it_chance);
   int selected_item = -1;
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
  auto om_pos = overmap::toOvermapHires(GPSpos);
  leaderless_hive(type->id, overmap::monsters_at(om_pos));
  // Do it for overmap above/below too
  om_pos.first.z = (0 == om_pos.first.z) ? -1 : 0;
  leaderless_hive(type->id, overmap::monsters_at(om_pos));
 }
// If we're a mission monster, update the mission
 if (mission_id != -1) {
  if (const auto miss = mission::from_id(mission_id)) {
      switch (miss->type->goal) {
      case MGOAL_FIND_MONSTER: miss->fail(); break;
      case MGOAL_KILL_MONSTER: miss->step_complete(1); break;
      default: mission_id = -1;	// active reset of assumed bug
      }
  } else mission_id = -1;	// active reset
 }
// Also, perform our death function
 (type->dies)(g, this);
// If our species fears seeing one of our own die, process that
 int anger_adjust = 0, morale_adjust = 0;
 if (type->anger & mfb(MTRIG_FRIEND_DIED)) anger_adjust += 15;
 if (type->placate & mfb(MTRIG_FRIEND_DIED)) anger_adjust -= 15;
 if (type->fear & mfb(MTRIG_FRIEND_DIED)) morale_adjust -= 15;
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

static constexpr monster_effect_type translate(mobile::effect src)
{
    switch (src)
    {
    case mobile::effect::DOWNED: return ME_DOWNED;
    case mobile::effect::STUNNED: return ME_STUNNED;
    case mobile::effect::DEAF: return ME_DEAF;
    case mobile::effect::BLIND: return ME_BLIND;
    case mobile::effect::POISONED: return ME_POISONED;
    default: return ME_NULL;    // \todo should be hard error
    }
}

void monster::add(effect src, int duration) { return add_effect(translate(src), duration); }
bool monster::has(effect src) const { return has_effect(translate(src)); }

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
    int i = effects.size();
    // intentionally check everything even though add effect guarantees only one effect of any type
    // as an automatic data repair
    while (0 <= --i) {
        if (effect == effects[i].type) EraseAt(effects, i);
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
   if (made_of(FLESH)) hurt(rng(3, 8));
   if (made_of(VEGGY)) hurt(rng(10, 20));
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
   EraseAt(effects, i);
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

void monster::make_friendly(int duration)
{
    if (0 != duration) plans.clear();
    friendly = duration;
}

void monster::make_friendly() { make_friendly(rng(5, 30) + rng(0, 20)); }

void monster::make_friendly(const player& u)
{
    if (is_friend(&u)) return; // already friendly

    if (is_friend(&(game::active()->u))) {
        make_friendly(0);
    } {
        make_friendly();
    }
}

void monster::make_ally(const player& u)
{
    if (is_friend(&u)) return; // already friendly

    make_friendly(is_friend(&(game::active()->u)) ? 0 : -1);
}

void monster::make_threat(const player& u)
{
    if (is_enemy(&u)) return; // already hostile

    make_friendly(is_enemy(&(game::active()->u)) ? -1 : 0);
}

bool monster::is_enemy(const player* survivor) const
{
    const bool pc_hostile = (0 == friendly);
    if (!survivor) return pc_hostile;
    if (auto _npc = dynamic_cast<const npc*>(survivor)) {
        // \todo monster-NPC relations independent of the player
        return pc_hostile == _npc->is_enemy();
    }
    return pc_hostile;
}

bool monster::is_friend(const player* survivor) const
{
    const bool pc_friendly = (0 != friendly);
    if (!survivor) return pc_friendly;
    if (auto _npc = dynamic_cast<const npc*>(survivor)) {
        // \todo monster-NPC relations independent of the player
        return pc_friendly == _npc->is_friend();
    }
    return pc_friendly;
}

bool monster::is_enemy(const monster* z) const
{
    if (has_flag(MF_ATTACKMON) || z->has_flag(MF_ATTACKMON)) {
        if (type->species != z->type->species) return true; // would look weird for secubots to attack other secubots, etc.
    }
    const bool pc_hostile = (0 == friendly);
    const bool other_pc_hostile = (0 == z->friendly);
    return pc_hostile != other_pc_hostile;
}

bool monster::is_friend(const monster* z) const { return !is_enemy(z); } // not really; would like is_neutral category

void monster::add_item(const item& it)
{
 inv.push_back(it);
}

bool monster::handle_knockback_into_impassable(const GPS_loc& dest, const std::string& victim)
{
    const bool u_see = (bool)game::active()->u.see(dest);

    if (is<liquid>(dest.ter())) {
        if (!has_flag(MF_SWIMS) && !has_flag(MF_AQUATIC)) {
            hurt(9999);
            if (u_see) messages.add("%s drowns!", victim.c_str());
            return true;
        }
    } else if (has_flag(MF_AQUATIC)) { // We swim but we're NOT in water
        hurt(9999);
        if (u_see) messages.add("%s flops around and dies!", victim.c_str());
        return true;
    }
    return false;
}

bool monster::if_visible_message(std::function<std::string()> other) const
{
    if (other) {
        auto g = game::active();
        if (g->u.see(*this)) {
            auto msg = other();
            if (!msg.empty()) {
                messages.add(msg);
                return true;
            }
        }
    }
    return false;
}
