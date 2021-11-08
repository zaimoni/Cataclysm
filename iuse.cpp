#include "iuse.h"
#include "game.h"
#include "mapdata.h"
#include "keypress.h"
#include "output.h"
#include "rng.h"
#include "line.h"
#include "monattack.h"
#include "recent_msg.h"
#include "zero.h"
#include "stl_limits.h"

#include <sstream>

// all callers of these handlers have a well-defined item available (and thus *should* have a valid pos from game::find_item

/* To mark an item as "removed from inventory", set its invlet to 0
   This is useful for traps (placed on ground), inactive bots, etc
 */
void iuse::sewage(player *p, item *it, bool t)
{
 p->vomit();
 if (one_in(4)) p->mutate();
}

void iuse::royal_jelly(player *p, item *it, bool t)
{
// TODO: Add other diseases here; royal jelly is a cure-all!
 p->pkill += 5;
 std::string message;
 if (p->has_disease(DI_FUNGUS)) {
  message = "You feel cleansed inside!";
  p->rem_disease(DI_FUNGUS);
 }
 if (p->has_disease(DI_BLIND)) {
  message = "Your sight returns!";
  p->rem_disease(DI_BLIND);
 }
 if (p->has_disease(DI_POISON) || p->has_disease(DI_FOODPOISON) ||
     p->has_disease(DI_BADPOISON)) {
  message = "You feel much better!";
  p->rem_disease(DI_POISON);
  p->rem_disease(DI_BADPOISON);
  p->rem_disease(DI_FOODPOISON);
 }
 if (p->has_disease(DI_ASTHMA)) {
  message = "Your breathing clears up!";
  p->rem_disease(DI_ASTHMA);
 }
 if (p->has_disease(DI_COMMON_COLD) || p->has_disease(DI_FLU)) {
  message = "You feel healther!";
  p->rem_disease(DI_COMMON_COLD);
  p->rem_disease(DI_FLU);
 }
 p->subjective_message(message);
}

static void _display_hp(WINDOW* w, player* p, int curhp, int i)
{
    const auto cur_hp_color = p->hp_color(hp_part(i));
    if (p->has_trait(PF_HPIGNORANT))
        mvwaddstrz(w, i + 2, 15, cur_hp_color.second, "***");
    else {
        mvwprintz(w, i + 2, 17-int_log10(curhp), cur_hp_color.second, "%d", cur_hp_color.first);
    }
}

static std::optional<hp_part> _get_heal_target(player* p, item* it)
{
    int ch;
    do {
        ch = getch();
        if (ch == '1') return hp_head;
        else if (ch == '2') return hp_torso;
        else if (ch == '3') {
            if (p->hp_cur[hp_arm_l] == 0) {
                p->subjective_message("That arm is broken.  It needs surgical attention.");
                it->charges++;
                return std::nullopt;
            }
            return hp_arm_l;
        }
        else if (ch == '4') {
            if (p->hp_cur[hp_arm_r] == 0) {
                p->subjective_message("That arm is broken.  It needs surgical attention.");
                it->charges++;
                return std::nullopt;
            }
            return hp_arm_r;
        } else if (ch == '5') {
            if (p->hp_cur[hp_leg_l] == 0) {
                p->subjective_message("That leg is broken.  It needs surgical attention.");
                it->charges++;
                return std::nullopt;
            }
            return hp_leg_l;
        } else if (ch == '6') {
            if (p->hp_cur[hp_leg_r] == 0) {
                p->subjective_message("That leg is broken.  It needs surgical attention.");
                it->charges++;
                return std::nullopt;
            }
            return hp_leg_r;
        } else if (ch == '7') {
            p->subjective_message("Never mind.");
            it->charges++;
            return std::nullopt;
        }
    } while (true);
}

// apparently NPCs have full surgery kits automatically?
// returns -1 if nothing needs healing
static int _npc_get_heal_target(npc* p)
{
    int healed = -1;
    int highest_damage = 0;
    for (int i = 0; i < num_hp_parts; i++) {
        int damage = p->hp_max[i] - p->hp_cur[i];
        if (i == hp_head) cataclysm::rational_scale<3,2>(damage);
        if (i == hp_torso) cataclysm::rational_scale<6,5>(damage);
        if (damage > highest_damage) {
            highest_damage = damage;
            healed = i;
        }
    }
    return healed;
}

// \todo adjust to allow treating other (N)PCs
void iuse::bandage(player *p, item *it, bool t)
{
 int bonus = p->sklevel[sk_firstaid];
 hp_part healed;

 if (p->is_npc()) { // NPCs heal whichever has sustained the most damage
     if (auto code = _npc_get_heal_target(static_cast<npc*>(p)); 0 <= code) {
         healed = hp_part(code);
     } else {
         return;
     }
 } else { // Player--present a menu
     static constexpr const char* bandage_options[] = {
         "Bandage where?",
         "1: Head",
         "2: Torso",
         "3: Left Arm",
         "4: Right Arm",
         "5: Left Leg",
         "6: Right Leg",
         "7: Exit"
     };
     constexpr const int bandage_height = sizeof(bandage_options) / sizeof(*bandage_options) + 2;

     // maximum string length...possible function target
     // C++20: std::ranges::max?
     int bandage_width = 0;
     for (const char* const line : bandage_options) bandage_width = cataclysm::max(bandage_width, strlen(line));
     bandage_width += 5; // historical C:Whales; 15 is magic constant for layout so likely want 20 width explicitly

     WINDOW* w = newwin(bandage_height, bandage_width, (VIEW - bandage_height) / 2, (SCREEN_WIDTH - bandage_width) / 2);
     wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
         LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX);
     int row = 0;
     for (const char* const line : bandage_options) {
         ++row;
         mvwaddstrz(w, row, 1, 1 == row ? c_ltred : c_ltgray, line);
     }

  int curhp;
  for (int i = 0; i < num_hp_parts; i++) {
   curhp = p->hp_cur[i];
   int tmpbonus = bonus;
   if (curhp != 0) {
    switch (hp_part(i)) {
     case hp_head:  curhp += 1;	tmpbonus *=  .8;	break;
     case hp_torso: curhp += 4;	tmpbonus *= 1.5;	break;
     default:       curhp += 3;				break;
    }
    curhp += tmpbonus;
    clamp_ub(curhp, p->hp_max[i]);
    _display_hp(w, p, curhp, i);
   } else	// curhp is 0; requires surgical attention
    mvwaddstrz(w, i + 2, 15, c_dkgray, "---");
  }
  wrefresh(w);
  const auto ok = _get_heal_target(p, it);
  werase(w);
  wrefresh(w);
  delwin(w);
  refresh();
  if (ok) healed = *ok;
  else return;
 }

 p->practice(sk_firstaid, 8);
 int dam = 0;
 if (healed == hp_head)
  dam = 1 + bonus * .8;
 else if (healed == hp_torso)
  dam = 4 + bonus * 1.5;
 else
  dam = 3 + bonus;
 p->heal(healed, dam);
}

// \todo adjust to allow treating other (N)PCs
void iuse::firstaid(player *p, item *it, bool t)
{
 int bonus = p->sklevel[sk_firstaid];
 hp_part healed;

 if (p->is_npc()) { // NPCs heal whichever has sustained the most damage
     if (auto code = _npc_get_heal_target(static_cast<npc*>(p)); 0 <= code) {
         healed = hp_part(code);
     } else {
         return;
     }
 } else { // Player--present a menu
 static constexpr const char* firstaid_options[] = {
     "Bandage where?",
     "1: Head",
     "2: Torso",
     "3: Left Arm",
     "4: Right Arm",
     "5: Left Leg",
     "6: Right Leg",
     "7: Exit"
 };
 constexpr const int firstaid_height = sizeof(firstaid_options) / sizeof(*firstaid_options) + 2;

 // maximum string length...possible function target
 // C++20: std::ranges::max?
 int firstaid_width = 0;
 for (const char* const line : firstaid_options) firstaid_width = cataclysm::max(firstaid_width, strlen(line));
 firstaid_width += 5; // historical C:Whales; 15 is magic constant for layout so likely want 20 width explicitly

 WINDOW* w = newwin(firstaid_height, firstaid_width, (VIEW - firstaid_height) / 2, (SCREEN_WIDTH - firstaid_width) / 2);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
     LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX);
 int row = 0;
 for (const char* const line : firstaid_options) {
     ++row;
     mvwaddstrz(w, row, 1, 1==row ? c_ltred : c_ltgray, line);
 }

  int curhp;
  for (int i = 0; i < num_hp_parts; i++) {
   curhp = p->hp_cur[i];
   int tmpbonus = bonus;
   if (curhp != 0) {
    switch (hp_part(i)) {
     case hp_head:  curhp += 10; tmpbonus *=  .8;	break;
     case hp_torso: curhp += 18; tmpbonus *= 1.5;	break;
     default:       curhp += 14;			break;
    }
    curhp += tmpbonus;
    clamp_ub(curhp, p->hp_max[i]);
    _display_hp(w, p, curhp, i);
   } else	// curhp is 0; requires surgical attention
    mvwaddstrz(w, i + 2, 15, c_dkgray, "---");
  }
  wrefresh(w);

  const auto ok = _get_heal_target(p, it);
  werase(w);
  wrefresh(w);
  delwin(w);
  refresh();
  if (ok) healed = *ok;
  else return;
 }

 p->practice(sk_firstaid, 8);
 int dam = 0;
 if (healed == hp_head)
  dam = 10 + bonus * .8;
 else if (healed == hp_torso)
  dam = 18 + bonus * 1.5;
 else
  dam = 14 + bonus;
 p->heal(healed, dam);
}

// \todo guard clause -- this is relatively easy to NPC-convert
void iuse::vitamins(player *p, item *it, bool t)
{
 if (p->has_disease(DI_TOOK_VITAMINS)) {
  p->subjective_message("You have the feeling that these vitamins won't do you any good.");
  return;
 }

 p->add_disease(DI_TOOK_VITAMINS, DAYS(1));
 if (p->health <= -2) p->health -= (p->health / 2);
 p->health += 3;
}

// Aspirin
void iuse::pkill_1(player *p, item *it, bool t)
{
 if (!p->is_npc()) messages.add("You take some %s.", it->tname().c_str());

 if (!p->has_disease(DI_PKILL1))
  p->add_disease(DI_PKILL1, MINUTES(12));
 else {
  for (int i = 0; i < p->illness.size(); i++) {
   if (p->illness[i].type == DI_PKILL1) {
    p->illness[i].duration = MINUTES(12);
    i = p->illness.size();
   }
  }
 }
}

// Codeine
void iuse::pkill_2(player *p, item *it, bool t)
{
 if (!p->is_npc()) messages.add("You take some %s.", it->tname().c_str());

 p->add_disease(DI_PKILL2, MINUTES(18));
}

void iuse::pkill_3(player *p, item *it, bool t)
{
 if (!p->is_npc()) messages.add("You take some %s.", it->tname().c_str());

 p->add_disease(DI_PKILL3, MINUTES(2));
 p->add_disease(DI_PKILL2, MINUTES(20));
}

void iuse::pkill_4(player *p, item *it, bool t)
{
 if (!p->is_npc()) messages.add("You shoot up.");

 p->add_disease(DI_PKILL3, MINUTES(8));
 p->add_disease(DI_PKILL2, MINUTES(20));
}

void iuse::pkill_l(player *p, item *it, bool t)
{
 if (!p->is_npc()) messages.add("You take some %s.", it->tname().c_str());

 p->add_disease(DI_PKILL_L, rng(12, 18) * MINUTES(30));
}

void iuse::xanax(player *p, item *it, bool t)
{
 if (!p->is_npc()) messages.add("You take some %s.", it->tname().c_str());

 p->add_disease(DI_TOOK_XANAX, (!p->has_disease(DI_TOOK_XANAX) ? MINUTES(90): MINUTES(20)));
}

void iuse::caff(player *p, item *it, bool t)
{
 p->fatigue -= dynamic_cast<const it_comest*>(it->type)->stim * 3;
}

void iuse::alcohol(player *p, item *it, bool t)
{
 int duration = MINUTES(68 - p->str_max); // Weaker characters are cheap drunks
 if (p->has_trait(PF_LIGHTWEIGHT)) duration += MINUTES(30);
 p->pkill += 8;
 p->add_disease(DI_DRUNK, duration);
 if (p->has_bionic(bio_ethanol)) p->charge_power(rng(2, 8));
}

void iuse::cig(player *p, item *it, bool t)
{
 if (!p->is_npc()) messages.add("You light a cigarette and smoke it.");
 p->add_disease(DI_CIG, MINUTES(20));
 for (int i = 0; i < p->illness.size(); i++) {
  if (p->illness[i].type == DI_CIG && p->illness[i].duration > HOURS(1) && !p->is_npc())
   messages.add("Ugh, too much smoke... you feel gross.");
 }
}

void iuse::weed(player *p, item *it, bool t)
{
 if (!p->is_npc()) messages.add("Good stuff, man!");

 int duration = MINUTES(6);
 if (p->has_trait(PF_LIGHTWEIGHT)) duration = MINUTES(9);
 p->hunger += 8;
 if (p->pkill < 15) p->pkill += 5;
 p->add_disease(DI_THC, duration);
}

void iuse::coke(player *p, item *it, bool t)
{
 if (!p->is_npc()) messages.add("You snort a bump.");

 int duration = TURNS(21 - p->str_cur);
 if (p->has_trait(PF_LIGHTWEIGHT)) duration += MINUTES(2);
 p->hunger -= 8;
 p->add_disease(DI_HIGH, duration);
}

void iuse::meth(player *p, item *it, bool t)
{
 int duration = MINUTES(1) * (40 - p->str_cur);
 if (p->has_charges(itm_lighter, 1)) {
  if (!p->is_npc()) messages.add("You smoke some crystals.");
  duration *= 1.5;
 } else if (!p->is_npc()) messages.add("You snort some crystals.");
 if (!p->has_disease(DI_METH)) duration += HOURS(1);
 int hungerpen = (p->str_cur < 10 ? 20 : 30 - p->str_cur);
 p->hunger -= hungerpen;
 p->add_disease(DI_METH, duration);
}

void iuse::poison(player *p, item *it, bool t)
{
 p->add_disease(DI_POISON, HOURS(1));
 p->add_disease(DI_FOODPOISON, HOURS(3));
}

void iuse::hallu(player *p, item *it, bool t)
{
 p->add_disease(DI_HALLU, HOURS(4));
}

void iuse::thorazine(player *p, item *it, bool t)
{
 p->fatigue += 15;
 p->rem_disease(DI_HALLU);
 p->rem_disease(DI_VISUALS);
 p->rem_disease(DI_HIGH);
 p->rem_disease(DI_THC);    // B-movie; situation in real life is more complicated
 if (!p->has_disease(DI_DERMATIK)) p->rem_disease(DI_FORMICATION);
 p->subjective_message("You feel somewhat sedated.");
}

void iuse::prozac(player *p, item *it, bool t) // \todo not powerful enough
{
 if (!p->has_disease(DI_TOOK_PROZAC) && p->morale_level() < 0)
  p->add_disease(DI_TOOK_PROZAC, HOURS(6));
 else
  p->stim += 3;
}

void iuse::sleep(player *p, item *it, bool t)
{
 p->fatigue += 40;
 p->subjective_message("You feel very sleepy...");
}

void iuse::iodine(player *p, item *it, bool t)
{
 p->add_disease(DI_IODINE, HOURS(2));
 p->subjective_message("You take an iodine tablet.");
}

void iuse::flumed(player *p, item *it, bool t)
{
 p->add_disease(DI_TOOK_FLUMED, HOURS(10));
 if (!p->is_npc()) messages.add("You take some %s", it->tname().c_str());
}

void iuse::flusleep(player *p, item *it, bool t)
{
 p->add_disease(DI_TOOK_FLUMED, HOURS(12));
 p->fatigue += 30;
 p->subjective_message("You feel very sleepy...");
}

void iuse::inhaler(player *p, item *it, bool t)
{
 p->rem_disease(DI_ASTHMA);
 p->subjective_message("You take a puff from your inhaler.");
}

void iuse::blech(player *p, item *it, bool t)
{
// TODO: Add more effects?
 p->subjective_message("Blech, that burns your throat!");
 p->vomit();
}

void iuse::mutagen(player *p, item *it, bool t)
{
 if (!one_in(3)) p->mutate();
}

void iuse::mutagen_3(player *p, item *it, bool t)
{
 p->mutate();
 if (!one_in(3)) p->mutate();
 if (one_in(2)) p->mutate();
}

void iuse::purifier(player *p, item *it, bool t)
{
 std::vector<int> valid;	// Which flags the player has
 for (int i = 1; i < PF_MAX2; i++) {
  if (p->has_trait(pl_flag(i)) && p->has_mutation(pl_flag(i))) valid.push_back(i);
 }
 if (valid.empty()) {
  messages.add("You feel cleansed.");
  return;
 }
 int num_cured = rng(1, valid.size());
 if (num_cured > 4) num_cured = 4;
 for (int i = 0; i < num_cured && valid.size() > 0; i++) {
  int index = rng(0, valid.size() - 1);
  p->remove_mutation(pl_flag(valid[index]) );
  valid.erase(valid.begin() + index);
 }
}

void iuse::marloss(player *p, item *it, bool t)
{
 if (p->is_npc()) return;
 const auto g = game::active();

 // If we have the marloss in our veins, we are a "breeder" and will spread
// alien lifeforms.
 if (p->has_trait(PF_MARLOSS)) {
  messages.add("As you eat the berry, you have a near-religious experience, feeling at one with your surroundings...");
  p->add_morale(MORALE_MARLOSS, 100, 1000);
  p->hunger = -100;
  monster goo(mtype::types[mon_blob]);
  goo.friendly = -1;
  int goo_spawned = 0;
  for (int x = p->pos.x - 4; x <= p->pos.x + 4; x++) {
   for (int y = p->pos.y - 4; y <= p->pos.y + 4; y++) {
	const auto dist = trig_dist(x, y, p->pos);
    if (rng(0, 10) > dist &&
        rng(0, 10) > dist)
     g->m.marlossify(x, y);
    if (one_in(10 + 5 * dist) && (goo_spawned == 0 || one_in(goo_spawned * 2))) {
     goo.spawn(x, y);
     g->z.push_back(goo);
     goo_spawned++;
    }
   }
  }
  return;
 }
/* If we're not already carriers of Marloss, roll for a random effect:
 * 1 - Mutate
 * 2 - Mutate
 * 3 - Mutate
 * 4 - Purify
 * 5 - Purify
 * 6 - Cleanse radiation + Purify
 * 7 - Fully satiate
 * 8 - Vomit
 * 9 - Give Marloss mutation
 */
 int effect = rng(1, 9);
 if (effect <= 3) {
  messages.add("This berry tastes extremely strange!");
  p->mutate();
 } else if (effect <= 6) { // Radiation cleanse is below
  messages.add("This berry makes you feel better all over.");
  p->pkill += 30;
  purifier(p, it, t);
 } else if (effect == 7) {
  messages.add("This berry is delicious, and very filling!");
  p->hunger = -100;
 } else if (effect == 8) {
  messages.add("You take one bite, and immediately vomit!");
  p->vomit();
 } else if (!p->has_trait(PF_MARLOSS)) {
  messages.add("You feel a strange warmth spreading throughout your body...");
  p->toggle_trait(PF_MARLOSS);
 }
 if (effect == 6) p->radiation = 0;
}

void iuse::dogfood(player *p, item *it, bool t)
{
 const auto g = game::active();

 it->invlet = 0;    // C:Whales: unconditional discard after use(opening)

 g->draw();
 mvprintw(0, 0, "Which direction?");
 point dir(get_direction(input()));
 if (dir.x == -2) {
  messages.add("Invalid direction.");
  return;
 }
 p->moves -= (mobile::mp_turn / 20) * 3;
 if (monster* const m_at = g->mon(p->GPSpos+dir)) {
  if (m_at->type->id == mon_dog) {
   messages.add("The dog seems to like you!");
   m_at->friendly = -1;
  } else
   messages.add("The %s seems quit unimpressed!", m_at->type->name.c_str());
 } else
  messages.add("You spill the dogfood all over the ground.");
}

// TOOLS below this point!

void iuse::lighter(pc& p, item& it)
{
 const auto g = game::active();

 g->draw();
 mvprintw(0, 0, "Light where?");
 const point dir(get_direction(input()));
 if (dir.x == -2) {
  messages.add("Invalid direction.");
  it.charges++;
  return;
 }
 p.moves -= (3 * mobile::mp_turn) / 20;
 const auto dest = dir + p.pos;

 if (g->m.flammable_items_at(dest.x, dest.y)) {
  g->m.add_field(g, dest, fd_fire, 1, 30);
 } else {
  messages.add("There's nothing to light there.");
  it.charges++;
 }
}

void iuse::sew(pc& p, item& it)
{
 char ch = p.get_invlet("Repair what?");
 const auto src = p.from_invlet(ch);
 if (!src) {
     messages.add("You do not have that item!");
     it.charges++;
     return;
 }

 item* fix = src->first; // backward compatibility
 if (!fix->is_armor()) {
  messages.add("That isn't clothing!");
  it.charges++;
  return;
 }
 if (!fix->made_of(COTTON) && !fix->made_of(WOOL)) {
  messages.add("Your %s is not made of cotton or wool.", fix->tname().c_str());
  it.charges++;
  return;
 }
 if (fix->damage < 0) {
  messages.add("Your %s is already enhanced.", fix->tname().c_str());
  it.charges++;
  return;
 };

 p.moves -= 5 * mobile::mp_turn;
 int rn = dice(4, 2 + p.sklevel[sk_tailor]);
 if (p.dex_cur < 8 && one_in(p.dex_cur)) rn -= rng(2, 6);
 if (p.dex_cur >= 16 || (p.dex_cur > 8 && one_in(16 - p.dex_cur))) rn += rng(2, 6);
 if (p.dex_cur > 16) rn += rng(0, p.dex_cur - 16);

 if (fix->damage == 0) {
  p.practice(sk_tailor, 10);
  if (rn <= 4) {
   messages.add("You damage your %s!", fix->tname().c_str());
   fix->damage++;
  } else if (rn >= 12) {
   messages.add("You make your %s extra-sturdy.", fix->tname().c_str());
   fix->damage--;
  } else
   messages.add("You practice your sewing.");
 } else {
  p.practice(sk_tailor, 8);

  rn -= rng(fix->damage, fix->damage * 2); // unclear that repair is actually more difficult than enhancement

  if (rn <= 4) {
   messages.add("You damage your %s further!", fix->tname().c_str());
   fix->damage++;
   if (fix->damage >= 5) {
    messages.add("You destroy it!");
    p.remove_discard(*src);
   }
  } else if (rn <= 6) {
   messages.add("You don't repair your %s, but you waste lots of thread.", fix->tname().c_str());
   clamp_lb<0>(it.charges -= rng(1, 8));
  } else if (rn <= 8) {
   messages.add("You repair your %s, but waste lots of thread.", fix->tname().c_str());
   fix->damage--;
   clamp_lb<0>(it.charges -= rng(1, 8));
  } else if (rn <= 16) {
   messages.add("You repair your %s!", fix->tname().c_str());
   fix->damage--;
  } else {
   messages.add("You repair your %s completely!", fix->tname().c_str());
   fix->damage = 0;
  }
 }
}

void iuse::scissors(pc& p, item& it)
{
 char ch = p.get_invlet("Chop up what?");
 auto src = p.from_invlet(ch);
 if (!src) {
     messages.add("You do not have that item!");
     return;
 }

 item* cut = src->first; // backward compatibility
 static auto is_string_like = [](int it) {
     switch (it)
     {
     case itm_string_36: return std::optional(std::pair("string", std::pair(6, itm_string_6)));
     case itm_rope_30: return std::optional(std::pair("rope", std::pair(5, itm_rope_6)));
     default: return std::optional<std::pair<const char*, std::pair<int, itype_id> > >();
     };
 };

 static auto drop_n_clone = [&](int n, item&& obj) {
     bool drop = false;
     while (0 < n--) {
         if (!drop) {
             // \todo do we want stacking here?
             if (p.volume_carried() >= p.volume_capacity() || !p.assign_invlet(obj)) drop = true;
         }
         if (drop) game::active()->m.add_item(p.pos, std::move(obj));
         else p.i_add(std::move(obj));
     }
 };

 if (cut->type->id == itm_rag) {
  messages.add("There's no point in cutting a rag.");
  return;
 }
 if (const auto dest = is_string_like(cut->type->id)) {
     p.moves -= 3 * (mobile::mp_turn) / 2;
     int pieces = dest->second.first; // backward compatibility
     auto i_type = dest->second.second;
     messages.add("You cut the %s into %d smaller pieces.", dest->first, dest->second.first);
     p.remove_discard(*src);
     drop_n_clone(pieces, item(item::types[i_type], int(messages.turn)));
     return;
 }
 if (!cut->made_of(COTTON)) {
  messages.add("You can only slice items made of cotton.");
  return;
 }
 const auto vol = cut->volume();
 p.moves -= (mobile::mp_turn / 4) * vol;
 int count = vol;
 if (p.sklevel[sk_tailor] == 0) count = rng(0, count);
 else if (p.sklevel[sk_tailor] == 1 && count >= 2) count -= rng(0, 2);

 if (dice(3, 3) > p.dex_cur) count -= rng(1, 3);

 if (count <= 0) {
  messages.add("You clumsily cut the %s into useless ribbons.", cut->tname().c_str());
  p.remove_discard(*src);
  return;
 }
 messages.add("You slice the %s into %d rag%s.", cut->tname().c_str(), count,
            (count == 1 ? "" : "s"));
 p.remove_discard(*src);
 drop_n_clone(count, item(item::types[itm_rag], int(messages.turn)));
}

void iuse::extinguisher(pc& p, item& it)
{
 const auto g = game::active();

 g->draw();
 mvprintz(0, 0, c_red, "Pick a direction to spray:");
 const point dir(get_direction(input()));
 if (dir.x == -2) {
  messages.add("Invalid direction!");
  it.charges++;
  return;
 }
 p.moves -= (mobile::mp_turn / 5) * 7;
 auto pt(dir + p.pos);
 {
 auto& fd = g->m.field_at(pt);
 if (fd.type == fd_fire) {
  if (0 >= (fd.density -= rng(2, 3))) g->m.remove_field(pt);
 }
 }
 if (monster* const m_at = g->mon(pt)) {
  m_at->moves -= (mobile::mp_turn / 2) * 3;
  const bool is_liquid = m_at->made_of(LIQUID); // assumed that liquid is water or similar -- dry ice extinguisher?
  if (p.see(*m_at)) {
      const auto z_name = grammar::capitalize(m_at->desc(grammar::noun::role::subject, grammar::article::definite));
      messages.add("%s is sprayed!", z_name.c_str());
      if (is_liquid) messages.add("%s is frozen!", z_name.c_str());
  }
  if (is_liquid) {
   if (m_at->hurt(rng(20, 60))) g->kill_mon(*m_at, &p);
   else m_at->speed /= 2;
  }
 }
 if (g->m.move_cost(pt) != 0) {
  pt += dir;
  auto& fd = g->m.field_at(pt);
  if (fd.type == fd_fire) {
   fd.density -= rng(0, 1) + rng(0, 1);
   if (fd.density <= 0) g->m.remove_field(pt);
   else if (3 < fd.density) fd.density = 3;
  }
 }
}

// xref: construction.cpp/"Board Up Door"
// interpretation: start terrain, dest terrain, # nails, # boards
static constexpr const std::pair<ter_id, std::tuple<ter_id, int, int> > deconstruct_boarded[] = {
    std::pair(t_window_boarded,std::tuple(t_window_empty, 8, 3)),
    std::pair(t_door_boarded,std::tuple(t_door_b, 12, 3))
};

// competes with std::find_if
// interpretation is iterating over std::pair<key, ....>
template<class range, class K>
auto linear_search(K key, range origin, range _end) // \todo migrate to zero.h
{
    do if (key == origin->first) return &origin->second;
    while (++origin != _end);
    return decltype(&origin->second)(nullptr);
}

void iuse::hammer(pc& p, item& it)
{
 const auto g = game::active();

 g->draw();
 mvprintz(0, 0, c_red, "Pick a direction in which to pry:");
 const point dir(get_direction(input()));
 if (dir.x == -2) {
  messages.add("Invalid direction!");
  return;
 }

 auto& type = g->m.ter(dir + p.pos);
 auto deconstruct = linear_search(type, std::begin(deconstruct_boarded), std::end(deconstruct_boarded));
 if (!deconstruct) {
     messages.add("Hammers can only remove boards from windows and doors.");
     messages.add("To board up a window or door, press *");
     return;
 }
 p.moves -= 5 * mobile::mp_turn;
 item it_nails(item::types[itm_nail], 0); // assumed pre-apocalypse
 it_nails.charges = std::get<1>(*deconstruct);
 g->m.add_item(p.pos, std::move(it_nails));
 item board(item::types[itm_2x4], 0);
 for (int i = 0; i < std::get<2>(*deconstruct); i++) g->m.add_item(p.pos, board);
 type = std::get<0>(*deconstruct);
}
 
void iuse::light_off(player &p, item& it)
{
    static auto me = []() { return "You turn the flashlight on.";  };
    static auto other = [&]() { return p.subject() + " turns the flashlight on.";  };

    p.if_visible_message(me, other);
    it.make(item::types[itm_flashlight_on]);
    it.active = true;
}

class message_relay
{
    const char* msg;

public:
    message_relay(decltype(msg) msg) noexcept : msg(msg) {}

    void operator()(const GPS_loc& view) {
        game::active()->if_visible_message(msg, view);
    }
    void operator()(const std::pair<pc*, int>& view) { messages.add(msg); }
    void operator()(const std::pair<npc*, int>& view) {
        game::active()->if_visible_message(msg, *(view.first));
    }
};

void iuse::light_on_off(item& it)
{
    const auto g = game::active();
    const auto pos = g->find(it).value();

    // Turning it off
    std::visit(message_relay("The flashlight flicks off."), pos);
    it.make(item::types[itm_flashlight]);
    it.active = false;
}

void iuse::water_purifier(pc& p, item& it)
{
 const auto purify = p.have_item(p.get_invlet("Purify what?"));
 if (!purify.second) {
	 messages.add("You do not have that idea!");
	 return;
 }
 if (purify.second->contents.empty()) {
only_water:
	 messages.add("You can only purify water.");
	 return;
 }
 auto& pure = purify.second->contents[0];
 if (pure.type->id != itm_water && pure.type->id != itm_salt_water) goto only_water;
 pure.make(item::types[itm_water]);
 pure.poison = 0;
}

void iuse::two_way_radio(pc& p, item& it)
{
// TODO: More options here.  Thoughts...
//       > Respond to the SOS of an NPC
//       > Report something to a faction
//       > Call another player
 static constexpr const char* radio_options[] = {
     "1: Radio a faction for help...",
     "2: Call Acquaitance...", // not implemented properly
     "3: General S.O.S.",
     "0: Cancel"
 };
 constexpr const int radio_height = sizeof(radio_options) / sizeof(*radio_options) + 2;

 // maximum string length...possible function target
 // C++20: std::ranges::max?
 int radio_width = 0;
 for (const char* const line : radio_options) radio_width = cataclysm::max(radio_width, strlen(line));
 radio_width += 5; // historical C:Whales

 const auto g = game::active();
 WINDOW* w = newwin(radio_height, radio_width, (VIEW - radio_height) / 2, (SCREEN_WIDTH - radio_width) / 2);
 wborder(w, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
            LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
// TODO: More options here.  Thoughts...
//       > Respond to the SOS of an NPC
//       > Report something to a faction
//       > Call another player
 int row = 0;
 for (const char* const line : radio_options) mvwaddstrz(w, ++row, 1, c_white, line);
 wrefresh(w);
 int ch = getch();
 if (ch == '1') {
  p.moves -= 3 * mobile::mp_turn;
  faction* fac = g->list_factions("Call for help...");
  if (fac == nullptr) {
   it.charges++;
   return;
  }
  int bonus = 0;
  if (fac->goal == FACGOAL_CIVILIZATION) bonus += 2;
  if (fac->has_job(FACJOB_MERCENARIES)) bonus += 4;
  if (fac->has_job(FACJOB_DOCTORS)) bonus += 2;
  if (fac->has_value(FACVAL_CHARITABLE)) bonus += 3;
  if (fac->has_value(FACVAL_LONERS)) bonus -= 3;
  if (fac->has_value(FACVAL_TREACHERY)) bonus -= rng(0, 8);
  bonus += fac->respects_u + 3 * fac->likes_u;
  if (bonus >= 25) {
   popup("They reply, \"Help is on the way!\"");
   g->add_event(EVENT_HELP, int(messages.turn) + fac->response_time(g->lev), fac->id, -1, -1);
   fac->respects_u -= rng(0, 8);
   fac->likes_u -= rng(3, 5);
  } else if (bonus >= -5) {
   popup("They reply, \"Sorry, you're on your own!\"");
   fac->respects_u -= rng(0, 5);
  } else {
   popup("They reply, \"Hah!  We hope you die!\"");
   fac->respects_u -= rng(1, 8);
  }

 } else if (ch == '2') {	// Call Acquaitance
// TODO: Implement me!
 } else if (ch == '3') {	// General S.O.S.
  p.moves -= (3 * mobile::mp_turn) / 2;
  const OM_loc my_om = overmap::toOvermap(g->u.GPSpos);
  std::vector<npc*> in_range;
  for (auto& _npc : g->cur_om.npcs) {
    if (4 > _npc.op_of_u.value) continue;
	if (30 >= rl_dist(overmap::toOvermap(_npc.GPSpos), my_om)) in_range.push_back(&_npc);
  }
  const auto ub = in_range.size();
  if (0 < ub) {
   npc* coming = in_range[rng(0, ub - 1)];
   popup("A reply!  %s says, \"I'm on my way; give me %d minutes!\"",
         coming->name.c_str(), coming->minutes_to_u(g->u));
   coming->mission = NPC_MISSION_RESCUE_U;
  } else
   popup("No-one seems to reply...");
 } else
  it.charges++;	// Canceled the call, get our charge back
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh();
}
 
void iuse::radio_off(pc& p, item& it)
{
  messages.add("You turn the radio on.");
  it.make(item::types[itm_radio_on]);
  it.active = true;
}

void iuse::radio_on(player* p, item* it, bool t)
{
 static constexpr const int RADIO_PER_TURN = 25;
 const auto g = game::active();

 if (t) {	// Normal use
  int best_signal; // backward compatibility
  std::string message;
  std::tie(best_signal, message) = g->cur_om.best_radio_signal(overmap::toOvermapHires(p->GPSpos));
  if (best_signal > 0) {
   for (int j = 0; j < message.length(); j++) {
    if (dice(10, 100) > dice(10, best_signal * 3)) message[j] = one_in(10) ? char(rng('a', 'z')) : '#';
   }

   std::vector<std::string> segments;
   while (message.length() > RADIO_PER_TURN) {
    int spot = message.find_last_of(' ', RADIO_PER_TURN);
    if (spot == std::string::npos) spot = RADIO_PER_TURN;
    segments.push_back( message.substr(0, spot) );
    message = message.substr(spot + 1);
   }
   segments.push_back(message);
   int index = messages.turn % (segments.size());
   std::ostringstream messtream;
   messtream << "radio: " << segments[index];
   message = messtream.str();
  }
  g->sound(g->find_item(it).value(), 6, message.c_str());
 } else {	// Turning it off
  messages.add("The radio dies.");
  it->make(item::types[itm_radio]);
  it->active = false;
 }
}

void iuse::crowbar(pc& p, item& it)
{
 const auto g = game::active();

 g->draw();
 mvprintw(0, 0, "Pry where?");
 const point dir(get_direction(input()));
 if (dir.x == -2) {
  messages.add("Invalid direction.");
  return;
 }

 auto& type = g->m.ter(dir + p.pos);
 if (type == t_door_c || type == t_door_locked || type == t_door_locked_alarm) {
  if (dice(4, 6) < dice(4, p.str_cur)) {
   messages.add("You pry the door open.");
   p.moves -= (mobile::mp_turn / 2) * 3 - (p.str_cur * (mobile::mp_turn / 20));
   type = t_door_o;
  } else {
   messages.add("You pry, but cannot open the door.");
   p.moves -= mobile::mp_turn;
  }
 } else if (type == t_manhole_cover) {
  if (dice(8, 8) < dice(8, p.str_cur)) {
   messages.add("You lift the manhole cover.");
   p.moves -= 5 * mobile::mp_turn - (p.str_cur * (mobile::mp_turn / 20));
   type = t_manhole;
   g->m.add_item(p.pos, item::types[itm_manhole_cover], 0);
  } else {
   messages.add("You pry, but cannot lift the manhole cover.");
   p.moves -= mobile::mp_turn;
  }
 } else if (type == t_crate_c) {
  if (p.str_cur >= rng(3, 30)) {
   messages.add("You pop the crate open.");
   p.moves -= (mobile::mp_turn / 2) * 3 - (p.str_cur * (mobile::mp_turn / 20));
   type = t_crate_o;
  } else {
   messages.add("You pry, but cannot open the crate.");
   p.moves -= mobile::mp_turn;
  } 
 } else {
  auto deconstruct = linear_search(type, std::begin(deconstruct_boarded), std::end(deconstruct_boarded));
  if (!deconstruct) {
      messages.add("There's nothing to pry there.");
      return;
  }
  p.moves -= 5 * mobile::mp_turn;
  item it_nails(item::types[itm_nail], 0); // assumed pre-apocalypse
  it_nails.charges = std::get<1>(*deconstruct);
  g->m.add_item(p.pos, std::move(it_nails));
  item board(item::types[itm_2x4], 0);
  for (int i = 0; i < std::get<2>(*deconstruct); i++)
   g->m.add_item(p.pos, board);
  type = std::get<0>(*deconstruct);
 }
}

void iuse::makemound(player *p, item *it, bool t)
{
 decltype(auto) terrain = p->GPSpos.ter();
 if (is<diggable>(terrain)) {
  messages.add("You churn up the earth here.");
  p->moves -= 3 * mobile::mp_turn;
  terrain = t_dirtmound;
 } else
  messages.add("You can't churn up this ground.");
}

void iuse::dig(pc& p, item& it)
{
 messages.add("You can dig a pit via the construction menu--hit *");
}

void iuse::chainsaw_off(player& p, item& it)
{
    static auto me_fail = []() { return "You yank the cord, but nothing happens.";  };
    static auto other_fail = [&]() { return grammar::capitalize(p.pronoun(grammar::noun::role::subject)) + " yanks the cord, but nothing happens.";  };

    p.moves -= (mobile::mp_turn / 5) * 4;
    if (rng(0, 10) - it.damage > 5 && it.charges > 0) {
        game::active()->sound(p.pos, 20, "With a roar, the chainsaw leaps to life!");
        it.make(item::types[itm_chainsaw_on]);
        it.active = true;
    } else
        p.if_visible_message(me_fail, other_fail);
}

void iuse::chainsaw_on(player& p, item& it)
{
 // Effects while simply on
 if (one_in(15)) game::active()->sound(p.pos, 12, grammar::capitalize(p.pronoun(grammar::noun::role::possessive)) + " chainsaw rumbles.");
}

void iuse::chainsaw_on_turnoff(player& p, item& it)
{
    static auto me = []() { return "Your chainsaw dies.";  };
    static auto other = [&]() { return p.possessive() + " chainsaw dies.";  };

    p.if_visible_message(me, other);
    it.make(item::types[itm_chainsaw_off]);
    it.active = false;
}

void iuse::jackhammer(pc& p, item& it)
{
 const auto g = game::active();

 g->draw();
 mvprintw(0, 0, "Drill in which direction?");
 point dir(get_direction(input()));
 if (dir.x == -2) {
  messages.add("Invalid direction.");
  return;
 }
 const auto dest = dir + p.pos;

 if (g->m.is_destructable(dest)) {
  g->m.destroy(g, dest.x, dest.y, false);
  p.moves -= 5 * mobile::mp_turn;
  g->sound(dir, 45, "TATATATATATATAT!");
 } else {
  messages.add("You can't drill there.");
  it.charges += it.is_tool()->charges_per_use;
 }
}

void iuse::set_trap(pc& p, item& it)
{
 const auto g = game::active();

 g->draw();
 mvprintw(0, 0, "Place where?");
 point dir(get_direction(input()));
 if (dir.x == -2) {
  messages.add("Invalid direction.");
  return;
 }
 point trap_pos(dir + p.pos);
 if (g->m.move_cost(trap_pos) != 2) {
  messages.add("You can't place a %s there.", it.tname().c_str());
  return;
 }

 static auto err_bury = [&]() {
     if (!p.has_amount(itm_shovel, 1)) return std::optional(std::string("You need a shovel."));
     else if (!g->m.has_flag(diggable, trap_pos)) return std::optional(std::string("You can't dig in that ") + name_of(g->m.ter(trap_pos)));
     return std::optional<std::string>(std::nullopt);
 };

 trap_id type = tr_null;
 bool buried = false;
 std::ostringstream message;
 int practice;

 switch (it.type->id) {
 case itm_boobytrap:
  message << "You set the boobytrap up and activate the grenade.";
  type = tr_boobytrap;
  practice = 4;
  break;
 case itm_bubblewrap:
  message << "You set the bubblewrap on the ground, ready to be popped.";
  type = tr_bubblewrap;
  practice = 2;
  break;
 case itm_beartrap:
  buried = (!err_bury() && query_yn("Bury the beartrap?"));
  type = (buried ? tr_beartrap_buried : tr_beartrap);
  message << "You " << (buried ? "bury" : "set") << " the beartrap.";
  practice = (buried ? 7 : 4); 
  break;
 case itm_board_trap:
  message << "You set the board trap on the " << name_of(g->m.ter(trap_pos)) << ", nails facing up.";
  type = tr_nailboard;
  practice = 2;
  break;
 case itm_tripwire:
// Must have a connection between solid squares.
  if (   (g->m.move_cost(trap_pos + Direction::N) != 2 && g->m.move_cost(trap_pos + Direction::S) != 2)
      || (g->m.move_cost(trap_pos + Direction::E) != 2 && g->m.move_cost(trap_pos + Direction::W) != 2)
      || (g->m.move_cost(trap_pos + Direction::NW) != 2 && g->m.move_cost(trap_pos + Direction::SE) != 2)
      || (g->m.move_cost(trap_pos + Direction::NE) != 2 && g->m.move_cost(trap_pos + Direction::SW) != 2)) {
   message << "You string up the tripwire.";
   type= tr_tripwire;
   practice = 3;
  } else {
   messages.add("You must place the tripwire between two solid tiles.");
   return;
  }
  break;
 case itm_crossbow_trap:
  message << "You set the crossbow trap.";
  type = tr_crossbow;
  practice = 4;
  break;
 case itm_shotgun_trap:
  message << "You set the shotgun trap.";
  type = tr_shotgun_2;
  practice = 5;
  break;
 case itm_blade_trap:
  trap_pos += dir;
  for (int i = -1; i <= 1; i++) {
   for (int j = -1; j <= 1; j++) {
    if (g->m.move_cost(trap_pos.x + i, trap_pos.y + j) != 2) {
     messages.add("That trap needs a 3x3 space to be clear, centered two tiles from you.");
     return;
    }
   }
  }
  message << "You set the blade trap two squares away.";
  type = tr_engine;
  practice = 12;
  break;
 case itm_landmine:
  buried = true;
  message << "You bury the landmine.";
  type = tr_landmine;
  practice = 7;
  break;
 default:
  messages.add("Tried to set a trap.  But got confused! %s", it.tname().c_str());
  return;
 }

 if (buried) {
     if (const auto err = err_bury()) {
         messages.add(*err);
         return;
     }
 }

 messages.add(message.str());
 p.practice(sk_traps, practice);
 g->m.add_trap(trap_pos, type);
 p.moves -= mobile::mp_turn + practice * (mobile::mp_turn/4);
 if (type == tr_engine) {
  for (decltype(auto) delta : Direction::vector) g->m.add_trap(trap_pos + delta, tr_blade);
 }
 it.invlet = 0; // Remove the trap from the player's inv
}

void iuse::geiger(player *p, item *it, bool t)
{
 const auto g = game::active();

 if (t) { // Every-turn use when it's on
  int rads = g->m.radiation(p->GPSpos);
  if (rads == 0) return;
  g->sound(p->pos, 6, "");
  if (rads > 50) messages.add("The geiger counter buzzes intensely.");
  else if (rads > 35) messages.add("The geiger counter clicks wildly.");
  else if (rads > 25) messages.add("The geiger counter clicks rapidly.");
  else if (rads > 15) messages.add("The geiger counter clicks steadily.");
  else if (rads > 8) messages.add("The geiger counter clicks slowly.");
  else if (rads > 4) messages.add("The geiger counter clicks intermittantly.");
  else messages.add("The geiger counter clicks once.");
  return;
 }
// Otherwise, we're activating the geiger counter
 const auto type = it->is_tool();
 assert(type);

 if (itm_geiger_on == type->id) {
  messages.add("The geiger counter's SCANNING LED flicks off.");
  it->make(item::types[itm_geiger_off]);
  it->active = false;
  return;
 }
 int ch = menu("Geiger counter:", { "Scan yourself", "Scan the ground", "Turn continuous scan on", "Cancel" });
 switch (ch) {
  case 1: messages.add("Your radiation level: %d", p->radiation); break;
  case 2: messages.add("The ground's radiation level: %d", g->m.radiation(p->GPSpos));
	  break;
  case 3:
   messages.add("The geiger counter's scan LED flicks on.");
   it->make(item::types[itm_geiger_on]);
   it->active = true;
   break;
  case 4:
   it->charges++;
   break;
 }
}

void iuse::teleport(player& p, item& it)
{
 p.moves -= mobile::mp_turn;
 game::active()->teleport(&p);
}

void iuse::can_goo(player& p, item& it)
{
 const auto g = game::active();

 it.make(item::types[itm_canister_empty]);

 static std::function<point()> where_is = [&]() {return p.pos + rng(within_rldist<2>); };
 static std::function<bool(const point&)> ok = [&](const point& dest) {return 0 < g->m.move_cost(dest); };

 auto goo_pos = LasVegasChoice(10, where_is, ok);
 if (!goo_pos) return;
 bool u_see = (bool)g->u.see(*goo_pos);
 if (u_see) messages.add("Living black goo emerges from the canister!");

 if (monster* const m_at = g->mon(*goo_pos)) {
     m_at->hit_by_blob();  // \todo handle robots ignoring this, etc.
     // \todo? yes, still hostile even if blob-converted ... does this make sense?
 } else {
  monster goo(mtype::types[mon_blob], *goo_pos);
  goo.make_friendly(p);
  g->z.push_back(std::move(goo));
 }

 // Unsure whether C:Whales is reasonable behavior (accept: can move through, *or* already has trap)
 static std::function<bool()> continue_ok = []() {return !one_in(4); };
 static std::function<bool(const point&)> ok_for_trap = [&](const point& dest) {return 0 < g->m.move_cost(dest) || tr_null != g->m.tr_at(dest); };

 goo_pos = LasVegasChoice(10, where_is, ok_for_trap, continue_ok);
 if (goo_pos) {
     if (g->u.see(*goo_pos)) messages.add("A nearby splatter of goo forms into a goo pit.");
     g->m.tr_at(*goo_pos) = tr_goo;
 };
}

static std::pair<itype_id, std::tuple<std::string, std::string, itype_id, int > > activate_spec[] = {
    std::pair(itm_pipebomb, std::tuple("light", "the fuse on the pipe bomb." , itm_pipebomb_act, 3)),
    std::pair(itm_grenade, std::tuple("pull", "the pin on the grenade." , itm_grenade_act, 5)),
    std::pair(itm_flashbang, std::tuple("pull", "the pin on the flashbang." , itm_flashbang_act, 5)),
    std::pair(itm_EMPbomb, std::tuple("pull", "the pin on the EMP grenade." , itm_EMPbomb_act, 3)),
    std::pair(itm_gasbomb, std::tuple("pull", "the pin on the teargas canister." , itm_gasbomb_act, 20)),
    std::pair(itm_smokebomb, std::tuple("pull", "the pin on the smoke bomb." , itm_smokebomb_act, 20)),
    std::pair(itm_molotov, std::tuple("light", "the molotov cocktail." , itm_molotov_lit, 1)),
    std::pair(itm_acidbomb, std::tuple("remove", "the divider, and the chemicals mix." , itm_acidbomb_act, 1)),
    std::pair(itm_dynamite, std::tuple("light", "the dynamite." , itm_dynamite_act, 20)),
    std::pair(itm_mininuke, std::tuple("activate", "the mininuke." , itm_mininuke_act, 10))
};

static void activate(player& p, item& it)
{
    auto record = linear_search(it.type->id, std::begin(activate_spec), std::end(activate_spec));
    if (!record) throw std::string("Unconfigured item for activate: ") + JSON_key(itype_id(it.type->id));    // invariant failure

    static auto msg = [&]() {return grammar::capitalize(p.subject()) + " " + p.regular_verb_agreement(std::get<0>(*record)) + " " + std::get<1>(*record); };

    p.if_visible_message(msg, msg); // \todo?  special-case this?
    it.make(item::types[std::get<2>(*record)]);
    it.charges = std::get<3>(*record);
    it.active = true;
}

void iuse::pipebomb(player& p, item& it)
{
    activate(p, it);
    p.use_charges(itm_lighter, 1);
}

void iuse::pipebomb_act(item& it)
{
    const auto g = game::active();
    const auto pos = g->find_item(&it).value();

    g->sound(pos, 0, "Ssssss"); // Vol 0 = only heard if you hold it
}

void iuse::pipebomb_act_explode(item& it)
{
    const auto g = game::active();
    const auto pos = g->find_item(&it).value();

    // The timer has run down
    if (one_in(10) && g->u.see(pos)) // \todo? allow fizzling out when not in sight?
        messages.add("The pipe bomb fizzles out.");
    else
        g->explosion(pos, rng(6, 14), rng(0, 4), false);
}

void iuse::grenade(player& p, item& it) { activate(p, it); }

void iuse::grenade_act(item& it)
{
    const auto g = game::active();
    const auto pos = g->find_item(&it).value();

    g->sound(pos, 0, "Tick.");	// Vol 0 = only heard if you hold it
}

void iuse::grenade_act_explode(item& it)
{
    const auto g = game::active();
    const auto pos = g->find_item(&it).value();

    g->explosion(pos, 12, 28, false); // When that timer runs down...
}

void iuse::flashbang(player& p, item& it) { activate(p, it); }

void iuse::flashbang_act(item& it)
{
 const auto g = game::active();
 const auto pos = g->find_item(&it).value();

 g->sound(pos, 0, "Tick.");	// Vol 0 = only heard if you hold it
}

void iuse::flashbang_act_explode(item& it)
{
    const auto g = game::active();
    const auto pos = g->find_item(&it).value();

    g->flashbang(pos); // When that timer runs down...
}

void iuse::c4(pc& p, item& it)
{
 int time = query_int("Set the timer to (0 to cancel)?");
 if (time == 0) {
  messages.add("Never mind.");
  return;
 }
 messages.add("You set the timer to %d.", time);
 it.make(item::types[itm_c4armed]);
 it.charges = time;
 it.active = true;
}

void iuse::c4armed(item& it)
{
    const auto g = game::active();
    const auto pos = g->find_item(&it).value();

    g->sound(pos, 0, "Tick.");	// Vol 0 = only heard if you hold it
}

void iuse::c4armed_explode(item& it)
{
    const auto g = game::active();
    const auto pos = g->find_item(&it).value();

    g->explosion(pos, 40, 3, false); // When that timer runs down...
}

void iuse::EMPbomb(player& p, item& it) { activate(p, it); }

void iuse::EMPbomb_act(item& it)
{
    const auto g = game::active();
    const auto pos = g->find_item(&it).value();

    g->sound(pos, 0, "Tick.");	// Vol 0 = only heard if you hold it
}

void iuse::EMPbomb_act_explode(item& it)
{
    const auto g = game::active();
    const auto pos = g->find_item(&it).value();

    static auto explode = [&](point pt) { g->emp_blast(pt.x, pt.y); };
    forall_do_inclusive(pos + within_rldist<4>, explode);
}

void iuse::gasbomb(player& p, item& it) { activate(p, it); }

void iuse::gasbomb_act(item& it)
{
    const auto g = game::active();
    const auto pos = g->find_item(&it).value();

    if (it.charges > 15) g->sound(pos, 0, "Tick.");	// Vol 0 = only heard if you hold it
    else {
        static auto contaminate = [&](point dest) {
            if (g->m.sees(pos, dest, 2) && g->m.move_cost(dest) > 0)
                g->m.add_field(g, dest, fd_tear_gas, 3);
        };

        forall_do_inclusive(pos + within_rldist<2>, contaminate);
    }
}

void iuse::gasbomb_act_off(item& it)
{
    it.make(item::types[itm_canister_empty]);
}

void iuse::smokebomb(player& p, item& it) { activate(p, it); }

void iuse::smokebomb_act(item& it)
{
    const auto g = game::active();
    const auto pos = g->find_item(&it).value();

    if (it.charges > 17) g->sound(pos, 0, "Tick.");	// Vol 0 = only heard if you hold it
    else {
        static auto contaminate = [&](point dest) {
            if (g->m.sees(pos, dest, 2) && g->m.move_cost(dest) > 0)
                g->m.add_field(g, dest, fd_smoke, rng(1, 2) + rng(0, 1));
        };

        forall_do_inclusive(pos + within_rldist<2>, contaminate);
    }
}

void iuse::smokebomb_act_off(item& it)
{
    it.make(item::types[itm_canister_empty]);
}

void iuse::acidbomb(player& p, item& it)
{
    activate(p, it);

    p.moves -= (3 * mobile::mp_turn) / 2;
    it.bday = int(messages.turn);
}
 
void iuse::acidbomb_act(item& it)
{
    const auto g = game::active();
    const auto where_is = g->find(it).value(); // invariant violation if this fails

    if (auto loc = std::get_if<GPS_loc>(&where_is)) { // not held by anyone
        if (const auto pos = g->toScreen(*loc)) {
            it.charges = 0;
            static auto contaminate = [&](point pt) {
                g->m.add_field(g, pt, fd_acid, 3);
            };

            forall_do_inclusive(*pos + within_rldist<1>, contaminate);
        }
    }
}

void iuse::molotov(player& p, item& it)
{
    activate(p, it);

    p.use_charges(itm_lighter, 1);
    p.moves -= (3 * mobile::mp_turn) / 2;
    it.bday = int(messages.turn);
}
 
class burn_molotov
{
    item& it;

public:
    burn_molotov(item& it) noexcept : it(it) {}

    void operator()(const GPS_loc& loc) {
        const auto g = game::active();

        if (const auto pos = g->toScreen(loc)) {
            it.charges = -1;
            g->explosion(*pos, 8, 0, true);
        }
    }

    void operator()(const std::pair<pc*, int>& who) {
        int age = int(messages.turn) - it.bday;
        if (5 <= age && rng(1, 50) < age) {
            messages.add("Your lit molotov goes out.");
            it.make(item::types[itm_molotov]);
            it.charges = 0;
            it.active = false;
        }
    }
    void operator()(const std::pair<npc*, int>& who) {
        int age = int(messages.turn) - it.bday;
        if (5 <= age && rng(1, 50) < age) {
            who.first->if_visible_message((who.first->possessive()+ " lit molotov goes out.").c_str());
            it.make(item::types[itm_molotov]);
            it.charges = 0;
            it.active = false;
        }
    }
};

void iuse::molotov_lit(item& it)
{
 const auto where_is = game::active()->find(it).value(); // invariant violation if this fails

 std::visit(burn_molotov(it), where_is);
}

void iuse::dynamite(player& p, item& it)
{
    activate(p, it);
    p.use_charges(itm_lighter, 1);
}

void iuse::dynamite_act(item& it)
{
    const auto g = game::active();
    const auto pos = g->find_item(&it).value();
    g->sound(pos, 0, "ssss...");	 // Simple timer effects
}

void iuse::dynamite_act_off(item& it)
{
    const auto g = game::active();
    const auto pos = g->find_item(&it).value();

    g->explosion(pos, 60, 0, false);		// When that timer runs down...
}

void iuse::mininuke(player& p, item& it) { activate(p, it); }

void iuse::mininuke_act(item& it)
{
    const auto g = game::active();
    const auto pos = g->find_item(&it).value();

    g->sound(pos, 2, "Tick."); 	// Simple timer effects
}

void iuse::mininuke_act_off(item& it)
{
    const auto g = game::active();
    const auto pos = g->find_item(&it).value();

    // When that timer runs down...
    g->explosion(pos, 200, 0, false);

    static auto contaminate = [&](point dest) {
        if (g->m.sees(pos, dest, 3) && g->m.move_cost(dest) > 0) g->m.add_field(g, dest, fd_nuke_gas, 3);
    };
    forall_do_inclusive(pos + within_rldist<3>, contaminate);
}

static auto _can_use_pheromone(const player& p)
{
    const auto pos(p.pos);

    const auto g = game::active();

    std::vector<monster*> valid;	// Valid targets
    for (int x = pos.x - 4; x <= pos.x + 4; x++) {
        for (int y = pos.y - 4; y <= pos.y + 4; y++) {
            if (monster* const m_at = g->mon(x, y)) {
                if ('Z' == m_at->symbol() && m_at->is_enemy(&p)) valid.push_back(m_at);
            }
        }
    }

    return valid;
}

std::optional<std::any> iuse::can_use_pheromone(const npc& p)
{
    auto valid(_can_use_pheromone(p));

    if (valid.empty()) return std::nullopt;
    return valid;
}

void iuse::pheromone(pc& p, item& it)
{
 const auto g = game::active();

 messages.add("You squeeze the pheromone ball...");

 auto valid(_can_use_pheromone(p));

 p.moves -= (3 * mobile::mp_turn) / 20;

 int converts = 0;
 for (decltype(auto) m_at : valid) {
     if (rng(0, 500) > m_at->hp) {
         converts++;
         m_at->make_friendly();
     }
 }

  if (converts == 0)
   messages.add("...but nothing happens.");
  else if (converts == 1)
   messages.add("...and a nearby zombie turns friendly!");
  else
   messages.add("...and several nearby zombies turn friendly!");
}

void iuse::pheromone(npc& p, item& it)
{
    const bool seen = p.if_visible_message((p.name + " squeezes a pheromone ball...").c_str());

    decltype(auto) valid = *std::any_cast<std::vector<monster*> >(&(*it._AI_relevant)); // Valid targets

    p.moves -= (3 * mobile::mp_turn) / 20;

    int converts = 0;
    for (decltype(auto) m_at : valid) {
        if (rng(0, 500) > m_at->hp) {
            converts++;
            m_at->make_friendly(p);
        }
    }

    if (seen) {
        p.if_visible_message(0 == converts ? "...but nothing happens." :
            (1 == converts ? "...and a nearby zombie turns friendly!" :
                "...and several nearby zombies turn friendly!"));
    } else {
        p.if_visible_message(0 == converts ? nullptr :
            (1 == converts ? "A nearby zombie turns friendly!" :
                "Several nearby zombies turn friendly!"));
    }
}

void iuse::portal(player& p, item& it)
{
 game::active()->m.add_trap(p.pos + rng(within_rldist<2>), tr_portal);
}

static auto _can_use_manhack(const player& p)
{
    const auto g = game::active();

    std::vector<point> valid;	// Valid spawn locations
    for (decltype(auto) delta : Direction::vector) {
        const auto pt(p.pos + delta);
        if (g->is_empty(pt)) valid.push_back(pt);
    }

    return valid;
}

std::optional<std::any> iuse::can_use_manhack(const npc& p)
{
    auto valid(_can_use_manhack(p));

    if (valid.empty()) return std::nullopt;
    return valid;
}

static void _use_manhack(player& p, item& it, const point& dest)
{
    const auto g = game::active();

    p.moves -= (3 * mobile::mp_turn) / 5;
    it.invlet = 0; // Remove the manhack from the player's inv
    monster manhack(mtype::types[mon_manhack], dest);
    if (rng(0, p.int_cur / 2) + p.sklevel[sk_electronics] / 2 + p.sklevel[sk_computer] >= rng(0, 4)) manhack.make_ally(p);
    else {
        p.subjective_message("You misprogram the manhack; it's hostile!");
        manhack.make_threat(p); // misprogrammed, hostile
    }
    g->z.push_back(manhack);
}

void iuse::manhack(pc& p, item& it)
{
    auto valid(_can_use_manhack(p));
    if (valid.empty()) {	// No valid points!
        messages.add("There is no adjacent square to release the manhack in!");
        return;
    }

    _use_manhack(p, it, valid[rng(0, valid.size() - 1)]);
}

void iuse::manhack(npc& p, item& it)
{
    decltype(auto) valid = *std::any_cast<std::vector<point> >(&(*it._AI_relevant)); // Valid spawn locations

    _use_manhack(p, it, valid[rng(0, valid.size() - 1)]);
    it.clear_relevance();
}

void iuse::turret(pc& p, item& it)
{
 const auto g = game::active();

 g->draw();
 mvprintw(0, 0, "Place where?");
 point dir(get_direction(input()));
 if (dir.x == -2) {
  messages.add("Invalid direction.");
  return;
 }
 p.moves -= mobile::mp_turn;
 auto dest(dir + p.pos);

 if (!g->is_empty(dest)) {
  messages.add("You cannot place a turret there.");
  return;
 }
 it.invlet = 0; // Remove the turret from the player's inv
 monster turret(mtype::types[mon_turret], dest);
 if (rng(0, p.int_cur / 2) + p.sklevel[sk_electronics] / 2 +
     p.sklevel[sk_computer] < rng(0, 6))
  messages.add("You misprogram the turret; it's hostile!");
 else
  turret.friendly = -1;
 g->z.push_back(std::move(turret));
}

void iuse::UPS_off(player& p, item& it)
{
    static auto pc_on = []() { return std::string("You turn the power supply on."); };
    static auto npc_on = [&]() { return grammar::capitalize(p.subject()) + " turns the power supply on."; };

    p.if_visible_message(pc_on, npc_on);
    if (p.is_wearing(itm_goggles_nv)) p.subjective_message("Your light amp goggles power on.");
    it.make(item::types[itm_UPS_on]);
    it.active = true;
}
 
void iuse::UPS_on_off(player& p, item& it)
{
  p.if_visible_message("The UPS powers off with a soft hum."); // \todo? should be if_audible_message or g->sound
  it.make(item::types[itm_UPS_off]);
  it.active = false;
}

class is_friend_of
{
    const npc& whom;

public:
    is_friend_of(const npc& whom) noexcept : whom(whom) {}
    ~is_friend_of() = default;

    bool operator()(monster* m) { return m->is_friend(&whom); }
    bool operator()(npc* p) { return whom.is_friend(p); }
    bool operator()(pc* p) { return whom.is_friend(p); }
};

std::optional<std::any> iuse::can_use_tazer(const npc& p)
{
    const auto g = game::active();
    std::vector<std::remove_reference_t<decltype(*(g->mob_at(p.GPSpos)))> > ret;

    for (decltype(auto) dir : Direction::vector) {
        const auto target(dir + p.GPSpos);
        if (auto who = g->mob_at(target)) {
            if (!std::visit(is_friend_of(p), *who)) {
                if (std::holds_alternative<monster*>(*who)) // don't have complete implementation yet
                    ret.push_back(std::move(*who));
            }
        }
    }
    if (ret.empty()) return std::nullopt;
    return ret;
}

static constexpr const int tazer_hit_modifier[] = { -2, -1, 0, 2, 4 };
static_assert(mtype::MS_MAX == std::end(tazer_hit_modifier) - std::begin(tazer_hit_modifier));

void iuse::tazer(pc& p, item& it)
{
    const auto g = game::active();

    g->draw();
    mvprintw(0, 0, "Shock in which direction?");
    point dir(get_direction(input()));
    if (dir.x == -2) throw std::string("Invalid direction.");

    const auto target(dir + p.GPSpos);
    monster* const z = g->mon(target);
    npc* const foe = g->nPC(target);
    if (!z && !foe) {
        messages.add("Your tazer crackles in the air."); // XXX no time cost for this?
        return;
    }

    int numdice = 3 + (p.dex_cur / 2.5) + p.sklevel[sk_melee] * 2;
    p.moves -= mobile::mp_turn;

    if (z) {
        numdice += tazer_hit_modifier[z->type->size];
        if (dice(numdice, 10) < dice(z->dodge(), 10)) {	// A miss!
            messages.add("You attempt to shock the %s, but miss.", z->name().c_str());
            return;
        }
        messages.add("You shock the %s!", z->name().c_str());
        int shock = rng(5, 25);
        z->moves -= shock * mobile::mp_turn;
        if (z->hurt(shock)) g->kill_mon(*z, &p);
        return;
    }

    if (foe) {
        if (foe->attitude != NPCATT_FLEE) foe->attitude = NPCATT_KILL;
        if (foe->str_max >= 17) numdice++;	// Minor bonus against huge people
        else if (foe->str_max <= 5) numdice--;	// Minor penalty against tiny people
        if (dice(numdice, 10) <= foe->dodge_roll()) {
            messages.add("You attempt to shock %s, but miss.", foe->name.c_str());
            return;
        }
        messages.add("You shock %s!", foe->name.c_str());
        int shock = rng(5, 20);
        foe->moves -= shock * mobile::mp_turn;
        foe->hurtall(shock);
        if (foe->hp_cur[hp_head] <= 0 || foe->hp_cur[hp_torso] <= 0) foe->die(g, true);
    }
}

void iuse::tazer(npc& p, item& it)
{
    const auto g = game::active();

    auto test = std::any_cast<std::vector<std::remove_reference_t<decltype(*(g->mob_at(p.GPSpos)))> > >(&(*it._AI_relevant));
    decltype(auto) threat = (*test)[rng(0, test->size() - 1)];

    int numdice = 3 + (p.dex_cur / 2.5) + p.sklevel[sk_melee] * 2;
    p.moves -= mobile::mp_turn;

    if (auto zz = std::get_if<monster*>(&threat)) {
        const auto z = *zz; // backward compatibility
        numdice += tazer_hit_modifier[z->type->size];
        static auto miss_msg = [&]() { return p.subject() + " attempts to shock " + z->desc(grammar::noun::role::direct_object, grammar::article::definite) + ", but misses."; };
        static auto hit_msg = [&]() { return p.subject() + " shocks " + z->desc(grammar::noun::role::direct_object, grammar::article::definite) + "!"; };

        if (dice(numdice, 10) < dice(z->dodge(), 10)) {	// A miss!
            p.if_visible_message(nullptr, miss_msg);
            return;
        }
        p.if_visible_message(nullptr, hit_msg);
        int shock = rng(5, 25);
        z->moves -= shock * mobile::mp_turn;
        if (z->hurt(shock)) g->kill_mon(*z, &p);
        it.clear_relevance();
        return;
    }
/*
    if (foe) {
        if (foe->attitude != NPCATT_FLEE) foe->attitude = NPCATT_KILL;
        if (foe->str_max >= 17) numdice++;	// Minor bonus against huge people
        else if (foe->str_max <= 5) numdice--;	// Minor penalty against tiny people
        if (dice(numdice, 10) <= foe->dodge_roll()) {
            messages.add("You attempt to shock %s, but miss.", foe->name.c_str());
            return;
        }
        messages.add("You shock %s!", foe->name.c_str());
        int shock = rng(5, 20);
        foe->moves -= shock * mobile::mp_turn;
        foe->hurtall(shock);
        if (foe->hp_cur[hp_head] <= 0 || foe->hp_cur[hp_torso] <= 0) foe->die(g, true);
    }
*/
    it.clear_relevance();
}

// \todo allow NPCs to use this to fix morale
void iuse::mp3(player& p, item& it)
{
 if (p.has_active_item(itm_mp3_on))
  p.subjective_message("You are already listening to an mp3 player!");
 else {
     static auto pc_play = []() { return std::string("You put in the earbuds and start listening to music."); };
     static auto npc_play = [&]() { return grammar::capitalize(p.desc(grammar::noun::role::subject) + " puts in the earbuds and start listening to music."); };
     p.if_visible_message(pc_play, npc_play);
     it.make(item::types[itm_mp3_on]);
     it.active = true;
 }
}

void iuse::mp3_on(player& p, item& it)
{
  if (!p.has_item(&it)) return;	// We're not carrying it!
  p.add_morale(MORALE_MUSIC, 1, 50);

  if (0 == int(messages.turn) % 10) {	// Every 10 turns, describe the music
   std::string sound;
   switch (rng(1, 10)) {
    case 1: sound = "a sweet guitar solo!";	p.stim++;	break;
    case 2: sound = "a funky bassline.";			break;
    case 3: sound = "some amazing vocals.";			break;
    case 4: sound = "some pumping bass.";			break;
    case 5: sound = "dramatic classical music.";
      if (p.int_cur >= 10) p.add_morale(MORALE_MUSIC, 1, 100);
	  break;
   }
   if (!sound.empty()) p.subjective_message(std::string("You listen to ") + sound);
  }
}

void iuse::mp3_on_turnoff(player& p, item& it)
{
    p.subjective_message("The mp3 player turns off.");
    it.make(item::types[itm_mp3]);
    it.active = false;
}

// Vortex stones are only found in the spiral map special,
// which has the mine finale *above* it.  PC-only until NPCs
// can navigate here.
void iuse::vortex(pc& p, item& it)
{
 const auto g = game::active();

 std::vector<point> spawn;
 for (int i = -3; i <= 3; i++) {
  point test(p.pos.x - 3, p.pos.y+i);
  if (g->is_empty(test)) spawn.push_back(test);
  test = point(p.pos.x + 3, p.pos.y + i);
  if (g->is_empty(test)) spawn.push_back(test);
  test = point(p.pos.x + i, p.pos.y - 3);
  if (g->is_empty(test)) spawn.push_back(test);
  test = point(p.pos.x + i, p.pos.y + 3);
  if (g->is_empty(test)) spawn.push_back(test);
 }
 if (spawn.empty()) {
  p.subjective_message("Air swirls around you for a moment.");
  it.make(item::types[itm_spiral_stone]);
  return;
 }

 messages.add("Air swirls all over...");
 p.moves -= mobile::mp_turn;
 it.make(item::types[itm_spiral_stone]);
 monster vortex(mtype::types[mon_vortex], spawn[rng(0, spawn.size() - 1)]);
 vortex.friendly = -1;
 g->z.push_back(vortex);
}

void iuse::dog_whistle(player& p, item& it)
{
    static auto pc_blows = []() { return std::string("You blow your dog whistle.");  };
    static auto npc_blows = [&]() {
        const auto subject = grammar::capitalize(p.subject());
        const auto possessive = p.pronoun(grammar::noun::role::possessive);
        return subject + " blows " + possessive + " dog whistle.";
    };

    p.if_visible_message(pc_blows, npc_blows);

 const auto g = game::active();

 for (decltype(auto) _dog : g->z) {
     if (mon_dog == _dog.type->id && _dog.is_friend(&p)) {
         const bool is_docile = _dog.has_effect(ME_DOCILE);

         static auto dog_reacts = [&]() {
             const auto d_name = grammar::capitalize(p.desc(grammar::noun::role::possessive)) + " " + _dog.desc(grammar::noun::role::subject);
             if (is_docile) return d_name + " looks ready to attack.";
             else return d_name + " goes docile.";
         };

         _dog.if_visible_message(dog_reacts);

         if (is_docile) _dog.rem_effect(ME_DOCILE); // \todo fix docility effect to be relative to master
         else _dog.add_effect(ME_DOCILE, -1);
     }
 }
}

// layman-usable blood drawing equipment.  No medical skill required.
void iuse::vacutainer(pc& p, item& it)
{
 if (!it.contents.empty()) {
  messages.add("That %s is full!", it.tname().c_str());
  return;
 }

 const auto g = game::active();
 item blood(item::types[itm_blood], messages.turn);
 bool drew_blood = false;
 for(const auto& corpse : g->m.i_at(p.GPSpos)) {
  if (corpse.type->id == itm_corpse && query_yn("Draw blood from %s?", corpse.tname().c_str())) {
   blood.corpse = corpse.corpse;
   drew_blood = true;
   break;
  }
 }

 if (!drew_blood && !query_yn("Draw your own blood?")) return;

 it.put_in(std::move(blood));
}
 

// 2021-10-01: archaic stub implementation.
// Cf. Gearhead I for what could be done.
/* MACGUFFIN FUNCTIONS
 * These functions should refer to it->associated_mission for the particulars
 */
void iuse::mcg_note(pc& p, item& it)
{
 std::ostringstream message;
 message << "Dear " << it.name << ":\n";
/*
 faction* fac = nullptr;
 direction dir = NORTH;
// Pick an associated faction
 switch (it->associated_mission) {
 case MISSION_FIND_FAMILY_FACTION:
  fac = &(g->factions[rng(0, g->factions.size() - 1)]);
  break;
 case MISSION_FIND_FAMILY_KIDNAPPER:
  fac = g->random_evil_faction();
  break;
 }
// Calculate where that faction is
 if (fac != nullptr) {
  int omx = g->cur_om.posx, omy = g->cur_om.posy;
  if (fac->omx != g->cur_om.posx || fac->omx != g->cur_om.posy)
   dir = direction_from(omx, omy, fac->omx, fac->omy);
  else
   dir = direction_from(g->levx, g->levy, fac->mapx, fac->mapy);
 }
// Produce the note and generate the next mission
 switch (it->associated_mission) {
 case MISSION_FIND_FAMILY_FACTION:
  if (fac->name == "The army")
   message << "\
I've been rescued by an army patrol.  They're taking me\n\
to their outpost to the " << direction_name(dir) << ".\n\
Please meet me there.  I need to know you're alright.";
  else
   message << "\
This group came through, looking for survivors.  They\n\
said they were members of this group calling itself\n" << fac->name << ".\n\
They've got a settlement to the " << direction_name(dir) << ", so\n\
I guess I'm heading there.  Meet me there as soon as\n\
you can, I need to know you're alright.";
  break;


  popup(message.str().c_str());
*/
}

void iuse::artifact(player *p, item *it, bool t)
{
    const auto art = it->is_artifact_tool();
    if (!art) {
        if (!it->is_artifact()) {
            debugmsg("iuse::artifact called on a non-artifact item! %s", it->tname().c_str());
            return;
        } else {
            debugmsg("iuse::artifact called on a non-tool artifact! %s", it->tname().c_str());
            return;
        }
    }

    const auto g = game::active();

 int num_used = rng(1, art->effects_activated.size());
 if (num_used < art->effects_activated.size())
  num_used += rng(1, art->effects_activated.size() - num_used);

 std::vector<art_effect_active> effects = art->effects_activated;
 for (int i = 0; i < num_used; i++) {
  int index = rng(0, effects.size() - 1);
  art_effect_active used = effects[index];
  effects.erase(effects.begin() + index);

  switch (used) {
  case AEA_STORM: {
   g->sound(p->pos, 10, "Ka-BOOM!");
   int num_bolts = rng(2, 4);
   for (int j = 0; j < num_bolts; j++) {
	point dir(direction_vector(direction(rng(NORTH,NORTHWEST))));
    int dist = rng(4, 12);
	point bolt(p->pos);
    for (int n = 0; n < dist; n++) {
	 bolt += dir;
     g->m.add_field(g, bolt, fd_electricity, rng(2, 3));
     if (one_in(4)) dir.x = (dir.x == 0) ? rng(0, 1) * 2 - 1 : 0;
     if (one_in(4)) dir.y = (dir.y == 0) ? rng(0, 1) * 2 - 1 : 0;
    }
   }
  } break;

  case AEA_FIREBALL:
   if (const auto fireball = g->look_around()) {
       g->explosion(*fireball, 8, 0, true);
   }
   break;

  case AEA_ADRENALINE:
   messages.add("You're filled with a roaring energy!");
   p->add_disease(DI_ADRENALINE, rng(MINUTES(20), MINUTES(25)));
   break;

  case AEA_MAP: {
   bool new_map = false;
   OM_loc<2> scan(g->cur_om.pos, point(0, 0));
   for (scan.second.x = int(g->lev.x / 2) - 20; scan.second.x <= int(g->lev.x / 2) + 20; scan.second.x++) {
    for (scan.second.y = int(g->lev.y / 2) - 20; scan.second.y <= int(g->lev.y / 2) + 20; scan.second.y++) {
     if (!overmap::seen_c(scan)) {
      new_map = true;
      overmap::seen(scan) = true;
     }
    }
   }
   if (new_map) {
    messages.add("You have a vision of the surrounding area...");
    p->moves -= 100;
   }
  } break;

  case AEA_BLOOD: {
   bool blood = false;
   for (int x = p->pos.x - 4; x <= p->pos.x + 4; x++) {
    for (int y = p->pos.y - 4; y <= p->pos.y + 4; y++) {
     if (!one_in(4) && g->m.add_field(g, x, y, fd_blood, 3) &&
         (blood || g->u.see(x, y)))
      blood = true;
    }
   }
   if (blood) messages.add("Blood soaks out of the ground and walls.");
  } break;

  case AEA_FATIGUE: {
   messages.add("The fabric of space seems to decay.");
   int x = rng(p->pos.x - 3, p->pos.x + 3), y = rng(p->pos.y - 3, p->pos.y + 3);
   auto& fd = g->m.field_at(x, y);
   if (fd.type == fd_fatigue) { 
	   if (fd.density < 3) fd.density++;
   } else g->m.add_field(g, x, y, fd_fatigue, rng(1, 2));
  } break;

  case AEA_ACIDBALL:
   if (const auto acidball = g->look_around()) {
       for (int x = acidball->x - 1; x <= acidball->x + 1; x++) {
           for (int y = acidball->y - 1; y <= acidball->y + 1; y++) {
               auto& fd = g->m.field_at(x, y);
               if (fd.type == fd_acid && fd.density < 3)
                   fd.density++;
               else
                   g->m.add_field(g, x, y, fd_acid, rng(2, 3));
           }
       }
   }
   break;

  case AEA_PULSE:
   g->sound(p->pos, 30, "The earth shakes!");
   for (int x = p->pos.x - 2; x <= p->pos.x + 2; x++) {
    for (int y = p->pos.y - 2; y <= p->pos.y + 2; y++) {
     g->m.bash(x, y, 40);
     g->m.bash(x, y, 40);  // Multibash effect, so that doors &c will fall
     g->m.bash(x, y, 40);
     if (g->m.is_destructable(x, y) && rng(1, 10) >= 3)
      g->m.ter(x, y) = t_rubble;
    }
   }
   break;

  case AEA_HEAL:
   messages.add("You feel healed.");
   p->healall(2);
   break;

  case AEA_CONFUSED:
   for (int x = p->pos.x - 8; x <= p->pos.x + 8; x++) {
    for (int y = p->pos.y - 8; y <= p->pos.y + 8; y++) {
	 if (monster* const m_at = g->mon(x,y)) m_at->add_effect(ME_STUNNED, rng(5, 15));
    }
   }
   break;

  case AEA_ENTRANCE:
   for (int x = p->pos.x - 8; x <= p->pos.x + 8; x++) {
    for (int y = p->pos.y - 8; y <= p->pos.y + 8; y++) {
	 if (monster* const m_at = g->mon(x,y)) {
	   if (m_at->is_enemy(p) && rng(0, 600) > m_at->hp) m_at->make_friendly();
	 }
    }
   }
   break;

  case AEA_BUGS: {
   int roll = rng(1, 10);
   mon_id bug = mon_null;
   int num = 0;
   std::vector<point> empty;
   for (decltype(auto) dir : Direction::vector) {
       point pt(p->pos + dir);
       if (g->is_empty(pt)) empty.push_back(pt);
   }
   if (empty.empty() || roll <= 4)
    messages.add("Flies buzz around you.");
   else if (roll <= 7) {
    messages.add("Giant flies appear!");
    bug = mon_fly;
    num = rng(2, 4);
   } else if (roll <= 9) {
    messages.add("Giant bees appear!");
    bug = mon_bee;
    num = rng(1, 3);
   } else {
    messages.add("Giant wasps appear!");
    bug = mon_wasp;
    num = rng(1, 2);
   }
   if (bug != mon_null) {
    monster spawned(mtype::types[bug]);
    spawned.friendly = -1;
    for (int i = 0; i < num && !empty.empty(); i++) {
     int index = rng(0, empty.size() - 1);
     spawned.spawn(empty[index]);
	 empty.erase(empty.begin() + index);
	 g->z.push_back(spawned);
    }
   }
  } break;

  case AEA_TELEPORT:
   g->teleport(p);
   break;

  case AEA_LIGHT:
   messages.add("The %s glows brightly!", it->tname().c_str());
   g->add_event(EVENT_ARTIFACT_LIGHT, int(messages.turn) + 30);
   break;

  case AEA_GROWTH: {
   monster tmptriffid(mtype::types[0], p->pos);
   mattack::growplants(g, &tmptriffid);
  } break;

  case AEA_HURTALL:
   for (int i = 0; i < g->z.size(); i++)
    g->z[i].hurt(rng(0, 5));
   break;

  case AEA_RADIATION:
   messages.add("Horrible gasses are emitted!");
   for (int x = p->pos.x - 1; x <= p->pos.x + 1; x++) {
    for (int y = p->pos.y - 1; y <= p->pos.y + 1; y++)
     g->m.add_field(g, x, y, fd_nuke_gas, rng(2, 3));
   }
   break;

  case AEA_PAIN:
   messages.add("You're wracked with pain!");
   p->pain += rng(5, 15);
   break;

  case AEA_MUTATE:
   if (!one_in(3)) p->mutate();
   break;

  case AEA_PARALYZE:
   messages.add("You're paralyzed!");
   p->moves -= rng(50, 200);
   break;

  case AEA_FIRESTORM:
   messages.add("Fire rains down around you!");
   for (int x = p->pos.x - 3; x <= p->pos.x + 3; x++) {
    for (int y = p->pos.y - 3; y <= p->pos.y + 3; y++) {
     if (!one_in(3)) g->m.add_field(g, x, y, fd_fire, 1 + rng(0, 1) * rng(0, 1), 30);
    }
   }
   break;

  case AEA_ATTENTION:
   messages.add("You feel like your action has attracted attention.");
   p->add_disease(DI_ATTENTION, HOURS(1) * rng(1, 3));
   break;

  case AEA_TELEGLOW:
   messages.add("You feel unhinged.");
   p->add_disease(DI_TELEGLOW, MINUTES(10) * rng(3, 12));
   break;

  case AEA_NOISE:
   messages.add("Your %s emits a deafening boom!", it->tname().c_str());
   g->sound(p->pos, 100, "");
   break;

  case AEA_SCREAM:
   messages.add("Your %s screams disturbingly.", it->tname().c_str());
   g->sound(p->pos, 40, "");
   p->add_morale(MORALE_SCREAM, -10);
   break;

  case AEA_DIM:
   messages.add("The sky starts to dim.");
   g->add_event(EVENT_DIM, int(messages.turn) + 50);
   break;

  case AEA_FLASH:
   messages.add("The %s flashes brightly!", it->tname().c_str());
   g->flashbang(p->pos);
   break;

  case AEA_VOMIT:
   messages.add("A wave of nausea passes through you!");
   p->vomit();
   break;

  // cf. trapfunc::shadow
  case AEA_SHADOWS: {
   int num_shadows = rng(4, 8);
   monster spawned(mtype::types[mon_shadow]);
   int num_spawned = 0;

   static std::function<point()> candidate = [&]() {
       if (one_in(2)) {
           return point(rng(p->pos.x - 5, p->pos.x + 5), one_in(2) ? p->pos.y - 5 : p->pos.y + 5);
       }
       else {
           return point(one_in(2) ? p->pos.x - 5 : p->pos.x + 5, rng(p->pos.y - 5, p->pos.y + 5));
       }
   };

   static std::function<bool(const point&)> ok = [&](const point& pt) {
       return g->is_empty(pt) && g->m.sees(pt, p->pos, 10);
   };

   for (int i = 0; i < num_shadows; i++) {
       if (auto pt = LasVegasChoice(5, candidate, ok)) {
           num_spawned++;
           spawned.sp_timeout = rng(8, 20);
           spawned.spawn(*pt);
           g->z.push_back(spawned);
       }
   }

   if (num_spawned > 1)
    messages.add("Shadows form around you.");
   else if (num_spawned == 1)
    messages.add("A shadow forms nearby.");
  } break;

  }
 }
}
