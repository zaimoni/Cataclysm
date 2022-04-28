#include "gui.hpp"
#include "monster.h"
#include "npc.h"
#include "pc.hpp"
#include "output.h"
#include "ui.h"
#include "options.h"

static bool in_bounds(const point& at)
{
    return 0 <= at.x && at.x < VIEW && 0 <= at.y && at.y < VIEW;
}

// cf. monster::draw
void draw_mob::operator()(const monster* whom)
{
    const auto delta = whom->pos - origin;
    if (!in_bounds(delta)) return;
    const point pt = delta + point(VIEW_CENTER);
    const auto type = whom->type;    // backward compatibility

    nc_color color = type->color;
    char sym = type->sym;

    if (mtype::tiles.count(type->id)) {
        //	 if (mvwaddfgtile(w, pt.y, pt.x, mtype::tiles[type->id].c_str())) sym = ' ';	// we still want any background color effects
        mvwaddfgtile(w, pt.y, pt.x, mtype::tiles[type->id].c_str());
    }

    if (whom->is_friend() && !invert) // viewpoint is PC
        mvwputch_hi(w, pt.y, pt.x, color, sym);
    else if (invert)
        mvwputch_inv(w, pt.y, pt.x, color, sym);
    else {
        color = whom->color_with_effects();
        mvwputch(w, pt.y, pt.x, color, sym);
    }
}

// cf. npc::draw
void draw_mob::operator()(const npc* whom)
{
    const auto delta = whom->pos - origin;
    if (!in_bounds(delta)) return;
    const point dest = delta + point(VIEW_CENTER);
    nc_color col = c_pink;
    if (NPCATT_KILL == whom->attitude) col = c_red;
    if (whom->is_friend())             col = c_green;
    else if (whom->is_following())     col = c_ltgreen;
    if (invert) mvwputch_inv(w, dest.y, dest.x, col, '@');
    else     mvwputch(w, dest.y, dest.x, col, '@');
}

void draw_mob::operator()(const pc* whom)
{
    const auto delta = whom->pos - origin;
    if (!in_bounds(delta)) return;

    const point at(delta + point(VIEW_CENTER));
    mvwputch(w, at.y, at.x, whom->color(), '@');
}

