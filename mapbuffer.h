#include "map.h"
#include "line.h"
#include <list>

class game;

class mapbuffer
{
 public:
  mapbuffer(game *g = NULL) : master_game(g) {};
  ~mapbuffer();

  void set_game(game *g) { master_game = g; }

  void load();
  void save();

  bool add_submap(int x, int y, int z, submap *sm);
  submap* lookup_submap(int x, int y, int z);

  int size() const { return submap_list.size(); };

 private:
  std::map<tripoint, submap*> submaps;	// candidate for absolute coordinates: tripoint (submap index),point (legal values 0..SEE-1 for both x,y)
  std::list<submap*> submap_list;
  game *master_game;
};
  
extern mapbuffer MAPBUFFER;
