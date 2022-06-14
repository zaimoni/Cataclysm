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
	  for (const auto& vt : _x) {
	    if (src == vt.second) ret.push_back(vt.first);
	  }
	  return ret;
	}

	bool set(char ch, T val) {	// only sets if not already set
	  bool ret = (_x.find(ch) == _x.end());
	  if (ret) _x[ch] = val;
	  return ret;
	}

	void unset(T val) {
	  auto it = _x.begin();
	  while (it != _x.end()) {
	    if (val == it->second) {
	      it = _x.erase(it);
	    } else {
	      ++it;
	    }
	  }
	}

	template<class act> void forall(act action) {
	  for(const auto& vt : _x) action(vt.first, vt.second);
	}
};

#endif
