#include "calendar.h"
#include "options.h"
#include <sstream>

calendar::calendar()
{
 second = 0;
 minute = 0;
 hour = 0;
 day = 0;
 season = SPRING;
 year = 0;
}

calendar::calendar(int Minute, int Hour, int Day, season_type Season, int Year)
{
 second = 0;
 minute = Minute;
 hour = Hour;
 day = Day;
 season = Season;
 year = Year;
}

calendar::calendar(int turn)
{
 int minutes = int(turn / 10);
 int hours = minutes / 60;
 int days = hours / 24;
 int seasons = days / DAYS_IN_SEASON;
 second = 6 * (turn % 10);
 minute = minutes % 60;
 hour = hours % 24;
 day = 1 + days % DAYS_IN_SEASON;
 season = season_type(seasons % 4);
 year = seasons / 4;
}

int calendar::get_turn() const
{
 int ret = second / 6;
 ret += minute * 10;
 ret += hour * 600;
 ret += day * 14400;
 ret += int(season) * 14400 * DAYS_IN_SEASON;
 ret += year * 14400 * 4 * DAYS_IN_SEASON;
 return ret;
}

calendar& calendar::operator =(int rhs)
{
 int minutes = int(rhs / 10);
 int hours = minutes / 60;
 int days = hours / 24;
 int seasons = days / DAYS_IN_SEASON;
 second = 6 * (rhs % 10);
 minute = minutes % 60;
 hour = hours % 24;
 day = days % DAYS_IN_SEASON;
 season = season_type(seasons % 4);
 year = seasons / 4;
 return *this;
}

calendar& calendar::operator -=(const calendar &rhs)
{
 calendar tmp(rhs);
 tmp.standardize();
 second -= tmp.second;
 minute -= tmp.minute;
 hour   -= tmp.hour;
 day    -= tmp.day;
 int tmpseason = int(season) - int(tmp.season);
 while (tmpseason < 0) {
  year--;
  tmpseason += 4;
 }
 season = season_type(tmpseason);
 year -= tmp.year;
 standardize();
 return *this;
}

calendar& calendar::operator -=(int rhs)
{
 calendar tmp(rhs);
 *this -= tmp;
 return *this;
}

calendar& calendar::operator +=(const calendar &rhs)
{
 second += rhs.second;
 minute += rhs.minute;
 hour   += rhs.hour;
 day    += rhs.day;
 int tmpseason = int(season) + int(rhs.season);
 while (tmpseason >= 4) {
  year++;
  tmpseason -= 4;
 }
 season = season_type(tmpseason);
 year += rhs.year;
 standardize();
 return *this;
}

calendar& calendar::operator +=(int rhs)
{
 second += rhs * 6;
 standardize();
 return *this;
}

void calendar::increment()
{
 second += 6;
 if (second >= 60)
  standardize();
}

void calendar::standardize()
{
 if (second >= 60) {
  minute += second / 60;
  second %= 60;
 }
 if (minute >= 60) {
  hour += minute / 60;
  minute %= 60;
 }
 if (hour >= 24) {
  day += hour / 24;
  hour %= 24;
 }
 int tmpseason = int(season);
 if (day >= DAYS_IN_SEASON) {
  tmpseason += day / DAYS_IN_SEASON;
  day %= DAYS_IN_SEASON;
 }
 if (tmpseason >= 4) {
  year += tmpseason / 4;
  tmpseason %= 4;
 }
 season = season_type(tmpseason);
}

int calendar::minutes_past_midnight() const
{
 int ret = minute + hour * 60;
 return ret;
}

moon_phase calendar::moon() const
{
 // strict upper bound on phase value should be 2*FULL_MOON (otherwise we double count new moon)
 // if we included waxing/waning in phases (reasonable), bound would be different
 const int phase = (day * (2*MOON_FULL))/ DAYS_IN_SEASON;
 //phase %= 4;   // currently redundant, but won't be once we go physical
 return (MOON_FULL >= phase) ? moon_phase(phase) : moon_phase(2*MOON_FULL - phase);
}

// return value is last minute which is at just moonlight level
calendar calendar::sunrise() const
{
 int start_hour = 0, end_hour = 0;
 switch (season) {
  case SPRING:
   start_hour = SUNRISE_SOLSTICE;
   end_hour   = SUNRISE_SUMMER;
   break;
  case SUMMER:
   start_hour = SUNRISE_SUMMER;
   end_hour   = SUNRISE_SOLSTICE;
   break;
  case AUTUMN:
   start_hour = SUNRISE_SOLSTICE;
   end_hour   = SUNRISE_WINTER;
   break;
  case WINTER:
   start_hour = SUNRISE_WINTER;
   end_hour   = SUNRISE_SOLSTICE;
   break;
 }
 double percent = double(double(day) / DAYS_IN_SEASON);
 double time = double(start_hour) * (1.- percent) + double(end_hour) * percent;

 calendar ret;
 ret.hour = int(time);
 time -= int(time);
 ret.minute = int(time * 60);

 return ret;
}

// return value is last minute which is at just sunlight level
calendar calendar::sunset() const
{
 int start_hour = 0, end_hour = 0;
 switch (season) {
  case SPRING:
   start_hour = SUNSET_SOLSTICE;
   end_hour   = SUNSET_SUMMER;
   break;
  case SUMMER:
   start_hour = SUNSET_SUMMER;
   end_hour   = SUNSET_SOLSTICE;
   break;
  case AUTUMN:
   start_hour = SUNSET_SOLSTICE;
   end_hour   = SUNSET_WINTER;
   break;
  case WINTER:
   start_hour = SUNSET_WINTER;
   end_hour   = SUNSET_SOLSTICE;
   break;
 }
 double percent = double(double(day) / DAYS_IN_SEASON);
 double time = double(start_hour) * (1.- percent) + double(end_hour) * percent;

 calendar ret;
 ret.hour = int(time);
 time -= int(time);
 ret.minute = int(time * 60);

 return ret;
}

bool calendar::is_night() const
{
 calendar sunrise_time = sunrise(), sunset_time = sunset();

 int mins         = minutes_past_midnight(),
     sunrise_mins = sunrise_time.minutes_past_midnight(),
     sunset_mins  = sunset_time.minutes_past_midnight();

 return (mins > sunset_mins + TWILIGHT_MINUTES || mins < sunrise_mins);
}

int calendar::sunlight() const
{
 int mins = minutes_past_midnight();
 int sunrise_mins = sunrise().minutes_past_midnight();
 int sunset_mins = sunset().minutes_past_midnight();

 int moonlight = 1 + moon() * MOONLIGHT_LEVEL;

 if (mins >= sunset_mins + TWILIGHT_MINUTES || mins <= sunrise_mins) // Night
  return moonlight;

 else if (mins > sunrise_mins && mins < sunrise_mins + TWILIGHT_MINUTES) {

  double percent = double(mins - sunrise_mins) / TWILIGHT_MINUTES;
  return int(moonlight * (1. - percent) + DAYLIGHT_LEVEL * percent);

 } else if (mins > sunset_mins && mins < sunset_mins + TWILIGHT_MINUTES) {

  double percent = double(mins - sunset_mins) / TWILIGHT_MINUTES;
  return int(DAYLIGHT_LEVEL * (1. - percent) + moonlight * percent);

 } else
  return DAYLIGHT_LEVEL;
}

std::string calendar::print_time() const
{
 std::ostringstream ret;
 const bool military_time = option_table::get()[OPT_24_HOUR];

 if (military_time) {
   int hours = hour % 24;
   if (hours < 10) ret << "0";
   ret << hours;
 } else {
   int hours = hour % 12;
   if (hours == 0) hours = 12;
   ret << hours << ":";
 }
 if (minute < 10) ret << "0";
 ret << minute;
 if (!military_time) ret << ((12 > hour) ? " AM" : " PM");

 return ret.str();
}

std::string calendar::textify_period()
{
 standardize();
 int am;
 std::string tx;
// Describe the biggest time period, as "<am> <tx>s", am = amount, tx = name
 if (year > 0) {
  am = year;
  tx = "year";
 } else if (season > 0) {
  am = season;
  tx = "season";
 } else if (day > 0) {
  am = day;
  tx = "day";
 } else if (hour > 0) {
  am = hour;
  tx = "hour";
 } else if (minute >= 5) {
  am = minute;
  tx = "minute";
 } else {
  am = second / 6 + minute * 10;
  tx = "turn";
 }

 std::ostringstream ret;
 ret << am << " " << tx << (am > 1 ? "s" : "");

 return ret.str();
}
