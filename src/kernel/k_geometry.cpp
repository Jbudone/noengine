
#include "k_geometry.h"

void Vertex::normal(Vertex& up, Vertex& right) {
	Vertex nUp = up-(*this); nUp.normalize();
	Vertex nRight = right-(*this); nRight.normalize();
	// Vertex norm = nRight.cross(nUp);
	Vertex norm = nUp.cross(nRight);
	nx = norm.x;
	ny = norm.y;
	nz = norm.z;
}

void Vertex::rotX(float t) {
	// |  1   0    0   | |x|
	// |  0  cos  sin  | |y|
	// |  0 -sin  cos  | |z|
	y = y*cos(t) + z*sin(t);
	z = -y*sin(t) + z*cos(t);
}

void Vertex::rotY(float t) {
	// | cos  0  -sin  | |x|
	// |  0   1    0   | |y|
	// | sin  0   cos  | |z|
	x = x*cos(t) - z*sin(t);
	z = x*sin(t) + z*cos(t);
}

void Vertex::rotZ(float t) {
	// | cos sin   0   | |x|
	// |-sin cos   0   | |y|
	// |  0   0    1   | |z|
	x = x*cos(t) + y*sin(t);
	y = -x*sin(t) + y*cos(t);
}



Triangle::Triangle(Vertex& v0, Vertex& v1, Vertex& v2) :
	v0(v0), v1(v1), v2(v2) { }

Plane::Plane(Triangle& triangle) {
	// norm: norm(cross(A-B,A-C))
	// 	= ||<(B-A)y(C-A)z - (B-A)z(C-A)y,
	//		 (B-A)z(C-A)x - (B-A)x(C-A)z,
	//		 (B-A)x(C-A)y - (B-A)y(C-A)x>||
	float BAx = triangle.v1.x - triangle.v0.x,
		  BAy = triangle.v1.y - triangle.v0.y,
		  BAz = triangle.v1.z - triangle.v0.z,
		  CAx = triangle.v2.x - triangle.v0.x,
		  CAy = triangle.v2.y - triangle.v0.y,
		  CAz = triangle.v2.z - triangle.v0.z;
	normal.x = BAy*CAz - BAz*CAy;
	normal.y = BAz*CAx - BAx*CAz;
	normal.z = BAx*CAy - BAy*CAx;

	// normalize the normal
	float normalLen = sqrt( normal.x*normal.x + normal.y*normal.y + normal.z*normal.z );
	normal.x /= normalLen;
	normal.y /= normalLen;
	normal.z /= normalLen;

	point = triangle.v0;
	d = -(normal.x*point.x + normal.y*point.y + normal.z*point.z);
	
}

Plane::Plane(Vertex& point, Vertex& normal)  : point(point), normal(normal) { }
Sphere::Sphere(Vertex& center, float radius) : center(center), radius(radius) { }
Circle::Circle(Vertex& center, float radius) : center(center), radius(radius) { }
Ray::Ray(Vertex& point, Vertex& direction)   : point(point), direction(direction) { }

Plane* bisect(Vertex& p0, Vertex& p1) {
	Vertex midpoint = (p0+p1)*0.5;
	Vertex direction = p1-p0;
	direction.normalize();
	Plane* bisector = new Plane(midpoint, direction);
	return bisector;
}

Circle* intersect(Plane* plane, Sphere* sphere) {

	// Circle center will be the point on plane nearest to sphere center
	// The path from the point to the plane is the normal of the plane (since it is perpendicular to the plane; this
	// will always be the shortest path). Hence the point on the plane is somewhere along the path from the point.
	// Nearest point,
	//  ax + by + cz = d ; n=<a,b,c>
	//  g = p - tn   (p is sphere point; g is point on plane)
	//  n*g = d      (this shows that g is a point on the plane)
	//  a(p_x - tn_x) + b(p_y - tn_y) + c(p_z - tn_z) = d
	//  n*p - tn*n = d
	//  t = (n*p - d)/(n*n)
	//  g = p - n((n*p-d)/(n*n))
	Vertex center;
	float a = plane->normal.x,
		  b = plane->normal.y,
		  c = plane->normal.z,
		  d = plane->d,
		  t = (plane->normal.dot(sphere->center) - d) / (plane->normal.dot(plane->normal));
	center.x = sphere->center.x - t*plane->normal.x;
	center.y = sphere->center.y - t*plane->normal.y;
	center.z = sphere->center.z - t*plane->normal.z;

	// Circle radius is: 0 for D>=r, otherwise sqrt(r^2-D^2)
	Vertex distance;
	distance.x = sphere->center.x - center.x;
	distance.y = sphere->center.y - center.y;
	distance.z = sphere->center.z - center.z;
	float distanceSq = distance.x*distance.x + distance.y*distance.y + distance.z*distance.z;
	float radiusSq   = sphere->radius*sphere->radius;
	if ( distanceSq >= radiusSq ) return 0; // sphere doesn't intersect plane

	float radius = sqrt( sphere->radius*sphere->radius - distanceSq );
	Circle* circle = new Circle( center, radius );
	return circle;
}

bool intersect(Circle* circle, Triangle* triangle, vector<Vertex>* points) {
	// TODO: doesn't intersect? return false
	
	// TODO: circle lays inside triangle? find discrete set of points
	
	// TODO: for each line in tri, find intersection(s) with circle
	// The tri is composed of 3 sides, 3 line segments:
	// 	s0{p1-p0}, s1{p2-p1}, s2{p0-p2}
	// 	s0: x = p0_x + t*(p1-p0)_x  , ...
	//
	// Circle:  (x-o_x)^2 + (y-o_y)^2 + (z-o_z)^2 = r^2
	// 			(p_x-o_x+t*d_x)^2 + (p_y-o_y+t*d_y)^2 + (p_z-o_z+t*d_z)^2 = r^2
	// 			p*p + t(2p*d) + t^2(d*d) = r^2
	// 			t = ( -2p*d +- sqrt( (2p*d)^2 - 4(p*p)(d*d) ) ) / ( 2p*p )
	vector<pair<Vertex, Vertex>> sides;
	sides.push_back({ triangle->v0, triangle->v1 });
	sides.push_back({ triangle->v1, triangle->v2 });
	sides.push_back({ triangle->v2, triangle->v0 });
	for ( auto side : sides ) {
		Vertex p1 = side.second;
		Vertex p0 = side.first;
		Vertex p  = p0 - circle->center;
		Vertex d  = Vertex( p1.x-p0.x, p1.y-p0.y, p1.z-p0.z );

		//	t = ( -2p*d +- sqrt( (2p*d)^2 - 4(p*p)(d*d) ) ) / ( 2p*p )
		//	  = ( A +- B ) / C
		float A = -2*p.dot(d);
		float B_a = 4*p.dot(d)*p.dot(d);
		float B_b = 4*p.dot(p)*d.dot(d);
		if (B_a<B_b) continue; // bad intersection!
		float B = sqrt( B_a - B_b );
		float C = 2*d.dot(d);
		

		float t_plus = (A+B)/C;
		float t_minus = (A-B)/C;

		Vertex v_plus  = Vertex( p0.x+t_plus*d.x, p0.y+t_plus*d.y, p0.z+t_plus*d.z );
		Vertex v_minus = Vertex( p0.x+t_minus*d.x, p0.y+t_minus*d.y, p0.z+t_minus*d.z );

		// can we add v_plus?
		if ( !(v_plus.x >= p0.x && v_plus.x >= p1.x) &&
			 !(v_plus.y >= p0.y && v_plus.y >= p1.y) &&
			 !(v_plus.z >= p0.z && v_plus.z >= p1.z) ) {
			points->push_back( v_plus );
		}

		// can we add v_minus?
		if ( t_plus != t_minus &&
			 !(v_minus.x >= p0.x && v_minus.x >= p1.x) &&
			 !(v_minus.y >= p0.y && v_minus.y >= p1.y) &&
			 !(v_minus.z >= p0.z && v_minus.z >= p1.z) ) {
			points->push_back( v_minus );
		}

	}

	return true;
}

bool intersect(Sphere* sphere, Triangle* triangle, vector<Vertex>* points) {

	
	vector<pair<Vertex, Vertex>> sides;
	sides.push_back({ triangle->v0, triangle->v1 });
	sides.push_back({ triangle->v1, triangle->v2 });
	sides.push_back({ triangle->v2, triangle->v0 });
	for ( auto side : sides ) {
		Vertex p1 = side.second;
		Vertex p0 = side.first;
		Vertex p  = p0 - sphere->center;
		Vertex d  = Vertex( p1.x-p0.x, p1.y-p0.y, p1.z-p0.z );

		//	t = ( -2p*d +- sqrt( (2p*d)^2 - 4(p*p)(d*d) ) ) / ( 2p*p )
		//	  = ( A +- B ) / C
		float A = -2*p.dot(d);
		float B_a = 4*p.dot(d)*p.dot(d);
		float B_b = 4*(p.dot(p)-sphere->radius*sphere->radius)*d.dot(d);
		if (B_a<B_b) continue; // bad intersection!
		float B = sqrt( B_a - B_b );
		float C = 2*d.dot(d);
		

		float t_plus = (A+B)/C;
		float t_minus = (A-B)/C;


		if ( t_plus >= 0 && t_plus <= 1 ) {

			Vertex v_plus  = Vertex( p0.x+t_plus*d.x, p0.y+t_plus*d.y, p0.z+t_plus*d.z );

			// can we add v_plus?
			if ( !(v_plus.x >= p0.x && v_plus.x >= p1.x) &&
					!(v_plus.y >= p0.y && v_plus.y >= p1.y) &&
					!(v_plus.z >= p0.z && v_plus.z >= p1.z) ) {
				Log(str(format("ADDING POINT+: (%1%,%2%,%3%) from p0(%4%,%5%,%6%) + %7%<%8%,%9%,%10%>")%
							v_plus.x%v_plus.y%v_plus.z%
							p0.x%p0.y%p0.z%
							t_plus%d.x%d.y%d.z));
				points->push_back( v_plus );
			}
		}

		if ( t_minus >= 0 && t_minus <= 1 ) {

			Vertex v_minus = Vertex( p0.x+t_minus*d.x, p0.y+t_minus*d.y, p0.z+t_minus*d.z );

			// can we add v_minus?
			if ( t_plus != t_minus &&
					!(v_minus.x >= p0.x && v_minus.x >= p1.x) &&
					!(v_minus.y >= p0.y && v_minus.y >= p1.y) &&
					!(v_minus.z >= p0.z && v_minus.z >= p1.z) ) {
				Log(str(format("ADDING POINT-: (%1%,%2%,%3%) from p0(%4%,%5%,%6%) + %7%<%8%,%9%,%10%>")%
							v_minus.x%v_minus.y%v_minus.z%
							p0.x%p0.y%p0.z%
							t_minus%d.x%d.y%d.z));
				points->push_back( v_minus );
			}

		}

	}

	return true;
}

Ray* intersect(Plane* plane1, Plane* plane2) {

	// Plane1: ax + by + cz = d
	// Plane2: ix + jy + kz = l
	//
	// Intersection: the direction of the line must be perpendicular to both normals of the planes. d = n1 x n2
	// Point 
	// 		x = 0    [arbitrary point]
	// 		z = ((j/b)*d -l)/(k - c*j/b)
	//		y = (-c*z -d) / b

	return 0; // TODO: errors with this..
	/*
	Vertex direction = plane1->normal.cross(plane2->normal);
	float a = plane1->normal.x,
		  b = plane1->normal.y,
		  c = plane1->normal.z,
		  i = plane2->normal.x,
		  j = plane2->normal.y,
		  k = plane2->normal.z,
		  d = plane1->point.dot(plane1->normal),
		  l = plane2->point.dot(plane2->normal),
		  x = 0,
		  z = ((j/b)*d - l)/(k - c*j/b),
		  y = (-c*z - d)/b;

	assert(plane1->normal.x != 0);
	assert(plane2->normal.x != 0);
	Vertex point(x,y,z);

	Ray* ray = new Ray( point, direction );
	return ray;
	*/
}

Vertex intersect(Ray* ray1, Ray* ray2) {
	// Ray: x = x0 + tDx  ;  y = y0 + tDy  ;  z = z0 + tDz
	//
	// Let ray2 be denoted with a '
	// 	x = x0 + tDx
	// 	t = (x - x0)/Dx
	// 	t = ((x0' + t'Dx') - x0)/Dx
	// 	t - t = 0 = ((x0' + t'Dx') - x0)/Dx - ((y0' + t'Dy') - y0)/Dy
	// 	0 = (x0' - x0)/Dx - (y0' - y0)/Dy + t'(Dx'/Dx - Dy'/Dy)
	// 	t' = ( (y0' - y0)/Dy - (x0' - x0)/Dx ) / ( Dx'/Dx - Dy'/Dy )
	//	x = x0' + t'Dx'  ;  y = y0' + t'Dy'  ;  z = z0' + t'Dz'
	
	Vertex v0 = ray1->point,
		   v1 = ray2->point,
		   D0 = ray1->direction,
		   D1 = ray2->direction;
	float t1  = ( (v1.y - v0.y)/D0.y - (v1.x - v0.x)/D0.x ) / ( D1.x/D0.x - D1.y/D0.y ),
		  x   = v1.x + t1*D1.x,
		  y   = v1.y + t1*D1.y,
		  z   = v1.z + t1*D1.z;

	Vertex hitpoint(x,y,z);
	return hitpoint;
}

Vertex* intersect(Ray* ray1, Ray* ray2, Ray* ray3) {

	/* 
	 * x = x0 + t0D0x = x1 + t1D1x = x2 + t2D2x
	 * 
	 * ...
	 *
	 * The idea is to find 2 components of 2 rays, in which Dsr!=0, Dtq!=0, DsrDtq != DsqDtr  (q, r components [x,y,z])
	 */

	// TODO: go through each ray/component: ray1[x]ray2[x] -> ray1[x]ray2[y] -> ...
	vector<Ray*> rays;
	rays.push_back(ray1);
	rays.push_back(ray2);
	rays.push_back(ray3);
	for (int j=0; j<3; ++j) {
		for (int k=j+1; k<3; ++k) {
			// Ray[j], Ray[k]

			for (int s=0; s<3; ++s) {
				for (int t=0; t<3; ++t) {
					if (s==t) continue; // s!=t
					// Components: s, t

					Ray* Rj = rays[j];
					Ray* Rk = rays[k];

					float Rjs, Rks, Rjt, Rkt; // start points
					float Djs, Dks, Djt, Dkt; // directions
					if (s==0) {
						Rjs = Rj->point.x;
						Rks = Rk->point.x;
						Djs = Rj->direction.x;
						Dks = Rk->direction.x;
					} else if (s==1) {
						Rjs = Rj->point.y;
						Rks = Rk->point.y;
						Djs = Rj->direction.y;
						Dks = Rk->direction.y;
					} else {
						Rjs = Rj->point.z;
						Rks = Rk->point.z;
						Djs = Rj->direction.z;
						Dks = Rk->direction.z;
					}

					if (t==0) {
						Rjt = Rj->point.x;
						Rkt = Rk->point.x;
						Djt = Rj->direction.x;
						Dkt = Rk->direction.x;
					} else if (t==1) {
						Rjt = Rj->point.y;
						Rkt = Rk->point.y;
						Djt = Rj->direction.y;
						Dkt = Rk->direction.y;
					} else {
						Rjt = Rj->point.z;
						Rkt = Rk->point.z;
						Djt = Rj->direction.z;
						Dkt = Rk->direction.z;
					}

					// FIXME: s -> j, t -> k
					// FIXME: q -> s, r -> t
					if (Dks == 0 || Djt == 0) continue; // denominators != 0
					if (Dkt*Djs == Djt*Dks) continue;   // denominator  != 0

					float t0 = ( (Rjs - Rks)/Dks + (Rkt - Rjt)*(Djs/(Djt*Dks)) ) / ( 1 - (Dkt*Djs)/(Djt*Dks) );
					Vertex hitpoint = Rk->point + Rk->direction*t0;
					Vertex* hit = new Vertex(hitpoint.x, hitpoint.y, hitpoint.z);

					return hit;
				}
			}
		}
	}

	return 0;
}

