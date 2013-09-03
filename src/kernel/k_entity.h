#ifndef __K_ENTITY_H__
#define __K_ENTITY_H__


#include "util.inc.h"
#include "libutils/lib_resmgr.h"
#include "libutils/lib_shdmgr.h"

#include "kernel/k_camera.h"
#include "kernel/k_mesh.h"
#include "kernel/k_world.h"

/*
 * Entity
 *
 * TODO
 *
 *  > server/client entity
 *  > body, illumination; cache object (read/write)
 *
 ***/


/*
=================================================

	Entity

	Its an entity..

=================================================
*/

struct AABB {
	AABB(){}
	float left, right, top, bottom, near, far;
};

class Mesh;
struct Page;
class Entity {
public:
	Entity();
	~Entity();
	void updateAABB();
	bool collides(Entity*);

	Mesh* mesh;
	AABB boundingBox;
	bool selected;
	uint guid;

	glm::vec3 velocity;
	Page* page;
};


#endif
