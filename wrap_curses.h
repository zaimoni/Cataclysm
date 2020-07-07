#ifndef WRAP_CURSES_H
#define WRAP_CURSES_H 1

// to force system-installed curses, define CURSES_HEADER appropriately
#ifndef CURSES_HEADER
#if (defined _WIN32 || defined WINDOWS)
#define CURSES_HEADER "catacurse.h"
#else
#define CURSES_HEADER  <curses.h>
#endif
#endif

#include CURSES_HEADER
#undef CURSES_HEADER

// integrity checks
#ifndef OK
#error curses library should provide OK macro
#elif OK
#error curses library OK macro should be zero
#endif
#ifndef ERR
#error curses library should provide ERR macro
#endif

// Not a standard curses define; only effective for noecho or timeout modes.
// No ALT key can be seen directly from curses, if this is possible.
#define KEY_ESCAPE 27

struct curses_delete {
    constexpr curses_delete() noexcept = default;
    void operator()(WINDOW* w) const { delwin(w); }
};

struct curses_full_delete {
    constexpr curses_full_delete() noexcept = default;
    void operator()(WINDOW* w) const {
        werase(w);
        wrefresh(w);    // \todo? unsure if this is actually needed
        delwin(w);
    }
};

#ifndef CURSES_HAS_TILESET
// stock curses library does not have tileset extensions; no-op them
inline bool load_tile(const char* src) { return false; }	// automatic failure
inline void flush_tilesheets() {}
inline bool mvwaddbgtile(WINDOW* win, int y, int x, const char* bgtile) { return false; }
inline bool mvwaddfgtile(WINDOW* win, int y, int x, const char* fgtile) { return false; }
#endif


#endif
