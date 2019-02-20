#ifndef _INVENTORY_H_
#define _INVENTORY_H_

#include "item.h"
#include <string>
#include <vector>

class game;
class map;

class inventory
{
 public:
  inventory() = default;
  inventory(const inventory& src) = default;
  ~inventory() = default;
  inventory& operator=  (const inventory &rhs) = default;
  inventory& operator=  (inventory &&rhs) = default;

  item& operator[] (int i);
  const item& operator[] (int i) const;
  std::vector<item>& stack_at(int i);
  const std::vector<item>& stack_at(int i) const { return const_cast<inventory*>(this)->stack_at(i); };
  std::vector<item> const_stack(int i) const;
  std::vector<item> as_vector();
  size_t size() const { return items.size(); }
  int num_items() const;

  inventory& operator+= (const inventory &rhs);
  inventory& operator+= (const item &rhs);
  inventory& operator+= (const std::vector<item> &rhs);
  inventory  operator+  (const inventory &rhs) const;
  inventory  operator+  (const item &rhs) const;
  inventory  operator+  (const std::vector<item> &rhs) const;

  void clear() { items.clear(); }
  void add_stack(const std::vector<item>& newits);
  void push_back(const std::vector<item>& newits) { add_stack(newits); }
  void add_item (item newit, bool keep_invlet = false);
  void add_item_keep_invlet(item newit) { add_item(newit, true); }
  void push_back(item newit) { add_item(newit); }

/* Check all items for proper stacking, rearranging as needed
 * game pointer is not necessary, but if supplied, will ensure no overlap with
 * the player's worn items / weapon
 */
  void restack(player *p = NULL);

  void form_from_map(map& m, point origin, int distance);

  std::vector<item> remove_stack(int index);
  item  remove_item(int index);
  item  remove_item(int stack, int index);
  item  remove_item_by_letter(char ch);
  item& item_by_letter(char ch);
  int   index_by_letter(char ch) const;

// Below, "amount" refers to quantity
//        "charges" refers to charges
  int  amount_of (itype_id it) const;
  int  charges_of(itype_id it) const;

  void use_amount (itype_id it, int quantity, bool use_container = false);
  void use_charges(itype_id it, int quantity);

  bool has_amount(itype_id it, int quantity) { return (amount_of(it) >= quantity); }
  bool has_charges(itype_id it, int quantity) { return (charges_of(it) >= quantity); }
  bool has_item(item *it) const; // Looks for a specific item

/* TODO: This stuff, I guess?
  std::string save();
  void load(std::string data);
*/

 private:
  static const std::vector<item> nullstack;
  std::vector< std::vector<item> > items;

  void assign_empty_invlet(item &it, player *p = NULL);
};

#endif
