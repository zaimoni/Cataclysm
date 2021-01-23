#ifndef CALENDAR_H
#define CALENDAR_H

#include <string>

enum season_type {
	SPRING = 0,
	SUMMER = 1,
	AUTUMN = 2,
	WINTER = 3
};

// How many minutes exist when the game starts - 8:00 AM
constexpr int STARTING_MINUTES = 480;
// The number of days in a season - this also defines moon cycles
constexpr int DAYS_IN_SEASON = 14;

// Convert minutes, hours, days to turns
constexpr int MINUTES(int x) { return x * 10; }
constexpr int HOURS(int x)   { return x * 600; }
constexpr int DAYS(int x)    { return x * 14400; }
// forward-compatibility
constexpr int TURNS(int x)   { return x; }

// so we have some idea of the time scaling issues
// Ignore difference between Julian Calendar and Gregorian Calendar
constexpr int PREAPOCALYPSE_HOURS_IN_YEAR = 365*24 + 6;
// Ignore multiple astronomical technicalities, for now
constexpr int PREAPOCALYPSE_MINUTES_IN_LUNAR_CYCLE = (29*24+12)*60 + 44;

// Times for sunrise, sunset at equinoxes
constexpr int SUNRISE_WINTER   = 7;
constexpr int SUNRISE_SOLSTICE = 6;
constexpr int SUNRISE_SUMMER   = 5;

constexpr int SUNSET_WINTER   = 17;
constexpr int SUNSET_SOLSTICE = 19;
constexpr int SUNSET_SUMMER   = 21;

// How long, in minutes, does sunrise/sunset last?
constexpr int TWILIGHT_MINUTES = 60;

// How much light moon provides--double for full moon
constexpr int MOONLIGHT_LEVEL = 4;
// How much light is provided in full daylight
constexpr int DAYLIGHT_LEVEL = 60;

enum moon_phase {
	MOON_NEW = 0,
	MOON_HALF,
	MOON_FULL
};

class calendar
{
 public:
  static constexpr const int mp_turn = 100; // move points/standard turn

// The basic data; note that "second" should always be a multiple of 6
  int second;
  int minute;
  int hour;
  int day;
  season_type season;
  int year;
// End data

  calendar();
  calendar(const calendar& copy) = default;
  calendar(int Minute, int Hour, int Day, season_type Season, int Year);
  calendar(int turn);	// this constructor requires the others to exist

  int get_turn() const;
  operator int() const { return get_turn(); } // for backwards compatibility
  calendar& operator = (const calendar& rhs) = default;
  calendar& operator = (int rhs);
  calendar& operator -=(const calendar& rhs);
  calendar& operator -=(int rhs);
  calendar& operator +=(const calendar& rhs);
  calendar& operator +=(int rhs);
  calendar  operator - (const calendar& rhs) const { return calendar(*this) -= rhs; };
  calendar  operator - (int rhs) const { return calendar(*this) -= rhs; };
  calendar  operator + (const calendar& rhs) const { return calendar(*this) += rhs; };
  calendar  operator + (int rhs) const { return calendar(*this) += rhs; };

  void increment();   // Add one turn / 6 seconds

  void standardize(); // Ensure minutes <= 59, hour <= 23, etc.

// Sunlight and day/night calculations
  int minutes_past_midnight() const; // Useful for sunrise/set calculations
  moon_phase moon() const;  // Find phase of moon
  calendar sunrise() const; // Current time of sunrise
  calendar sunset() const;  // Current time of sunset
  bool is_night() const;    // After sunset + TWILIGHT_MINUTES, before sunrise
  int sunlight() const;     // Current amount of sun/moonlight; uses preceding funcs

// Print-friendly stuff
  std::string print_time() const;
  std::string textify_period(); // "1 second" "2 hours" "two days"
};

#endif
