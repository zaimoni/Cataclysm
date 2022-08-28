#ifndef SUBMAP_H
#define SUBMAP_H 1

#include "ui.h"
#include "vehicle.h"
#include "computer.h"
#include "mapdata.h"
#include "mtype.h"
#include "zero.h"
#include <iosfwd>
#include <memory>

struct defense_game;
class map;
class mapbuffer;

struct spawn_point {
    point pos;
    int count;
    mon_id type;
    int faction_id;
    int mission_id;
    bool friendly;
    std::string _name;
    spawn_point(mon_id T, int C, int X, int Y, int FAC, int MIS, bool F, const std::string& N)
    : pos(X, Y), count(C), type(T), faction_id(FAC), mission_id(MIS), friendly(F), _name(N) {}
    spawn_point() noexcept : pos(-1,-1), count(0), type(mon_null),faction_id(-1),mission_id(-1),friendly(false) {}
    spawn_point(const spawn_point& src) = default;
    spawn_point(spawn_point&& src) = default;
    ~spawn_point() = default;
    spawn_point& operator=(const spawn_point& src) = default;
    spawn_point& operator=(spawn_point && src) = default;
};

struct submap {
    int turn_last_touched;
    std::vector<spawn_point> spawns;

private:
    std::vector<item> itm[SEEX][SEEY]; // Items on each square
    std::vector<std::shared_ptr<vehicle> > vehicles;
    computer comp;
    field   fld[SEEX][SEEY]; // Field on each square
    int     rad[SEEX][SEEY]; // Irradiation of each square
    ter_id  ter[SEEX][SEEY]; // Terrain on each square
    trap_id trp[SEEX][SEEY]; // Trap on each square
    int active_item_count;
    int field_count;
    tripoint GPS;   // cache field -- GPS_loc first coordinate, where we are

public:
    using vehicles_t = decltype(vehicles);
    using proxy_vehicles_t = std::vector<std::weak_ptr<vehicle> >;

    friend std::ostream& operator<<(std::ostream& os, const submap& src);

    submap();	// mapgen.cpp: sole caller there
    submap(const submap& src) = default;
    submap(submap&& src) = default;
    ~submap() = default;
    submap& operator=(const submap& src) = default;	// plausibly should make ACID but we don't actually copy submaps frequently
    submap& operator=(submap&& src) = default;

    submap(std::istream& is, tripoint& gps);
    friend std::ostream& operator<<(std::ostream& os, const submap& src);

    void set(const tripoint src, const Badge<mapbuffer>& auth);
    GPS_loc toGPS(const point& origin, const Badge<map>& auth) const { return GPS_loc(GPS, origin); }

    static constexpr bool in_bounds(int x, int y) { return 0 <= x && x < SEE && 0 <= y && y < SEE; }
    static constexpr bool in_bounds(const point& p) { return in_bounds(p.x, p.y); }
    static std::optional<item> for_drop(ter_id dest, const itype* type, int birthday);

    int& radiation(const point& p) { return rad[p.x][p.y]; }
    int radiation(const point& p) const { return rad[p.x][p.y]; }
    ter_id& terrain(const point& p) { return ter[p.x][p.y]; }
    ter_id terrain(const point& p) const { return ter[p.x][p.y]; }
    trap_id& trap_at(const point& p) { return trp[p.x][p.y]; }
    trap_id trap_at(const point& p) const { return trp[p.x][p.y]; }

    field& field_at(const point& p) { return fld[p.x][p.y]; }
    const field& field_at(const point& p) const { return fld[p.x][p.y]; }
    void remove_field(const point& p);
    field* add(const point& p, field&& src);

    std::vector<item>& items_at(const point& p) { return itm[p.x][p.y]; }
    const std::vector<item>& items_at(const point& p) const { return itm[p.x][p.y]; }

    // including vehicles is more complicated
    // 2020-12-18: vehicles only enable the bashable flag, others are as-if terrain only
   // bool has_flag_ter_only(t_flag flag, const point& pt) const { return ter_t::list[ter[pt.x][pt.y]].flags & mfb(flag); };
    template<t_flag flag> bool has_flag_ter_only(const point& pt) const { return ter_t::list[ter[pt.x][pt.y]].flags & mfb(flag); };
    int move_cost_ter_only(const point& pt) const { return ter_t::list[ter[pt.x][pt.y]].movecost; };

    void process_active_items(); // map.cpp; only caller there
    bool process_fields(); // field.cpp

    void add_spawn(mon_id type, int count, const point& pt, bool friendly, int faction_id, int mission_id, std::string name); // mapgen.cpp
    void add_spawn(const monster& mon); // mapgen.cpp

    void add(item&& new_item, const point& dest);

    computer* add_computer(const point& pt, std::string&& name, int security);
    computer* computer_at(const point& pt, const Badge<map>& auth);

    vehicle* add_vehicle(vhtype_id type, point pos, int deg);
    void add(std::shared_ptr<vehicle> veh, const Badge<map>& auth);
    void destroy(vehicle& veh);
    std::optional<std::pair<vehicle*, int>> veh_at(const GPS_loc& loc);
    std::optional<std::pair<const vehicle*, int>> veh_at(const GPS_loc& loc) const;

    void veh_gain_moves(proxy_vehicles_t& acc, const Badge<map>& auth);

    void post_init(const Badge<defense_game>& auth);
    // mapgen.cpp support
    void rotate_vehicles(int turns, const Badge<map>& auth);
    void mapgen_swap(submap& dest, const Badge<map>& auth);
    static void mapgen_move_cycle(submap* const* cycle, ptrdiff_t len, const Badge<map>& auth);
    void mapgen_xform(point(*op)(const point&), const Badge<map>& auth);
};

#endif
