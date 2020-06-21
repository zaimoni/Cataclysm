#ifndef _DISEASE_H_
#define _DISEASE_H_

// This is not a normal include file; it "should be" inlined into player.cpp.  Just check that required headers we control are in place.
#ifndef _GAME_H_
#error need to include game.h
#endif
#ifndef _BODYPART_H_
#error need to include bodypart.h
#endif

// Permanent disease capped at 3 days
#define MIN_DISEASE_AGE DAYS(-3)

struct stat_delta {
    int Str;
    int Dex;
    int Per;
    int Int;
    int Speed;
};

stat_delta dis_stat_effects(const player& p, const disease& dis)
{
    stat_delta ret = { 0,0,0,0,0 };
    int bonus;
    switch (dis.type) {
    case DI_GLARE:
        ret.Per = -1;
        break;

    case DI_COLD:
        ret.Dex -= dis.duration / MINUTES(8);
        break;

    case DI_COLD_FACE:
        ret.Per -= dis.duration / MINUTES(8);
        break;

    case DI_COLD_HANDS:
        ret.Dex -= 1 + dis.duration / MINUTES(4);
        break;

    case DI_HOT:
        ret.Int = -1;
        break;

    case DI_HEATSTROKE:
        ret.Str = -2;
        ret.Per = -1;
        ret.Int = -2;
        break;

    case DI_FBFACE:
        ret.Per = -2;
        break;

    case DI_FBHANDS:
        ret.Dex = -4;
        break;

    case DI_FBFEET:
        ret.Str = -1;
        break;

    case DI_COMMON_COLD:
        if (p.has_disease(DI_TOOK_FLUMED)) {
            ret.Str = -1;
            ret.Int = -1;
        } else {
            ret.Str = -3;
            ret.Dex = -1;
            ret.Int = -2;
            ret.Per = -1;
        }
        break;

    case DI_FLU:
        if (p.has_disease(DI_TOOK_FLUMED)) {
            ret.Str = -2;
            ret.Int = -1;
        } else {
            ret.Str = -4;
            ret.Dex = -2;
            ret.Int = -2;
            ret.Per = -1;
        }
        break;

    case DI_SMOKE:
        ret.Str = -1;
        ret.Int = -1;
        break;

    case DI_TEARGAS:
        ret.Str = -2;
        ret.Dex = -2;
        ret.Int = -1;
        ret.Per = -4;
        break;

    case DI_BOOMERED:
        ret.Per = -5;
        break;

    case DI_SAP:
        ret.Dex = -3;
        break;

    case DI_FUNGUS:
        ret.Str = -1;
        ret.Dex = -1;
        break;

    case DI_SLIMED:
        ret.Dex = -2;
        break;

    case DI_DRUNK:
        // We get 600 turns, or one hour, of DI_DRUNK for each drink we have (on avg)
        // So, the duration of DI_DRUNK is a good indicator of how much alcohol is in
        //  our system.
        ret.Per -= int(dis.duration / MINUTES(100));
        ret.Dex -= int(dis.duration / MINUTES(100));
        ret.Int -= int(dis.duration / MINUTES(70));
        ret.Str -= int(dis.duration / MINUTES(150));
        if (dis.duration <= HOURS(1)) ret.Str += 1;
        break;

    case DI_CIG:
        if (dis.duration >= HOURS(1)) {	// Smoked too much
            ret.Str = -1;
            ret.Dex = -1;
        } else {
            ret.Dex = 1;
            ret.Int = 1;
            ret.Per = 1;
        }
        break;

    case DI_HIGH:
        ret.Int = -1;
        ret.Per = -1;
        break;

    case DI_THC:
        ret.Int = -1;
        ret.Per = -1;
        break;

    case DI_POISON:
        ret.Per = -1;
        ret.Dex = -1;
        if (!p.has_trait(PF_POISRESIST)) ret.Str = -2;
        break;

    case DI_BADPOISON:
        ret.Per = -2;
        ret.Dex = -2;
        ret.Str = p.has_trait(PF_POISRESIST) ? -1 : -3;
        break;

    case DI_FOODPOISON:
        ret.Str = p.has_trait(PF_POISRESIST) ? -1 : -3;
        ret.Per = -1;
        ret.Dex = -1;
        break;

    case DI_SHAKES:
        ret.Dex = -4;
        ret.Str = -1;
        break;

    case DI_WEBBED:
        ret.Str = -2;
        ret.Dex = -4;
        break;

    case DI_RAT:
        ret.Int -= dis.duration / MINUTES(2);
        ret.Str -= dis.duration / MINUTES(5);
        ret.Per -= dis.duration / (MINUTES(5) / 2);
        break;

    case DI_FORMICATION:
        ret.Int = -2;
        ret.Str = -1;
        break;

    case DI_HALLU:
        // This assumes that we were given DI_HALLU with a 3600 (6-hour) lifespan
        if (HOURS(4) > dis.duration) {	// Full symptoms
            ret.Per = -2;
            ret.Int = -1;
            ret.Dex = -2;
            ret.Str = -1;
        }
        break;

    case DI_ADRENALINE:
        if (MINUTES(15) < dis.duration) {	// 5 minutes positive effects
            ret.Str = 5;
            ret.Dex = 3;
            ret.Int = -8;
            ret.Per = 1;
        } else if (MINUTES(15) > dis.duration) {	// 15 minutes come-down
            ret.Str = -2;
            ret.Dex = -1;
            ret.Int = -1;
            ret.Per = -1;
        }
        break;

    case DI_ASTHMA:
        ret.Str = -2;
        ret.Dex = -3;
        break;

    case DI_METH:
        if (dis.duration > HOURS(1)) {
            ret.Str = 2;
            ret.Dex = 2;
            ret.Int = 3;
            ret.Per = 3;
        } else {
            ret.Str = -3;
            ret.Dex = -2;
            ret.Int = -1;
        }
        break;

    case DI_EVIL: {
        bool lesser = false; // Worn or wielded; diminished effects
        if (p.weapon.is_artifact() && p.weapon.is_tool()) {
            const it_artifact_tool* const tool = dynamic_cast<const it_artifact_tool*>(p.weapon.type);
            lesser = any(tool->effects_carried, AEP_EVIL) || any(tool->effects_wielded, AEP_EVIL);
        }
        if (!lesser) {
            for (const auto& it : p.worn) {
                if (!it.is_artifact()) continue;
                if (lesser = any(dynamic_cast<const it_artifact_armor*>(it.type)->effects_worn, AEP_EVIL)) break;
            }
        }

        if (lesser) { // Only minor effects, some even good!
            ret.Str += (dis.duration > MINUTES(450) ? 10 : dis.duration / MINUTES(45));
            if (dis.duration < HOURS(1))
                ret.Dex++;
            else
                ret.Dex -= (dis.duration > HOURS(6) ? 10 : (dis.duration - HOURS(1)) / MINUTES(30));
            ret.Int -= (dis.duration > MINUTES(300) ? 10 : (dis.duration - MINUTES(50)) / MINUTES(25));
            ret.Per -= (dis.duration > MINUTES(480) ? 10 : (dis.duration - MINUTES(80)) / MINUTES(40));
        }
        else { // Major effects, all bad.
            ret.Str -= (dis.duration > MINUTES(500) ? 10 : dis.duration / MINUTES(50));
            ret.Dex -= (dis.duration > HOURS(1) ? 10 : dis.duration / HOURS(1));
            ret.Int -= (dis.duration > MINUTES(450) ? 10 : dis.duration / MINUTES(45));
            ret.Per -= (dis.duration > MINUTES(400) ? 10 : dis.duration / MINUTES(40));
        }
    } break;
    }
    return ret;
}

void dis_effect(game *g, player &p, disease& dis)
{
 const auto delta = dis_stat_effects(p, dis);
 p.str_cur += delta.Str;
 p.dex_cur += delta.Dex;
 p.int_cur += delta.Int;
 p.per_cur += delta.Per;

 int bonus;
 switch (dis.type) {
 case DI_WET:
  p.add_morale(MORALE_WET, -1, -50);
  break;

 case DI_COLD_FACE:
  if (dis.duration >= MINUTES(20) || (dis.duration >= MINUTES(10) && one_in(MINUTES(30) - dis.duration)))
   p.add_disease(DI_FBFACE, MINUTES(5));
  break;

 case DI_COLD_HANDS:
  if (dis.duration >= MINUTES(20) || (dis.duration >= MINUTES(10) && one_in(MINUTES(30) - dis.duration)))
   p.add_disease(DI_FBHANDS, MINUTES(5));
  break;

 case DI_COLD_FEET:
  if (dis.duration >= MINUTES(20) || (dis.duration >= MINUTES(10) && one_in(MINUTES(30) - dis.duration)))
   p.add_disease(DI_FBFEET, MINUTES(5));
  break;

 case DI_HOT:
  if (rng(0, MINUTES(50)) < dis.duration) p.add_disease(DI_HEATSTROKE, TURNS(2));
  break;
   
 case DI_COMMON_COLD:
  if (int(messages.turn) % 300 == 0) p.thirst++;
  if (one_in(300)) {
   p.moves -= 80;
   if (!p.is_npc()) {
    messages.add("You cough noisily.");
    g->sound(p.pos, 12, "");
   } else
    g->sound(p.pos, 12, "loud coughing");
  }
  break;

 case DI_FLU:
  if (int(messages.turn) % 300 == 0) p.thirst++;
  if (one_in(300)) {
   p.moves -= 80;
   if (!p.is_npc()) {
    messages.add("You cough noisily.");
    g->sound(p.pos, 12, "");
   } else
    g->sound(p.pos, 12, "loud coughing");
  }
  if (one_in(3600) || (p.has_trait(PF_WEAKSTOMACH) && one_in(3000)) || (p.has_trait(PF_NAUSEA) && one_in(2400))) {
   if (!p.has_disease(DI_TOOK_FLUMED) || one_in(2)) p.vomit();
  }
  break;

 case DI_SMOKE:
  if (one_in(5)) {
   if (!p.is_npc()) {
    messages.add("You cough heavily.");
    g->sound(p.pos, 12, "");
   } else
    g->sound(p.pos, 12, "a hacking cough.");
   p.moves -= 80;
   p.hurt(g, bp_torso, 0, 1 - (rng(0, 1) * rng(0, 1)));
  }
  break;

 case DI_TEARGAS:
  if (one_in(3)) {
   if (!p.is_npc()) {
    messages.add("You cough heavily.");
    g->sound(p.pos, 12, "");
   } else
    g->sound(p.pos, 12, "a hacking cough");
   p.moves -= 100;
   p.hurt(g, bp_torso, 0, rng(0, 3) * rng(0, 1));
  }
  break;

 case DI_ONFIRE:
  p.hurtall(3);
  for (int i = 0; i < p.worn.size(); i++) {
   if (p.worn[i].made_of(VEGGY) || p.worn[i].made_of(PAPER)) {  // \todo V0.2.3+ want to handle wood here but having it auto-destruct doesn't seem reasonable
    EraseAt(p.worn, i);
    i--;
   } else if ((p.worn[i].made_of(COTTON) || p.worn[i].made_of(WOOL)) && one_in(10)) {
    EraseAt(p.worn, i);
    i--;
   } else if (p.worn[i].made_of(PLASTIC) && one_in(50)) {	// \todo V0.2.1+ thermoplastic might melt on the way which also causes damage
    EraseAt(p.worn, i);
    i--;
   }
  }
  break;

 case DI_SPORES:
  if (one_in(30)) p.add_disease(DI_FUNGUS, -1);
  break;

 case DI_FUNGUS:
  bonus = p.has_trait(PF_POISRESIST) ? 100 : 0;
  p.moves -= 10;
  if (dis.duration > HOURS(-1)) {	// First hour symptoms
   if (one_in(160 + bonus)) {
    if (!p.is_npc()) {
     messages.add("You cough heavily.");
     g->sound(p.pos, 12, "");
    } else
     g->sound(p.pos, 12, "a hacking cough");
    p.pain++;
   }
   if (one_in(100 + bonus)) {
    if (!p.is_npc()) messages.add("You feel nauseous.");
   }
   if (one_in(100 + bonus)) {
    if (!p.is_npc()) messages.add("You smell and taste mushrooms.");
   }
  } else if (dis.duration > HOURS(-6)) {	// One to six hours
   if (one_in(600 + bonus * 3)) {
    if (!p.is_npc()) messages.add("You spasm suddenly!");
    p.moves -= 100;
    p.hurt(g, bp_torso, 0, 5);
   }
   if ((p.has_trait(PF_WEAKSTOMACH) && one_in(1600 + bonus *  8)) ||
       (p.has_trait(PF_NAUSEA) && one_in(800 + bonus * 6)) ||
       one_in(2000 + bonus * 10)) {
    if (!p.is_npc()) messages.add("You vomit a thick, gray goop.");
    else if (g->u_see(p.pos)) messages.add("%s vomits a thick, gray goop.", p.name.c_str());
    p.moves = -200;
    p.hunger += 50;
    p.thirst += 68;
   }
  } else {	// Full symptoms
   if (one_in(1000 + bonus * 8)) {
    if (!p.is_npc()) messages.add("You double over, spewing live spores from your mouth!");
    else if (g->u_see(p.pos)) messages.add("%s coughs up a stream of live spores!", p.name.c_str());
    p.moves = -500;
    monster spore(mtype::types[mon_spore]);
    for (int i = -1; i <= 1; i++) {
     for (int j = -1; j <= 1; j++) {
	  point dest(p.pos.x + i, p.pos.y + j);
	  monster* const m_at = g->mon(dest);
      if (g->m.move_cost(dest) > 0 && one_in(5)) {
       if (m_at) {	// Spores hit a monster
        if (g->u_see(dest)) messages.add("The %s is covered in tiny spores!", m_at->name().c_str());
        if (!m_at->make_fungus()) g->kill_mon(*m_at);
       } else {	// \todo infect npcs
        spore.spawn(dest);
        g->z.push_back(spore);
       }
      }
     }
    }
   } else if (one_in(6000 + bonus * 20)) {
    if (!p.is_npc()) messages.add("Fungus stalks burst through your hands!");
    else if (g->u_see(p.pos)) messages.add("Fungus stalks burst through %s's hands!", p.name.c_str());
    p.hurt(g, bp_arms, 0, 60);
    p.hurt(g, bp_arms, 1, 60);
   }
  }
  break;

 case DI_LYING_DOWN:
  p.moves = 0;
  if (p.can_sleep(g->m)) {
   dis.duration = 1;
   if (!p.is_npc()) messages.add("You fall asleep.");
   p.add_disease(DI_SLEEP, HOURS(10));
  }
  if (dis.duration == 1 && !p.has_disease(DI_SLEEP))
   if (!p.is_npc()) messages.add("You try to sleep, but can't...");
  break;

 case DI_SLEEP:
  p.moves = 0;
  if (int(messages.turn) % 25 == 0) {
   if (p.fatigue > 0) p.fatigue -= 1 + rng(0, 1) * rng(0, 1);
   if (p.has_trait(PF_FASTHEALER))
    p.healall(rng(0, 1));
   else
    p.healall(rng(0, 1) * rng(0, 1) * rng(0, 1));
   if (p.fatigue <= 0 && p.fatigue > -20) {
    p.fatigue = -25;
    messages.add("Fully rested.");
    dis.duration = dice(3, 100);
   }
  }
  if (int(messages.turn) % 100 == 0 && !p.has_bionic(bio_recycler)) {
// Hunger and thirst advance more slowly while we sleep.
   p.hunger--;
   p.thirst--;
  }
  if (rng(5, 80) + rng(0, 120) + rng(0, abs(p.fatigue)) +
      rng(0, abs(p.fatigue * 5)) < g->light_level() &&
      (p.fatigue < 10 || one_in(p.fatigue / 2))) {
   messages.add("The light wakes you up.");
   dis.duration = 1;
  }
  break;

 case DI_PKILL1:
  if (dis.duration <= MINUTES(7) && dis.duration % 7 == 0 && p.pkill < 15)
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
  if (dis.duration <= HOURS(1)) p.str_cur += 1;
  if (dis.duration > MINUTES(200) + MINUTES(10) * dice(2, 100) && 
      (p.has_trait(PF_WEAKSTOMACH) || p.has_trait(PF_NAUSEA) || one_in(20)))
   p.vomit();
  if (!p.has_disease(DI_SLEEP) && dis.duration >= MINUTES(450) &&
      one_in(500 - int(dis.duration / MINUTES(8)))) {
   if (!p.is_npc()) messages.add("You pass out.");
   p.add_disease(DI_SLEEP, dis.duration / 2);
  }
  break;

 case DI_CIG:
  if (dis.duration >= HOURS(1)) {	// Smoked too much
   if (dis.duration >= HOURS(2) && (one_in(50) ||
                                (p.has_trait(PF_WEAKSTOMACH) && one_in(30)) ||
                                (p.has_trait(PF_NAUSEA) && one_in(20))))
    p.vomit();
  }
  break;

 case DI_POISON:
  if (one_in(p.has_trait(PF_POISRESIST) ? 900 : 150)) {
   if (!p.is_npc()) messages.add("You're suddenly wracked with pain!");
   p.pain++;
   p.hurt(g, bp_torso, 0, rng(0, 2) * rng(0, 1));
  }
  break;

 case DI_BADPOISON:
  if (one_in(p.has_trait(PF_POISRESIST) ? 500 : 100)) {
   if (!p.is_npc()) messages.add("You're suddenly wracked with pain!");
   p.pain += 2;
   p.hurt(g, bp_torso, 0, rng(0, 2));
  }
  break;

 case DI_FOODPOISON:
  bonus = p.has_trait(PF_POISRESIST) ? 600 : 0;
  if (one_in(300 + bonus)) {
   if (!p.is_npc()) messages.add("You're suddenly wracked with pain and nausea!");
   p.hurt(g, bp_torso, 0, 1);
  }
  if ((p.has_trait(PF_WEAKSTOMACH) && one_in(300 + bonus)) ||
      (p.has_trait(PF_NAUSEA) && one_in(50 + bonus)) ||
      one_in(600 + bonus)) 
   p.vomit();
  break;

 case DI_DERMATIK: {
  int formication_chance = 600;
  if (dis.duration > HOURS(-4) && dis.duration < 0) formication_chance = 2400 + dis.duration;
  if (one_in(formication_chance)) p.add_disease(DI_FORMICATION, HOURS(2));

  if (dis.duration < HOURS(-4) && one_in(2400)) p.vomit();

  if (dis.duration < DAYS(-1)) { // Spawn some larvae!
// Choose how many insects; more for large characters
   int num_insects = 1;
   while (num_insects < 6 && rng(0, 10) < p.str_max) num_insects++;
// Figure out where they may be placed
   std::vector<point> valid_spawns;
   for (int x = p.pos.x - 1; x <= p.pos.y + 1; x++) {
    for (int y = p.pos.y - 1; y <= p.pos.y + 1; y++) {
     if (g->is_empty(x, y))
      valid_spawns.push_back(point(x, y));
    }
   }
   if (valid_spawns.size() >= 1) {
    p.rem_disease(DI_DERMATIK); // No more infection!  yay.
    if (!p.is_npc()) messages.add("Insects erupt from your skin!");
    else if (g->u_see(p.pos)) messages.add("Insects erupt from %s's skin!", p.name.c_str());
    p.moves -= 600;
    monster grub(mtype::types[mon_dermatik_larva]);
    while (valid_spawns.size() > 0 && num_insects > 0) {
     num_insects--;
// Hurt the player
     body_part burst = bp_torso;
     if (one_in(3)) burst = bp_arms;
     else if (one_in(3)) burst = bp_legs;
     p.hurt(g, burst, rng(0, 1), rng(4, 8));
// Spawn a larva
     int sel = rng(0, valid_spawns.size() - 1);
     grub.spawn(valid_spawns[sel]);
     valid_spawns.erase(valid_spawns.begin() + sel);
// Sometimes it's a friendly larva!
	 grub.friendly = (one_in(3) ? -1 : 0);
     g->z.push_back(grub);
    }
   }
  }
 } break;

 case DI_RAT:
  if (rng(30, 100) < rng(0, dis.duration) && one_in(3)) p.vomit();
  if (rng(0, 100) < rng(0, dis.duration)) p.mutation_category_level[MUTCAT_RAT]++;
  if (rng(50, 500) < rng(0, dis.duration)) p.mutate();
  break;

 case DI_FORMICATION:
  if (one_in(10 + 40 * p.int_cur)) {
   if (!p.is_npc())
    messages.add("You start scratching yourself all over!");
   else if (g->u_see(p.pos))
    messages.add("%s starts scratching %s all over!", p.name.c_str(), (p.male ? "himself" : "herself"));
   p.cancel_activity();
   p.moves -= 150;
   p.hurt(g, bp_torso, 0, 1);
  }
  break;

 case DI_HALLU:
// This assumes that we were given DI_HALLU with a 3600 (6-hour) lifespan
  if (dis.duration > HOURS(5)) {	// First hour symptoms
   if (one_in(300)) {
    if (!p.is_npc()) messages.add("You feel a little strange.");
   }
  } else if (dis.duration > HOURS(4)) {	// Coming up
   if (one_in(100) || (p.has_trait(PF_WEAKSTOMACH) && one_in(100))) {
    if (!p.is_npc()) messages.add("You feel nauseous.");
    p.hunger -= 5;
   }
   if (!p.is_npc()) {
    if (one_in(200)) messages.add("Huh?  What was that?");
    else if (one_in(200)) messages.add("Oh god, what's happening?");
    else if (one_in(200)) messages.add("Of course... it's all fractals!");
   }
  } else if (dis.duration == HOURS(4))	// Visuals start
   p.add_disease(DI_VISUALS, HOURS(4));
  else {	// Full symptoms
   if (one_in(50)) g->z.push_back(monster(mtype::types[mon_hallu_zom + rng(0, 3)], p.pos.x + rng(-10, 10), p.pos.y + rng(-10, 10)));	// Generate phantasm
  }
  break;

 case DI_ADRENALINE: // positive/negative effects are all stat adjustments
  if (dis.duration == MINUTES(15)) {	// 15 minutes come-down
   if (!p.is_npc()) messages.add("Your adrenaline rush wears off.  You feel AWFUL!");
   p.moves -= 300;
  }
  break;

 case DI_ASTHMA:
  if (dis.duration > HOURS(2)) {
   if (!p.is_npc()) messages.add("Your asthma overcomes you.  You stop breathing and die...");
   p.hurtall(500);
  }
  break;

 case DI_ATTACK_BOOST:
 case DI_DAMAGE_BOOST:
 case DI_DODGE_BOOST:
 case DI_ARMOR_BOOST:
 case DI_SPEED_BOOST:
  if (dis.intensity > 1) dis.intensity--;
  break;

 case DI_TELEGLOW:
// Default we get around 300 duration points per teleport (possibly more
// depending on the source).
// TODO: Include a chance to teleport to the nether realm.
  if (dis.duration > HOURS(10)) {	// 20 teles (no decay; in practice at least 21)
   if (one_in(1000 - ((dis.duration - HOURS(10)) / 10))) {
    if (!p.is_npc()) messages.add("Glowing lights surround you, and you teleport.");
    g->teleport();
    if (one_in(10)) p.rem_disease(DI_TELEGLOW);
   }
   if (one_in(1200 - ((dis.duration - HOURS(10)) / 5)) && one_in(20)) {
    if (!p.is_npc()) messages.add("You pass out.");
    p.add_disease(DI_SLEEP, HOURS(2));
    if (one_in(6)) p.rem_disease(DI_TELEGLOW);
   }
  }
  if (dis.duration > HOURS(6)) { // 12 teles
   if (one_in(4000 - (dis.duration - HOURS(6))/4)) {
    int x, y, tries = 0;
    do {
     x = p.pos.x + rng(-4, 4);
     y = p.pos.y + rng(-4, 4);
     tries++;
    } while (((x == p.pos.x && y == p.pos.y) || g->mon(x, y)) && tries < 10);
    if (tries < 10) {
     if (g->m.move_cost(x, y) == 0) g->m.ter(x, y) = t_rubble;
     g->z.push_back(monster(mtype::types[(mongroup::moncats[mcat_nether])[rng(0, mongroup::moncats[mcat_nether].size() - 1)]], x, y));
     if (g->u_see(x, y)) {
      g->u.cancel_activity_query("A monster appears nearby!");
	  messages.add("A portal opens nearby, and a monster crawls through!");
     }
     if (one_in(2)) p.rem_disease(DI_TELEGLOW);
    }
   }
   if (one_in(3500 - (dis.duration - HOURS(6))/4)) {
    if (!p.is_npc()) messages.add("You shudder suddenly.");
    p.mutate();
    if (one_in(4)) p.rem_disease(DI_TELEGLOW);
   }
  }
  if (dis.duration > HOURS(4)) {	// 8 teleports
   if (one_in(10000 - dis.duration)) p.add_disease(DI_SHAKES, rng(MINUTES(4), MINUTES(8)));
   if (one_in(12000 - dis.duration)) {
    if (!p.is_npc()) messages.add("Your vision is filled with bright lights...");
    p.add_disease(DI_BLIND, rng(MINUTES(1), MINUTES(2)));
    if (one_in(8)) p.rem_disease(DI_TELEGLOW);
   }
   if (one_in(5000) && !p.has_disease(DI_HALLU)) {
    p.add_disease(DI_HALLU, HOURS(6));
    if (one_in(5)) p.rem_disease(DI_TELEGLOW);
   }
  }
  if (one_in(4000)) {
   if (!p.is_npc()) messages.add("You're suddenly covered in ectoplasm.");
   p.add_disease(DI_BOOMERED, MINUTES(10));
   if (one_in(4)) p.rem_disease(DI_TELEGLOW);
  }
  if (one_in(10000)) {
   p.add_disease(DI_FUNGUS, -1);
   p.rem_disease(DI_TELEGLOW);
  }
  break;

 case DI_ATTENTION:
  if (one_in( 100000 / dis.duration ) && one_in( 100000 / dis.duration ) && one_in(250)) {
   int x, y, tries = 0;
   do {
    x = p.pos.x + rng(-4, 4);
    y = p.pos.y + rng(-4, 4);
    tries++;
   } while (((x == p.pos.x && y == p.pos.y) || g->mon(x, y)) && tries < 10);
   if (tries < 10) {
    if (g->m.move_cost(x, y) == 0) g->m.ter(x, y) = t_rubble;
    g->z.push_back(monster(mtype::types[(mongroup::moncats[mcat_nether])[rng(0, mongroup::moncats[mcat_nether].size() - 1)]], x, y));
    if (g->u_see(x, y)) {
     g->u.cancel_activity_query("A monster appears nearby!");
     messages.add("A portal opens nearby, and a monster crawls through!");
    }
    dis.duration /= 4;
   }
  }
  break;
 }
}

#endif
