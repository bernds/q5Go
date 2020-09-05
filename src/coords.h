#ifndef COORDS_H
#define COORDS_H

using coord_pair = std::pair<unsigned, unsigned>;
using coord_transform = coord_pair (*) (unsigned, unsigned, unsigned, unsigned);

struct alignment {
	bool top = false, left = false, bot = false, right = false;
	bool operator== (const alignment &other) const
	{
		return top == other.top && left == other.left && bot == other.bot && right == other.right;
	}
	bool operator!= (const alignment &other) const { return !operator== (other); }
};

#endif
