#include "tileray.h"
#include <math.h>
#include <stdlib.h>

// math.h need not be XOPEN
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static const int sx[4] = { 1, -1, -1, 1 };
static const int sy[4] = { 1, 1, -1, -1 };

tileray::tileray(int adx, int ady) noexcept
: deltax(adx), deltay(ady), leftover(0), direction(0), last_dx(0), last_dy(0), steps(0), infinite(false)
{
    if (adx || ady) direction = (int)(atan2(deltay, deltax) * 180.0 / M_PI);
}

tileray::tileray (int adir): direction (adir)
{
    init (adir);
}

void tileray::init (int adir)
{
    leftover = 0;
    direction = adir;
    if (direction < 0)
        direction += 360;
    if (direction >= 360)
        direction -= 360;
    last_dx = 0;
    last_dy = 0;
    deltax = abs((int) (cos ((float) direction * M_PI / 180.0) * 100));
    deltay = abs((int) (sin ((float) direction * M_PI / 180.0) * 100));
    infinite = true;
    steps = 0;
}

int tileray::dir4 () const
{
    if (direction >= 45 && direction <= 135) return 1;
    else if (direction > 135 && direction < 225) return 2;
    else if (direction >= 225 && direction <= 315) return 3;
    else return 0;
}

char tileray::dir_symbol (char sym) const
{
    switch (sym)
    {
    case 'j':
        switch (dir4())
        {
        default:
        case 0: return 'h';
        case 1: return 'j';
        case 2: return 'h';
        case 3: return 'j';
        }
    case 'h':
        switch (dir4())
        {
        default:
        case 0: return 'j';
        case 1: return 'h';
        case 2: return 'j';
        case 3: return 'h';
        }
    case 'y':
        switch (dir4())
        {
        default:
        case 0: return 'u';
        case 1: return 'n';
        case 2: return 'b';
        case 3: return 'y';
        }
    case 'u':
        switch (dir4())
        {
        default:
        case 0: return 'n';
        case 1: return 'b';
        case 2: return 'y';
        case 3: return 'u';
        }
    case 'n':
        switch (dir4())
        {
        default:
        case 0: return 'b';
        case 1: return 'y';
        case 2: return 'u';
        case 3: return 'n';
        }
    case 'b':
        switch (dir4())
        {
        default:
        case 0: return 'y';
        case 1: return 'u';
        case 2: return 'n';
        case 3: return 'b';
        }
    case '^':
        switch (dir4())
        {
        default:
        case 0: return '>';
        case 1: return 'v';
        case 2: return '<';
        case 3: return '^';
        }
    case '[':
        switch (dir4())
        {
        default:
        case 0: return '-';
        case 1: return '[';
        case 2: return '-';
        case 3: return '[';
        }
    case ']':
        switch (dir4())
        {
        default:
        case 0: return '-';
        case 1: return ']';
        case 2: return '-';
        case 3: return ']';
        }
    case '|':
        switch (dir4())
        {
        default:
        case 0: return '-';
        case 1: return '|';
        case 2: return '-';
        case 3: return '|';
        }
    case '-':
        switch (dir4())
        {
        default:
        case 0: return '|';
        case 1: return '-';
        case 2: return '|';
        case 3: return '-';
        }
    case '=':
        switch (dir4())
        {
        default:
        case 0: return 'H';
        case 1: return '=';
        case 2: return 'H';
        case 3: return '=';
        }
    case 'H':
        switch (dir4())
        {
        default:
        case 0: return '=';
        case 1: return 'H';
        case 2: return '=';
        case 3: return 'H';
        }
    case '\\':
        switch (dir4())
        {
        default:
        case 0: return '/';
        case 1: return '\\';
        case 2: return '/';
        case 3: return '\\';
        }
    case '/':
        switch (dir4())
        {
        default:
        case 0: return '\\';
        case 1: return '/';
        case 2: return '\\';
        case 3: return '/';
        }
    default:;
    }
    return sym;
}

int tileray::ortho_dx(int od) const
{
    int quadr = (direction / 90) % 4;
    od *= -sy[quadr];
    return mostly_vertical()? od : 0;
}

int tileray::ortho_dy(int od) const
{
    int quadr = (direction / 90) % 4;
    od *= sx[quadr];
    return mostly_vertical()? 0 : od;
}

bool tileray::mostly_vertical() const
{
    return abs(deltax) <= abs(deltay);
}

void tileray::advance (int num)
{
    last_dx = last_dy = 0;
    int ax = abs(deltax);
    int ay = abs(deltay);
    int anum = abs (num);
    for (int i = 0; i < anum; i++)
    {
        if (mostly_vertical ())
        { // mostly vertical line
            leftover += ax;
            if (leftover >= ay)
            {
                last_dx++;
                leftover -= ay;
            }
            last_dy++;
        }
        else
        { // mostly horizontal line
            leftover += ay;
            if (leftover >= ax)
            {
                last_dy++;
                leftover -= ax;
            }
            last_dx++;
        }
        steps++;
    }

    // offset calculated for 0-90 deg quadrant, we need to adjust if direction is other
    int quadr = (direction / 90) % 4;
    last_dx *= sx[quadr];
    last_dy *= sy[quadr];
    if (num < 0)
    {
        last_dx = -last_dx;
        last_dy = -last_dy;
    }
}

bool tileray::end() const
{
    if (infinite) return true;
    return mostly_vertical()? steps >= abs(deltay) - 1 : steps >= abs(deltax) - 1;
}


