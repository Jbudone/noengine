#ifndef __K_TERRAIN_H__
#define __K_TERRAIN_H__


/**
 *
 * Terrain Engine
 *
 * TODO
 *
 *  - load texture, step through and build terrain out of
 *    procedural generation
 *  - march into quads for gpu
 *  - compress into derivatational format (x & y)
 *  - LOD
 *  - morph LOD's
 *  - merge grids of compression
 *  - setup octree of chunks
 *  - optimize chunks (position, AABB, split/merge chunks)
 *  - raycast & circle project onto voxels
 *  - sculpting
 *  - delete/extend terrain
 *  - compress sculpt
 *  - serialize/read from sculpt
 *  - store data
 *  - recreate terrain from raw data
 *  - portal & paging (use 2 separate systems; take the
 *    intersection of both during creation, then turn into
 *    one single paging system which user can traverse
 *    through)
 *  - multishading
 *  - subgraph creation (NOTE: will need to use ptr to only
 *    limit of subgraph details to the outlining voxels, but
 *    be able to adapt to sculpting changes); use subgraph
 *    for different shaders, texture specifications, blending
 *    techniques, etc.; multiple terrains (during
 *    construction check which terrain the voxel belongs to)
 *  - shading & blending techniques (for various terrain
 *    situations)
 *  - create physics
 **/


#include "util.inc.h"

#include "extern/GL/glew.h"

#include "libutils/lib_resmgr.h"
#include "libutils/lib_shdmgr.h"
#include "kernel/k_camera.h"


/*
=================================================

	Terrain

	All terrain handling, managing and storing. This will be
	able to procedurally generate terrain, as well as
	various terrain features (river networks, caverns,
	foliage, etc.)

=================================================
*/

struct Voxel;
struct Chunk;
template <class T> struct Point;
template <class T> struct QuadTree;
class TerrainVertexBuffer;
struct ControlPoint;
struct TerrainSelection;
class Terrain {
	GLuint vao, vbo;
	const uint width  = 5000,
		  	   depth  = 5000,
			   height = 5000; // maximum height
public:

	Terrain(GLuint gl);
	~Terrain();
	void clearTerrain();
	TerrainSelection* terrainPick(float xw, float yw);

	Chunk* headChunk = 0;

	QuadTree<TerrainVertexBuffer>* verticesQuadTree;
	QuadTree<ControlPoint>* compressedQuadTree;
	vector<TerrainVertexBuffer> vertexBuffer;
	GLuint gl;
	void construct();
	void render();

	/* Procedural Generation
	 *
	 * Adopts the Perlin noise for a heightmap like terrain;
	 * this type of terrain will need to be converted and
	 * optimized into a voxel octree format afterwards
	 ***/
	void decompressTerrain(); // clears current terrain and recreates it from the compressedQuadTree
	void generateTerrain(); // clears current terrain and creates a heightmap type terrain
	void fetchCriticalPoints(); // search through the voxel quadtree and setup the critical points (derivative control points) quadtree
	void generateTerrain2(); // a test..
	QuadTree<TerrainVertexBuffer>* generateVoxel(int x, int z, QuadTree<TerrainVertexBuffer>* backVoxel = 0, QuadTree<TerrainVertexBuffer>* rightVoxel = 0);
	Chunk* createChunks(Point<int> point, Chunk* below, Chunk* behind);
	Chunk* addVoxelIntoChunk(Chunk* chunk, Voxel* voxel);
};

template<class T>
struct Point {
	Point() { }
	Point(T x, T y, T z) : x(x), y(y), z(z) { }
	T x; T y; T z;
};

struct Slope {
	char v, a;
};

struct ControlPoint {
	Slope dzdx;
	Slope dzdy;

	float v_x, v_y, v_z;
	float n_x, n_y, n_z;
};


struct TerrainVertexBuffer {
	TerrainVertexBuffer(float v_x, float v_y, float v_z) : v_x(v_x), v_y(v_y), v_z(v_z) { }
	float v_x, v_y, v_z;
};

struct Voxel {
	QuadTree<TerrainVertexBuffer>* vertex;
	ControlPoint* controlPoint;
	Bitfield<16> metadata;
};

template<class T>
struct QuadTree {
	QuadTree(T self, QuadTree<T>* forward, QuadTree<T>* left, QuadTree<T>* backward, QuadTree<T>* right) : self(self), forward(forward), left(left), backward(backward), right(right) { }
	QuadTree(T self) : self(self) { }
	QuadTree<T>* forward=0;
	QuadTree<T>* left=0;
	QuadTree<T>* backward=0;
	QuadTree<T>* right=0;
	T self;
};

template<class T>
struct LinkedList {
	T* node;
	LinkedList<T>* next;
};

template<class T>
struct LinkedList_Ordered {
	uchar i;
	T* node;
	LinkedList_Ordered<T>* next;
};

/* LinkedList_Line
 *
 * Start a line from a lower left point going upwards to an
 * upper right point; all items which are added to this
 * linked list must provide a point in space which will be
 * projected onto this line to determine where it fits in
 * the linked-list
 **/
template<class T>
struct LinkedList_Line {

	LinkedList_Line(float startX, float startY, float startZ, float endX, float endY, float endZ) {
		head = 0;
		start.x = startX;
		start.y = startY;
		start.z = startZ;

		direction.x = endX - startX;
		direction.y = endY - startY;
		direction.z = endZ - startZ;
		float length = direction.x * direction.x + direction.y * direction.y + direction.z * direction.z;
		length = sqrt(length);
		direction.x /= length;
		direction.y /= length;
		direction.z /= length;

	}
	LinkedList_Ordered<T>* head;
	LinkedList_Ordered<T>* addNode(T* item, float x, float y, float z) {
		// normalize point x,y,z
		float length = x*x + y*y + z*z;
		x /= length;
		y /= length;
		z /= length;

		// project onto line
		float t = x*direction.x + y*direction.y + z*direction.z;
		uchar i = quantizeUFloat(t);

		if ( !head ) {
			head = new LinkedList_Ordered<T>();
			head->i = i;
			head->node = item;
			return head;
		}

		LinkedList_Ordered<T>* prev = head;
		LinkedList_Ordered<T>* cur;
		while ( (cur = prev->next) ) {
			if ( cur->i > i ) break;
			prev = cur;
		}

		LinkedList_Ordered<T>* node = new LinkedList_Ordered<T>();
		node->next = cur;
		prev->next = node;
		node->i = i;
		return cur;

	}
	Point<float> start;
	Point<float> direction;
};

/* LinkedList_Circle
 *
 * Project the given line into polar coordinates, use this
 * to order the linked-list for each point added (starting
 * from 90deg and working around clock-wise)
 **/
template<class T>
struct LinkedList_Circle {
	LinkedList_Circle(float x, float y, float z);
	LinkedList<T>* head;
	LinkedList<T>* addNode(T* item, float x, float y, float z);
};

/* TerrainSelection
 *
 * When selection the terrain, there will be multiple voxels
 * which are selected, and each belong to a specific group
 * to best match its selection and how it will be affected
 *
 * inner_selection - these voxels belong within the
 * 				inner-area of the selection, and will not be
 * 				affected other than a simple translation to
 * 				match their neighbours
 * 				LinkedList<Voxels>: imagine a straight line
 * 				from the bottom left to the top right of the
 * 				selection area, the selected voxels are
 * 				projected onto that line and stored as a
 * 				linked-list from that line
 * inner_neighbours - the neighbours of the inner_selection
 * 				voxels; these must lay between a given
 * 				bounds, otherwise those voxels will not be
 * 				included. Its even possible to have no
 * 				voxels matching this selection; however it
 * 				may be important to include false voxels
 * 				which WOULD belong in this area for each
 * 				voxel in the inner_selection; hence those
 * 				false voxels may be provided here too
 * 				LinkedList<Voxels>: starting from the
 * 				very top points, circling clockwise along
 * 				this region to add each voxel into this
 * 				linked-list
 * mid_neighbours - the neighbours between the
 * 				inner_neighbours and outer_neighbours; these
 * 				guys should exist for subdivision needs
 * 				LinkedList<Voxels>: starting from the
 * 				very top of the region, circle clockwise
 * 				adding each voxel into the linked-list
 * outer_neighbours - the outside neighbours; these are used
 * 				for turning into control points for the
 * 				sculpted area
 * 				LinkedList<Voxels>: starting from the
 * 				top, circle clockwise along this region
 * 				adding each voxel into the linked-list
 *
 **/
struct TerrainSelection {
	LinkedList_Line<QuadTree<Voxel>> inner_selection;
	LinkedList_Circle<QuadTree<Voxel>> inner_neighbours;
	LinkedList_Circle<QuadTree<Voxel>> mid_neighbours;
	LinkedList_Circle<QuadTree<Voxel>> outer_neighbours;
};





/* Chunk
 *
 * Used for storing a set of voxels surrounded by an AABB;
 * chunk contains voxels, triangles formed by voxels, and
 * partial triangles formed between voxels within this chunk
 * and an adjacent chunk. Chunks are apart of a HexTree
 * where each face may be connected to another Chunk. Each
 * chunk also contains a list of portals (ptr to portal
 * table) of portals that its connected to.
 *
 * 		Multiple Portals
 * 	A chunk contains some arbitrary set of voxels within a
 * 	given AABB area. Some of those voxels may be the ground
 * 	in one portal while the other portals could be the
 * 	ceiling of the portal immediately below. For this we
 * 	need to assume that chunks could be inside multiple
 * 	portals
 * 	TODO: perhaps for efficiency, a packed bitfield
 * 	containing indices to each portal the chunk belongs to;
 * 	x bytes w/ max 64 portals == (x*8/6) = y portals per
 * 	chunk
 *
 *
 * 		Voxel Storage
 *	A line is stretched from the lower (x,y,z) corner of the
 *	chunk to the far (x,y,z) corner; voxels are projected
 *	onto this line and stored in a linked list along this
 *	line
 *
 *		Triangle Storage
 *	TODO
 *
 *
 * 		Voxel Connections across Chunks
 * Grouped voxels that form a triangle may be shared
 * across chunks which are not immediately adjacent to each
 * other; eg. 1 point in this chunk, 1 point in neighbour
 * chunk, and 1 point in its neighbour (to our corner or
 * folding point)
 *
 * 			 |*   |
 * 		|----|----|
 * 		|   *|*   |
 * 		|    |    |
 * 		|----|----|
 *
 **/
struct Chunk {
	Chunk(Point<int> position);
	Point<int> worldOffset;
	static const Point<int> chunkSize;
	LinkedList_Line<Voxel> voxels;

	// HexTree
	Chunk* above    = 0;
	Chunk* below    = 0;
	Chunk* left     = 0;
	Chunk* right    = 0;
	Chunk* infront  = 0;
	Chunk* behind   = 0;
};

#endif
