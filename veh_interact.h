#ifndef _VEH_INTERACT_H_
#define _VEH_INTERACT_H_

#include "inventory.h"

class vehicle;
class game;

class veh_interact
{
public:
	point c;
	point dd;
    int sel_part;
    char sel_cmd;
private:
    int cpart;
    WINDOW *w_mode;
    WINDOW *w_msg;
    WINDOW *w_disp;
    WINDOW *w_parts;
    WINDOW *w_stats;
    WINDOW *w_list;

    vehicle* const veh;
    game* const _g;
    bool has_wrench;
    bool has_welder;
    bool has_hacksaw;
    inventory crafting_inv;

    int part_at (point d);
    void move_cursor (int dx, int dy);
    int cant_do (char mode);

    void do_install(int reason);
    void do_repair(int reason);
    void do_refill(int reason);
    void do_remove(int reason);

    void display_veh ();
    void display_stats ();
    void display_mode (char mode);
    void display_list (int pos);

    std::vector<int> can_mount;
    std::vector<bool> has_mats;
    std::vector<int> need_repair;
    std::vector<int> parts_here;
    int ptank;
    bool obstruct;
    bool has_fuel;
public:
    veh_interact (int cx, int cy, game *gm, vehicle *v);
	veh_interact() = delete;
	veh_interact(const veh_interact& src) = delete;
	veh_interact(veh_interact&& src) = delete;
	veh_interact& operator=(const veh_interact& src) = delete;
	veh_interact& operator=(veh_interact&& src) = delete;

    void exec ();
};

vehicle& complete_vehicle(game* g);

#endif
