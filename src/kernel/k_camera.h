#ifndef __K_CAMERA_H__
#define __K_CAMERA_H__



#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <boost/format.hpp>
#include "config.h"

#include "typeinfo/t_errors.h"
#include "libutils/lib_logger.h"
#include "libutils/lib_resmgr.h"
#define GLM_SWIZZLE
#include "extern/GL/glm/glm.hpp"                  // std mat/vec obj
#include "extern/GL/glm/gtc/matrix_transform.hpp" // perspective
#include "extern/GL/glm/gtc/type_ptr.hpp"         // value_ptr
#include "extern/GL/glm/gtx/quaternion.hpp"

using namespace std;
using namespace Logger;

using boost::format;
using boost::str;
/*
 * Camera
 *
 * TODO
 *
 *  > is an entity
 *  > attaches to entity
 *
 ***/


class Camera {
public:
	Camera() {}

	glm::vec3 position = glm::vec3(  0.0f, 100.0f, 0.0f );
	glm::vec3 rotation = glm::vec3( 3.85f, 0.0f, 3.15f );

	glm::mat4 perspective;
	glm::mat4 view;
	glm::mat4 perspectiveView;

	void update();
	void rotX(int offset);
	void rotY(int offset);
	void move(int x, int y, int z);

	float fov    = CAM_FOV;
	float aspect = WIN_WIDTH / WIN_HEIGHT;
	float near   = CAM_NEAR;
	float far    = CAM_FAR;
	float width  = WIN_WIDTH;
	float height = WIN_HEIGHT;

	float rotation_speed = 0.0010f;
	float move_speed     = 1;
	bool move_fast       = false;
};

extern Camera camera;

#endif
