#ifndef _SETTLEMENT_H_
#define _SETTLEMENT_H_

#include "omdata.h"
#include "faction.h"

// looks like a very early prototype
// \todo check C:DDA for current status of this
struct settlement {
 settlement();
 settlement(int mapx, int mapy);
// void pick_faction(game *g, int omx, int omy);
 void set_population();
 void populate(game *g) { };

 int num(oter_id ter);
 void add_building(oter_id ter);

 faction fact;
 int posx;
 int posy;
 int pop;
 int size;
 //int buildings[ot_wall - ot_set_house + 1];
};

#endif
