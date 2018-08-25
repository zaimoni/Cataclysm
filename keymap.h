#ifndef KEYMAP_H
#define KEYMAP_H 1

#include <vector>
#include <map>

template<class T>
class keymap
{
private:
	std::map<char, T> _x;
public:
	keymap() = default;
	~keymap() = default;

	T translate(char ch) {
	  const auto i = _x.find(ch);
	  return (i == _x.end()) ? T(0) : i->second;	// possibly _x[ch] instead
	}

	std::vector<char> translate(T src) {
      std::vector<char> ret;
	  for (const auto it : _x) {
	    if (src == it.second) ret.push_back(it.first);
	  }
      return ret;
	}

	bool set(char ch, T val) {	// only sets if not already set
      bool ret = (_x.find(ch) == _x.end());
	  if (ret) _x[ch] = val;
	  return ret;
	}

	void unset(T val) {
restart:
	 auto it = _x.begin();
	 while(it != _x.end()) {
	   if (val == it->second) {
		 _x.erase(it);
		 goto restart;
	   }
	   ++it;
	 }
	}

	template<class act> void forall(act action) {
	  for(const auto it : _x) action(it.first, it.second);
	}
};

#endif
