#ifndef KEYPRESS_H
#define KEYPRESS_H

#ifdef SOCRATES_DAIMON
#error "not used for socrates-daimon"
#endif

#include "enums.h"
#include "action.h"
#include "keymap.h"

// Simple text input--translates numpad to vikeys
int input();

// Uses the keymap to figure out direction properly
point get_direction(int ch);

extern keymap<action_id> keys;

#endif
