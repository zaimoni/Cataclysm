#ifndef _ENUMS_H_
#define _ENUMS_H_

enum material {
MNULL = 0,
//Food Materials
LIQUID, VEGGY, FLESH, POWDER,
//Clothing
COTTON, WOOL, LEATHER, KEVLAR,
//Other
STONE, PAPER, WOOD, PLASTIC, GLASS, IRON, STEEL, SILVER
};

struct point {
 int x;
 int y;
 point(int X = 0, int Y = 0) : x (X), y (Y) {}
 point(const point &p) = default;
};

struct tripoint {
 int x;
 int y;
 int z;
 tripoint(int X = 0, int Y = 0, int Z = 0) : x (X), y (Y), z (Z) {}
 tripoint(const tripoint &p) = default;
};

#endif
