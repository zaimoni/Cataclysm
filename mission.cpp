#include "mission.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "json.h"
#include "recent_msg.h"

int mission::next_id = mission::MIN_ID;

void mission::global_reset() {
    next_id = MIN_ID;
}

void mission::global_fromJSON(const cataclysm::JSON& src)
{
    if (src.has_key("next_id")) {
        const auto& _next = src["next_id"];
        if (_next.has_key("mission") && fromJSON(_next["mission"], next_id) && MIN_ID <= next_id) return;
    }
    next_id = MIN_ID;
}

void mission::global_toJSON(cataclysm::JSON& dest)
{
    if (MIN_ID < next_id) dest.set("mission", std::to_string(next_id));
}


static const char* JSON_transcode[] = {
    "GET_ANTIBIOTICS",
    "GET_SOFTWARE",
    "GET_ZOMBIE_BLOOD_ANAL",
    "RESCUE_DOG",
    "KILL_ZOMBIE_MOM",
    "REACH_SAFETY",
    "GET_BOOK"
};

DEFINE_JSON_ENUM_SUPPORT_HARDCODED_NONZERO(mission_id, JSON_transcode)

mission* mission::from_id(int uid) { return game::active()->find_mission(uid); }

mission mission_type::create(game *g, int npc_id)
{
 mission ret;
 ret.assign_id();
 ret.type = this;
 ret.npc_id = npc_id;
 ret.item_id = item_id;
 ret.value = value;
 ret.follow_up = follow_up;

 if (deadline_low != 0 || deadline_high != 0)
  ret.deadline = int(messages.turn) + rng(deadline_low, deadline_high);
 else
  ret.deadline = 0;

 return ret;
}

void mission::step_complete(const int stage)
{
    step = stage;
    switch (type->goal) {
    case MGOAL_FIND_ITEM:
    case MGOAL_FIND_MONSTER:
    case MGOAL_KILL_MONSTER: {
        if (auto _npc = npc::find_alive_r(npc_id)) target = overmap::toOvermap(_npc->GPSpos);
        else target = _ref<decltype(target)>::invalid;
    } break;
    }
}

bool mission::is_complete(const player& u, const int _npc_id) const
{
    switch (type->goal) {
    case MGOAL_GO_TO: return rl_dist(overmap::toOvermap(u.GPSpos), target) <= 1;

    case MGOAL_FIND_ITEM:
        if (!u.has_amount(type->item_id, 1)) return false;
        return 0 >= npc_id || npc_id == _npc_id;

    case MGOAL_FIND_ANY_ITEM:
        if (!u.has_mission_item(uid)) return false;
        return 0 >= npc_id || npc_id == _npc_id;

    case MGOAL_FIND_MONSTER:
        if (0 < npc_id && npc_id != _npc_id) return false;
        return game::active_const()->find_first([=](const monster& _mon) { return _mon.mission_id == uid; });

    case MGOAL_FIND_NPC: return (npc_id == _npc_id);
    case MGOAL_KILL_MONSTER: return 1 <= step;
    default: return false;
    }
}

void mission::fail()
{
    failed = true;
    auto g = game::active();
    (*type->fail)(g, this);
    g->u.fail(*this);    // \todo ultimately should check all players and NPCs
}

std::string mission_dialogue (mission_id id, talk_topic state)
{
 switch (id) {

 case MISSION_GET_ANTIBIOTICS:
  switch (state) {
   case TALK_MISSION_DESCRIBE:
    switch (rng(1, 4)) {
     case 1: return "Hey <name_g>... I really need your help...";
     case 2: return "<swear!><punc> I'm hurting...";
     case 3: return "This infection is bad, <very> bad...";
     case 4: return "Oh god, it <swear> hurts...";
    }
    break;
   case TALK_MISSION_OFFER:
    return "\
I'm infected.  Badly.  I need you to get some antibiotics for me...";
   case TALK_MISSION_ACCEPTED:
    return "\
Oh, thank god, thank you so much!  I won't last more than a couple of days, \
so hurry...";
   case TALK_MISSION_REJECTED:
    return "\
What?!  Please, <ill_die> without your help!";
   case TALK_MISSION_ADVICE:
    return "\
There's a town nearby.  Check pharmacies; it'll be behind the counter.";
   case TALK_MISSION_INQUIRE:
    return "Find any antibiotics yet?";
   case TALK_MISSION_SUCCESS:
    return "Oh thank god!  I'll be right as rain in no time.";
   case TALK_MISSION_SUCCESS_LIE:
    return "What?!  You're lying, I can tell!  Ugh, forget it!";
   case TALK_MISSION_FAILURE:
    return "How am I not dead already?!";
  }
  break;

 case MISSION_GET_SOFTWARE:
  switch (state) {
   case TALK_MISSION_DESCRIBE:
    return "Oh man, I can't believe I forgot to download it...";
   case TALK_MISSION_OFFER:
    return "There's some important software on my computer that I need on USB.";
   case TALK_MISSION_ACCEPTED:
    return "\
Thanks!  Just pull the data onto this USB drive and bring it to me.";
   case TALK_MISSION_REJECTED:
    return "Seriously?  It's an easy job...";
   case TALK_MISSION_ADVICE:
    return "Take this USB drive.  Use the console, and download the software.";
   case TALK_MISSION_INQUIRE:
    return "So, do you have my software yet?";
   case TALK_MISSION_SUCCESS:
    return "Excellent, thank you!";
   case TALK_MISSION_SUCCESS_LIE:
    return "What?!  You liar!";
   case TALK_MISSION_FAILURE:
    return "Wow, you failed?  All that work, down the drain...";
  }
  break;

 case MISSION_GET_ZOMBIE_BLOOD_ANAL:
  switch (state) {
   case TALK_MISSION_DESCRIBE:
    return "\
It could be very informative to perform an analysis of zombie blood...";
   case TALK_MISSION_OFFER:
    return "\
I need someone to get a sample of zombie blood, take it to a hospital, and \
perform a centrifuge analysis of it.";
   case TALK_MISSION_ACCEPTED:
    return "\
Excellent.  Take this vacutainer; once you've produced a zombie corpse, use it \
to extrace blood from the body, then take it to a hospital for analysis.";
   case TALK_MISSION_REJECTED:
    return "\
Are you sure?  The scientific value of that blood data could be priceless...";
   case TALK_MISSION_ADVICE:
    return "\
The centrifuge is a bit technical; you might want to study up on the usage of \
computers before completing that part.";
   case TALK_MISSION_INQUIRE:
    return "Well, do you have the data yet?";
   case TALK_MISSION_SUCCESS:
    return "Excellent!  This may be the key to removing the infection.";
   case TALK_MISSION_SUCCESS_LIE:
    return "Wait, you couldn't possibly have the data!  Liar!";
   case TALK_MISSION_FAILURE:
    return "What a shame, that data could have proved invaluable...";
  }
  break;
  
 case MISSION_RESCUE_DOG:
  switch (state) {
   case TALK_MISSION_DESCRIBE:
    return "Oh, my poor puppy...";

   case TALK_MISSION_OFFER:
    return "\
I left my poor dog in a house, not far from here.  Can you retrieve it?";

   case TALK_MISSION_ACCEPTED:
    return "\
Thank you!  Please hurry back!";

   case TALK_MISSION_REJECTED:
    return "\
Please, think of my poor little puppy!";

   case TALK_MISSION_ADVICE:
    return "\
Take my dog whistle; if the dog starts running off, blow it and he'll return \
to your side.";

   case TALK_MISSION_INQUIRE:
    return "\
Have you found my dog yet?";

   case TALK_MISSION_SUCCESS:
    return "\
Thank you so much for finding him!";

   case TALK_MISSION_SUCCESS_LIE:
    return "What?!  You're lying, I can tell!  Ugh, forget it!";

   case TALK_MISSION_FAILURE:
    return "Oh no!  My poor puppy...";

  }
  break;

 case MISSION_KILL_ZOMBIE_MOM:
  switch (state) {
   case TALK_MISSION_DESCRIBE:
    return "Oh god, I can't believe it happened...";
   case TALK_MISSION_OFFER:
    return "\
My mom... she's... she was killed, but then she just got back up... she's one \
of those things now.  Can you put her out of her misery for me?";
   case TALK_MISSION_ACCEPTED:
    return "Thank you... she would've wanted it this way.";
   case TALK_MISSION_REJECTED:
    return "Please reconsider, I know she's suffering...";
   case TALK_MISSION_ADVICE:
    return "Find a gun if you can, make it quick...";
   case TALK_MISSION_INQUIRE:
    return "Well...?  Did you... finish things for my mom?";
   case TALK_MISSION_SUCCESS:
    return "Thank you.  I couldn't rest until I knew that was finished.";
   case TALK_MISSION_SUCCESS_LIE:
    return "What?!  You're lying, I can tell!  Ugh, forget it!";
   case TALK_MISSION_FAILURE:
    return "Really... that's too bad.";
  }
  break;
 }
 return "Someone forgot to code this mission's messages!";	// break statements bypass the default case
}

