#include "player.h"
#include "mutation.h"
#include "game.h"
#include "rng.h"
#include "stl_typetraits.h"
#include "recent_msg.h"

void player::mutate()
{
 bool force_bad = one_in(3); // 33% chance!
 if (has_trait(PF_ROBUST) && force_bad && one_in(3)) force_bad = false; // 11% chance!

// First, see if we should ugrade/extend an existing mutation
 std::vector<pl_flag> upgrades;
 for (int i = 1; i < PF_MAX2; i++) {
  if (has_trait(i)) {
   for(const auto tmp : mutation_branch::data[i].replacements) {
    if (!has_trait(tmp) && !has_child_flag(tmp) &&
        (!force_bad || mutation_branch::traits[tmp].points <= 0))
     upgrades.push_back(tmp);
   }
   for(const auto tmp : mutation_branch::data[i].additions) {
    if (!has_trait(tmp) && !has_child_flag(tmp) &&
        (!force_bad || mutation_branch::traits[tmp].points <= 0))
     upgrades.push_back(tmp);
   }
  }
 }

 if (const auto upgrade_size = upgrades.size();  0 < upgrade_size && rng(0, upgrade_size + 4) < upgrade_size) {
     mutate_towards(upgrades[rng(0, upgrade_size - 1)]);
     return;
 }

// Next, see if we should mutate within a given category
 mutation_category cat = MUTCAT_NULL;

 int total = 0, highest = 0;
 for (int i = 0; i < NUM_MUTATION_CATEGORIES; i++) {
  total += mutation_category_level[i];
  if (mutation_category_level[i] > highest) {
   cat = mutation_category(i);
   highest = mutation_category_level[i];
  }
 }

 if (rng(0, total) > highest) cat = MUTCAT_NULL; // Not a strong enough pull, just mutate something random!

 std::vector<pl_flag> valid; // Valid mutations
 bool first_pass = (cat != MUTCAT_NULL);

 do {
// If we tried once with a non-null category, and couldn't find anything valid
// there, try again with MUTCAT_NULL
  if (cat != MUTCAT_NULL && !first_pass)
   cat = MUTCAT_NULL;

  if (cat == MUTCAT_NULL) { // Pull the full list
   for (int i = 1; i < PF_MAX2; i++) {
    if (mutation_branch::data[i].valid)
     valid.push_back( pl_flag(i) );
   }
  } else // Pull the category's list
   valid = mutations_from_category(cat);

// Remove anything we already have, or that we have a child of, or that's
// positive and we're forcing bad
  int ub = valid.size();
  while (0 <= --ub) {
      auto mut = valid[ub];
      if (has_trait(mut) || has_child_flag(mut) ||
          (force_bad && mutation_branch::traits[mut].points > 0)) {
          valid.erase(valid.begin() + ub);
      }
  }

  if (!valid.empty()) break;
  first_pass = false; // So we won't repeat endlessly
   
 } while (cat != MUTCAT_NULL);


 if (!valid.empty()) mutate_towards(valid[rng(0, valid.size() - 1)]);
}

// mutation_effect handles things like destruction of armor, etc.
static void mutation_effect(player& p, pl_flag mut)
{
    auto g = game::active();
    bool is_u = (&p == &(g->u));
    bool destroy = false, skip_cloth = false;
    std::vector<body_part> bps;

    switch (mut) {
        // Push off gloves
    case PF_WEBBED:
    case PF_ARM_TENTACLES:
    case PF_ARM_TENTACLES_4:
    case PF_ARM_TENTACLES_8:
        bps.push_back(bp_hands);
        break;
        // Destroy gloves
    case PF_TALONS:
        destroy = true;
        bps.push_back(bp_hands);
        break;
        // Destroy mouthwear
    case PF_BEAK:
    case PF_MANDIBLES:
        destroy = true;
        bps.push_back(bp_mouth);
        break;
        // Destroy footwear
    case PF_HOOVES:
        destroy = true;
        bps.push_back(bp_feet);
        break;
        // Destroy torsowear
    case PF_SHELL:
        destroy = true;
        bps.push_back(bp_torso);
        break;
        // Push off all helmets
    case PF_HORNS_CURLED:
    case PF_CHITIN3:
        bps.push_back(bp_head);
        break;
        // Push off non-cloth helmets
    case PF_HORNS_POINTED:
    case PF_ANTENNAE:
    case PF_ANTLERS:
        skip_cloth = true;
        bps.push_back(bp_head);
        break;

    case PF_STR_UP:
        p.str_max++;
        break;
    case PF_STR_UP_2:
        p.str_max += 2;
        break;
    case PF_STR_UP_3:
        p.str_max += 4;
        break;
    case PF_STR_UP_4:
        p.str_max += 7;
        break;

    case PF_DEX_UP:
        p.dex_max++;
        break;
    case PF_DEX_UP_2:
        p.dex_max += 2;
        break;
    case PF_DEX_UP_3:
        p.dex_max += 4;
        break;
    case PF_DEX_UP_4:
        p.dex_max += 7;
        break;

    case PF_INT_UP:
        p.int_max++;
        break;
    case PF_INT_UP_2:
        p.int_max += 2;
        break;
    case PF_INT_UP_3:
        p.int_max += 4;
        break;
    case PF_INT_UP_4:
        p.int_max += 7;
        break;

    case PF_PER_UP:
        p.per_max++;
        break;
    case PF_PER_UP_2:
        p.per_max += 2;
        break;
    case PF_PER_UP_3:
        p.per_max += 4;
        break;
    case PF_PER_UP_4:
        p.per_max += 7;
        break;
    }

    for (int i = 0; i < p.worn.size(); i++) {
        item& armor = p.worn[i];
        const auto coverage = (dynamic_cast<const it_armor*>(armor.type))->covers;
        for (int j = 0; j < bps.size(); j++) {
            if (coverage & mfb(bps[j])) {
                if (destroy) {
                    if (is_u) messages.add("Your %s is destroyed!", armor.tname().c_str());
                } else {
                    if (is_u) messages.add("Your %s is pushed off.", armor.tname().c_str());
                    g->m.add_item(p.pos, std::move(armor));
                }
                EraseAt(p.worn, i);
                i--;
                break;
            }
        }
    }
}

static pl_flag regress_mutation(pl_flag mut, const player& p)
{
    for (const auto pre : mutation_branch::data[mut].prereqs) {
        if (p.has_trait(pre)) {
            for (const auto tmp : mutation_branch::data[pre].replacements) {
                if (tmp == mut) return pre;
            }
        }
    }
    return PF_NULL;
}

void player::mutate_towards(pl_flag mut)
{
retry:
 if (remove_child_flag(mut)) return;

 if (!mutation_branch::data[mut].cancels.empty()) {
     auto cancel(mutation_branch::data[mut].cancels);
     int ub = cancel.size();
     while (0 <= --ub) if (!has_trait(cancel[ub])) cancel.erase(cancel.begin() + ub);

     if (!cancel.empty()) {
         remove_mutation(cancel[rng(0, cancel.size() - 1)]);
         return;
     }
 }

 const auto& prereq = mutation_branch::data[mut].prereqs;
 if (!prereq.empty()) {
     bool has_prereqs = false;
     for (decltype(auto) pre : prereq) {
         if (has_trait(pre)) {
             has_prereqs = true;
             break;
         }
     }
     if (!has_prereqs) { // tail-recurse to pre-requisite
         mut = prereq[rng(0, prereq.size() - 1)];
         goto retry;
     }
 }

 auto replacing = regress_mutation(mut, *this);

 static auto me_replace = [&]() {
     return std::string("Your ") + mutation_branch::traits[replacing].name + " turns into " + mutation_branch::traits[mut].name + "!";
 };
 static auto other_replace = [&]() {
     return possessive() + mutation_branch::traits[replacing].name + " turns into " + mutation_branch::traits[mut].name + "!";
 };

 static auto me_gain = [&]() {
     return std::string("You gain ") + mutation_branch::traits[mut].name + "!";
 };
 static auto other_gain = [&]() {
     return name + " gains " + mutation_branch::traits[mut].name + "!";
 };

 toggle_trait(mut);
 if (replacing) {  // Check if one of the prereqs that we have TURNS INTO this one
  if_visible_message(me_replace, other_replace);
  toggle_trait(replacing);
 } else
  if_visible_message(me_gain, other_gain);
 mutation_effect(*this, mut);

// Weight us towards any categories that include this mutation
 for (int i = 0; i < NUM_MUTATION_CATEGORIES; i++) {
  if (cataclysm::any(mutations_from_category(mutation_category(i)), mut))
   mutation_category_level[i] += 8;
  else if (mutation_category_level[i] > 0 && !one_in(mutation_category_level[i]))
   mutation_category_level[i]--;
 }

}

// mutation_loss_effect handles what happens when you lose a mutation
static void mutation_loss_effect(player& p, pl_flag mut)
{
    switch (mut) {
    case PF_STR_UP:
        p.str_max--;
        break;
    case PF_STR_UP_2:
        p.str_max -= 2;
        break;
    case PF_STR_UP_3:
        p.str_max -= 4;
        break;
    case PF_STR_UP_4:
        p.str_max -= 7;
        break;

    case PF_DEX_UP:
        p.dex_max--;
        break;
    case PF_DEX_UP_2:
        p.dex_max -= 2;
        break;
    case PF_DEX_UP_3:
        p.dex_max -= 4;
        break;
    case PF_DEX_UP_4:
        p.dex_max -= 7;
        break;

    case PF_INT_UP:
        p.int_max--;
        break;
    case PF_INT_UP_2:
        p.int_max -= 2;
        break;
    case PF_INT_UP_3:
        p.int_max -= 4;
        break;
    case PF_INT_UP_4:
        p.int_max -= 7;
        break;

    case PF_PER_UP:
        p.per_max--;
        break;
    case PF_PER_UP_2:
        p.per_max -= 2;
        break;
    case PF_PER_UP_3:
        p.per_max -= 4;
        break;
    case PF_PER_UP_4:
        p.per_max -= 7;
        break;
    }
}

bool player::remove_mutation(pl_flag mut)
{
 if (!has_trait(mut)) return false;

 static auto me_lose = [&]() {
     return std::string("You lose your ") + mutation_branch::traits[mut].name + ".";
 };

 static auto other_lose = [&]() {
     return name+" loses their " + mutation_branch::traits[mut].name + ".";
 };

// Check if there's a prereq we should shrink back into
 pl_flag replacing = regress_mutation(mut, *this);

 static auto me_replace = [&]() {
     return std::string("Your ") + mutation_branch::traits[mut].name + " turns into " + mutation_branch::traits[replacing].name + ".";
 };

 static auto other_replace = [&]() {
     return possessive() + mutation_branch::traits[mut].name + " turns into " + mutation_branch::traits[replacing].name + ".";
 };

 toggle_trait(mut);
 if (replacing != PF_NULL) {
     if_visible_message(me_replace, other_replace);
     toggle_trait(replacing);
     mutation_loss_effect(*this, mut);
     mutation_effect(*this, replacing);
 } else {
     if_visible_message(me_lose, other_lose);
     mutation_loss_effect(*this, mut);
 }

 return true;
}

// these two could be sunk into static functions, but would have less legible source code afterwards
pl_flag player::has_child_flag(pl_flag flag) const
{
 for(const auto tmp : mutation_branch::data[flag].replacements) {
  // XXX depth-first search
  if (has_trait(tmp)) return tmp;
  if (pl_flag ret = has_child_flag(tmp)) return ret;
 }
 return PF_NULL;
}

bool player::remove_child_flag(pl_flag flag)
{
 if (pl_flag remove = has_child_flag(flag)) return remove_mutation(remove);
 return false;
}
