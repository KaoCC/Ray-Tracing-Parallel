
#ifndef _RAY_HPP_
#define _RAY_HPP_

#include "Vec.hpp"

struct Ray {
	Vec o, d;		/* Origin, Direction*/

	Ray(const Vec& origin, const Vec& direction);
};


#endif