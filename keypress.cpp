#include "keypress.h"
#include "wrap_curses.h"

keymap<action_id> keys;

/// <summary>mostly translates arrow keys to vi-keys</summary>
int input()
{
 int ch = getch();
 switch (ch) {
  case KEY_UP:    return 'k';
  case KEY_LEFT:  return 'h';
  case KEY_RIGHT: return 'l';
  case KEY_DOWN:  return 'j';
  case 459: return '\n';
  default:  return ch;
 }
}

point get_direction(int ch)
{
 switch (keys.translate(char(ch))) {
 case ACTION_MOVE_NW: return point(-1, -1);
 case ACTION_MOVE_NE: return point(1, -1);
 case ACTION_MOVE_W: return point(-1, 0);
 case ACTION_MOVE_S: return point(0, 1);
 case ACTION_MOVE_N: return point(0, -1);
 case ACTION_MOVE_E: return point(1, 0);
 case ACTION_MOVE_SW: return point(-1, 1);
 case ACTION_MOVE_SE: return point(1, 1);
 case ACTION_PAUSE:
 case ACTION_PICKUP: return point(0, 0);
 default: return point(-2, -2);
 }
}
