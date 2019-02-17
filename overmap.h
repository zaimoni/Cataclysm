#ifndef _OVERMAP_H_
#define _OVERMAP_H_
#include "enums.h"
#include "string.h"
#include "omdata.h"
#include "mongroup.h"
#include "output.h"
#include "npc.h"
#include <vector>

struct city {
 int x;	// legal range 0...OMAPX-1
 int y;	// legal range 0...OMAPY-1
 int s;
 city(int X = -1, int Y = -1, int S = -1) : x (X), y (Y), s (S) {}
};

struct om_note {
 int x;	// legal range 0...OMAPX-1
 int y;	// legal range 0...OMAPY-1
 int num;
 std::string text;
 om_note(int X = -1, int Y = -1, int N = -1, std::string T = "") :
         x (X), y (Y), num (N), text (T) {}
};

struct radio_tower {
 int x;	// legal range 0...OMAPX-1
 int y;	// legal range 0...OMAPY-1
 int strength;
 std::string message;
 radio_tower(int X = -1, int Y = -1, int S = -1, std::string M = "") :
             x (X), y (Y), strength (S), message (M) {}
};

class overmap
{
 public:
  overmap() : pos(999, 999, 999) {};
  overmap(game *g, int x, int y, int z) : pos(x,y,z) { open(g); };
  ~overmap() = default;
  void save(const std::string& name, int x, int y, int z);
  void save(const std::string& name) { save(name, pos.x, pos.y, pos.z); }
  void generate(game *g, overmap* north, overmap* east, overmap* south,
                overmap* west);
  void generate_sub(overmap* above);
  void make_tutorial();
  void first_house(int &x, int &y);

  void process_mongroups(); // Makes them die out, maybe more

/* Returns the closest point of terrain type [type, type + type_range)
 * Use type_range of 4, for instance, to match all gun stores (4 rotations).
 * dist is set to the distance between the two points.
 * You can give dist a value, which will be used as the maximum distance; a
 *  value of 0 will search the entire overmap.
 * Use must_be_seen=true if only terrain seen by the player should be searched.
 * If no such tile can be found, (-1, -1) is returned.
 */
  point find_closest(point origin, oter_id type, int type_range,
                     int &dist, bool must_be_seen);
  std::vector<point> find_all(point origin, oter_id type, int type_range,
                            int &dist, bool must_be_seen);
  std::vector<point> find_terrain(std::string term, int cursx, int cursy);
  int closest_city(point p) const;
  point random_house_in_city(int city_id);
  int dist_from_city(point p) const;
// Interactive point choosing; used as the map screen
  point choose_point(game *g);

  oter_id& ter(int x, int y);
  unsigned zones(int x, int y);
  std::vector<mongroup*> monsters_at(int x, int y);
  bool is_safe(int x, int y); // true if monsters_at is empty, or only woodland
  bool&   seen(int x, int y);

  bool has_note(int x, int y);
  std::string note(int x, int y);
  void add_note(int x, int y, std::string message);
  point find_note(point origin, const std::string& text);
  void delete_note(int x, int y);
  point display_notes();
  
  tripoint pos;
  std::vector<city> cities;
#if PROTOTYPE
  std::vector<settlement> towns;	// prototyping variable; #include "settlement.h" rather than overmap.cpp when taking live
#endif
  std::vector<mongroup> zg;
  std::vector<radio_tower> radios;
  std::vector<npc> npcs;

 private:
  oter_id t[OMAPX][OMAPY];
  bool s[OMAPX][OMAPY];
  std::vector<om_note> notes;
  std::vector<city> roads_out;

  void open(game *g);
  //Drawing
  void draw(WINDOW *w, game *g, int &cursx, int &cursy, 
                   int &origx, int &origy, char &ch, bool blink);
  // Overall terrain
  void place_river(point pa, point pb);
  void place_forest();
  // City Building
  void place_cities(std::vector<city> &cities, int min);
  void put_buildings(int x, int y, int dir, city town);
  void make_road(int cx, int cy, int cs, int dir, city town);
  void build_lab(int x, int y, int s);
  void build_anthill(int x, int y, int s);
  void build_tunnel(int x, int y, int s, int dir);
  void build_slimepit(int x, int y, int s);
  void build_mine(int x, int y, int s);
  void place_rifts();
  // Connection highways
  void place_hiways(std::vector<city> cities, oter_id base);
  void place_subways(std::vector<point> stations);
  void make_hiway(int x1, int y1, int x2, int y2, oter_id base);
  void building_on_hiway(int x, int y, int dir);
  // Polishing
  bool is_road(oter_id base, int x, int y); // Dependant on road type
  bool is_road(int x, int y);
  void polish(oter_id min = ot_null, oter_id max = ot_tutorial);
  void good_road(oter_id base, int x, int y);
  void good_river(int x, int y);
  // Monsters, radios, etc.
  void place_specials();
  void place_special(overmap_special special, point p);
  void place_mongroups();
  void place_radios();
  // File I/O
};
#endif
