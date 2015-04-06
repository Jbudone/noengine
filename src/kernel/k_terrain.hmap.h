#ifndef __K_TERRAIN_HMAP_H__
#define __K_TERRAIN_HMAP_H__


#include "util.inc.h"
#include <csignal>

#include "extern/GL/glew.h"

#include "libutils/lib_resmgr.h"
#include "libutils/lib_shdmgr.h"
#include "kernel/k_camera.h"


/*
=================================================

	Terrain

	...

=================================================
*/

/* TODO
 *  > Terrain generation: diamond square (midpoint displacement) -- write to heightmap
 *  > Normals 
 *  > Texture splatting, tessellation?
 *  > LOD points interpolation
 *  > adaptive restricted quadtree
 *  > Triangle strips rendering
 *  > Memory friendly -- byte squeezing in quads/tiles
 *  > UV-Mapping (tri-planar texture mapping of large-scale texture)
 */

typedef float quaderr;

struct iTriangle;

struct Terrain;
struct Quad {
	Quad(Terrain* terrain, Quad* parent=0) :
		parent(parent), terrain(terrain), enabled(false),
		west(0), east(0), north(0), south(0),
		childNW(0), childNE(0), childSE(0), childSW(0),
		dependsWE(0), dependsNS(0) { }
	Quad* parent;
	Quad* childNW;
	Quad* childNE;
	Quad* childSE;
	Quad* childSW;

	Quad* dependsWE; // depends on west/east parent's neighbours
	Quad* dependsNS; // depends on north/south parent's neighbours

	// neighbours
	Quad* west;
	Quad* east;
	Quad* north;
	Quad* south;

	float error;
	GLuint vao;
	vector<iTriangle> triangles;
	Terrain* terrain; // TODO: better way to refer to terrain? place Quad in terrain struct?
	struct IBO { uint offset; uint length; GLuint ibo; };
	IBO iboList[9];

	const static int min_width = 32; // NOTE: should be (x-1) divisible by 2
	
	void buildQuad(int x, int z, int width, int depth);
	void render(); // TODO: render from threshold error
	void enableRender();
	void clearRender();
	pair<int,int> center;
	float radius;
	bool enabled;
};

class Terrain {
public:
	Terrain(uint gl);
	~Terrain();

	Quad* headQuad;
	float width, depth;
	float hmapHeight;
	float errorThreshold;
	float minBase;
	float maxEdgeLength;
	struct SampledVertex { float x, y, z; };
	// vector<SampledVertex> samples;
	SampledVertex* samples;
	float* sampleHeights;
	float scaleDepth; // scaled depth of heightmap
	bool drawLines;
	vector<Vertex> vertices;
	vector<iTriangle> triangles;
	void generateTerrain(int size);
	void generateTerrain2(int size);
	void generateTerrain3(int size);
	float randf(){ return rand()/((float)RAND_MAX); }

	void construct();
	void render();
	GLuint gl;
	GLuint glTexture;
	GLuint glTextureSnow;
	GLuint glTextureGrass;
	GLuint glTextureBumpy;
	GLuint glTextureDetail;
	GLuint vbo, vao, elementBuffer;

	float getElevation(float x, float z);
	struct CircularInterpolation { float interpolation; float min; float max; bool foundSome; };
	CircularInterpolation circularSelection(float x, float z, float radius);
};

// Indexed Triangle (refers to indices of verts in vertexBuffer)
struct iTriangle {
	iTriangle(uint p0, uint p1, uint p2) : p0(p0), p1(p1), p2(p2) { }
	uint p0, p1, p2;
	// uchar metadata;
	bool operator ==(iTriangle& triangle) { 
		// TODO: fix this (backface culling)
		return ((p0==triangle.p0||p0==triangle.p1||p0==triangle.p2)&&(p1==triangle.p0||p1==triangle.p1||p1==triangle.p2)&&(p2==triangle.p0||p2==triangle.p1||p2==triangle.p2));
	}
};


#endif
