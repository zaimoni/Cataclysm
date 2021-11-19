#include "bionics.h"
#include "player.h"
#include "game.h"
#include "rng.h"
#include "keypress.h"
#include "item.h"
#include "line.h"
#include "recent_msg.h"
#include "stl_typetraits.h"

static const char* JSON_transcode[] = {
	"batteries",
	"metabolics",
	"solar",
	"furnace",
	"ethanol",
	"memory",
	"ears",
	"eye_enhancer",
	"membrane",
	"targeting",
	"gills",
	"purifier",
	"climate",
	"storage",
	"recycler",
	"digestion",
	"tools",
	"shock",
	"heat_absorb",
	"carbon",
	"armor_head",
	"armor_torso",
	"armor_arms",
	"armor_legs",
	"flashlight",
	"night_vision",
	"infrared",
	"face_mask",
	"ads",
	"ods",
	"scent_mask",
	"scent_vision",
	"cloak",
	"painkiller",
	"nanobots",
	"heatsink",
	"resonator",
	"time_freeze",
	"teleport",
	"blood_anal",
	"blood_filter",
	"alarm",
	"evap",
	"lighter",
	"claws",
	"blaster",
	"laser",
	"emp",
	"hydraulics",
	"water_extractor",
	"magnet",
	"fingerhack",
	"lockpick",
	"ground_sonar",
	"",	// max_bio_start
	"banish",
	"gate_out",
	"gate_in",
	"nightfall",
	"drilldown",
	"heatwave",
	"lightning",
	"tremor",
	"flashflood",
	"",	// max_bio_good
	"dis_shock",
	"dis_acid",
	"drain",
	"noise",
	"power_weakness",
	"stiff"
};

DEFINE_JSON_ENUM_SUPPORT_HARDCODED_NONZERO(bionic_id, JSON_transcode)

const bionic_data bionic::type[] = {
{"NULL bionics", false, false, 0, 0, "\
If you're seeing this, it's a bug."},
// NAME          ,PW_SRC, ACT ,COST, TIME,
{"Battery System", true, false, 0, 0, "\
You have a battery draining attachment, and thus can make use of the energy\n\
contained in normal, everyday batteries.  Use 'E' to consume batteries."},
{"Metabolic Interchange", true, false, 0, 0, "\
Your digestive system and power supply are interconnected.  Any drain on\n\
energy instead increases your hunger."},
{"Solar Panels", true, false, 0, 0, "\
You have a few solar panels installed.  While in direct sunlight, your power\n\
level will slowly recharge."},
{"Internal Furnace", true, false, 0, 0, "\
You can burn nearly any organic material as fuel (use 'E'), recharging your\n\
power level.  Some materials will burn better than others."},
{"Ethanol Burner", true, false, 0, 0, "\
You burn alcohol as fuel in an extremely efficient reaction.  However, you\n\
will still suffer the inebriating effects of the substance."},

{"Enhanced Memory Banks", false, false, 1, 0, "\
Your memory has been enhanced with small quantum storage drives.  Any time\n\
you start to forget a skill, you have a chance at retaining all knowledge, at\n\
the cost of a small amount of power."},
{"Enhanced Hearing", false, false, 0, 0, "\
Your hearing has been drastically improved, allowing you to hear ten times\n\
better than the average person.  Additionally, high-intensity sounds will be\n\
automatically dampened before they can damage your hearing."},
{"Diamond Cornea", false, false, 0, 0, "\
Your vision is greatly enhanced, giving you a +2 bonus to perception."},
{"Nictating Membrane", false, false, 0, 0, "\
Your eyes have a thin membrane that closes over your eyes while underwater,\n\
negating any vision penalties."},
{"Targeting System", false, false, 0, 0, "\
Your eyes are equipped with range finders, and their movement is synced with\n\
that of your arms, to a degree.  Shots you fire will be much more accurate,\n\
particularly at long range."},
{"Membrane Oxygenator", false, false, 1, 0, "\
An oxygen interchange system automatically switches on while underwater,\n\
slowly draining your energy reserves but providing oxygen."},
{"Air Filtration System", false, false, 1, 0, "\
Implanted in your trachea is an advanced filtration system.  If toxins find\n\
their way into your windpipe, the filter will attempt to remove them."},
{"Internal Climate Control", false, false, 0, 0, "\
Throughout your body lies a network of thermal piping which eases the effects\n\
of high and low ambient temperatures.  It has an operating range of 0 to 140\n\
degrees Fahrenheit."},
{"Internal Storage", false, false, 0, 0, "\
Space inside your chest cavity has been converted into a storage area.  You\n\
may carry an extra 8 units of volume."},
{"Recycler Unit", false, false, 0, 0, "\
Your digestive system has been outfitted with a series of filters and\n\
processors, allowing you to reclaim waste liquid and, to a lesser degree,\n\
nutrients.  The net effect is a greatly reduced need to eat and drink."},
{"Expanded Digestive System", false, false, 0, 0, "\
You have been outfitted with three synthetic stomachs and industrial-grade\n\
intestines.  Not only can you extract much more nutrition from food, but you\n\
are highly resistant to foodborne illness, and can sometimes eat rotten food."},
{"Integrated Toolset", false, false, 0, 0, "\
Implanted in your hands and fingers is a complete tool set - screwdriver,\n\
hammer, wrench, and heating elements.  You can use this in place of many\n\
tools when crafting."},
{"Electroshock Unit", false, false, 1, 0, "\
While fighting unarmed, or with a weapon that conducts electricity, there is\n\
a chance that a successful hit will shock your opponent, inflicting extra\n\
damage and disabling them temporarily at the cost of some energy."},
{"Heat Drain", false, false, 1, 0, "\
While fighting unarmed against a warm-blooded opponent, there is a chance\n\
that a successful hit will drain body heat, inflicting a small amount of\n\
extra damage, and increasing your power reserves slightly."},
{"Subdermal Carbon Filament", false, false, 0, 0, "\
Lying just beneath your skin is a thin armor made of carbon nanotubes. This\n\
reduces bashing damage by 2 and cutting damage by 4, but also reduces your\n\
dexterity by 2."},
{"Alloy Plating - Head", false, false, 0, 0, "\
The flesh on your head has been replaced by a strong armor, protecting both\n\
your head and jaw regions, but increasing encumberance by 2 and decreasing\n\
perception by 1."},
{"Alloy Plating - Torso", false, false, 0, 0, "\
The flesh on your torso has been replaced by a strong armor, protecting you\n\
greatly, but increasing your encumberance by 2."},
{"Alloy Plating - Arms", false, false, 0, 0, "\
The flesh on your arms has been replaced by a strong armor, protecting you\n\
greatly, but decreasing your dexterity by 1."},
{"Alloy Plating - Legs", false, false, 0, 0, "\
The flesh on your legs has been replaced by a strong armor, protecting you\n\
greatly, but decreasing your speed by 15."},

{"Cranial Flashlight", false, true, 1, 30, "\
Mounted between your eyes is a small but powerful LED flashlight."},
{"Implanted Night Vision", false, true, 1, 20, "\
Your eyes have been modified to amplify existing light, allowing you to see\n\
in the dark."},
{"Infrared Vision", false, true, 1, 4, "\
Your range of vision extends into the infrared, allowing you to see warm-\n\
blooded creatures in the dark, and even through walls."},
{"Facial Distortion", false, true, 1, 10, "\
Your face is actually made of a compound which may be molded by electrical\n\
impulses, making you impossible to recognize.  While not powered, however,\n\
the compound reverts to its default shape."},
{"Active Defense System", false, true, 1, 7, "\
A thin forcefield surrounds your body, continually draining power.  Anything\n\
attempting to penetrate this field has a chance of being deflected at the\n\
cost of more energy.  Melee attacks will be stopped more often than bullets."},
{"Offensive Defense System", false, true, 1, 6, "\
A thin forcefield surrounds your body, continually draining power.  This\n\
field does not deflect penetration, but rather delivers a very strong shock,\n\
damaging unarmed attackers and those with a conductive weapon."},
{"Olfactory Mask", false, true, 1, 8, "\
While this system is powered, your body will produce very little odor, making\n\
it nearly impossible for creatures to track you by scent."},
{"Scent Vision", false, true, 1, 30, "\
While this system is powered, you're able to visually sense your own scent,\n\
making it possible for you to recognize your surroundings even if you can't\n\
see it."},
{"Cloaking System", false, true, 2, 1, "\
This high-power system uses a set of cameras and LEDs to make you blend into\n\
your background, rendering you fully invisible to normal vision.  However,\n\
you may be detected by infrared, sonar, etc."},
{"Sensory Dulling", false, true, 2, 0, "\
Your nervous system is wired to allow you to inhibit the signals of pain,\n\
allowing you to dull your senses at will.  However, the use of this system\n\
may cause delayed reaction time and drowsiness."},
{"Repair Nanobots", false, true, 5, 0, "\
Inside your body is a fleet of tiny dormant robots.  Once charged from your\n\
energy banks, they will flit about your body, repairing any damage."},
{"Thermal Dissapation", false, true, 1, 6, "\
Powerful heatsinks supermaterials are woven into your flesh.  While powered,\n\
this system will prevent heat damage up to 2000 degrees fahrenheit.  Note\n\
that this does not affect your internal temperature."},
{"Sonic Resonator", false, true, 4, 0, "\
Your entire body may resonate at very high power, creating a short-range\n\
shockwave.  While it will not to much damage to flexible creatures, stiff\n\
items such as walls, doors, and even robots will be severely damaged."},
{"Time Dilation", false, true, 3, 0, "\
At an immense cost of power, you may increase your body speed and reactions\n\
dramatically, essentially freezing time.  You are still delicate, however,\n\
and violent or rapid movements may damage you due to friction."},
{"Teleportation Unit", false, true, 10, 0, "\
This highly experimental unit folds space over short distances, instantly\n\
transporting your body up to 25 feet at the cost of much power.  Note that\n\
prolonged or frequent use may have dangerous side effects."},
{"Blood Analysis", false, true, 1, 0, "\
Small sensors have been implanted in your heart, allowing you to analyse your\n\
blood.  This will detect many illnesses, drugs, and other conditions."},
{"Blood Filter", false, true, 3, 0, "\
A filtration system in your heart allows you to actively filter out chemical\n\
impurities, primarily drugs.  It will have limited impact on viruses.  Note\n\
that it is not a targeted filter; ALL drugs in your system will be affected."},
{"Alarm System", false, true, 1, 400, "\
A motion-detecting alarm system will notice almost all movement within a\n\
fifteen-foot radius, and will silently alert you.  This is very useful during\n\
sleep, or if you suspect a cloaked pursuer."},
{"Aero-Evaporator", false, true, 8, 0, "\
This unit draws moisture from the surrounding air, which then is poured from\n\
a fingertip in the form of water.  It may fail in very dry environments."},
{"Mini-Flamethrower", false, true, 3, 0, "\
The index fingers of both hands have powerful fire starters which extend from\n\
the tip."},
{"Adamantium Claws", false, true, 3, 0, "\
Your fingers can withdraw into your hands, allowing a set of vicious claws to\n\
extend.  These do considerable cutting damage, but prevent you from holding\n\
anything else."},
{"Fusion Blaster Arm", false, true, 2, 0, "\
Your left arm has been replaced by a heavy-duty fusion blaster!  You may use\n\
your energy banks to fire a damaging heat ray; however, you are unable to use\n\
or carry two-handed items, and may only fire handguns."},
{"Finger-Mounter Laser", false, true, 2, 0, "\
One of your fingers has a small high-powered laser embedded in it.  This long\n\
range weapon is not incredibly damaging, but is very accurate, and has the\n\
potential to start fires."},
{"Directional EMP", false, true, 4, 0, "\
Mounted in the palms of your hand are small parabolic EMP field generators.\n\
You may use power to fire a short-ranged blast which will disable electronics\n\
and robots."},
{"Hydraulic Muscles", false, true, 1, 3, "\
While activated, the muscles in your arms will be greatly enchanced,\n\
increasing your strength by 20."},
{"Water Extraction Unit", false, true, 2, 0, "\
Nanotubs embedded in the palm of your hand will pump any available fluid out\n\
of a dead body, cleanse it of impurities and convert it into drinkable water.\n\
You must, however, have a container to store the water in."},
{"Electromagnetic Unit", false, true, 2, 0, "\
Embedded in your hand is a powerful electromagnet, allowing you to pull items\n\
made of iron over short distances."},
{"Fingerhack", false, true, 1, 0, "\
One of your fingers has an electrohack embedded in it; an all-purpose hacking\n\
unit used to override control panels and the like (but not computers).  Skill\n\
in computers is important, and a failed use may damage your circuits."},
{"Fingerpick", false, true, 1, 0, "\
One of your fingers has an electronic lockpick embedded in it.  This auto-\n\
matic system will quickly unlock all but the most advanced key locks without\n\
any skill required on the part of the user."},
{"Terranian Sonar", false, true, 1, 5, "\
Your feet are equipped with precision sonar equipment, allowing you to detect\n\
the movements of creatures below the ground."},

{"max_bio_start - BUG", false, false, 0, 0, "\
This is a placeholder bionic meant to demarkate those which a new character\n\
can start with.  If you are reading this, you have found a bug!"},

{"Banishment", false, true, 40, 0, "\
You can briefly open a one-way gate to the nether realm, banishing a single\n\
target there permanently.  This is not without its dangers, however, as the\n\
inhabitants of the nether world may take notice."},
{"Gate Out", false, true, 55, 0, "\
You can temporarily open a two-way gate to the nether realm, accessible only\n\
by you.  This will remove you from immediate danger, but may attract the\n\
attention of the nether world's inhabitants..."},
{"Gate In", false, true, 35, 0, "\
You can temporarily open a one-way gate from the nether realm.  This will\n\
attract the attention of its horrifying inhabitants, who may begin to pour\n\
into reality."},
{"Artificial Night", false, true, 5, 1, "\
Photon absorbtion panels will attract and obliterate photons within a 100'\n\
radius, casting an area around you into pitch darkness."},
{"Borehole Drill", false, true, 30, 0, "\
Your legs can transform into a powerful drill which will bury you 50 feet\n\
into the earth.  Be careful to only drill down when you know you will be able\n\
to get out, or you'll simply dig your own tomb."},
{"Heatwave", false, true, 45, 0, "\
At the cost of immense power, you can cause dramatic spikes in the ambient\n\
temperature around you.  These spikes are very short-lived, but last long\n\
enough to ignite wood, paper, gunpowder, and other substances."},
{"Chain Lightning", false, true, 48, 0, "\
Powerful capacitors unleash an electrical storm at your command, sending a\n\
chain of highly-damaging lightning through the air.  It has the power to\n\
injure several opponents before grounding itself."},
{"Tremor Pads", false, true, 40, 0, "\
Using tremor pads in your feet, you can cause a miniature earthquake.  The\n\
shockwave will damage enemies (particularly those digging underground), tear\n\
down walls, and churn the earth."},
{"Flashflood", false, true, 35, 0, "\
By drawing the moisture from the air, and synthesizing water from in-air\n\
elements, you can create a massive puddle around you.  The effects are more\n\
powerful when used near a body of water."},

{"max_bio_good - BUG", false, false, 0, 0, "\
This is a placeholder bionic.  If you are reading this, you have found a bug!"},

{"Electrical Discharge", false, false, 0, 0, "\
A malfunctioning bionic which occasionally discharges electricity through\n\
your body, causing pain and brief paralysis but no damage."},
{"Acidic Discharge", false, false, 0, 0, "\
A malfunctioning bionic which occasionally discharges acid into your muscles,\n\
causing sharp pain and minor damage."},
{"Electrical Drain", false, false, 0, 0, "\
A malfunctioning bionic.  It doesn't perform any useful function, but will\n\
occasionally draw power from your batteries."},
{"Noisemaker", false, false, 0, 0, "\
A malfunctioning bionic.  It will occasionally emit a loud burst of noise."},
{"Power Overload", false, false, 0, 0, "\
Damaged power circuits cause short-circuiting inside your muscles when your\n\
batteries are above 75%% capacity, causing greatly reduced strength.  This\n\
has no effect if you have no internal batteries."},
{"Wire-induced Stiffness", false, false, 0, 0, "\
Improperly installed wires cause a physical stiffness in most of your body,\n\
causing increased encumberance."}

};

// Why put this in a Big Switch?  Why not let bionics have pointers to
// functions, much like monsters and items?
//
// Well, because like diseases, which are also in a Big Switch, bionics don't
// share functions....
void player::activate_bionic(int b, game *g)
{
 assert(this == &game::active()->u); // \todo eliminate this precondition
 bionic& bio = my_bionics[b];
 const auto& bio_type = bionic::type[bio.id];
 int power_cost = bionic::type[bio.id].power_cost;
 if (weapon.type->id == itm_bio_claws && bio.id == bio_claws) power_cost = 0;
 if (power_level < power_cost) {
  if (bio.powered) {
   messages.add("Your %s powers down.", bio_type.name.c_str());
   bio.powered = false;
  } else
   messages.add("You cannot power your %s", bio_type.name.c_str());
  return;
 }

 if (bio.powered && bio.charge > 0) {
// Already-on units just lose a bit of charge
  bio.charge--;
 } else {
// Not-on units, or those with zero charge, have to pay the power cost
  if (bio_type.charge_time > 0) {
   bio.powered = true;
   bio.charge = bio_type.charge_time;
  }
  power_level -= power_cost;
 }

 switch (bio.id) {

 case bio_painkiller:
  pkill += 6;
  pain -= 2;
  if (pkill > pain) pkill = pain;
  break;

 case bio_nanobots:
  healall(4);
  break;

 case bio_resonator:
  g->sound(pos, 30, "VRRRRMP!");
  for (int i = pos.x - 1; i <= pos.x + 1; i++) {
   for (int j = pos.y - 1; j <= pos.y + 1; j++) {
    g->m.bash(i, j, 40);
    g->m.bash(i, j, 40);	// Multibash effect, so that doors &c will fall
    g->m.bash(i, j, 40);
    if (g->m.is_destructable(i, j) && rng(1, 10) >= 4)
     g->m.ter(i, j) = t_rubble;
   }
  }
  break;

 case bio_time_freeze:
  moves += 100 * power_level;
  power_level = 0;
  messages.add("Your speed suddenly increases!");
  if (one_in(3)) {
   messages.add("Your muscles tear with the strain.");
   hurt(g, bp_arms, 0, rng(5, 10));
   hurt(g, bp_arms, 1, rng(5, 10));
   hurt(g, bp_legs, 0, rng(7, 12));
   hurt(g, bp_legs, 1, rng(7, 12));
   hurt(g, bp_torso, 0, rng(5, 15));
  }
  if (one_in(5)) add_disease(DI_TELEGLOW, rng(MINUTES(5), MINUTES(40)));
  break;

 case bio_teleport:
  g->teleport();
  add_disease(DI_TELEGLOW, MINUTES(30));
  break;

// TODO: More stuff here (and bio_blood_filter)
 case bio_blood_anal:
  {
  constexpr const point analysis_offset(10,3);
  std::vector<std::string> bad;
  std::vector<std::string> good;
  if (has_disease(DI_FUNGUS)) bad.push_back("Fungal Parasite");
  if (has_disease(DI_DERMATIK)) bad.push_back("Insect Parasite");
  if (has_disease(DI_POISON)) bad.push_back("Poison");
  if (radiation > 0) bad.push_back("Irradiated");
  if (has_disease(DI_PKILL1)) good.push_back("Minor Painkiller");
  if (has_disease(DI_PKILL2)) good.push_back("Moderate Painkiller");
  if (has_disease(DI_PKILL3)) good.push_back("Heavy Painkiller");
  if (has_disease(DI_PKILL_L)) good.push_back("Slow-Release Painkiller");
  if (has_disease(DI_DRUNK)) good.push_back("Alcohol");
  if (has_disease(DI_CIG)) good.push_back("Nicotine");
  if (has_disease(DI_HIGH)) good.push_back("Intoxicant: Other");
  if (has_disease(DI_THC)) good.push_back("Intoxicant: THC");
  if (has_disease(DI_TOOK_PROZAC)) good.push_back("Prozac");
  if (has_disease(DI_TOOK_FLUMED)) good.push_back("Antihistamines");
  if (has_disease(DI_ADRENALINE)) good.push_back("Adrenaline Spike");
  const size_t ideal_lines = clamped_lb<1>(good.size()+bad.size());
  const int analysis_height = clamped_ub(ideal_lines + 2, VIEW - analysis_offset.y - 2); // \todo why not VIEW - analysis_offset.y?
  const int analysis_width = 40; // \todo why not SCREEN_WIDTH - analysis_offset.x
  std::unique_ptr<WINDOW, curses_full_delete> w(newwin(analysis_height, analysis_width, analysis_offset.y, analysis_offset.x));
  wborder(w.get(), LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
  if (good.empty() && bad.empty())
   mvwaddstrz(w.get(), 1, 1, c_white, "No effects.");
  else {
   for (int line = 1; line < analysis_height-1 && line <= good.size() + bad.size(); line++) {
    if (line <= bad.size())
     mvwaddstrz(w.get(), line, 1, c_red, bad[line - 1].c_str());
    else
     mvwaddstrz(w.get(), line, 1, c_green, good[line - 1 - bad.size()].c_str());
   }
  }
  }
  refresh();
  getch();
  break;

 case bio_blood_filter:
 {
	 static const decltype(DI_FUNGUS) drowse[] = {
		 DI_FUNGUS,
		 DI_POISON,
		 DI_PKILL1,
		 DI_PKILL2,
		 DI_PKILL3,
		 DI_PKILL_L,
		 DI_DRUNK,
		 DI_CIG,
		 DI_HIGH,
		 DI_THC,
		 DI_TOOK_PROZAC,
		 DI_TOOK_FLUMED,
		 DI_ADRENALINE
	 };

	 static auto decline = [&](disease& ill) { return cataclysm::any(drowse, ill.type); };
	 rem_disease(decline);
 }
  break;

 case bio_evap:
  if (query_yn("Drink directly? Otherwise you will need a container.")) {
   thirst -= 50;
   const int min_thirst = has_trait(PF_GOURMAND) ? -60 : -20;
   if (min_thirst > thirst) {
	 messages.add("You can't finish it all!");
	 thirst = min_thirst;
   }
  } else {
   auto& it = i_at(g->u.get_invlet("Choose a container:"));
   if (it.type == nullptr) {
    messages.add("You don't have that item!");
    power_level += bionic::type[bio_evap].power_cost;
   } else if (const auto cont = it.is_container()) {
	   if (it.volume_contained() + 1 > cont->contains) {
		   messages.add("There's no space left in your %s.", it.tname().c_str());
		   power_level += bionic::type[bio_evap].power_cost;
	   } else if (!(cont->flags & con_wtight)) {
		   messages.add("Your %s isn't watertight!", it.tname().c_str());
		   power_level += bionic::type[bio_evap].power_cost;
	   } else {
		   messages.add("You pour water into your %s.", it.tname().c_str());
		   it.put_in(item(item::types[itm_water], 0)); // XXX \todo should this be the correct origin time?
	   }
   } else {
    messages.add("That %s isn't a container!", it.tname().c_str());
    power_level += bionic::type[bio_evap].power_cost;
   }
  }
  break;

 case bio_lighter:
  g->draw();
  mvprintw(0, 0, "Torch in which direction?");
  {
  point dir(get_direction(input()));
  if (dir.x == -2) {
   messages.add("Invalid direction.");
   power_level += bionic::type[bio_lighter].power_cost;
   return;
  }
  dir += pos;
  if (!g->m.add_field(g, dir, fd_fire, 1))	// Unsuccessful.
   messages.add("You can't light a fire there.");
  }
  break;

 case bio_claws:
  if (weapon.type->id == itm_bio_claws) {
   messages.add("You withdraw your claws.");
   weapon = item::null;
  } else if (weapon.type->id != 0) {
   messages.add("Your claws extend, forcing you to drop your %s.", weapon.tname().c_str());
   g->m.add_item(pos, std::move(weapon));
   weapon = item(item::types[itm_bio_claws], 0);
   weapon.invlet = '#';
  } else {
   messages.add("Your claws extend!");
   weapon = item(item::types[itm_bio_claws], 0);
   weapon.invlet = '#';
  }
  break;

 case bio_blaster: {
  item tmp_item(std::move(weapon));
  weapon = item(item::types[itm_bio_blaster], 0);
  weapon.curammo = dynamic_cast<it_ammo*>(item::types[itm_bio_fusion]);
  weapon.charges = 1;
  g->refresh_all();
  g->plfire(false);
  weapon = std::move(tmp_item);
  }
  break;

 case bio_laser: {
  item tmp_item(std::move(weapon));
  weapon = item(item::types[itm_v29], 0);
  weapon.curammo = dynamic_cast<it_ammo*>(item::types[itm_laser_pack]);
  weapon.charges = 1;
  g->refresh_all();
  g->plfire(false);
  weapon = std::move(tmp_item);
  }
  break;

 case bio_emp:
  g->draw();
  mvprintw(0, 0, "Fire EMP in which direction?");
  {
  point dir(get_direction(input()));
  if (dir.x == -2) {
   messages.add("Invalid direction.");
   power_level += bionic::type[bio_emp].power_cost;
   return;
  }
  dir += pos;
  g->emp_blast(dir.x, dir.y);
  }
  break;

 case bio_hydraulics:
  messages.add("Your muscles hiss as hydraulic strength fills them!");
  break;

 case bio_water_extractor:
   {
   bool have_extracted = false;
   for (const auto& tmp : g->m.i_at(GPSpos)) {
	   if (tmp.type->id == itm_corpse && query_yn("Extract water from the %s", tmp.tname().c_str())) {
		   have_extracted = true;
		   auto& it = i_at(g->u.get_invlet("Choose a container:"));
		   if (nullptr == it.type) {
			   messages.add("You don't have that item!");
			   power_level += bionic::type[bio_water_extractor].power_cost;
		   } else if (const auto cont = it.is_container()) {
			   if (it.volume_contained() + 1 > cont->contains) {
				   messages.add("There's no space left in your %s.", it.tname().c_str());
				   power_level += bionic::type[bio_water_extractor].power_cost;
			   }
			   else {
				   messages.add("You pour water into your %s.", it.tname().c_str());
				   it.put_in(item(item::types[itm_water], 0));
			   }
		   } else {
			   messages.add("That %s isn't a container!", it.tname().c_str());
			   power_level += bionic::type[bio_water_extractor].power_cost;
		   }
		   break;
	   }
	   if (!have_extracted) power_level += bionic::type[bio_water_extractor].power_cost;	// We never chose a corpse
   }
  }
  break;

 case bio_magnet:
  for (int i = pos.x - 10; i <= pos.x + 10; i++) {
   for (int j = pos.y - 10; j <= pos.y + 10; j++) {
	auto& stack = g->m.i_at(i, j);
	if (stack.empty()) continue;
	const auto t = g->m.sees(i, j, pos, -1);
	std::vector<point> traj(line_to(i, j, pos, (t ? *t : 0)));
    traj.insert(traj.begin(), point(i, j));
    for (int k = 0; k < stack.size(); k++) {
     if (stack[k].made_of(IRON) || stack[k].made_of(STEEL)){
	  bool it_is_landed = false;
      item tmp_item = stack[k];
      g->m.i_rem(i, j, k);
	  point prior;
	  for (decltype(auto) pt : traj) {
		  if (monster* const z = g->mon(pt)) {
			  if (z->hurt(tmp_item.weight() * 2)) g->kill_mon(*z, this);
			  g->m.add_item(pt, std::move(tmp_item));
			  it_is_landed = true;
			  break;
		  } else if (pt != traj.front() && g->m.move_cost(pt) == 0) {
			  std::string snd;
			  g->m.bash(pt, tmp_item.weight() * 2, snd);
			  g->sound(pt, 12, snd); // C:Whales coincidentally SEE
			  g->m.add_item(prior, std::move(tmp_item));
			  it_is_landed = true;
			  break;
		  }
		  prior = pt;
	  }
      if (!it_is_landed) g->m.add_item(pos, std::move(tmp_item));
     }
    }
   }
  }
  break;

 case bio_lockpick:
  g->draw();
  mvprintw(0, 0, "Unlock in which direction?");
  {
  point dir(get_direction(input()));
  if (dir.x == -2) {
   messages.add("Invalid direction.");
   power_level += bionic::type[bio_lockpick].power_cost;
   return;
  }
  dir += pos;
  if (g->m.rewrite_test<t_door_locked, t_door_c>(dir)) {
   moves -= 40;
   messages.add("You unlock the door.");
  } else
   messages.add("You can't unlock that %s.", name_of(g->m.ter(dir)).c_str());
  }
  break;

 }
}

static void bionics_install_failure(pc* u, int success)
{
 success = abs(success) - rng(1, 10);
 int failure_level = 0;
 if (success <= 0) {
  messages.add("The installation fails without incident.");
  return;
 }

 while (success > 0) {
  failure_level++;
  success -= rng(1, 10);
 }

 int fail_type = rng(1, (failure_level > 5 ? 5 : failure_level));

 static constexpr const char* const no_go[] = {
  "You flub the installation",
  "You mess up the installation",
  "The installation fails",
  "The installation is a failure",
  "You screw up the installation"
 };

 std::string fail_text(no_go[rng(0, sizeof(no_go)/sizeof(*no_go)-1)]);

 if (fail_type == 3 && u->my_bionics.size() == 0)
  fail_type = 2; // If we have no bionics, take damage instead of losing some

 switch (fail_type) {
 case 1:
  fail_text += ", causing great pain.";
  u->pain += rng(failure_level * 3, failure_level * 6);
  break;

 case 2:
  fail_text += " and your body is damaged.";
  u->hurtall(rng(failure_level, failure_level * 2));
  break;

 case 3:
  fail_text += " and ";
  fail_text += (u->my_bionics.size() <= failure_level ? "all" : "some");
  fail_text += " of your existing bionics are lost.";
  for (int i = 0; i < failure_level && u->my_bionics.size() > 0; i++) {
   int rem = rng(0, u->my_bionics.size() - 1);
   EraseAt(u->my_bionics, rem);
  }
  break;

 case 4:
  fail_text += " and do damage to your genetics, causing mutation.";
  messages.add(fail_text); // Failure text comes BEFORE mutation text
  while (failure_level > 0) {
   u->mutate();
   failure_level -= rng(1, failure_level + 2);
  }
  return;	// So the failure text doesn't show up twice

 case 5:
 {
  fail_text += ", causing a faulty installation.";
  std::vector<bionic_id> valid;
  for (int i = max_bio_good + 1; i < max_bio; i++) {
   bionic_id id = bionic_id(i);
   if (!u->has_bionic(id)) valid.push_back(id);
  }
  if (valid.empty()) {	// We've got all the bad bionics!
   if (u->max_power_level > 0) {
    messages.add("You lose power capacity!");
    u->max_power_level = rng(0, u->max_power_level - 1);
   }
// TODO: What if we can't lose power capacity?  No penalty?
  } else {
   int index = rng(0, valid.size() - 1);
   u->add_bionic(valid[index]);
  }
 }
  break;
 }

 messages.add(fail_text);
}

// forcing the sole caller to handler the null pointer just complicates the "bypass code" for that critical data design error.
bool pc::install_bionics(const it_bionic* type)
{
	if (type == nullptr) {
#ifdef ZAIMONI_ASSERT_STD_LOGIC
		assert(0 && "Tried to install null bionic");	// for stacktrace in debug build
#endif
		debuglog("Tried to install null bionic");	// audit trail
		debugmsg("Tried to install null bionic");	// UI (player action failure)
		return false;
	}
	std::string bio_name = type->name.substr(5);	// Strip off "CBM: "
	std::unique_ptr<WINDOW, curses_full_delete> w(newwin(VIEW, SCREEN_WIDTH, 0, 0));

	int pl_skill = int_cur + sklevel[sk_electronics] * 4 +
		sklevel[sk_firstaid] * 3 +
		sklevel[sk_mechanics] * 2;

	int skint = pl_skill / 4;
	int skdec = ((pl_skill * 10) / 4) % 10;

	// Header text
	mvwaddstrz(w.get(), 0, 0, c_white, "Installing bionics:");
	mvwaddstrz(w.get(), 0, 20, type->color, bio_name.c_str());

	// Dividing bars
	draw_hline(w.get(), 1, c_ltgray, LINE_OXOX);
	draw_hline(w.get(), 21, c_ltgray, LINE_OXOX);

	// Init the list of bionics
	for (int i = 1; i < type->options.size(); i++) {
		bionic_id id = type->options[i];
		mvwaddstrz(w.get(), i + 2, 0, (has_bionic(id) ? c_ltred : c_ltblue), bionic::type[id].name.c_str());
	}
	// Helper text
	mvwprintz(w.get(), 2, 40, c_white, "Difficulty of this module: %d", type->difficulty);
	mvwprintz(w.get(), 3, 40, c_white, "Your installation skill:   %d.%d", skint, skdec);
	mvwaddstrz(w.get(), 4, 40, c_white, "Installation requires high intelligence,");
	mvwaddstrz(w.get(), 5, 40, c_white, "and skill in electronics, first aid, and");
	mvwaddstrz(w.get(), 6, 40, c_white, "mechanics (in that order of importance).");

	int chance_of_success = (100 * pl_skill) / (pl_skill + 4 * type->difficulty);

	mvwaddstrz(w.get(), 8, 40, c_white, "Chance of success:");

	nc_color col_suc;
	if (chance_of_success >= 95)
		col_suc = c_green;
	else if (chance_of_success >= 80)
		col_suc = c_ltgreen;
	else if (chance_of_success >= 60)
		col_suc = c_yellow;
	else if (chance_of_success >= 35)
		col_suc = c_ltred;
	else
		col_suc = c_red;

	mvwprintz(w.get(), 8, 59, col_suc, "%d%%", chance_of_success);

	mvwaddstrz(w.get(), 10, 40, c_white, "Failure may result in crippling damage,");
	mvwaddstrz(w.get(), 11, 40, c_white, "loss of existing bionics, genetic damage");
	mvwaddstrz(w.get(), 12, 40, c_white, "or faulty installation.");
	wrefresh(w.get());

	static constexpr const int BATTERY_AMOUNT = 4; // How much batteries increase your power

	if (type->id == itm_bionics_battery) {	// No selection list; just confirm
		mvwprintz(w.get(), 2, 0, h_ltblue, "Battery Level +%d", BATTERY_AMOUNT);
		mvwaddstrz(w.get(), 22, 0, c_ltblue, bionic::type[bio_batteries].description.c_str());
		int ch;
		wrefresh(w.get());
		do ch = getch();
		while (ch != 'q' && ch != '\n' && ch != KEY_ESCAPE);
		if (ch == '\n') {
			practice(sk_electronics, (100 - chance_of_success) * 1.5);
			practice(sk_firstaid, (100 - chance_of_success) * 1.0);
			practice(sk_mechanics, (100 - chance_of_success) * 0.5);
			int success = chance_of_success - rng(1, 100);
			if (success > 0) {
				messages.add("Successfully installed batteries.");
				max_power_level += BATTERY_AMOUNT;
			} else
				bionics_install_failure(this, success);
			refresh_all();
			return true;
		}
		refresh_all();
		return false;
	}

	int selection = 0;
	int ch;

	do {
		bionic_id id = type->options[selection];
		const auto& bio = bionic::type[id];
		mvwaddstrz(w.get(), 2 + selection, 0, (has_bionic(id) ? h_ltred : h_ltblue), bio.name.c_str());

		// Clear the bottom three lines...
		draw_hline(w.get(), 22, c_ltgray, ' ');
		draw_hline(w.get(), 23, c_ltgray, ' ');
		draw_hline(w.get(), 24, c_ltgray, ' ');

		// ...and then fill them with the description of the selected bionic
		mvwaddstrz(w.get(), 22, 0, c_ltblue, bio.description.c_str());

		wrefresh(w.get());
		ch = input();
		switch (ch) {

		case 'j':
			mvwaddstrz(w.get(), 2 + selection, 0, (has_bionic(id) ? c_ltred : c_ltblue), bio.name.c_str());
			if (selection == type->options.size() - 1) selection = 0;
			else selection++;
			break;

		case 'k':
			mvwaddstrz(w.get(), 2 + selection, 0, (has_bionic(id) ? c_ltred : c_ltblue), bio.name.c_str());
			if (selection == 0) selection = type->options.size() - 1;
			else selection--;
			break;

		}
		if (ch == '\n' && has_bionic(id)) {
			popup("You already have a %s!", bio.name.c_str());
			ch = 'a';
		}
	} while (ch != '\n' && ch != 'q' && ch != KEY_ESCAPE);

	if (ch == '\n') {
		practice(sk_electronics, (100 - chance_of_success) * 1.5);
		practice(sk_firstaid, (100 - chance_of_success) * 1.0);
		practice(sk_mechanics, (100 - chance_of_success) * 0.5);
		bionic_id id = type->options[selection];
		int success = chance_of_success - rng(1, 100);
		if (success > 0) {
			messages.add("Successfully installed %s.", bionic::type[id].name.c_str());
			add_bionic(id);
		}
		else
			bionics_install_failure(this, success);
		refresh_all();
		return true;
	}
	refresh_all();
	return false;
}
