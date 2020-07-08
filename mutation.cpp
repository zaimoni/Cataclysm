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
// If we tried once with a non-NULL category, and couldn't find anything valid
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
        for (int j = 0; j < bps.size(); j++) {
            if ((dynamic_cast<const it_armor*>(p.worn[i].type))->covers& mfb(bps[j])) {
                if (destroy) {
                    if (is_u) messages.add("Your %s is destroyed!", p.worn[i].tname().c_str());
                }
                else {
                    if (is_u) messages.add("Your %s is pushed off.", p.worn[i].tname().c_str());
                    g->m.add_item(p.pos, p.worn[i]);
                }
                EraseAt(p.worn, i);
                i--;
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
 if (has_child_flag(mut)) {
  remove_child_flag(mut);
  return;
 }

 std::vector<pl_flag> cancel(mutation_branch::data[mut].cancels);
 int ub = cancel.size();
 while (0 <= --ub) if (!has_trait(cancel[ub])) cancel.erase(cancel.begin() + ub);

 if (!cancel.empty()) {
  remove_mutation(cancel[rng(0, cancel.size() - 1)]);
  return;
 }

 bool has_prereqs = false;
 std::vector<pl_flag> prereq(mutation_branch::data[mut].prereqs);
 for (int i = 0; i < prereq.size(); i++) {
  if (has_trait(prereq[i])) {
    has_prereqs = true;
    break;
  }
 }
 if (!has_prereqs && !prereq.empty()) {
  mutate_towards(prereq[rng(0, prereq.size() - 1)]);
  return;
 }

 toggle_trait(mut);
 if (auto replacing = regress_mutation(mut, *this)) {  // Check if one of the prereqs that we have TURNS INTO this one
  messages.add("Your %s turns into %s!", mutation_branch::traits[replacing].name.c_str(), mutation_branch::traits[mut].name.c_str());
  toggle_trait(replacing);
 } else
  messages.add("You gain %s!", mutation_branch::traits[mut].name.c_str());
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

void player::remove_mutation(pl_flag mut)
{
// Check if there's a prereq we should shrink back into
 pl_flag replacing = PF_NULL;
 for(const auto pre : mutation_branch::data[mut].prereqs) {
  for(const auto tmp : mutation_branch::data[pre].replacements) {
   if (tmp == mut) {
    replacing = pre;
    break;
   }
  }
 }

 toggle_trait(mut);
 if (replacing != PF_NULL) {
  messages.add("Your %s turns into %s.", mutation_branch::traits[mut].name.c_str(), mutation_branch::traits[replacing].name.c_str());
  toggle_trait(replacing);
  mutation_loss_effect(*this, mut);
  mutation_effect(*this, replacing);
 } else {
  messages.add("You lose your %s.", mutation_branch::traits[mut].name.c_str());
  mutation_loss_effect(*this, mut);
 }

}

bool player::has_child_flag(pl_flag flag) const
{
 for(const auto tmp : mutation_branch::data[flag].replacements) {
  if (has_trait(tmp) || has_child_flag(tmp))	// XXX depth-first search
   return true;
 }
 return false;
}

void player::remove_child_flag(pl_flag flag)
{
 for(const auto tmp : mutation_branch::data[flag].replacements) {
  if (has_trait(tmp)) {
   remove_mutation(tmp);
   return;
  } else if (has_child_flag(tmp)) {
   remove_child_flag(tmp);
   return;
  }
 }
}
