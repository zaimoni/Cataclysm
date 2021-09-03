#ifndef _MATERIAL_ENUM_H_
#define _MATERIAL_ENUM_H_

enum material {
MNULL = 0,
//Food Materials
LIQUID, VEGGY, FLESH, POWDER, // LIQUID is really water, or close to it
//Clothing
COTTON, WOOL, LEATHER, KEVLAR,
//Other
STONE, PAPER, WOOD, PLASTIC, GLASS, IRON, STEEL, SILVER
};

// doesn't really belong here -- Socrates' Daimon requirement 2020-03-09
// default assignment operator is often not ACID (requires utility header)
#define DEFINE_ACID_ASSIGN_W_SWAP(TYPE)	\
TYPE& TYPE::operator=(const TYPE& src)	\
{	\
	TYPE tmp(src);	\
	std::swap(*this, tmp);	\
	return *this;	\
}

#define DEFINE_ACID_ASSIGN_W_MOVE(TYPE)	\
TYPE& TYPE::operator=(const TYPE& src)	\
{	\
	TYPE tmp(src);	\
	return *this = std::move(tmp);	\
}

#endif
