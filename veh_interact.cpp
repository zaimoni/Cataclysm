#include "veh_interact.h"
#include "vehicle.h"
#include "keypress.h"
#include "game.h"
#include "output.h"
#include "crafting.h"
#include "options.h"
#include "recent_msg.h"
#include <string>


veh_interact::veh_interact (int cx, int cy, game *gm, vehicle *v)
: c(cx, cy), dd(0, 0), sel_cmd(' '), cpart(-1), veh(v), _g(gm)
{
}

void veh_interact::exec ()
{
    //        x1      x2
    // y1 ----+------+--
    //        |      |
    // y2 ----+------+
    //               |
    //               |
    constexpr int winw1 = 12;
// XXX probably should do something more auditable
#define winw2 35
    constexpr const int winh2 = 12;
    const int winw12 = winw1 + winw2 + 1;
    const int winw3 = SCREEN_WIDTH - winw1 - winw2 - 2;
    const int winh3 = VIEW - TABBED_HEADER_HEIGHT - winh2 - 2;
    const int winx1 = winw1;
    const int winx2 = winw1 + winw2 + 1;
    const int winy1 = TABBED_HEADER_HEIGHT;
    const int winy2 = TABBED_HEADER_HEIGHT + winh2 + 1;

    //               h   w    y     x
	WINDOW* const w_grid  = newwin(VIEW, SCREEN_WIDTH,  0,    0);
	w_mode  = newwin(1, SCREEN_WIDTH, 0,    0);
    w_msg   = newwin(TABBED_HEADER_HEIGHT - 1, SCREEN_WIDTH, 1,    0);
    w_disp  = newwin(winh2, winw1,  winy1 + 1, 0);
    w_parts = newwin(winh2, winw2,  winy1 + 1, winx1 + 1);
    w_stats = newwin(winh3, winw12, winy2 + 1, 0);
    w_list  = newwin(winh2 + winh3 + 1, winw3, winy1 + 1, winx2 + 1);

    for (int i = 0; i < VIEW; i++)
    {
        mvwputch(w_grid, i, winx2, c_ltgray, i == winy1 || i == winy2? LINE_XOXX : LINE_XOXO);
        if (i >= winy1 && i < winy2)
            mvwputch(w_grid, i, winx1, c_ltgray, LINE_XOXO);
    }
    for (int i = 0; i < SCREEN_WIDTH; i++)
    {
        mvwputch(w_grid, winy1, i, c_ltgray, i == winx1? LINE_OXXX : (i == winx2? LINE_OXXX : LINE_OXOX));
        if (i < winx2)
            mvwputch(w_grid, winy2, i, c_ltgray, i == winx1? LINE_XXOX : LINE_OXOX);
    }
    wrefresh(w_grid);

    crafting_inv = crafting_inventory(_g->m, _g->u);

    int charges = dynamic_cast<it_tool*>(item::types[itm_welder])->charges_per_use;
    has_wrench = crafting_inv.has_amount(itm_wrench, 1) ||
                 crafting_inv.has_amount(itm_toolset, 1);
    has_hacksaw = crafting_inv.has_amount(itm_hacksaw, 1) ||
                  crafting_inv.has_amount(itm_toolset, 1);
    has_welder = (crafting_inv.has_amount(itm_welder, 1) &&
                  crafting_inv.has_charges(itm_welder, charges)) ||
                 (crafting_inv.has_amount(itm_toolset, 1) &&
                 crafting_inv.has_charges(itm_toolset, charges/5));
        
    display_stats ();
    display_veh   ();
    move_cursor (0, 0);
    bool finish = false;
    while (!finish)
    {
        int ch = input(); // See keypress.h
        int dx, dy;
        get_direction (dx, dy, ch);
        if (ch == KEY_ESCAPE) finish = true;
        else if (dx != -2 && (dx || dy) &&
            c.x + dx >= -6 && c.x + dx < 6 &&
            c.y + dy >= -6 && c.y + dy < 6)
            move_cursor(dx, dy);
        else
        {
            int mval = cant_do(ch);
            display_mode (ch);
            switch (ch)
            {
            case 'i': do_install(mval); break;
            case 'r': do_repair(mval);  break;
            case 'f': do_refill(mval);  break; // not morale-gated; other commands are
            case 'o': do_remove(mval);  break;
            default:;
            }
            if (sel_cmd != ' ')
                finish = true;
            display_mode (' ');
        }
    }
    werase(w_grid);
    werase(w_mode);
    werase(w_msg);
    werase(w_disp);
    werase(w_parts);
    werase(w_stats);
    werase(w_list);
    delwin(w_grid);
    delwin(w_mode);
    delwin(w_msg);
    delwin(w_disp);
    delwin(w_parts);
    delwin(w_stats);
    delwin(w_list);
    erase();
}

// not really C error code convention -- 0 is all-clear, but positive values are hard errors rather than soft errors
int veh_interact::cant_do (char mode)
{
    switch (mode)
    {
    case 'i': // install mode
        return can_mount.size() > 0? (!has_wrench || !has_welder? 2 : 0) : 1;
    case 'r': // repair mode
        return need_repair.size() > 0 && cpart >= 0? (!has_welder? 2 : 0) : 1;
    case 'f': // refill mode
        return ptank >= 0? (!has_fuel? 2 : 0) : 1;
    case 'o': // remove mode
        return cpart < 0? 1 : 
               (parts_here.size() < 2 && !veh->can_unmount(cpart)? 2 : 
               (!has_wrench || !has_hacksaw? 3 :
               (_g->u.sklevel[sk_mechanics] < 2 ? 4 : 0)));
    default:
        return -1;
    }
}

void veh_interact::do_install(int reason)
{
    werase (w_msg);
    if (_g->u.morale_level() < MIN_MORALE_CRAFT) 
    { // See morale.h
        mvwprintz(w_msg, 0, 1, c_ltred, "Your morale is too low to construct...");
        wrefresh (w_msg);
        return;
    }
    switch (reason)
    {
    case 1:
        mvwprintz(w_msg, 0, 1, c_ltred, "Cannot install any part here.");
        wrefresh (w_msg);
        return;
    case 2:
        mvwprintz(w_msg, 0, 1, c_ltgray, "You need a wrench and a powered welder to install parts.");
        mvwprintz(w_msg, 0, 12, has_wrench? c_ltgreen : c_red, "wrench");
        mvwprintz(w_msg, 0, 25, has_welder? c_ltgreen : c_red, "powered welder");
        wrefresh (w_msg);
        return;
    default:;
    }
    mvwprintz(w_mode, 0, 1, c_ltgray, "Choose new part to install here:");
    wrefresh (w_mode);
    int pos = 0;
    int engines = 0;
    int dif_eng = 0;
    for (int p = 0; p < veh->parts.size(); p++)
        if (veh->part_flag (p, vpf_engine))
        {
            engines++;
            dif_eng = dif_eng / 2 + 12;
        }
    while (true) {
        sel_part = can_mount[pos];
        display_list (pos);
		const auto& p_info = vpart_info::list[sel_part];
		const itype_id itm = p_info.item;
		const bool has_comps = crafting_inv.has_amount(itm, 1);
		const bool has_skill = _g->u.sklevel[sk_mechanics] >= p_info.difficulty;
        werase (w_msg);
		const int slen = item::types[itm]->name.length();
        mvwprintz(w_msg, 0, 1, c_ltgray, "Needs %s and level %d skill in mechanics.", 
                  item::types[itm]->name.c_str(), p_info.difficulty);
        mvwprintz(w_msg, 0, 7, has_comps? c_ltgreen : c_red, item::types[itm]->name.c_str());
        mvwprintz(w_msg, 0, 18+slen, has_skill? c_ltgreen : c_red, "%d", p_info.difficulty);
		const bool eng = p_info.flags & mfb (vpf_engine);
        const bool has_skill2 = !eng || (_g->u.sklevel[sk_mechanics] >= dif_eng);
        if (engines && eng) // already has engine
        {
            mvwprintz(w_msg, 1, 1, c_ltgray, 
                      "You also need level %d skill in mechanics to install additional engine.", 
                      dif_eng);
            mvwprintz(w_msg, 1, 21, has_skill2? c_ltgreen : c_red, "%d", dif_eng);
        }
        wrefresh (w_msg);
        int ch = input(); // See keypress.h
        int dx, dy;
        get_direction (dx, dy, ch);
        if ((ch == '\n' || ch == ' ') && has_comps && has_skill && has_skill2)
        {
            sel_cmd = 'i';
            return;
        } else if (ch == KEY_ESCAPE) {
            werase (w_list);
            wrefresh (w_list);
            werase (w_msg);
            break;
        }
        if (dy == -1 || dy == 1)
        {
            pos += dy;
            if (pos < 0)
                pos = can_mount.size()-1;
            else
            if (pos >= can_mount.size())
                pos = 0;
        }
    }
}

void veh_interact::do_repair(int reason)
{
    werase (w_msg);
    if (_g->u.morale_level() < MIN_MORALE_CRAFT) 
    { // See morale.h
        mvwprintz(w_msg, 0, 1, c_ltred, "Your morale is too low to construct...");
        wrefresh (w_msg);
        return;
    }
    switch (reason)
    {
    case 1:
        mvwprintz(w_msg, 0, 1, c_ltred, "There are no damaged parts here.");
        wrefresh (w_msg);
        return;
    case 2:
        mvwprintz(w_msg, 0, 1, c_ltgray, "You need a powered welder to repair.");
        mvwprintz(w_msg, 0, 12, has_welder? c_ltgreen : c_red, "powered welder");
        wrefresh (w_msg);
        return;
    default:;
    }
    mvwprintz(w_mode, 0, 1, c_ltgray, "Choose a part here to repair:");
    wrefresh (w_mode);
    int pos = 0;
    while (true) {
        sel_part = parts_here[need_repair[pos]];
        werase (w_parts);
        veh->print_part_desc (w_parts, 0, winw2, cpart, need_repair[pos]);
        wrefresh (w_parts);
        werase (w_msg);
        bool has_comps = true;
        int dif = veh->part_info(sel_part).difficulty + (veh->parts[sel_part].hp <= 0? 0 : 2);
        bool has_skill = dif <= _g->u.sklevel[sk_mechanics];
        mvwprintz(w_msg, 0, 1, c_ltgray, "You need level %d skill in mechanics.", dif);
        mvwprintz(w_msg, 0, 16, has_skill? c_ltgreen : c_red, "%d", dif);
        if (veh->parts[sel_part].hp <= 0) {
            itype_id itm = veh->part_info(sel_part).item;
            has_comps = crafting_inv.has_amount(itm, 1);
            mvwprintz(w_msg, 1, 1, c_ltgray, "You also need a wrench and %s to replace broken one.", 
                    item::types[itm]->name.c_str());
            mvwprintz(w_msg, 1, 17, has_wrench? c_ltgreen : c_red, "wrench");
            mvwprintz(w_msg, 1, 28, has_comps? c_ltgreen : c_red, item::types[itm]->name.c_str());            
        }
        wrefresh (w_msg);
        int ch = input(); // See keypress.h
        int dx, dy;
        get_direction (dx, dy, ch);
        if ((ch == '\n' || ch == ' ') && has_comps && (veh->parts[sel_part].hp > 0 || has_wrench) && has_skill)
        {
            sel_cmd = 'r';
            return;
        } else if (ch == KEY_ESCAPE) {
            werase (w_parts);
            veh->print_part_desc (w_parts, 0, winw2, cpart, -1);
            wrefresh (w_parts);
            werase (w_msg);
            break;
        }
        if (dy == -1 || dy == 1)
        {
            pos += dy;
            if (pos < 0)
                pos = need_repair.size()-1;
            else
            if (pos >= need_repair.size())
                pos = 0;
        }
    }
}

void veh_interact::do_refill(int reason)
{
    werase (w_msg);
    switch (reason)
    {
    case 1:
        mvwprintz(w_msg, 0, 1, c_ltred, "There's no fuel tank here.");
        wrefresh (w_msg);
        return;
    case 2:
        {
        const auto f_name = vehicle::fuel_name(veh->part_info(ptank).fuel_type);
        mvwprintz(w_msg, 0, 1, c_ltgray, "You need %s.", f_name);
        mvwprintz(w_msg, 0, 10, c_red, f_name);
        }
        wrefresh (w_msg);
        return;
    default:;
    }
    sel_cmd = 'f';
    sel_part = ptank;
}

void veh_interact::do_remove(int reason)
{
    werase (w_msg);
    if (_g->u.morale_level() < MIN_MORALE_CRAFT) 
    { // See morale.h
        mvwprintz(w_msg, 0, 1, c_ltred, "Your morale is too low to construct...");
        wrefresh (w_msg);
        return;
    }
    switch (reason)
    {
    case 1:
        mvwprintz(w_msg, 0, 1, c_ltred, "No parts here.");
        wrefresh (w_msg);
        return;
    case 2:
        mvwprintz(w_msg, 0, 1, c_ltred, "You cannot remove mount point while something is attached to it.");
        wrefresh (w_msg);
        return;
    case 3:
        mvwprintz(w_msg, 0, 1, c_ltgray, "You need a wrench and a hack saw to remove parts.");
        mvwprintz(w_msg, 0, 12, has_wrench? c_ltgreen : c_red, "wrench");
        mvwprintz(w_msg, 0, 25, has_hacksaw? c_ltgreen : c_red, "hack saw");
        wrefresh (w_msg);
        return;
    case 4:
        mvwprintz(w_msg, 0, 1, c_ltred, "You need level 2 mechanics skill to remove parts.");
        wrefresh (w_msg);
        return;
    default:;
    }
    mvwprintz(w_mode, 0, 1, c_ltgray, "Choose a part here to remove:");
    wrefresh (w_mode);
    int first = parts_here.size() > 1? 1 : 0;
    int pos = first;
    while (true)
    {
        sel_part = parts_here[pos];
        werase (w_parts);
        veh->print_part_desc (w_parts, 0, winw2, cpart, pos);
        wrefresh (w_parts);
        int ch = input(); // See keypress.h
        int dx, dy;
        get_direction (dx, dy, ch);
        if (ch == '\n' || ch == ' ')
        {
            sel_cmd = 'o';
            return;
        }
        else
        if (ch == KEY_ESCAPE)
        {
            werase (w_parts);
            veh->print_part_desc (w_parts, 0, winw2, cpart, -1);
            wrefresh (w_parts);
            werase (w_msg);
            break;
        }
        if (dy == -1 || dy == 1)
        {
            pos += dy;
            if (pos < first)
                pos = parts_here.size()-1;
            else
            if (pos >= parts_here.size())
                pos = first;
        }
    }
}

int veh_interact::part_at (point d)
{
	point vd(-dd.x - d.y, d.x - dd.y);
	for (const auto p : veh->external_parts) {
        if (veh->parts[p].mount_d == vd) return p;
    }
    return -1;
}

void veh_interact::move_cursor (int dx, int dy)
{
    mvwputch (w_disp, c.y+6, c.x+6, cpart >= 0? veh->part_color (cpart) : c_black, special_symbol(cpart >= 0? veh->part_sym (cpart) : ' '));
    c.x += dx;
    c.y += dy;
    cpart = part_at (c);
	const point vd(-dd.x - c.y, c.x - dd.y);
	const point vpos(veh->global() + veh->coord_translate(vd));
    bool obstruct = _g->m.move_cost_ter_only (vpos.x, vpos.y) == 0;
    vehicle * const oveh = _g->m.veh_at (vpos.x, vpos.y);
    if (oveh && oveh != veh) obstruct = true;
    nc_color col = cpart >= 0? veh->part_color (cpart) : c_black;
    mvwputch (w_disp, c.y+6, c.x+6, obstruct? red_background(col) : hilite(col),
                      special_symbol(cpart >= 0? veh->part_sym (cpart) : ' '));
    wrefresh (w_disp);
    werase (w_parts);
    veh->print_part_desc (w_parts, 0, winw2, cpart, -1);
    wrefresh (w_parts);

    can_mount.clear();
    has_mats.clear();
    if (!obstruct)
        for (int i = 1; i < num_vparts; i++) {
            if (veh->can_mount (vd.x, vd.y, (vpart_id) i)) can_mount.push_back (i);
        }
    need_repair.clear();
    parts_here.clear();
    ptank = -1;
    if (cpart >= 0) {
        parts_here = veh->internal_parts(cpart);
        parts_here.insert (parts_here.begin(), cpart);
        for (int i = 0; i < parts_here.size(); i++) {
            int p = parts_here[i];
            const auto& part = veh->parts[p];
            const auto& p_info = veh->part_info(p);
            if (part.hp < p_info.durability) need_repair.push_back (i);
            if (p_info.has_flag<vpf_fuel_tank>() && part.amount < p_info._size) ptank = p;
        }
    }
    has_fuel = (ptank >= 0) ? veh->refill(_g->u, ptank, true) : false;
    werase (w_msg);
    wrefresh (w_msg);
    display_mode (' ');
}

void veh_interact::display_veh ()
{
    int x1 = 12, y1 = 12, x2 = -12, y2 = -12;
    for (int ep = 0; ep < veh->external_parts.size(); ep++)
    {
        int p = veh->external_parts[ep];
        if (veh->parts[p].mount_d.x < x1) x1 = veh->parts[p].mount_d.x;
        if (veh->parts[p].mount_d.y < y1) y1 = veh->parts[p].mount_d.y;
        if (veh->parts[p].mount_d.x > x2) x2 = veh->parts[p].mount_d.x;
        if (veh->parts[p].mount_d.y > y2) y2 = veh->parts[p].mount_d.y;
    }
	dd = point(0, 0);
    if (x2 - x1 < 11) { x1--; x2++; }
    if (y2 - y1 < 11 ) { y1--; y2++; }
    if (x1 < -5) dd.x = -5 - x1;
    else if (x2 > 6) dd.x = 6 - x2;
    if (y1 < -6) dd.y = -6 - y1;
    else if (y2 > 5) dd.y = 5 - y2;

    for (int ep = 0; ep < veh->external_parts.size(); ep++)
    {
        int p = veh->external_parts[ep];
        char sym = veh->part_sym (p);
        nc_color col = veh->part_color (p);
        int y = -(veh->parts[p].mount_d.x + dd.x);
        int x = veh->parts[p].mount_d.y + dd.y;
        mvwputch (w_disp, 6+y, 6+x, c.x == x && c.y == y? hilite(col) : col, special_symbol(sym));
        if (c.x == x && c.y == y) cpart = p;
    }
    wrefresh (w_disp);
}

void veh_interact::display_stats ()
{
    bool conf = veh->valid_wheel_config();
    mvwprintz(w_stats, 0, 1, c_ltgray, "Name: ");
    mvwprintz(w_stats, 0, 7, c_ltgreen, veh->name.c_str());
    if (option_table::get()[OPT_USE_METRIC_SYS]) {
     mvwprintz(w_stats, 1, 1, c_ltgray, "Safe speed:      Km/h");
     mvwprintz(w_stats, 1, 14, c_ltgreen,"%3d", int(veh->safe_velocity(false) * 0.0161f));
     mvwprintz(w_stats, 2, 1, c_ltgray, "Top speed:       Km/h");
     mvwprintz(w_stats, 2, 14, c_ltred, "%3d", int(veh->max_velocity(false) * 0.0161f));
     mvwprintz(w_stats, 3, 1, c_ltgray, "Accel.:          Kmh/t");
     mvwprintz(w_stats, 3, 14, c_ltblue,"%3d", int(veh->acceleration(false) * 0.0161f));
    } else {
     mvwprintz(w_stats, 1, 1, c_ltgray, "Safe speed:      mph");
     mvwprintz(w_stats, 1, 14, c_ltgreen,"%3d", veh->safe_velocity(false) / 100);
     mvwprintz(w_stats, 2, 1, c_ltgray, "Top speed:       mph");
     mvwprintz(w_stats, 2, 14, c_ltred, "%3d", veh->max_velocity(false) / 100);
     mvwprintz(w_stats, 3, 1, c_ltgray, "Accel.:          mph/t");
     mvwprintz(w_stats, 3, 14, c_ltblue,"%3d", veh->acceleration(false) / 100);
    }
    mvwprintz(w_stats, 4, 1, c_ltgray, "Mass:            kg");
    mvwprintz(w_stats, 4, 12, c_ltblue,"%5d", (int) (veh->total_mass() / 4 * 0.45));
    mvwprintz(w_stats, 1, 26, c_ltgray, "K dynamics:        ");
    mvwprintz(w_stats, 1, 37, c_ltblue, "%3d", (int) (veh->k_dynamics() * 100));
    mvwputch (w_stats, 1, 41, c_ltgray, '%');
    mvwprintz(w_stats, 2, 26, c_ltgray, "K mass:            ");
    mvwprintz(w_stats, 2, 37, c_ltblue, "%3d", (int) (veh->k_mass() * 100));
    mvwputch (w_stats, 2, 41, c_ltgray, '%');
    mvwprintz(w_stats, 5, 1, c_ltgray, "Wheels: ");
    mvwprintz(w_stats, 5, 11, conf? c_ltgreen : c_ltred, conf? "enough" : "  lack");
    mvwprintz(w_stats, 6, 1, c_ltgray,  "Fuel usage (safe):        ");
    int xfu = 20;
    int ftypes[3] = { AT_GAS, AT_BATT, AT_PLASMA };
    nc_color fcs[3] = { c_ltred, c_yellow, c_ltblue };
    bool first = true;
    for (int i = 0; i < 3; i++)
    {
        int fu = veh->basic_consumption (ftypes[i]);
        if (fu > 0)
        {
            fu = fu / 100;
            if (fu < 1)
                fu = 1;
            if (!first)
                mvwprintz(w_stats, 6, xfu++, c_ltgray, "/");
            mvwprintz(w_stats, 6, xfu++, fcs[i], "%d", fu);
            if (fu > 9) xfu++;
            if (fu > 99) xfu++;
            first = false;
        }
    }
    veh->print_fuel_indicator (w_stats, 0, 42);
    wrefresh (w_stats);
}

void veh_interact::display_mode (char mode)
{
    werase (w_mode);
    if (mode == ' ')
    {
        bool mi = !cant_do('i');
        bool mr = !cant_do('r');
        bool mf = !cant_do('f');
        bool mo = !cant_do('o');
        mvwprintz(w_mode, 0, 1, mi? c_ltgray : c_dkgray, "install");
        mvwputch (w_mode, 0, 1, mi? c_ltgreen : c_green, 'i');
        mvwprintz(w_mode, 0, 9, mr? c_ltgray : c_dkgray, "repair");
        mvwputch (w_mode, 0, 9, mr? c_ltgreen : c_green, 'r');
        mvwprintz(w_mode, 0, 16, mf? c_ltgray : c_dkgray, "refill");
        mvwputch (w_mode, 0, 18, mf? c_ltgreen : c_green, 'f');
        mvwprintz(w_mode, 0, 23, mo? c_ltgray : c_dkgray, "remove");
        mvwputch (w_mode, 0, 26, mo? c_ltgreen : c_green, 'o');
    }
    // C++20: constexpr strlen?
    mvwprintz(w_mode, 0, SCREEN_WIDTH - (sizeof("-back") + sizeof("ESC") - 1), c_ltgreen, "ESC");
    mvwprintz(w_mode, 0, SCREEN_WIDTH - sizeof("-back"), c_ltgray, "-back");
    wrefresh (w_mode);
}

void veh_interact::display_list (int pos)
{
    const int page_size = getmaxy(w_list);

    werase (w_list);
    int page = pos / page_size;
    for (int i = page * page_size; i < (page + 1) * page_size && i < can_mount.size(); i++)
    {
        const int y = i - page * page_size;
		const auto& p_info = vpart_info::list[can_mount[i]];
        const itype_id itm = p_info.item;
        const bool has_comps = crafting_inv.has_amount(itm, 1);
        const bool has_skill = _g->u.sklevel[sk_mechanics] >= p_info.difficulty;
        const nc_color col = (has_comps && has_skill) ? c_white : c_dkgray;
        mvwprintz(w_list, y, 3, pos == i ? hilite (col) : col, p_info.name);
        mvwputch (w_list, y, 1,  p_info.color, special_symbol (p_info.sym));
    }
    wrefresh (w_list);
}

vehicle& complete_vehicle(game* g)
{
    if (g->u.activity.values.size() < 7) throw std::string("Invalid activity ACT_VEHICLE values: ") + std::to_string(g->u.activity.values.size());
    vehicle* const veh = g->m.veh_at (g->u.activity.values[0], g->u.activity.values[1]);
    if (!veh) throw std::string("Activity ACT_VEHICLE: vehicle not found");

    char cmd = (char) g->u.activity.index;
    int dx = g->u.activity.values[4];
    int dy = g->u.activity.values[5];
    int part = g->u.activity.values[6];
    std::vector<component> comps;
    std::vector<component> tools;
    int welder_charges = dynamic_cast<it_tool*>(item::types[itm_welder])->charges_per_use;
    itype_id itm;
    bool broken;

    int dd = 2;
    switch (cmd)
    {
    case 'i':
        if (veh->install_part(dx, dy, (vpart_id)part) < 0) throw std::string("complete_vehicle install part fails dx=") + std::to_string(dx) + " dy=" + std::to_string(dy) + " id=" + std::to_string(part);
        comps.push_back(component(vpart_info::list[part].item, 1));
        consume_items(g->m, g->u, comps);
        tools.push_back(component(itm_welder, welder_charges));
        tools.push_back(component(itm_toolset, welder_charges/5));
        consume_tools(g->m, g->u, tools);
		messages.add("You install a %s into the %s.", vpart_info::list[part].name, veh->name.c_str());
        g->u.practice (sk_mechanics, vpart_info::list[part].difficulty * 5 + 20);
        break;
    case 'r':
        if (veh->parts[part].hp <= 0) {
            comps.push_back(component(veh->part_info(part).item, 1));
            consume_items(g->m, g->u, comps);
            tools.push_back(component(itm_wrench, 1));
            consume_tools(g->m, g->u, tools);
            tools.clear();
            dd = 0;
            veh->insides_dirty = true;
        }
        tools.push_back(component(itm_welder, welder_charges));
        tools.push_back(component(itm_toolset, welder_charges/5));
        consume_tools(g->m, g->u, tools);
        veh->parts[part].hp = veh->part_info(part).durability;
		messages.add("You repair the %s's %s.", veh->name.c_str(), veh->part_info(part).name);
        g->u.practice (sk_mechanics, (vpart_info::list[part].difficulty + dd) * 5 + 20);
        break;
    case 'f':
        if (!veh->refill(g->u, part, true)) throw std::string("complete_vehicle refill broken");
        veh->refill(g->u, part);
        break;
    case 'o':
		for (decltype(auto) it : veh->parts[part].items) g->m.add_item(g->u.pos, std::move(it));
        veh->parts[part].items.clear();
        itm = veh->part_info(part).item;
        broken = veh->parts[part].hp <= 0;
        if (veh->parts.size() < 2) {
			messages.add("You completely dismantle %s.", veh->name.c_str());
            g->u.activity.type = ACT_NULL;
            g->m.destroy_vehicle (veh);
        } else {
            messages.add("You remove %s%s from %s.", broken? "broken " : "",
                        veh->part_info(part).name, veh->name.c_str());
            veh->remove_part (part);
        }
        if (!broken) g->m.add_item(g->u.pos, item::types[itm], messages.turn);
        g->u.practice (sk_mechanics, 2 * 5 + 20);
        break;
    default:;
        
    }

    return *veh;
}



