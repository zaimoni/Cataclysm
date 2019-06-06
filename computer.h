#ifndef _COMPUTER_H_
#define _COMPUTER_H_

#include "item.h"
#include <vector>
#include <string>
#include <iosfwd>

class game;
class player;

enum computer_action
{
 COMPACT_NULL = 0,
 COMPACT_OPEN,
 COMPACT_SAMPLE,
 COMPACT_RELEASE,
 COMPACT_TERMINATE,
 COMPACT_PORTAL,
 COMPACT_CASCADE,
 COMPACT_RESEARCH,
 COMPACT_MAPS,
 COMPACT_MAP_SEWER,
 COMPACT_MISS_LAUNCH,
 COMPACT_MISS_DISARM,
 COMPACT_LIST_BIONICS,
 COMPACT_ELEVATOR_ON,
 COMPACT_AMIGARA_LOG,
 COMPACT_AMIGARA_START,
 COMPACT_DOWNLOAD_SOFTWARE,
 COMPACT_BLOOD_ANAL,
 NUM_COMPUTER_ACTIONS
};

enum computer_failure
{
 COMPFAIL_NULL = 0,
 COMPFAIL_SHUTDOWN,
 COMPFAIL_ALARM,
 COMPFAIL_MANHACKS,
 COMPFAIL_SECUBOTS,
 COMPFAIL_DAMAGE,
 COMPFAIL_PUMP_EXPLODE,
 COMPFAIL_PUMP_LEAK,
 COMPFAIL_AMIGARA,
 COMPFAIL_DESTROY_BLOOD,
 NUM_COMPUTER_FAILURES
};

const char* JSON_key(computer_action src);
const char* JSON_key(computer_failure src);

namespace cataclysm {

	template<>
	struct JSON_parse<computer_action>
	{
		computer_action operator()(const char* src);
		computer_action operator()(const std::string& src) { return operator()(src.c_str()); };
	};

	template<>
	struct JSON_parse<computer_failure>
	{
		computer_failure operator()(const char* src);
		computer_failure operator()(const std::string& src) { return operator()(src.c_str()); };
	};

}


struct computer_option
{
 std::string name;
 computer_action action;
 int security;

 computer_option(std::string N = "Unknown", computer_action A = COMPACT_NULL, int S = 0) :
   name (N), action (A), security (S) {};

 computer_option(std::istream& is);
 friend std::ostream& operator<<(std::ostream& os, const computer_option& src);
};

class computer
{
private:
// Save/load
 friend std::istream& operator>>(std::istream& is, computer& dest);
 friend std::ostream& operator<<(std::ostream& os, const computer& src);

 std::vector<computer_option> options;   // Things we can do
 std::vector<computer_failure> failures; // Things that happen if we fail a hack
 int security; // Difficulty of simply logging in
 WINDOW *w_terminal; // Output window
public:
 std::string name; // "Jon's Computer", "Lab 6E77-B Terminal Omega"
 int mission_id; // Linked to a mission?

 computer(std::string Name = "", int Security = 0) : name(Name), security(Security), w_terminal(NULL), mission_id(-1) {}
 ~computer();

 computer & operator=(const computer &rhs);
// Initialization
 void add_option(std::string opt_name, computer_action action, int Security);
 void add_failure(computer_failure failure);
// Basic usage
 void shutdown_terminal(); // Shutdown (free w_terminal, etc)
 void use(game *g);
 bool hack_attempt(player *p, int Security = -1);// -1 defaults to main security

private:

// Called by use()
 void activate_function      (game *g, computer_action action);
// Generally called when we fail a hack attempt
 void activate_random_failure(game *g);
// ...but we can also choose a specific failure.
 void activate_failure       (game *g, computer_failure fail);

 bool blood_analysis_precondition(const std::vector<item>& inv);

// OUTPUT/INPUT
// Reset to a blank terminal (e.g. at start of usage loop)
 void reset_terminal();
// Prints a line to the terminal (with printf flags)
 void print_line(const char *text, ...);
// For now, the same as print_line but in red (TODO: change this?)
 void print_error(const char *text, ...);
// Prints code-looking gibberish
 void print_gibberish_line();
// Prints a line and waits for Y/N/Q
 char query_ynq(const char *text, ...);
// Same as query_ynq, but returns true for y or Y
 bool query_bool(const char *text, ...);
};

#endif
