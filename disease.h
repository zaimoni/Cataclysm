#ifndef _DISEASE_H_
#define _DISEASE_H_

#include "game.h"
#include "bodypart.h"
#include <sstream>

#define MIN_DISEASE_AGE (-43200) // Permanent disease capped @ 3 days
  
void dis_effect(game *g, player &p, disease &dis)
{
 int bonus;
 int junk;
 switch (dis.type) {
 case DI_GLARE:
  p.per_cur -= 1;
  break;

 case DI_WET:
  p.add_morale(MORALE_WET, -1, -50);
  break;

 case DI_COLD:
  p.dex_cur -= int(dis.duration / 80);
  break;

 case DI_COLD_FACE:
  p.per_cur -= int(dis.duration / 80);
  if (dis.duration >= 200 ||
      (dis.duration >= 100 && one_in(300 - dis.duration)))
   p.add_disease(DI_FBFACE, 50, g);
  break;

 case DI_COLD_HANDS:
  p.dex_cur -= 1 + int(dis.duration / 40);
  if (dis.duration >= 200 ||
      (dis.duration >= 100 && one_in(300 - dis.duration)))
   p.add_disease(DI_FBHANDS, 50, g);
  break;

 case DI_COLD_LEGS:
  break;

 case DI_COLD_FEET:
  if (dis.duration >= 200 ||
      (dis.duration >= 100 && one_in(300 - dis.duration)))
   p.add_disease(DI_FBFEET, 50, g);
  break;

 case DI_HOT:
  if (rng(0, 500) < dis.duration)
   p.add_disease(DI_HEATSTROKE, 2, g);
  p.int_cur -= 1;
  break;
   
 case DI_HEATSTROKE:
  p.str_cur -=  2;
  p.per_cur -=  1;
  p.int_cur -=  2;
  break;

 case DI_FBFACE:
  p.per_cur -= 2;
  break;

 case DI_FBHANDS:
  p.dex_cur -= 4;
  break;

 case DI_FBFEET:
  p.str_cur -=  1;
  break;

 case DI_COMMON_COLD:
  if (int(g->turn) % 300 == 0)
   p.thirst++;
  if (p.has_disease(DI_TOOK_FLUMED)) {
   p.str_cur--;
   p.int_cur--;
  } else {
   p.str_cur -= 3;
   p.dex_cur--;
   p.int_cur -= 2;
   p.per_cur--;
  }
  if (one_in(300)) {
   p.moves -= 80;
   if (!p.is_npc()) {
    g->add_msg("You cough noisily.");
    g->sound(p.posx, p.posy, 12, "");
   } else
    g->sound(p.posx, p.posy, 12, "loud coughing");
  }
  break;

 case DI_FLU:
  if (int(g->turn) % 300 == 0)
   p.thirst++;
  if (p.has_disease(DI_TOOK_FLUMED)) {
   p.str_cur -= 2;
   p.int_cur--;
  } else {
   p.str_cur -= 4;
   p.dex_cur -= 2;
   p.int_cur -= 2;
   p.per_cur--;
  }
  if (one_in(300)) {
   p.moves -= 80;
   if (!p.is_npc()) {
    g->add_msg("You cough noisily.");
    g->sound(p.posx, p.posy, 12, "");
   } else
    g->sound(p.posx, p.posy, 12, "loud coughing");
  }
  if (one_in(3600) || (p.has_trait(PF_WEAKSTOMACH) && one_in(3000)) ||
      (p.has_trait(PF_NAUSEA) && one_in(2400))) {
   if (!p.has_disease(DI_TOOK_FLUMED) || one_in(2))
    p.vomit(g);
  }
  break;

 case DI_SMOKE:
  p.str_cur--;
  p.dex_cur--;
  if (one_in(5)) {
   if (!p.is_npc()) {
    g->add_msg("You cough heavily.");
    g->sound(p.posx, p.posy, 12, "");
   } else
    g->sound(p.posx, p.posy, 12, "a hacking cough.");
   p.moves -= 80;
   p.hurt(g, bp_torso, 0, 1 - (rng(0, 1) * rng(0, 1)));
  }
  break;

 case DI_TEARGAS:
  p.str_cur -= 2;
  p.dex_cur -= 2;
  p.int_cur -= 1;
  p.per_cur -= 4;
  if (one_in(3)) {
   if (!p.is_npc()) {
    g->add_msg("You cough heavily.");
    g->sound(p.posx, p.posy, 12, "");
   } else
    g->sound(p.posx, p.posy, 12, "a hacking cough");
   p.moves -= 100;
   p.hurt(g, bp_torso, 0, rng(0, 3) * rng(0, 1));
  }
  break;

 case DI_ONFIRE:
  p.hurtall(3);
  for (int i = 0; i < p.worn.size(); i++) {
   if (p.worn[i].made_of(VEGGY) || p.worn[i].made_of(PAPER) ||
       p.worn[i].made_of(PAPER)) {
    p.worn.erase(p.worn.begin() + i);
    i--;
   } else if ((p.worn[i].made_of(COTTON) || p.worn[i].made_of(WOOL)) &&
              one_in(10)) {
    p.worn.erase(p.worn.begin() + i);
    i--;
   } else if (p.worn[i].made_of(PLASTIC) && one_in(50)) {
    p.worn.erase(p.worn.begin() + i);
    i--;
   }
  }
  break;

 case DI_BOOMERED:
  p.per_cur -= 5;
  break;

 case DI_SAP:
  p.dex_cur -= 3;
  break;

 case DI_SPORES:
  if (one_in(30))
   p.add_disease(DI_FUNGUS, -1, g);
  break;

 case DI_FUNGUS:
  bonus = 0;
  if (p.has_trait(PF_POISRESIST))
   bonus = 100;
  p.moves -= 10;
  p.str_cur -= 1;
  p.dex_cur -= 1;
  if (dis.duration > -600) {	// First hour symptoms
   if (one_in(160 + bonus)) {
    if (!p.is_npc()) {
     g->add_msg("You cough heavily.");
     g->sound(p.posx, p.posy, 12, "");
    } else
     g->sound(p.posx, p.posy, 12, "a hacking cough");
    p.pain++;
   }
   if (one_in(100 + bonus)) {
    if (!p.is_npc())
     g->add_msg("You feel nauseous.");
   }
   if (one_in(100 + bonus)) {
    if (!p.is_npc())
     g->add_msg("You smell and taste mushrooms.");
   }
  } else if (dis.duration > -3600) {	// One to six hours
   if (one_in(600 + bonus * 3)) {
    if (!p.is_npc())
     g->add_msg("You spasm suddenly!");
    p.moves -= 100;
    p.hurt(g, bp_torso, 0, 5);
   }
   if ((p.has_trait(PF_WEAKSTOMACH) && one_in(1600 + bonus *  8)) ||
       (p.has_trait(PF_NAUSEA) && one_in(800 + bonus * 6)) ||
       one_in(2000 + bonus * 10)) {
    if (!p.is_npc())
     g->add_msg("You vomit a thick, gray goop.");
    else if (g->u_see(p.posx, p.posy, junk))
     g->add_msg("%s vomits a thick, gray goop.", p.name.c_str());
    p.moves = -200;
    p.hunger += 50;
    p.thirst += 68;
   }
  } else {	// Full symptoms
   if (one_in(1000 + bonus * 8)) {
    if (!p.is_npc())
     g->add_msg("You double over, spewing live spores from your mouth!");
    else if (g->u_see(p.posx, p.posy, junk))
     g->add_msg("%s coughs up a stream of live spores!", p.name.c_str());
    p.moves = -500;
    int sporex, sporey;
    monster spore(g->mtypes[mon_spore]);
    for (int i = -1; i <= 1; i++) {
     for (int j = -1; j <= 1; j++) {
      sporex = p.posx + i;
      sporey = p.posy + j;
      if (g->m.move_cost(sporex, sporey) > 0 && one_in(5)) {
       if (g->mon_at(sporex, sporey) >= 0) {	// Spores hit a monster
        if (g->u_see(sporex, sporey, junk))
         g->add_msg("The %s is covered in tiny spores!",
                    g->z[g->mon_at(sporex, sporey)].name().c_str());
        if (!g->z[g->mon_at(sporex, sporey)].make_fungus(g))
         g->kill_mon(g->mon_at(sporex, sporey));
       } else {
        spore.spawn(sporex, sporey);
        g->z.push_back(spore);
       }
      }
     }
    }
   } else if (one_in(6000 + bonus * 20)) {
    if (!p.is_npc())
     g->add_msg("Fungus stalks burst through your hands!");
    else if (g->u_see(p.posx, p.posy, junk))
     g->add_msg("Fungus stalks burst through %s's hands!", p.name.c_str());
    p.hurt(g, bp_arms, 0, 60);
    p.hurt(g, bp_arms, 1, 60);
   }
  }
  break;

 case DI_SLIMED:
  p.dex_cur -= 2;
  break;

 case DI_LYING_DOWN:
  p.moves = 0;
  if (p.can_sleep(g)) {
   dis.duration = 1;
   if (!p.is_npc())
    g->add_msg("You fall asleep.");
   p.add_disease(DI_SLEEP, 6000, g);
  }
  if (dis.duration == 1 && !p.has_disease(DI_SLEEP))
   if (!p.is_npc())
    g->add_msg("You try to sleep, but can't...");
  break;

 case DI_SLEEP:
  p.moves = 0;
  if (int(g->turn) % 25 == 0) {
   if (p.fatigue > 0)
    p.fatigue -= 1 + rng(0, 1) * rng(0, 1);
   if (p.has_trait(PF_FASTHEALER))
    p.healall(rng(0, 1));
   else
    p.healall(rng(0, 1) * rng(0, 1) * rng(0, 1));
   if (p.fatigue <= 0 && p.fatigue > -20) {
    p.fatigue = -25;
    g->add_msg("Fully rested.");
    dis.duration = dice(3, 100);
   }
  }
  if (int(g->turn) % 100 == 0 && !p.has_bionic(bio_recycler)) {
// Hunger and thirst advance more slowly while we sleep.
   p.hunger--;
   p.thirst--;
  }
  if (rng(5, 80) + rng(0, 120) + rng(0, abs(p.fatigue)) +
      rng(0, abs(p.fatigue * 5)) < g->light_level() &&
      (p.fatigue < 10 || one_in(p.fatigue / 2))) {
   g->add_msg("The light wakes you up.");
   dis.duration = 1;
  }
  break;

 case DI_PKILL1:
  if (dis.duration <= 70 && dis.duration % 7 == 0 && p.pkill < 15)
   p.pkill++;
  break;

 case DI_PKILL2:
  if (dis.duration % 7 == 0 &&
      (one_in(p.addiction_level(ADD_PKILLER)) ||
       one_in(p.addiction_level(ADD_PKILLER))   ))
   p.pkill += 2;
  break;

 case DI_PKILL3:
  if (dis.duration % 2 == 0 &&
      (one_in(p.addiction_level(ADD_PKILLER)) ||
       one_in(p.addiction_level(ADD_PKILLER))   ))
   p.pkill++;
  break;

 case DI_PKILL_L:
  if (dis.duration % 20 == 0 && p.pkill < 40 &&
      (one_in(p.addiction_level(ADD_PKILLER)) ||
       one_in(p.addiction_level(ADD_PKILLER))   ))
   p.pkill++;
  break;

 case DI_IODINE:
  if (p.radiation > 0 && one_in(16))
   p.radiation--;
  break;

 case DI_TOOK_XANAX:
  if (dis.duration % 25 == 0 && (p.stim > 0 || one_in(2)))
   p.stim--;
  break;

 case DI_DRUNK:
// We get 600 turns, or one hour, of DI_DRUNK for each drink we have (on avg)
// So, the duration of DI_DRUNK is a good indicator of how much alcohol is in
//  our system.
  p.per_cur -= int(dis.duration / 1000);
  p.dex_cur -= int(dis.duration / 1000);
  p.int_cur -= int(dis.duration /  700);
  p.str_cur -= int(dis.duration / 1500);
  if (dis.duration <= 600)
   p.str_cur += 1;
  if (dis.duration > 2000 + 100 * dice(2, 100) && 
      (p.has_trait(PF_WEAKSTOMACH) || p.has_trait(PF_NAUSEA) || one_in(20)))
   p.vomit(g);
  if (!p.has_disease(DI_SLEEP) && dis.duration >= 4500 &&
      one_in(500 - int(dis.duration / 80))) {
   if (!p.is_npc())
    g->add_msg("You pass out.");
   p.add_disease(DI_SLEEP, dis.duration / 2, g);
  }
  break;

 case DI_CIG:
  if (dis.duration >= 600) {	// Smoked too much
   p.str_cur--;
   p.dex_cur--;
   if (dis.duration >= 1200 && (one_in(50) ||
                                (p.has_trait(PF_WEAKSTOMACH) && one_in(30)) ||
                                (p.has_trait(PF_NAUSEA) && one_in(20))))
    p.vomit(g);
  } else {
   p.dex_cur++;
   p.int_cur++;
   p.per_cur++;
  }
  break;

 case DI_HIGH:
  p.int_cur--;
  p.per_cur--;
  break;

 case DI_POISON:
  if ((!p.has_trait(PF_POISRESIST) && one_in(150)) ||
      ( p.has_trait(PF_POISRESIST) && one_in(900))   ) {
   if (!p.is_npc())
    g->add_msg("You're suddenly wracked with pain!");
   p.pain++;
   p.hurt(g, bp_torso, 0, rng(0, 2) * rng(0, 1));
  }
  p.per_cur--;
  p.dex_cur--;
  if (!p.has_trait(PF_POISRESIST))
   p.str_cur -= 2;
  break;

 case DI_BADPOISON:
  if ((!p.has_trait(PF_POISRESIST) && one_in(100)) ||
      ( p.has_trait(PF_POISRESIST) && one_in(500))   ) {
   if (!p.is_npc())
    g->add_msg("You're suddenly wracked with pain!");
   p.pain += 2;
   p.hurt(g, bp_torso, 0, rng(0, 2));
  }
  p.per_cur -= 2;
  p.dex_cur -= 2;
  if (!p.has_trait(PF_POISRESIST))
   p.str_cur -= 3;
  else
   p.str_cur--;
  break;

 case DI_FOODPOISON:
  bonus = 0;
  if (p.has_trait(PF_POISRESIST))
   bonus = 600;
  if (one_in(300 + bonus)) {
   if (!p.is_npc())
    g->add_msg("You're suddenly wracked with pain and nausea!");
   p.hurt(g, bp_torso, 0, 1);
  }
  if ((p.has_trait(PF_WEAKSTOMACH) && one_in(300 + bonus)) ||
      (p.has_trait(PF_NAUSEA) && one_in(50 + bonus)) ||
      one_in(600 + bonus)) 
   p.vomit(g);
  p.str_cur -= 3;
  p.dex_cur--;
  p.per_cur--;
  if (p.has_trait(PF_POISRESIST))
   p.str_cur += 2;
  break;

 case DI_SHAKES:
  p.dex_cur -= 4;
  p.str_cur--;
  break;

 case DI_DERMATIK: {
  int formication_chance = 600;
  if (dis.duration > -2400 && dis.duration < 0)
   formication_chance = 2400 + dis.duration;
  if (one_in(formication_chance))
   p.add_disease(DI_FORMICATION, 1200, g);

  if (dis.duration < -2400 && one_in(2400))
   p.vomit(g);

  if (dis.duration < -14400) { // Spawn some larvae!
// Choose how many insects; more for large characters
   int num_insects = 1;
   while (num_insects < 6 && rng(0, 10) < p.str_max)
    num_insects++;
// Figure out where they may be placed
   std::vector<point> valid_spawns;
   for (int x = p.posx - 1; x <= p.posy + 1; x++) {
    for (int y = p.posy - 1; y <= p.posy + 1; y++) {
     if (g->is_empty(x, y))
      valid_spawns.push_back(point(x, y));
    }
   }
   if (valid_spawns.size() >= 1) {
    int t;
    p.rem_disease(DI_DERMATIK); // No more infection!  yay.
    if (!p.is_npc())
     g->add_msg("Insects erupt from your skin!");
    else if (g->u_see(p.posx, p.posy, t))
     g->add_msg("Insects erupt from %s's skin!", p.name.c_str());
    p.moves -= 600;
    monster grub(g->mtypes[mon_dermatik_larva]);
    while (valid_spawns.size() > 0 && num_insects > 0) {
     num_insects--;
// Hurt the player
     body_part burst = bp_torso;
     if (one_in(3))
      burst = bp_arms;
     else if (one_in(3))
      burst = bp_legs;
     p.hurt(g, burst, rng(0, 1), rng(4, 8));
// Spawn a larva
     int sel = rng(0, valid_spawns.size() - 1);
     grub.spawn(valid_spawns[sel].x, valid_spawns[sel].y);
     valid_spawns.erase(valid_spawns.begin() + sel);
// Sometimes it's a friendly larva!
     if (one_in(3))
      grub.friendly = -1;
     else
      grub.friendly =  0;
     g->z.push_back(grub);
    }
   }
  }
 } break;

 case DI_WEBBED:
  p.str_cur -= 2;
  p.dex_cur -= 4;
  break;

 case DI_RAT:
  p.int_cur -= int(dis.duration / 20);
  p.str_cur -= int(dis.duration / 50);
  p.per_cur -= int(dis.duration / 25);
  if (rng(30, 100) < rng(0, dis.duration) && one_in(3))
   p.vomit(g);
  if (rng(0, 100) < rng(0, dis.duration))
   p.mutation_category_level[MUTCAT_RAT]++;
  if (rng(50, 500) < rng(0, dis.duration))
   p.mutate(g);
  break;

 case DI_FORMICATION:
  p.int_cur -= 2;
  p.str_cur -= 1;
  if (one_in(10 + 40 * p.int_cur)) {
   int t;
   if (!p.is_npc()) {
    g->add_msg("You start scratching yourself all over!");
    g->cancel_activity();
   } else if (g->u_see(p.posx, p.posy, t))
    g->add_msg("%s starts scratching %s all over!", p.name.c_str(),
               (p.male ? "himself" : "herself"));
   p.moves -= 150;
   p.hurt(g, bp_torso, 0, 1);
  }
  break;

 case DI_HALLU:
// This assumes that we were given DI_HALLU with a 3600 (6-hour) lifespan
  if (dis.duration > 3000) {	// First hour symptoms
   if (one_in(300)) {
    if (!p.is_npc())
     g->add_msg("You feel a little strange.");
   }
  } else if (dis.duration > 2400) {	// Coming up
   if (one_in(100) || (p.has_trait(PF_WEAKSTOMACH) && one_in(100))) {
    if (!p.is_npc())
     g->add_msg("You feel nauseous.");
    p.hunger -= 5;
   }
   if (!p.is_npc()) {
    if (one_in(200))
     g->add_msg("Huh?  What was that?");
    else if (one_in(200))
     g->add_msg("Oh god, what's happening?");
    else if (one_in(200))
     g->add_msg("Of course... it's all fractals!");
   }
  } else if (dis.duration == 2400)	// Visuals start
   p.add_disease(DI_VISUALS, 2400, g);
  else {	// Full symptoms
   p.per_cur -= 2;
   p.int_cur -= 1;
   p.dex_cur -= 2;
   p.str_cur -= 1;
   if (one_in(50)) {	// Generate phantasm
    monster phantasm(g->mtypes[mon_hallu_zom + rng(0, 3)]);
    phantasm.spawn(p.posx + rng(-10, 10), p.posy + rng(-10, 10));
    g->z.push_back(phantasm);
   }
  }
  break;

 case DI_ADRENALINE:
  if (dis.duration > 150) {	// 5 minutes positive effects
   p.str_cur += 5;
   p.dex_cur += 3;
   p.int_cur -= 8;
   p.per_cur += 1;
  } else if (dis.duration == 150) {	// 15 minutes come-down
   if (!p.is_npc())
    g->add_msg("Your adrenaline rush wears off.  You feel AWFUL!");
   p.moves -= 300;
  } else {
   p.str_cur -= 2;
   p.dex_cur -= 1;
   p.int_cur -= 1;
   p.per_cur -= 1;
  }
  break;

 case DI_ASTHMA:
  if (dis.duration > 1200) {
   if (!p.is_npc())
    g->add_msg("Your asthma overcomes you.  You stop breathing and die...");
   p.hurtall(500);
  }
  p.str_cur -= 2;
  p.dex_cur -= 3;
  break;

 case DI_METH:
  if (dis.duration > 600) {
   p.str_cur += 2;
   p.dex_cur += 2;
   p.int_cur += 3;
   p.per_cur += 3;
  } else {
   p.str_cur -= 3;
   p.dex_cur -= 2;
   p.int_cur -= 1;
  }
  break;

 case DI_ATTACK_BOOST:
 case DI_DAMAGE_BOOST:
 case DI_DODGE_BOOST:
 case DI_ARMOR_BOOST:
 case DI_SPEED_BOOST:
  if (dis.intensity > 1)
   dis.intensity--;
  break;

 case DI_TELEGLOW:
// Default we get around 300 duration points per teleport (possibly more
// depending on the source).
// TODO: Include a chance to teleport to the nether realm.
  if (dis.duration > 6000) {	// 20 teles (no decay; in practice at least 21)
   if (one_in(1000 - ((dis.duration - 6000) / 10))) {
    if (!p.is_npc())
     g->add_msg("Glowing lights surround you, and you teleport.");
    g->teleport();
    if (one_in(10))
     p.rem_disease(DI_TELEGLOW);
   }
   if (one_in(1200 - ((dis.duration - 6000) / 5)) && one_in(20)) {
    if (!p.is_npc())
     g->add_msg("You pass out.");
    p.add_disease(DI_SLEEP, 1200, g);
    if (one_in(6))
     p.rem_disease(DI_TELEGLOW);
   }
  }
  if (dis.duration > 3600) { // 12 teles
   if (one_in(4000 - int(.25 * (dis.duration - 3600)))) {
    mon_id type = (mongroup::moncats[mcat_nether])[rng(0, mongroup::moncats[mcat_nether].size() - 1)];
    monster beast(g->mtypes[type]);
    int x, y, tries = 0;
    do {
     x = p.posx + rng(-4, 4);
     y = p.posy + rng(-4, 4);
     tries++;
    } while (((x == p.posx && y == p.posy) || g->mon_at(x, y) != -1) &&
             tries < 10);
    if (tries < 10) {
     if (g->m.move_cost(x, y) == 0)
      g->m.ter(x, y) = t_rubble;
     beast.spawn(x, y);
     g->z.push_back(beast);
     if (g->u_see(x, y, junk)) {
      g->cancel_activity_query("A monster appears nearby!");
      g->add_msg("A portal opens nearby, and a monster crawls through!");
     }
     if (one_in(2))
      p.rem_disease(DI_TELEGLOW);
    }
   }
   if (one_in(3500 - int(.25 * (dis.duration - 3600)))) {
    if (!p.is_npc())
     g->add_msg("You shudder suddenly.");
    p.mutate(g);
    if (one_in(4))
     p.rem_disease(DI_TELEGLOW);
   }
  }
  if (dis.duration > 2400) {	// 8 teleports
   if (one_in(10000 - dis.duration))
    p.add_disease(DI_SHAKES, rng(40, 80), g);
   if (one_in(12000 - dis.duration)) {
    if (!p.is_npc())
     g->add_msg("Your vision is filled with bright lights...");
    p.add_disease(DI_BLIND, rng(10, 20), g);
    if (one_in(8))
     p.rem_disease(DI_TELEGLOW);
   }
   if (one_in(5000) && !p.has_disease(DI_HALLU)) {
    p.add_disease(DI_HALLU, 3600, g);
    if (one_in(5))
     p.rem_disease(DI_TELEGLOW);
   }
  }
  if (one_in(4000)) {
   if (!p.is_npc())
    g->add_msg("You're suddenly covered in ectoplasm.");
   p.add_disease(DI_BOOMERED, 100, g);
   if (one_in(4))
    p.rem_disease(DI_TELEGLOW);
  }
  if (one_in(10000)) {
   p.add_disease(DI_FUNGUS, -1, g);
   p.rem_disease(DI_TELEGLOW);
  }
  break;

 case DI_ATTENTION:
  if (one_in( 100000 / dis.duration ) && one_in( 100000 / dis.duration ) &&
      one_in(250)) {
   mon_id type = (mongroup::moncats[mcat_nether])[rng(0, mongroup::moncats[mcat_nether].size() - 1)];
   monster beast(g->mtypes[type]);
   int x, y, tries = 0, junk;
   do {
    x = p.posx + rng(-4, 4);
    y = p.posy + rng(-4, 4);
    tries++;
   } while (((x == p.posx && y == p.posy) || g->mon_at(x, y) != -1) &&
            tries < 10);
   if (tries < 10) {
    if (g->m.move_cost(x, y) == 0)
     g->m.ter(x, y) = t_rubble;
    beast.spawn(x, y);
    g->z.push_back(beast);
    if (g->u_see(x, y, junk)) {
     g->cancel_activity_query("A monster appears nearby!");
     g->add_msg("A portal opens nearby, and a monster crawls through!");
    }
    dis.duration /= 4;
   }
  }
  break;

 case DI_EVIL: {
  bool lesser = false; // Worn or wielded; diminished effects
  if (p.weapon.is_artifact() && p.weapon.is_tool()) {
   it_artifact_tool *tool = dynamic_cast<it_artifact_tool*>(p.weapon.type);
   for (int i = 0; i < tool->effects_carried.size(); i++) {
    if (tool->effects_carried[i] == AEP_EVIL)
     lesser = true;
   }
   for (int i = 0; i < tool->effects_wielded.size(); i++) {
    if (tool->effects_wielded[i] == AEP_EVIL)
     lesser = true;
   }
  }
  for (int i = 0; !lesser && i < p.worn.size(); i++) {
   if (p.worn[i].is_artifact()) {
    it_artifact_armor *armor = dynamic_cast<it_artifact_armor*>(p.worn[i].type);
    for (int i = 0; i < armor->effects_worn.size(); i++) {
     if (armor->effects_worn[i] == AEP_EVIL)
      lesser = true;
    }
   }
  }

  if (lesser) { // Only minor effects, some even good!
   p.str_cur += (dis.duration > 4500 ? 10 : int(dis.duration / 450));
   if (dis.duration < 600)
    p.dex_cur++;
   else
    p.dex_cur -= (dis.duration > 3600 ? 10 : int((dis.duration - 600) / 300));
   p.int_cur -= (dis.duration > 3000 ? 10 : int((dis.duration - 500) / 250));
   p.per_cur -= (dis.duration > 4800 ? 10 : int((dis.duration - 800) / 400));
  } else { // Major effects, all bad.
   p.str_cur -= (dis.duration > 5000 ? 10 : int(dis.duration / 500));
   p.dex_cur -= (dis.duration > 6000 ? 10 : int(dis.duration / 600));
   p.int_cur -= (dis.duration > 4500 ? 10 : int(dis.duration / 450));
   p.per_cur -= (dis.duration > 4000 ? 10 : int(dis.duration / 400));
  }
 } break;
 }
}

#endif
