#ifndef __K_GEOMETRY_H__
#define __K_GEOMETRY_H__


#include "util.inc.h"


/*
=================================================

	Geometry

	Geometry data structures and intersection testing

=================================================
*/

/*
template<uint row_count, uint col_count>
struct Matrix {
	Matrix() : rows(row_count), columns(col_count) { }
	uint rows, columns;
	float[row_count][col_count];
};
*/

struct Vertex {
	Vertex() { }
	Vertex(float x, float y, float z) : x(x), y(y), z(z), nx(0.0f), ny(0.0f), nz(0.0f), dx(0.0f), dz(0.0f) { }
	float x, y, z;
	float nx, ny, nz;
	float dx, dz;
	void normal(Vertex& up, Vertex& right);
	bool operator ==(Vertex& vertex) {
		float rpt = 0.01f;
		return (
			(fabs(x-vertex.x) <= rpt) && 
			(fabs(y-vertex.y) <= rpt) && 
			(fabs(z-vertex.z) <= rpt) );
		// int rpt = 1000; // rounding point
		// return ((int)(x*rpt) == (int)(vertex.x*rpt) &&
		// 		(int)(y*rpt) == (int)(vertex.y*rpt) &&
		// 		(int)(z*rpt) == (int)(vertex.z*rpt));
	}
	bool between(Vertex& v1, Vertex& v2) {
		return (
			((x>=v1.x && x<=v2.x)||(x>=v2.x && x<=v1.x)) &&
			((y>=v1.y && y<=v2.y)||(y>=v2.y && y<=v1.y)) &&
			((z>=v1.z && z<=v2.z)||(z>=v2.z && z<=v1.z))
		);
	}
	float dot(Vertex& v) { return x*v.x+y*v.y+z*v.z; }
	Vertex cross(Vertex& v) { return Vertex((y*v.z-z*v.y),(z*v.x-x*v.z),(x*v.y-y*v.x)); }
	void normalize() { float length = sqrt(this->dot(*this)); x/=length; y/=length; z/=length; }
	Vertex operator-(const Vertex& rhs) const { return Vertex(x-rhs.x, y-rhs.y, z-rhs.z); }
	Vertex operator+(const Vertex& rhs) const { return Vertex(x+rhs.x, y+rhs.y, z+rhs.z); }
	Vertex operator*(const float t) const { return Vertex(t*x,t*y,t*z); }
	void rotX(float t);
	void rotY(float t);
	void rotZ(float t);
};

struct Triangle {
	Triangle(Vertex&, Vertex&, Vertex&);
	Vertex v0;
	Vertex v1;
	Vertex v2;
	bool operator ==(Triangle& triangle) { 
		return (
			(v0==triangle.v0 || v0==triangle.v1 || v0 == triangle.v2) &&
			(v1==triangle.v0 || v1==triangle.v1 || v1 == triangle.v2) &&
			(v2==triangle.v0 || v2==triangle.v1 || v2 == triangle.v2) );
	}
};


struct Plane {
	Plane(Triangle&);
	Plane(Vertex&, Vertex&);
	Vertex normal;
	Vertex point;
	float  d;
};

struct Sphere {
	Sphere(Vertex&, float);
	Vertex center;
	float radius;
};

struct Circle {
	Circle(Vertex&, float);
	Vertex center;
	float radius;
};

struct Ray {
	Ray(Vertex&, Vertex&);
	Vertex point;
	Vertex direction;
};

Plane* bisect(Vertex&, Vertex&);

Circle* intersect(Plane*, Sphere*);
bool intersect(Circle*, Triangle*, vector<Vertex>*);
bool intersect(Sphere*, Triangle*, vector<Vertex>*);
Ray* intersect(Plane*, Plane*);
Vertex* intersect(Ray*, Ray*, Ray*);
Vertex intersect(Ray*, Ray*);

#endif
