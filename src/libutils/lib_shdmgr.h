#ifndef __LIB_SHDMGR_H__
#define __LIB_SHDMGR_H__


/*
 * Shader Manager
 *
 * TODO
 *
 *  - transfer over shaders & rendergroup stuff
 *  - multipass rendering
 *  - improved protocol for defining rendergroups,
 *  	and selecting a rendergroup
 *  - remove glewInit from here (should only be init'd once
 *  	and in a separate context)
 *
 ***/


#include "util.inc.h"
#include "libutils/lib_resmgr.h"

#include <sstream>
#include "extern/GL/glew.h"

/*
=================================================

	Shader Manager

	Handles all shaders and GL programs

=================================================
*/

struct ShaderParameter;
struct ShaderSet;
struct ShaderProgram;
struct RenderGroup;
class Entity;
class ShaderManager {

	vector<ShaderSet*>     shaders;
	vector<ShaderProgram*> programs;
			
public:
	vector<RenderGroup*>   renderers;
	ShaderManager();
	~ShaderManager();

	ShaderProgram* createProgram();
	RenderGroup*   createRenderer(ShaderProgram*);
	t_error linkProgram(ShaderProgram*);

	t_error loadShader(ShaderProgram*, const char*, GLenum, uint, ShaderSet**);
	t_error addShaderParameter(ShaderSet*, const char*, uint);
};




struct ShaderParameter {
	string name;
	uint description;
	bool output;
};

struct ShaderSet {
	uint description;
	vector<ShaderParameter*> parameters;
	vector<GLuint> shaderLinks;
};

struct ShaderProgram {
	GLuint programid;
	uint description;
	vector<ShaderSet*> shaders;
};

struct RenderGroup {
	ShaderProgram* program;
	vector<Entity*> entities;
};



// Shaders
const uint SHD_PHONG = 1 << 0;
const uint SHD_BUMP  = 1 << 1;
const uint SHD_HGHLT = 1 << 2;
const uint SHD_NONE  = 1 << 3;

// Shader Input
const uint SHDIN_POSITION = 1;
const uint SHDIN_COLOR    = 2;
const uint SHDIN_NORMAL   = 3;
const uint SHDIN_TEXCOORD = 4;
const uint SHDIN_TEXTURE  = 5;
const uint SHDIN_TEXBUMP  = 6;


#endif
