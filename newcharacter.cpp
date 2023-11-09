#include "player.h"
#include "game.h"
#include "output.h"
#include "rng.h"
#include "keypress.h"
#include "skill.h"
#include "json.h"
#include "stl_limits.h"
//#include <unistd.h>
#include <fstream>
#include <ranges>

// Colors used in this file: (Most else defaults to c_ltgray)
#define COL_STAT_ACT		c_ltred    // Selected stat

#define HIGH_STAT 14 // The point after which stats cost double
#define MAX_TRAIT_POINTS 12 // How many points from traits

int set_stats(WINDOW* w, player *u, int &points);
int set_traits(WINDOW* w, player *u, int &points);
int set_skills(WINDOW* w, player *u, int &points);
int set_description(WINDOW* w, player *u, int &points);

static int random_good_trait() { return rng(1, PF_SPLIT - 1); }
static int random_bad_trait() { return rng(PF_SPLIT + 1, PF_MAX - 1); }
static int random_skill() { return rng(1, num_skill_types - 1); }

static int calc_HP(int strength, bool tough)
{
    int ret = (60 + 3 * strength);
    if (tough) cataclysm::rational_scale<6, 5>(ret);
    return ret;
}

// \todo V0.2.1+ trigger uniform max hp recalculation on any change to str_max or the PF_TOUGH trait
void player::normalize()
{
    int max_hp = calc_HP(str_max, has_trait(PF_TOUGH));

    for (int i = 0; i < num_hp_parts; i++) hp_cur[i] = hp_max[i] = max_hp;
}

static std::string template_filename(const std::string& src)
{
    std::string ret("data/");
    ret += src;
    return ret += ".template";
}

bool pc::create(game *g, character_type type, std::string tempname)
{
 weapon = item::null;
 WINDOW* w = newwin(VIEW, SCREEN_WIDTH, 0, 0);
 int tab = 0, points = 38;
 if (type != PLTYPE_CUSTOM) {
  switch (type) {
   case PLTYPE_RANDOM: {
    str_max = rng(6, 12);
    dex_max = rng(6, 12);
    int_max = rng(6, 12);
    per_max = rng(6, 12);
    points -= str_max + dex_max + int_max + per_max;
    if constexpr (12 > HIGH_STAT) {
        if (str_max > HIGH_STAT) points -= (str_max - HIGH_STAT);
        if (dex_max > HIGH_STAT) points -= (dex_max - HIGH_STAT);
        if (int_max > HIGH_STAT) points -= (int_max - HIGH_STAT);
        if (per_max > HIGH_STAT) points -= (per_max - HIGH_STAT);
    }
 
    int num_gtraits = 0, num_btraits = 0, rn, tries;
    while (points < 0 || rng(-3, 20) > points) {
     if (num_btraits < MAX_TRAIT_POINTS && one_in(3)) {
      tries = 0;
      do {
       rn = random_bad_trait();
       tries++;
      } while ((has_trait(rn) ||
              num_btraits - mutation_branch::traits[rn].points > MAX_TRAIT_POINTS) && tries < 5);
      if (tries < 5) {
       toggle_trait(rn);
       points -= mutation_branch::traits[rn].points;
       num_btraits -= mutation_branch::traits[rn].points;
      }
     } else {
      switch (rng(1, 4)) {
       case 1: if (str_max > 5) { str_max--; points++; } break;
       case 2: if (dex_max > 5) { dex_max--; points++; } break;
       case 3: if (int_max > 5) { int_max--; points++; } break;
       case 4: if (per_max > 5) { per_max--; points++; } break;
      }
     }
    }
    while (points > 0) {
     switch (rng((num_gtraits < MAX_TRAIT_POINTS ? 1 : 5), 9)) {
     case 1:
     case 2:
     case 3:
     case 4:
      rn = random_good_trait();
	  if (!has_trait(rn)) {
		const int cost = mutation_branch::traits[rn].points;
        if (points >= cost && num_gtraits + cost <= MAX_TRAIT_POINTS) {
          toggle_trait(rn);
          points -= cost;
          num_gtraits += cost;
        }
	  }
      break;
     case 5:
      switch (rng(1, 4)) {
       case 1: if (str_max < HIGH_STAT) { str_max++; points--; } break;
       case 2: if (dex_max < HIGH_STAT) { dex_max++; points--; } break;
       case 3: if (int_max < HIGH_STAT) { int_max++; points--; } break;
       case 4: if (per_max < HIGH_STAT) { per_max++; points--; } break;
      }
      break;

     case 6:
     case 7:
     case 8:
     case 9:
      rn = random_skill();
      if (points >= sklevel[rn] + 1) {
       points -= sklevel[rn] + 1;
       sklevel[rn] += 2;
      }
      break;
     }
    }
   } break;
   case PLTYPE_TEMPLATE: {
    const std::string filename = template_filename(tempname);
	std::ifstream fin(filename.c_str());
    if (!fin.is_open()) {
     debugmsg("Couldn't open %s!", filename.c_str());
     return false;
    }
	*this = pc(cataclysm::JSON(fin));
    points = 0;
   } break;
  }
  tab = 3;
 } else
  points = 6;

 static constexpr const char* labels[] = {"STATS", "TRAITS", "SKILLS", "DESCRIPTION", nullptr};

 do {
  draw_tabs(w, tab, labels); // C:Whales color scheme c_ltgray, h_ltgray
  // We could lift the horizontal line drawing to here, but that interferes with proofreading. 2020-11-03 zaimoni
  switch (tab) {
   case 0: tab += set_stats      (w, this, points); break;
   case 1: tab += set_traits     (w, this, points); break;
   case 2: tab += set_skills     (w, this, points); break;
   case 3: tab += set_description(w, this, points); break;
  }
 } while (0 <= tab && (std::end(labels) - std::begin(labels) - 1) > tab);
 delwin(w);

 if (0 > tab) return false;
 
 // Character is finalized.  Now just set up HP, &c
 normalize();

 // \todo lift these up into normalize when NPCs can start with traits
 if (has_trait(PF_GLASSJAW)) {
  hp_max[hp_head] = int(hp_max[hp_head] * .85);
  hp_cur[hp_head] = hp_max[hp_head];
 }
 if (has_trait(PF_SMELLY))
  scent = 800;

 // bionics were inherited as PC-only \todo catch this when NPC bionics go up
 if (has_trait(PF_ANDROID)) {
  add_bionic(bionic_id(rng(bio_memory, max_bio_start - 1)));// Other
  if (bionic::types[my_bionics[0].id].power_cost > 0) {
   add_bionic(bionic_id(rng(1, bio_ethanol)));	// Power Source
   max_power_level = 10;
   power_level = 10;
  } else {
   bionic_id tmpbio;
   do tmpbio = bionic_id(rng(bio_ethanol + 1, bio_armor_legs));
   while (bionic::types[tmpbio].power_cost > 0);
   add_bionic(tmpbio);
   max_power_level = 0;
   power_level = 0;
  }

  // \todo character creation debugging hook
/* CHEATER'S STUFF

  add_bionic(bionic_id(rng(0, bio_ethanol)));	// Power Source
  for (int i = 0; i < 5; i++)
   add_bionic(bionic_id(rng(bio_memory, max_bio_start - 1)));// Other
  max_power_level = 80;
  power_level = 80;

End of cheatery */
 }

 if (has_trait(PF_MARTIAL_ARTS)) {
  static constexpr const itype_id ma_types[] = { itm_style_karate, itm_style_judo, itm_style_aikido, itm_style_tai_chi, itm_style_taekwando };
  itype_id ma_type;
  do {
   int choice = menu("Pick your style:", { "Karate", "Judo", "Aikido", "Tai Chi", "Taekwando"});
   ma_type = ma_types[choice - 1];
   item tmpitem = item(item::types[ma_type], 0);
   full_screen_popup(tmpitem.info(true).c_str());
  } while (!query_yn("Use this style?"));
  styles.push_back(ma_type);
 }
 if (!styles.empty())
  weapon = item(item::types[ styles[0] ], 0, ':');
 else
  weapon = item(item::types[0], 0);
// Nice to start out less than naked.
 worn.push_back(item(item::types[itm_jeans], 0, 'a'));
 worn.push_back(item(item::types[itm_tshirt], 0, 'b'));
 worn.push_back(item(item::types[itm_sneakers], 0, 'c'));
// The near-sighted get to start with glasses.
 if (has_trait(PF_MYOPIC)) worn.push_back(item(item::types[itm_glasses_eye], 0, 'd'));
// Likewise, the asthmatic start with their medication.
 if (has_trait(PF_ASTHMA)) inv.push_back(item(item::types[itm_inhaler], 0, 'a' + worn.size()));
// make sure we have no mutations
 for (int i = 0; i < PF_MAX2; i++) my_mutations[i] = false;
 return true;
}

int set_stats(WINDOW* w, player *u, int &points)
{
 unsigned char sel = 1;
// Draw horizontal lines
 draw_hline(w, TABBED_HEADER_HEIGHT + 1, c_ltgray, LINE_OXOX);
 draw_hline(w, VIEW - 4, c_ltgray, LINE_OXOX);

 mvwaddstrz(w, 11, 0, c_ltgray, "\
   j/k, 8/2, or arrows select\n\
    a statistic.\n\
   l, 6, or right arrow\n\
    increases the statistic.\n\
   h, 4, or left arrow\n\
    decreases the statistic.\n\
\n\
   > Takes you to the next tab.\n\
   < Returns you to the main menu.");

 static constexpr const int col2 = 2 + sizeof("Points left: %d  ")+13; // C:Whales: 33
 static constexpr const int flush_rows[] = { TABBED_HEADER_HEIGHT , 6, 7, 8, 9 };
 static constexpr const int flush_rows_col2[] = { 10, 11 }; // 11 collides with legend, above

 // these two may need to be exposed
 static constexpr const int MIN_STAT = 4;
 static constexpr const int MAX_STAT = 20;

 // do not want capturing lambdas here (dynamic RAM allocation unacceptable)
 static auto dec_stat = [](int& dest, decltype(points) pts) {
     if (MIN_STAT < dest) {
         pts += (dest > HIGH_STAT) ? 2 : 1;
         dest--;
     }
 };
 static auto inc_stat = [](int& dest, decltype(points) pts) {
     const int cost = (dest > HIGH_STAT) ? 2 : 1;
     if (MAX_STAT > dest && cost <= pts) {
         pts -= cost;
         dest++;
     }
 };

 do {
  // This is I/O; only need to conserve refresh calls, not CPU 2020-10-31 zaimoni
  for (auto y : flush_rows) draw_hline(w, TABBED_HEADER_HEIGHT, c_black, 'x', 2);
  for (auto y : flush_rows_col2) draw_hline(w, TABBED_HEADER_HEIGHT, c_black, 'x', col2);
  mvwprintz(w, TABBED_HEADER_HEIGHT, 2, c_ltgray, "Points left: %d", points);

  // draw stats
  mvwprintz(w, 6, 2, (6 == 5 + sel) ? COL_STAT_ACT : c_ltgray, "Strength:     %d", u->str_max);
  mvwprintz(w, 7, 2, (7 == 5 + sel) ? COL_STAT_ACT : c_ltgray, "Dexterity:    %d", u->dex_max);
  mvwprintz(w, 8, 2, (8 == 5 + sel) ? COL_STAT_ACT : c_ltgray, "Intelligence: %d", u->int_max);
  mvwprintz(w, 9, 2, (9 == 5 + sel) ? COL_STAT_ACT : c_ltgray, "Perception:   %d", u->per_max);

  switch (sel) {
  case 1:
   if (u->str_max >= HIGH_STAT) mvwaddstrz(w, TABBED_HEADER_HEIGHT, col2, c_ltred, "Increasing Str further costs 2 points.");
   mvwprintz(w, 6, col2, COL_STAT_ACT, "Base HP: %d", calc_HP(u->str_max, u->has_trait(PF_TOUGH)));
   if (option_table::get()[OPT_USE_METRIC_SYS]) {
       mvwprintz(w, 7, col2, COL_STAT_ACT, "Carry weight: %d kg", u->weight_capacity(false) / 9); // slight underestimate: 2.25lbs vs. 2.20-ish lbs per kilogram
   } else {
       mvwprintz(w, 7, col2, COL_STAT_ACT, "Carry weight: %d lbs", u->weight_capacity(false) / 4);
   }
   mvwprintz(w, 8, col2, COL_STAT_ACT, "Melee damage: %d", u->base_damage(false));
   mvwaddstrz(w, 9, col2, COL_STAT_ACT, "  Strength also makes you more resistant to");
   mvwaddstrz(w,10, col2, COL_STAT_ACT, "many diseases and poisons, and makes actions");
   mvwaddstrz(w,11, col2, COL_STAT_ACT, "which require brute force more effective.");
   break;

  case 2:
   if (u->dex_max >= HIGH_STAT) mvwaddstrz(w, TABBED_HEADER_HEIGHT, col2, c_ltred, "Increasing Dex further costs 2 points.");
   mvwprintz(w, 6, col2, COL_STAT_ACT, "Melee to-hit bonus: +%d", u->base_to_hit(false));
   {
   int dex_mod = u->ranged_dex_mod(false);
   mvwprintz(w, 7, col2, COL_STAT_ACT, "Ranged %s: %s%d", (0 >= dex_mod ? "bonus" : "penalty"), (0 >= dex_mod ? "+" : "-"), abs(dex_mod));
   dex_mod = u->throw_dex_mod(false);
   mvwprintz(w, 8, col2, COL_STAT_ACT, "Throwing %s: %s%d", (0 >= dex_mod ? "bonus" : "penalty"), (0 >= dex_mod ? "+" : "-"), abs(dex_mod));
   }
   mvwaddstrz(w, 9, col2, COL_STAT_ACT, "  Dexterity also enhances many actions which");
   mvwaddstrz(w,10, col2, COL_STAT_ACT, "require finesse.");
   break;

  case 3:
   if (u->int_max >= HIGH_STAT) mvwaddstrz(w, TABBED_HEADER_HEIGHT, col2, c_ltred, "Increasing Int further costs 2 points.");
   mvwprintz(w, 6, col2, COL_STAT_ACT, "Skill comprehension: %d%%", u->comprehension_percent(sk_null, false));
   mvwprintz(w, 7, col2, COL_STAT_ACT, "Read times: %d%%", u->read_speed(false));
   mvwaddstrz(w, 8, col2, COL_STAT_ACT, "  Intelligence is also used when crafting,");
   mvwaddstrz(w, 9, col2, COL_STAT_ACT, "installing bionics, and interacting with");
   mvwaddstrz(w,10, col2, COL_STAT_ACT, "NPCs.");
   break;

  case 4:
   if (u->per_max >= HIGH_STAT) mvwaddstrz(w, TABBED_HEADER_HEIGHT, col2, c_ltred, "Increasing Per further costs 2 points.");
   {
   int per_mod = u->ranged_per_mod(false);
   mvwprintz(w, 6, col2, COL_STAT_ACT, "Ranged %s: %s%d", (0 >= per_mod ? "bonus" : "penalty"), (0 >= per_mod ? "+" : "-"), abs(per_mod));
   }
   mvwaddstrz(w, 7, col2, COL_STAT_ACT, "  Perception is also used for detecting");
   mvwaddstrz(w, 8, col2, COL_STAT_ACT, "traps and other things of interest.");
   break;
  }
 
  wrefresh(w);
  switch (int ch = input()) {
  case 'j':
      if (4 > sel) sel++;
      break;
  case 'k':
      if (1 < sel) sel--;
      break;
  case 'h':
      switch (sel) {
      case 1:
          dec_stat(u->str_max, points);
          break;
      case 2:
          dec_stat(u->dex_max, points);
          break;
      case 3:
          dec_stat(u->int_max, points);
          break;
      case 4:
          dec_stat(u->per_max, points);
          break;
      }
      break;
  case 'l':
      switch (sel) {
      case 1:
          inc_stat(u->str_max, points);
          break;
      case 2:
          inc_stat(u->dex_max, points);
          break;
      case 3:
          inc_stat(u->int_max, points);
          break;
      case 4:
          inc_stat(u->per_max, points);
          break;
      }
      break;
  case '<':
      if (query_yn("Return to main menu?")) return -1;
      break;
  case '>':  return 1;
  }
 } while (true);
}

int set_traits(WINDOW* w, player *u, int &points)
{
// Track how many good / bad POINTS we have; cap both at MAX_TRAIT_POINTS
 int num_good = 0, num_bad = 0;
 for (int i = 0; i < PF_SPLIT; i++) {
  if (u->has_trait(i))
   num_good += mutation_branch::traits[i].points;
 }
 for (int i = PF_SPLIT + 1; i < PF_MAX; i++) {
  if (u->has_trait(i))
   num_bad += abs(mutation_branch::traits[i].points);
 }

 const int v_span = VIEW - (TABBED_HEADER_HEIGHT + 2 + 4 + 1); // vertical span
 const int v_span_div_2 = v_span / 2;
 const int mid_pt = SCREEN_WIDTH / 2;
// Draw horizontal lines
 draw_hline(w, TABBED_HEADER_HEIGHT + 1, c_ltgray, LINE_OXOX);
 draw_hline(w, VIEW - 4, c_ltgray, LINE_OXOX);

 const auto good_traits = std::ranges::subrange(mutation_branch::traits + 1, mutation_branch::traits + PF_SPLIT);
 const auto bad_traits = std::ranges::subrange(mutation_branch::traits + PF_SPLIT + 1, mutation_branch::traits + PF_MAX);

 for (int i = 0; i <= v_span; i++) {
  draw_hline(w, 5 + i, c_dkgray, ' ');
 }
#if NO_OP_LEGEND
 // this is being overwritten by the good traits
 mvwprintz(w,11,32, c_ltgray, "h   l");
 mvwprintz(w,12,32, c_ltgray, "<   >");
 mvwprintz(w,13,32, c_ltgray, "4   6");
 mvwprintz(w,15,32, c_ltgray, "Space");
 mvwprintz(w,16,31, c_ltgray,"Toggles");
#endif

 int cur_trait;
 nc_color col_on, col_off, hi_on, hi_off;

 static auto draw_traits = [&](int col, decltype(good_traits)& src) {
     const auto trait_min = src.begin() - mutation_branch::traits;
     const auto trait_max = src.end() - mutation_branch::traits;
     int origin = 0;
     if (v_span >= trait_max - trait_min || cur_trait <= trait_min + v_span_div_2) {
         origin = trait_min;
     } else if (cur_trait >= trait_max - (v_span_div_2 + 2)) {
         origin = trait_max - (v_span + 1);
     } else {
         origin = cur_trait - v_span_div_2;
     }

     int delta = 0;
     for (decltype(auto) trait : src) {
         const auto i = &trait - mutation_branch::traits;
         if (origin > i) continue;
         draw_hline(w, 5 + delta, c_ltgray, ' ', col, col + mid_pt); // Clear the line
         if (i == cur_trait) {
             mvwaddstrz(w, 5 + delta, col, (u->has_trait(i) ? hi_on : hi_off), trait.name.c_str());
         } else {
             mvwaddstrz(w, 5 + delta, col, (u->has_trait(i) ? col_on : col_off), trait.name.c_str());
         }
         delta++;
     }
 };

 static auto draw_inactive_traits = [&](int col, decltype(good_traits)& src) {
     int delta = 0;
     for (decltype(auto) trait : src) {
         draw_hline(w, 5 + delta, c_dkgray, ' ', col, col + mid_pt);
         mvwaddstrz(w, 5 + delta, col, c_dkgray, trait.name.c_str());
         delta++;
     }
 };

 int cur_adv = 1, cur_dis = PF_SPLIT + 1;
 bool using_adv = true;	// True if we're selecting advantages, false if we're selecting disadvantages
 draw_inactive_traits(mid_pt, bad_traits);

 static constexpr nc_color COL_TR_GOOD = c_green; // Good trait descriptive text
 static constexpr nc_color COL_TR_GOOD_OFF = c_ltgreen; // A toggled-off good trait
 static constexpr nc_color COL_TR_GOOD_ON = c_green; // A toggled-on good trait
 static constexpr nc_color COL_TR_BAD = c_red; // Bad trait descriptive text
 static constexpr nc_color COL_TR_BAD_OFF = c_ltred; // A toggled-off bad trait
 static constexpr nc_color COL_TR_BAD_ON = c_red; // A toggled-on bad trait

 do {
  draw_hline(w, 3, c_ltgray, ' ');
  mvwprintz(w,  3,  2, c_ltgray, "Points left: %d", points);
  mvwprintz(w,  3, 21 - int_log10(num_good), c_ltgreen, "%d/%d", num_good, MAX_TRAIT_POINTS);
  mvwprintz(w,  3, 34 - int_log10(num_bad), c_ltred, "%d/%d", num_bad, MAX_TRAIT_POINTS);
// Clear the bottom of the screen.
  draw_hline(w, VIEW - 3, c_ltgray, ' ');
  draw_hline(w, VIEW - 2, c_ltgray, ' ');
  draw_hline(w, VIEW - 1, c_ltgray, ' ');
  if (using_adv) {
   col_on  = COL_TR_GOOD_ON;
   col_off = COL_TR_GOOD_OFF;
   hi_on   = hilite(col_on);
   hi_off  = hilite(col_off);
   cur_trait = cur_adv;
   mvwprintz(w,  3, mid_pt, COL_TR_GOOD, "%s costs %d points",
	   mutation_branch::traits[cur_adv].name.c_str(), mutation_branch::traits[cur_adv].points);
   mvwaddstrz(w, VIEW - 3, 0, COL_TR_GOOD, mutation_branch::traits[cur_adv].description.c_str());
   draw_traits(0, good_traits);
  } else {
   col_on  = COL_TR_BAD_ON;
   col_off = COL_TR_BAD_OFF;
   hi_on   = hilite(col_on);
   hi_off  = hilite(col_off);
   cur_trait = cur_dis;
   mvwprintz(w,  3, mid_pt, COL_TR_BAD, "%s earns %d points",
	   mutation_branch::traits[cur_dis].name.c_str(), mutation_branch::traits[cur_dis].points * -1);
   mvwaddstrz(w, VIEW - 3, 0, COL_TR_BAD, mutation_branch::traits[cur_dis].description.c_str());
   draw_traits(mid_pt, bad_traits);
  }

  wrefresh(w);
  switch (input()) {
   case 'h':
   case 'l':
   case '\t':
    if (!using_adv) draw_inactive_traits(mid_pt, bad_traits);
    else draw_inactive_traits(0, good_traits);
    using_adv = !using_adv;
    wrefresh(w);
    break;
   case 'k':
    if (using_adv) {
     if (cur_adv > 1) --cur_adv;
    } else {
     if (cur_dis > PF_SPLIT + 1) --cur_dis;
    }
    break;
   case 'j':
   if (using_adv) {
     if (cur_adv < PF_SPLIT - 1) ++cur_adv;
    } else {
     if (cur_dis < PF_MAX - 1) ++cur_dis;
    }
    break;
   case ' ':
   case '\n':
    {
    const int cost = mutation_branch::traits[cur_trait].points;
    if (u->has_trait(cur_trait)) {
     if (points + cost >= 0) {
      u->toggle_trait(cur_trait);
      points += cost;
      if (using_adv) num_good -= cost;
      else num_bad += cost;
     } else
      mvwprintz(w,  3, 2, c_red, "Points left: %d  ", points);
    } else if (using_adv && num_good + cost > MAX_TRAIT_POINTS)
     popup("Sorry, but you can only take %d points of advantages.", MAX_TRAIT_POINTS);
    else if (!using_adv && num_bad - cost > MAX_TRAIT_POINTS)
     popup("Sorry, but you can only take %d points of disadvantages.", MAX_TRAIT_POINTS);
    else if (points >= cost) {
     u->toggle_trait(cur_trait);
     points -= cost;
     if (using_adv) num_good += cost;
     else num_bad -= cost;
    }
	}
    break;
   case '<': return -1;
   case '>': return 1;
  }
 } while (true);
}

int set_skills(WINDOW* w, player *u, int &points)
{
 static constexpr const nc_color COL_SKILL_USED = c_green; // A skill with at least one point
 const int v_span = VIEW - (TABBED_HEADER_HEIGHT + 2 + 4 + 1); // vertical span
 const int v_span_div_2 = v_span / 2;

// Draw horizontal lines
 draw_hline(w, TABBED_HEADER_HEIGHT + 1, c_ltgray, LINE_OXOX);
 draw_hline(w, VIEW - 4, c_ltgray, LINE_OXOX);

 int cur_sk = 1;

 static auto draw_skills = [&](int origin) {
     for (int delta = 0; delta <= v_span; delta++) { // has to be increasing order due to implementation details
         const int i = origin + delta;
         draw_hline(w, 5 + delta, c_ltgray, ' ');
         if (0 == u->sklevel[i]) {
             mvwaddstrz(w, 5 + delta, 0, (i == cur_sk ? h_ltgray : c_ltgray), skill_name(skill(i)));
         } else {
             mvwaddstrz(w, 5 + delta, 0, (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED), skill_name(skill(i)));
             for (int j = 0; j < u->sklevel[i]; j++)
                 wputch(w, (i == cur_sk ? hilite(COL_SKILL_USED) : COL_SKILL_USED), '*');
         }
     }
 };

 do {
  draw_hline(w, 3, c_ltgray, ' ');
  mvwprintz(w,  3, 2, c_ltgray, "Points left: %d  ", points);
// Clear the bottom of the screen.
  draw_hline(w, VIEW - 3, c_ltgray, ' ');
  draw_hline(w, VIEW - 2, c_ltgray, ' ');
  draw_hline(w, VIEW - 1, c_ltgray, ' ');
  const int cost = u->sklevel[cur_sk] + 1;
  mvwprintz(w,  3, 30, ((cost <= points) ? COL_SKILL_USED : c_ltred), "Upgrading %s costs %d points", skill_name(skill(cur_sk)), cost);
  mvwaddstrz(w, VIEW - 3, 0, COL_SKILL_USED, skill_description(skill(cur_sk)));

  if (cur_sk <= v_span_div_2 + 1) {
   draw_skills(1);
  } else if (cur_sk >= num_skill_types - (v_span_div_2 + 2)) {
   draw_skills(num_skill_types - (v_span + 1));
  } else {
   draw_skills(cur_sk - v_span_div_2);
  }
   
  wrefresh(w);
  switch (input()) {
   case 'j':
    if (cur_sk < num_skill_types - 1) cur_sk++;
    break;
   case 'k':
    if (cur_sk > 1) cur_sk--;
    break;
   case 'h':
    if (u->sklevel[cur_sk] > 0) {
     points += u->sklevel[cur_sk] - 1;
     u->sklevel[cur_sk] -= 2;
    }
    break;
   case 'l':
    if (cost <= points) {
     points -= cost;
     u->sklevel[cur_sk] += 2;
    }
    break;
   case '<': return -1;
   case '>': return 1;
  }
 } while (true);
}

void save_template(const player& u)
{
	std::string name = string_input_popup("Name of template:");
	if (0 >= name.length()) return;
    const std::string playerfile = template_filename(name);
	std::ofstream fout(playerfile.c_str());
	if (fout.is_open()) fout << toJSON(u);
	else debugmsg("Sorry, couldn't open %s.", playerfile.c_str());
}

int set_description(WINDOW* w, player *u, int &points)
{
 static constexpr const int MAX_NAME_LEN = 30;

// Draw horizontal lines
 draw_hline(w, TABBED_HEADER_HEIGHT + 1, c_ltgray, LINE_OXOX);
 draw_hline(w, VIEW - 4, c_ltgray, LINE_OXOX);

 mvwprintz(w,  3, 2, c_ltgray, "Points left: %d  ", points);

 mvwaddstrz(w, 6, 2, c_ltgray, "Name: ______________________________     (Press TAB to move off this line)");
 mvwaddstrz(w, 8, 2, c_ltgray, "Gender: Male Female                      (Press spacebar to toggle)");
 mvwaddstrz(w,10, 2, c_ltgray, "When your character is finished and you're ready to start playing, press >");
 mvwaddstrz(w,12, 2, c_ltgray, "To go back and review your character, press <");
 mvwaddstrz(w, 14, 2, c_green, "To pick a random name for your character, press ?.");
 mvwaddstrz(w, 16, 2, c_green, "To save this character as a template, press !.");
 
 int line = 1;
 bool noname = false;

 do {
  if (u->male) {
   mvwaddstrz(w, 8, 10, c_ltred, "Male");
   mvwaddstrz(w, 8, 15, c_ltgray, "Female");
  } else {
   mvwaddstrz(w, 8, 10, c_ltgray, "Male");
   mvwaddstrz(w, 8, 15, c_ltred, "Female");
  }

  if (!noname) {
   mvwaddstrz(w, 6, 8, c_ltgray, u->name.c_str());
   if (line == 1) wputch(w, h_ltgray, '_');
  }
  mvwaddstrz(w, 8, 2, (2 == line ? h_ltgray : c_ltgray), "Gender:");

  wrefresh(w);
  if (noname) {
   draw_hline(w, 6, c_ltgray, '_', 8, 8 + MAX_NAME_LEN);
   noname = false;
  }

  switch (int ch = getch())
  {
  case '>': {
      if (points > 0) mvwprintz(w, 3, 2, c_red, "Points left: %d    You must use the rest of your points!", points);
      else if (u->name.size() == 0) {
          mvwaddstrz(w, 6, 8, h_ltgray, "______NO NAME ENTERED!!!!_____");
          noname = true;
          wrefresh(w);
          if (query_yn("Are you SURE you're finished? Your name will be randomly generated.")) {
              u->pick_name();
              return 1;
          }
      } else if (query_yn("Are you SURE you're finished?")) return 1;
      else refresh();
      }
      break;
  case '<': return -1;
  case '!': {
      if (points > 0) popup("You cannot save a template with unused points!");
      else save_template(*u);
      mvwaddstrz(w, 12, 2, c_ltgray, "To go back and review your character, press <");
      wrefresh(w);
      }
      break;
  case '?':
      draw_hline(w, 6, c_ltgray, '_', 8, 8 + MAX_NAME_LEN);
      u->pick_name();
      break;
  default:
      switch (line) {
      case 1:
          if (ch == KEY_BACKSPACE || ch == 127) {
              if (u->name.size() > 0) {
                  mvwputch(w, 6, 8 + u->name.size(), c_ltgray, '_');
                  u->name.erase(u->name.end() - 1);
              }
          } else if (ch == '\t') {
              line = 2;
              mvwputch(w, 6, 8 + u->name.size(), c_ltgray, '_');
          } else if (((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == ' ') && u->name.size() < MAX_NAME_LEN) {
              u->name.push_back(ch);
          }
          break;
      case 2:
          if (ch == ' ') u->male = !u->male;
          else if (ch == 'k' || ch == '\t') {
              line = 1;
              mvwputch(w, 8, 8, c_ltgray, ':');
          }
          break;
      }
      break;
  }
 } while (true);
}
