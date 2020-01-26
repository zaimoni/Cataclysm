#ifndef _OVERMAP_H_
#define _OVERMAP_H_
#include "enums.h"
#include "omdata.h"
#include "output.h"
#include "npc.h"
#include <string.h>
#include <vector>
#include <iosfwd>

enum {
	OMAP = 180,
	OMAPX = OMAP,
	OMAPY = OMAP
};

// currently have to use proofreading to distinguish between these
typedef std::pair<tripoint, point> GPS_loc;
typedef std::pair<tripoint, point> OM_loc;

struct city {
 int x;	// legal range 0...OMAPX-1
 int y;	// legal range 0...OMAPY-1
 int s;
 city(int X = -1, int Y = -1, int S = -1) : x (X), y (Y), s (S) {}

 friend bool fromJSON(const cataclysm::JSON& _in, city& dest);
 friend cataclysm::JSON toJSON(const city& src);
};

struct om_note {
 int x;	// legal range 0...OMAPX-1
 int y;	// legal range 0...OMAPY-1
 int num;
 std::string text;
 om_note(int X = -1, int Y = -1, int N = -1, std::string T = "") :
         x (X), y (Y), num (N), text (T) {}

 om_note(std::istream& is);
 friend std::ostream& operator<<(std::ostream& os, const om_note& src);
};

struct radio_tower {
 int x;	// legal range 0...OMAPX-1
 int y;	// legal range 0...OMAPY-1
 int strength;
 std::string message;
 radio_tower(int X = -1, int Y = -1, int S = -1, std::string M = "") :
             x (X), y (Y), strength (S), message (M) {}

 friend bool fromJSON(const cataclysm::JSON& _in, radio_tower& dest);
 friend cataclysm::JSON toJSON(const radio_tower& src);
};

class overmap
{
 public:
  overmap() : pos(999, 999, 999) {};
  overmap(game *g, int x, int y, int z) : pos(x,y,z) { open(g); };
  ~overmap() = default;
  void save(const std::string& name, int x, int y, int z) const;
  void save(const std::string& name) const { save(name, pos.x, pos.y, pos.z); }
  static void saveall();
  static std::string terrain_filename(const tripoint& pos);
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
  point find_closest(point origin, oter_id type, int type_range, int dist=0, bool must_be_seen=false) const;
#if DEAD_FUNC
  std::vector<point> find_all(point origin, oter_id type, int type_range, int &dist, bool must_be_seen);
#endif
  std::vector<point> find_terrain(const std::string& term) const;
  const city* closest_city(point p) const;
  point random_house_in_city(const city* c) const;
  int dist_from_city(point p) const;
// Interactive point choosing; used as the map screen
  point choose_point(game *g);

  oter_id& ter(int x, int y);
  oter_id& ter(const point& pt) { return ter(pt.x, pt.y); };
  static oter_id& ter(OM_loc OMpos);
  oter_id ter(int x, int y) const { return const_cast<overmap*>(this)->ter(x, y); };	// \todo specialize?
  oter_id ter(const point& pt) const { return const_cast<overmap*>(this)->ter(pt.x, pt.y); };
  static oter_id ter_c(OM_loc OMpos);

  // unsigned zones(const point& pt);	// no definition
  std::vector<mongroup*> monsters_at(int x, int y);
  std::vector<const mongroup*> monsters_at(int x, int y) const;
  static bool is_safe(const OM_loc& loc); // true if monsters_at is empty, or only woodland
  bool& seen(int x, int y);
  bool& seen(const point& pt) { return seen(pt.x, pt.y); };
  static bool& seen(OM_loc OMpos);
  bool seen(int x, int y) const { return const_cast<overmap*>(this)->seen(x, y); };	// \todo specialize?
  bool seen(const point& pt) const { return const_cast<overmap*>(this)->seen(pt.x, pt.y); };
  static bool seen_c(OM_loc OMpos);
  static void expose(OM_loc OMpos);

  bool has_note(const point& pt) const;
  static bool has_note(OM_loc OMpos, std::string& dest);
  void add_note(const point& pt, std::string message);
  void add_note(const point& pt, const char* const message) { add_note(pt, std::string(message)); };
  point find_note(point origin, const std::string& text) const;
  void delete_note(const point& pt);
  point display_notes() const;

  static GPS_loc toGPS(const point& screen_pos);
  static OM_loc toOvermap(const GPS_loc GPSpos);
  static OM_loc normalize(const OM_loc& OMpos);
  static void self_normalize(OM_loc& OMpos);

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

  void clear_seen() { memset(s,0,sizeof(s)); }
  void clear_terrain(oter_id src);

  void open(game *g);
  void generate(game* g, const overmap* north, const overmap* east, const overmap* south, const overmap* west);
  void generate_sub(const overmap* above);
  //Drawing
  void draw(WINDOW *w, game *g, point& curs, point& orig, char &ch, bool blink) const;
  // Overall terrain
  void place_river(point pa, point pb);
  void place_forest();
  // City Building
  void place_cities(std::vector<city> &cities, int min);
  void put_buildings(int x, int y, int dir, const city& town);
  void make_road(int cx, int cy, int cs, int dir, const city& town);
  void build_lab(const city& origin);
  void build_anthill(const city& origin);
  void build_tunnel(int x, int y, int s, int dir);
  void build_slimepit(const city& origin);
  void build_mine(city origin);
  void place_rifts();
  // Connection highways
  void place_hiways(const std::vector<city>& cities, oter_id base);
// void place_subways(std::vector<point> stations);	// dead function
  void make_hiway(int x1, int y1, int x2, int y2, oter_id base);
  void building_on_hiway(int x, int y, int dir);
  // Polishing
  bool is_road(oter_id base, int x, int y) const; // Dependant on road type
  bool is_road(int x, int y) const;
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
