#ifndef _CRAFTING_H_
#define _CRAFTING_H_

#include "itype.h"
#include "skill_enum.h"

enum craft_cat {
CC_NULL = 0,
CC_WEAPON,
CC_FOOD,
CC_ELECTRONIC,
CC_ARMOR,
CC_MISC,
NUM_CC
};

struct component
{
 itype_id type;
 int count;

 component(itype_id TYPE = itm_null, int COUNT = 0) : type (TYPE), count (COUNT) {}
};

struct recipe
{
 static std::vector<recipe*> recipes;	// The list of valid recipes; const recipe* won't work due to defense subgame

 int id;
 itype_id result;
 craft_cat category;
 skill sk_primary;
 skill sk_secondary;
 int difficulty;
 int time;

 std::vector<component> tools[5];
 std::vector<component> components[5];

 recipe() { id = 0; result = itm_null; category = CC_NULL; sk_primary = sk_null;
            sk_secondary = sk_null; difficulty = 0; time = 0; }
 recipe(int pid, itype_id pres, craft_cat cat, skill p1, skill p2, int pdiff,
        int ptime) :
  id (pid), result (pres), category (cat), sk_primary (p1), sk_secondary (p2),
  difficulty (pdiff), time (ptime) {}

 static void init();
};

void consume_items(map& m, player& u, const std::vector<component>& components);	// \todo enable NPC construction
void consume_tools(map& m, player& u, const std::vector<component>& tools);

#endif