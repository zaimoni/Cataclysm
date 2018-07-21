#ifndef WRAP_CURSES_H

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

#ifndef CURSES_HAS_TILESET
// stock curses library does not have tileset extensions; no-op them
inline bool load_tile(const char* src) { return false; }	// automatic failure
#endif


#endif