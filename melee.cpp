#include "player.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "recent_msg.h"
#include "stl_limits.h"

#include <sstream>
#include <stdlib.h>

int  stumble(player &u);
std::string melee_verb(technique_id tech, std::string your, player &p,
                       int bash_dam, int cut_dam, int stab_dam);

/* Melee Functions!
 * These all belong to class player.
 *
 * STATE QUERIES
 * bool is_armed() - True if we are armed with any weapon.
 * bool unarmed_attack() - True if we are NOT armed with any weapon, but still
 *  true if we're wielding a bionic weapon (at this point, just itm_bio_claws).
 *
 * HIT DETERMINATION
 * int base_to_hit() - The base number of sides we get in hit_roll().
 *                     Dexterity / 2 + sk_melee
 * int hit_roll() - The player's hit roll, to be compared to a monster's or
 *   player's dodge_roll().  This handles weapon bonuses, weapon-specific
 *   skills, torso encumberment penalties and drunken master bonuses.
 */

bool player::is_armed() const
{
 return (weapon.type->id != 0 && !weapon.is_style());
}

bool player::unarmed_attack() const
{   // 2021-12-08 all styles have the IF_UNARMED_WEAPON set in their constructor
 return (weapon.type->id == 0 || /* weapon.is_style() || */
         weapon.has_flag(IF_UNARMED_WEAPON));
}

int player::base_to_hit(bool real_life, int stat) const
{
 if (stat == -999) stat = (real_life ? dex_cur : dex_max);
 return 1 + int(stat / 2) + sklevel[sk_melee];
}

int player::hit_roll() const
{
 int stat = dex_cur;
// Some martial arts use something else to determine hits!
 switch (weapon.type->id) {
  case itm_style_tiger:
   stat = (str_cur * 2 + dex_cur) / 3;
   break;
  case itm_style_leopard:
   stat = (per_cur + int_cur + dex_cur * 2) / 4;
   break;
  case itm_style_snake:
   stat = (per_cur + dex_cur) / 2;
   break;
 }
 int numdice = base_to_hit(stat) + weapon.type->m_to_hit + disease_intensity(DI_ATTACK_BOOST);
 int sides = 10 - encumb(bp_torso);
 int best_bonus = 0;
 if (sides < 2) sides = 2;

// Are we unarmed?
 if (unarmed_attack()) {
  best_bonus = sklevel[sk_unarmed];
  if (sklevel[sk_unarmed] > 4) best_bonus += sklevel[sk_unarmed] - 4; // Extra bonus for high levels
 }

// Using a bashing weapon?
 if (weapon.is_bashing_weapon()) {
  int bash_bonus = int(sklevel[sk_bashing] / 3);
  if (bash_bonus > best_bonus) best_bonus = bash_bonus;
 }

// Using a cutting weapon?
 if (weapon.is_cutting_weapon()) {
  int cut_bonus = int(sklevel[sk_cutting] / 2);
  if (cut_bonus > best_bonus) best_bonus = cut_bonus;
 }

// Using a spear?
 if (weapon.has_flag(IF_SPEAR) || weapon.has_flag(IF_STAB)) {
  int stab_bonus = int(sklevel[sk_stabbing] / 2);
  if (stab_bonus > best_bonus) best_bonus = stab_bonus;
 }

 numdice += best_bonus; // Use whichever bonus is best.

// Drunken master makes us hit better
 if (has_trait(PF_DRUNKEN)) {
     if (const auto drunk = disease_level(DI_DRUNK)) {
         if (unarmed_attack())
             numdice += drunk / MINUTES(30);
         else
             numdice += drunk / MINUTES(40);
     }
 }

 if (numdice < 1) {
  numdice = 1;
  sides = 8 - encumb(bp_torso);	// can result in sides < 1...problematic
 }

 return dice(numdice, sides);
}

static void hit_message(std::string subject, std::string verb, std::string target, int dam, bool crit)
{
  if (dam <= 0)
	messages.add("%s %s %s but do%s no damage.", subject.c_str(), verb.c_str(),
			target.c_str(), (subject == "You" ? "" : "es"));
  else
	messages.add("%s%s %s %s for %d damage.", (crit ? "Critical! " : ""),
			subject.c_str(), verb.c_str(), target.c_str(), dam);
}

static int attack_speed(const player& u, bool missed)
{
    int move_cost = u.weapon.attack_time() + 20 * u.encumb(bp_torso);
    if (auto weak = u.has_light_bones()) move_cost *= mutation_branch::light_bones_attack_tus[weak - 1];
    return clamped_lb<25>(move_cost - u.disease_intensity(DI_SPEED_BOOST));
}

// could micro-optimize based on call graph, but this is not CPU-intensive
static void melee_practice(player& u, bool hit, bool unarmed, bool bashing, bool cutting, bool stabbing)
{
    if (!hit) {
        u.practice(sk_melee, rng(2, 5));
        if (unarmed) u.practice(sk_unarmed, 2);
        if (bashing) u.practice(sk_bashing, 2);
        if (cutting) u.practice(sk_cutting, 2);
        if (stabbing) u.practice(sk_stabbing, 2);
    } else {
        u.practice(sk_melee, rng(5, 10));
        if (unarmed) u.practice(sk_unarmed, rng(5, 10));
        if (bashing) u.practice(sk_bashing, rng(5, 10));
        if (cutting) u.practice(sk_cutting, rng(5, 10));
        if (stabbing) u.practice(sk_stabbing, rng(5, 10));
    }
}

int player::hit_mon(game *g, monster *z, bool allow_grab) // defaults to true
{
 bool is_u = (this == &(g->u));	// Affects how we'll display messages
 if (is_u)
  z->add_effect(ME_HIT_BY_PLAYER, 100); // Flag as attacked by us
 const bool can_see = (is_u || g->u.see(pos));	// XXX this non-use suggests this function is never called by npcs

 std::string You  = (is_u ? "You"  : name);
 std::string Your = (is_u ? "Your" : name + "'s");
 std::string your = (is_u ? "your" : (male ? "his" : "her"));
 std::string verb = "hit";
 std::string target = "the " + z->name();

// If !allow_grab, then we already grabbed them--meaning their dodge is hampered
 int mondodge = (allow_grab ? z->dodge_roll() : z->dodge_roll() / 3);

 bool missed = (hit_roll() < mondodge ||
                one_in(4 + dex_cur + weapon.type->m_to_hit));

 int move_cost = attack_speed(*this, missed);

 if (missed) {
  int stumble_pen = stumble(*this);
  if (is_u) {	// Only display messages if this is the player
   if (weapon.has_technique(TEC_FEINT, this))
    messages.add("You feint.");
   else if (stumble_pen >= 60)
    messages.add("You miss and stumble with the momentum.");
   else if (stumble_pen >= 10)
    messages.add("You swing wildly and miss.");
   else
    messages.add("You miss.");
  }
  melee_practice(*this, false, unarmed_attack(),
                 weapon.is_bashing_weapon(), weapon.is_cutting_weapon(),
                 (weapon.has_flag(IF_SPEAR) || weapon.has_flag(IF_STAB)));
  move_cost += stumble_pen;
  if (weapon.has_technique(TEC_FEINT, this)) move_cost = rng(move_cost / 3, move_cost);
  moves -= move_cost;
  return 0;
 }
 moves -= move_cost;

 bool critical_hit = scored_crit(mondodge);

 int bash_dam = roll_bash_damage(z, critical_hit);
 int cut_dam  = roll_cut_damage(z, critical_hit);
 int stab_dam = roll_stab_damage(z, critical_hit);

 int pain = 0; // Boost to pain; required for perform_technique

// Moves lost to getting your weapon stuck
 const int stuck_penalty = weapon.is_style() ? 0 : roll_stuck_penalty(z, (stab_dam >= cut_dam));

// Pick one or more special attacks
 technique_id technique = pick_technique(g, z, nullptr, critical_hit, allow_grab);

// Handles effects as well; not done in melee_affect_*
 perform_technique(technique, g, z, nullptr, bash_dam, cut_dam, stab_dam, pain);
 z->speed -= int(pain / 2);

// Mutation-based attacks
 perform_special_attacks(g, z, nullptr, bash_dam, cut_dam, stab_dam);

// Handles speed penalties to monster & us, etc
 melee_special_effects(g, z, nullptr, critical_hit, bash_dam, cut_dam, stab_dam);

// Make a rather quiet sound, to alert any nearby monsters
 if (weapon.type->id != itm_style_ninjutsu) g->sound(pos, 8, ""); // Ninjutsu is silent!

 verb = melee_verb(technique, your, *this, bash_dam, cut_dam, stab_dam);

 int dam = bash_dam + (cut_dam > stab_dam ? cut_dam : stab_dam);

 hit_message(You.c_str(), verb.c_str(), target.c_str(), dam, critical_hit);

 bool bashing = (bash_dam >= 10 && !unarmed_attack());
 bool cutting = (cut_dam >= 10 && cut_dam >= stab_dam);
 bool stabbing = (stab_dam >= 10 && stab_dam >= cut_dam);
 melee_practice(*this, true, unarmed_attack(), bashing, cutting, stabbing);

 if (allow_grab && technique == TEC_GRAB) {
// Move our weapon to a temp slot, if it's not unarmed
  if (!unarmed_attack()) {
   item tmpweap = unwield();
   dam += hit_mon(g, z, false); // False means a second grab isn't allowed
   weapon = std::move(tmpweap);
  } else
   dam += hit_mon(g, z, false); // False means a second grab isn't allowed
 }

 if (dam >= 5 && has_artifact_with(AEP_SAP_LIFE)) healall( rng(dam / 10, dam / 5) );
 return dam;
}

void player::hit_player(game *g, player &p, bool allow_grab)
{
 const bool is_u = (this == &(g->u));	// Affects how we'll display messages
 const bool can_see = (is_u || g->u.see(pos));
 if (is_u && p.is_npc()) dynamic_cast<npc&>(p).make_angry();

 std::string You  = (is_u ? "You"  : name);
 std::string Your = (is_u ? "Your" : name + "'s");
 std::string your = (is_u ? "your" : (male ? "his" : "her"));
 std::string verb = "hit";

// Divide their dodge roll by 2 if this is a grab
 int target_dodge = p.dodge_roll()/(allow_grab ? 1 : 2);
 int hit_value = hit_roll() - target_dodge;
 bool missed = (hit_roll() <= 0);

 int move_cost = attack_speed(*this, missed);

 if (missed) {
  int stumble_pen = stumble(*this);
  if (is_u) {	// Only display messages if this is the player
   if (weapon.has_technique(TEC_FEINT, this))
    messages.add("You feint.");
   else if (stumble_pen >= 60)
    messages.add("You miss and stumble with the momentum.");
   else if (stumble_pen >= 10)
    messages.add("You swing wildly and miss.");
   else
    messages.add("You miss.");
  }
  melee_practice(*this, false, unarmed_attack(),
                 weapon.is_bashing_weapon(), weapon.is_cutting_weapon(),
                 (weapon.has_flag(IF_SPEAR) || weapon.has_flag(IF_STAB)));
  move_cost += stumble_pen;
  if (weapon.has_technique(TEC_FEINT, this))
   move_cost = rng(move_cost / 3, move_cost);
  moves -= move_cost;
  return;
 }
 moves -= move_cost;

 body_part bp_hit;
 int side = rng(0, 1);
 hit_value += rng(-10, 10);
 if (hit_value >= 30)
  bp_hit = bp_eyes;
 else if (hit_value >= 20)
  bp_hit = bp_head;
 else if (hit_value >= 10)
  bp_hit = bp_torso;
 else if (one_in(4))
  bp_hit = bp_legs;
 else
  bp_hit = bp_arms;

 std::string target = (p.is_npc() ? p.name + "'s " : "your ");
 target += body_part_name(bp_hit, side);

 bool critical_hit = scored_crit(target_dodge);

 int bash_dam = roll_bash_damage(nullptr, critical_hit);
 int cut_dam  = roll_cut_damage(nullptr, critical_hit);
 int stab_dam = roll_stab_damage(nullptr, critical_hit);

 technique_id tech_def = p.pick_defensive_technique(g, nullptr, this);
 p.perform_defensive_technique(tech_def, g, nullptr, this, bp_hit, side,
                               bash_dam, cut_dam, stab_dam);

 if (bash_dam + cut_dam + stab_dam <= 0)
  return; // Defensive technique canceled our attack!

 if (critical_hit) // Crits cancel out Toad Style's armor boost
  p.rem_disease(DI_ARMOR_BOOST);

 int pain = 0; // Boost to pain; required for perform_technique

// Moves lost to getting your weapon stuck
 const int stuck_penalty = weapon.is_style() ? 0 : roll_stuck_penalty(nullptr, (stab_dam >= cut_dam));

// Pick one or more special attacks
 technique_id technique = pick_technique(g, nullptr, &p, critical_hit, allow_grab);

// Handles effects as well; not done in melee_affect_*
 perform_technique(technique, g, nullptr, &p, bash_dam, cut_dam, stab_dam, pain);
 p.pain += pain;

// Mutation-based attacks
 perform_special_attacks(g, nullptr, &p, bash_dam, cut_dam, stab_dam);

// Handles speed penalties to monster & us, etc
 melee_special_effects(g, nullptr, &p, critical_hit, bash_dam, cut_dam, stab_dam);

// Make a rather quiet sound, to alert any nearby monsters
 if (weapon.type->id != itm_style_ninjutsu) // Ninjutsu is silent!
  g->sound(pos, 8, "");

 p.hit(g, bp_hit, side, bash_dam, (cut_dam > stab_dam ? cut_dam : stab_dam));

 verb = melee_verb(technique, your, *this, bash_dam, cut_dam, stab_dam);
 int dam = bash_dam + (cut_dam > stab_dam ? cut_dam : stab_dam);
 hit_message(You.c_str(), verb.c_str(), target.c_str(), dam, critical_hit);

 bool bashing = (bash_dam >= 10 && !unarmed_attack());
 bool cutting = (cut_dam >= 10 && cut_dam >= stab_dam);
 bool stabbing = (stab_dam >= 10 && stab_dam >= cut_dam);
 melee_practice(*this, true, unarmed_attack(), bashing, cutting, stabbing);

 if (dam >= 5 && has_artifact_with(AEP_SAP_LIFE)) healall( rng(dam / 10, dam / 5) );

 if (allow_grab && technique == TEC_GRAB) {
// Move our weapon to a temp slot, if it's not unarmed
  if (p.weapon.has_technique(TEC_BREAK, &p) &&
      dice(p.melee_skill(), 12) > dice(melee_skill(), 10)) {
   if (is_u)
    messages.add("%s %s the grab!", target.c_str(), p.regular_verb_agreement("break").c_str());
  } else if (!unarmed_attack()) {
   item tmpweap = unwield();
   hit_player(g, p, false); // False means a second grab isn't allowed
   weapon = std::move(tmpweap);
  } else
   hit_player(g, p, false); // False means a second grab isn't allowed
 }
 if (tech_def == TEC_COUNTER) {
  if (!p.is_npc()) messages.add("Counter-attack!");
  p.hit_player(g, *this);
 }
}

int stumble(player &u)
{
 int stumble_pen = 2 * u.weapon.volume() + u.weapon.weight();
 if (u.has_trait(PF_DEFT)) stumble_pen = int(stumble_pen * .3) - 10;
 if (stumble_pen < 0) stumble_pen = 0;
// TODO: Reflect high strength bonus in newcharacter.cpp
 if (stumble_pen > 0 && (u.str_cur >= 15 || u.dex_cur >= 21 ||
                         one_in(16 - u.str_cur) || one_in(22 - u.dex_cur)))
  stumble_pen = rng(0, stumble_pen);

 return stumble_pen;
}

bool player::scored_crit(int target_dodge) const
{
 bool to_hit_crit = false, dex_crit = false, skill_crit = false;
 int num_crits = 0;

// Weapon to-hit roll
 int chance = 25;
 if (unarmed_attack()) { // Unarmed attack: 1/2 of unarmed skill is to-hit
  for (int i = 1; i <= int(sklevel[sk_unarmed] * .5); i++)
   chance += (50 / (2 + i));
 }
 if (weapon.type->m_to_hit > 0) {
  for (int i = 1; i <= weapon.type->m_to_hit; i++)
   chance += (50 / (2 + i));
 } else if (chance < 0) {
  for (int i = 0; i > weapon.type->m_to_hit; i--)
   chance /= 2;
 }
 if (rng(0, 99) < chance + 4 * disease_intensity(DI_ATTACK_BOOST)) num_crits++;

// Dexterity to-hit roll
// ... except sometimes we don't use dexterity!
 int stat = dex_cur;
// Some martial arts use something else to determine hits!
 switch (weapon.type->id) {
  case itm_style_tiger:
   stat = (str_cur * 2 + dex_cur) / 3;
   break;
  case itm_style_leopard:
   stat = (per_cur + int_cur + dex_cur * 2) / 4;
   break;
  case itm_style_snake:
   stat = (per_cur + dex_cur) / 2;
   break;
 }
 chance = 25;
 if (stat > 8) {
  for (int i = 9; i <= stat; i++)	// max design stat 20 (pathological behavior above this)
   chance += (21 - i); // 12, 11, 10...
 } else {
  int decrease = 5;
  for (int i = 7; i >= stat; i--) {
   chance -= decrease;
   if (i % 2 == 0) decrease--;
  }
 }
 if (rng(0, 99) < chance) num_crits++;

// Skill level roll
 int best_skill = 0;

 if (weapon.is_bashing_weapon() && sklevel[sk_bashing] > best_skill) best_skill = sklevel[sk_bashing];
 if (weapon.is_cutting_weapon() && sklevel[sk_cutting] > best_skill) best_skill = sklevel[sk_cutting];
 if ((weapon.has_flag(IF_SPEAR) || weapon.has_flag(IF_STAB)) && sklevel[sk_stabbing] > best_skill) best_skill = sklevel[sk_stabbing];
 if (unarmed_attack() && sklevel[sk_unarmed] > best_skill) best_skill = sklevel[sk_unarmed];

 best_skill += int(sklevel[sk_melee] / 2.5);

 chance = 25;
 if (best_skill > 3) {
  for (int i = 3; i < best_skill; i++)
   chance += (50 / (2 + i));
 } else if (chance < 3) {
  for (int i = 3; i > best_skill; i--)
   chance /= 2;
 }
 if (rng(0, 99) < chance + 4 * disease_intensity(DI_ATTACK_BOOST)) num_crits++;

 if (num_crits == 3) return true;
 else if (num_crits == 2) return (hit_roll() >= target_dodge * 1.5 && !one_in(4));
 return false;
}

int player::dodge() const
{
 if (has_disease(DI_SLEEP) || has_disease(DI_LYING_DOWN)) return 0;
 if (activity.type != ACT_NULL) return 0;
 int ret = 4 + (dex_cur / 2);
 ret += sklevel[sk_dodge];
 ret += disease_intensity(DI_DODGE_BOOST);
 ret -= (encumb(bp_legs) / 2) + encumb(bp_torso);
 ret += current_speed() / ((mobile::mp_turn/2)*3);
 if (has_trait(PF_TAIL_LONG)) ret += 4;
 if (has_trait(PF_TAIL_FLUFFY)) ret += 8;
 if (has_trait(PF_WHISKERS)) ret += 1;
 if (has_trait(PF_WINGS_BAT)) ret -= 3;
 if (str_max >= 16) ret--; // Penalty if we're huge
 else if (str_max <= 5) ret++; // Bonus if we're small
 if (dodges_left <= 0) { // We already dodged this turn
  if (rng(1, sklevel[sk_dodge] + dex_cur + 15) <= sklevel[sk_dodge] + dex_cur)
   ret = rng(0, ret);
  else
   ret = 0;
 }
 dodges_left--;
// If we're over our cap, average it with our cap
 const int cap = dex_cur / 2 + sklevel[sk_dodge] * 2;
 if (ret > cap) ret = ( ret + cap) / 2;
 return ret;
}

int player::dodge_roll() const { return dice(dodge(), 6); }

int player::base_damage(bool real_life, int stat) const
{
 if (stat == -999)
  stat = (real_life ? str_cur : str_max);
 int dam = (real_life ? rng(0, stat / 2) : stat / 2);
// Bonus for statong characters
 if (stat > 10)
  dam += int((stat - 9) / 2);
// Big bonus for super-human characters
 if (stat > 20)
  dam += int((stat - 20) * 1.5);

 return dam;
}

int player::roll_bash_damage(const monster *z, bool crit) const
{
 int ret = 0;
 int stat = str_cur; // Which stat determines damage?
 int skill = sklevel[unarmed_attack() ? sk_unarmed : sk_bashing];
 
 switch (weapon.type->id) { // Some martial arts change which stat
  case itm_style_crane:
   stat = (dex_cur * 2 + str_cur) / 3;
   break;
  case itm_style_snake:
   stat = int(str_cur + per_cur) / 2;
   break;
  case itm_style_dragon:
   stat = int(str_cur + int_cur) / 2;
   break;
 }

 ret = base_damage(true, stat);

// Drunken Master damage bonuses
 if (has_trait(PF_DRUNKEN)) {
     if (const auto drunk = disease_level(DI_DRUNK)) {  // zero if doesn't have this "disease"
        // Remember, a single drink gives 1 hour's levels of DI_DRUNK
         int mindrunk, maxdrunk;
         if (unarmed_attack()) {
             mindrunk = drunk / MINUTES(60);
             maxdrunk = drunk / MINUTES(25);
         } else {
             mindrunk = drunk / MINUTES(90);
             maxdrunk = drunk / MINUTES(40);
         }
         ret += rng(mindrunk, maxdrunk);
     }
 }

 int bash_dam = int(stat / 2) + weapon.damage_bash(),
     bash_cap = 5 + stat + skill;

 if (unarmed_attack())
  bash_dam = rng(0, int(stat / 2) + sklevel[sk_unarmed]);

 if (crit) {
  cataclysm::rational_scale<3,2>(bash_dam);
  bash_cap *= 2;
 }

 if (bash_dam > bash_cap)// Cap for weak characters
  bash_dam = (bash_cap * 3 + bash_dam) / 4;

 if (is<MF_PLASTIC>(z)) bash_dam /= rng(2, 4);

 int bash_min = bash_dam / 4;

 bash_dam = rng(bash_min, bash_dam);
 
 if (bash_dam < skill + int(stat / 2))
  bash_dam = rng(bash_dam, skill + int(stat / 2));

 ret += bash_dam;

 ret += disease_intensity(DI_DAMAGE_BOOST);

// Finally, extra crit effects
 if (crit) {
  ret += int(stat / 2);
  ret += skill;
 }
 if (z) ret -= z->armor_bash();

 return (ret < 0 ? 0 : ret);
}

int player::roll_cut_damage(const monster *z, bool crit) const
{
 if (weapon.has_flag(IF_SPEAR)) return 0;  // Stabs, doesn't cut!
 int z_armor_cut = z ? z->armor_cut() - sklevel[sk_cutting] / 2 : 0;

 if (crit) z_armor_cut /= 2;
 if (z_armor_cut < 0) z_armor_cut = 0;

 int ret = weapon.damage_cut() - z_armor_cut;

 if (unarmed_attack() && !wearing_something_on(bp_hands)) {
  if (has_trait(PF_CLAWS)) ret += 6;
  if (has_trait(PF_TALONS)) ret += 6 + (sklevel[sk_unarmed] > 8 ? 8 : sklevel[sk_unarmed]);
  if (has_trait(PF_SLIME_HANDS) && !is<MF_ACIDPROOF>(z)) ret += rng(4, 6);
 }

 if (ret <= 0) return 0; // No negative damage!

// 80%, 88%, 96%, 104%, 112%, 116%, 120%, 124%, 128%, 132%
 if (sklevel[sk_cutting] <= 5)
  ret += ret * (2 * sklevel[sk_cutting] - 5) / 25;
 else
  ret += ret * (sklevel[sk_cutting] - 2) / 25;

 if (crit) ret += ret * sklevel[sk_cutting] / 12;

 return ret;
}

int player::roll_stab_damage(const monster *z, bool crit) const
{
 int ret = 0;
 int z_armor = (z == nullptr ? 0 : z->armor_cut() - 3 * sklevel[sk_stabbing]);

 if (crit) z_armor /= 3;
 if (z_armor < 0) z_armor = 0;

 if (unarmed_attack() && !wearing_something_on(bp_hands)) {
  ret = 0 - z_armor;
  if (has_trait(PF_CLAWS)) ret += 6;
  if (has_trait(PF_NAILS) && z_armor == 0) ret++;
  if (has_trait(PF_THORNS)) ret += 4;
 } else if (weapon.has_flag(IF_SPEAR) || weapon.has_flag(IF_STAB))
  ret = int((weapon.damage_cut() - z_armor) / 4);
 else
  return 0; // Can't stab at all!

 if (z != nullptr && z->speed > 100) { // Bonus against fast monsters
  int speed_min = (z->speed - 100) / 10, speed_max = (z->speed - 100) / 5;
  int speed_dam = rng(speed_min, speed_max);
  if (speed_dam > ret * 2) speed_dam = ret * 2;
  if (speed_dam > 0) ret += speed_dam;
 }

 if (ret <= 0) return 0; // No negative stabbing!

 if (crit) {
  int numerator = 10 + 2 * sklevel[sk_stabbing];
  if (25 < numerator) numerator = 25;
  ret += (ret*(numerator - 10)) / 10;
 }

 return ret;
}

int player::roll_stuck_penalty(const monster *z, bool stabbing) const
{
 int ret = 0;
 const int basharm = (z ? z->armor_bash() : 6);
 const int cutarm  = (z ? z->armor_cut() : 6);
 const int cutdam = weapon.damage_cut();
 if (stabbing)
  ret = cutdam * 3 + basharm * 3 + cutarm * 3 - dice(sklevel[sk_stabbing], 10);
 else
  ret = cutdam * 4 + basharm * 5 + cutarm * 4 - dice(sklevel[sk_cutting], 10);

 if (ret >= cutdam * 10) return cutdam * 10;
 return (ret < 0 ? 0 : ret);
}

technique_id player::pick_technique(const game *g, const monster *z, const player *p, bool crit, bool allowgrab) const
{
 if (z == nullptr && p == nullptr) return TEC_NULL; // \todo error condition?
 const mobile* const mob = z ? static_cast<const mobile*>(z) : p;
 const bool downed = mob->has(effect::DOWNED);
 const int base_str_req = mob->min_technique_power();

 std::vector<technique_id> possible;

 if (allowgrab) { // Check if grabs AREN'T REALLY ALLOWED
  if (is<MF_PLASTIC>(z)) allowgrab = false;
 }

 if (crit) { // Some are crit-only

  if (weapon.has_technique(TEC_SWEEP, this) && !is<MF_FLIES>(z) && !downed)
   possible.push_back(TEC_SWEEP);

  if (weapon.has_technique(TEC_PRECISE, this)) possible.push_back(TEC_PRECISE);

  if (weapon.has_technique(TEC_BRUTAL, this) && !downed &&
      str_cur + sklevel[sk_melee] >= 4 + base_str_req)
   possible.push_back(TEC_BRUTAL);

 }

 if (possible.empty()) { // Use non-crits only if any crit-onlies aren't used

  if (weapon.has_technique(TEC_DISARM, this) && !z && !p->unarmed_attack() &&
      dice(dex_cur + sklevel[sk_unarmed], 8) > dice(p->melee_skill(), 10))
   possible.push_back(TEC_DISARM);

  if (weapon.has_technique(TEC_GRAB, this) && allowgrab) possible.push_back(TEC_GRAB);
  if (weapon.has_technique(TEC_RAPID, this)) possible.push_back(TEC_RAPID);

  if (weapon.has_technique(TEC_THROW, this) && !downed &&
      str_cur + sklevel[sk_melee] >= 4 + base_str_req * 4 + rng(-4, 4))
   possible.push_back(TEC_THROW);

  if (weapon.has_technique(TEC_WIDE, this)) { // Count monsters
   int enemy_count = 0;
   for (decltype(auto) delta : Direction::vector) {
       const point pt(pos + delta);
       // XXX faction-unaware
       if (const monster* const m_at = g->mon(pt)) {
           if (m_at->is_enemy(this)) enemy_count++;
           else enemy_count -= 2;
       }
       if (const npc* const nPC = g->nPC(pt)) {
           if (nPC->attitude == NPCATT_KILL) enemy_count++;
           else enemy_count -= 2;
       } // XXX \todo handle npc wide technique vs player
   }
   if (enemy_count >= (possible.empty() ? 2 : 3)) possible.push_back(TEC_WIDE);
  }

 } // if (possible.empty())
  
 if (possible.empty()) return TEC_NULL;

 possible.push_back(TEC_NULL); // Always a chance to not use any technique

 return possible[rng(0, possible.size() - 1)];
}

void player::perform_technique(technique_id technique, game *g, monster *z,
                               player *p, int &bash_dam, int &cut_dam,
                               int &stab_dam, int &pain)
{
 assert(z || p);

 // self-affecting techniques
 switch (technique) {
 case TEC_RAPID:
     moves += attack_speed(*this, false) / 2;
     return;
 case TEC_BLOCK:
     cataclysm::rational_scale<7, 10>(bash_dam);
     return;
 }

 mobile* const mob = z ? static_cast<mobile*>(z) : p;
 const std::string You = desc(grammar::noun::role::subject);
 const std::string target = mob->desc(grammar::noun::role::direct_object, grammar::article::definite);

 const bool u_see = (!is_npc() || g->u.see(pos));

// The rest affect our target, and thus depend on z vs. p
 switch (technique) {

 case TEC_SWEEP:
  if (is_not<MF_FLIES>(z)) {
   z->add_effect(ME_DOWNED, rng(1, 2));
   bash_dam += z->fall_damage();
  } else if (p != nullptr && p->weapon.type->id != itm_style_judo) {
   p->add_disease(DI_DOWNED, rng(1, 2));
   bash_dam += 3;
  }
  break;

 case TEC_PRECISE:
  mob->add(effect::STUNNED, rng(1, 4));
  pain += rng(5, 8);
  break;

 case TEC_BRUTAL:
  mob->add(effect::STUNNED, 1);
  mob->knockback_from(GPSpos);
  break;

 case TEC_THROW:
// Throws are less predictable than brutal strikes.
// We knock them back from a tile adjacent to us!
  mob->knockback_from(GPSpos + rng(within_rldist<1>));
  if (z != nullptr) {
   z->add_effect(ME_DOWNED, rng(1, 2));
  } else if (p != nullptr) {
   if (p->weapon.type->id != itm_style_judo)
    p->add_disease(DI_DOWNED, rng(1, 2));
  }
  break;

 case TEC_WIDE: {
  int count_hit = 0;
  const auto tar = mob->GPSpos;
  for (int dir = direction::NORTH; dir <= direction::NORTHWEST; ++dir) {
      const auto test = tar + direction_vector((direction)dir);
      if (test == GPSpos) continue; // Don't self-hit
      if (const auto m_at = g->mon(test)) {
          if (hit_roll() >= rng(0, 5) + m_at->dodge_roll()) {
              count_hit++;
              int dam = roll_bash_damage(m_at, false) + roll_cut_damage(m_at, false);
              m_at->hurt(dam);
              if (u_see) messages.add("%s %s %s for %d damage!", You.c_str(), regular_verb_agreement("hit").c_str(), m_at->desc(grammar::noun::role::direct_object, grammar::article::definite).c_str(), dam);
          }
      }
      if (const auto nPC = g->nPC(test)) {
          if (hit_roll() >= rng(0, 5) + nPC->dodge_roll()) {
              count_hit++;
              int dam = roll_bash_damage(nullptr, false);	// looks like (n)PC armor won't work, but covered in the hit function
              int cut = roll_cut_damage(nullptr, false);
              nPC->hit(g, bp_legs, 3, dam, cut);
              if (u_see) messages.add("%s %s %s for %d damage!", You.c_str(), regular_verb_agreement("hit").c_str(), nPC->name.c_str(), dam + cut);
          }
      }
      // XXX \todo FIX: genuine players immune to TEC_WIDE multi-attack
  }

  if (!is_npc()) messages.add("%d enemies hit!", count_hit);
 } break;

 case TEC_DISARM:
  g->m.add_item(p->pos, p->unwield());
  if (u_see) messages.add("%s %s %s!", You.c_str(), regular_verb_agreement("disarm").c_str(), target.c_str());
  break;

 } // switch (tech)
}

technique_id player::pick_defensive_technique(game *g, const monster *z, player *p)
{
 if (blocks_left == 0) return TEC_NULL;

 assert(z || p);
 const mobile* const mob = z ? static_cast<const mobile*>(z) : p;

 const int foe_melee_skill = mob->melee_skill();
 const int foe_dodge = mob->dodge_roll();

 int foe_size = 0;
 if (z) foe_size = 4 + z->type->size * 4;
 else if (p) {
  foe_size = 12;
  if (p->str_max <= 5) foe_size -= 3;
  if (p->str_max >= 12) foe_size += 3;
 }

 blocks_left--;
 if (weapon.has_technique(TEC_WBLOCK_3) &&
     dice(melee_skill(), 12) > dice(foe_melee_skill, 10))
  return TEC_WBLOCK_3;

 if (weapon.has_technique(TEC_WBLOCK_2) &&
     dice(melee_skill(), 6) > dice(foe_melee_skill, 10))
  return TEC_WBLOCK_2;
 
 if (weapon.has_technique(TEC_WBLOCK_1) &&
     dice(melee_skill(), 3) > dice(foe_melee_skill, 10))
  return TEC_WBLOCK_1;

 if (weapon.has_technique(TEC_DEF_DISARM, this) &&
     z == nullptr && !p->unarmed_attack() &&
     dice(dex_cur + sklevel[sk_unarmed], 8) > dice(foe_melee_skill,  10))
  return TEC_DEF_DISARM;

 if (weapon.has_technique(TEC_DEF_THROW, this) && 
     str_cur + sklevel[sk_melee] >= foe_size + rng(-4, 4) &&
     hit_roll() > rng(1, 5) + foe_dodge && !one_in(3))
  return TEC_DEF_THROW;

 if (weapon.has_technique(TEC_COUNTER, this) &&
     hit_roll() > rng(1, 10) + foe_dodge && !one_in(3))
  return TEC_COUNTER;

 if (weapon.has_technique(TEC_BLOCK_LEGS, this) &&
     (hp_cur[hp_leg_l] >= 20 || hp_cur[hp_leg_r] >= 20) &&
     dice(melee_skill() + sklevel[sk_unarmed], 13) >
     dice(8 + foe_melee_skill, 10))
  return TEC_BLOCK_LEGS;

 if (weapon.has_technique(TEC_BLOCK, this) &&
     (hp_cur[hp_arm_l] >= 20 || hp_cur[hp_arm_r] >= 20) &&
     dice(melee_skill() + sklevel[sk_unarmed], 16) >
     dice(6 + foe_melee_skill, 10))
  return TEC_BLOCK;

 blocks_left++; // We didn't use any blocks, so give it back!
 return TEC_NULL;
}

void player::perform_defensive_technique(
  technique_id technique, game *g, monster *z, player *p,
  body_part &bp_hit, int &side, int &bash_dam, int &cut_dam, int &stab_dam)

{
 assert(z || p);
 mobile* const mob = z ? static_cast<mobile*>(z) : p;
 const std::string You = grammar::capitalize(desc(grammar::noun::role::subject));
 const std::string your = pronoun(grammar::noun::role::possessive);
 const std::string target = mob->desc(grammar::noun::role::direct_object, grammar::article::definite);
 const bool u_see = (!is_npc() || g->u.see(pos));

 static auto enhanced_block = [&](int ma) { // XXX itype_id
     switch (ma)
     {
     case itm_style_tai_chi: return per_cur - 6;
     case itm_style_taekwando: return str_cur - 6;
     default: return 0;
     }
 };

 switch (technique) {
  case TEC_BLOCK:
  case TEC_BLOCK_LEGS: {
   if (technique == TEC_BLOCK) {
    bp_hit = bp_arms;
	side = (hp_cur[hp_arm_l] >= hp_cur[hp_arm_r]) ? 0 : 1;
   } else { // Blocking with our legs
    bp_hit = bp_legs;
	side = (hp_cur[hp_leg_l] >= hp_cur[hp_leg_r]) ? 0 : 1;
   }
   if (u_see)
    messages.add("%s %s with %s %s.", You.c_str(), regular_verb_agreement("block").c_str(), your.c_str(), body_part_name(bp_hit, side));
   bash_dam /= 2;
   // styles never are worse than an unskilled block
   if (const int bonus = enhanced_block(weapon.type->id); 0 < bonus) {
       bash_dam *= clamped_lb<30>(100 - 8 * bonus);
       bash_dam /= 100;
   }
  } break;

  case TEC_WBLOCK_1:
  case TEC_WBLOCK_2:
  case TEC_WBLOCK_3:
// TODO: Cause weapon damage
   bash_dam = 0;
   cut_dam = 0;
   stab_dam = 0;
   if (u_see)
    messages.add("%s %s with %s %s.", You.c_str(), regular_verb_agreement("block").c_str(), your.c_str(), weapon.tname().c_str());
   [[fallthrough]];
  case TEC_COUNTER:
   break; // Handled elsewhere

  case TEC_DEF_THROW:
   if (u_see)
    messages.add("%s %s %s!", You.c_str(), regular_verb_agreement("throw").c_str(), target.c_str());
   bash_dam = 0;
   cut_dam  = 0;
   stab_dam = 0;
   mob->add(effect::DOWNED, rng(1, 2));
   mob->knockback_from(GPSpos + rng(within_rldist<1>));
   break;

  case TEC_DEF_DISARM:
   g->m.add_item(p->pos, p->unwield());
// Re-roll damage, without our weapon
   bash_dam = p->roll_bash_damage(nullptr, false);
   cut_dam  = p->roll_cut_damage(nullptr, false);
   stab_dam = p->roll_stab_damage(nullptr, false);
   if (u_see)
    messages.add("%s disarm%s %s!", You.c_str(), regular_verb_agreement("disarm").c_str(), target.c_str());
   break;

 } // switch (technique)
}

void player::perform_special_attacks(game *g, monster *z, player *p,
                                     int &bash_dam, int &cut_dam, int &stab_dam)
{
 assert(z || p);
 mobile* const mob = z ? static_cast<mobile*>(z) : p;
 bool can_poison = false;
 int bash_armor = (z == nullptr ? 0 : z->armor_bash());
 int cut_armor  = (z == nullptr ? 0 : z->armor_cut());
 int stab_armor = cataclysm::rational_scaled<4, 5>(cut_armor);
 const auto special_attacks = mutation_attacks(*mob);

 for (decltype(auto) attack : special_attacks) {
     bool did_damage = false;
     if (attack.bash > bash_armor) {
         bash_dam += attack.bash;
         did_damage = true;
     }
     if (attack.cut > cut_armor) {
         cut_dam += attack.cut - cut_armor;
         did_damage = true;
     }
     if (attack.stab > stab_armor) {
         stab_dam += attack.stab - stab_armor;
         did_damage = true;
     }

     if (   !can_poison && has_trait(PF_POISONOUS)
         && (attack.cut > cut_armor || attack.stab > stab_armor) && one_in(2))
         can_poison = true;

     if (did_damage) subjective_message(attack.text); // \todo NPC-competent messaging
 }

 static auto am_toxic = [&]() {
     return grammar::SVO_sentence(*this, "poison", mob->desc(grammar::noun::role::direct_object, grammar::article::definite), "!");
 };

 if (can_poison) {
     if (!mob->has(effect::POISONED)) if_visible_message(am_toxic);
     mob->add(effect::POISONED, TURNS(6));
 }
}

void player::melee_special_effects(game *g, monster *z, player *p, bool crit,
                                   int &bash_dam, int &cut_dam, int &stab_dam)
{
 assert(z || p);
 mobile* const mob = z ? static_cast<mobile*>(z) : p;
 bool is_u = !is_npc();
 bool can_see = (is_u || g->u.see(pos));
 const std::string you = desc(grammar::noun::role::subject);
 const std::string You = grammar::capitalize(std::string(you));
 const std::string your = desc(grammar::noun::role::possessive);
 const std::string Your = grammar::capitalize(std::string(your));
 const std::string target = mob->desc(grammar::noun::role::direct_object, grammar::article::definite);
 const std::string target_possessive = mob->desc(grammar::noun::role::possessive, grammar::article::definite);
 point tar = z ? z->pos : p->pos;

// Bashing effects
 mob->moves -= rng(0, bash_dam * 2);

// Bashing crit
 if (crit && !unarmed_attack()) {
  int turns_stunned = clamped_ub<6>(bash_dam / 20 + rng(0, sklevel[sk_bashing] / 2));
  if (turns_stunned > 0) mob->add(effect::STUNNED, turns_stunned);
 }

// Stabbing effects
 int stab_moves = rng(stab_dam / 2, 3 * stab_dam /2);
 if (crit) cataclysm::rational_scale<3,2>(stab_moves);
 if (stab_moves >= 3 * (mobile::mp_turn / 2)) {
  if (can_see)
   messages.add("%s force%s %s to the ground!", You.c_str(), (is_u ? "" : "s"), target.c_str());
  mob->moves -= stab_moves / 2;
  mob->add(effect::DOWNED, 1);
 } else mob->moves -= stab_moves;

// Bonus attacks!
 const bool target_electrified = is<MF_ELECTRIC>(z);
 bool shock_them = (!target_electrified && has_bionic(bio_shock)
                 && 2 <= power_level && unarmed_attack() && one_in(3));

 bool drain_them = (has_bionic(bio_heat_absorb) && power_level >= 1 &&
                    !is_armed() && !is_not<MF_WARM>(z));

 if (drain_them) power_level--;
 if (one_in(2)) drain_them = false;	// Only works half the time

 if (shock_them) {
  power_level -= 2;
  int shock = rng(2, 5);
  if (z) {
   z->hurt( shock * rng(1, 3) );
   z->moves -= shock * (mobile::mp_turn / 5) * 9;
   if (can_see)
    messages.add("%s shock%s %s!", You.c_str(), (is_u ? "" : "s"), target.c_str());
  } else {
   p->hurt(bp_torso, 0, shock * rng(1, 3));
   p->moves -= shock * (mobile::mp_turn / 5) * 4;
  }
 }

 if (drain_them) {
  charge_power(rng(0, 4));
  if (can_see)
   messages.add("%s drain%s %s body heat!", You.c_str(), (is_u ? "" : "s"),
               target_possessive.c_str());
  mob->moves -= rng(80, 120);
  if (z) z->speed -= rng(4, 6);
 }

 if (target_electrified) {
     if (!wearing_something_on(bp_hands) && weapon.conductive()) {
         hurtall(rng(0, 1));
         moves -= rng(0, 50);
         if (is_u) messages.add("Contact with %s shocks you!", target.c_str());
     }
 }

// Glass weapons shatter sometimes
 const int weapon_vol = weapon.volume();
 if (weapon.made_of(GLASS) && rng(0, weapon_vol + 8) < weapon_vol + str_cur) {
  if (can_see) messages.add("%s %s shatters!", Your.c_str(), weapon.tname().c_str());
  g->sound(pos, 16, "");
// Dump its contents on the ground
  for(decltype(auto) obj : weapon.contents) g->m.add_item(pos, std::move(obj));
  hit(g, bp_arms, 1, 0, rng(0, weapon_vol * 2));// Take damage
  if (weapon.is_two_handed(*this))// Hurt left arm too, if it was big
   hit(g, bp_arms, 0, 0, rng(0, weapon_vol));
  cut_dam += rng(0, 5 + int(weapon_vol * 1.5));// Hurt the monster extra
  remove_weapon();
 }

// Getting your weapon stuck
 int cutting_penalty = roll_stuck_penalty(z, stab_dam > cut_dam);
 if (weapon.has_flag(IF_MESSY)) { // e.g. chainsaws
  cutting_penalty /= 6; // Harder to get stuck
  for (int x = tar.x - 1; x <= tar.x + 1; x++) {
   for (int y = tar.y - 1; y <= tar.y + 1; y++) {
    if (!one_in(3)) {
     if (const auto blood = bleeds(z)) {
         auto& fd = g->m.field_at(x, y);
         if (fd.type == blood) {
             if (fd.density < 3) fd.density++;
         }
         else g->m.add_field(g, x, y, blood, 1);
     }
    }
   }
  }
 }
 if (!unarmed_attack() && cutting_penalty > dice(str_cur * 2, 20)) {
     static auto weapon_is_stuck = [&]() {
         return Your + weapon.tname() + " gets stuck in " + target + ", pulling it out of " + your + " hands!";
     };

     if_visible_message(weapon_is_stuck);
  if (z) {
   z->speed *= (weapon.has_flag(IF_SPEAR) || weapon.has_flag(IF_STAB)) ? .7 : .85;
   z->add_item(unwield());
  } else
   g->m.add_item(pos, unwield());
 } else {
     static auto weapon_is_almost_stuck = [&]() {
         return Your + weapon.tname() + " gets stuck in " + target + ", but " + you + " yank it free.";
     };

  if (z && (cut_dam >= z->hp || stab_dam >= z->hp)) {
   cutting_penalty /= 2;
   cutting_penalty -= rng(sklevel[sk_cutting], sklevel[sk_cutting] * 2 + 2);
  }
  if (cutting_penalty > 0) moves -= cutting_penalty;
  if (cutting_penalty >= 50) if_visible_message(weapon_is_almost_stuck);
  if (z && (weapon.has_flag(IF_SPEAR) || weapon.has_flag(IF_STAB))) z->speed *= .9;
 }

// Finally, some special effects for martial arts
 switch (weapon.type->id) {
  case itm_style_karate:
   dodges_left++;
   blocks_left += 2;
   break;

  case itm_style_aikido:
   bash_dam /= 2;
   break;

  case itm_style_capoeira:
   add_disease(DI_DODGE_BOOST, 2, 2);
   break;

  case itm_style_muay_thai:
   if (z) {
	   if (z->type->size >= MS_LARGE) bash_dam += rng(z->type->size, 3 * z->type->size);
   } else if (p->str_max >= 12)
	   bash_dam += rng((p->str_max - 8) / 4, 3 * (p->str_max - 8) / 4);
   break;

  case itm_style_tiger:
   add_disease(DI_DAMAGE_BOOST, 2, 2, 10);
   break;

  case itm_style_centipede:
   add_disease(DI_SPEED_BOOST, 2, 4, 40);
   break;

  case itm_style_venom_snake:
      switch (disease_intensity(DI_VIPER_COMBO))
      {
      case 0:
          if (crit) {
              subjective_message("Tail whip!  Viper Combo Initiated!");
              bash_dam += 5;
              add_disease(DI_VIPER_COMBO, 2, 1, 2);
          }
          break;
      case 1:
          subjective_message("Snakebite!");
          std::swap(bash_dam, stab_dam);
          add_disease(DI_VIPER_COMBO, 2, 1, 2); // Upgrade to Viper Strike
          break;
      default: // intended to be case 2
          if (hp_cur[hp_arm_l] >= cataclysm::rational_scaled<3, 4>(hp_max[hp_arm_l]) &&
              hp_cur[hp_arm_r] >= cataclysm::rational_scaled<3, 4>(hp_max[hp_arm_r])) {
              subjective_message("Viper STRIKE!");
              bash_dam *= 3;
          } else
              subjective_message("Your injured arms prevent a viper strike!");
          rem_disease(DI_VIPER_COMBO);
          break;
      }
   break;

  case itm_style_scorpion:
   if (crit) {
    if (!is_npc()) messages.add("Stinger Strike!");
    mob->add(effect::STUNNED, TURNS(2));
    const auto origin(mob->GPSpos);
    mob->knockback_from(origin);
    if (origin != mob->GPSpos) mob->knockback_from(origin); // Knock a 2nd time if the first worked
   }
   break;

  case itm_style_zui_quan:
   dodges_left = 50; // Basically, unlimited.
   break;
 }
}

std::vector<special_attack> player::mutation_attacks(const mobile& mob) const
{
 std::vector<special_attack> ret;

 const std::string You  = grammar::capitalize(subject());
 const std::string Your = grammar::capitalize(possessive());
 const std::string your = pronoun(grammar::noun::role::possessive);
 const std::string target = mob.desc(grammar::noun::role::direct_object, grammar::article::definite);

 std::ostringstream text;

 if (has_trait(PF_FANGS) && !wearing_something_on(bp_mouth) &&
     one_in(20 - dex_cur - sklevel[sk_unarmed])) {
  text.str(You);
  text << regular_verb_agreement(" sink") << your << " fangs into " << target << "!";
  special_attack tmp(text.str());
  tmp.stab = 20;
  ret.push_back(tmp);
 }

 if (has_trait(PF_MANDIBLES) && one_in(22 - dex_cur - sklevel[sk_unarmed])) {
  text.str(You);
  text << regular_verb_agreement(" slice") << target << " with " << your << " mandibles!";
  special_attack tmp(text.str());
  tmp.cut = 12;
  ret.push_back(tmp);
 }

 if (has_trait(PF_BEAK) && one_in(15 - dex_cur - sklevel[sk_unarmed])) {
  text.str(You);
  text << regular_verb_agreement(" peck") << target << "!";
  special_attack tmp(text.str());
  tmp.stab = 15;
  ret.push_back(tmp);
 }
  
 if (has_trait(PF_HOOVES) && one_in(25 - dex_cur - 2 * sklevel[sk_unarmed])) {
  text.str(You);
  text << regular_verb_agreement(" kick") << target << " with " << your << " hooves!";
  special_attack tmp(text.str());
  tmp.bash = clamped_ub<40>(str_cur * 3);
  ret.push_back(tmp);
 }

 if (has_trait(PF_HORNS) && one_in(20 - dex_cur - sklevel[sk_unarmed])) {
  text.str(You);
  text << regular_verb_agreement(" headbutt") << target << " with " << your << " horns!";
  special_attack tmp(text.str());
  tmp.bash = 3;
  tmp.stab = 3;
  ret.push_back(tmp);
 }

 if (has_trait(PF_HORNS_CURLED) && one_in(20 - dex_cur - sklevel[sk_unarmed])) {
  text.str(You);
  text << regular_verb_agreement(" headbutt") << target << " with " << your << " curled horns!";
  special_attack tmp(text.str());
  tmp.bash = 14;
  ret.push_back(tmp);
 }

 if (has_trait(PF_HORNS_POINTED) && one_in(22 - dex_cur - sklevel[sk_unarmed])){
  text.str(You);
  text << regular_verb_agreement(" stab") << target << " with " << your << " pointed horns!";
  special_attack tmp(text.str());
  tmp.stab = 24;
  ret.push_back(tmp);
 }

 if (has_trait(PF_ANTLERS) && one_in(20 - dex_cur - sklevel[sk_unarmed])) {
  text.str(You);
  text << regular_verb_agreement(" butt") << target << " with " << your << " antlers!";
  special_attack tmp(text.str());
  tmp.bash = 4;
  ret.push_back(tmp);
 }

 if (has_trait(PF_TAIL_STING) && one_in(3) && one_in(10 - dex_cur)) {
  text.str(You);
  text << regular_verb_agreement(" sting") << target << " with " << your << " tail!";
  special_attack tmp(text.str());
  tmp.stab = 20;
  ret.push_back(tmp);
 }

 if (has_trait(PF_TAIL_CLUB) && one_in(3) && one_in(10 - dex_cur)) {
  text.str(You);
  text << regular_verb_agreement(" hit") << target << " with " << your << " tail!";
  special_attack tmp(text.str());
  tmp.bash = 18;
  ret.push_back(tmp);
 }

 if (has_trait(PF_ARM_TENTACLES) || has_trait(PF_ARM_TENTACLES_4) ||
     has_trait(PF_ARM_TENTACLES_8)) {
  int num_attacks = 1;
  if (has_trait(PF_ARM_TENTACLES_4)) num_attacks = 3;
  if (has_trait(PF_ARM_TENTACLES_8)) num_attacks = 7;
  if (weapon.is_two_handed(*this)) num_attacks--;

  for (int i = 0; i < num_attacks; i++) {
   if (one_in(18 - dex_cur - sklevel[sk_unarmed])) {
    text.str(You);
    text << regular_verb_agreement(" slap") << target << " with " << your << " tentacle!";
    special_attack tmp(text.str());
    tmp.bash = str_cur / 2;
    ret.push_back(tmp);
   }
  }
 }

 return ret;
}

std::string melee_verb(technique_id tech, std::string your, player &p,
                       int bash_dam, int cut_dam, int stab_dam)
{
    if (tech) {
        if (const auto style = p.weapon.is_style()) {
            if (const auto m = style->data(tech)) return p.regular_verb_agreement(m->name);
        }
    }

 std::ostringstream ret;

 switch (tech) {

  case TEC_SWEEP:
   ret << p.regular_verb_agreement("sweep") << " " << your << " " << p.weapon.tname() << " at";
   break;

  case TEC_PRECISE:
   ret << p.regular_verb_agreement("jab") << " " << your << " " << p.weapon.tname() << " at";
   break;

  case TEC_BRUTAL:
   ret << p.regular_verb_agreement("slam") << " " << your << " " << p.weapon.tname() << " against";
   break;

  case TEC_GRAB:
   ret << p.regular_verb_agreement("wrap") << " " << your << " " << p.weapon.tname() << " around";
   break;

  case TEC_WIDE:
   ret << p.regular_verb_agreement("swing") << " " << your << " " << p.weapon.tname() << " wide at";
   break;

  case TEC_THROW:
   ret << p.regular_verb_agreement("use") << " " << your << " " << p.weapon.tname() << " to toss";
   break;

  default: // No tech, so check our damage levels
   if (bash_dam >= cut_dam && bash_dam >= stab_dam) {
    if (bash_dam >= 30) return p.regular_verb_agreement("clobber");
    if (bash_dam >= 20) return p.regular_verb_agreement("batter");
    if (bash_dam >= 10) return p.regular_verb_agreement("whack");
    return p.regular_verb_agreement("hit");
   }
   if (cut_dam >= stab_dam) {
    if (cut_dam >= 30) return p.regular_verb_agreement("hack");
    if (cut_dam >= 20) return p.regular_verb_agreement("slice");
    if (cut_dam >= 10) return p.regular_verb_agreement("cut");
    return p.regular_verb_agreement("nick");
   }
// Only stab damage is left
   if (stab_dam >= 30) return p.regular_verb_agreement("impale");
   if (stab_dam >= 20) return p.regular_verb_agreement("pierce");
   if (stab_dam >= 10) return p.regular_verb_agreement("stab");
   return p.regular_verb_agreement("poke");
 } // switch (tech)

 return ret.str();
}
 