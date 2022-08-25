#include "trap_handler.hpp"
#include "trap.h"
#include "rng.h"
#include "game.h"
#include "recent_msg.h"
#include "stl_typetraits_late.h"

// \todo replace this with a reality_bubble version
void trap_fully_triggered(map& m, const point& pt, const std::vector<item_drop_spec>& drop_these)
{
    m.tr_at(pt) = tr_null;
    for (decltype(auto) drop : drop_these) {
        int n = drop.qty;
        while (0 <= --n) {
            // hard-code one_in processing for now
            if (1 < drop.modifier && !one_in(drop.modifier)) continue;
            m.add_item(pt, item::types[drop.what], 0); // could use messages.turn instead; current representation doesn't track age of components used to build the trap
        }
    }
}

void trap_fully_triggered(GPS_loc loc, const std::vector<item_drop_spec>& drop_these)
{
    loc.trap_at() = tr_null;
    for (decltype(auto) drop : drop_these) {
        int n = drop.qty;
        while (0 <= --n) {
            // hard-code one_in processing for now
            if (1 < drop.modifier && !one_in(drop.modifier)) continue;
            loc.add(item(item::types[drop.what], 0)); // could use messages.turn instead; current representation doesn't track age of components used to build the trap
        }
    }
}

void trap::trigger(monster& mon) const
{
    if (actm) actm(game::active(), &mon); // legacy adapter
    if (act_pos) act_pos(mon.pos);
}

void trap::trigger(player& u) const
{
    if (act) act(game::active(), u.pos.x, u.pos.y); // legacy adapter
    if (act_pos) act_pos(u.pos);
}

void trap::trigger(player& u, const point& tr_pos) const
{
    if (act) act(game::active(), tr_pos.x, tr_pos.y); // legacy adapter
    if (act_pos) act_pos(tr_pos);
}

void trap::trigger(player& u, const GPS_loc& tr_loc) const
{
    if (const auto pos = game::active()->toScreen(tr_loc)) {
        if (act) act(game::active(), pos->x, pos->y); // legacy adapter
        if (act_pos) act_pos(*pos);
    }
}

void trapfunc::bubble(game *g, int x, int y)
{
 messages.add("You step on some bubblewrap!");
}

void trapfunc::bubble(const point& pt)
{
    auto g = game::active();
    g->sound(pt, 18, "Pop!");
    g->m.tr_at(pt) = tr_null;
}

void trapfunc::beartrap(game *g, int x, int y)
{
 messages.add("A bear trap closes on your foot!");
 g->sound(point(x, y), 8, "SNAP!");
 g->u.hit(g, bp_legs, rng(0, 1), 10, 16);
 g->u.add_disease(DI_BEARTRAP, -1);
 // the two beartraps have the same configuration
 trap_fully_triggered(g->m, point(x, y), trap::traps[tr_beartrap]->trigger_components);
}

void trapfuncm::beartrap(game *g, monster *z)
{
 g->sound(z->pos, 8, "SNAP!");
 if (z->hurt(35)) g->kill_mon(*z);
 else {
  z->moves = 0;
  z->add_effect(ME_BEARTRAP, rng(8, 15));
 }
 z->GPSpos.trap_at() = tr_null;
 item beartrap(item::types[itm_beartrap], 0);
 z->add_item(beartrap); // \todo would prefer parity with player case
}

void trapfunc::board(game *g, int x, int y)
{
 messages.add("You step on a spiked board!");
 g->u.hit(g, bp_feet, 0, 0, rng(6, 10));
 g->u.hit(g, bp_feet, 1, 0, rng(6, 10));
}

void trapfuncm::board(game *g, monster *z)
{
 if (g->u.see(*z)) messages.add("%s steps on a spiked board!",
     grammar::capitalize(z->desc(grammar::noun::role::subject, grammar::article::definite)).c_str());
 if (z->hurt(rng(6, 10))) g->kill_mon(*z);
 else z->moves -= 80;
}

void trapfunc::tripwire(game *g, int x, int y)
{
 const point pt(x, y);
 messages.add("You trip over a tripwire!");
 std::vector<point> valid;
 for (decltype(auto) dir : Direction::vector) {
     const auto dest(pt + dir);
     if (g->is_empty(dest)) valid.push_back(dest); // No monster, NPC, or player, plus valid for movement
 }
 if (!valid.empty()) g->u.screenpos_set(valid[rng(0, valid.size() - 1)]);
 g->u.moves -= (mobile::mp_turn / 2) * 3;
 if (rng(5, 20) > g->u.dex_cur) g->u.hurtall(rng(1, 4));
}

void trapfuncm::tripwire(game *g, monster *z)
{
 if (g->u.see(*z))
     messages.add("The %s trips over a tripwire!",
                  grammar::capitalize(z->desc(grammar::noun::role::subject, grammar::article::definite)).c_str());
 z->stumble(g, false);
 if (rng(0, 10) > z->type->sk_dodge && z->hurt(rng(1, 4))) g->kill_mon(*z);
}

void trapfunc::crossbow(game *g, int x, int y)
{
 bool add_bolt = true;
 messages.add("You trigger a crossbow trap!");
 if (!one_in(4) && rng(8, 20) > g->u.dodge()) {
  static constexpr const std::pair<body_part, int> bp_hit[] = { {bp_feet, 1}, {bp_legs, 3}, {bp_torso, 5}, {bp_head, 1} };
  const body_part hit = use_rarity_table(rng(0, force_consteval<rarity_table_nonstrict_ub(std::begin(bp_hit), std::end(bp_hit))>), std::begin(bp_hit), std::end(bp_hit));
  const int side = rng(0, 1);
  messages.add("Your %s is hit!", body_part_name(hit, side));
  g->u.hit(g, hit, side, 0, rng(20, 30));
  add_bolt = !one_in(10);
 } else messages.add("You dodge the shot!");
 trap_fully_triggered(g->m, point(x, y), trap::traps[tr_crossbow]->trigger_components);
 if (add_bolt) g->m.add_item(x, y, item::types[itm_bolt_steel], 0);
}
  
void trapfuncm::crossbow(game *g, monster *z)
{
 bool add_bolt = true;
 std::optional<std::string> z_name;
 const bool seen = g->u.see(*z);
 if (seen) z_name = z->desc(grammar::noun::role::direct_object, grammar::article::definite);
 if (!one_in(4)) {
  if (seen) messages.add("A bolt shoots out and hits %s!", z_name->c_str());
  if (z->hurt(rng(20, 30))) g->kill_mon(*z);
  add_bolt = !one_in(10);
 } else if (seen) messages.add("A bolt shoots out, but misses %s.", z_name->c_str());
 trap_fully_triggered(g->m, z->pos, trap::traps[tr_crossbow]->trigger_components);
 if (add_bolt) g->m.add_item(z->pos, item::types[itm_bolt_steel], 0);
}

void trapfunc::shotgun(game *g, int x, int y)
{
 messages.add("You trigger a shotgun trap!");
 auto& trap = g->m.tr_at(x, y);
 int shots = (tr_shotgun_1 != trap && (one_in(8) || one_in(20 - g->u.str_max))) ? 2 : 1;
 if (rng(5, 50) > g->u.dodge()) {
  static constexpr const std::pair<body_part, int> bp_hit[] = { {bp_feet, 1}, {bp_legs, 3}, {bp_torso, 5}, {bp_head, 1} };
  const body_part hit = use_rarity_table(rng(0, force_consteval<rarity_table_nonstrict_ub(std::begin(bp_hit), std::end(bp_hit))>), std::begin(bp_hit), std::end(bp_hit));
  const int side = rng(0, 1);
  messages.add("Your %s is hit!", body_part_name(hit, side));
  g->u.hit(g, hit, side, 0, rng(40 * shots, 60 * shots));
 } else
  messages.add("You dodge the shot!");
 if (shots == 2 || tr_shotgun_1 == trap) {
  // the two shotguns have the same configuration
  trap_fully_triggered(g->m, point(x, y), trap::traps[tr_shotgun_2]->trigger_components);
 } else trap = tr_shotgun_1;
}
 
void trapfuncm::shotgun(game *g, monster *z)
{
 const static int evade_double_shot[mtype::MS_MAX] = { 100, 16, 12, 8, 2 };	// corresponding typical PC strmax: sub-zero, 4, 8, 12, 18
 auto& trap = z->GPSpos.trap_at();
 int shots = (tr_shotgun_1 != trap && (one_in(8) || one_in(evade_double_shot[z->type->size]))) ? 2 : 1;
 if (g->u.see(*z))
   messages.add("A shotgun fires and hits %s!",
                z->desc(grammar::noun::role::direct_object, grammar::article::definite).c_str());
 if (z->hurt(rng(40 * shots, 60 * shots))) g->kill_mon(*z);
 if (shots == 2 || tr_shotgun_1 == trap) {
  // the two shotguns have the same configuration
  trap_fully_triggered(g->m, z->pos, trap::traps[tr_shotgun_2]->trigger_components);
 } else trap = tr_shotgun_1;
}


void trapfunc::blade(game *g, int x, int y)
{
 messages.add("A machete swings out and hacks your torso!");
 g->u.hit(g, bp_torso, 0, 12, 30);
}

void trapfuncm::blade(game *g, monster *z)
{
 if (g->u.see(*z))
     messages.add("A machete swings out and hacks %s!",
                  z->desc(grammar::noun::role::direct_object, grammar::article::definite).c_str());
 int cutdam = clamped_lb<0>(30 - z->armor_cut());
 int bashdam = clamped_lb<0>(12 - z->armor_bash());
 if (z->hurt(bashdam + cutdam)) g->kill_mon(*z);
}

void trapfunc::landmine(game *g, int x, int y)
{
 messages.add("You trigger a landmine!");
}

void trapfuncm::landmine(game *g, monster *z)
{
 if (g->u.see(z->pos)) messages.add("The %s steps on a landmine!", z->name().c_str());
}

void trapfunc::landmine(const point& pt)
{
    auto g = game::active();
    g->explosion(pt, 10, 8, false);
    g->m.tr_at(pt) = tr_null;
}

void trapfunc::boobytrap(game *g, int x, int y)
{
 messages.add("You trigger a boobytrap!");
}

void trapfuncm::boobytrap(game *g, monster *z)
{
 if (g->u.see(z->pos)) messages.add("The %s triggers a boobytrap!", z->name().c_str());
}

void trapfunc::boobytrap(const point& pt)
{
    auto g = game::active();
    g->explosion(pt, 18, 12, false);
    g->m.tr_at(pt) = tr_null;
}

void trapfunc::telepad(game *g, int x, int y)
{
 g->sound(point(x, y), 6, "vvrrrRRMM*POP!*");
 messages.add("The air shimmers around you...");
 g->teleport();
}

void trapfuncm::telepad(game *g, monster *z)
{
 g->sound(z->pos, 6, "vvrrrRRMM*POP!*");
 if (g->u.see(*z)) {
     messages.add("The air shimmers around %s...",
         z->desc(grammar::noun::role::direct_object, grammar::article::definite).c_str());
 }

 g->teleport_to(*z, g->teleport_destination_unsafe(z->pos, 10));
}

void trapfunc::goo(game *g, int x, int y)
{
 messages.add("You step in a puddle of thick goo.");
 g->u.infect(DI_SLIMED, bp_feet, 6, 20);
 if (one_in(3)) {
  messages.add("The acidic goo eats away at your feet.");
  g->u.hit(g, bp_feet, 0, 0, 5);
  g->u.hit(g, bp_feet, 1, 0, 5);
 }
 g->m.tr_at(x, y) = tr_null;
}

void trapfuncm::goo(game *g, monster *z)
{
    if (z->hit_by_blob()) z->GPSpos.trap_at() = tr_null;
}

void trapfunc::dissector(game *g, int x, int y)
{
 messages.add("Electrical beams emit from the floor and slice your flesh!");
 g->sound(point(x, y), 10, "BRZZZAP!");
 g->u.hit(g, bp_head,  0, 0, 15);
 g->u.hit(g, bp_torso, 0, 0, 20);
 g->u.hit(g, bp_arms,  0, 0, 12);
 g->u.hit(g, bp_arms,  1, 0, 12);
 g->u.hit(g, bp_hands, 0, 0, 10);
 g->u.hit(g, bp_hands, 1, 0, 10);
 g->u.hit(g, bp_legs,  0, 0, 12);
 g->u.hit(g, bp_legs,  1, 0, 12);
 g->u.hit(g, bp_feet,  0, 0, 10);
 g->u.hit(g, bp_feet,  1, 0, 10);
}

void trapfuncm::dissector(game *g, monster *z)
{
 g->sound(z->pos, 10, "BRZZZAP!");
 if (z->hurt(60)) g->explode_mon(*z);
}

void trapfunc::pit(game *g, int x, int y)
{
 messages.add("You fall in a pit!");
 if (g->u.has_trait(PF_WINGS_BIRD)) messages.add("You flap your wings and flutter down gracefully.");
 else {
  int dodge = g->u.dodge();
  int damage = rng(10, 20) - rng(dodge, dodge * 5);
  if (damage > 0) {
   messages.add("You hurt yourself!");
   g->u.hurtall(rng(damage/2, damage));
   g->u.hit(g, bp_legs, 0, damage, 0);
   g->u.hit(g, bp_legs, 1, damage, 0);
  } else
   messages.add("You land nimbly.");
 }
 g->u.add_disease(DI_IN_PIT, -1);
}

void trapfuncm::pit(game *g, monster *z)
{
 if (g->u.see(z->pos)) messages.add("The %s falls in a pit!", z->name().c_str());	// the trap is visible, even if the monster normally isn't
 if (z->hurt(rng(10, 20))) g->kill_mon(*z);
 else z->moves = -1000;
}

void trapfunc::pit_spikes(game *g, int x, int y)
{
 messages.add("You fall in a pit!");
 if (g->u.has_trait(PF_WINGS_BIRD)) messages.add("You flap your wings and flutter down gracefully.");
 else if (rng(5, 30) < g->u.dodge()) messages.add("You avoid the spikes within.");
 else {
  static constexpr const std::pair<body_part, int> bp_hit[] = { {bp_legs, 2}, {bp_arms, 2}, {bp_torso, 6} };
  const body_part hit = use_rarity_table(rng(0, force_consteval<rarity_table_nonstrict_ub(std::begin(bp_hit), std::end(bp_hit))>), std::begin(bp_hit), std::end(bp_hit));
  const int side = rng(0, 1);
  messages.add("The spikes impale your %s!", body_part_name(hit, side));
  g->u.hit(g, hit, side, 0, rng(20, 50));
  if (one_in(4)) {
   messages.add("The spears break!");
   trap_fully_triggered(g->m, point(x, y), trap::traps[tr_spike_pit]->trigger_components);
   // override trap_at effects, above
   g->m.ter(x, y) = t_pit;
   g->m.tr_at(x, y) = tr_pit;
  }
 }
 g->u.add_disease(DI_IN_PIT, -1);
}

void trapfuncm::pit_spikes(game *g, monster *z)
{
 const bool sees = g->u.see(*z);
 if (sees) messages.add("The %s falls in a spiked pit!", z->name().c_str());
 if (z->hurt(rng(20, 50))) g->kill_mon(*z);
 else z->moves -= 10 * mobile::mp_turn;

 if (one_in(4)) {
  if (sees) messages.add("The spears break!");
  trap_fully_triggered(g->m, z->pos, trap::traps[tr_spike_pit]->trigger_components);
  // override trap_at effects, above
  z->GPSpos.ter() = t_pit;
  z->GPSpos.trap_at() = tr_pit;
 }
}

void trapfunc::lava(game *g, int x, int y)
{
 messages.add("The %s burns you horribly!", name_of(g->m.ter(x, y)).c_str());
 g->u.hit(g, bp_feet, 0, 0, 20);
 g->u.hit(g, bp_feet, 1, 0, 20);
 g->u.hit(g, bp_legs, 0, 0, 20);
 g->u.hit(g, bp_legs, 1, 0, 20);
}

void trapfuncm::lava(game *g, monster *z)
{
 if (g->u.see(*z))
  messages.add("The %s burns the %s!", name_of(g->m.ter(z->pos)).c_str(), z->name().c_str());

 int dam = 30;
 if (z->made_of(FLESH)) dam = 50;
 if (z->made_of(VEGGY)) dam = 80;
 if (z->made_of(PAPER) || z->made_of(LIQUID) || z->made_of(POWDER) ||
     z->made_of(WOOD)  || z->made_of(COTTON) || z->made_of(WOOL))
  dam = 200;
 if (z->made_of(STONE)) dam = 15;
 if (z->made_of(KEVLAR) || z->made_of(STEEL)) dam = 5;

 if (z->hurt(dam)) g->kill_mon(*z);
}
 

void trapfunc::sinkhole(game *g, int x, int y)
{
    static std::function<std::optional<point>(point)> dest_ok = [&](point pt) {
        std::optional<point> ret;
        auto pos = g->u.pos + pt;
        if (0 < g->m.move_cost(pos) && tr_pit != g->m.tr_at(pos)) ret = pos;
        return ret;
    };

    static auto waste_rope = [&]() {
        g->u.use_amount(itm_rope_30, 1);
        g->m.add_item(g->u.pos + rng(within_rldist<1>), item::types[itm_rope_30], messages.turn);
    };

 messages.add("You step into a sinkhole, and start to sink down!");
 if (g->u.has_amount(itm_rope_30, 1) && query_yn("Throw your rope to try to catch soemthing?")) {
  const int throwroll = g->u.sklevel[sk_throw] + rng(0, g->u.str_cur + g->u.dex_cur);
  if (throwroll >= 12) {
   messages.add("The rope catches something!");
   if (6 < g->u.sklevel[sk_unarmed] + rng(0, g->u.str_cur)) {
       auto safe = grep(within_rldist<1>, dest_ok);

    if (safe.empty()) {
     messages.add("There's nowhere to pull yourself to, and you sink!");
     waste_rope();
     g->u.GPSpos.trap_at() = tr_pit;
     g->vertical_move(-1, true);
    } else {
     messages.add("You pull yourself to safety!  The sinkhole collapses.");
     int index = rng(0, safe.size() - 1);
     g->u.screenpos_set(safe[index]);
     g->update_map(g->u.pos.x, g->u.pos.y);
     g->u.GPSpos.trap_at() = tr_pit;
    }
   } else {
    messages.add("You're not strong enough to pull yourself out...");
    g->u.moves -= mobile::mp_turn;
    waste_rope();
    g->vertical_move(-1, true);
   }
  } else {
   messages.add("Your throw misses completely, and you sink!");
   if (one_in((g->u.str_cur + g->u.dex_cur) / 3)) waste_rope();
   g->u.GPSpos.trap_at() = tr_pit;
   g->vertical_move(-1, true);
  }
 } else {
  messages.add("You sink into the sinkhole!");
  g->vertical_move(-1, true);
 }
}

void trapfunc::ledge(game *g, int x, int y)
{
 messages.add("You fall down a level!");
 g->vertical_move(-1, true);
}

void trapfuncm::ledge(game *g, monster *z)
{
 messages.add("The %s falls down a level!", z->name().c_str());
 g->kill_mon(*z);
}

void trapfunc::temple_flood(game *g, int x, int y)
{
 messages.add("You step on a loose tile, and water starts to flood the room!");
 for (int i = 0; i < SEEX * MAPSIZE; i++) {
  for (int j = 0; j < SEEY * MAPSIZE; j++) {
   if (g->m.tr_at(i, j) == tr_temple_flood)
    g->m.tr_at(i, j) = tr_null;
  }
 }
 event::add(event(EVENT_TEMPLE_FLOOD, messages.turn + 3));
}

void trapfunc::temple_toggle(game *g, int x, int y)
{
 messages.add("You hear the grinding of shifting rock.");
 ter_id type = g->m.ter(x, y);
 for (int i = 0; i < SEEX * MAPSIZE; i++) {
  for (int j = 0; j < SEEY * MAPSIZE; j++) {
   auto& t = g->m.ter(i, j);
   switch (type) {
    case t_floor_red:
     if (t_rock_green == t) t = t_floor_green;
     else if (t_floor_green == t) t = t_rock_green;
     break;
    case t_floor_green:
     if (t_rock_blue == t) t = t_floor_blue;
     else if (t_floor_blue == t) t = t_rock_blue;
     break;
    case t_floor_blue:
     if (t_rock_red == t) t = t_floor_red;
     else if (t_floor_red == t) t = t_rock_red;
     break;
   }
  }
 }
}

void trapfunc::glow(game *g, int x, int y)
{
 if (one_in(3)) {
  messages.add("You're bathed in radiation!");
  g->u.radiation += rng(10, 30);
 } else if (one_in(4)) {
  messages.add("A blinding flash strikes you!");
  g->flashbang(g->u.pos);
 } else
  messages.add("Small flashes surround you.");
}

void trapfuncm::glow(game *g, monster *z)
{
 if (one_in(3)) {
  if (z->hurt( rng(5, 10) )) g->kill_mon(*z);
  else z->speed *= .9;
 }
}

static const char* desc_hum(int volume)
{
    if (10 >= volume) return "hrm";
    else if (50 >= volume) return "hrmmm";
    else if (100 >= volume) return "HRMMM";
    else return "VRMMMMMM";
}

void trapfunc::hum(const point& pt)
{
    const int volume = rng(1, 200);
    game::active()->sound(pt, volume, desc_hum(volume));
}

// cf. AEA_SHADOWS
void trapfunc::shadow(game *g, int x, int y)
{
    static std::function<point()> candidate = [&]() {
        if (one_in(2)) {
            return point(rng(g->u.pos.x - 5, g->u.pos.x + 5), one_in(2) ? g->u.pos.y - 5 : g->u.pos.y + 5);
        } else {
            return point(one_in(2) ? g->u.pos.x - 5 : g->u.pos.x + 5, rng(g->u.pos.y - 5, g->u.pos.y + 5));
        }
    };

    static std::function<bool(const point&)> ok = [&](const point& pt) {
        return g->is_empty(pt) && g->m.sees(pt, g->u.pos, 10);
    };

    if (auto pt = LasVegasChoice(5, candidate, ok)) {
        messages.add("A shadow forms nearby.");
        monster spawned(mtype::types[mon_shadow], *pt);
        spawned.sp_timeout = rng(2, 10);
        g->z.push_back(spawned);
        g->m.tr_at(x, y) = tr_null;
    }
}

void trapfunc::drain(game *g, int x, int y)
{
 messages.add("You feel your life force sapping away.");
 g->u.hurtall(1);
}

void trapfuncm::drain(game *g, monster *z)
{
 if (z->hurt(1)) g->kill_mon(*z);
}

void trapfunc::snake(game *g, int x, int y)
{
 if (one_in(3)) {
     static std::function<point()> nominate_spawn_pos = [&]() {
         point ret;
         if (one_in(2)) {
             ret.x = rng(-5, 5);
             ret.y = (one_in(2) ? -5 : 5);
         } else {
             ret.x = (one_in(2) ? -5 : 5);
             ret.y = rng(-5, 5);
         };
         return g->u.pos + ret;
     };

     static std::function<bool(const point&)> spawn_ok = [&](const point& pt) { return g->is_empty(pt) && g->m.sees(pt, g->u.pos, 10); };

     if (const auto spawn = LasVegasChoice(5, nominate_spawn_pos, spawn_ok)) {
         messages.add("A shadowy snake forms nearby.");
         g->z.push_back(monster(mtype::types[mon_shadow_snake], *spawn));
         g->m.tr_at(*spawn) = tr_null;
         return;
     }
 }
}

void trapfunc::snake(const point& pt)
{
    auto g = game::active();
    g->sound(pt, 10, "ssssssss");
    if (one_in(6)) g->m.tr_at(pt) = tr_null;
}
