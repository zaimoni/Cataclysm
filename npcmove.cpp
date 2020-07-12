#include <sstream>
#include "npc.h"
#include "rng.h"
#include "game.h"
#include "line.h"
#include "mapbuffer.h"
#include "act_obj.h"
#include "recent_msg.h"

#define TARGET_PLAYER -2

// A list of items used for escape, in order from least to most valuable
static const itype_id ESCAPE_ITEMS[] = {	// \todo mod target
 itm_cola, itm_caffeine, itm_energy_drink, itm_canister_goo, itm_smokebomb,
 itm_smokebomb_act, itm_adderall, itm_coke, itm_meth, itm_teleporter,
 itm_pheromone
};
#define NUM_ESCAPE_ITEMS (sizeof(ESCAPE_ITEMS)/sizeof(ESCAPE_ITEMS))

// A list of alternate attack items (e.g. grenades), from least to most valuable
static const itype_id ALT_ATTACK_ITEMS[] = {	// \todo mod target
 itm_knife_combat, itm_spear_wood, itm_molotov, itm_pipebomb, itm_grenade,
 itm_gasbomb, itm_bot_manhack, itm_tazer, itm_dynamite, itm_mininuke,
 itm_molotov_lit, itm_pipebomb_act, itm_grenade_act, itm_gasbomb_act,
 itm_dynamite_act, itm_mininuke_act
};
#define NUM_ALT_ATTACK_ITEMS (sizeof(ALT_ATTACK_ITEMS)/sizeof(ALT_ATTACK_ITEMS))

// all of these classes are designed for immediate use, not scheduling (i.e., IsLegal implementation may include parts that belong in IsPerformable)
class use_escape_obj : public cataclysm::action
{
	player& _actor;
	int inv_index;
public:
	use_escape_obj(player& actor, int index) : _actor(actor), inv_index(index) {
#ifndef NDEBUG
		if (!IsLegal()) throw std::logic_error("illegal escape item");
#endif
	};
	~use_escape_obj() = default;
	bool IsLegal() const override {
		if (0 > inv_index || _actor.inv.size() <= inv_index) return false;
		const auto& used = _actor.inv[inv_index];
		return used.is_food() || used.is_food_container() || used.is_tool();
	}
	void Perform() const override {
		// C:Whales npc::use_escape_item inlined

		/* There is a static list of items that NPCs consider to be "escape items," so
		 * we can just use a switch here to decide what to do based on type.  See
		 * ESCAPE_ITEMS, defined in npc.h
		 */

		auto& used = _actor.inv[inv_index];

		if (used.is_food() || used.is_food_container()) {
			_actor.eat(inv_index);
			return;
		}
#ifndef NDEBUG
		if (!used.is_tool()) throw std::logic_error("escape item was neither edible nor a tool");
#endif

		const it_tool* const tool = dynamic_cast<const it_tool*>(used.type);
		(*tool->use)(game::active(), &_actor, &used, false);
		used.charges -= tool->charges_per_use;
		if (0 == used.invlet) _actor.inv.remove_item(inv_index);
	}
	const char* name() const override {
		return "Use escape item";
	}
};

class move_step_screen : public cataclysm::action
{
	npc& _actor;	// \todo would like player here
	point _dest;
	const char* _desc;
public:
	move_step_screen(npc& actor, const point& dest, const char* desc) : _actor(actor), _dest(dest), _desc(desc) {
#ifndef NDEBUG
		if (!IsLegal()) throw std::logic_error("unreasonable move");
#endif
	};
	~move_step_screen() = default;
	bool IsLegal() const override {
		return _actor.can_move_to(game::active()->m, _dest);	// stricter than what npc::move_to actually supports
	}
	void Perform() const override {
		_actor.move_to(game::active(), _dest);
	}
	const char* name() const override {
		if (_desc && *_desc) return _desc;
		return "Moving (screen-relative)";	// failover
	}
};

class move_path_screen : public cataclysm::action
{
	npc& _actor;	// \todo would like player here
	size_t _threshold;
	void (npc::* _op)();
	const char* _desc;
public:
	move_path_screen(npc& actor, const point& dest, size_t threshold, void (npc::* op)(), const char* desc) : _actor(actor), _threshold(threshold),_op(op),_desc(desc) {
		_actor.update_path(game::active()->m, dest);
		if (_actor.path.empty()) throw std::runtime_error("tried to follow empty path");
		if (_actor.path.size() <= _threshold && !_op) throw std::runtime_error("NULL path action");
#ifndef NDEBUG
		if (!IsLegal()) throw std::logic_error("unreasonable pathing");
#endif
	};
	~move_path_screen() = default;
	bool IsLegal() const override {
		return _actor.path.size() <= _threshold || _actor.can_move_to(game::active()->m, _actor.path[0]);	// stricter than what npc::move_to actually supports
	}
	void Perform() const override {
		if (_actor.path.size() <= _threshold) (_actor.*_op)();
		else _actor.move_to(game::active(), _actor.path[0]);
	}
	const char* name() const override {
		if (_desc && *_desc) return _desc;
		return "Following path";	// failover
	}
};

class fire_weapon_screen : public cataclysm::action
{
	npc& _actor;	// \todo would like player here
	point _tar;
	mutable std::vector<point> _trajectory;
	bool _burst;
	const char* _desc;
public:
	fire_weapon_screen(npc& actor, const point& tar, bool burst, const char* desc=0) : _actor(actor), _tar(tar), _burst(burst), _desc(desc) {
		auto g = game::active();
		if (tar != actor.pos) {	// required for legality/performability
			int linet;
			const int light = g->light_level();
			_trajectory = line_to(actor.pos, tar, (g->m.sees(actor.pos, tar, actor.sight_range(light), linet) ? linet : 0));
		}
#ifndef NDEBUG
		if (!IsLegal()) throw std::logic_error("unreasonable targeting");
#endif
	};
	~fire_weapon_screen() = default;
	bool IsLegal() const override {
		return _actor.weapon.is_gun() && _tar != _actor.pos;
	}
	void Perform() const override {
		game::active()->fire(_actor, _tar, _trajectory, _burst);
	}
	const char* name() const override {
		if (_desc && *_desc) return _desc;
		return (_burst ? "Fire a burst" : "Shoot");	// failover
	}
};

class target_player : public cataclysm::action
{
	npc& _actor;
	player& _target;
	void (npc::* _op)(player& pl);
	const char* _desc;
public:
	target_player(npc& actor, player& target, void (npc::* op)(player& pl), const char* desc) : _actor(actor), _target(target), _op(op), _desc(desc) {
#ifndef NDEBUG
		if (!IsLegal()) throw std::logic_error("illegal targeting of player");
#endif
	}
	~target_player() = default;
	bool IsLegal() const override {
		return !_actor.path.empty() && _actor.path.back() == _target.pos;	// e.g., result of npc::update_path.
	}
	void Perform() const override {
		(_actor.*_op)(_target);
	}
	const char* name() const override {
		if (_desc && *_desc) return _desc;
		return "targeting player";	// failover
	}
};

template<class PC/*=npc */>	// default class works for MSVC++, not MingW64
class target_inventory : public cataclysm::action
{
	PC& _actor;
	int _inv_index;
	bool (PC::* _op)(int inv_target);
	const char* _desc;
public:
	target_inventory(PC& actor, int inv_index, bool (PC::* op)(int inv_target), const char* desc) : _actor(actor), _inv_index(inv_index), _op(op), _desc(desc) {
#ifndef NDEBUG
		if (!IsLegal()) throw std::logic_error("illegal targeting of inventory");
#endif
	}
	~target_inventory() = default;
	bool IsLegal() const override {
		if (0 <= _inv_index) return _actor.inv.size() > _inv_index;
		else if (-1 == _inv_index) return true;	// not always correct: requesting weapon
		else return _actor.styles.size() >= -_inv_index;	// not always correct, but sometimes is
	}
	void Perform() const override {
		// \todo need to checkpoint before/after for actor state; if "no change" and _op fails then critical bug (infinite loop)
		// C:Whales handled this for eating by forcing moves=0 afterwards, but this resulted in NPCs eating faster than PCs
		(_actor.*_op)(_inv_index);
	}
	const char* name() const override {
		if (_desc && *_desc) return _desc;
		return "targeting inventory";	// failover
	}
};

std::string npc_action_name(npc_action action);

// Used in npc::drop_items()
struct ratio_index
{
 double ratio;
 int index;
 ratio_index(double R, int I) : ratio (R), index (I) {};
};

// class npc functions!

void npc::move(game *g)
{
 wand.tick();	// countdown timers
 pl.tick();

 ai_action action(npc_undecided,std::unique_ptr<cataclysm::action>());
 int danger = 0, total_danger = 0, target = -1;

 choose_monster_target(g, target, danger, total_danger);
 if (game::debugmon)
  debugmsg("NPC %s: target = %d, danger = %d, range = %d",
           name.c_str(), target, danger, confident_range());

 if (is_enemy()) {
  int pl_danger = player_danger( &(g->u) );
  if (pl_danger > danger || target == -1) {
   target = TARGET_PLAYER;
   danger = pl_danger;
   if (game::debugmon) debugmsg("NPC %s: Set target to PLAYER, danger = %d", name.c_str(), danger);
  }
 }
#ifndef NDEBUG
 if (0 < danger && TARGET_PLAYER != target && (0 > target || g->z.size() <= target)) throw std::logic_error("danger without a target");
#endif
// TODO: Place player-aiding actions here, with a weight

 // Bravery check appears to have been disabled due to excessive randomness.
 //if (!bravery_check(danger) || !bravery_check(total_danger) || 
 if (target == TARGET_PLAYER && attitude == NPCATT_FLEE)
  action = method_of_fleeing(g, target);
 else if (danger > 0 || (target == TARGET_PLAYER && attitude == NPCATT_KILL))
  action = method_of_attack(g, target, danger);

 if (!action.second && npc_undecided == action.first) {	// No present danger
  action = address_needs(g, danger);
  if (game::debugmon) debugmsg("address_needs %s", action.second ? action.second->name() : npc_action_name(action.first).c_str());
  if (action.first == npc_undecided) action = address_player(g);
  if (game::debugmon) debugmsg("address_player %s", action.second ? action.second->name() : npc_action_name(action.first).c_str());
  if (action.first == npc_undecided) {
   if (mission == NPC_MISSION_SHELTER || has_disease(DI_INFECTION))
    action = ai_action(npc_pause, std::unique_ptr<cataclysm::action>());
   else if (has_new_items)
    action = scan_new_items(g, target);
   else if (!fetching_item)
    find_item(g);
   if (game::debugmon) debugmsg("find_item %s", action.second ? action.second->name() : npc_action_name(action.first).c_str());
   if (fetching_item)		// Set to true if find_item() found something
     action = ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new move_path_screen(*this, it, 1, &npc::pick_up_item, "Pick up items")));
   else if (is_following() && update_path(g->m, g->u.pos, follow_distance())) {	// No items, so follow the player?
    action = ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new move_step_screen(const_cast<npc&>(*this), path.front(), "Follow player")));
   } else
    action = long_term_goal_action(g); // Do our long-term action
   if (game::debugmon) debugmsg("long_term_goal_action %s", action.second ? action.second->name() : npc_action_name(action.first).c_str());
  }
 }

#ifndef NDEBUG
 if (!action.first && !action.second) throw std::logic_error("npc::execute_action will fail");
#endif

 if (game::debugmon) debugmsg("%s chose action %s.", name.c_str(), action.second ? action.second->name() : npc_action_name(action.first).c_str());

 execute_action(g, action, target);
}

static bool decode_target(int target, npc::ai_target& dest)
{
	auto g = game::active();
	if (0 <= target && g->z.size() > target) {
		auto& mon = g->z[target];
		dest = npc::ai_target(mon.pos, std::tuple<monster*, player*, npc*>(&mon, 0, 0));
		return true;
	} else if (TARGET_PLAYER == target) {
		auto& u = g->u;
		dest = npc::ai_target(u.pos, std::tuple<monster*, player*, npc*>(0, &u, 0));
		return true;
	} else if (TARGET_PLAYER > target && g->active_npc.size()>((TARGET_PLAYER-1) - target)) {	// 2019-11-21: UNTESTED
		auto npc_index = (TARGET_PLAYER - 1) - target;
		if (g->active_npc.size() > npc_index) {
			auto& nPC = g->active_npc[npc_index];
			dest = npc::ai_target(nPC.pos, std::tuple<monster*, player*, npc*>(0, 0, &nPC));
			return true;
		}
	}
	return false;
}

void npc::execute_action(game *g, const ai_action& action, int target)
{
 if (action.second && action.second->IsPerformable()) {	// if we have an action object, use that
	 action.second->Perform();
	 return;
 }
 
 // C:Whales action processing
 npc::ai_target Target;
 const int oldmoves = moves;
 const int light = g->light_level();

 switch (action.first) {

 case npc_pause:
  move_pause();
  break;

#if DEAD_FUNC
 case npc_sleep:
/* TODO: Open a dialogue with the player, allowing us to ask if it's alright if
 * we get some sleep, how long watch shifts should be, etc.
 */
  //add_disease(DI_LYING_DOWN, 300, g);
  if (is_friend() && g->u_see(pos))
   say(g, "I'm going to sleep.");
  break;
#endif

 case npc_heal:
  heal_self(g);
  break;

#if DEAD_FUNC
 case npc_drop_items:
  drop_items(g, weight_carried() - weight_capacity() / 4,
                volume_carried() - volume_capacity());
  break;
#endif

 case npc_melee:
  if (decode_target(target, Target)) {
	  if (auto mon = std::get<0>(Target.second)) melee_monster(g, *mon);
	  else if (auto pc = std::get<1>(Target.second)) melee_player(g, *pc);
	  else if (auto nPC = std::get<2>(Target.second)) melee_player(g, *nPC);
	  // invariant violation if no case processes
	  break;
  }
#ifndef NDEBUG
  throw std::logic_error("npc_melee invoked w/o target");
#endif
  
 case npc_alt_attack:
  alt_attack(g, target);
  break;

#if DEAD_FUNC
 case npc_heal_player:
  update_path(g->m, g->u.pos);
  if (path.size() == 1)	// We're adjacent to u, and thus can heal u
   heal_player(g, g->u);
  else if (path.size() > 0)
   move_to_next(g);
  else
   move_pause();
  break;
#endif

 case npc_talk_to_player:
  talk_to_u(g);
  moves = 0;
  break;

 case npc_goto_destination:
  go_to_destination(g);
  break;

 case npc_avoid_friendly_fire:
  avoid_friendly_fire(g, target);
  break;

 default:
  debugmsg("Unknown NPC action (%d)", action.first);
 }

 if (oldmoves == moves) {
  debugmsg("NPC didn't use its moves.  Action %d.  Turning on debug mode.", action.first);
  game::debugmon = true;
 }
}

void npc::choose_monster_target(game *g, int &enemy, int &danger,
                                int &total_danger)
{
 const bool defend_u = g->sees_u(pos) && is_defending();
 int highest_priority = 0;
 total_danger = 0;

 for (int i = 0; i < g->z.size(); i++) {
  monster *mon = &(g->z[i]);
  if (g->pl_sees(this, mon)) {
   int distance = (100 * rl_dist(pos, mon->pos)) / mon->speed;
   double hp_percent = (mon->type->hp - mon->hp) / mon->type->hp;
   int priority = mon->type->difficulty * (1 + hp_percent) - distance;
   int monster_danger = (mon->type->difficulty * mon->hp) / mon->type->hp;
   if (!mon->is_fleeing(*this)) monster_danger++;

   if (mon->friendly != 0) {
    priority = -999;
    monster_danger *= -1;
   }/* else if (mon->speed < current_speed(g)) {
    priority -= 10;
    monster_danger -= 10;
   } else
    priority *= 1 + (.1 * distance);
*/
   total_danger += int(monster_danger / (distance == 0 ? 1 : distance));

   bool okay_by_rules = true;
   if (is_following()) {
    switch (combat_rules.engagement) {
     case ENGAGE_NONE:
      okay_by_rules = false;
      break;
     case ENGAGE_CLOSE:
      okay_by_rules = (distance <= 6);
      break;
     case ENGAGE_WEAK:
      okay_by_rules = (mon->hp <= average_damage_dealt());
      break;
     case ENGAGE_HIT:
      okay_by_rules = (mon->has_effect(ME_HIT_BY_PLAYER));
      break;
    }
   }

   if (okay_by_rules && monster_danger > danger) {
    danger = monster_danger;
    if (enemy == -1) {
     highest_priority = priority;
     enemy = i;
    }
   }

   if (okay_by_rules && priority > highest_priority) {
    highest_priority = priority;
    enemy = i;
   } else if (okay_by_rules && defend_u) {
    priority = mon->type->difficulty * (1 + hp_percent);
    distance = (100 * rl_dist(g->u.pos, mon->pos)) / mon->speed;
    priority -= distance;
    if (mon->speed < current_speed(g)) priority -= 10;
    priority *= (personality.bravery + personality.altruism + op_of_u.value) / 15;
    if (priority > highest_priority) {
     highest_priority = priority;
     enemy = i;
    }
   }
  }
 }
}

static npc::ai_action _flee(const npc& actor, const point& fear)
{
	point escape;
	if (actor.move_away_from(game::active()->m, fear, escape)) {
		return npc::ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new move_step_screen(const_cast<npc&>(actor), escape, "Fleeing")));	// C:Whales failure mode was npc_pause
	}
	return npc::ai_action(npc_undecided, std::unique_ptr<cataclysm::action>());
}

npc::ai_action npc::method_of_fleeing(game *g, int enemy) const
{
 int it = choose_escape_item();
 if (0 <= it) // We have an escape item!
  return ai_action(npc_pause,std::unique_ptr<cataclysm::action>(new use_escape_obj(*const_cast<npc*>(this), it)));	// C:Whales failure mode was npc_pause

 int speed = (enemy == TARGET_PLAYER ? g->u.current_speed(g) : g->z[enemy].speed);
 point enemy_loc = (enemy == TARGET_PLAYER ? g->u.pos : g->z[enemy].pos);
 int distance = rl_dist(pos, enemy_loc);

 if (speed > 0 && (100 * distance) / speed <= 4 && speed > current_speed(g))
  return method_of_attack(g, enemy, -1); // Can't outrun, so attack

 return _flee(*this, enemy_loc);
}

bool npc::reload(int inv_index)
{
	moves -= weapon.reload_time(*this);
	if (!weapon.reload(*this, inv_index)) {
		debugmsg("NPC reload failed.");
		return false;
	}
	recoil = 6;
	if (game::active()->u_see(pos)) messages.add("%s reloads %s %s.", name.c_str(), (male ? "his" : "her"), weapon.tname().c_str());
	return true;
}

bool npc::choose_empty_gun(const std::vector<int>& empty_guns, int& inv_index) const
{
	inv_index = -1;
	bool ammo_found = false;	// alias for 0 <= inv_index
	for (auto gun : empty_guns) {
		const item& it = inv[gun];
#ifndef NDEBUG
		if (!it.is_gun()) throw std::logic_error("empty_guns must contain only ranged weapons");
#endif
		bool am = has_ammo((dynamic_cast<const it_gun*>(it.type))->ammo);
		if (!ammo_found || am) {
			inv_index = gun;
			ammo_found = (ammo_found || am);
		}
	}
	return ammo_found;
}

static npc::ai_action _melee(const npc& actor, const point& tar)
{
	if (const_cast<npc&>(actor).update_path(game::active()->m, tar, 0)) {	// \todo want to think of npc::path as mutable here
		if (1 < actor.path.size()) return npc::ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new move_step_screen(const_cast<npc&>(actor), actor.path.front(), "Melee")));
		return npc::ai_action(npc_melee, std::unique_ptr<cataclysm::action>());
	}
	// \todo maybe follow player if appropriate
	return const_cast<npc&>(actor).look_for_player(game::active()->u);
}

npc::ai_action npc::method_of_attack(game *g, int target, int danger) const
{
 point tar((target == TARGET_PLAYER) ? g->u.pos : g->z[target].pos);

 bool can_use_gun = (!is_following() || combat_rules.use_guns),
	 can_use_grenades = (!is_following() || combat_rules.use_grenades);

 const int dist = rl_dist(pos, tar);
 const int target_HP = (target == TARGET_PLAYER) ? g->u.hp_percentage() * g->u.hp_max[hp_torso] : g->z[target].hp;

 if (can_use_gun) {
  if (need_to_reload()) {
	  const auto inv_index = can_reload();
	  if (0 <= inv_index) return ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new target_inventory<npc>(*const_cast<npc*>(this), inv_index, &npc::reload, "Reload")));
  }
  if (emergency(danger_assessment(g)) && alt_attack_available()) return ai_action(npc_alt_attack, std::unique_ptr<cataclysm::action>());
  if (weapon.is_gun() && weapon.charges > 0) {
   if (dist > confident_range()) {
	const auto inv_index = can_reload();
    if (0 <= inv_index && enough_time_to_reload(g, target, weapon))
     return ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new target_inventory<npc>(*const_cast<npc*>(this), inv_index, &npc::reload, "Reload")));
    else
	 return _melee(*this, tar);
   }
   const it_gun* const gun = dynamic_cast<const it_gun*>(weapon.type);
   if (!wont_hit_friend(g, tar)) return ai_action(npc_avoid_friendly_fire, std::unique_ptr<cataclysm::action>());
   const bool want_burst = (dist <= confident_range() / 3 && weapon.charges >= gun->burst && gun->burst > 1 &&
	   (target_HP >= weapon.curammo->damage * 3 || emergency(danger * 2)));
   return ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new fire_weapon_screen(*const_cast<npc*>(this), tar, want_burst)));
  }
 }

// Check if there's something better to wield
 bool has_better_melee = false;
 std::vector<int> empty_guns;
 if (can_use_gun) {
	 int gun;
	 if (best_gun(target, gun, empty_guns, has_better_melee)) {
		 return ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new target_inventory<npc>(*const_cast<npc*>(this), gun, &npc::wield, "Wield loaded gun")));
	 }
 } else {
	 has_better_melee = can_wield_better_melee();
 }

 const bool has_empty_gun = !empty_guns.empty();
 int wield_this;

 if (has_empty_gun && choose_empty_gun(empty_guns, wield_this))
	 return ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new target_inventory<npc>(*const_cast<npc*>(this), wield_this, &npc::wield, "Wield empty gun")));
 else if (has_better_melee) {
	 if (best_melee_weapon(wield_this)) return ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new target_inventory<npc>(*const_cast<npc*>(this), wield_this, &npc::wield, "Wield melee weapon")));
 }

 return _melee(*this, tar);
}

npc::ai_action npc::address_needs(game *g, int danger) const
{
 if (has_healing_item()) {
  for (int i = 0; i < num_hp_parts; i++) {
   hp_part part = hp_part(i);
   if ((part == hp_head  && hp_cur[i] <= 35) ||
       (part == hp_torso && hp_cur[i] <= 25) ||
       hp_cur[i] <= 15)
    return ai_action(npc_heal, std::unique_ptr<cataclysm::action>());
  }
 }

 int inv_index;
 if (!took_painkiller() && pain - pkill >= 15) {
   inv_index = pick_best_painkiller(inv);
   if (0 <= inv_index) ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new target_inventory<player>(*const_cast<npc*>(this), inv_index, &player::eat, "Use painkillers")));
 }

 inv_index = can_reload();
 if (0 <= inv_index) return ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new target_inventory<npc>(*const_cast<npc*>(this), inv_index, &npc::reload, "Reload")));

 if (   (danger <= NPC_DANGER_VERY_LOW && (hunger > 40 || thirst > 40))
	 ||  thirst > 80
	 || hunger > 160) {
	inv_index = pick_best_food(inv);
	if (0 <= inv_index)
	  return ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new target_inventory<player>(*const_cast<npc*>(this), inv_index, &player::eat, "Eat")));
  }

#if DEAD_FUNC
 if (weight_carried() > weight_capacity() / 4 ||
     volume_carried() > volume_capacity())
  return npc_drop_items;
#endif

/*
 if (danger <= 0 && fatigue > 191)
  return npc_sleep;
*/

// TODO: Mutation & trait related needs
// e.g. finding glasses; getting out of sunlight if we're an albino; etc.

 return ai_action(npc_undecided, std::unique_ptr<cataclysm::action>());
}

npc::ai_action npc::address_player(game *g)
{
 if ((attitude == NPCATT_TALK || attitude == NPCATT_TRADE) && g->sees_u(pos)) {
  if (update_path(g->m, g->u.pos, 6)) {
   if (one_in(10)) say(g, "<lets_talk>");
   return ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new move_step_screen(const_cast<npc&>(*this), path.front(), "Follow player")));
  } else
   return ai_action(npc_talk_to_player, std::unique_ptr<cataclysm::action>()); // Close enough to talk to you
 }

 if (attitude == NPCATT_MUG && g->sees_u(pos)) {
  if (update_path(g->m, g->u.pos, 0)) {
	if (one_in(3)) say(g, "Don't move a <swear> muscle...");
	return ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new target_player(*this,g->u,&npc::mug_player, "Mug player")));
  }
 }

 if (attitude == NPCATT_WAIT_FOR_LEAVE) {
  patience--;
  if (patience <= 0) {
   patience = 0;
   attitude = NPCATT_KILL;
   return method_of_attack(g, TARGET_PLAYER, player_danger( &(g->u) ));
  }
  return ai_action(npc_undecided, std::unique_ptr<cataclysm::action>());
 }
 
 if (attitude == NPCATT_FLEE) return _flee(*this, g->u.pos);

 if (attitude == NPCATT_LEAD) {
  if (rl_dist(pos, g->u.pos) >= 12 || !g->sees_u(pos)) {
   int intense = disease_intensity(DI_CATCH_UP);
   if (intense < 10) {
    say(g, "<keep_up>");
    add_disease(DI_CATCH_UP, 5, 1, 15);
    return ai_action(npc_pause, std::unique_ptr<cataclysm::action>());
   } else if (intense == 10) {
    say(g, "<im_leaving_you>");
    add_disease(DI_CATCH_UP, 5, 1, 15);
    return ai_action(npc_pause, std::unique_ptr<cataclysm::action>());
   } else
    return ai_action(npc_goto_destination, std::unique_ptr<cataclysm::action>());
  } else
   return ai_action(npc_goto_destination, std::unique_ptr<cataclysm::action>());
 }
 return ai_action(npc_undecided, std::unique_ptr<cataclysm::action>());
}

npc::ai_action npc::long_term_goal_action(game *g)	// XXX this was being prototyped
{
 if (game::debugmon) debugmsg("long_term_goal_action()");
 path.clear();

 if (mission == NPC_MISSION_SHOPKEEP || mission == NPC_MISSION_SHELTER)
  return ai_action(npc_pause, std::unique_ptr<cataclysm::action>());	// Shopkeeps just stay put.

// TODO: Follow / look for player
 if (is_following()) return ai_action(npc_pause, std::unique_ptr<cataclysm::action>());	// historical C:Whales for already-in-range

 if (!has_destination()) set_destination(g);
 if (has_destination()) return ai_action(npc_goto_destination, std::unique_ptr<cataclysm::action>());

 return ai_action(npc_undecided, std::unique_ptr<cataclysm::action>());
}
 
itype_id npc::alt_attack_available() const
{
 for (int i = 0; i < NUM_ALT_ATTACK_ITEMS; i++) {
  if ((!is_following() || combat_rules.use_grenades ||
       !(item::types[ALT_ATTACK_ITEMS[i]]->item_flags & mfb(IF_GRENADE))) &&
      has_amount(ALT_ATTACK_ITEMS[i], 1))
   return ALT_ATTACK_ITEMS[i];
 }
 return itm_null;
}

int npc::choose_escape_item() const
{
 int best = -1, ret = -1;
 for (size_t i = 0; i < inv.size(); i++) {
  const it_comest* food = 0;
  const auto& it = inv[i];
  for (int j = 0; j < NUM_ESCAPE_ITEMS; j++) {
   if (it.type->id != ESCAPE_ITEMS[j]) continue;	// \todo some sort of relevance check (needs context)
   if (j < best) continue;
   if (j == best && it.charges >= inv[ret].charges) continue;
   if (!food && it.is_food()) food = dynamic_cast<const it_comest*>(inv[i].type);
   if ((!food || stim < food->stim ||            // Avoid guzzling down
        (food->stim >= 10 && stim < food->stim * 2))) { //  Adderall etc.
    ret = i;
    best = j;
	break;
   }
  }
 }
 return ret;
}

// Index defaults to -1, i.e., wielded weapon
int npc::confident_range(int index) const
{
 if (index == -1 && (!weapon.is_gun() || weapon.charges <= 0)) return 1;

 double deviation = 0;
 int max = 0;
 if (index == -1) {
  const it_gun* const firing = dynamic_cast<const it_gun*>(weapon.type);
// We want at least 50% confidence that missed_by will be < .5.
// missed_by = .00325 * deviation * range <= .5; deviation * range <= 156
// (range <= 156 / deviation) is okay, so confident range is (156 / deviation)
// Here we're using median values for deviation, for a around-50% estimate.
// See game::fire (ranged.cpp) for where these computations come from

  if (sklevel[firing->skill_used] < 5) deviation += 3.5 * (5 - sklevel[firing->skill_used]);
  else deviation -= 2.5 * (sklevel[firing->skill_used] - 5);
  if (sklevel[sk_gun] < 3) deviation += 1.5 * (3 - sklevel[sk_gun]);
  else deviation -= .5 * (sklevel[sk_gun] - 3);

  if (per_cur < 8) deviation += 2 * (9 - per_cur);
  else deviation -= (per_cur > 16 ? 8 : per_cur - 8);
  if (dex_cur < 6) deviation += 4 * (6 - dex_cur);
  else if (dex_cur < 8) deviation += 8 - dex_cur;
  else if (dex_cur > 8) deviation -= .5 * (dex_cur - 8);

  deviation += .5 * encumb(bp_torso) + 2 * encumb(bp_eyes);

  if (weapon.curammo == NULL)	// This shouldn't happen, but it does sometimes
   debugmsg("%s has NULL curammo!", name.c_str()); // TODO: investigate this bug
  else {
   deviation += .5 * weapon.curammo->accuracy;
   max = weapon.range();
  }
  deviation += .5 * firing->accuracy;
  deviation += 3 * recoil;

 } else { // We aren't firing a gun, we're throwing something!

  const item& thrown = inv[index];
  max = throw_range(index); // The max distance we can throw
  int deviation = 0;
  if (sklevel[sk_throw] < 8) deviation += rng(0, 8 - sklevel[sk_throw]);
  else deviation -= sklevel[sk_throw] - 6;

  deviation += throw_dex_mod();

  if (per_cur < 6) deviation += rng(0, 8 - per_cur);
  else if (per_cur > 8) deviation -= per_cur - 8;

  deviation += rng(0, encumb(bp_hands) * 2 + encumb(bp_eyes) + 1);
  if (thrown.volume() > 5) deviation += rng(0, 1 + (thrown.volume() - 5) / 4);
  if (thrown.volume() == 0) deviation += rng(0, 3);

  deviation += rng(0, 1 + abs(str_cur - thrown.weight()));
 }

// Using 180 for now for extra-confident NPCs.
 int ret = (max > int(180 / deviation) ? max : int(180 / deviation));
 if (ret > weapon.curammo->range) return weapon.curammo->range;
 return ret;
}

// Index defaults to -1, i.e., wielded weapon
bool npc::wont_hit_friend(game *g, int tarx, int tary, int index) const
{
 if (rl_dist(pos, tarx, tary) == 1) return true; // If we're *really* sure that our aim is dead-on

 int linet = 0, dist = sight_range(g->light_level());
 int confident = confident_range(index);

 std::vector<point> traj = line_to(pos, tarx, tary, (g->m.sees(pos, tarx, tary, dist, linet) ? linet : 0));

 for (int i = 0; i < traj.size(); i++) {
  int dist = rl_dist(pos, traj[i]);
  int deviation = 1 + int(dist / confident);
  for (int x = traj[i].x - deviation; x <= traj[i].x + deviation; x++) {
   for (int y = traj[i].y - deviation; y <= traj[i].y + deviation; y++) {
// Hit the player?
    if (is_friend() && g->u.pos.x == x && g->u.pos.y == y)
     return false;
// Hit a friendly monster?
/*
    for (int n = 0; n < g->z.size(); n++) {
     if (g->z[n].friendly != 0 && g->z[n].posx == x && g->z[n].posy == y)
      return false;
    }
*/
// Hit an NPC that's on our team?
/*
    for (int n = 0; n < g->active_npc.size(); n++) {
     npc* guy = &(g->active_npc[n]);
     if (guy != this && (is_friend() == guy->is_friend()) &&
         guy->posx == x && guy->posy == y)
      return false;
    }
*/
   }
  }
 }
 return true;
}

int player::can_reload() const
{
	if (weapon.is_gun()) {
		if (weapon.has_flag(IF_RELOAD_AND_SHOOT)) {
			messages.add("Your %s does not need to be reloaded; it reloads and fires in a single action.", weapon.tname().c_str());
			return -1;
		}
		if (weapon.ammo_type() == AT_NULL) {
			messages.add("Your %s does not reload normally.", weapon.tname().c_str());
			return -1;
		}
		if (weapon.charges == weapon.clip_size()) {
			messages.add("Your %s is fully loaded!", weapon.tname().c_str());
			return -1;
		}
		int index = weapon.pick_reload_ammo(*this, true);
		if (0 > index) {
			messages.add("Out of ammo!");
			return -1;
		}
		return index;
	} else if (weapon.is_tool()) {
		const it_tool* const tool = dynamic_cast<const it_tool*>(weapon.type);
		if (tool->ammo == AT_NULL) {
			messages.add("You can't reload a %s!", weapon.tname().c_str());
			return -1;
		}
		const int index = weapon.pick_reload_ammo(*this, true);
		if (0 > index) {
			// Reload failed
			messages.add("Out of %s!", ammo_name(tool->ammo).c_str());
			return -1;
		}
		return index;
	} else if (!is_armed()) {
		messages.add("You're not wielding anything.");
		return -1;
	}
	messages.add("You can't reload a %s!", weapon.tname().c_str());
	return -1;
}

int npc::can_reload() const
{
   if (!weapon.is_gun()) return -1;	// \todo extend this to tools

// if (weapon.has_flag(IF_RELOAD_AND_SHOOT)) return -1;	// ignored by NPCs
   if (weapon.ammo_type() == AT_NULL) return -1;
   if (weapon.charges == weapon.clip_size()) return -1;
   return weapon.pick_reload_ammo(*this, false);
}

bool npc::need_to_reload() const
{
 if (!weapon.is_gun()) return false;

 return (weapon.charges < dynamic_cast<const it_gun*>(weapon.type)->clip * .1);
}

bool npc::enough_time_to_reload(game *g, int target, const item &gun) const
{
 int rltime = gun.reload_time(*this);
 double turns_til_reloaded = rltime / current_speed(g);
 int dist, speed;

 if (target == TARGET_PLAYER) {
  if (g->sees_u(pos) && g->u.weapon.is_gun() && rltime > 200)
   return false; // Don't take longer than 2 turns if player has a gun
  dist = rl_dist(pos, g->u.pos);
  speed = speed_estimate(g->u.current_speed(g));
 } else if (target >= 0) {
  dist = rl_dist(pos, g->z[target].pos);
  speed = speed_estimate(g->z[target].speed);
 } else
  return true; // No target, plenty of time to reload

 double turns_til_reached = (dist * 100) / speed;

 return (turns_til_reloaded < turns_til_reached);
}

template<class T, class COORD>
static void _normalize_path(T& path, const COORD& origin)
{
	while(!path.empty() && origin == path.front()) EraseAt(path, 0);
	// while we could cut corners, we don't yet have a "required waypoint" designation (i.e.
	// a non-optimal path may be achieving an ai-required side effect)
	// \todo? re-routing around non-hostiles, and other issues in npc::move_to
}

bool npc::path_is_usable(const map& m)
{
	_normalize_path(path, pos);
	if (path.empty()) return false;
	const auto& pt = path.front();
	// \todo cf npc::move_to for other issues (re-routing around non-hostiles)
	return 1 == rl_dist(pos, pt) || m.sees(pos, pt, -1);
	// path repair and optimization are for a different function: need a non-destructive version for multi-target search
}

template<class T, class COORD>
static bool _path_goes_there(const T& path, const COORD& dest)
{
	if (path.empty()) return false;
	return dest == path.back();
}

template<class T, class ITER>
static bool _path_goes_there(const T& path, ITER start, ITER end)
{
	if (path.empty()) return false;
	auto& last = path.back();
	while (start != end) {
		if (last == *(start++)) return true;
	}
	return false;
}

bool npc::update_path(const map& m, const point& pt, const size_t longer_than)
{
	if (path_is_usable(m) && _path_goes_there(path,pt)) return longer_than < path.size();	// usable, already leads to destination: no need to recalculate
	auto new_path = m.route(pos, pt);
	_normalize_path(new_path, pos);
	if (longer_than >= new_path.size()) return false;
	path = std::move(new_path);
	return true;
}


void npc::update_path(const map& m, const point& pt)
{
 if (path_is_usable(m) && path.back() == pt) return;	// usable, already leads to destination: no need to recalculate
 path = m.route(pos, pt);
 _normalize_path(path, pos);
}

bool player::landing_zone_ok() // \todo more complex approach; favor "near entry point", etc.
{
    if (can_enter(GPSpos)) return true;
    size_t lz_ub = 0;
    point lz[SEEX * SEEY];
    point pt;
    for (pt.x = 0; pt.x < SEEX; ++pt.x) {
        for (pt.y = 0; pt.y < SEEY; ++pt.y) {
            if (can_enter(GPS_loc(GPSpos.first, pt))) lz[lz_ub++] = pt;
        }
    }
    const bool ret = (0 < lz_ub);
    if (ret) GPSpos.second = lz[rng(0, lz_ub - 1)];
    return ret;
}

bool player::can_enter(const GPS_loc& _GPSpos) const // should act like player::can_move_to
{
    if (const auto sm = MAPBUFFER.lookup_submap(_GPSpos.first)) {
        auto g = game::active();
        point pt;
        if (g->toScreen(_GPSpos, pt)) {
            // destination is actually in reality bubble: we are contemplating spawning
            if (g->mon(pt) || g->nPC(pt)) return false; // so do not spawn on monsters or NPCs
            const auto& m = g->m;
            return 0 < m.move_cost(pt) /* || m.has_flag(bashable, pt) */;   // \todo would be interesting if we could bash from just outside the reality bubble?
        } else {    // formally moving outside of the reality bubble
    //      return 0 < sm->move_cost(_GPSpos.second) || sm->has_flag(bashable, _GPSpos.second); // \todo includes vehicles
            return 0 < sm->move_cost_ter_only(_GPSpos.second) || sm->has_flag_ter_only<bashable>(_GPSpos.second);
        }
    }
    return true;    // if submap not yet created, optimistically assume everything is ok
}

bool player::can_move_to(const map& m, const point& pt) const
{
 return (m.move_cost(pt) > 0 || m.has_flag(bashable, pt)) && rl_dist(pos, pt) <= 1;
}

void npc::move_to(game *g, point pt)
{
 if (has_disease(DI_DOWNED)) {
  moves -= 100;
  return;
 }
 if (recoil > 0) {	// Start by dropping recoil a little
  if (int(str_cur / 2) + sklevel[sk_gun] >= recoil)
   recoil = 0;
  else {
   recoil -= int(str_cur / 2) + sklevel[sk_gun];
   recoil = int(recoil / 2);
  }
 }
 if (has_disease(DI_STUNNED)) {
  pt.x = rng(pos.x - 1, pos.x + 1);
  pt.y = rng(pos.y - 1, pos.y + 1);
 }
 // ok to try to path to something in sight
 if (rl_dist(pos, pt) > 1) {
/*
  debugmsg("Tried to move_to more than one space! (%d, %d) to (%d, %d)",
           posx, posy, x, y);
  debugmsg("Route is size %d.", path.size());
*/
  int linet;
  if (g->m.sees(pos, pt, -1, linet)) pt = line_to(pos, pt, linet)[0];
 }
 if (pt == pos)	// We're just pausing!
  moves -= 100;
 else if (auto mon = g->mon(pt)) {	// Shouldn't happen, but it might.
  melee_monster(g, *mon);
 } else if (g->u.pos == pt) {
  say(g, "<let_me_pass>");
  moves -= 100;
 } else if (g->nPC(pt))
// \todo Determine if it's an enemy NPC (hit them), or a friendly in the way
  moves -= 100;
 else if (g->m.move_cost(pt) > 0) {
  screenpos_set(pt);
  moves -= run_cost(g->m.move_cost(pt) * 50);
  _normalize_path(path, pos); // maintain path (should cost CPU)
 } else if (g->m.open_door(pt.x, pt.y, (g->m.ter(pos) == t_floor)))
  moves -= 100;
 else if (g->m.has_flag(bashable, pt)) {
  moves -= 110;
  std::string bashsound;
  int smashskill = int(str_cur / 2 + weapon.type->melee_dam);
  g->m.bash(pt, smashskill, bashsound);
  g->sound(pt, 18, bashsound);
 } else
  moves -= 100;
}

void npc::move_to_next(game *g)
{
 _normalize_path(path, pos);
 if (path.empty()) {
  debugmsg("npc::move_to_next() called with an empty path!");
  move_pause();
  return;
 }
 move_to(g, path[0]);
}

// TODO: Rewrite this.  It doesn't work well and is ugly.
void npc::avoid_friendly_fire(game *g, int target)
{
 ai_target Target;
 if (!decode_target(target, Target)) {
#ifndef NDEBUG
	 throw std::logic_error("npc::avoid_friendly_fire called with invalid target");
#else
	 debugmsg("npc::avoid_friendly_fire() called with no target!");
	 move_pause();
	 return;
#endif
 } else if (auto mon = std::get<0>(Target.second)) {
	 if (!one_in(3)) say(g, "<move> so I can shoot that %s!", mon->name().c_str());
 }	// \todo similar message when targeting NPC?

 point tar = Target.first;	// backward compatibility

 int xdir = (tar.x > pos.x ? 1 : -1), ydir = (tar.y > pos.y ? 1 : -1);
 direction dir_to_target = direction_from(pos, tar);
 std::vector<point> valid_moves;

 // 2019-02-16: re-implemented along Angband lines
 int delta = 4 * rng(0, 1) - 2;	// sideways
 valid_moves.push_back(pos + direction_vector(rotate_clockwise(dir_to_target, delta)));
 valid_moves.push_back(pos + direction_vector(rotate_clockwise(dir_to_target, -delta)));
 delta = 6 * rng(0, 1) - 3;	// backward
 valid_moves.push_back(pos + direction_vector(rotate_clockwise(dir_to_target, delta)));
 valid_moves.push_back(pos + direction_vector(rotate_clockwise(dir_to_target, -delta)));
 delta = 2 * rng(0, 1) - 1;	// towards
 valid_moves.push_back(pos + direction_vector(rotate_clockwise(dir_to_target, delta)));
 valid_moves.push_back(pos + direction_vector(rotate_clockwise(dir_to_target, -delta)));

 for (int i = 0; i < valid_moves.size(); i++) {
  if (can_move_to(g->m, valid_moves[i])) {
   move_to(g, valid_moves[i]);
   return;
  }
 }

/* If we're still in the function at this point, maneuvering can't help us. So,
 * might as well address some needs.
 * We pass a <danger> value of NPC_DANGER_VERY_LOW + 1 so that we won't start
 * eating food (or, god help us, sleeping).
 */
 ai_action action = address_needs(g, NPC_DANGER_VERY_LOW + 1);
 if (!action.first && !action.second) action.first = npc_pause;
 execute_action(g, action, target);
}

bool player::move_away_from(const map& m, const point& tar, point& dest) const
{
	std::vector<point> options;
	point d(0, 0);
	if (tar.x < pos.x) d.x = 1;
	else if (tar.x > pos.x) d.x = -1;
	if (tar.y < pos.y) d.y = 1;
	else if (tar.y < pos.y) d.y = -1;

	// 2019-02-16: re-implemented along Angband lines
	direction best = direction_from(0, 0, d);
	options.push_back(pos + direction_vector(best));	// consistency
	int delta = 2 * rng(0, 1) - 1;
	options.push_back(pos + direction_vector(rotate_clockwise(best, delta)));	// 45 degrees off
	options.push_back(pos + direction_vector(rotate_clockwise(best, -delta)));

	delta = 4 * rng(0, 1) - 2;
	options.push_back(pos + direction_vector(rotate_clockwise(best, delta)));	// 90 degrees off
	options.push_back(pos + direction_vector(rotate_clockwise(best, -delta)));

	// looks strange to go the other way when backed against a wall
	if (trig_dist(tar, options[4]) < trig_dist(tar, options[3])) std::swap(options[3], options[4]);

	for (const auto& pt : options) if (can_move_to(m, pt)) {
		dest = pt;
		return true;
	}
	return false;
}

void npc::move_pause()
{
 moves = 0;
 if (recoil > 0) {
  if (str_cur + 2 * sklevel[sk_gun] >= recoil)
   recoil = 0;
  else {
   recoil -= str_cur + 2 * sklevel[sk_gun];
   recoil = int(recoil / 2);
  }
 }
}

void npc::find_item(game *g)
{
 fetching_item = false;
 int best_value = minimum_item_value();
 int range = sight_range(g->light_level());
 if (range > 12) range = 12;
 int minx = pos.x - range, maxx = pos.x + range,
     miny = pos.y - range, maxy = pos.y + range;
 item* pickup = NULL;
 if (minx < 0) minx = 0;
 if (miny < 0) miny = 0;
 if (maxx >= SEEX * MAPSIZE) maxx = SEEX * MAPSIZE - 1;
 if (maxy >= SEEY * MAPSIZE) maxy = SEEY * MAPSIZE - 1;

 for (int x = minx; x <= maxx; x++) {
  for (int y = miny; y <= maxy; y++) {
   if (g->m.sees(pos, x, y, range)) {
	for(auto& obj : g->m.i_at(x, y)) {
     int itval = value(obj);
     int wgt = obj.weight(), vol = obj.volume();
     if (itval > best_value &&
          (weight_carried() + wgt <= weight_capacity() / 4 &&
           volume_carried() + vol <= volume_capacity()       )) {
      it = point(x,y);
	  pickup = &obj;
      best_value = itval;
      fetching_item = true;
     }
    }
   }
  }
 }

 if (fetching_item && is_following())
  say(g, "Hold on, I want to pick up that %s.", pickup->tname().c_str());
}

void npc::pick_up_item()
{
 auto g = game::active();
// We're adjacent to the item; grab it!
 moves -= 100;
 fetching_item = false;
 std::vector<item>& items = g->m.i_at(it);
 int total_volume = 0, total_weight = 0; // How much the items will add
 std::vector<int> pickup; // Indices of items we want
// int worst_value = worst_item_value();	//	\todo V 0.2.1+ actually use this when deciding what to pick up?
 const int vol_carried = volume_carried();
 const int wgt_carried = weight_carried();
 const int vol_capacity = volume_capacity();
 const int wgt_capacity = weight_capacity();

 for (int i = 0; i < items.size(); i++) {	// \todo V 0.2.1+ be more aware of what we're doing here (packing problem)
  auto& it = items[i];
  int itval = value(it), vol = it.volume(), wgt = it.weight();
  if (itval >= minimum_item_value() &&
      (vol_carried + total_volume + vol <= vol_capacity &&
       wgt_carried + total_weight + wgt <= wgt_capacity / 4)) {
   pickup.push_back(i);
   total_volume += vol;
   total_weight += wgt;
  }
 }
// Describe the pickup to the player
 bool u_see_me = g->u_see(pos), u_see_items = g->u_see(it);
 // \todo good use case for std::string or std::strstream here
 if (u_see_me) {
  if (pickup.size() == 1) {
   if (u_see_items)
    messages.add("%s picks up a %s.", name.c_str(), items[pickup[0]].tname().c_str());
   else
    messages.add("%s picks something up.", name.c_str());
  } else if (pickup.size() == 2) {
   if (u_see_items)
    messages.add("%s picks up a %s and a %s.", name.c_str(),
               items[pickup[0]].tname().c_str(),
               items[pickup[1]].tname().c_str());
   else
    messages.add("%s picks up a couple of items.", name.c_str());
  } else
   messages.add("%s picks up several items.", name.c_str());
 } else if (u_see_items) {
  if (pickup.size() == 1)
   messages.add("Someone picks up a %s.", items[pickup[0]].tname().c_str());
  else if (pickup.size() == 2)
   messages.add("Someone picks up a %s and a %s", 
              items[pickup[0]].tname().c_str(),
              items[pickup[1]].tname().c_str());
  else
   messages.add("Someone picks up several items.");
 }
  
 for (int i = 0; i < pickup.size(); i++) i_add(items[pickup[i]]);
 {	// indexes are added in strictly increasing order.  Doing this separately as we are not guaranteed this does not invalidate our pointer
 int i = pickup.size();
 while (0 < i--) g->m.i_rem(it, pickup[i]);
 }
}

#if PROTOTYPE
void npc::drop_items(game *g, int weight, int volume)
{
 if (game::debugmon) {
  debugmsg("%s is dropping items-%d,%d (%d items, wgt %d/%d, vol %d/%d)",
           name.c_str(), weight, volume, inv.size(), weight_carried(),
           weight_capacity() / 4, volume_carried(), volume_capacity());
  int wgtTotal = 0, volTotal = 0;
  for (size_t i = 0; i < inv.size(); i++) {
   wgtTotal += inv[i].volume();
   volTotal += inv[i].weight();
   debugmsg("%s (%d of %d): %d/%d, total %d/%d", inv[i].tname().c_str(), i,
            inv.size(), inv[i].weight(), inv[i].volume(), wgtTotal, volTotal);
  }
 }
  
 int weight_dropped = 0, volume_dropped = 0;
 std::vector<ratio_index> rWgt, rVol; // Weight/Volume to value ratios

// First fill our ratio vectors, so we know which things to drop first
 for (size_t i = 0; i < inv.size(); i++) {
  double wgt_ratio, vol_ratio;
  if (value(inv[i]) == 0) {
   wgt_ratio = 99999;
   vol_ratio = 99999;
  } else {
   wgt_ratio = inv[i].weight() / value(inv[i]);
   vol_ratio = inv[i].volume() / value(inv[i]);
  }
  bool added_wgt = false, added_vol = false;
  for (int j = 0; j < rWgt.size() && !added_wgt; j++) {
   if (wgt_ratio > rWgt[j].ratio) {
    added_wgt = true;
    rWgt.insert(rWgt.begin() + j, ratio_index(wgt_ratio, i));
   }
  }
  if (!added_wgt)
   rWgt.push_back(ratio_index(wgt_ratio, i));
  for (int j = 0; j < rVol.size() && !added_vol; j++) {
   if (vol_ratio > rVol[j].ratio) {
    added_vol = true;
    rVol.insert(rVol.begin() + j, ratio_index(vol_ratio, i));
   }
  }
  if (!added_vol)
   rVol.push_back(ratio_index(vol_ratio, i));
 }

 std::ostringstream item_name; // For description below
 int num_items_dropped = 0; // For description below
// Now, drop items, starting from the top of each list
 while (weight_dropped < weight || volume_dropped < volume) {
// weight and volume may be passed as 0 or a negative value, to indicate that
// decreasing that variable is not important.
  int dWeight = (weight <= 0 ? -1 : weight - weight_dropped);
  int dVolume = (volume <= 0 ? -1 : volume - volume_dropped);
  int index;
// Which is more important, weight or volume?
  if (dWeight > dVolume) {
   index = rWgt[0].index;
   rWgt.erase(rWgt.begin());
// Fix the rest of those indices.
   for (auto& w_ratio : rWgt) {
    if (w_ratio.index > index) w_ratio.index--;
   }
  } else {
   index = rVol[0].index;
   rVol.erase(rVol.begin());
// Fix the rest of those indices.
   for (auto& vol_ratio : rVol) {
    if (vol_ratio.index > index) vol_ratio.index--;
   }
  }
  weight_dropped += inv[index].weight();
  volume_dropped += inv[index].volume();
  item dropped = i_remn(index);
  num_items_dropped++;
  if (num_items_dropped == 1)
   item_name << dropped.tname();
  else if (num_items_dropped == 2)
   item_name << " and a " << dropped.tname();
  g->m.add_item(pos, dropped);
 }
// Finally, describe the action if u can see it
 std::string item_name_str = item_name.str();
 if (g->u_see(pos)) {
  if (num_items_dropped >= 3)
   messages.add("%s drops %d items.", name.c_str(), num_items_dropped);
  else
   messages.add("%s drops a %s.", name.c_str(), item_name_str.c_str());
 }
}
#endif

npc::ai_action npc::scan_new_items(game *g, int target)
{
 const bool can_use_gun = (!is_following() || combat_rules.use_guns);
// Check if there's something better to wield
 bool has_better_melee = false;
 std::vector<int> empty_guns;
 if (can_use_gun) {
  int gun;
  if (best_gun(target, gun, empty_guns, has_better_melee)) {
   return ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new target_inventory<npc>(*this, gun, &npc::wield, "Wield loaded gun")));
  }
 } else {
  has_better_melee = can_wield_better_melee();
 }

 const bool has_empty_gun = !empty_guns.empty();
 int wield_this;

 if (has_empty_gun && choose_empty_gun(empty_guns, wield_this))
	 return ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new target_inventory<npc>(*const_cast<npc*>(this), wield_this, &npc::wield, "Wield empty gun")));
 else if (has_better_melee) {
	 if (best_melee_weapon(wield_this)) return ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new target_inventory<npc>(*const_cast<npc*>(this), wield_this, &npc::wield, "Wield melee weapon")));
 }

 return ai_action(npc_pause, std::unique_ptr<cataclysm::action>());
}

void npc::melee_monster(game *g, monster& monhit)
{
 int dam = hit_mon(g, &monhit);
 if (monhit.hurt(dam)) g->kill_mon(monhit);
}

void npc::melee_player(game *g, player &foe)
{
 hit_player(g, foe);
}

bool npc::best_melee_weapon(int& inv_index) const
{
	int best_score = 0, index = -999;
	for (size_t i = 0; i < inv.size(); i++) {
		int score = inv[i].melee_value(sklevel);
		if (score > best_score) {
			best_score = score;
			index = i;
		}
	}
	if (!styles.empty() && // Wield a style if our skills warrant it
		best_score < 15 * sklevel[sk_unarmed] + 8 * sklevel[sk_melee]) {
		// TODO: More intelligent style choosing
		inv_index = -rng(1, styles.size());
		return true;
	}
	if (index == -999) return false;
	inv_index = index;
	return true;
}

bool npc::can_wield_better_melee() const
{
	for (size_t i = 0; i < inv.size(); i++) {
		if (inv[i].melee_value(sklevel) > weapon.melee_value(sklevel) * 1.1) return true;
	}
	return false;
}

bool npc::best_gun(const int target, int& inv_index, std::vector<int>& empty_guns, bool& has_better_melee) const
{
	bool ret = false;
	int max = 0;
	inv_index = -1;
	has_better_melee = false;
	empty_guns.clear();
	for (size_t i = 0; i < inv.size(); i++) {
		const auto& obj = inv[i];
		if (obj.is_gun() && obj.charges > 0) {
			if (obj.charges > max) {
				max = obj.charges;
				inv_index = i;
				ret = true;
			}
		} else if (obj.is_gun() && enough_time_to_reload(game::active(), target, obj)) empty_guns.push_back(i);
		else if (obj.melee_value(sklevel) > weapon.melee_value(sklevel) * 1.1) has_better_melee = true;
	}
	return ret;
}

static bool thrown_item(const item& used)	// not general enough to migrate to item member function
{
	if (used.active) return true;	// already activated
	switch (used.type->id)
	{
	case itm_knife_combat:
	case itm_spear_wood: return true; // whitelist of throwing weapons \todo? bitflag to enable JSON configuration
	default: return false;
	}
}

void npc::alt_attack(game *g, int target)
{
 ai_target Target;

 if (!decode_target(target, Target)) {
#ifndef NDEBUG
	 throw std::logic_error("npc::alt_attack called without a valid target");
#else
	 debugmsg("npc::alt_attack() called with target = %d", target);
	 move_pause();
	 return;
#endif
 }

 point tar = Target.first;	// backward compatibility

 const itype_id which = alt_attack_available();
 DEBUG_FAIL_OR_LEAVE(itm_null == which, return);	// We ain't got shit!  Definitely should not happen.

 int index;
 item *used = NULL;
 if (weapon.type->id == which) {
  used = &weapon;
  index = -1;
 } else {
  for (size_t i = 0; i < inv.size(); i++) {
   if (inv[i].type->id == which) {
    used = &(inv[i]);
    index = i;
   }
  }
 }
 DEBUG_FAIL_OR_LEAVE(!used, return);	// invariant violation

// Are we going to throw this item?
 if (!thrown_item(*used)) activate_item(g, *used);
 else { // We are throwing it!

  std::vector<point> trajectory;
  const int light = g->light_level();
  const int dist = rl_dist(pos, tar);
  const bool no_friendly_fire = wont_hit_friend(g, tar, index);
  const int conf = confident_range(index);

  if (dist <= conf && no_friendly_fire) {
   {
   int linet;
   trajectory = line_to(pos, tar, (g->m.sees(pos, tar, light, linet) ? linet : 0));
   }
   moves -= 125;
   if (g->u_see(pos))
    messages.add("%s throws a %s.", name.c_str(), used->tname().c_str());
   g->throw_item(*this, tar, *used, trajectory);
   i_remn(index);

  } else if (!no_friendly_fire) {// Danger of friendly fire

   if (!used->active || used->charges > 2) // Safe to hold on to, for now
    avoid_friendly_fire(g, target); // Maneuver around player
   else { // We need to throw this live (grenade, etc) NOW! Pick another target?
    for (int dist = 2; dist <= conf; dist++) {
     for (int x = pos.x - dist; x <= pos.x + dist; x++) {
      for (int y = pos.y - dist; y <= pos.y + dist; y++) {
       int newtarget = g->mon_at(x, y);
       int newdist = rl_dist(pos, x, y);
// TODO: Change "newdist >= 2" to "newdist >= safe_distance(used)"
// Molotovs are safe at 2 tiles, grenades at 4, mininukes at 8ish
       if (newdist <= conf && newdist >= 2 && newtarget != -1 &&
           wont_hit_friend(g, x, y, index)) { // Friendlyfire-safe!
        alt_attack(g, newtarget);
        return;
       }
      }
     }
    }
/* If we have reached THIS point, there's no acceptible monster to throw our
 * grenade or whatever at.  Since it's about to go off in our hands, better to
 * just chuck it as far away as possible--while being friendly-safe.
 */
    int best_dist = 0;
    for (int dist = 2; dist <= conf; dist++) {
     for (int x = pos.x - dist; x <= pos.x + dist; x++) {
      for (int y = pos.y - dist; y <= pos.y + dist; y++) {
       int new_dist = rl_dist(pos, x, y);
       if (new_dist > best_dist && wont_hit_friend(g, x, y, index)) {
        best_dist = new_dist;
		tar = point(x, y);
       }
      }
     }
    }
/* Even if tarx/tary didn't get set by the above loop, throw it anyway.  They
 * should be equal to the original location of our target, and risking friendly
 * fire is better than holding on to a live grenade / whatever.
 */
	{
    int linet;
	trajectory = line_to(pos, tar, ((g->m.sees(pos, tar, light, linet)) ? linet : 0));
	}
    moves -= 125;
    if (g->u_see(pos))
     messages.add("%s throws a %s.", name.c_str(), used->tname().c_str());
    g->throw_item(*this, tar, *used, trajectory);
    i_remn(index);
   }

  } else { // Within this block, our chosen target is outside of our range
   update_path(g->m, tar);
   move_to_next(g); // Move towards the target
  }
 } // Done with throwing-item block
}

void npc::activate_item(game *g, item& it)	// unclear whether this "works"; parallel is npc::use_escape_item
{
 if (it.is_tool()) {
  const it_tool* const tool = dynamic_cast<const it_tool*>(it.type);
  (*tool->use)(g, this, &it, false);
 } else if (it.is_food()) {
  const it_comest* const comest = dynamic_cast<const it_comest*>(it.type);
  (*comest->use)(g, this, &it, false);
 }
}

std::pair<hp_part, int> player::worst_injury() const
{
	std::pair<hp_part, int> ret(hp_torso, 0);

	int i = num_hp_parts;
	while (0 <= --i) {
		if (hp_max[i] <= hp_cur[i]) continue;
		int delta = hp_max[i] - hp_cur[i];
		// The head and torso are weighted more heavily than other body parts
		if (hp_head == i) delta *= 3;
		else if (hp_torso == i) delta *= 2;
		if (delta > ret.second) {
			ret.first = hp_part(i);
			ret.second = delta;
		}
	}
	// deflate before return for accuracy later on
	if (hp_head == ret.second) ret.second /= 3;
	else if (hp_torso == ret.second) ret.second /= 2;
	return ret;
}

std::pair<itype_id, int> player::would_heal(const std::pair<hp_part, int>& injury) const
{
	std::pair<itype_id, int> ret(itm_null, 0);	// note that itm_null is 0 i.e. C-false

	if (ret.second < injury.second && has_amount(itm_bandages, 1)) {
		ret.first = itm_bandages;
		switch (injury.first) {
		case hp_head:  ret.second = 1 + 1.6 * sklevel[sk_firstaid]; break;
		case hp_torso: ret.second = 4 + 3 * sklevel[sk_firstaid]; break;
		default:       ret.second = 3 + 2 * sklevel[sk_firstaid];
		}
	}
	if (ret.second < injury.second && has_amount(itm_1st_aid, 1)) {
		switch (injury.first) {
		case hp_head:  ret.second = 10 + 1.6 * sklevel[sk_firstaid]; break;
		case hp_torso: ret.second = 20 + 3 * sklevel[sk_firstaid]; break;
		default:       ret.second = 15 + 2 * sklevel[sk_firstaid];
		}
	}
	return ret;
}

#if PROTOTYPE
void npc::heal_player(game *g, player &patient)
{
 int dist = rl_dist(pos, patient.pos);
 
 if (dist > 1) { // We need to move to the player
  update_path(g->m, patient.pos);
  move_to_next(g);
 } else { // Close enough to heal!
  const auto injury = patient.worst_injury();
  const auto predicted_heal = would_heal(injury);
  if (!predicted_heal.first) FATAL("tried to heal w/o means to heal");	// \todo consider action-looping instead, or converting later action loops to FATAL

  bool u_see_me      = g->u_see(pos),
       u_see_patient = g->u_see(patient.pos);
  if (patient.is_npc()) {
   if (u_see_me) {
    if (u_see_patient)
     messages.add("%s heals %s.",  name.c_str(), patient.name.c_str());
    else
     messages.add("%s heals someone.", name.c_str());
   } else if (u_see_patient)
    messages.add("Someone heals %s.", patient.name.c_str());
  } else if (u_see_me)
   messages.add("%s heals you.", name.c_str());
  else
   messages.add("Someone heals you.");

  use_charges(predicted_heal.first, 1);
  patient.heal(injury.first, predicted_heal.second);
  moves -= 250;
 
  if (!patient.is_npc()) {
 // Test if we want to heal the player further
   if (op_of_u.value * 4 + op_of_u.trust + personality.altruism * 3 +
       (fac_has_value(FACVAL_CHARITABLE) ?  5 : 0) +
       (fac_has_job  (FACJOB_DOCTORS)    ? 15 : 0) - op_of_u.fear * 3 <  25) {
    attitude = NPCATT_FOLLOW;
    say(g, "That's all the healing I can do.");
   } else
    say(g, "Hold still, I can heal you more.");
  }
 }
}
#endif

void npc::heal_self(game *g)
{
 const auto injury = worst_injury();
 const auto predicted_heal = would_heal(injury);
 if (!predicted_heal.first) FATAL("tried to heal self w/o means to heal");

 use_charges(predicted_heal.first, 1);
 heal(injury.first, predicted_heal.second);
 moves -= 250;

 if (g->u_see(pos))
	 messages.add("%s heals %sself.", name.c_str(), (male ? "him" : "her"));
}

int npc::pick_best_painkiller(const inventory& _inv) const
{
	int difference = 9999, index = -1;
	for (size_t i = 0; i < _inv.size(); i++) {
		int diff = 9999;
		switch (_inv[i].type->id)
		{
		case itm_aspirin:
			if (35 < pain) continue;	// from legacy has_painkiller: arguable
			diff = abs(pain - 15);
			break;
		case itm_codeine:
			diff = abs(pain - 30);
			break;
		case itm_oxycodone:
			if (50 > pain) continue;	// from legacy has_painkiller: not as enjoyable as heroin
			diff = abs(pain - 60);
			break;
		case itm_heroin:
			diff = abs(pain - 100);
			break;
		case itm_tramadol:
			diff = abs(pain - 40) / 2; // Bonus since it's long-acting
			break;
		default: continue;	// not a painkiller
		}
		if (diff < difference) {
			difference = diff;
			index = i;
		}
	}
	return index;
}

int npc::pick_best_food(const inventory& _inv) const
{
 int best_hunger = 999, best_thirst = 999, index = -1;
 bool thirst_more_important = (thirst > hunger * 1.5);
 for (size_t i = 0; i < _inv.size(); i++) {
  int eaten_hunger = -1, eaten_thirst = -1;
  const item& it = _inv[i];
  const it_comest* food = NULL;
  if (it.is_food()) food = dynamic_cast<const it_comest*>(it.type);
  else if (it.is_food_container()) food = dynamic_cast<const it_comest*>(it.contents[0].type);
  if (!food) continue;
  eaten_hunger = hunger - food->nutr;
  eaten_thirst = thirst - food->quench;
  if (0 >= eaten_hunger) continue;	// not desperate enough to risk vomiting, etc.
  if (   (thirst_more_important ? (eaten_thirst < best_thirst) : (eaten_hunger < best_hunger))
	  || (eaten_thirst == best_thirst && eaten_hunger < best_hunger)
	  || (eaten_hunger == best_hunger && eaten_thirst < best_thirst)) {
    if (eaten_hunger < best_hunger) best_hunger = eaten_hunger;
    if (eaten_thirst < best_thirst) best_thirst = eaten_thirst;
    index = i;
  }
 }
 return index;
}

void npc::mug_player(player &mark)
{
 auto g = game::active();
 if (rl_dist(pos, mark.pos) > 1) { // We have to travel; path already updated
  move_to_next(g);
  return;
 }
  bool u_see_me   = g->u_see(pos),
       u_see_mark = g->u_see(mark.pos);
  if (mark.cash > 0) {
   cash += mark.cash;
   mark.cash = 0;
   moves = 0;
// Describe the action
   if (mark.is_npc()) {
    if (u_see_me) {
     if (u_see_mark)
      messages.add("%s takes %s's money!", name.c_str(), mark.name.c_str());
     else
      messages.add("%s takes someone's money!", name.c_str());
    } else if (u_see_mark)
     messages.add("Someone takes %s's money!", mark.name.c_str());
   } else {
    if (u_see_me)
     messages.add("%s takes your money!", name.c_str());
    else
     messages.add("Someone takes your money!");
   }
  } else { // We already have their money; take some goodies!
// value_mod affects at what point we "take the money and run"
// A lower value means we'll take more stuff
   double value_mod = 1 - double((10 - personality.bravery)    * .05) -
                          double((10 - personality.aggression) * .04) -
                          double((10 - personality.collector)  * .06);
   if (!mark.is_npc()) {
    value_mod += double(op_of_u.fear * .08);
    value_mod -= double((8 - op_of_u.value) * .07);
   }
   int best_value = minimum_item_value() * value_mod, index = -1;
   for (size_t i = 0; i < mark.inv.size(); i++) {
	const item& it = mark.inv[i];
	const auto current_value = value(it);
    if (current_value >= best_value &&
        volume_carried() + it.volume() <= volume_capacity() &&
        weight_carried() + it.weight() <= weight_capacity()   ) {
     best_value = current_value;
     index = i;
    }
   }
   if (index == -1) { // Didn't find anything worthwhile!
    attitude = NPCATT_FLEE;
    if (!one_in(3)) say(g, "<done_mugging>");
    moves -= 100;
   } else {
    bool u_see_me   = g->u_see(pos),
         u_see_mark = g->u_see(mark.pos);
    item stolen = mark.i_remn(index);
    if (mark.is_npc()) {
     if (u_see_me) {
      if (u_see_mark)
       messages.add("%s takes %s's %s.", name.c_str(), mark.name.c_str(), stolen.tname().c_str());
      else
       messages.add("%s takes something from somebody.", name.c_str());
     } else if (u_see_mark)
      messages.add("Someone takes %s's %s.", mark.name.c_str(), stolen.tname().c_str());
    } else {
     if (u_see_me)
      messages.add("%s takes your %s.", name.c_str(), stolen.tname().c_str());
     else
      messages.add("Someone takes your %s.", stolen.tname().c_str());
    }
    i_add(stolen);
    moves -= 100;
    if (!mark.is_npc()) op_of_u.value -= rng(0, 1); // Decrease the value of the player
   }
  }
}

npc::ai_action npc::look_for_player(player& sought)
{
	auto g = game::active();
	const int light = g->light_level();

	// 2019-11-19: but nothing sets pl?
	if (saw_player_recently() && g->m.sees(pos, pl.x, light)) {
		// npc::pl is the point where we last saw the player
		if (update_path(g->m, pl.x, 0))
			return ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new move_step_screen(*this, path.front(), "Look for player")));
	}
	const int range = sight_range(light);
	if (g->m.sees(pos, sought.pos, range)) {
		// invariant violation.
#ifndef NDEBUG
		throw std::logic_error("trying to look for player in sight");
#else
		if (sought.is_npc())
			debugmsg("npc::look_for_player() called, but we can see %s!",
				sought.name.c_str());
		else
			debugmsg("npc::look_for_player() called, but we can see u!");
		return ai_action(npc_pause, std::unique_ptr<cataclysm::action>());	// failover
#endif
	}
	if (!path.empty() && !g->m.sees(pos, path.back(), range))
		return ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new move_step_screen(*this, path.front(), "Look for player")));

	// collate possibilities (this is a rewrite target)
	std::vector<point> possibilities;
	point dest;
	for (dest.x = 1; dest.x < SEEX * MAPSIZE; dest.x += 11) { // 1, 12, 23, 34
		for (dest.y = 1; dest.y < SEEY * MAPSIZE; dest.y += 11) {
			if (pos!=dest && g->m.sees(pos, dest, range)) possibilities.push_back(dest);
		}
	}
	if (_path_goes_there(path, possibilities.begin(), possibilities.end())) {
		if (one_in(6)) say(g, "<wait>");
		return ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new move_step_screen(*this, path.front(), "Look for player")));
	}

	auto count = possibilities.size();
	while (0 < count) {
		int index = rng(0, --count);
		if (update_path(g->m, possibilities[index], 0)) {
			if (one_in(6)) say(g, "<wait>");
			return ai_action(npc_pause, std::unique_ptr<cataclysm::action>(new move_step_screen(*this, path.front(), "Look for player")));
		}
		possibilities.erase(possibilities.begin() + index);
	}
	say(g, "<wait>");
	return ai_action(npc_pause, std::unique_ptr<cataclysm::action>());	// stall-out failover
}

bool npc::saw_player_recently() const
{
 return (pl.x.x >= 0 && pl.x.x < SEEX * MAPSIZE && pl.x.y >= 0 && pl.x.y < SEEY * MAPSIZE && pl.live());
}

bool npc::has_destination() const
{
 return (goal.second.x >= 0 && goal.second.x < OMAPX && goal.second.y >= 0 && goal.second.y < OMAPY);
}

void npc::set_destination(game *g)
{
/* TODO: Make NPCs' movement more intelligent.
 * Right now this function just makes them attempt to address their needs:
 *  if we need ammo, go to a gun store, if we need food, go to a grocery store,
 *  and if we don't have any needs, pick a random spot.
 * What it SHOULD do is that, if there's time; but also look at our mission and
 *  our faction to determine more meaningful actions, such as attacking a rival
 *  faction's base, or meeting up with someone friendly.  NPCs should also
 *  attempt to reach safety before nightfall, and possibly similar goals.
 * Also, NPCs should be able to assign themselves missions like "break into that
 *  lab" or "map that river bank."
 */
 decide_needs();
 if (needs.empty()) // We don't need anything in particular.
  needs.push_back(need_none);
 std::vector<oter_id> options;
 switch(needs[0]) {
  case need_ammo:	options.push_back(ot_house_north);
  case need_gun:	options.push_back(ot_s_gun_north); break;

  case need_weapon:	options.push_back(ot_s_gun_north);
			options.push_back(ot_s_sports_north);
			options.push_back(ot_s_hardware_north); break;

  case need_drink:	options.push_back(ot_s_gas_north);
			options.push_back(ot_s_pharm_north);
			options.push_back(ot_s_liquor_north);
  case need_food:	options.push_back(ot_s_grocery_north); break;

  default:		options.push_back(ot_house_north);
			options.push_back(ot_s_gas_north);
			options.push_back(ot_s_pharm_north);
			options.push_back(ot_s_hardware_north);
			options.push_back(ot_s_sports_north);
			options.push_back(ot_s_liquor_north);
			options.push_back(ot_s_gun_north);
			options.push_back(ot_s_library_north);
 }

 oter_id dest_type = options[rng(0, options.size() - 1)];

 const auto om = overmap::toOvermap(GPSpos);
 g->cur_om.find_closest(om.second, dest_type, 4, goal);
}

void npc::go_to_destination(game *g)
{
  auto om = overmap::toOvermap(GPSpos);
  if (goal == om) {	// We're at our desired map square!
   move_pause();
   reach_destination();
  }
  // NPCs historically don't go underground/change Z levels much \todo fix as part of long-range pathing
  point s(goal.first.x==om.first.x && goal.first.y==goal.second.y ? cmp(goal.second, om.second) : cmp(point(om.first.x,om.first.y),point(goal.first.x,goal.first.y)));
// sx and sy are now equal to the direction we need to move in
  point d;
  point target(pos + 8 * s);
  int light = g->light_level();
// x and y are now equal to a local square that's close by
  for (int i = 0; i < 8; i++) {
   for (d.x = 0 - i; d.x <= i; d.x++) {
    for (d.y = 0 - i; d.y <= i; d.y++) {
	 const point dest(target+d);
     if ((g->m.move_cost(dest) > 0 ||
          g->m.has_flag(bashable, dest) ||
          g->m.ter(dest) == t_door_c) &&
         g->m.sees(pos, dest, light)) {
      path = g->m.route(pos, dest);
      if (!path.empty() && can_move_to(g->m, path[0])) {
       move_to_next(g);
	   om = overmap::toOvermap(GPSpos);	// GPSpos updated so this updates
       if (goal == om) {	// We're at our desired map square!
        reach_destination();
       }
       return;
      } else {
       move_pause();	// XXX error path
       return;
      }
     }
    }
   }
  }
  move_pause();	// XXX error path
}

std::string npc_action_name(npc_action action)
{
 switch (action) {
  case npc_undecided:		return "Undecided";
  case npc_pause:		return "Pause";
#if DEAD_FUNC
  case npc_sleep:		return "Sleep";
  case npc_drop_items:		return "Drop items";
  case npc_heal_player:		return "Heal player";
#endif
  case npc_heal:		return "Heal self";
  case npc_melee:		return "Melee";
  case npc_alt_attack:		return "Use alternate attack";
  case npc_talk_to_player:	return "Talk to player";
  case npc_goto_destination:	return "Go to destination";
  case npc_avoid_friendly_fire:	return "Avoid friendly fire";
  default: 			return "Unnamed action";
 }
}
