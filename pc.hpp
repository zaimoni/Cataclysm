#ifndef PC_HPP
#define PC_HPP 1

#include "player.h"

// Ok to migrate per-player game UI here.
class pc final : public player
{
public:
	pc() = default;
	pc(const cataclysm::JSON& src) : player(src) {}
	pc(const pc& rhs) = default;
	pc(pc&& rhs) = default;
	virtual ~pc() = default;
//	friend cataclysm::JSON toJSON(const player & src); // will need to adjust this eventually

	pc& operator=(const pc& rhs);
	pc& operator=(pc&& rhs) = default;

	char get_invlet(std::string title);
	std::vector<item> multidrop();

	// over in game.cpp(!)
	void refresh_all() const;
};

#endif
