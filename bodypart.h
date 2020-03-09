#ifndef _BODYPART_H_
#define _BODYPART_H_

#ifndef SOCRATES_DAIMON
#include <string>
#endif

enum body_part {
 bp_head = 0,
 bp_eyes,
 bp_mouth,
 bp_torso,
 bp_arms,
 bp_hands,
 bp_legs,
 bp_feet,
 num_bp
};

#ifndef SOCRATES_DAIMON
std::string body_part_name(body_part bp, int side);
std::string encumb_text(body_part bp);

body_part random_body_part();
#endif

#endif
