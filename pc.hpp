#ifndef PC_HPP
#define PC_HPP 1

#include "player.h"

// Ok to migrate per-player game UI here.
class pc final : public player
{
public:
	pc();
	pc(const cataclysm::JSON& src);
	pc(const pc& rhs) = default;
	pc(pc&& rhs) = default;
	virtual ~pc() = default;
	friend cataclysm::JSON toJSON(const pc& src);

	pc& operator=(const pc& rhs) = default;
	pc& operator=(pc&& rhs) = default;

	// abstract ui
	bool if_visible_message(const char* msg) const override;

	// invlets
	char get_invlet(std::string title = "Inventory:");
	std::vector<item> multidrop();

	static char inc_invlet(char src);
	static char dec_invlet(char src);
	bool assign_invlet(item& it) const;
	bool assign_invlet_stacking_ok(item& it) const;

	// invlet UI
	void use(char let);
	void read(char let); // Read a book	V 0.2.8+ \todo enable for NPCs

	// footstep UI
	void add_footstep(const point& orig, int volume);
	void draw_footsteps(/* WINDOW* */ void* w);

	// safe mode/auto-run commands
	void toggle_safe_mode();
	void toggle_autosafe_mode();
	void ignore_enemy();

	// safe mode/auto-run implementation
	void stop_on_sighting(int new_seen);
	std::optional<std::string> move_is_unsafe() const;
	bool feels_safe() const { return 0 < run_mode; }

	// player kill tracking
	void record_kill(const monster& m) override;
	std::vector<std::pair<const mtype*, int> > summarize_kills();

	// newcharacter.cpp
	bool create(game* g, character_type type, std::string tempname = "");

	// game.cpp(!)
	void refresh_all() const;

private:
	bool install_bionics(const it_bionic* type);

	std::vector<point> footsteps;	// visual cue to monsters moving out of the players sight
	std::vector<int> kills; // per monster type

	// autorun UI
	int mostseen;	 // # of mons seen last turn; if this increases, run_mode++
	int turnssincelastmon; // needed for auto run mode
	char run_mode; // 0 - Normal run always; 1 - Running allowed, but if a new
		   //  monsters spawns, go to 2 - No movement allowed
	bool autosafemode; // is autosafemode enabled?

	mutable char next_inv;	// Determines which letter the next inv item will have
};

#endif
