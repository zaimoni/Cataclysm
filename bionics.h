#ifndef _BIONICS_H_
#define _BIONICS_H_

#include "bionics_enum.h"
#include "enum_json.h"
#include <string>
#include <iosfwd>

DECLARE_JSON_ENUM_SUPPORT(bionic_id)

struct bionic_data {
 std::string name;
 bool power_source;
 bool activated;	// If true, then the below function only happens when
			// activated; otherwise, it happens every turn
 int power_cost;
 int charge_time;	// How long, when activated, between drawing power_cost
			// If 0, it draws power once
 std::string description;
};

struct bionic {
 static const bionic_data type[];

 bionic_id id;
 char invlet;
 bool powered;
 int charge;

 bionic() : id(bio_batteries), invlet('a'), powered(false), charge(0) {};
 bionic(bionic_id pid, char pinvlet) : id(pid), invlet(pinvlet), powered(false), charge(0) {};
};
 
#endif
