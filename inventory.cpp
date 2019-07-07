#include <sstream>
#include "inventory.h"
#include "game.h"
#include "keypress.h"

using namespace cataclysm;

template<> std::vector<item> discard<std::vector<item> >::x(0);
const std::vector<item> inventory::nullstack(0);

DEFINE_ACID_ASSIGN_W_MOVE(inventory);

item& inventory::operator[] (int i)
{
 if (i < 0 || i >= items.size()) {
  debugmsg("Attempted to access item %d in an inventory (size %d)", i, items.size());
  return (discard<item>::x = item::null);
 }

 return items[i][0];
}

const item& inventory::operator[] (int i) const
{
	if (i < 0 || i >= items.size()) {
		debugmsg("Attempted to access item %d in an inventory (size %d)", i, items.size());
		return (discard<item>::x = item::null);
	}

	return items[i][0];
}

std::vector<item>& inventory::stack_at(int i)
{
 if (i < 0 || i >= items.size()) {
  debugmsg("Attempted to access stack %d in an inventory (size %d)",
           i, items.size());
  return (discard<std::vector<item> >::x = nullstack);
 }
 return items[i];
}

std::vector<item> inventory::const_stack(int i) const
{
 if (i < 0 || i >= items.size()) {
  debugmsg("Attempted to access stack %d in an inventory (size %d)",
           i, items.size());
  return nullstack;
 }
 return items[i];
}

#if DEAD_FUNC
std::vector<item> inventory::as_vector()
{
 std::vector<item> ret;
 for (const auto& stack : items) {
  for (const auto& it : stack) ret.push_back(it);
 }
 return ret;
}
#endif

int inventory::num_items() const
{
 int ret = 0;
 for (const auto& stack : items) ret += stack.size();

 return ret;
}

inventory& inventory::operator+= (const inventory &rhs)
{
 for (size_t i = 0; i < rhs.size(); i++) add_stack(rhs.const_stack(i));
 return *this;
}

inventory& inventory::operator+= (const std::vector<item> &rhs)
{
 for (const auto& it : rhs) add_item(it);
 return *this;
}

inventory& inventory::operator+= (const item &rhs)
{
 add_item(rhs);
 return *this;
}

// 2018-08-16: leave these out-of-line for now (until we have a speed problem prioritize executable size)
inventory inventory::operator+ (const inventory &rhs) const
{
 return inventory(*this) += rhs;
}

inventory inventory::operator+ (const std::vector<item> &rhs) const
{
 return inventory(*this) += rhs;
}

inventory inventory::operator+ (const item &rhs) const
{
 return inventory(*this) += rhs;
}

void inventory::add_stack(const std::vector<item>& newits)
{
 for (const auto& it : newits) add_item(it, true);
}

void inventory::add_item(item newit, bool keep_invlet)
{
 if (keep_invlet && !newit.invlet_is_okay()) assign_empty_invlet(newit); // Keep invlet is true, but invlet is invalid!
 if (newit.is_style()) return; // Styles never belong in our inventory.

 for (auto& stack : items) {
  item& first = stack[0];
  if (first.stacks_with(newit)) {
/*
   if (keep_invlet)
    first.invlet = newit.invlet;
   else
*/
    newit.invlet = first.invlet;
   stack.push_back(newit);
   return;
  } else if (keep_invlet && first.invlet == newit.invlet)
   assign_empty_invlet(first);
 }
 if (!newit.invlet_is_okay() || index_by_letter(newit.invlet) != -1) 
  assign_empty_invlet(newit);

 std::vector<item> newstack;
 newstack.push_back(newit);
 items.push_back(newstack);
}

void inventory::restack(player *p)
{
 inventory tmp;
 for (size_t i = 0; i < size(); i++) {
  for (const auto& it : items[i]) tmp.add_item(it);
 }
 clear();
 if (p) {
// Doing it backwards will preserve older items' invlet
/*
  for (size_t i = tmp.size() - 1; i >= 0; i--) {
   if (p->has_weapon_or_armor(tmp[i].invlet)) 
    tmp.assign_empty_invlet(tmp[i], p);
  }
*/
  for (size_t i = 0; i < tmp.size(); i++) {
   if (!tmp[i].invlet_is_okay() || p->has_weapon_or_armor(tmp[i].invlet)) {
    tmp.assign_empty_invlet(tmp[i], p);
	for (auto& it : tmp.items[i]) it.invlet = tmp[i].invlet;
   }
  }
 }
 for (size_t i = 0; i < tmp.size(); i++)
  items.push_back(tmp.items[i]);
}

void inventory::form_from_map(const map& m, point origin, int range)
{
 items.clear();
 for (int x = origin.x - range; x <= origin.x + range; x++) {
  for (int y = origin.y - range; y <= origin.y + range; y++) {
   for(auto& obj : m.i_at(x, y))
    if (!obj.made_of(LIQUID)) add_item(obj);
// Kludge for now!
   if (m.field_at(x, y).type == fd_fire) {
    item fire(item::types[itm_fire], 0);
    fire.charges = 1;
    add_item(fire);
   }
  }
 }
}

std::vector<item> inventory::remove_stack(int index)
{
 if (index < 0 || index >= items.size()) {
  debugmsg("Tried to remove_stack(%d) from an inventory (size %d)",
           index, items.size());
  return nullstack;
 }
 std::vector<item> ret(std::move(items[index]));
 items.erase(items.begin() + index);
 return ret;
}

item inventory::remove_item(int index)
{
 if (index < 0 || index >= items.size()) {
  debugmsg("Tried to remove_item(%d) from an inventory (size %d)",
           index, items.size());
  return item::null;
 }

 item ret = items[index][0];
 items[index].erase(items[index].begin());
 if (items[index].empty())
  items.erase(items.begin() + index);

 return ret;
}

item inventory::remove_item(int stack, int index)
{
 if (stack < 0 || stack >= items.size()) {
  debugmsg("Tried to remove_item(%d, %d) from an inventory (size %d)",
           stack, index, items.size());
  return item::null;
 } else if (index < 0 || index >= items[stack].size()) {
  debugmsg("Tried to remove_item(%d, %d) from an inventory (stack is size %d)",
           stack, index, items[stack].size());
  return item::null;
 }

 item ret = items[stack][index];
 items[stack].erase(items[stack].begin() + index);
 if (items[stack].empty())
  items.erase(items.begin() + stack);

 return ret;
}

item inventory::remove_item_by_letter(char ch)
{
 for (int i = 0; i < items.size(); i++) {
  if (items[i][0].invlet == ch) {
   if (items[i].size() > 1)
    items[i][1].invlet = ch;
   return remove_item(i);
  }
 }

 return item::null;
}

item& inventory::item_by_letter(char ch)
{
 for(auto& stack : items) { 
  auto& it = stack[0];
  if (it.invlet == ch) return it;
 }
 return (discard<item>::x = item::null);
}

int inventory::index_by_letter(char ch) const
{
 if (ch == KEY_ESCAPE) return -1;
 for (int i = 0; i < items.size(); i++) {
  if (items[i][0].invlet == ch) return i;
 }
 return -1;
}

int inventory::amount_of(itype_id it) const
{
 int count = 0;
 for (const auto& stack : items) {
  for (const auto& obj : stack) {
   if (obj.type->id == it) count++;
   for (const auto& within : obj.contents) {
    if (within.type->id == it) count++;
   }
  }
 }
 return count;
}

int inventory::charges_of(itype_id it) const
{
 int count = 0;
 for (const auto& stack : items) {
  for (const auto& obj : stack) {
   if (obj.type->id == it) count += (obj.charges < 0) ? 1 : obj.charges;
   for (const auto& within : obj.contents) {
    if (within.type->id == it) count += (within.charges < 0) ? 1 : within.charges;
   }
  }
 }
 return count;
}

void inventory::use_amount(itype_id it, int quantity, bool use_container)
{
 for (int i = 0; i < items.size() && quantity > 0; i++) {
  for (int j = 0; j < items[i].size() && quantity > 0; j++) {
// First, check contents
   bool used_item_contents = false;
   for (int k = 0; k < items[i][j].contents.size() && quantity > 0; k++) {
    if (items[i][j].contents[k].type->id == it) {
     quantity--;
     items[i][j].contents.erase(items[i][j].contents.begin() + k);
     k--;
     used_item_contents = true;
    }
   }
// Now check the item itself
   if (use_container && used_item_contents) {
    items[i].erase(items[i].begin() + j);
    j--;
    if (items[i].empty()) {
     items.erase(items.begin() + i);
     i--;
     j = 0;
    }
   } else if (items[i][j].type->id == it && quantity > 0) {
    quantity--;
    items[i].erase(items[i].begin() + j);
    j--;
    if (items[i].empty()) {
     items.erase(items.begin() + i);
     i--;
     j = 0;
    }
   }
  }
 }
}

void inventory::use_charges(itype_id it, int quantity)
{
 for (int i = 0; i < items.size() && quantity > 0; i++) {
  for (int j = 0; j < items[i].size() && quantity > 0; j++) {
// First, check contents
   for (int k = 0; k < items[i][j].contents.size() && quantity > 0; k++) {
    if (items[i][j].contents[k].type->id == it) {
     if (items[i][j].contents[k].charges <= quantity) {
      quantity -= items[i][j].contents[k].charges;
      if (items[i][j].contents[k].destroyed_at_zero_charges()) {
       items[i][j].contents.erase(items[i][j].contents.begin() + k);
       k--;
      } else items[i][j].contents[k].charges = 0;
     } else {
      items[i][j].contents[k].charges -= quantity;
      return;
     }
    }
   }
// Now check the item itself
   if (items[i][j].type->id == it) {
    if (items[i][j].charges <= quantity) {
     quantity -= items[i][j].charges;
     if (items[i][j].destroyed_at_zero_charges()) {
      items[i].erase(items[i].begin() + j);
      j--;
      if (items[i].empty()) {
       items.erase(items.begin() + i);
       i--;
       j = 0;
      }
     } else items[i][j].charges = 0;
    } else {
     items[i][j].charges -= quantity;
     return;
    }
   }
  }
 }
}
 
bool inventory::has_item(item *it) const
{
 for (int i = 0; i < items.size(); i++) {
  for (int j = 0; j < items[i].size(); j++) {
   if (it == &(items[i][j])) return true;
   for (int k = 0; k < items[i][j].contents.size(); k++) {
    if (it == &(items[i][j].contents[k])) return true;
   }
  }
 }
 return false;
}

void inventory::assign_empty_invlet(item &it, player *p)
{
 for (int ch = 'a'; ch <= 'z'; ch++) {
  if (index_by_letter(ch) == -1 && (!p || !p->has_weapon_or_armor(ch))) {
   it.invlet = ch;
   return;
  }
 }
 for (int ch = 'A'; ch <= 'Z'; ch++) {
  if (index_by_letter(ch) == -1 && (!p || !p->has_weapon_or_armor(ch))) {
   it.invlet = ch;
   return;
  }
 }
 it.invlet = '`';
}
