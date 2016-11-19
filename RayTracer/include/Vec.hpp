
#ifndef _VEC_HPP_
#define	_VEC_HPP_

class Vec {

public:
	Vec();
	Vec(float a, float b, float c);
	
	Vec& operator=(const Vec & other);	//copy assignment

	Vec operator+(const Vec & v) const;
	Vec operator-(const Vec & v) const;
	Vec operator*(float val) const;

	Vec mult(const Vec & v) const;
	Vec& norm();
	float dot (const Vec & v) const;
	Vec cross(const Vec & v) const;

	void clear();


	float x, y, z; // for position, or color (r,g,b)
};

#endif