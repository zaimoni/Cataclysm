#include "mapbuffer.h"
#include "game.h"
#include "output.h"
#include "recent_msg.h"
#include "saveload.h"
#include <fstream>

mapbuffer MAPBUFFER;

mapbuffer::~mapbuffer()
{
 for(auto it : submap_list) delete it;
}

bool mapbuffer::add_submap(int x, int y, int z, submap *sm)
{
 tripoint p(x, y, z);
 if (submaps.count(p) != 0) return false;

 if (master_game) sm->turn_last_touched = int(messages.turn);
 submap_list.push_back(sm);
 submaps[p] = sm;
 return true;
}

submap* mapbuffer::lookup_submap(int x, int y, int z)
{
 tripoint p(x, y, z);

 return (0 < submaps.count(p)) ? submaps[p] : NULL;
}

void mapbuffer::save()
{
 std::map<tripoint, submap*>::iterator it;
 std::ofstream fout;
 fout.open("save/maps.txt");

 fout << submap_list.size() << std::endl;
 int percent = 0;

 for (it = submaps.begin(); it != submaps.end(); it++) {
  percent++;
  if (percent % 100 == 0)
   popup_nowait("Please wait as the map saves [%s%d/%d]",
                (percent < 100 ?  percent < 10 ? "  " : " " : ""), percent,
                submap_list.size());
  fout << it->first.x << " " << it->first.y << " " << it->first.z << std::endl;
  fout << *it->second;
 }
 // Close the file; that's all we need.
 fout.close();
}

void mapbuffer::load()
{
 if (!master_game) {
  debugmsg("Can't load mapbuffer without a master_game");
  return;
 }
 std::map<tripoint, submap*>::iterator it;
 std::ifstream fin;
 fin.open("save/maps.txt");
 if (!fin.is_open())
  return;

 char ch = 0;
 int itx, ity, t, d, a, num_submaps;
 item it_tmp;
 std::string databuff;
 fin >> num_submaps;

 while (!fin.eof()) {
  int percent = submap_list.size();
  if (percent % 100 == 0)
   popup_nowait("Please wait as the map loads [%s%d/%d]",
                (percent < 100 ?  percent < 10 ? "  " : " " : ""), percent,
                num_submaps);
  int locx, locy, locz, turn;
  fin >> locx >> locy >> locz;
  submap * sm = new submap(fin, master_game);
  submap_list.push_back(sm);
  submaps[ tripoint(locx, locy, locz) ] = sm;
 }
 fin.close();
}
