#ifndef __LIB_RESMGR_H__
#define __LIB_RESMGR_H__


#include <stdio.h>
#include <string>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <boost/format.hpp>
#include <boost/tokenizer.hpp>


#include "extern/GL/glew.h"

#include "util.inc.h"

#include "kernel/k_mesh.h"
#include "kernel/k_entity.h"
#include "kernel/k_camera.h"
#include "kernel/k_world.h"

using namespace std;
using namespace Logger;

using boost::format;
using boost::str;
using boost::tokenizer;
/*
 * Resource Manager
 *
 * TODO
 *
 *  > multipassing (load shaders, unload shaders, RTT in multipassing)
 *  > free resources
 *  > mesh instancing
 *  > shared textures
 *
 ***/


/*
=================================================

	Resource Manager

	contains the GL program, pointers, shader handling and various
	resource loading, storage and handling

=================================================
*/

typedef boost::char_separator<char> CharSeparator;
typedef tokenizer< CharSeparator >  CharTokenizer;
struct Texture;
class Mesh;
class Entity;
class World;
struct ShaderSet;
struct ShaderParameter;
struct ShaderProgram;
struct RenderGroup;
namespace ResourceManager {

		// resource loading
		Entity* LoadMesh(RenderGroup* renderer, const char* filename);
		t_error LoadMeshMaterial(const char* filename, Mesh* mesh);

		extern vector<Texture> textures;

		namespace ResourceManagerSystem { };

		extern World* world;
		void LoadWorld(bool renderable);
		void shutdown();
};


struct Texture {
	Texture(const char* filename, const uchar* imagedata, int width, int height);
	Texture(const Texture &rhs);
	Texture& operator=(const Texture &rhs);
	~Texture();
	int getSize();
	char *filename;
	uchar *imagedata;
	int width, height;
};

inline int tokenToInt(tokenizer<boost::char_separator<char>>::iterator token) { return atoi((*token).c_str()); }
inline unsigned short tokenToUShort(tokenizer<boost::char_separator<char>>::iterator token) { return (ushort)atof((*token).c_str());}
inline float tokenToFloat(tokenizer<boost::char_separator<char>>::iterator token) { return atof((*token).c_str()); }

#endif
