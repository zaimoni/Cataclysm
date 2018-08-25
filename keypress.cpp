#include "keypress.h"
#include "wrap_curses.h"

keymap<action_id> keys;

long input()
{
 long ch = getch();
 switch (ch) {
  case KEY_UP:    return 'k';
  case KEY_LEFT:  return 'h';
  case KEY_RIGHT: return 'l';
  case KEY_DOWN:  return 'j';
  case 459: return '\n';
  default:  return ch;
 }
}

void get_direction(int &x, int &y, char ch)
{
 x = 0;
 y = 0;
 switch (ch) {
 case 'y':
  x = -1;
  y = -1;
  return;
 case 'u':
  x = 1;
  y = -1;
  return;
 case 'h':
  x = -1;
  return;
 case 'j':
  y = 1;
  return;
 case 'k':
  y = -1;
  return;
 case 'l':
  x = 1;
  return;
 case 'b':
  x = -1;
  y = 1;
  return;
 case 'n':
  x = 1;
  y = 1;
  return;
 case '.':
 case ',':
 case 'g':
  x = 0;
  y = 0;
  return;
 default:
  x = -2;
  y = -2;
 }
}

point get_direction(char ch)
{
 switch (keys.translate(ch)) {
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
