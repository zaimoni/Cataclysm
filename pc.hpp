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
	bool is_enemy(const player* survivor = nullptr) const override; // target wants to kill/mug us
	void subjective_message(const std::string& msg) const override;
	void subjective_message(const char* msg) const override { subjective_message(std::string(msg)); }
	bool if_visible_message(std::function<std::string()> msg) const override;
	bool if_visible_message(std::function<std::string()> me, std::function<std::string()> other) const override { return if_visible_message(me); }
	bool if_visible_message(const char* msg) const override;
	bool ask_yn(const char* msg, std::function<bool()> ai = nullptr) const override;
	bool ask_yn(const std::string& msg, std::function<bool()> ai = nullptr) const { return ask_yn(msg.c_str(), ai); }
	int use_active(item& it) override;
	std::optional<item_spec> choose(const char* prompt, std::function<std::optional<std::string>(const item_spec&)> fail) override;
	void disarm(GPS_loc loc);
	void complete_butcher();

	// grammatical support
	std::string regular_verb_agreement(const std::string& verb) const override { return verb; }
	std::string to_be() const override { return "are"; }
	std::string subject() const override { return "you"; }
	std::string direct_object() const override { return "you"; }
	std::string indirect_object() const override { return "you"; }
	std::string possessive() const override { return "your"; }
	std::string pronoun(role r) const override;

	// invlets
	char get_invlet(std::string title = "Inventory:");
	std::vector<item> multidrop();
	std::optional<item_spec_const> has_in_inventory(char let) const;
	bool wear(char let);	// Wear item; returns false on fail

	static char inc_invlet(char src);
	static char dec_invlet(char src);
	bool assign_invlet(item& it) const;
	bool assign_invlet_stacking_ok(item& it) const;
	void reassign_item(); // Reassign the letter of an item   '='

	// invlet UI
	item i_rem(char let);	// Remove item from inventory; returns ret_null on fail
	void use(char let);
	void read(char let); // Read a book	V 0.2.8+ \todo enable for NPCs
	bool takeoff(char let);// Take off item; returns false on fail

	// bionics
	void power_bionics(); // V 0.2.1 extend to NPCs

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
	void target_dead(int deceased);
	void set_target(int whom) { target = whom; }
	bool is_target(int whom) const { return target == whom; }
	void validate_target(std::function<bool(int)> ok) { if (!ok(target)) target = -1; }

	// newcharacter.cpp
	bool create(game* g, character_type type, std::string tempname = "");

	// game.cpp(!)
	void refresh_all() const;

private:
	void consume(item& food) override;
	bool install_bionics(const it_bionic* type);

	std::vector<point> footsteps;	// visual cue to monsters moving out of the players sight
	std::vector<int> kills; // per monster type

	// autorun UI
	int mostseen;	 // # of mons seen last turn; if this increases, run_mode++
	int turnssincelastmon; // needed for auto run mode
	char run_mode; // 0 - Normal run always; 1 - Running allowed, but if a new
		   //  monsters spawns, go to 2 - No movement allowed
	bool autosafemode; // is autosafemode enabled?

	int target; // non-negative values are index into game::z.  Negative values would signal targeting NPCs or other PCs; -1 is self-targeting.

	mutable char next_inv;	// Determines which letter the next inv item will have
};

#endif
