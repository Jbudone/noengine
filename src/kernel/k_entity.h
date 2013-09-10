#ifndef __K_ENTITY_H__
#define __K_ENTITY_H__


/*
 * Entity
 *
 * TODO
 *
 *  - body, illumination; cache object (read/write)
 *  - improved collision detection (varying accuracy levels)
 *  - extend model to include Lights, Polysoup, etc.
 *
 ***/


#include "util.inc.h"
#include "libutils/lib_resmgr.h"
#include "libutils/lib_shdmgr.h"

#include "kernel/k_camera.h"
#include "kernel/k_mesh.h"
#include "kernel/k_world.h"



struct AABB {
	AABB(){}
	float left, right, top, bottom, near, far;
};


/*
=================================================

	Entity

	Its an entity..

=================================================
*/

class Mesh;
struct Page;
class Entity {
public:
	Entity();
	~Entity();
	void updateAABB();
	bool collides(Entity*);


	// mesh specific details
	Mesh* mesh;
	AABB boundingBox;
	bool selected;
	uint guid;

	glm::vec3 velocity;
	Page* page;
};


#endif
