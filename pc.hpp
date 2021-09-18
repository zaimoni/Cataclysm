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

	// over in game.cpp(!)
	void refresh_all() const;

private:
	char next_inv;	// Determines which letter the next inv item will have
};

#endif
