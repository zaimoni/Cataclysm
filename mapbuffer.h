#ifndef MAPBUFFER_H
#define MAPBUFFER_H

#include "enums.h"
#include <map>

struct submap;

class mapbuffer // \todo natural singleton, but likely needs pre-requisites loaded before it is loaded for that
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
  submap* lookup_submap(const tripoint& src);
  submap* lookup_submap(int x, int y, int z) { return lookup_submap(tripoint(x, y, z)); }

  std::size_t size() const { return submaps.size(); }

 private:
  std::map<tripoint, submap*> submaps;	// candidate for absolute coordinates: tripoint (submap index),point (legal values 0..SEE-1 for both x,y)
};

extern mapbuffer MAPBUFFER;

#endif
