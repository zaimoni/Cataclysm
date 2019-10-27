#include "mapbuffer.h"
#include "mapdata.h"
#include "output.h"
#include "recent_msg.h"
#include "saveload.h"
#include "ios_file.h"
#include <fstream>

mapbuffer MAPBUFFER;

mapbuffer::~mapbuffer()
{
 for (auto x : submaps) delete x.second;
}

bool mapbuffer::add_submap(int x, int y, int z, submap *sm)
{
 tripoint p(x, y, z);
 if (submaps.count(p) != 0) return false;

 sm->turn_last_touched = int(messages.turn);
 submaps[p] = sm;
 return true;
}

submap* mapbuffer::lookup_submap(int x, int y, int z)
{
 tripoint p(x, y, z);

 return (0 < submaps.count(p)) ? submaps[p] : NULL;
}

#define MAP_FILE "save/maps.txt"

void mapbuffer::save()
{

 DECLARE_AND_ACID_OPEN(std::ofstream, fout, MAP_FILE, return;)

 fout << submaps.size() << std::endl;
 int percent = 0;

 for(const auto& it : submaps) {
  percent++;
  if (percent % 100 == 0)
   popup_nowait("Please wait as the map saves [%s%d/%d]",
                (percent < 100 ?  percent < 10 ? "  " : " " : ""), percent,
                submaps.size());
  fout << it.first << std::endl;
  fout << *it.second;
 }
 // Close the file; that's all we need.
 OFSTREAM_ACID_CLOSE(fout, MAP_FILE)

}

void mapbuffer::load()
{
 DECLARE_AND_OPEN_SILENT(std::ifstream, fin, MAP_FILE, return;)

 int num_submaps;
 fin >> num_submaps;

 while (!fin.eof()) {
  int percent = submaps.size();
  if (percent % 100 == 0)
   popup_nowait("Please wait as the map loads [%s%d/%d]",
                (percent < 100 ?  percent < 10 ? "  " : " " : ""), percent,
                num_submaps);
  tripoint where;
  fin >> where;
  submaps[ where ] = new submap(fin);
 }
 fin.close();
}
#undef MAP_FILE
