

#ifndef _SPHERE_HPP_
#define _SPHERE_HPP_

#include "Vec.hpp"
#include "Ray.hpp"

enum Refl {
	DIFFuse, SPECular, REFRactive
}; /* material types, used in radiance() */

struct Sphere {

	float rad;		/* radius */
	Vec p, e, c;	/* position, emission, color */
	enum Refl refl; /* reflection type (DIFFuse, SPECular, REFRactive) */


	Sphere();
	Sphere(float radius, const Vec& pos, const Vec& emi, const Vec& color, Refl reflection);

	float intersect(const Ray & r) const;

};


#endif