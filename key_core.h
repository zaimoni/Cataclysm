#ifndef _KEY_CORE_H_
#define _KEY_CORE_H_

// Not a standard curses define; only effective for noecho or timeout modes.
// No ALT key can be seen directly from curses, if this is possible.
// \todo migrate to wrap_curses.h?
#define KEY_ESCAPE 27
#endif
