#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <string>

enum option_key {
OPT_NULL = 0,
OPT_FORCE_YN, // Y/N versus y/n
OPT_USE_CELSIUS, // Display temp as C not F
OPT_USE_METRIC_SYS, // Display speed as Km/h not mph
OPT_NO_CBLINK, // No bright backgrounds
OPT_24_HOUR, // 24 hour time
OPT_SNAP_TO_TARGET, // game::firing snaps to target
OPT_SAFEMODE, // Safemode on by default?
OPT_AUTOSAFEMODE, // Autosafemode on by default?
NUM_OPTION_KEYS
};

// Historically, this was intended to be a singleton.  Enforce this.
class option_table
{
private:
 double options[NUM_OPTION_KEYS];

 option_table();
 option_table(const option_table& src) = delete;
 option_table(option_table&& src) = delete;
 ~option_table() = default;
 option_table& operator=(const option_table& src) = delete;

public:
 double& operator[] (option_key i) { return options[i]; };
 double& operator[] (int i) { return options[i]; };

 static option_table& get();
private:
 void load();
};

void save_options();
std::string option_name(option_key key);

#endif
