
#include "kernel/k_camera.h"

Camera camera;

void Camera::update() {

	// setup projection 
	glm::mat4 perspective = glm::perspective( fov, aspect, near, far );

	// setup camera view
	glm::quat quatZ = glm::angleAxis( glm::degrees( rotation.z ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
	glm::quat quatY = glm::angleAxis( glm::degrees( rotation.y ), glm::vec3( -1.0f, 0.0f, 0.0f ) );
	glm::quat quatX = glm::angleAxis( glm::degrees( rotation.x ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
	glm::quat quat = quatY * quatX * quatZ;
	view = glm::translate( glm::toMat4(quat), position );

	// glm::vec3 flipPerspective = glm::vec3( -1.0f, -1.0f, 1.0f );
	// glm::vec3 flipPerspective = glm::vec3( 1.0f, 1.0f, 1.0f );
	perspectiveView = perspective * view;
	// perspectiveView = glm::scale( perspectiveView, flipPerspective );
}

void Camera::rotX(int offset) {
	rotation.y += rotation_speed * offset;
	update();
}

void Camera::rotY(int offset) {
	rotation.x += rotation_speed * offset;
	update();
}

void Camera::move(int x, int y, int z) {

	// setup rotation for local camera translation
	glm::quat quatZ = glm::angleAxis(glm::degrees(rotation.z), glm::vec3(0.0f,0.0f,1.0f));
	glm::quat quatY = glm::angleAxis(glm::degrees(rotation.y), glm::vec3(-1.0f,0.0f,0.0f));
	glm::quat quatX = glm::angleAxis(glm::degrees(rotation.x), glm::vec3(0.0f,1.0f,0.0f));
	glm::quat quat = quatY * quatX * quatZ;
	glm::mat4 rotate = glm::toMat4(quat);
	rotate = glm::inverse(rotate);


	// translate camera
	glm::vec4 translation =  glm::vec4(x,y,z,1.0f);
	translation = rotate * translation;
	translation = move_speed * translation;
	translation.w /= 10;

	position.x += translation.x / translation.w;
	position.y += translation.y / translation.w;
	position.z += translation.z / translation.w;
	update();

	
	Page* curPage = ResourceManager::world->fetchPage(position.z, position.x);
	if ( ResourceManager::world->page != curPage ) {
		Log(str(format("Moved to new page (%1%,%2%)")%position.z%position.x));
	}
	ResourceManager::world->page = curPage;
	
}
