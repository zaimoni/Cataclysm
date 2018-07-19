#ifndef _IUSE_H_
#define _IUSE_H_

class game;
class item;
class player;

class iuse
{
 public:
  static void none		(game *g, player *p, item *it, bool t) { };
// FOOD AND DRUGS (ADMINISTRATION)
  static void sewage		(game *g, player *p, item *it, bool t);
  static void royal_jelly	(game *g, player *p, item *it, bool t);
  static void bandage		(game *g, player *p, item *it, bool t);
  static void firstaid		(game *g, player *p, item *it, bool t);
  static void vitamins		(game *g, player *p, item *it, bool t);
  static void caff		(game *g, player *p, item *it, bool t);
  static void alcohol		(game *g, player *p, item *it, bool t);
  static void pkill_1		(game *g, player *p, item *it, bool t);
  static void pkill_2		(game *g, player *p, item *it, bool t);
  static void pkill_3		(game *g, player *p, item *it, bool t);
  static void pkill_4		(game *g, player *p, item *it, bool t);
  static void pkill_l		(game *g, player *p, item *it, bool t);
  static void xanax		(game *g, player *p, item *it, bool t);
  static void cig		(game *g, player *p, item *it, bool t);
  static void weed		(game *g, player *p, item *it, bool t);
  static void coke		(game *g, player *p, item *it, bool t);
  static void meth		(game *g, player *p, item *it, bool t);
  static void poison		(game *g, player *p, item *it, bool t);
  static void hallu		(game *g, player *p, item *it, bool t);
  static void thorazine	(game *g, player *p, item *it, bool t);
  static void prozac		(game *g, player *p, item *it, bool t);
  static void sleep		(game *g, player *p, item *it, bool t);
  static void iodine		(game *g, player *p, item *it, bool t);
  static void flumed		(game *g, player *p, item *it, bool t);
  static void flusleep		(game *g, player *p, item *it, bool t);
  static void inhaler		(game *g, player *p, item *it, bool t);
  static void blech		(game *g, player *p, item *it, bool t);
  static void mutagen		(game *g, player *p, item *it, bool t);
  static void mutagen_3	(game *g, player *p, item *it, bool t);
  static void purifier		(game *g, player *p, item *it, bool t);
  static void marloss		(game *g, player *p, item *it, bool t);
  static void dogfood		(game *g, player *p, item *it, bool t);

// TOOLS
  static void lighter		(game *g, player *p, item *it, bool t);
  static void sew		(game *g, player *p, item *it, bool t);
  static void scissors		(game *g, player *p, item *it, bool t);
  static void extinguisher	(game *g, player *p, item *it, bool t);
  static void hammer		(game *g, player *p, item *it, bool t);
  static void light_off	(game *g, player *p, item *it, bool t);
  static void light_on		(game *g, player *p, item *it, bool t);
  static void water_purifier	(game *g, player *p, item *it, bool t);
  static void two_way_radio	(game *g, player *p, item *it, bool t);
  static void radio_off	(game *g, player *p, item *it, bool t);
  static void radio_on		(game *g, player *p, item *it, bool t);
  static void crowbar		(game *g, player *p, item *it, bool t);
  static void makemound	(game *g, player *p, item *it, bool t);
  static void dig		(game *g, player *p, item *it, bool t);
  static void chainsaw_off	(game *g, player *p, item *it, bool t);
  static void chainsaw_on	(game *g, player *p, item *it, bool t);
  static void jackhammer	(game *g, player *p, item *it, bool t);
  static void set_trap		(game *g, player *p, item *it, bool t);
  static void geiger		(game *g, player *p, item *it, bool t);
  static void teleport		(game *g, player *p, item *it, bool t);
  static void can_goo		(game *g, player *p, item *it, bool t);
  static void pipebomb		(game *g, player *p, item *it, bool t);
  static void pipebomb_act	(game *g, player *p, item *it, bool t);
  static void grenade		(game *g, player *p, item *it, bool t);
  static void grenade_act	(game *g, player *p, item *it, bool t);
  static void flashbang	(game *g, player *p, item *it, bool t);
  static void flashbang_act	(game *g, player *p, item *it, bool t);
  static void c4    		(game *g, player *p, item *it, bool t);
  static void c4armed  	(game *g, player *p, item *it, bool t);
  static void EMPbomb		(game *g, player *p, item *it, bool t);
  static void EMPbomb_act	(game *g, player *p, item *it, bool t);
  static void gasbomb		(game *g, player *p, item *it, bool t);
  static void gasbomb_act	(game *g, player *p, item *it, bool t);
  static void smokebomb	(game *g, player *p, item *it, bool t);
  static void smokebomb_act	(game *g, player *p, item *it, bool t);
  static void acidbomb		(game *g, player *p, item *it, bool t);
  static void acidbomb_act	(game *g, player *p, item *it, bool t);
  static void molotov		(game *g, player *p, item *it, bool t);
  static void molotov_lit	(game *g, player *p, item *it, bool t);
  static void dynamite		(game *g, player *p, item *it, bool t);
  static void dynamite_act	(game *g, player *p, item *it, bool t);
  static void mininuke		(game *g, player *p, item *it, bool t);
  static void mininuke_act	(game *g, player *p, item *it, bool t);
  static void pheromone	(game *g, player *p, item *it, bool t);
  static void portal		(game *g, player *p, item *it, bool t);
  static void manhack		(game *g, player *p, item *it, bool t);
  static void turret		(game *g, player *p, item *it, bool t);
  static void UPS_off		(game *g, player *p, item *it, bool t);
  static void UPS_on		(game *g, player *p, item *it, bool t);
  static void tazer		(game *g, player *p, item *it, bool t);
  static void mp3		(game *g, player *p, item *it, bool t);
  static void mp3_on		(game *g, player *p, item *it, bool t);
  static void vortex		(game *g, player *p, item *it, bool t);
  static void dog_whistle	(game *g, player *p, item *it, bool t);
  static void vacutainer	(game *g, player *p, item *it, bool t);
// MACGUFFINS
  static void mcg_note		(game *g, player *p, item *it, bool t);
// ARTIFACTS
// This function is used when an artifact is activated
// It examines the item's artifact-specific properties
// See artifact.h for a list
  static void artifact		(game *g, player *p, item *it, bool t);
  static void heal		(game *g, player *p, item *it, bool t);
  static void twist_space	(game *g, player *p, item *it, bool t);
  static void mass_vampire	(game *g, player *p, item *it, bool t);
  static void growth		(game *g, player *p, item *it, bool t);
  static void water		(game *g, player *p, item *it, bool t);
  static void lava		(game *g, player *p, item *it, bool t);
  
};

#endif
