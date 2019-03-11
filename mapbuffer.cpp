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
  submap *sm = it->second;
  fout << sm->turn_last_touched << std::endl;
// Dump the terrain.
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++)
    fout << int(sm->ter[i][j]) << " ";
   fout << std::endl;
  }
 // Dump the radiation
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) 
    fout << sm->rad[i][j] << " ";
  }
  fout << std::endl;
 
 // Items section; designate it with an I.  Then check itm[][] for each square
 //   in the grid and print the coords and the item's details.
 // Designate it with a C if it's contained in the prior item.
 // Also, this wastes space since we print the coords for each item, when we
 //   could be printing a list of items for each coord (except the empty ones)
  item tmp;
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    for (int k = 0; k < sm->itm[i][j].size(); k++) {
     tmp = sm->itm[i][j][k];
     fout << "I " << i << " " << j << std::endl;
     fout << tmp.save_info() << std::endl;
     for (int l = 0; l < tmp.contents.size(); l++)
      fout << "C " << std::endl << tmp.contents[l].save_info() << std::endl;
    }
   }
  }
 // Output the traps
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    if (sm->trp[i][j] != tr_null)
     fout << "T " << i << " " << j << " " << sm->trp[i][j] <<
     std::endl;
   }
  }
 
 // Output the fields
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    const field& tmpf = sm->fld[i][j];
    if (tmpf.type != fd_null) fout << "F " << i << " " << j << " " << tmpf << std::endl;
   }
  }
 // Output the spawn points
  for(const auto& s : sm->spawns) fout << "S " << s << std::endl;

 // Output the vehicles
  for (int i = 0; i < sm->vehicles.size(); i++) {
   fout << "V ";
   sm->vehicles[i].save (fout);
  }
 // Output the computer
  if (sm->comp.name != "") fout << "c " << sm->comp << std::endl;
  fout << "----" << std::endl;
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
  submap* sm = new submap;
  fin >> locx >> locy >> locz >> turn;
  sm->turn_last_touched = turn;
  int turndif = int(messages.turn);
  if (turndif < 0) turndif = 0;
// Load terrain
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    int tmpter;
    fin >> tmpter;
    sm->ter[i][j] = ter_id(tmpter);
    sm->itm[i][j].clear();
    sm->trp[i][j] = tr_null;
    sm->fld[i][j] = field();
   }
  }
// Load irradiation
  for (int j = 0; j < SEEY; j++) {
   for (int i = 0; i < SEEX; i++) {
    int radtmp;
    fin >> radtmp;
    radtmp -= int(turndif / 100);	// Radiation slowly decays
    if (radtmp < 0)
     radtmp = 0;
    sm->rad[i][j] = radtmp;
   }
  }
// Load items and traps and fields and spawn points and vehicles
  std::string string_identifier;
  do {
   fin >> string_identifier; // "----" indicates end of this submap
   t = 0;
   if (string_identifier == "I") {
    fin >> itx >> ity;
    getline(fin, databuff); // Clear out the endline
    getline(fin, databuff);
    it_tmp.load_info(databuff);
    sm->itm[itx][ity].push_back(it_tmp);
    if (it_tmp.active)
     sm->active_item_count++;
   } else if (string_identifier == "C") {
    getline(fin, databuff); // Clear out the endline
    getline(fin, databuff);
    int index = sm->itm[itx][ity].size() - 1;
    it_tmp.load_info(databuff);
    sm->itm[itx][ity][index].put_in(it_tmp);
    if (it_tmp.active)
     sm->active_item_count++;
   } else if (string_identifier == "T") {
    fin >> itx >> ity >> t;
    sm->trp[itx][ity] = trap_id(t);
   } else if (string_identifier == "F") {
    fin >> itx >> ity;
	fin >> sm->fld[itx][ity];
    sm->field_count++;
   } else if (string_identifier == "S") sm->spawns.push_back(spawn_point(fin));
   else if (string_identifier == "V") {
    vehicle veh(master_game);
    veh.load (fin);
    //veh.smx = gridx;
    //veh.smy = gridy;
    sm->vehicles.push_back(veh);
   } else if (string_identifier == "c") {
	fin >> sm->comp;
	getline(fin, databuff); // Clear out the endline
   }
  } while (string_identifier != "----" && !fin.eof());

  submap_list.push_back(sm);
  submaps[ tripoint(locx, locy, locz) ] = sm;
 }
 fin.close();
}
