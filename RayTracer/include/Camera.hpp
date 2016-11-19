
#ifndef _CAMERA_HPP_
#define	_CAMERA_HPP_

#include "Vec.hpp"

struct Camera{
	/* User defined values */
	Vec orig, target;

	/* Calculated values */
	Vec dir, x, y;
};

#endif