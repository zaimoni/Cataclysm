#ifndef _WEATHER_H_
#define _WEATHER_H_

#include "color.h"
#include "enum_json.h"
#include <string>

class game;

namespace cataclysm {
	class JSON;
}

enum weather_type {
 WEATHER_NULL,		// For data and stuff
 WEATHER_CLEAR,		// No effects
 WEATHER_SUNNY,		// Glare if no eye protection
 WEATHER_CLOUDY,	// No effects
 WEATHER_DRIZZLE,	// Light rain
 WEATHER_RAINY,		// Lots of rain, sight penalties
 WEATHER_THUNDER,	// Warns of lightning to come
 WEATHER_LIGHTNING,	// Rare lightning strikes!
 WEATHER_ACID_DRIZZLE,	// No real effects; warning of acid rain
 WEATHER_ACID_RAIN,	// Minor acid damage
 WEATHER_FLURRIES,	// Light snow
 WEATHER_SNOW,		// Medium snow
 WEATHER_SNOWSTORM,	// Heavy snow
 NUM_WEATHER_TYPES
};

DECLARE_JSON_ENUM_SUPPORT(weather_type);
bool fromJSON(const cataclysm::JSON& src, weather_type& dest);

struct weather_datum
{
 static const weather_datum data[NUM_WEATHER_TYPES];

 std::string name;
 nc_color color;
 int avg_temperature[4]; // Spring, Summer, Winter, Fall
 int ranged_penalty;
 int sight_penalty; // Penalty to max sight range
 int mintime, maxtime; // min/max time it lasts, in minutes
 bool dangerous; // If true, our activity gets interrupted
 void (*effect)(game *);
};

#endif // _WEATHER_H_
