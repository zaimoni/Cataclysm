#ifndef _MONATTACK_H_
#define _MONATTACK_H_

class game;
class monster;

class mattack
{
 public:
  static void none(monster& z) { };
  static void antqueen(monster& z);
  static void shriek	(game *g, monster *z);
  static void acid		(game *g, monster *z);
  static void shockstorm	(game *g, monster *z);
  static void boomer		(game *g, monster *z);
  static void resurrect     (game *g, monster *z);
  static void science		(game *g, monster *z);
  static void growplants	(game *g, monster *z);
  static void grow_vine	(game *g, monster *z);
  static void vine		(game *g, monster *z);
  static void spit_sap	(game *g, monster *z);
  static void triffid_heartbeat(game *g, monster *z);
  static void fungus		(game *g, monster *z);
  static void fungus_sprout(monster& z);
  static void leap		(game *g, monster *z);
  static void dermatik	(game *g, monster *z);
  static void plant		(game *g, monster *z);
  static void disappear(monster& z);
  static void formblob	(game *g, monster *z);
  static void dogthing	(game *g, monster *z);
  static void tentacle	(game *g, monster *z);
  static void vortex	(game *g, monster *z);
  static void gene_sting	(game *g, monster *z);
  static void stare		(game *g, monster *z);
  static void fear_paralyze	(game *g, monster *z);
  static void photograph	(game *g, monster *z);
  static void tazer(monster& z);
  static void smg(monster& z);
  static void flamethrower(monster& z);
  static void copbot		(game *g, monster *z);
  static void multi_robot	(game *g, monster *z); // Pick from tazer, smg, flame
  static void ratking(monster& z);
  static void generator(monster& z);
  static void upgrade(monster& z);
  static void breathe(monster& z);
};

#endif
