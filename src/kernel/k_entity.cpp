#include "kernel/k_entity.h"

Entity::Entity() {
	velocity = glm::vec3(0.0f, 0.0f, 0.0f);
	page = 0;
	guid = 0; // this is essentially a null guid
}

Entity::~Entity() {
	delete mesh;
}

void Entity::updateAABB() {
	float left=0,
		  right=0,
		  top=0,
		  bottom=0,
		  near=0,
		  far=0;
	for ( auto v : mesh->vertexBuffers ) {
		if ( v.v_x < left ) left   = v.v_x;
		if ( v.v_x > right ) right = v.v_x;

		if ( v.v_y > top ) top       = v.v_y;
		if ( v.v_y < bottom ) bottom = v.v_y;

		if ( v.v_z < near ) near = v.v_z;
		if ( v.v_z > far ) far   = v.v_z;
	}

	
	boundingBox.left   = left;
	boundingBox.right  = right;
	boundingBox.top    = top;
	boundingBox.bottom = bottom;
	boundingBox.near   = near;
	boundingBox.far    = far;
	
}

bool Entity::collides(Entity* entity) {
	AABB me;
		me.left=boundingBox.left+mesh->position.x; me.right=boundingBox.right+mesh->position.x;
		me.top=boundingBox.top+mesh->position.y; me.bottom=boundingBox.bottom+mesh->position.y;
		me.far=boundingBox.far+mesh->position.z; me.near=boundingBox.near+mesh->position.z;
	AABB you;
		you.left=entity->boundingBox.left +entity->mesh->position.x;   you.right= entity->boundingBox.right+  entity->mesh->position.x;
		you.top= entity->boundingBox.top  +entity->mesh->position.y;   you.bottom=entity->boundingBox.bottom+ entity->mesh->position.y;
		you.far= entity->boundingBox.far  +entity->mesh->position.z;   you.near=  entity->boundingBox.near+   entity->mesh->position.z;
	Log(str(format("ME:\n  %1% \n ---------- \n %2% |          | %3% \n ---------- \n %4%")%me.top%me.left%me.right%me.bottom));
	Log(str(format("YOU:\n  %1% \n ---------- \n %2% |          | %3% \n ---------- \n %4%")%you.top%you.left%you.right%you.bottom));
	if ( boundingBox.right + mesh->position.x > entity->boundingBox.left + entity->mesh->position.x &&
		 boundingBox.left  + mesh->position.x < entity->boundingBox.right + entity->mesh->position.x &&

	     boundingBox.top     + mesh->position.y > entity->boundingBox.bottom + entity->mesh->position.y &&
		 boundingBox.bottom  + mesh->position.y < entity->boundingBox.top    + entity->mesh->position.y &&

	     boundingBox.far     + mesh->position.z > entity->boundingBox.near   + entity->mesh->position.z &&
		 boundingBox.near    + mesh->position.z < entity->boundingBox.far    + entity->mesh->position.z ) {

		return true;
	}

	return false;

}
