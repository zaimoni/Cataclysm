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

int main(int argc, char *argv[])
{
 srand(time(NULL));

 // XXX want to load tiles before initscr; implies errors before initscr() go to C stderr or C stdout	\todo IMPLEMENT
 // want a stderr.txt as well (cf. Wesnoth 1.12- [went away in Wesnoth 1.14])

 // C:DDA retained the data directory, but has a very different layout
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
			// XXX put the JSON where it belongs
			if (JSON::object == incoming.mode() && !incoming.empty()) JSON::cache["tiles"] = std::move(incoming);
		} catch (const std::exception& e) {
			std::cerr << "tiles.json: " << e.what();
		}
		fin.close();
		}
	}
 }
#endif
 // XXX \todo check that tiles exist on hard drive
 // when we support mods, we load their tiles configuration from tiles.json as well (and check their filepaths are ok
 // XXX preload tiles into the currses extensions
 if (JSON::cache.count("tiles"))
	{
	JSON tmp = JSON::cache["tiles"];
	// \todo filter out all non-scalar values
	// \todo filter out all values that fail to preload
	}

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

