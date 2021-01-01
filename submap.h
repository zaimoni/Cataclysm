#ifndef SUBMAP_H
#define SUBMAP_H 1

#include "ui.h"
#include "vehicle.h"
#include "computer.h"
#include "mapdata.h"
#include "mtype.h"

struct spawn_point {
    point pos;
    int count;
    mon_id type;
    int faction_id;
    int mission_id;
    bool friendly;
    std::string name;
    spawn_point(mon_id T = mon_null, int C = 0, int X = -1, int Y = -1,
        int FAC = -1, int MIS = -1, bool F = false,
        std::string N = "NONE") :
        pos(X, Y), count(C), type(T), faction_id(FAC),
        mission_id(MIS), friendly(F), name(N) {}
};

struct submap {
    ter_id			ter[SEEX][SEEY]; // Terrain on each square
    std::vector<item>	itm[SEEX][SEEY]; // Items on each square
    trap_id		trp[SEEX][SEEY]; // Trap on each square
    field			fld[SEEX][SEEY]; // Field on each square
    int			rad[SEEX][SEEY]; // Irradiation of each square
    int active_item_count;
    int field_count;
    int turn_last_touched;
    std::vector<spawn_point> spawns;
    std::vector<vehicle> vehicles;
    computer comp;

    submap();	// mapgen.cpp: sole caller there
    submap(const submap& src) = default;
    submap(submap&& src) = default;
    ~submap() = default;
    submap& operator=(const submap& src) = default;	// plausibly should make ACID but we don't actually copy submaps frequently
    submap& operator=(submap&& src) = default;

    submap(std::istream& is);
    friend std::ostream& operator<<(std::ostream& os, const submap& src);

    static constexpr bool in_bounds(int x, int y) { return 0 <= x && x < SEE && 0 <= y && y < SEE; }
    static constexpr bool in_bounds(const point& p) { return in_bounds(p.x, p.y); }

    // including vehicles is more complicated
    // 2020-12-18: vehicles only enable the bashable flag, others are as-if terrain only
   // bool has_flag_ter_only(t_flag flag, const point& pt) const { return ter_t::list[ter[pt.x][pt.y]].flags & mfb(flag); };
    template<t_flag flag> bool has_flag_ter_only(const point& pt) const { return ter_t::list[ter[pt.x][pt.y]].flags & mfb(flag); };
    int move_cost_ter_only(const point& pt) const { return ter_t::list[ter[pt.x][pt.y]].movecost; };

    void process_active_items(); // map.cpp; only caller there

    void add_spawn(mon_id type, int count, const point& pt, bool friendly, int faction_id, int mission_id, std::string name); // mapgen.cpp
    void add_spawn(const monster& mon); // mapgen.cpp
};

#endif
