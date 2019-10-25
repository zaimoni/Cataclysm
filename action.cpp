#include "game.h"
#include "keypress.h"
#include "ios_file.h"
#include <fstream>

const char* const  default_keymap_txt = "\
# This is the keymapping for Cataclysm.\n\
# You can start a line with # to make it a comment--it will be ignored.\n\
# Blank lines are ignored too.\n\
# Extra whitespace, including tab, is ignored, so format things how you like.\n\
# If you wish to restore defaults, simply remove this file.\n\
\n\
# The format for each line is an action identifier, followed by several\n\
# keys.  Any action may have an unlimited number of keys bound to it.\n\
# If you bind the same key to multiple actions, the second and subsequent\n\
# bindings will be ignored--and you'll get a warning when the game starts.\n\
# Keys are case-sensitive, of course; c and C are different.\n\
 \n\
# WARNING: If you skip an action identifier, there will be no key bound to\n\
# that action!  You will be NOT be warned of this when the game starts.\n\
# If you're going to mess with stuff, maybe you should back this file up?\n\
\n\
# It is okay to split commands across lines.\n\
# pause . 5      is equivalent to:\n\
# pause .\n\
# pause 5\n\
\n\
# Note that movement keybindings ONLY apply to movement (for now).\n\
# That is, binding w to move_n will let you use w to move north, but you\n\
# cannot use w to smash north, examine to the north, etc.\n\
# For now, you must use vikeys, the numpad, or arrow keys for those actions.\n\
# This is planned to change in the future.\n\
\n\
# Finally, there is no support for special keys, like spacebar, Home, and\n\
# so on.  This is not a planned feature, but if it's important to you, please\n\
# let me know.\n\
\n\
# MOVEMENT:\n\
pause     . 5\n\
move_n    k 8\n\
move_ne   u 9\n\
move_e    l 6\n\
move_se   n 3\n\
move_s    j 2\n\
move_sw   b 1\n\
move_w    h 4\n\
move_nw   y 7\n\
move_down >\n\
move_up   <\n\
\n\
# ENVIRONMENT INTERACTION\n\
open  o\n\
close c\n\
smash s\n\
examine e\n\
pickup , g\n\
butcher B\n\
chat C\n\
look ; x\n\
\n\
# INVENTORY & QUASI-INVENTORY INTERACTION\n\
inventory i\n\
organize =\n\
apply a\n\
wear W\n\
take_off T\n\
eat E\n\
read R\n\
wield w\n\
pick_style _\n\
reload r\n\
unload U\n\
throw t\n\
fire f\n\
fire_burst F\n\
drop d\n\
drop_adj D\n\
bionics p\n\
\n\
# LONG TERM & SPECIAL ACTIONS\n\
wait ^\n\
craft &\n\
construct *\n\
sleep $\n\
safemode !\n\
autosafe \"\n\
ignore_enemy '\n\
save S\n\
quit Q\n\
\n\
# INFO SCREENS\n\
player_data @\n\
map m :\n\
missions M\n\
factions #\n\
morale %\n\
messages P\n\
help ?\n\
\n\
# DEBUG FUNCTIONS\n\
debug_mode ~\n\
# debug Z\n\
# debug_scent -\n\
";

#define KEY_FILE "data/keymap.txt"
void game::load_keyboard_settings()
{
 std::ifstream fin(KEY_FILE);
 if (!fin) { // It doesn't exist
  std::ofstream fout(KEY_FILE);
  fout << default_keymap_txt;
  fout.close();
  fin.open(KEY_FILE);
 }
 if (!fin) { // Still can't open it--probably bad permissions
  debugmsg("Can't open data/keymap.txt.  This may be a permissions issue.");
  return;
 }
 while (!fin.eof()) {
  std::string id;
  fin >> id;
  if (id == "")
   getline(fin, id); // Empty line, chomp it
  else if (id[0] != '#') {
   action_id act = look_up_action(id);
   if (act == ACTION_NULL)
    debugmsg("\
Warning!  data/keymap.txt contains an unknown action, \"%s\"\n\
Fix data/keymap.txt at your next chance!", id.c_str());
   else {
    while (fin.peek() != '\n' && !fin.eof()) {
     char ch;
     fin >> ch;
	 if (!keys.set(ch,act))
      debugmsg("\
Warning!  '%c' assigned twice in the keymap!\n\
%s is being ignored.\n\
Fix data/keymap.txt at your next chance!", ch, id.c_str());
    }
   }
  } else {
   getline(fin, id); // Clear the whole line
  }
 }
}

void game::save_keymap()
{
 DECLARE_AND_ACID_OPEN(std::ofstream, fout, KEY_FILE, return;)
 keys.forall([&fout](char key, action_id act) { fout << action_ident(act) << " " << key << std::endl; });
 OFSTREAM_ACID_CLOSE(fout, KEY_FILE)
}
#undef KEY_FILE

std::string action_ident(action_id act)
{
 switch (act) {
  case ACTION_PAUSE:
   return "pause";
  case ACTION_MOVE_N:
   return "move_n";
  case ACTION_MOVE_NE:
   return "move_ne";
  case ACTION_MOVE_E:
   return "move_e";
  case ACTION_MOVE_SE:
   return "move_se";
  case ACTION_MOVE_S:
   return "move_s";
  case ACTION_MOVE_SW:
   return "move_sw";
  case ACTION_MOVE_W:
   return "move_w";
  case ACTION_MOVE_NW:
   return "move_nw";
  case ACTION_MOVE_DOWN:
   return "move_down";
  case ACTION_MOVE_UP:
   return "move_up";
  case ACTION_OPEN:
   return "open";
  case ACTION_CLOSE:
   return "close";
  case ACTION_SMASH:
   return "smash";
  case ACTION_EXAMINE:
   return "examine";
  case ACTION_PICKUP:
   return "pickup";
  case ACTION_BUTCHER:
   return "butcher";
  case ACTION_CHAT:
   return "chat";
  case ACTION_LOOK:
   return "look";
  case ACTION_INVENTORY:
   return "inventory";
  case ACTION_ORGANIZE:
   return "organize";
  case ACTION_USE:
   return "apply";
  case ACTION_WEAR:
   return "wear";
  case ACTION_TAKE_OFF:
   return "take_off";
  case ACTION_EAT:
   return "eat";
  case ACTION_READ:
   return "read";
  case ACTION_WIELD:
   return "wield";
  case ACTION_PICK_STYLE:
   return "pick_style";
  case ACTION_RELOAD:
   return "reload";
  case ACTION_UNLOAD:
   return "unload";
  case ACTION_THROW:
   return "throw";
  case ACTION_FIRE:
   return "fire";
  case ACTION_FIRE_BURST:
   return "fire_burst";
  case ACTION_DROP:
   return "drop";
  case ACTION_DIR_DROP:
   return "drop_adj";
  case ACTION_BIONICS:
   return "bionics";
  case ACTION_WAIT:
   return "wait";
  case ACTION_CRAFT:
   return "craft";
  case ACTION_CONSTRUCT:
   return "construct";
  case ACTION_SLEEP:
   return "sleep";
  case ACTION_TOGGLE_SAFEMODE:
   return "safemode";
  case ACTION_TOGGLE_AUTOSAFE:
   return "autosafe";
  case ACTION_IGNORE_ENEMY:
   return "ignore_enemy";
  case ACTION_SAVE:
   return "save";
  case ACTION_QUIT:
   return "quit";
  case ACTION_PL_INFO:
   return "player_data";
  case ACTION_MAP:
   return "map";
  case ACTION_MISSIONS:
   return "missions";
  case ACTION_FACTIONS:
   return "factions";
  case ACTION_MORALE:
   return "morale";
  case ACTION_MESSAGES:
   return "messages";
  case ACTION_HELP:
   return "help";
  case ACTION_DEBUG:
   return "debug";
  case ACTION_DISPLAY_SCENT:
   return "debug_scent";
  case ACTION_TOGGLE_DEBUGMON:
   return "debug_mode";
  case ACTION_NULL:
   return "null";
 }
 return "unknown";
}

action_id look_up_action(std::string ident)
{
 for (int i = 0; i < NUM_ACTIONS; i++) {
  if (action_ident( action_id(i) ) == ident)
   return action_id(i);
 }
 return ACTION_NULL;
}

std::string action_name(action_id act)
{
 switch (act) {
  case ACTION_PAUSE:
   return "Pause";
  case ACTION_MOVE_N:
   return "Move North";
  case ACTION_MOVE_NE:
   return "Move Northeast";
  case ACTION_MOVE_E:
   return "Move East";
  case ACTION_MOVE_SE:
   return "Move Southeast";
  case ACTION_MOVE_S:
   return "Move South";
  case ACTION_MOVE_SW:
   return "Move Southwest";
  case ACTION_MOVE_W:
   return "Move West";
  case ACTION_MOVE_NW:
   return "Move Northwest";
  case ACTION_MOVE_DOWN:
   return "Descend Stairs";
  case ACTION_MOVE_UP:
   return "Ascend Stairs";
  case ACTION_OPEN:
   return "Open Door";
  case ACTION_CLOSE:
   return "Close Door";
  case ACTION_SMASH:
   return "Smash Nearby Terrain";
  case ACTION_EXAMINE:
   return "Examine Nearby Terrain";
  case ACTION_PICKUP:
   return "Pick Item(s) Up";
  case ACTION_BUTCHER:
   return "Butcher";
  case ACTION_CHAT:
   return "Chat with NPC";
  case ACTION_LOOK:
   return "Look Around";
  case ACTION_INVENTORY:
   return "Open Inventory";
  case ACTION_ORGANIZE:
   return "Swap Inventory Letters";
  case ACTION_USE:
   return "Apply or Use Item";
  case ACTION_WEAR:
   return "Wear Item";
  case ACTION_TAKE_OFF:
   return "Take Off Worn Item";
  case ACTION_EAT:
   return "Eat";
  case ACTION_READ:
   return "Read";
  case ACTION_WIELD:
   return "Wield";
  case ACTION_PICK_STYLE:
   return "Select Unarmed Style";
  case ACTION_RELOAD:
   return "Reload Wielded Item";
  case ACTION_UNLOAD:
   return "Unload or Empty Wielded Item";
  case ACTION_THROW:
   return "Throw Item";
  case ACTION_FIRE:
   return "Fire Wielded Item";
  case ACTION_FIRE_BURST:
   return "Burst-Fire Wielded Item";
  case ACTION_DROP:
   return "Drop Item";
  case ACTION_DIR_DROP:
   return "Drop Item to Adjacent Tile";
  case ACTION_BIONICS:
   return "View/Activate Bionics";
  case ACTION_WAIT:
   return "Wait for Several Minutes";
  case ACTION_CRAFT:
   return "Craft Items";
  case ACTION_CONSTRUCT:
   return "Construct Terrain";
  case ACTION_SLEEP:
   return "Sleep";
  case ACTION_TOGGLE_SAFEMODE:
   return "Toggle Safemode";
  case ACTION_TOGGLE_AUTOSAFE:
   return "Toggle Auto-Safemode";
  case ACTION_IGNORE_ENEMY:
   return "Ignore Nearby Enemy";
  case ACTION_SAVE:
   return "Save and Quit";
  case ACTION_QUIT:
   return "Commit Suicide";
  case ACTION_PL_INFO:
   return "View Player Info";
  case ACTION_MAP:
   return "View Map";
  case ACTION_MISSIONS:
   return "View Missions";
  case ACTION_FACTIONS:
   return "View Factions";
  case ACTION_MORALE:
   return "View Morale";
  case ACTION_MESSAGES:
   return "View Message Log";
  case ACTION_HELP:
   return "View Help";
  case ACTION_DEBUG:
   return "Debug Menu";
  case ACTION_DISPLAY_SCENT:
   return "View Scentmap";
  case ACTION_TOGGLE_DEBUGMON:
   return "Toggle Debug Messages";
  case ACTION_NULL:
   return "No Action";
 }
 return "Someone forgot to name an action.";
}
