/* Main Loop for cataclysm */

#include "game.h"
#include "options.h"
#include "mapbuffer.h"
#include "file.h"
#include "JSON.h"
#include <time.h>
#include <fstream>
#include <iostream>
#include <exception>

using namespace cataclysm;

static bool scalar_on_hard_drive(const JSON& x)
{
	if (!x.is_scalar()) return false;
	OS_dir working;
	// we use a hashtag to cope with tilesheets.
	const char* const test = x.scalar().c_str();
	const char* const index = strchr(test, '#');
	return working.exists(!index ? test : std::string(test,index-test).c_str());
}

static bool preload_image(const JSON& x)
{
	if (!x.is_scalar() || x.empty()) return false;
	return load_tile(x.scalar().c_str());
}

int main(int argc, char *argv[])
{
 srand(time(NULL));

 // XXX want to load tiles before initscr; implies errors before initscr() go to C stderr or C stdout	\todo IMPLEMENT
 // want a stderr.txt as well (cf. Wesnoth 1.12- [went away in Wesnoth 1.14])

 // C:DDA retained the data directory, but has a very different layout
 // following code was based on one file/tile, but C:DDA has pre-prepared tilesheets
#ifdef OS_dir
 {
 OS_dir working;
 std::ifstream fin;
 if (working.exists("data/json/tiles.json"))	// base game tiles specification
	{
	fin.open("data/json/tiles.json");
	if (fin.is_open())
		{
		try {
			JSON incoming(fin);
			fin.close();
			// specification for tiles: all values are to be valid relative paths to images on the hard drive
			if (   JSON::object == incoming.mode()
				&& !incoming.empty()
				&& incoming.destructive_grep(scalar_on_hard_drive)) {
				JSON::cache["tiles"] = std::move(incoming);
			}
		} catch (const std::exception& e) {
			fin.close();
			std::cerr << "tiles.json: " << e.what();
		}
		}
	}
 }
#endif
 // when we support mods, we load their tiles configuration from tiles.json as well (and check their filepaths are ok
 // value null could be used to unset a pre-existing value
 if (JSON::cache.count("tiles") && !JSON::cache["tiles"].destructive_grep(preload_image)) JSON::cache.erase("tiles");
 // now: wire tiles to types.  For now just do terrain (ter_t indexed by ter_id)  Cf. C:DDA for ideas

// ncurses stuff
 initscr(); // Initialize ncurses
 noecho();  // Don't echo keypresses
 cbreak();  // C-style breaks (e.g. ^C to SIGINT)
 keypad(stdscr, true); // Numpad is numbers
 init_colors(); // See color.cpp
 curs_set(0); // Invisible cursor

 rand();  // For some reason a call to rand() seems to be necessary to avoid
          // repetion.
 bool quit_game = false;
 game *g = new game;
 MAPBUFFER = mapbuffer(g);
 MAPBUFFER.load();
 load_options();
 do {
  g->setup();
  while (!g->do_turn());
  if (g->game_quit())
   quit_game = true;
 } while (!quit_game);
 MAPBUFFER.save();
 erase(); // Clear screen
 endwin(); // End ncurses
 system("clear"); // Tell the terminal to clear itself
 return 0;
}

