#ifndef _BODYPART_H_
#define _BODYPART_H_

#include "bodypart_enum.h"

#include <string>

std::string body_part_name(body_part bp, int side);
const char* encumb_text(body_part bp);

body_part random_body_part();

#endif
