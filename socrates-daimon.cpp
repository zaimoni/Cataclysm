/* Main Loop for Socrates' Daimon, Cataclysm analysis utility */

#include "color.h"
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[])
{
	// \todo parse command line options

	srand(time(NULL));

	// ncurses stuff
	initscr(); // Initialize ncurses
	noecho();  // Don't echo keypresses
	cbreak();  // C-style breaks (e.g. ^C to SIGINT)
	keypad(stdscr, true); // Numpad is numbers
	init_colors(); // See color.cpp
	curs_set(0); // Invisible cursor

	// route the command line options here

	erase(); // Clear screen
	endwin(); // End ncurses
	system("clear"); // Tell the terminal to clear itself
	return 0;
}

