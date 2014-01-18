#ifndef __K_GEOMETRY_H__
#define __K_GEOMETRY_H__


#include "util.inc.h"


/*
=================================================

	Geometry

	Geometry data structures and intersection testing

=================================================
*/

struct Vertex {
	Vertex() { }
	Vertex(float v_x, float v_y, float v_z) : v_x(v_x), v_y(v_y), v_z(v_z) { }
	float v_x, v_y, v_z;
	bool operator ==(Vertex& vertex) {
		float rpt = 0.01f;
		return (
			(fabs(v_x-vertex.v_x) <= rpt) && 
			(fabs(v_y-vertex.v_y) <= rpt) && 
			(fabs(v_z-vertex.v_z) <= rpt) );
		// int rpt = 1000; // rounding point
		// return ((int)(v_x*rpt) == (int)(vertex.v_x*rpt) &&
		// 		(int)(v_y*rpt) == (int)(vertex.v_y*rpt) &&
		// 		(int)(v_z*rpt) == (int)(vertex.v_z*rpt));
	}
	bool between(Vertex& v1, Vertex& v2) {
		return (
			((v_x>=v1.v_x && v_x<=v2.v_x)||(v_x>=v2.v_x && v_x<=v1.v_x)) &&
			((v_y>=v1.v_y && v_y<=v2.v_y)||(v_y>=v2.v_y && v_y<=v1.v_y)) &&
			((v_z>=v1.v_z && v_z<=v2.v_z)||(v_z>=v2.v_z && v_z<=v1.v_z))
		);
	}
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

};

struct Sphere {
	Sphere();
};

struct Circle {
	Circle();
};

Circle* intersect(Plane*, Sphere*);

#endif
