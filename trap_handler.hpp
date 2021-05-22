#ifndef TRAP_HANDLER_HPP
#define TRAP_HANDLER_HPP 1

class game;
class monster;

struct trapfunc {
	static void bubble(game* g, int x, int y);
	static void beartrap(game* g, int x, int y);
	static void snare(game* g, int x, int y) { }; // \todo? implement
	static void board(game* g, int x, int y);
	static void tripwire(game* g, int x, int y);
	static void crossbow(game* g, int x, int y);
	static void shotgun(game* g, int x, int y);
	static void blade(game* g, int x, int y);
	static void landmine(game* g, int x, int y);
	static void telepad(game* g, int x, int y);
	static void goo(game* g, int x, int y);
	static void dissector(game* g, int x, int y);
	static void sinkhole(game* g, int x, int y);
	static void pit(game* g, int x, int y);
	static void pit_spikes(game* g, int x, int y);
	static void lava(game* g, int x, int y);
	static void portal(game* g, int x, int y) { }; // \todo major game feature
	static void ledge(game* g, int x, int y);
	static void boobytrap(game* g, int x, int y);
	static void temple_flood(game* g, int x, int y);
	static void temple_toggle(game* g, int x, int y);
	static void glow(game* g, int x, int y);
	static void hum(game* g, int x, int y);
	static void shadow(game* g, int x, int y);
	static void drain(game* g, int x, int y);
	static void snake(game* g, int x, int y);
};

struct trapfuncm {
	static void bubble(game* g, monster* z);
	static void beartrap(game* g, monster* z);
	static void board(game* g, monster* z);
	static void tripwire(game* g, monster* z);
	static void crossbow(game* g, monster* z);
	static void shotgun(game* g, monster* z);
	static void blade(game* g, monster* z);
	static void snare(game* g, monster* z) { }; // \todo implement
	static void landmine(game* g, monster* z);
	static void telepad(game* g, monster* z);
	static void goo(game* g, monster* z);
	static void dissector(game* g, monster* z);
	static void sinkhole(game* g, monster* z) { };
	static void pit(game* g, monster* z);
	static void pit_spikes(game* g, monster* z);
	static void lava(game* g, monster* z);
	static void portal(game* g, monster* z) { }; // \todo major game feature
	static void ledge(game* g, monster* z);
	static void boobytrap(game* g, monster* z);
	static void glow(game* g, monster* z);
	static void hum(game* g, monster* z);
	static void drain(game* g, monster* z);
	static void snake(game* g, monster* z);
};

#endif
