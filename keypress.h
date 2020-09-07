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
// If ch is vikey, x & y are set to corresponding direction; ch=='y'->x=-1,y=-1
void get_direction(int &x, int &y, int ch);
// Uses the keymap to figure out direction properly
point get_direction(int ch);

extern keymap<action_id> keys;

#endif
