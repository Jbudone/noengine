#ifndef __K_MESH_H__
#define __K_MESH_H__


#include "util.inc.h"
#include <math.h>

#include "extern/GL/glew.h"
#include "extern/soil/SOIL.h"
#define GLM_SWIZZLE
#include "extern/GL/glm/glm.hpp"                  // std mat/vec obj
#include "extern/GL/glm/gtc/matrix_transform.hpp" // perspective
#include "extern/GL/glm/gtc/type_ptr.hpp"         // value_ptr

#include "libutils/lib_resmgr.h"
#include "kernel/k_camera.h"

/*
 * Mesh
 *
 * TODO
 *
 *  - create instance of mesh
 *  - handling multiple materials, textures
 *  - delete vao in construction (in case previously setup)
 *
 ***/


/*
=================================================

	Mesh

	your basic mesh

=================================================
*/

struct VertexBuffer {
	float v_x, v_y, v_z;
	float t_s, t_t;
	float n_x, n_y, n_z;
	VertexBuffer(const float v_x, const float v_y, const float v_z,
				 const float t_s, const float t_t,
				 const float n_x, const float n_y, const float n_z) :
		v_x(v_x), v_y(v_y), v_z(v_z),
		t_s(t_s), t_t(t_t),
		n_x(n_x), n_y(n_y), n_z(n_z) { }
};

struct TextureFile {
	TextureFile(string filename, uchar texType) :
		filename(filename), texType(texType) { }
	string filename;
	uchar texType;

};

typedef struct Triple<float,float,float> fTriple;

class Mesh;
class MeshRenderData {

	GLuint vao, vbo;

public:
	GLuint textures[2];
	MeshRenderData();
	~MeshRenderData();

	// texture types
	// NOTE: must start from 0 and go up incrementally (works with gl memory)
	static const uchar MESH_RENDER_TEXTURE = 0;
	static const uchar MESH_RENDER_TEXBUMP = 1;

	GLuint gl;
	t_error loadTexture(const char* filename, uchar texType = MESH_RENDER_TEXTURE);
	t_error construct(Mesh* mesh,bool reconstruction=false);
	t_error clear();
	t_error render(glm::mat4 model);

	bool hasTexture = false;
	bool hasBumpmap = false;
	Mesh* mesh;
};
class Mesh {
public:
	Mesh(bool);
	~Mesh();

	// vertex details
	void addVertex(const float, const float, const float);
	void addVertexNormal(const float, const float, const float);
	void addVertexTex(const float, const float);
	void pushVertex(ushort, ushort, ushort);

	void loadTexture(const char*, const uchar = MeshRenderData::MESH_RENDER_TEXTURE);

	float rayIntersectTriangle(glm::vec3, glm::vec3, glm::vec3, glm::vec3, glm::vec3);
	float lineIntersects(glm::vec3, glm::vec3);

	vector< float > vertices;
	vector< float > normals;
	vector< float > colors;
	vector< float > texcoords;
	vector< VertexBuffer > vertexBuffers;
	vector< TextureFile > textures;

	fTriple color_ambient = fTriple(1.0, 0.3, 0.0);
	fTriple color_diffuse;
	fTriple color_specular;

	glm::vec3 position = glm::vec3(2.0f, 0.0f, 0.0f);
	bool renderable; // will this mesh be rendered? (uses MeshRenderData)
	MeshRenderData* renderData;
	void setupRenderData();
	void render();
};

#endif
