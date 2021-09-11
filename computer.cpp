#include "computer.h"
#include "game.h"
#include "monster.h"
#include "overmap.h"
#include "output.h"
#include "recent_msg.h"
#include "om_cache.hpp"
#include "line.h"
#include "rng.h"

#include <fstream>
#include <utility>
#include <cstdarg>
#include <stdexcept>

// JSON enum transcoding
static const char* JSON_transcode_compact[] = {
	"OPEN",
	"SAMPLE",
	"RELEASE",
	"TERMINATE",
	"PORTAL",
	"CASCADE",
	"RESEARCH",
	"MAPS",
	"MAP_SEWER",
	"MISS_LAUNCH",
	"MISS_DISARM",
	"LIST_BIONICS",
	"ELEVATOR_ON",
	"AMIGARA_LOG",
	"AMIGARA_START",
	"DOWNLOAD_SOFTWARE",
	"BLOOD_ANAL"
};

static const char* JSON_transcode_compfail[] = {
	"SHUTDOWN",
	"ALARM",
	"MANHACKS",
	"SECUBOTS",
	"DAMAGE",
	"PUMP_EXPLODE",
	"PUMP_LEAK",
	"AMIGARA",
	"DESTROY_BLOOD"
};

DEFINE_JSON_ENUM_SUPPORT_HARDCODED_NONZERO(computer_action, JSON_transcode_compact)
DEFINE_JSON_ENUM_SUPPORT_HARDCODED_NONZERO(computer_failure, JSON_transcode_compfail)

// main implementation
computer::computer(const computer& src)
: options(src.options),failures(src.failures),security(src.security),w_terminal(nullptr),name(src.name),mission_id(src.mission_id)
{
}

computer::computer(computer&& src)
: options(std::move(src.options)), failures(std::move(src.failures)), security(src.security), w_terminal(src.w_terminal),
  name(std::move(src.name)), mission_id(src.mission_id)
{
	src.w_terminal = nullptr;
}

computer::~computer()
{
 if (w_terminal != nullptr)
  delwin(w_terminal);
}

DEFINE_ACID_ASSIGN_W_MOVE(computer)

computer& computer::operator=(computer&& src)
{
	options = std::move(src.options);
	failures = std::move(src.failures);
	security = src.security;
	w_terminal = src.w_terminal;
	name = std::move(src.name);
	mission_id = src.mission_id;
	src.w_terminal = nullptr;
	return *this;
}

void computer::add_option(std::string opt_name, computer_action action,
                          int Security)
{
 options.push_back(computer_option(opt_name, action, Security));
}

void computer::add_failure(computer_failure failure)
{
 failures.push_back(failure);
}

void computer::shutdown_terminal()
{
 werase(w_terminal);
 delwin(w_terminal);
 w_terminal = nullptr;
}

void computer::use(game *g)
{
 if (w_terminal == nullptr) w_terminal = newwin(VIEW, SCREEN_WIDTH, 0, 0);
 wborder(w_terminal, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                     LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 
 print_line("Logging into %s...", name.c_str());

 if (security > 0) {
  print_error("ERROR!  Access denied!");
  switch (query_ynq("Bypass security?")) {
  case 'q':
  case 'Q':
   shutdown_terminal();
   return;

  case 'n':
  case 'N':
   print_line("Shutting down... press any key.");
   getch();
   shutdown_terminal();
   return;

  case 'y':
  case 'Y':
   if (!hack_attempt(&(g->u))) {
    if (failures.size() == 0) {
     print_line("Maximum login attempts exceeded. Press any key...");
     getch();
     shutdown_terminal();
     return;
    }
    activate_random_failure(g);
    shutdown_terminal();
    return;
   } else { // Successful hack attempt
    security = 0;
    print_line("Login successful.  Press any key...");
    getch();
    reset_terminal();
   }
  }
 } else { // No security
  print_line("Login successful.  Press any key...");
  getch();
  reset_terminal();
 }

// Main computer loop

 bool done = false; // Are we done using the computer?
 do {
  //reset_terminal();
  print_line("");
  print_line("%s - Root Menu", name.c_str());
  for (int i = 0; i < options.size(); i++)
   print_line("%d - %s", i + 1, options[i].name.c_str());
  print_line("Q - Quit and shut down");
  print_line("");
 
  int ch;
  do
   ch = getch();
  while (ch != 'q' && ch != 'Q' && (ch < '1' || ch - '1' >= options.size()));
  if (ch == 'q' || ch == 'Q')
   done = true;
  else { // We selected an option other than quit.
   ch -= '1'; // So '1' -> 0; index in options.size()
   computer_option current = options[ch];
   if (current.security > 0) {
    print_error("Password required.");
    if (query_bool("Hack into system?")) {
     if (!hack_attempt(&(g->u), current.security)) {
      activate_random_failure(g);
      shutdown_terminal();
      return;
     } else {
      activate_function(g, current.action);
      reset_terminal();
     }
    }
   } else // No need to hack, just activate
    activate_function(g, current.action);
  } // Done processing a selected option.
 } while (!done); // Done with main terminal loop

 shutdown_terminal(); // This should have been done by now, but just in case.
}

bool computer::hack_attempt(player *p, int Security)
{
 if (Security == -1)
  Security = security; // Set to main system security if no value passed

 p->practice(sk_computer, 5 + Security * 2);
 int player_roll = p->sklevel[sk_computer];
 if (p->int_cur < 8 && one_in(2))
  player_roll -= rng(0, 8 - p->int_cur);
 else if (p->int_cur > 8 && one_in(3))
  player_roll += rng(0, p->int_cur - 8);

 return (dice(player_roll, 6) >= dice(Security, 6));
}

static const char* blood_analysis_precondition(const std::vector<item>& inv)
{
    if (inv.empty()) return "ERROR: Please place sample in centrifuge.";
    if (inv.size() > 1) return "ERROR: Please remove all but one sample from centrifuge.";
    if (inv[0].type->id != itm_vacutainer) return "ERROR: Please use vacutainer-contained samples.";
    if (inv[0].contents.empty()) return "ERROR: Vacutainer empty.";
    if (inv[0].contents[0].type->id != itm_blood) return "ERROR: Please only use blood samples.";
    return nullptr;
}

void computer::activate_function(game *g, computer_action action)
{
 switch (action) {

  case COMPACT_NULL:
   break; // Why would this be called?

  case COMPACT_OPEN:
   g->m.translate<t_door_metal_locked, t_floor>();  // fix \todo this opens *all doors in the map*; something more precise would be useful e.g. police station
   print_line("Doors opened.");
   break;

  case COMPACT_SAMPLE:
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
     if (g->m.ter(x, y) == t_sewage_pump) {
      for (int x1 = x - 1; x1 <= x + 1; x1++) {
       for (int y1 = y - 1; y1 <= y + 1; y1++ ) {
        if (g->m.ter(x1, y1) == t_counter) {
         bool found_item = false;
		 for(auto& it : g->m.i_at(x1, y1)) {
          if (it.is_container() && it.contents.empty()) {
           it.put_in( item(item::types[itm_sewage], messages.turn) );
           found_item = true;
          }
         }
         if (!found_item) g->m.add_item(x1, y1, item(item::types[itm_sewage], messages.turn));
        }
       }
      }
     }
    }
   }
   break;

  case COMPACT_RELEASE:
   g->sound(g->u.pos, 40, "An alarm sounds!");
   g->m.translate<t_reinforced_glass_h, t_floor>();
   g->m.translate<t_reinforced_glass_v, t_floor>();
   print_line("Containment shields opened.");
   break;

  case COMPACT_TERMINATE:
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
	 monster* const z = g->mon(x,y);
	 if (!z) continue;
	 const auto terrain = g->m.ter(x, y + 1);
	 switch(terrain)
	 {
	 case t_wall_h:
	 case t_reinforced_glass_h:
		 // abuse auto-conversion of enumerations to int and automatic type promotion
		 if ((t_wall_h + t_reinforced_glass_h)-terrain == g->m.ter(x, y - 1)) g->kill_mon(*z, &g->u);
	 }
    }
   }
   print_line("Subjects terminated.");
   break;

  case COMPACT_PORTAL:
   for (int i = 0; i < SEEX * MAPSIZE; i++) {
    for (int j = 0; j < SEEY * MAPSIZE; j++) {
     int numtowers = 0;
     for (int xt = i - 2; xt <= i + 2; xt++) {
      for (int yt = j - 2; yt <= j + 2; yt++) {
       if (g->m.ter(xt, yt) == t_radio_tower)
        numtowers++;
      }
     }
     if (numtowers == 4) {
      if (g->m.tr_at(i, j) == tr_portal)
       g->m.tr_at(i, j) = tr_null;
      else
       g->m.add_trap(i, j, tr_portal);
     }
    }
   }
   break;

  case COMPACT_CASCADE: {
   if (!query_bool("WARNING: Resonance cascade carries severe risk!  Continue?"))
    return;
   const zaimoni::gdi::box<point> scan(g->u.pos - point(10), g->u.pos + point(10));
   const std::vector<point> cascade_points(grep(scan, [&](point pt) { return t_radio_tower == g->m.ter(pt); }));
   if (const size_t ub = cascade_points.size()) {
       g->resonance_cascade(cascade_points[rng(0, ub - 1)]); // usually fizzles
   } else {
       g->resonance_cascade(g->u.pos);
   }
  } break;

  case COMPACT_RESEARCH: {
   int lines = 0, notes = 0;
   std::string log, tmp;
   int ch;
   std::ifstream fin;
   fin.open("data/LAB_NOTES");
   if (!fin.is_open()) {
    debugmsg("Couldn't open ./data/LAB_NOTES for reading");
    return;
   }
   while (fin.good()) {
    ch = fin.get();
    if (ch == '%')
     notes++;
   }
    
   while (lines < 10) {
    fin.clear();
    fin.seekg(0, std::ios::beg);
    fin.clear();
    int choice = rng(1, notes);
    while (choice > 0) {
     getline(fin, tmp);
     if (tmp.find_first_of('%') == 0)
      choice--;
    }
    getline(fin, tmp);
    do {
     lines++;
     if (lines < 15 && tmp.find_first_of('%') != 0) {
      log.append(tmp);
      log.append("\n");
     }
    } while(tmp.find_first_of('%') != 0 && getline(fin, tmp));
   }
   print_line(" %s", log.c_str());
   print_line("Press any key...");
   getch();
  } break;

  case COMPACT_MAPS: {
   const point anchor(g->om_location().second);
   int minx = anchor.x - 60;
   int maxx = anchor.x + 60;
   int miny = anchor.y - 60;
   int maxy = anchor.y + 60;

   OM_loc<2> scan(tripoint(g->cur_om.pos.x, g->cur_om.pos.y, 0), point(0, 0));
   for (scan.second.x = minx; scan.second.x <= maxx; scan.second.x++) {
       for (scan.second.y = miny; scan.second.y <= maxy; scan.second.y++) overmap::expose(scan);
   }

   print_line("Surface map data downloaded.");
  } break;

  case COMPACT_MAP_SEWER: {
   const point anchor(g->om_location().second);
   int minx = anchor.x - 60;
   int maxx = anchor.x + 60;
   int miny = anchor.y - 60;
   int maxy = anchor.y + 60;

   OM_loc<2> scan(tripoint(g->cur_om.pos.x, g->cur_om.pos.y, 0), point(0, 0));
   for (scan.second.x = minx; scan.second.x <= maxx; scan.second.x++) {
       for (scan.second.y = miny; scan.second.y <= maxy; scan.second.y++) {
           const auto terrain = overmap::ter_c(scan);
           if (   is_between<ot_sewer_ns, ot_sewer_nesw>(terrain)
               || is_between<ot_sewage_treatment, ot_sewage_treatment_under>(terrain))
               overmap::expose(scan);
       }
   }

   print_line("Sewage map data downloaded.");
  } break;


  case COMPACT_MISS_LAUNCH: {
   auto targ_om = om_cache::get().create(tripoint(g->cur_om.pos.x, g->cur_om.pos.y, 0));
// Target Acquisition.
   const auto target = targ_om.choose_point(g);
   if (!target) {
    print_line("Launch canceled.");
    return;
   }
// Figure out where the glass wall is...
   int wall_spot = 0;
   for (int i = g->u.pos.x; i < g->u.pos.x + SEEX * 2 && wall_spot == 0; i++) {
    if (g->m.ter(i, 10) == t_wall_glass_v) wall_spot = i;
   }
// ...and put radioactive to the right of it
   for (int i = wall_spot + 1; i < SEEX * 2 - 1; i++) {
    for (int j = 1; j < SEEY * 2 - 1; j++) {
     if (one_in(3)) g->m.add_field(nullptr, i, j, fd_nuke_gas, 3);
    }
   }
// For each level between here and the surface, remove the missile
   const tripoint revert_pos(g->cur_om.pos);
   for (int level = g->cur_om.pos.z; level < 0; level++) {
    om_cache::get().load(g->cur_om, tripoint(revert_pos.x, revert_pos.y, level));
    tinymap tmpmap;
    tmpmap.load(g, project_xy(g->lev));
    tmpmap.translate<t_missile, t_hole>();
    tmpmap.save(g->cur_om.pos, messages.turn, project_xy(g->lev));
   }
   om_cache::get().load(g->cur_om, revert_pos);
   for (int x = target->x - 2; x <= target->x + 2; x++) {
    for (int y = target->y -  2; y <= target->y + 2; y++)
     g->nuke(point(x, y));  // \todo should be re-specified to take a rectangle; also needs to be cross-overmap aware
   }
  } break;

  case COMPACT_MISS_DISARM: // TODO: This!
   break;

  case COMPACT_LIST_BIONICS: {
   std::vector<std::string> names;
   int more = 0;
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
	 for(const auto& it : g->m.i_at(x, y)) {
      if (it.is_bionic()) {
       if (names.size() < 9) names.push_back(it.tname());
       else more++;
      }
     }
    }
   }
   for (int i = 0; i < names.size(); i++) print_line(names[i].c_str());
   if (more > 0) print_line("%d OTHERS FOUND...", more);
  } break;

  case COMPACT_ELEVATOR_ON:
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
	 g->m.rewrite<t_elevator_control_off, t_elevator_control>(x,y);
    }
   }
   print_line("Elevator activated.");
   break;

  case COMPACT_AMIGARA_LOG: // TODO: This is static, move to data file?
   print_line("NEPower Mine(%d:%d) Log", g->lev.x, g->lev.y);
   print_line("\
ENTRY 47:\n\
Our normal mining routine has unearthed a hollow chamber.  This would not be\n\
out of the ordinary, save for the odd, perfectly vertical faultline found.\n\
This faultline has several odd concavities in it which have the more\n\
superstitious crew members alarmed; they seem to be of human origin.\n\
\n\
ENTRY 48:\n\
The concavities are between 10 and 20 feet tall, and run the length of the\n\
faultline.  Each one is vaguely human in shape, but with the proportions of\n\
the limbs, neck and head greatly distended, all twisted and curled in on\n\
themselves.\n");
   if (!query_bool("Continue reading?"))
    return;
   reset_terminal();
   print_line("NEPower Mine(%d:%d) Log", g->lev.x, g->lev.y);
   print_line("\
ENTRY 49:\n\
We've stopped mining operations in this area, obviously, until archaeologists\n\
have the chance to inspect the area.  This is going to set our schedule back\n\
by at least a week.  This stupid artifact-preservation law has been in place\n\
for 50 years, and hasn't even been up for termination despite the fact that\n\
these mining operations are the backbone of our economy.\n\
\n\
ENTRY 52:\n\
Still waiting on the archaeologists.  We've done a little light insepction of\n\
the faultline; our sounding equipment is insufficient to measure the depth of\n\
the concavities.  The equipment is rated at 15 miles depth, but it isn't made\n\
for such narrow tunnels, so it's hard to say exactly how far back they go.\n");
   if (!query_bool("Continue reading?"))
    return;
   reset_terminal();
   print_line("NEPower Mine(%d:%d) Log", g->lev.x, g->lev.y);
   print_line("\
ENTRY 54:\n\
I noticed a couple of the guys down in the chamber with a chisel, breaking\n\
off a piece of the sheer wall.  I'm looking the other way.  It's not like\n\
the eggheads are going to notice a little piece missing.  Fuck em.\n\
\n\
ENTRY 55:\n\
Well, the archaeologists are down there now with a couple of the boys as\n\
guides.  They're hardly Indiana Jones types; I doubt they been below 20\n\
feet.  I hate taking guys off assignment just to babysit the scientists, but\n\
if they get hurt we'll be shut down for god knows how long.\n\
\n\
ENTRY 58:\n\
They're bringing in ANOTHER CREW?  Christ, it's just some cave carvings!  I\n\
know that's sort of a big deal, but come on, these guys can't handle it?\n");
   if (!query_bool("Continue reading?"))
    return;
   reset_terminal();
   for (int i = 0; i < 10; i++)
    print_gibberish_line();
   print_line("");
   print_line("");
   print_line("");
   print_line("AMIGARA PROJECT");
   print_line("");
   print_line("");
   if (!query_bool("Continue reading?"))
    return;
   reset_terminal();
   print_line("\
SITE %d%d%d%d%d\n\
PERTINANT FOREMAN LOGS WILL BE PREPENDED TO NOTES",
g->cur_om.pos.x, g->cur_om.pos.y, g->lev.x, g->lev.y, abs(g->lev.z));
   print_line("\n\
MINE OPERATIONS SUSPENDED; CONTROL TRANSFERRED TO AMIGARA PROJECT UNDER\n\
   IMPERATIVE 2:07B\n\
FAULTLINE SOUNDING HAS PLACED DEPTH AT 30.09 KM\n\
DAMAGE TO FAULTLINE DISCOVERED; NEPOWER MINE CREW PLACED UNDER ARREST FOR\n\
   VIOLATION OF REGULATION 87.08 AND TRANSFERRED TO LAB 89-C FOR USE AS\n\
   SUBJECTS\n\
QUALITIY OF FAULTLINE NOT COMPROMISED\n\
INITIATING STANDARD TREMOR TEST...");
   print_gibberish_line();
   print_gibberish_line();
   print_line("");
   print_error("FILE CORRUPTED, PRESS ANY KEY...");
   getch();
   reset_terminal();
   break;

  case COMPACT_AMIGARA_START:
   g->add_event(EVENT_AMIGARA, int(messages.turn) + 10, 0, 0, 0);
   if (!g->u.has_artifact_with(AEP_PSYSHIELD)) g->u.add_disease(DI_AMIGARA, MINUTES(2));
   break;

  case COMPACT_DOWNLOAD_SOFTWARE:
   if (!g->u.has_amount(itm_usb_drive, 1))
    print_error("USB drive required!");
   else {   // XXX \todo maybe allow downloading non-mission software?
    mission *miss = mission::from_id(mission_id);
    if (miss == nullptr) {
     debugmsg("Computer couldn't find its mission!");
     return;
    }
    if (item* usb_drive = g->u.pick_usb()) {
        item software(item::types[miss->item_id], 0);   // this software is pre-Cataclysm
        software.mission_id = mission_id;
        usb_drive->contents.clear();
        usb_drive->put_in(std::move(software));
        print_line("Software downloaded.");
    }
   }
   break;

  case COMPACT_BLOOD_ANAL: {
      auto centrifuge_at = find_first(zaimoni::gdi::box<point>(g->u.pos + point(-2), g->u.pos + point(2)), [&](point pt) { return t_centrifuge == g->m.ter(pt); });
      if (!centrifuge_at) {
          debuglog("blood analysis without centrifuge");
#ifndef NDEBUG
          throw std::logic_error("blood analysis without centrifuge");
#else
          debugmsg("blood analysis without centrifuge");
          return;
#endif
      }
      const auto& inv = g->m.i_at(*centrifuge_at);
      if (auto err = blood_analysis_precondition(inv)) {
          print_error(err);
          return;
      }
      const item& blood = inv[0].contents[0]; // Success!
      if (blood.corpse == nullptr || blood.corpse->id == mon_null)
          print_line("Result:  Human blood, no pathogens found.");
      else if (blood.corpse->sym == 'Z') {
          print_line("Result:  Human blood.  Unknown pathogen found.");
          print_line("Pathogen bonded to erythrocytes and leukocytes.");
          if (query_bool("Download data?")) {
              if (!g->u.has_amount(itm_usb_drive, 1))
                  print_error("USB drive required!");
              else if (item* const usb_drive = g->u.pick_usb()) {
                  usb_drive->contents.clear();
                  usb_drive->put_in(item(item::types[itm_software_blood_data], messages.turn)); // recent scan, so use current turn
                  print_line("Software downloaded.");
              }
          }
      } else print_line("Result: Unknown blood type.  Test nonconclusive.");
      print_line("Press any key...");
      getch();
  }
   break;

 } // switch (action)
}

void computer::activate_random_failure(game *g)
{
 computer_failure fail = (failures.empty() ? COMPFAIL_SHUTDOWN :
                          failures[rng(0, failures.size() - 1)]);
 activate_failure(g, fail);
}

void computer::activate_failure(game *g, computer_failure fail)
{
 // speculate CPU to try to minimize binary file size
 zaimoni::gdi::box<point> robot_spawn_range(g->u.pos - point(3), g->u.pos + point(3));
 auto nominate_robot_spawn_pos = [&]() { return rng(robot_spawn_range); };

 switch (fail) {

  case COMPFAIL_NULL:
   break;	// Do nothing.  Why was this even called >:|

  case COMPFAIL_SHUTDOWN:
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
     if (g->m.has_flag(console, x, y)) g->m.ter(x, y) = t_console_broken;
    }
   }
   break;

  case COMPFAIL_ALARM:
   g->sound(g->u.pos, 60, "An alarm sounds!");
   if (g->lev.z > 0 && !g->event_queued(EVENT_WANTED))
    g->add_event(EVENT_WANTED, int(messages.turn) + 300, 0, g->lev.x, g->lev.y);
   break;

  case COMPFAIL_MANHACKS: {
   int num_robots = rng(4, 8);
   const char* msg = nullptr;
   for (int i = 0; i < num_robots; i++) {
    if (const auto fail = LasVegasChoice<point>(10, nominate_robot_spawn_pos, game::isEmpty)) {
        msg = "Manhacks drop from compartments in the ceiling.";
        g->z.push_back(monster(mtype::types[mon_manhack], *fail));
    }
   }
   if (msg) messages.add(msg);
  } break;

  case COMPFAIL_SECUBOTS: {
   int num_robots = 1;
   const char* msg = nullptr;
   for (int i = 0; i < num_robots; i++) {
    if (const auto fail = LasVegasChoice<point>(10, nominate_robot_spawn_pos, game::isEmpty)) {
        msg = "Secubots emerge from compartments in the floor.";
        g->z.push_back(monster(mtype::types[mon_secubot], *fail));
    }
   }
   if (msg) messages.add(msg);
  } break;

  case COMPFAIL_DAMAGE:
   messages.add("The console electrocutes you!");
   g->u.hurtall(rng(1, 10));
   break;

  case COMPFAIL_PUMP_EXPLODE:
   messages.add("The pump explodes!");
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
	 if (g->m.rewrite_test<t_sewage_pump, t_rubble>(x,y)) g->explosion(x, y, 10, 0, false);
    }
   }
   break;

  case COMPFAIL_PUMP_LEAK:
   messages.add("Sewage leaks!");
   for (int x = 0; x < SEEX * MAPSIZE; x++) {
    for (int y = 0; y < SEEY * MAPSIZE; y++) {
     if (g->m.ter(x, y) == t_sewage_pump) {
      point p(x, y);
      int leak_size = rng(4, 10);
      for (int i = 0; i < leak_size; i++) {
       std::vector<point> next_move;
	   point dest(p + Direction::E);
       if (g->m.move_cost(dest) > 0) next_move.push_back(dest);
	   dest = p + Direction::S;
	   if (g->m.move_cost(dest) > 0) next_move.push_back(dest);
	   dest = p + Direction::W;
	   if (g->m.move_cost(dest) > 0) next_move.push_back(dest);
	   dest = p + Direction::N;
	   if (g->m.move_cost(dest) > 0) next_move.push_back(dest);

	   if (next_move.empty()) break;
       
       p = next_move[rng(0, next_move.size() - 1)];
       g->m.ter(p) = t_sewage;
      }
     }
    }
   }
   break;

  case COMPFAIL_AMIGARA:
   g->add_event(EVENT_AMIGARA, int(messages.turn) + 5, 0, 0, 0);
   g->u.add_disease(DI_AMIGARA, MINUTES(2));
   g->explosion(rng(map::reality_bubble_extent), 10, 10, false);
   g->explosion(rng(map::reality_bubble_extent), 10, 10, false);
   break;

  case COMPFAIL_DESTROY_BLOOD:
   for (int x = g->u.pos.x - 2; x <= g->u.pos.x + 2; x++) {
    for (int y = g->u.pos.y - 2; y <= g->u.pos.y + 2; y++) {
     if (g->m.ter(x, y) == t_centrifuge) {
	  auto& inv = g->m.i_at(x, y);
      if (auto err = blood_analysis_precondition(inv)) {
        print_error(err);
      } else {
        print_error("ERROR: Disruptive Spin");
        print_error("ERROR: Blood sample destroyed.");
	    inv[0].contents.clear();
      }
     }
    }
   }
   getch();
   break;
 }// switch (fail)
}

bool computer::query_bool(const char *mes, ...)
{
 if (reject_not_whitelisted_printf(mes)) return false;  // auto-fail
// Translate the printf flags
 va_list ap;
 va_start(ap, mes);
 char buff[6000];
#ifdef _MSC_VER
 vsprintf_s<sizeof(buff)>(buff, mes, ap);
#else
 vsnprintf(buff, sizeof(buff), mes, ap);
#endif
 va_end(ap);
// Print, appending (Y/N/Q)
 print_line((std::string(buff) += " (Y/N/Q)").c_str());
 int ret;
 do ret = getch();
 while (ret != 'y' && ret != 'Y' && ret != 'n' && ret != 'N' && ret != 'q' && ret != 'Q');
 return (ret == 'y' || ret == 'Y');
}
 
char computer::query_ynq(const char *mes, ...)
{
 if (reject_not_whitelisted_printf(mes)) return 'N';    // auto-fail
// Translate the printf flags
 va_list ap;
 va_start(ap, mes);
 char buff[6000];
#ifdef _MSC_VER
 vsprintf_s<sizeof(buff)>(buff, mes, ap);
#else
 vsnprintf(buff, sizeof(buff), mes, ap);
#endif
 va_end(ap);
// Print, appending (Y/N/Q)
 print_line((std::string(buff) += " (Y/N/Q)").c_str());
 int ret;
 do ret = getch();
 while (ret != 'y' && ret != 'Y' && ret != 'n' && ret != 'N' && ret != 'q' && ret != 'Q');
 return ret;
}

void computer::print_line(const char *mes, ...)
{
// Translate the printf flags
 if (reject_not_whitelisted_printf(mes)) return;
 va_list ap;
 va_start(ap, mes);
 char buff[6000];
#ifdef _MSC_VER
 vsprintf_s<sizeof(buff)>(buff, mes, ap);
#else
 vsnprintf(buff, sizeof(buff), mes, ap);
#endif
 va_end(ap);
// Replace any '\n' with "\n " to allow for the border
 std::string message(buff);
 size_t pos = 0;
 while ((pos = message.find('\n', pos)) != std::string::npos) {
	 message.replace(pos, 1, "\n ");
	 pos += 2;
 }
// Print the line.
 wprintz(w_terminal, c_green, " %s\n", message.c_str());
// Reprint the border, in case we pushed a line over it
 wborder(w_terminal, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                     LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 wrefresh(w_terminal);
}

void computer::print_error(const char *mes, ...)
{
 if (reject_not_whitelisted_printf(mes)) return;
// Translate the printf flags
 va_list ap;
 va_start(ap, mes);
 char buff[6000];
#ifdef _MSC_VER
 vsprintf_s<sizeof(buff)>(buff, mes, ap);
#else
 vsnprintf(buff, sizeof(buff), mes, ap);
#endif
 va_end(ap);
// Print the line.
 wprintz(w_terminal, c_red, " %s\n", buff);
// Reprint the border, in case we pushed a line over it
 wborder(w_terminal, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                     LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 wrefresh(w_terminal);
}

void computer::print_gibberish_line()
{
 std::string gibberish;
 int length = rng(50, 70);
 for (int i = 0; i < length; i++) {
  switch (rng(0, 4)) {
  case 0: gibberish += '0' + rng(0, 9); break;
  case 1:
  case 2: gibberish += 'a' + rng(0, 25); break;
  case 3:
  case 4: gibberish += 'A' + rng(0, 25); break;
  }
 }
 waddstrz(w_terminal, c_yellow, gibberish.c_str());
 wborder(w_terminal, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                     LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 wrefresh(w_terminal);
}

void computer::reset_terminal()
{
 werase(w_terminal);
 wborder(w_terminal, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                     LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 wmove(w_terminal, 1, 1);
 wrefresh(w_terminal);
}
