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
  static void mutagen		(player *p, item *it, bool t);
  static void mutagen_3	(player *p, item *it, bool t);
  static void purifier		(player *p, item *it, bool t);
  static void marloss		(player *p, item *it, bool t);
  static void dogfood		(player *p, item *it, bool t);

// TOOLS
  static void lighter		(player *p, item *it, bool t);
  static void sew		(player *p, item *it, bool t);
  static void scissors		(player *p, item *it, bool t);
  static void extinguisher	(player *p, item *it, bool t);
  static void hammer		(player *p, item *it, bool t);
  static void light_off	(player *p, item *it, bool t);
  static void light_on		(player *p, item *it, bool t);
  static void water_purifier	(player *p, item *it, bool t);
  static void two_way_radio	(player *p, item *it, bool t);
  static void radio_off	(player *p, item *it, bool t);
  static void radio_on		(player *p, item *it, bool t);
  static void crowbar		(player *p, item *it, bool t);
  static void makemound	(player *p, item *it, bool t);
  static void dig		(player *p, item *it, bool t);
  static void chainsaw_off	(player *p, item *it, bool t);
  static void chainsaw_on	(player *p, item *it, bool t);
  static void jackhammer	(player *p, item *it, bool t);
  static void set_trap		(player *p, item *it, bool t);
  static void geiger		(player *p, item *it, bool t);
  static void teleport		(player *p, item *it, bool t);
  static void can_goo		(player *p, item *it, bool t);
  static void pipebomb		(player *p, item *it, bool t);
  static void pipebomb_act	(player *p, item *it, bool t);
  static void grenade		(player *p, item *it, bool t);
  static void grenade_act	(player *p, item *it, bool t);
  static void flashbang	(player *p, item *it, bool t);
  static void flashbang_act	(player *p, item *it, bool t);
  static void c4    		(player *p, item *it, bool t);
  static void c4armed  	(player *p, item *it, bool t);
  static void EMPbomb		(player *p, item *it, bool t);
  static void EMPbomb_act	(player *p, item *it, bool t);
  static void gasbomb		(player *p, item *it, bool t);
  static void gasbomb_act	(player *p, item *it, bool t);
  static void smokebomb	(player *p, item *it, bool t);
  static void smokebomb_act	(player *p, item *it, bool t);
  static void acidbomb		(player *p, item *it, bool t);
  static void acidbomb_act	(player *p, item *it, bool t);
  static void molotov		(player *p, item *it, bool t);
  static void molotov_lit	(player *p, item *it, bool t);
  static void dynamite		(player *p, item *it, bool t);
  static void dynamite_act	(player *p, item *it, bool t);
  static void mininuke		(player *p, item *it, bool t);
  static void mininuke_act	(player *p, item *it, bool t);
  static void pheromone	(player *p, item *it, bool t);
  static void portal		(player *p, item *it, bool t);
  static void manhack(pc& p, item& it);
  static void manhack(npc& p, item& it);
  static std::optional<std::any> can_use_manhack(const npc& p);
  static void turret		(pc& p, item& it);
  static void UPS_off		(player *p, item *it, bool t);
  static void UPS_on		(player *p, item *it, bool t);
  static std::optional<std::any> can_use_tazer(const npc& p);
  static void tazer(pc& p, item& it);
  static void tazer(npc& p, item& it);
  static void mp3		(player *p, item *it, bool t);
  static void mp3_on		(player *p, item *it, bool t);
  static void vortex		(pc& p, item& it);
  static void dog_whistle	(player *p, item *it, bool t);
  static void vacutainer	(pc& p, item& it);
// MACGUFFINS
  static void mcg_note		(pc& p, item& it);
// ARTIFACTS
// This function is used when an artifact is activated
// It examines the item's artifact-specific properties
// See artifact.h for a list
  static void artifact		(player *p, item *it, bool t);
  // historical declarations without definitions
/*  static void heal		(player *p, item *it, bool t);
  static void twist_space	(player *p, item *it, bool t);
  static void mass_vampire	(player *p, item *it, bool t);
  static void growth		(player *p, item *it, bool t);
  static void water		(player *p, item *it, bool t);
  static void lava		(player *p, item *it, bool t); */
  
};

#endif
