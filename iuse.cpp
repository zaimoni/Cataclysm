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
void iuse::sewage(game *g, player *p, item *it, bool t)
{
 p->vomit();
 if (one_in(4)) p->mutate();
}

void iuse::royal_jelly(game *g, player *p, item *it, bool t)
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
 if (!p->is_npc()) messages.add(message.c_str());
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

static bool _get_heal_target(player* p, item* it, hp_part& healed)
{
    int ch;
    do {
        ch = getch();
        if (ch == '1')
            healed = hp_head;
        else if (ch == '2')
            healed = hp_torso;
        else if (ch == '3') {
            if (p->hp_cur[hp_arm_l] == 0) {
                messages.add("That arm is broken.  It needs surgical attention.");
                it->charges++;
                return false;
            }
            else
                healed = hp_arm_l;
        }
        else if (ch == '4') {
            if (p->hp_cur[hp_arm_r] == 0) {
                messages.add("That arm is broken.  It needs surgical attention.");
                it->charges++;
                return false;
            }
            else
                healed = hp_arm_r;
        }
        else if (ch == '5') {
            if (p->hp_cur[hp_leg_l] == 0) {
                messages.add("That leg is broken.  It needs surgical attention.");
                it->charges++;
                return false;
            }
            else
                healed = hp_leg_l;
        }
        else if (ch == '6') {
            if (p->hp_cur[hp_leg_r] == 0) {
                messages.add("That leg is broken.  It needs surgical attention.");
                it->charges++;
                return false;
            }
            else
                healed = hp_leg_r;
        }
        else if (ch == '7') {
            messages.add("Never mind.");
            it->charges++;
            return false;
        }
    } while (ch < '1' || ch > '7');
    return true;
}

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

void iuse::bandage(game *g, player *p, item *it, bool t) 
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
  const bool ok = _get_heal_target(p, it, healed);
  werase(w);
  wrefresh(w);
  delwin(w);
  refresh();
  if (!ok) return;
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

void iuse::firstaid(game *g, player *p, item *it, bool t) 
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

  const bool ok = _get_heal_target(p, it, healed);
  werase(w);
  wrefresh(w);
  delwin(w);
  refresh();
  if (!ok) return;
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

void iuse::vitamins(game *g, player *p, item *it, bool t)
{
 if (p->has_disease(DI_TOOK_VITAMINS)) {
  if (p == &(g->u)) messages.add("You have the feeling that these vitamins won't do you any good.");
  return;
 }

 p->add_disease(DI_TOOK_VITAMINS, DAYS(1));
 if (p->health <= -2) p->health -= (p->health / 2);
 p->health += 3;
}

// Aspirin
void iuse::pkill_1(game *g, player *p, item *it, bool t)
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
void iuse::pkill_2(game *g, player *p, item *it, bool t)
{
 if (!p->is_npc()) messages.add("You take some %s.", it->tname().c_str());

 p->add_disease(DI_PKILL2, MINUTES(18));
}

void iuse::pkill_3(game *g, player *p, item *it, bool t)
{
 if (!p->is_npc()) messages.add("You take some %s.", it->tname().c_str());

 p->add_disease(DI_PKILL3, MINUTES(2));
 p->add_disease(DI_PKILL2, MINUTES(20));
}

void iuse::pkill_4(game *g, player *p, item *it, bool t)
{
 if (!p->is_npc()) messages.add("You shoot up.");

 p->add_disease(DI_PKILL3, MINUTES(8));
 p->add_disease(DI_PKILL2, MINUTES(20));
}

void iuse::pkill_l(game *g, player *p, item *it, bool t)
{
 if (!p->is_npc()) messages.add("You take some %s.", it->tname().c_str());

 p->add_disease(DI_PKILL_L, rng(12, 18) * MINUTES(30));
}

void iuse::xanax(game *g, player *p, item *it, bool t)
{
 if (!p->is_npc()) messages.add("You take some %s.", it->tname().c_str());

 p->add_disease(DI_TOOK_XANAX, (!p->has_disease(DI_TOOK_XANAX) ? MINUTES(90): MINUTES(20)));
}

void iuse::caff(game *g, player *p, item *it, bool t)
{
 p->fatigue -= dynamic_cast<const it_comest*>(it->type)->stim * 3;
}

void iuse::alcohol(game *g, player *p, item *it, bool t)
{
 int duration = MINUTES(68 - p->str_max); // Weaker characters are cheap drunks
 if (p->has_trait(PF_LIGHTWEIGHT)) duration += MINUTES(30);
 p->pkill += 8;
 p->add_disease(DI_DRUNK, duration);
}

void iuse::cig(game *g, player *p, item *it, bool t)
{
 if (!p->is_npc()) messages.add("You light a cigarette and smoke it.");
 p->add_disease(DI_CIG, MINUTES(20));
 for (int i = 0; i < p->illness.size(); i++) {
  if (p->illness[i].type == DI_CIG && p->illness[i].duration > HOURS(1) && !p->is_npc())
   messages.add("Ugh, too much smoke... you feel gross.");
 }
}

void iuse::weed(game *g, player *p, item *it, bool t)
{
 if (!p->is_npc()) messages.add("Good stuff, man!");

 int duration = MINUTES(6);
 if (p->has_trait(PF_LIGHTWEIGHT)) duration = MINUTES(9);
 p->hunger += 8;
 if (p->pkill < 15) p->pkill += 5;
 p->add_disease(DI_THC, duration);
}

void iuse::coke(game *g, player *p, item *it, bool t)
{
 if (!p->is_npc()) messages.add("You snort a bump.");

 int duration = TURNS(21 - p->str_cur);
 if (p->has_trait(PF_LIGHTWEIGHT)) duration += MINUTES(2);
 p->hunger -= 8;
 p->add_disease(DI_HIGH, duration);
}

void iuse::meth(game *g, player *p, item *it, bool t)
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

void iuse::poison(game *g, player *p, item *it, bool t)
{
 p->add_disease(DI_POISON, HOURS(1));
 p->add_disease(DI_FOODPOISON, HOURS(3));
}

void iuse::hallu(game *g, player *p, item *it, bool t)
{
 p->add_disease(DI_HALLU, HOURS(4));
}

void iuse::thorazine(game *g, player *p, item *it, bool t)
{
 p->fatigue += 15;
 p->rem_disease(DI_HALLU);
 p->rem_disease(DI_VISUALS);
 p->rem_disease(DI_HIGH);
 p->rem_disease(DI_THC);    // B-movie; situation in real life is more complicated
 if (!p->has_disease(DI_DERMATIK)) p->rem_disease(DI_FORMICATION);
 if (!p->is_npc()) messages.add("You feel somewhat sedated.");
}

void iuse::prozac(game *g, player *p, item *it, bool t) // \todo not powerful enough
{
 if (!p->has_disease(DI_TOOK_PROZAC) && p->morale_level() < 0)
  p->add_disease(DI_TOOK_PROZAC, HOURS(6));
 else
  p->stim += 3;
}

void iuse::sleep(game *g, player *p, item *it, bool t)
{
 p->fatigue += 40;
 if (!p->is_npc()) messages.add("You feel very sleepy...");
}

void iuse::iodine(game *g, player *p, item *it, bool t)
{
 p->add_disease(DI_IODINE, HOURS(2));
 if (!p->is_npc()) messages.add("You take an iodine tablet.");
}

void iuse::flumed(game *g, player *p, item *it, bool t)
{
 p->add_disease(DI_TOOK_FLUMED, HOURS(10));
 if (!p->is_npc()) messages.add("You take some %s", it->tname().c_str());
}

void iuse::flusleep(game *g, player *p, item *it, bool t)
{
 p->add_disease(DI_TOOK_FLUMED, HOURS(12));
 p->fatigue += 30;
 if (!p->is_npc()) messages.add("You feel very sleepy...");
}

void iuse::inhaler(game *g, player *p, item *it, bool t)
{
 p->rem_disease(DI_ASTHMA);
 if (!p->is_npc()) messages.add("You take a puff from your inhaler.");
}

void iuse::blech(game *g, player *p, item *it, bool t)
{
// TODO: Add more effects?
 if (!p->is_npc()) messages.add("Blech, that burns your throat!");
 p->vomit();
}

void iuse::mutagen(game *g, player *p, item *it, bool t)
{
 if (!one_in(3)) p->mutate();
}

void iuse::mutagen_3(game *g, player *p, item *it, bool t)
{
 p->mutate();
 if (!one_in(3)) p->mutate();
 if (one_in(2)) p->mutate();
}

void iuse::purifier(game *g, player *p, item *it, bool t)
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

void iuse::marloss(game *g, player *p, item *it, bool t)
{
 if (p->is_npc()) return;
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
  purifier(g, p, it, t);
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

void iuse::dogfood(game *g, player *p, item *it, bool t)
{
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

void iuse::lighter(game *g, player *p, item *it, bool t)
{
 g->draw();
 mvprintw(0, 0, "Light where?");
 point dir(get_direction(input()));
 if (dir.x == -2) {
  messages.add("Invalid direction.");
  it->charges++;
  return;
 }
 p->moves -= 15;
 dir += p->pos;
 if (g->m.flammable_items_at(dir.x, dir.y)) {
  g->m.add_field(g, dir, fd_fire, 1, 30);
 } else {
  messages.add("There's nothing to light there.");
  it->charges++;
 }
}

void iuse::sew(game *g, player *p, item *it, bool t)
{
 char ch = g->inv("Repair what?");
 item* fix = &(p->i_at(ch));
 if (fix == nullptr || fix->is_null()) {
  messages.add("You do not have that item!");
  it->charges++;
  return;
 }
 if (!fix->is_armor()) {
  messages.add("That isn't clothing!");
  it->charges++;
  return;
 }
 if (!fix->made_of(COTTON) && !fix->made_of(WOOL)) {
  messages.add("Your %s is not made of cotton or wool.", fix->tname().c_str());
  it->charges++;
  return;
 }
 if (fix->damage < 0) {
  messages.add("Your %s is already enhanced.", fix->tname().c_str());
  it->charges++;
  return;
 } else if (fix->damage == 0) {
  p->moves -= 500;
  p->practice(sk_tailor, 10);
  int rn = dice(4, 2 + p->sklevel[sk_tailor]);
  if (p->dex_cur < 8 && one_in(p->dex_cur)) rn -= rng(2, 6);
  if (p->dex_cur >= 16 || (p->dex_cur > 8 && one_in(16 - p->dex_cur))) rn += rng(2, 6);
  if (p->dex_cur > 16) rn += rng(0, p->dex_cur - 16);
  if (rn <= 4) {
   messages.add("You damage your %s!", fix->tname().c_str());
   fix->damage++;
  } else if (rn >= 12) {
   messages.add("You make your %s extra-sturdy.", fix->tname().c_str());
   fix->damage--;
  } else
   messages.add("You practice your sewing.");
 } else {
  p->moves -= 500;
  p->practice(sk_tailor, 8);
  int rn = dice(4, 2 + p->sklevel[sk_tailor]);
  rn -= rng(fix->damage, fix->damage * 2);
  if (p->dex_cur < 8 && one_in(p->dex_cur)) rn -= rng(2, 6);
  if (p->dex_cur >= 8 && (p->dex_cur >= 16 || one_in(16 - p->dex_cur))) rn += rng(2, 6);
  if (p->dex_cur > 16) rn += rng(0, p->dex_cur - 16);

  if (rn <= 4) {
   messages.add("You damage your %s further!", fix->tname().c_str());
   fix->damage++;
   if (fix->damage >= 5) {
    messages.add("You destroy it!");
    p->i_rem(ch);
   }
  } else if (rn <= 6) {
   messages.add("You don't repair your %s, but you waste lots of thread.", fix->tname().c_str());
   int waste = rng(1, 8);
   if (waste > it->charges)
    it->charges = 0;
   else
    it->charges -= waste;
  } else if (rn <= 8) {
   messages.add("You repair your %s, but waste lots of thread.", fix->tname().c_str());
   fix->damage--;
   int waste = rng(1, 8);
   if (waste > it->charges)
    it->charges = 0;
   else
    it->charges -= waste;
  } else if (rn <= 16) {
   messages.add("You repair your %s!", fix->tname().c_str());
   fix->damage--;
  } else {
   messages.add("You repair your %s completely!", fix->tname().c_str());
   fix->damage = 0;
  }
 }
}

void iuse::scissors(game *g, player *p, item *it, bool t)
{
 char ch = g->inv("Chop up what?");
 item* cut = &(p->i_at(ch));
 if (cut->type->id == 0) {
  messages.add("You do not have that item!");
  return;
 }
 if (cut->type->id == itm_rag) {
  messages.add("There's no point in cutting a rag.");
  return;
 }
 if (cut->type->id == itm_string_36 || cut->type->id == itm_rope_30) {
  p->moves -= 150;
  bool is_string = (cut->type->id == itm_string_36);
  int pieces = (is_string ? 6 : 5);
  messages.add("You cut the %s into %d smaller pieces.",
             (is_string ? "string" : "rope"), pieces);
  itype_id piece_id = (is_string ? itm_string_6 : itm_rope_6);
  item string(item::types[piece_id], int(messages.turn), g->nextinv);
  p->i_rem(ch);
  bool drop = false;
  for (int i = 0; i < pieces; i++) {
   if (!drop) {
       if (p->volume_carried() >= p->volume_capacity() || !g->assign_invlet(string, *p)) drop = true;
   }
   if (drop) g->m.add_item(p->pos, string);
   else p->i_add(string);
  }
  return;
 }
 if (!cut->made_of(COTTON)) {
  messages.add("You can only slice items made of cotton.");
  return;
 }
 p->moves -= 25 * cut->volume();
 int count = cut->volume();
 if (p->sklevel[sk_tailor] == 0) count = rng(0, count);
 else if (p->sklevel[sk_tailor] == 1 && count >= 2) count -= rng(0, 2);

 if (dice(3, 3) > p->dex_cur) count -= rng(1, 3);

 if (count <= 0) {
  messages.add("You clumsily cut the %s into useless ribbons.", cut->tname().c_str());
  p->i_rem(ch);
  return;
 }
 messages.add("You slice the %s into %d rag%s.", cut->tname().c_str(), count,
            (count == 1 ? "" : "s"));
 item rag(item::types[itm_rag], int(messages.turn), g->nextinv);
 p->i_rem(ch);
 bool drop = false;
 for (int i = 0; i < count; i++) {
  if (!drop) {
      if (p->volume_carried() >= p->volume_capacity() || !g->assign_invlet(rag, *p)) drop = true;
  }
  if (drop) g->m.add_item(p->pos, rag);
  else p->i_add(rag);
 }
}

void iuse::extinguisher(game *g, player *p, item *it, bool t)
{
 g->draw();
 mvprintz(0, 0, c_red, "Pick a direction to spray:");
 point dir(get_direction(input()));
 if (dir.x == -2) {
  messages.add("Invalid direction!");
  it->charges++;
  return;
 }
 p->moves -= (mobile::mp_turn / 5) * 7;
 point pt(dir + p->pos);
 {
 auto& fd = g->m.field_at(pt);
 if (fd.type == fd_fire) {
  if (0 >= (fd.density -= rng(2, 3))) g->m.remove_field(pt);
 }
 }
 if (monster* const m_at = g->mon(pt)) {
  m_at->moves -= (mobile::mp_turn / 2) * 3;
  const bool is_liquid = m_at->made_of(LIQUID); // assumed that liquid is water or similar -- dry ice extinguisher?
  if (g->u.see(*m_at)) {
      const auto z_name = grammar::capitalize(m_at->desc(grammar::noun::role::subject, grammar::article::definite));
      messages.add("%s is sprayed!", z_name.c_str());
      if (is_liquid) messages.add("%s is frozen!", z_name.c_str());
  }
  if (is_liquid) {
   if (m_at->hurt(rng(20, 60))) g->kill_mon(*m_at, p);
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

void iuse::hammer(game *g, player *p, item *it, bool t)
{
 g->draw();
 mvprintz(0, 0, c_red, "Pick a direction in which to pry:");
 point dir(get_direction(input()));
 if (dir.x == -2) {
  messages.add("Invalid direction!");
  return;
 }
 dir += p->pos;
 auto& type = g->m.ter(dir);
 auto deconstruct = linear_search(type, std::begin(deconstruct_boarded), std::end(deconstruct_boarded));
 if (!deconstruct) {
     messages.add("Hammers can only remove boards from windows and doors.");
     messages.add("To board up a window or door, press *");
     return;
 }
 p->moves -= 5 * mobile::mp_turn;
 item it_nails(item::types[itm_nail], 0, g->nextinv);
 it_nails.charges = std::get<1>(*deconstruct);
 g->m.add_item(p->pos, std::move(it_nails));
 item board(item::types[itm_2x4], 0, g->nextinv);
 for (int i = 0; i < std::get<2>(*deconstruct); i++) g->m.add_item(p->pos, board);
 type = std::get<0>(*deconstruct);
}
 
void iuse::light_off(game *g, player *p, item *it, bool t)
{
 if (it->charges == 0) messages.add("The flashlight's batteries are dead.");
 else {
  messages.add("You turn the flashlight on.");
  it->make(item::types[itm_flashlight_on]);
  it->active = true;
 }
}
 
void iuse::light_on(game *g, player *p, item *it, bool t)
{
 if (t) {	// Normal use
// Do nothing... game::light_level() handles this
 } else {	// Turning it off
  messages.add("The flashlight flicks off.");
  it->make(item::types[itm_flashlight]);
  it->active = false;
 }
}

void iuse::water_purifier(game *g, player *p, item *it, bool t)
{
 const auto purify = p->have_item(g->inv("Purify what?"));
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

void iuse::two_way_radio(game *g, player *p, item *it, bool t)
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
  p->moves -= 300;
  faction* fac = g->list_factions("Call for help...");
  if (fac == nullptr) {
   it->charges++;
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
  p->moves -= 150;
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
  it->charges++;	// Canceled the call, get our charge back
 werase(w);
 wrefresh(w);
 delwin(w);
 refresh();
}
 
void iuse::radio_off(game *g, player *p, item *it, bool t)
{
 if (it->charges == 0) messages.add("It's dead.");
 else {
  messages.add("You turn the radio on.");
  it->make(item::types[itm_radio_on]);
  it->active = true;
 }
}

void iuse::radio_on(game *g, player *p, item *it, bool t)
{
 static constexpr const int RADIO_PER_TURN = 25;

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

void iuse::crowbar(game *g, player *p, item *it, bool t)
{
 g->draw();
 mvprintw(0, 0, "Pry where?");
 point dir(get_direction(input()));
 if (dir.x == -2) {
  messages.add("Invalid direction.");
  return;
 }
 dir += p->pos;
 auto& type = g->m.ter(dir);
 if (type == t_door_c || type == t_door_locked || type == t_door_locked_alarm) {
  if (dice(4, 6) < dice(4, p->str_cur)) {
   messages.add("You pry the door open.");
   p->moves -= (mobile::mp_turn / 2) * 3 - (p->str_cur * (mobile::mp_turn / 20));
   type = t_door_o;
  } else {
   messages.add("You pry, but cannot open the door.");
   p->moves -= mobile::mp_turn;
  }
 } else if (type == t_manhole_cover) {
  if (dice(8, 8) < dice(8, p->str_cur)) {
   messages.add("You lift the manhole cover.");
   p->moves -= 5 * mobile::mp_turn - (p->str_cur * (mobile::mp_turn / 20));
   type = t_manhole;
   g->m.add_item(p->pos, item::types[itm_manhole_cover], 0);
  } else {
   messages.add("You pry, but cannot lift the manhole cover.");
   p->moves -= mobile::mp_turn;
  }
 } else if (type == t_crate_c) {
  if (p->str_cur >= rng(3, 30)) {
   messages.add("You pop the crate open.");
   p->moves -= (mobile::mp_turn / 2) * 3 - (p->str_cur * (mobile::mp_turn / 20));
   type = t_crate_o;
  } else {
   messages.add("You pry, but cannot open the crate.");
   p->moves -= mobile::mp_turn;
  } 
 } else {
  auto deconstruct = linear_search(type, std::begin(deconstruct_boarded), std::end(deconstruct_boarded));
  if (!deconstruct) {
      messages.add("There's nothing to pry there.");
      return;
  }
  p->moves -= 5 * mobile::mp_turn;
  item it_nails(item::types[itm_nail], 0, g->nextinv);
  it_nails.charges = std::get<1>(*deconstruct);
  g->m.add_item(p->pos, std::move(it_nails));
  item board(item::types[itm_2x4], 0, g->nextinv);
  for (int i = 0; i < std::get<2>(*deconstruct); i++)
   g->m.add_item(p->pos, board);
  type = std::get<0>(*deconstruct);
 }
}

void iuse::makemound(game *g, player *p, item *it, bool t)
{
 if (g->m.has_flag(diggable, p->pos)) {
  messages.add("You churn up the earth here.");
  p->moves -= 3 * mobile::mp_turn;
  p->GPSpos.ter() = t_dirtmound;
 } else
  messages.add("You can't churn up this ground.");
}

void iuse::dig(game *g, player *p, item *it, bool t)
{
 messages.add("You can dig a pit via the construction menu--hit *");
}

void iuse::chainsaw_off(game *g, player *p, item *it, bool t)
{
 p->moves -= (mobile::mp_turn / 5) * 4;
 if (rng(0, 10) - it->damage > 5 && it->charges > 0) {
  g->sound(p->pos, 20, "With a roar, the chainsaw leaps to life!");
  it->make(item::types[itm_chainsaw_on]);
  it->active = true;
 } else
  messages.add("You yank the cord, but nothing happens.");
}

void iuse::chainsaw_on(game *g, player *p, item *it, bool t)
{
 if (t) {	// Effects while simply on
  if (one_in(15)) g->sound(p->pos, 12, "Your chainsaw rumbles.");
 } else {	// Toggling
  messages.add("Your chainsaw dies.");
  it->make(item::types[itm_chainsaw_off]);
  it->active = false;
 }
}

void iuse::jackhammer(game *g, player *p, item *it, bool t)
{
 g->draw();
 mvprintw(0, 0, "Drill in which direction?");
 point dir(get_direction(input()));
 if (dir.x == -2) {
  messages.add("Invalid direction.");
  return;
 }
 dir += p->pos;
 if (g->m.is_destructable(dir)) {
  g->m.destroy(g, dir.x, dir.y, false);
  p->moves -= 5 * mobile::mp_turn;
  g->sound(dir, 45, "TATATATATATATAT!");
 } else {
  messages.add("You can't drill there.");
  it->charges += (dynamic_cast<const it_tool*>(it->type))->charges_per_use;
 }
}

void iuse::set_trap(game *g, player *p, item *it, bool t)
{
 g->draw();
 mvprintw(0, 0, "Place where?");
 point dir(get_direction(input()));
 if (dir.x == -2) {
  messages.add("Invalid direction.");
  return;
 }
 point trap_pos(dir + p->pos);
 if (g->m.move_cost(trap_pos) != 2) {
  messages.add("You can't place a %s there.", it->tname().c_str());
  return;
 }

 trap_id type = tr_null;
 bool buried = false;
 std::ostringstream message;
 int practice;

 switch (it->type->id) {
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
  buried = (p->has_amount(itm_shovel, 1) &&
            g->m.has_flag(diggable, trap_pos) &&
            query_yn("Bury the beartrap?"));
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
  messages.add("Tried to set a trap.  But got confused! %s", it->tname().c_str());
  return;
 }

 if (buried) {
  if (!p->has_amount(itm_shovel, 1)) {
   messages.add("You need a shovel.");
   return;
  } else if (!g->m.has_flag(diggable, trap_pos)) {
   messages.add("You can't dig in that %s", name_of(g->m.ter(trap_pos)).c_str());
   return;
  }
 }

 messages.add(message.str().c_str());
 p->practice(sk_traps, practice);
 g->m.add_trap(trap_pos, type);
 p->moves -= mobile::mp_turn + practice * (mobile::mp_turn/4);
 if (type == tr_engine) {
  for (decltype(auto) delta : Direction::vector) g->m.add_trap(trap_pos + delta, tr_blade);
 }
 it->invlet = 0; // Remove the trap from the player's inv
}

void iuse::geiger(game *g, player *p, item *it, bool t)
{
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
 const it_tool* const type = dynamic_cast<const it_tool*>(it->type);
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

void iuse::teleport(game *g, player *p, item *it, bool t)
{
 p->moves -= mobile::mp_turn;
 g->teleport(p);
}

void iuse::can_goo(game *g, player *p, item *it, bool t)
{
 it->make(item::types[itm_canister_empty]);
 int tries = 0, goox, gooy;
 do {
  goox = p->pos.x + rng(-2, 2);
  gooy = p->pos.y + rng(-2, 2);
  tries++;
 } while (g->m.move_cost(goox, gooy) == 0 && tries < 10);
 if (tries == 10) return;
 if (monster* const m_at = g->mon(goox, gooy)) {
  if (g->u_see(goox, gooy))
   messages.add("Black goo emerges from the canister and envelopes a %s!", m_at->name().c_str());
  m_at->poly(mtype::types[mon_blob]);
  m_at->speed -= rng(5, 25);
  m_at->hp = m_at->speed;
 } else {
  if (g->u_see(goox, gooy))
   messages.add("Living black goo emerges from the canister!");
  monster goo(mtype::types[mon_blob], goox, gooy);
  goo.friendly = -1;
  g->z.push_back(goo);
 }
 tries = 0;
 while (!one_in(4) && tries < 10) {
  tries = 0;
  do {
   goox = p->pos.x + rng(-2, 2);
   gooy = p->pos.y + rng(-2, 2);
   tries++;
  } while (g->m.move_cost(goox, gooy) == 0 &&
           g->m.tr_at(goox, gooy) == tr_null && tries < 10);
  if (tries < 10) {
   if (g->u_see(goox, gooy))
    messages.add("A nearby splatter of goo forms into a goo pit.");
   g->m.tr_at(goox, gooy) = tr_goo;
  }
 }
}

void iuse::pipebomb(game *g, player *p, item *it, bool t)
{
 if (!p->has_charges(itm_lighter, 1)) {
  messages.add("You need a lighter!");
  return;
 }
 p->use_charges(itm_lighter, 1);
 messages.add("You light the fuse on the pipe bomb.");
 it->make(item::types[itm_pipebomb_act]);
 it->charges = 3;
 it->active = true;
}

void iuse::pipebomb_act(game *g, player *p, item *it, bool t)
{
 const auto pos = g->find_item(it);
 assert(pos);
 if (t) // Simple timer effects
  g->sound(*pos, 0, "Ssssss");	// Vol 0 = only heard if you hold it
 else {	// The timer has run down
  if (one_in(10) && g->u_see(*pos))
   messages.add("The pipe bomb fizzles out.");
  else
   g->explosion(*pos, rng(6, 14), rng(0, 4), false);
 }
}
 
void iuse::grenade(game *g, player *p, item *it, bool t)
{
 messages.add("You pull the pin on the grenade.");
 it->make(item::types[itm_grenade_act]);
 it->charges = 5;
 it->active = true;
}

void iuse::grenade_act(game *g, player *p, item *it, bool t)
{
 const auto pos = g->find_item(it);
 assert(pos);
 if (t) // Simple timer effects
  g->sound(*pos, 0, "Tick.");	// Vol 0 = only heard if you hold it
 else	// When that timer runs down...
  g->explosion(*pos, 12, 28, false);
}

void iuse::flashbang(game *g, player *p, item *it, bool t)
{
 messages.add("You pull the pin on the flashbang.");
 it->make(item::types[itm_flashbang_act]);
 it->charges = 5;
 it->active = true;
}

void iuse::flashbang_act(game *g, player *p, item *it, bool t)
{
 const auto pos = g->find_item(it);
 assert(pos);
 if (t) // Simple timer effects
  g->sound(*pos, 0, "Tick.");	// Vol 0 = only heard if you hold it
 else	// When that timer runs down...
  g->flashbang(*pos);
}

void iuse::c4(game *g, player *p, item *it, bool t)
{
 int time = query_int("Set the timer to (0 to cancel)?");
 if (time == 0) {
  messages.add("Never mind.");
  return;
 }
 messages.add("You set the timer to %d.", time);
 it->make(item::types[itm_c4armed]);
 it->charges = time;
 it->active = true;
}

void iuse::c4armed(game *g, player *p, item *it, bool t)
{
 const auto pos = g->find_item(it);
 assert(pos);
 if (t) // Simple timer effects
  g->sound(*pos, 0, "Tick.");	// Vol 0 = only heard if you hold it
 else	// When that timer runs down...
  g->explosion(*pos, 40, 3, false);
}

void iuse::EMPbomb(game *g, player *p, item *it, bool t)
{
 messages.add("You pull the pin on the EMP grenade.");
 it->make(item::types[itm_EMPbomb_act]);
 it->charges = 3;
 it->active = true;
}

void iuse::EMPbomb_act(game *g, player *p, item *it, bool t)
{
 const auto pos = g->find_item(it);
 assert(pos);
 if (t)	// Simple timer effects
  g->sound(*pos, 0, "Tick.");	// Vol 0 = only heard if you hold it
 else {	// When that timer runs down...
  static auto explode = [&](point pt) { g->emp_blast(pt.x, pt.y); };
  forall_do_inclusive(*pos + within_rldist<4>, explode);
 }
}

void iuse::gasbomb(game *g, player *p, item *it, bool t)
{
 messages.add("You pull the pin on the teargas canister.");
 it->make(item::types[itm_gasbomb_act]);
 it->charges = 20;
 it->active = true;
}

void iuse::gasbomb_act(game *g, player *p, item *it, bool t)
{
 const auto pos = g->find_item(it);
 assert(pos);
 if (t) {
  if (it->charges > 15) g->sound(*pos, 0, "Tick.");	// Vol 0 = only heard if you hold it
  else {
   for (int i = -2; i <= 2; i++) {
    for (int j = -2; j <= 2; j++) {
	 point dest(pos->x + i, pos->y + j);
     if (g->m.sees(*pos, dest, 3) && g->m.move_cost(dest) > 0)
      g->m.add_field(g, dest, fd_tear_gas, 3);
    }
   }
  }
 } else it->make(item::types[itm_canister_empty]);
}

void iuse::smokebomb(game *g, player *p, item *it, bool t)
{
 messages.add("You pull the pin on the smoke bomb.");
 it->make(item::types[itm_smokebomb_act]);
 it->charges = 20;
 it->active = true;
}

void iuse::smokebomb_act(game *g, player *p, item *it, bool t)
{
 const auto pos = g->find_item(it);
 assert(pos);
 if (t) {
  if (it->charges > 17) g->sound(*pos, 0, "Tick.");	// Vol 0 = only heard if you hold it
  else {
   for (int i = -2; i <= 2; i++) {
    for (int j = -2; j <= 2; j++) {
	 point dest(pos->x + i, pos->y + j);
     if (g->m.sees(*pos, dest, 3) && g->m.move_cost(dest) > 0)
      g->m.add_field(g, dest, fd_smoke, rng(1, 2) + rng(0, 1));
    }
   }
  }
 } else it->make(item::types[itm_canister_empty]);
}

void iuse::acidbomb(game *g, player *p, item *it, bool t)
{
 messages.add("You remove the divider, and the chemicals mix.");
 p->moves -= 150;
 it->make(item::types[itm_acidbomb_act]);
 it->charges = 1;
 it->bday = int(messages.turn);
 it->active = true;
}
 
void iuse::acidbomb_act(game *g, player *p, item *it, bool t)
{
 if (!p->has_item(it)) {
  auto pos = g->find_item(it);
  assert(pos);
  it->charges = 0;
  for (int x = pos->x - 1; x <= pos->x + 1; x++) {
   for (int y = pos->y - 1; y <= pos->y + 1; y++)
    g->m.add_field(g, x, y, fd_acid, 3);
  }
 }
}

void iuse::molotov(game *g, player *p, item *it, bool t)
{
 if (!p->has_charges(itm_lighter, 1)) {
  messages.add("You need a lighter!");
  return;
 }
 p->use_charges(itm_lighter, 1);
 messages.add("You light the molotov cocktail.");
 p->moves -= 150;
 it->make(item::types[itm_molotov_lit]);
 it->charges = 1;
 it->bday = int(messages.turn);
 it->active = true;
}
 
void iuse::molotov_lit(game *g, player *p, item *it, bool t)
{
 int age = int(messages.turn) - it->bday;
 if (!p->has_item(it)) {
  it->charges = -1;
  g->explosion(g->find_item(it).value(), 8, 0, true);
 } else if (age >= 5) { // More than 5 turns old = chance of going out
  if (rng(1, 50) < age) {
   messages.add("Your lit molotov goes out.");
   it->make(item::types[itm_molotov]);
   it->charges = 0;
   it->active = false;
  }
 }
}

void iuse::dynamite(game *g, player *p, item *it, bool t)
{
 if (!p->has_charges(itm_lighter, 1)) {
  messages.add("You need a lighter!");
  return;
 }
 p->use_charges(itm_lighter, 1);
 messages.add("You light the dynamite.");
 it->make(item::types[itm_dynamite_act]);
 it->charges = 20;
 it->active = true;
}

void iuse::dynamite_act(game *g, player *p, item *it, bool t)
{
 const auto pos = g->find_item(it);
 assert(pos);
 if (t) g->sound(*pos, 0, "ssss...");	 // Simple timer effects
 else g->explosion(*pos, 60, 0, false);		// When that timer runs down...
}

void iuse::mininuke(game *g, player *p, item *it, bool t)
{
 messages.add("You activate the mininuke.");
 it->make(item::types[itm_mininuke_act]);
 it->charges = 10;
 it->active = true;
}

void iuse::mininuke_act(game *g, player *p, item *it, bool t)
{
 const auto pos = g->find_item(it);
 assert(pos);
 if (t) g->sound(*pos, 2, "Tick."); 	// Simple timer effects
 else {	// When that timer runs down...
  g->explosion(*pos, 200, 0, false);
  for (int i = -4; i <= 4; i++) {
   for (int j = -4; j <= 4; j++) {
	point dest(pos->x + i, pos->y + j);
    if (g->m.sees(*pos, dest, 3) && g->m.move_cost(dest) > 0)
     g->m.add_field(g, dest, fd_nuke_gas, 3);
   }
  }
 }
}

void iuse::pheromone(game *g, player *p, item *it, bool t)
{
 point pos(p->pos);

 bool is_u = !p->is_npc(), can_see = (is_u || g->u_see(p->pos));
 if (pos.x == -999 || pos.y == -999) return;

 if (is_u)
  messages.add("You squeeze the pheromone ball...");
 else if (can_see)
  messages.add("%s squeezes a pheromone ball...", p->name.c_str());
 p->moves -= 15;

 int converts = 0;
 for (int x = pos.x - 4; x <= pos.x + 4; x++) {
  for (int y = pos.y - 4; y <= pos.y + 4; y++) {
   if (monster* const m_at = g->mon(x,y)) {
     if (m_at->symbol() == 'Z' && m_at->is_enemy(p) && rng(0, 500) > m_at->hp) {
      converts++;
	  m_at->make_friendly();
     }
   }
  }
 }

 if (can_see) {
  if (converts == 0)
   messages.add("...but nothing happens.");
  else if (converts == 1)
   messages.add("...and a nearby zombie turns friendly!");
  else
   messages.add("...and several nearby zombies turn friendly!");
 }
}
 

void iuse::portal(game *g, player *p, item *it, bool t)
{
 g->m.add_trap(p->pos + rng(within_rldist<2>), tr_portal);
}

void iuse::manhack(game *g, player *p, item *it, bool t)
{
 std::vector<point> valid;	// Valid spawn locations
 for (decltype(auto) delta : Direction::vector) {
     const point pt(p->pos + delta);
     if (g->is_empty(pt)) valid.push_back(pt);
 }
 if (valid.empty()) {	// No valid points!
  messages.add("There is no adjacent square to release the manhack in!");
  return;
 }
 p->moves -= 60;
 it->invlet = 0; // Remove the manhack from the player's inv
 monster manhack(mtype::types[mon_manhack], valid[rng(0, valid.size() - 1)]);
 if (rng(0, p->int_cur / 2) + p->sklevel[sk_electronics] / 2 +
     p->sklevel[sk_computer] < rng(0, 4))
  messages.add("You misprogram the manhack; it's hostile!");
 else
  manhack.friendly = -1;
 g->z.push_back(manhack);
}

void iuse::turret(game *g, player *p, item *it, bool t)
{
 g->draw();
 mvprintw(0, 0, "Place where?");
 point dir(get_direction(input()));
 if (dir.x == -2) {
  messages.add("Invalid direction.");
  return;
 }
 p->moves -= 100;
 dir += p->pos;
 if (!g->is_empty(dir)) {
  messages.add("You cannot place a turret there.");
  return;
 }
 it->invlet = 0; // Remove the turret from the player's inv
 monster turret(mtype::types[mon_turret], dir);
 if (rng(0, p->int_cur / 2) + p->sklevel[sk_electronics] / 2 +
     p->sklevel[sk_computer] < rng(0, 6))
  messages.add("You misprogram the turret; it's hostile!");
 else
  turret.friendly = -1;
 g->z.push_back(turret);
}

void iuse::UPS_off(game *g, player *p, item *it, bool t)
{
 if (it->charges == 0) messages.add("The power supply's batteries are dead.");
 else {
  messages.add("You turn the power supply on.");
  if (p->is_wearing(itm_goggles_nv)) messages.add("Your light amp goggles power on.");
  it->make(item::types[itm_UPS_on]);
  it->active = true;
 }
}
 
void iuse::UPS_on(game *g, player *p, item *it, bool t)
{
 if (t) {	// Normal use
	// Does nothing
 } else {	// Turning it off
  messages.add("The UPS powers off with a soft hum.");
  it->make(item::types[itm_UPS_off]);
  it->active = false;
 }
}

void iuse::tazer(game *g, player *p, item *it, bool t)
{
 static const int tazer_hit_modifier[mtype::MS_MAX] = {-2, -1, 0, 2, 4};

 g->draw();
 mvprintw(0, 0, "Shock in which direction?");
 point dir(get_direction(input()));
 if (dir.x == -2) {
  messages.add("Invalid direction.");
  it->charges += (dynamic_cast<const it_tool*>(it->type))->charges_per_use;
  return;
 }
 const auto target(dir + p->GPSpos);
 monster* const z = g->mon(target);
 npc* const foe = g->nPC(target);
 if (!z && !foe) {
  messages.add("Your tazer crackles in the air."); // XXX no time cost for this?
  return;
 }

 int numdice = 3 + (p->dex_cur / 2.5) + p->sklevel[sk_melee] * 2;
 p->moves -= mobile::mp_turn;

 if (z) {
  numdice += tazer_hit_modifier[z->type->size];
  if (dice(numdice, 10) < dice(z->dodge(), 10)) {	// A miss!
   messages.add("You attempt to shock the %s, but miss.", z->name().c_str());
   return;
  }
  messages.add("You shock the %s!", z->name().c_str());
  int shock = rng(5, 25);
  z->moves -= shock * mobile::mp_turn;
  if (z->hurt(shock)) g->kill_mon(*z, p);
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
  if (foe->hp_cur[hp_head]  <= 0 || foe->hp_cur[hp_torso] <= 0) foe->die(g, true);
 }

}

void iuse::mp3(game *g, player *p, item *it, bool t)
{
 if (it->charges == 0)
  messages.add("The mp3 player's batteries are dead.");
 else if (p->has_active_item(itm_mp3_on))
  messages.add("You are already listening to an mp3 player!");
 else {
  messages.add("You put in the earbuds and start listening to music.");
  it->make(item::types[itm_mp3_on]);
  it->active = true;
 }
}

void iuse::mp3_on(game *g, player *p, item *it, bool t)
{
 if (t) {	// Normal use
  if (!p->has_item(it)) return;	// We're not carrying it!
  p->add_morale(MORALE_MUSIC, 1, 50);

  if (int(messages.turn) % 10 == 0) {	// Every 10 turns, describe the music
   std::string sound = "";
   switch (rng(1, 10)) {
    case 1: sound = "a sweet guitar solo!";	p->stim++;	break;
    case 2: sound = "a funky bassline.";			break;
    case 3: sound = "some amazing vocals.";			break;
    case 4: sound = "some pumping bass.";			break;
    case 5: sound = "dramatic classical music.";
      if (p->int_cur >= 10) p->add_morale(MORALE_MUSIC, 1, 100);
	  break;
   }
   if (sound.length() > 0) messages.add("You listen to %s", sound.c_str());
  }
 } else {	// Turning it off
  messages.add("The mp3 player turns off.");
  it->make(item::types[itm_mp3]);
  it->active = false;
 }
}

void iuse::vortex(game *g, player *p, item *it, bool t)
{
 std::vector<point> spawn;
 for (int i = -3; i <= 3; i++) {
  point test(p->pos.x - 3, p->pos.y+i);
  if (g->is_empty(test)) spawn.push_back(test);
  test = point(p->pos.x + 3, p->pos.y + i);
  if (g->is_empty(test)) spawn.push_back(test);
  test = point(p->pos.x + i, p->pos.y - 3);
  if (g->is_empty(test)) spawn.push_back(test);
  test = point(p->pos.x + i, p->pos.y + 3);
  if (g->is_empty(test)) spawn.push_back(test);
 }
 if (spawn.empty()) {
  if (!p->is_npc()) messages.add("Air swirls around you for a moment.");
  it->make(item::types[itm_spiral_stone]);
  return;
 }

 messages.add("Air swirls all over...");
 p->moves -= 100;
 it->make(item::types[itm_spiral_stone]);
 monster vortex(mtype::types[mon_vortex], spawn[rng(0, spawn.size() - 1)]);
 vortex.friendly = -1;
 g->z.push_back(vortex);
}

void iuse::dog_whistle(game *g, player *p, item *it, bool t)
{
 if (!p->is_npc()) messages.add("You blow your dog whistle.");
 for (decltype(auto) _dog : g->z) {
     if (_dog.is_friend(p) && _dog.type->id == mon_dog) {
         const bool is_docile = _dog.has_effect(ME_DOCILE);
         if (g->u.see(_dog)) {
             const auto d_name = _dog.desc(grammar::noun::role::subject);
             if (is_docile) messages.add("Your %s looks ready to attack.", d_name.c_str());
             else messages.add("Your %s goes docile.", d_name.c_str());
         }
         if (is_docile) _dog.rem_effect(ME_DOCILE);
         else _dog.add_effect(ME_DOCILE, -1);
     }
 }
}

void iuse::vacutainer(game *g, player *p, item *it, bool t)	// XXX disabled for npcs; \todo fix
{
 if (p->is_npc()) return; // No NPCs for now!

 if (!it->contents.empty()) {
  messages.add("That %s is full!", it->tname().c_str());
  return;
 }

 item blood(item::types[itm_blood], messages.turn);
 bool drew_blood = false;
 for(const auto& it : g->m.i_at(p->GPSpos)) {
  if (it.type->id == itm_corpse && query_yn("Draw blood from %s?", it.tname().c_str())) {
   blood.corpse = it.corpse;
   drew_blood = true;
   break;
  }
 }

 if (!drew_blood && !query_yn("Draw your own blood?")) return;

 it->put_in(blood);
}
 

/* MACGUFFIN FUNCTIONS
 * These functions should refer to it->associated_mission for the particulars
 */
void iuse::mcg_note(game *g, player *p, item *it, bool t)
{
 std::ostringstream message;
 message << "Dear " << it->name << ":\n";
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

void iuse::artifact(game *g, player *p, item *it, bool t)
{
 if (!it->is_artifact()) {
  debugmsg("iuse::artifact called on a non-artifact item! %s",
           it->tname().c_str());
  return;
 } else if (!it->is_tool()) {
  debugmsg("iuse::artifact called on a non-tool artifact! %s",
           it->tname().c_str());
  return;
 }
 const it_artifact_tool* const art = dynamic_cast<const it_artifact_tool*>(it->type);
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
         (blood || g->u_see(x, y)))
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
