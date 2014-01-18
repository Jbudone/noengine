
#include "k_geometry.h"

Triangle::Triangle(Vertex& v0, Vertex& v1, Vertex& v2) :
	v0(v0), v1(v1), v2(v2) { }

Plane::Plane(Triangle& triangle) {

}

Sphere::Sphere() {

}

Circle::Circle() {

}

Circle* intersect(Plane* plane, Sphere* sphere) {
	return 0;
}

