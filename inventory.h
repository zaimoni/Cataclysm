#ifndef _INVENTORY_H_
#define _INVENTORY_H_

#include "item.h"
#include "enums.h"
#include <string>
#include <vector>

class game;
class map;
class player;
struct GPS_loc;

class inventory
{
 public:
  inventory() = default;
  inventory(const cataclysm::JSON& src);
  inventory(const inventory& src) = default;
  inventory(inventory&& src) = default;
  ~inventory() = default;
  inventory& operator=(const inventory &rhs);
  inventory& operator=(inventory &&rhs) = default;
  friend cataclysm::JSON toJSON(const inventory& src);

  inventory(const GPS_loc& origin, int range); // map inventory

  item& operator[] (int i);
  const item& operator[] (int i) const;
  std::vector<item>& stack_at(int i);
  const std::vector<item>& stack_at(int i) const { return const_cast<inventory*>(this)->stack_at(i); };
  std::vector<item> const_stack(int i) const;
// std::vector<item> as_vector();	// dead function
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
#if DEAD_FUNC
  void push_back(const std::vector<item>& newits) { add_stack(newits); }
#endif
  void add_item(const item& newit, bool keep_invlet = false) { _add_item(item(newit), keep_invlet); }
  void add_item(item&& newit, bool keep_invlet = false) { _add_item(std::move(newit), keep_invlet); }
  void push_back(const item& newit) { add_item(newit); }
  void push_back(item&& newit) { add_item(std::move(newit)); }

/* Check all items for proper stacking, rearranging as needed
 * player pointer is not necessary, but if supplied, will ensure no overlap with
 * the player's worn items / weapon
 */
  void restack(player* p = nullptr);

  void destroy_stack(int index);
  item  remove_item(int index);
  item  remove_item(int stack, int index);
  item  remove_item_by_letter(char ch);
  int   index_by_letter(char ch) const;

  /// <summary>Inverts operator[]</summary>
  /// <returns>-1 on failure, or valid index for operator[]</returns>
  int index_of(const item& it) const;

// Below, "amount" refers to quantity
//        "charges" refers to charges
  int  amount_of (itype_id it) const;
  int  charges_of(itype_id it) const;

  void use_amount (itype_id it, int quantity, bool use_container = false);
  unsigned int use_charges(itype_id it, int quantity);

  bool has_amount(itype_id it, int quantity) const { return amount_of(it) >= quantity; }
  bool has_charges(itype_id it, int quantity) const { return charges_of(it) >= quantity; }
  bool has_item(item *it) const; // Looks for a specific item

 private:
  std::vector< std::vector<item> > items;

  void assign_empty_invlet(item &it, player* p = nullptr);
  void _add_item(item&& newit, bool keep_invlet);
};

#endif
