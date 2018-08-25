#include "keypress.h"
#include "game.h"

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

void get_direction(game *g, int &x, int &y, char ch)
{
 x = 0;
 y = 0;
 const action_id act = keys.translate(ch);

 switch (act) {
 case ACTION_MOVE_NW:
  x = -1;
  y = -1;
  return;
 case ACTION_MOVE_NE:
  x = 1;
  y = -1;
  return;
 case ACTION_MOVE_W:
  x = -1;
  return;
 case ACTION_MOVE_S:
  y = 1;
  return;
 case ACTION_MOVE_N:
  y = -1;
  return;
 case ACTION_MOVE_E:
  x = 1;
  return;
 case ACTION_MOVE_SW:
  x = -1;
  y = 1;
  return;
 case ACTION_MOVE_SE:
  x = 1;
  y = 1;
  return;
 case ACTION_PAUSE:
 case ACTION_PICKUP:
  x = 0;
  y = 0;
  return;
 default:
  x = -2;
  y = -2;
 }
}
