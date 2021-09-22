#ifndef PC_HPP
#define PC_HPP 1

#include "player.h"

// Ok to migrate per-player game UI here.
class pc final : public player
{
public:
	pc() = default;
	pc(const cataclysm::JSON& src);
	pc(const pc& rhs) = default;
	pc(pc&& rhs) = default;
	virtual ~pc() = default;
	friend cataclysm::JSON toJSON(const pc& src);

	pc& operator=(const pc& rhs) = default;
	pc& operator=(pc&& rhs) = default;

	char get_invlet(std::string title = "Inventory:");
	std::vector<item> multidrop();

	static char inc_invlet(char src);
	static char dec_invlet(char src);
	bool assign_invlet(item& it) const;
	bool assign_invlet_stacking_ok(item& it) const;

	void add_footstep(const point& orig, int volume);
	void draw_footsteps(/* WINDOW* */ void* w);

	// over in game.cpp(!)
	void refresh_all() const;

private:
	mutable char next_inv;	// Determines which letter the next inv item will have
	std::vector<point> footsteps;	// visual cue to monsters moving out of the players sight
};

#endif
