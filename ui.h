#ifndef UI_H
#define UI_H 1

// display constants related to map scaling/implementation and the curses console
enum {
	// SEEX/SEEY is how far the player can see in the X/Y direction (at least, without scrolling).
	// each map segment will need to be at least this wide/high; map is at least 3x as wide/tall
	SEE = 12,	// for when it matters that they are explicitly equal
	SEEX = SEE,
	SEEY = SEE,
#ifdef ARCHAIC_UI
	VIEW = 2 * SEEX + 1,	// required curses view window height.  Default 25.  Fatcat graphical clients can resize as needed.
	VIEW_CENTER = VIEW/2,	// not redundant once configuration options go in
	PANELX = 55,
	// the canonical view with the player-scale map had a 55-wide "control panel", for a total width of 25+55=80
	SCREEN_WIDTH = VIEW + PANELX,
#endif
	// minimap
	MINIMAP_WIDTH_HEIGHT = 7,
	// status bar
	STATUS_BAR_HEIGHT = 4,
	// mon info
	MON_INFO_HEIGHT = 12,
	TABBED_HEADER_HEIGHT = 3,
	VBAR_X = 30 // number of screens have hard-coded left pane 30 width
};

#ifndef ARCHAIC_UI
// Technical debt: macros dependent on option_table @ options.h
#define VIEW ((int)(option_table::get()[OPT_VIEW]))
#define VIEW_CENTER (VIEW/2)
#define PANELX ((int)(option_table::get()[OPT_PANELX]))
#define SCREEN_WIDTH ((int)(option_table::get()[OPT_SCREENWIDTH]))
#endif

#endif

