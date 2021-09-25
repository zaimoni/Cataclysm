#include "pc.hpp"
#include "output.h"
#include "ui.h"
#include "stl_typetraits.h"
#include "options.h"
#include "recent_msg.h"
#include "rng.h"
#include "line.h"
#include "Zaimoni.STL/GDI/box.hpp"
#include "fragment.inc/rng_box.hpp"
#include "wrap_curses.h"
#include <array>
#include <memory>

pc::pc()
: kills(mon_type_count(), 0), next_inv('d'), mostseen(0), turnssincelastmon(0), run_mode(option_table::get()[OPT_SAFEMODE] ? 1 : 0), autosafemode(option_table::get()[OPT_AUTOSAFEMODE])
{
}

char pc::inc_invlet(char src)
{
    switch (src) {
        case 'z': return 'A';
        case 'Z': return 'a';
        default: return ++src;
    };
}

char pc::dec_invlet(char src)
{
    switch (src) {
        case 'a': return 'Z';
        case 'A': return 'z';
        default: return --src;
    };
}

bool pc::assign_invlet(item& it) const
{
    // This while loop guarantees the inventory letter won't be a repeat. If it
    // tries all 52 letters, it fails.
    int iter = 0;
    while (has_item(next_inv)) {
        next_inv = inc_invlet(next_inv);
        if (52 <= ++iter) return false;
    }
    it.invlet = next_inv;
    return true;
}

bool pc::assign_invlet_stacking_ok(item& it) const
{
    // This while loop guarantees the inventory letter won't be a repeat. If it
    // tries all 52 letters, it fails.
    int iter = 0;
    while (has_item(next_inv) && !i_at(next_inv).stacks_with(it)) {
        next_inv = inc_invlet(next_inv);
        if (52 <= ++iter) return false;
    }
    it.invlet = next_inv;
    return true;
}


static const std::string CATEGORIES[8] =
{ "FIREARMS:", "AMMUNITION:", "CLOTHING:", "COMESTIBLES:",
 "TOOLS:", "BOOKS:", "WEAPONS:", "OTHER:" };

static void print_inv_statics(const pc& u, WINDOW* w_inv, std::string title, const std::vector<char>& dropped_items)
{
    // Print our header
    mvwprintw(w_inv, 0, 0, title.c_str());

    const int mid_pt = SCREEN_WIDTH / 2;
    const int three_quarters = 3 * SCREEN_WIDTH / 4;

    // Print weight
    mvwprintw(w_inv, 0, mid_pt, "Weight: ");
    const int my_w_capacity = u.weight_capacity();
    const int my_w_carried = u.weight_carried();
    wprintz(w_inv, (my_w_carried >= my_w_capacity / 4) ? c_red : c_ltgray, "%d", my_w_carried);
    wprintz(w_inv, c_ltgray, "/%d/%d", my_w_capacity / 4, my_w_capacity);

    // Print volume
    mvwprintw(w_inv, 0, three_quarters, "Volume: ");
    const int my_v_capacity = u.volume_capacity() - 2;
    const int my_v_carried = u.volume_carried();
    wprintz(w_inv, (my_v_carried > my_v_capacity) ? c_red : c_ltgray, "%d", my_v_carried);
    wprintw(w_inv, "/%d", my_v_capacity);

    // Print our weapon
    mvwaddstrz(w_inv, 2, mid_pt, c_magenta, "WEAPON:");
    if (u.is_armed()) {
        const bool dropping_weapon = cataclysm::any(dropped_items, u.weapon.invlet);
        if (dropping_weapon)
            mvwprintz(w_inv, 3, mid_pt, c_white, "%c + %s", u.weapon.invlet, u.weapname().c_str());
        else
            mvwprintz(w_inv, 3, mid_pt, u.weapon.color_in_inventory(u), "%c - %s", u.weapon.invlet, u.weapname().c_str());
    }
    else if (u.weapon.is_style())
        mvwprintz(w_inv, 3, mid_pt, c_ltgray, "%c - %s", u.weapon.invlet, u.weapname().c_str());
    else
        mvwaddstrz(w_inv, 3, mid_pt + 2, c_ltgray, u.weapname().c_str());
    // Print worn items
    if (!u.worn.empty()) {
        mvwaddstrz(w_inv, 5, mid_pt, c_magenta, "ITEMS WORN:");
        int row = 6;
        for (const item& armor : u.worn) {
            const bool dropping_armor = cataclysm::any(dropped_items, armor.invlet);
            mvwprintz(w_inv, row++, mid_pt, (dropping_armor ? c_white : c_ltgray), "%c + %s", armor.invlet, armor.tname().c_str());
        }
    }
}

static auto find_firsts(const inventory& inv)
{
    std::array<int, 8> firsts;
    firsts.fill(-1);

    for (size_t i = 0; i < inv.size(); i++) {
        if (firsts[0] == -1 && inv[i].is_gun())
            firsts[0] = i;
        else if (firsts[1] == -1 && inv[i].is_ammo())
            firsts[1] = i;
        else if (firsts[2] == -1 && inv[i].is_armor())
            firsts[2] = i;
        else if (firsts[3] == -1 &&
            (inv[i].is_food() || inv[i].is_food_container()))
            firsts[3] = i;
        else if (firsts[4] == -1 && (inv[i].is_tool() || inv[i].is_gunmod() ||
            inv[i].is_bionic()))
            firsts[4] = i;
        else if (firsts[5] == -1 && inv[i].is_book())
            firsts[5] = i;
        else if (firsts[6] == -1 && inv[i].is_weap())
            firsts[6] = i;
        else if (firsts[7] == -1 && inv[i].is_other())
            firsts[7] = i;
    }

    return firsts;
}

// C:Whales: game::inv
char pc::get_invlet(std::string title)
{
    static const std::vector<char> null_vector;
    const int maxitems = VIEW - 5;	// Number of items to show at one time.
    int ch = '.';
    int start = 0, cur_it;
    sort_inv();
    inv.restack(this);
    {
    std::unique_ptr<WINDOW, curses_full_delete> w_inv(newwin(VIEW, SCREEN_WIDTH, 0, 0));
    print_inv_statics(*this, w_inv.get(), title, null_vector);
    // Gun, ammo, weapon, armor, food, tool, book, other
    const auto firsts = find_firsts(inv);

    do {
        if (ch == '<' && start > 0) { // Clear lines and shift
            for (int i = 1; i < VIEW; i++) draw_hline(w_inv.get(), i, c_black, ' ', 0, SCREEN_WIDTH / 2);
            start -= maxitems;
            if (start < 0) start = 0;
            mvwprintw(w_inv.get(), maxitems + 2, 0, "         ");
        }
        if (ch == '>' && cur_it < inv.size()) { // Clear lines and shift
            start = cur_it;
            mvwprintw(w_inv.get(), maxitems + 2, 12, "            ");
            for (int i = 1; i < VIEW; i++) draw_hline(w_inv.get(), i, c_black, ' ', 0, SCREEN_WIDTH / 2);
        }
        int cur_line = 2;
        for (cur_it = start; cur_it < start + maxitems && cur_line < 23; cur_it++) {
            // Clear the current line;
            mvwprintw(w_inv.get(), cur_line, 0, "                                    ");
            // Print category header
            for (int i = 0; i < 8; i++) {
                if (cur_it == firsts[i]) {
                    mvwaddstrz(w_inv.get(), cur_line, 0, c_magenta, CATEGORIES[i].c_str());
                    cur_line++;
                }
            }
            if (cur_it < inv.size()) {
                const item& it = inv[cur_it];
                mvwputch(w_inv.get(), cur_line, 0, c_white, it.invlet);
                mvwaddstrz(w_inv.get(), cur_line, 1, it.color_in_inventory(*this), it.tname().c_str());
                if (inv.stack_at(cur_it).size() > 1)
                    wprintw(w_inv.get(), " [%d]", inv.stack_at(cur_it).size());
                if (it.charges > 0)
                    wprintw(w_inv.get(), " (%d)", it.charges);
                else if (it.contents.size() == 1 && it.contents[0].charges > 0)
                    wprintw(w_inv.get(), " (%d)", it.contents[0].charges);
            }
            cur_line++;
        }
        if (start > 0)
            mvwprintw(w_inv.get(), maxitems + 4, 0, "< Go Back");
        if (cur_it < inv.size())
            mvwprintw(w_inv.get(), maxitems + 4, 12, "> More items");
        wrefresh(w_inv.get());
        ch = getch();
    } while (ch == '<' || ch == '>');
    } // force destruction of std::unique_ptr
    refresh_all();
    return ch;
}

// C:Whales: game::multidrop
std::vector<item> pc::multidrop()
{
    sort_inv();
    inv.restack(this);
    WINDOW* w_inv = newwin(VIEW, SCREEN_WIDTH, 0, 0);
    const int maxitems = VIEW - 5; // Number of items to show at one time.
    std::vector<int> dropping(inv.size(), 0);
    int count = 0; // The current count
    std::vector<char> weapon_and_armor; // Always single, not counted
    bool warned_about_bionic = false; // Printed add_msg re: dropping bionics
    print_inv_statics(*this, w_inv, "Multidrop:", weapon_and_armor);
    // Gun, ammo, weapon, armor, food, tool, book, other
    const auto firsts = find_firsts(inv);

    int ch = '.';
    int start = 0;
    size_t cur_it = 0;
    do {
        if (ch == '<' && start > 0) {
            for (int i = 1; i < VIEW; i++) draw_hline(w_inv, i, c_black, ' ', 0, SCREEN_WIDTH / 2);
            start -= maxitems;
            if (start < 0) start = 0;
            mvwprintw(w_inv, maxitems + 2, 0, "         ");
        }
        if (ch == '>' && cur_it < inv.size()) {
            start = cur_it;
            mvwprintw(w_inv, maxitems + 2, 12, "            ");
            for (int i = 1; i < VIEW; i++) draw_hline(w_inv, i, c_black, ' ', 0, SCREEN_WIDTH / 2);
        }
        int cur_line = 2;
        for (cur_it = start; cur_it < start + maxitems && cur_line < VIEW - 2; cur_it++) {
            // Clear the current line;
            mvwprintw(w_inv, cur_line, 0, "                                    ");
            // Print category header
            for (int i = 0; i < 8; i++) {
                if (cur_it == firsts[i]) {
                    mvwaddstrz(w_inv, cur_line, 0, c_magenta, CATEGORIES[i].c_str());
                    cur_line++;
                }
            }
            if (cur_it < inv.size()) {
                const item& it = inv[cur_it];
                mvwputch(w_inv, cur_line, 0, c_white, it.invlet);
                char icon = '-';
                if (dropping[cur_it] >= inv.stack_at(cur_it).size())
                    icon = '+';
                else if (dropping[cur_it] > 0)
                    icon = '#';
                nc_color col = (dropping[cur_it] == 0 ? c_ltgray : c_white);
                mvwprintz(w_inv, cur_line, 1, col, " %c %s", icon, it.tname().c_str());
                if (inv.stack_at(cur_it).size() > 1)
                    wprintz(w_inv, col, " [%d]", inv.stack_at(cur_it).size());
                if (it.charges > 0) wprintz(w_inv, col, " (%d)", it.charges);
                else if (it.contents.size() == 1 && it.contents[0].charges > 0)
                    wprintw(w_inv, " (%d)", it.contents[0].charges);
            }
            cur_line++;
        }
        if (start > 0) mvwprintw(w_inv, maxitems + 4, 0, "< Go Back");
        if (cur_it < inv.size()) mvwprintw(w_inv, maxitems + 4, 12, "> More items");
        wrefresh(w_inv);
        ch = getch();
        if (ch >= '0' && ch <= '9') {
            ch -= '0';
            count *= 10;
            count += ch;
        }
        else if (auto obj = from_invlet(ch)) {
            if (0 > obj->second) { // Not from inventory
                int found = false;
                for (int i = 0; i < weapon_and_armor.size() && !found; i++) {
                    if (weapon_and_armor[i] == ch) {
                        EraseAt(weapon_and_armor, i);
                        found = true;
                        print_inv_statics(*this, w_inv, "Multidrop:", weapon_and_armor);
                    }
                }
                if (!found) {
                    if (ch == weapon.invlet && is_between(num_items + 1, weapon.type->id, num_all_items - 1)) {
                        if (!warned_about_bionic)
                            messages.add("You cannot drop your %s.", weapon.tname().c_str());
                        warned_about_bionic = true;
                    }
                    else {
                        weapon_and_armor.push_back(ch);
                        print_inv_statics(*this, w_inv, "Multidrop:", weapon_and_armor);
                    }
                }
            }
            else {
                const int index = obj->second;
                const size_t ub = inv.stack_at(index).size();
                if (count == 0) {
                    dropping[index] = (0 == dropping[index]) ? ub : 0;
                }
                else {
                    dropping[index] = (count >= ub) ? ub : count;
                }
            }
            count = 0;
        }
    } while (ch != '\n' && ch != KEY_ESCAPE && ch != ' ');
    werase(w_inv);
    delwin(w_inv);
    erase();
    refresh_all();

    std::vector<item> ret;

    if (ch != '\n') return ret; // Canceled!

    int current_stack = 0;
    size_t max_size = inv.size();
    for (size_t i = 0; i < max_size; i++) {
        for (int j = 0; j < dropping[i]; j++) {
            if (current_stack >= 0) {
                if (inv.stack_at(current_stack).size() == 1) {
                    ret.push_back(inv.remove_item(current_stack));
                    current_stack--;
                }
                else
                    ret.push_back(inv.remove_item(current_stack));
            }
        }
        current_stack++;
    }

    for (int i = 0; i < weapon_and_armor.size(); i++)
        ret.push_back(i_rem(weapon_and_armor[i]));

    return ret;
}

// add_footstep will create a list of locations to draw monster
// footsteps. these will be more or less accurate depending on the
// characters hearing and how close they are
void pc::add_footstep(const point& orig, int volume)
{
    if (orig == pos) return;
    else if (see(orig)) return;

    int distance = rl_dist(orig, pos);
    int err_offset;
    // \todo V 0.2.1 rethink this (effect is very loud, very close sounds don't have good precision
    if (volume / distance < 2)
        err_offset = 3;
    else if (volume / distance < 3)
        err_offset = 2;
    else
        err_offset = 1;
    if (has_bionic(bio_ears)) err_offset--;
    if (has_trait(PF_BADHEARING)) err_offset++;

    if (0 >= err_offset) {
        footsteps.push_back(orig);
        return;
    }

    const zaimoni::gdi::box<point> spread(point(-err_offset), point(err_offset));
    int tries = 0;
    do {
        const auto pt = orig + rng(spread);
        if (pt != pos && !see(pt)) {
            footsteps.push_back(pt);
            return;
        }
    } while (++tries < 10);
}

// draws footsteps that have been created by monsters moving about
void pc::draw_footsteps(void* w)
{
    auto w_terrain = reinterpret_cast<WINDOW*>(w);

    for (const point& step : footsteps) {
        const point draw_at(point(VIEW_CENTER) + step - pos);
        mvwputch(w_terrain, draw_at.y, draw_at.x, c_yellow, '?');
    }
    footsteps.clear(); // C:Whales behavior; never reaches savefile, display cleared on save/load cycling
    wrefresh(w_terrain);
    return;
}

void pc::toggle_safe_mode()
{
    if (0 == run_mode) {
        run_mode = 1;
        messages.add("Safe mode ON!");
    } else {
        turnssincelastmon = 0;
        run_mode = 0;
        if (autosafemode)
            messages.add("Safe mode OFF! (Auto safe mode still enabled!)");
        else
            messages.add("Safe mode OFF!");
    }
}

void pc::toggle_autosafe_mode()
{
    if (autosafemode) {
        messages.add("Auto safe mode OFF!");
        autosafemode = false;
    } else {
        messages.add("Auto safe mode ON");
        autosafemode = true;
    }
}

void pc::stop_on_sighting(int new_seen)
{
    if (new_seen > mostseen) {
        cancel_activity_query("Monster spotted!");
        turnssincelastmon = 0;
        if (1 == run_mode) run_mode = 2;	// Stop movement!
    } else if (autosafemode) { // Auto-safemode
        turnssincelastmon++;
        if (turnssincelastmon >= 50 && 0 == run_mode) run_mode = 1;
    }

    mostseen = new_seen;
}

void pc::ignore_enemy()
{
    if (2 == run_mode) {
        messages.add("Ignoring enemy!");
        run_mode = 1;
    }
}

std::optional<std::string> pc::move_is_unsafe() const
{
    // Monsters around and we don't wanna run
    if (2 == run_mode) return "Monster spotted--safe mode is on! (Press '!' to turn it off or ' to ignore monster.)";
    return std::nullopt;
}
