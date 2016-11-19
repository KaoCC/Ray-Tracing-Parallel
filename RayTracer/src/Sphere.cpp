
#include "Sphere.hpp"

Sphere::Sphere():
	rad(0), p(), e(), c(), refl()
{
}

Sphere::Sphere(float radius,const Vec& pos,const Vec& emi, const Vec& color, Refl reflection):
	rad(radius), p(pos), e(emi), c(color), refl(reflection)
{

}

float Sphere::intersect(const Ray & r) const
{

	return 0; //tmp
}