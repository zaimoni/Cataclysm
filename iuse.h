#ifndef _IUSE_H_
#define _IUSE_H_

#include <optional>
#include <any>

class game;
class item;
class player;
class npc;
class pc;

class iuse
{
 public:
// FOOD AND DRUGS (ADMINISTRATION)
  static void sewage		(player *p, item *it, bool t);
  static void royal_jelly	(player *p, item *it, bool t);
  static void bandage		(player *p, item *it, bool t);
  static void firstaid		(player *p, item *it, bool t);
  static void vitamins		(player *p, item *it, bool t);
  static void caff		(player *p, item *it, bool t);
  static void alcohol		(player *p, item *it, bool t);
  static void pkill_1		(player *p, item *it, bool t);
  static void pkill_2		(player *p, item *it, bool t);
  static void pkill_3		(player *p, item *it, bool t);
  static void pkill_4		(player *p, item *it, bool t);
  static void pkill_l		(player *p, item *it, bool t);
  static void xanax		(player *p, item *it, bool t);
  static void cig		(player *p, item *it, bool t);
  static void weed		(player *p, item *it, bool t);
  static void coke		(player *p, item *it, bool t);
  static void meth		(player *p, item *it, bool t);
  static void poison		(player *p, item *it, bool t);
  static void hallu		(player *p, item *it, bool t);
  static void thorazine	(player *p, item *it, bool t);
  static void prozac		(player *p, item *it, bool t);
  static void sleep		(player *p, item *it, bool t);
  static void iodine		(player *p, item *it, bool t);
  static void flumed		(player *p, item *it, bool t);
  static void flusleep		(player *p, item *it, bool t);
  static void inhaler		(player *p, item *it, bool t);
  static void blech		(player *p, item *it, bool t);
  static void mutagen(player& p, item& it);
  static void mutagen_3(player& p, item& it);
  static void purifier(player& p, item& it);
  static void marloss(pc& p, item& it);
  static void dogfood(pc& p, item& it); // but eating by player is unimplemented

// TOOLS
  static void lighter(pc& p, item& it);
  static void sew(pc& p, item& it);
  static void scissors(pc& p, item& it);
  static void extinguisher(pc& p, item& it);
  static void hammer(pc& p, item& it);
  static void light_off(player& p, item& it);
  static void light_on_off(item& it);
  static void water_purifier(pc& p, item& it);
  static void two_way_radio(pc& p, item& it);
  static void radio_off(pc& p, item& it);
  static void radio_on(pc& p, item& it);
  static void radio_on_off(pc& p, item& it);
  static void crowbar(pc& p, item& it);
  static void makemound(pc& p, item& it);
  static void dig(pc& p, item& it);
  static void chainsaw_off(player& p, item& it);
  static void chainsaw_on(player& p, item& it);
  static void chainsaw_on_turnoff(player& p, item& it);
  static void jackhammer(pc& p, item& it);
  static void set_trap(pc& p, item& it);
  static void geiger(pc& p, item& it);
  static void geiger_on(player& p, item& it);
  static void geiger_on_off(player& p, item& it);
  static void teleport(player& p, item& it);
  static void can_goo(player& p, item& it);
  static void pipebomb(player& p, item& it);
  static void pipebomb_act(item& it);
  static void pipebomb_act_explode(item& it);
  static void grenade(player& p, item& it);
  static void grenade_act(item& it);
  static void grenade_act_explode(item& it);
  static void flashbang(player& p, item& it);
  static void flashbang_act(item& it);
  static void flashbang_act_explode(item& it);
  static void c4(pc& p, item& it);
  static void c4armed(item& it);
  static void c4armed_explode(item& it);
  static void EMPbomb(player& p, item& it);
  static void EMPbomb_act(item& it);
  static void EMPbomb_act_explode(item& it);
  static void gasbomb(player& p, item& it);
  static void gasbomb_act(item& it);
  static void gasbomb_act_off(item& it);
  static void smokebomb(player& p, item& it);
  static void smokebomb_act(item& it);
  static void smokebomb_act_off(item& it);
  static void acidbomb(player& p, item& it);
  static void acidbomb_act(item& it);
  static void molotov(player& p, item& it);
  static void molotov_lit(item& it);
  static void dynamite(player& p, item& it);
  static void dynamite_act(item& it);
  static void dynamite_act_off(item& it);
  static void mininuke(player& p, item& it);
  static void mininuke_act(item& it);
  static void mininuke_act_off(item& it);
  static void pheromone(pc& p, item& it);
  static void pheromone(npc& p, item& it);
  static std::optional<std::any> can_use_pheromone(const npc& p);
  static void portal(player& p, item& it);
  static void manhack(pc& p, item& it);
  static void manhack(npc& p, item& it);
  static std::optional<std::any> can_use_manhack(const npc& p);
  static void turret(pc& p, item& it);
  static void UPS_off(player& p, item& it);
  static void UPS_on_off(player& p, item& it);
  static std::optional<std::any> can_use_tazer(const npc& p);
  static void tazer(pc& p, item& it);
  static void tazer(npc& p, item& it);
  static void mp3(player& p, item& it);
  static void mp3_on(player& p, item& it);
  static void mp3_on_turnoff(player& p, item& it);
  static void vortex(pc& p, item& it);
  static void dog_whistle(player& p, item& it);
  static void vacutainer(pc& p, item& it);
// MACGUFFINS
  static void mcg_note(pc& p, item& it);
// ARTIFACTS
// This function is used when an artifact is activated
// It examines the item's artifact-specific properties
// See artifact.h for a list
  static void artifact(pc& p, item& it);
  // historical declarations without definitions
/*  static void heal		(player *p, item *it, bool t);
  static void twist_space	(player *p, item *it, bool t);
  static void mass_vampire	(player *p, item *it, bool t);
  static void growth		(player *p, item *it, bool t);
  static void water		(player *p, item *it, bool t);
  static void lava		(player *p, item *it, bool t); */
  
};

#endif
