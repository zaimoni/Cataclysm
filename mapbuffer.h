#include <map>

#include "enums.h"

struct submap;

class mapbuffer
{
 public:
  mapbuffer() = default;
  mapbuffer(const mapbuffer& src) = delete;
  mapbuffer(mapbuffer&& src) = default;
  ~mapbuffer();	// raw pointers involved so cannot default-destruct or default-copy
  mapbuffer& operator=(const mapbuffer& src) = delete;
  mapbuffer& operator=(mapbuffer&& src) = default;

  void load();
  void save();

  bool add_submap(int x, int y, int z, submap *sm);
  submap* lookup_submap(int x, int y, int z);

  size_t size() const { return submaps.size(); };

 private:
  std::map<tripoint, submap*> submaps;	// candidate for absolute coordinates: tripoint (submap index),point (legal values 0..SEE-1 for both x,y)
};
  
extern mapbuffer MAPBUFFER;
