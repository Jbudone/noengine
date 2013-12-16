#ifndef __K_TERRAIN_H__
#define __K_TERRAIN_H__


/**
 *
 * Terrain Engine
 *
 * Top Priority
 *  -> CLEAN
 *  -> Tri-tree
 *  -> T-junction
 * 	-> Pooled VBO's w/ Shaders + Dynamic Drawing
 * 	-> Fast Querying(picking) + Caching
 * 	-> Delaunay & LOD rendering ;; Show/Control LOD + Chunks
 * 	-> Multirendering ;; Control wireframe
 * 	-> Physics
 * 	-> Sculpting (CSG)
 * 	-> Server/Client
 * 	-> Redo Tree / Undo
 * 	-> Portal
 * 	-> Smooth + Parent Tris
 * 	-> Tessellation
 * 	-> Geometry shader
 * 	-> Automatic & Set shader areas
 * 	-> Draw heightmap/destructable area
 * 	-> Noise & Destruction area on CSG
 * 	-> Automatic formation (separate program?) + Feed into
 * 		this terrain
 * 	-> Weather + Sky
 * 	-> Grass ;; Vegetation shaders ;; Formation of
 * 		vegetation
 * 	-> Plant billboards ;; L-Systems
 * 	-> Trees ;; L-Systems
 * 	-> Oceans
 *
 * TODO
 *  - preparation for terrain system
 *  	> Terrain > Smooth > Tessellation + Geometry/Normals
 *  	(markings & pickaxe & footprints)
 *  		
 *  		Normal/Geometry maps: use conditional query to
 *  			specified chunks to defer shading on
 *  			certain tris (range); sort VBO's by shaders,
 *  			use pools for newly added tris (LIST)
 *  			CHUNKS[]*(SHADERS[]+LOD[])  better
 *  			queries/manipulation
 *  			Use an atlas (in chunk) of
 *  			shaders/positions; can also help with
 *  			queries and selecting triangles
 *  			Keep outside list of shaders with array of
 *  			Chunks that are apart of it, then load
 *  			shader -> render each chunk
 *  			Store other data for these shaders
 *  			(repeatedly picking at a chunk area starts
 *  			to break it down and eventually destructs
 *  			the terrain)
 *
 *  		Shadows: colour value for each triangle which is
 *  		metadata for shadows; dynamic editing for shadow
 *  		(double the memory?) which can check and cache
 *  		nearest light sources and objects and update
 *  		colour value; needs to update everytime
 *  		something which could affect it updates position
 *
 *  		Create list of tessellation control points which
 *  		are connected via a tree; tri's/patches select
 *  		nearest neighbour control points and interpolate
 *  		from current position/normal
 *
 *
 *  	> Pooled vertices & triangles
 *
 *  		Patches: sort tris by shaders[] -> patches[] in
 *  		vbo; then have a tree ptr atlas to positions..
 *
 *  		ShaderList: a list of shaders within Chunk, each
 *  		shader is a vbo of tris; vbo's are patch sorted;
 *  		defer shader shading to be done from the terrain
 *  		renderer?(list of separate shaders to render
 *  		(sorted by position of group;[cache position
 *  		nearest you] stored as groups), list of chunks
 *  		to render each); pool of tris which are sorted
 *  		by patches; linked list of shaders across chunks
 *  		(could use for non-LOD and draw-once-per-frame)
 *  		SHADERS[] + CHUNKS[]*LOD[]
 *
 *  	> Show chunk(s) in different LOD formats (eg. when
 *  		sculpting)
 *  	> tri-tree; parent tri's?
 *  	> Delaunay simplification
 *  	> decals (multirender? separate triangles from
 *  		shader lists? separate geometry?)
 *
 *  		RangedTris: order tri's in chunk by shader, then
 *  			on render take shaderid as arg and render
 *  			range of tri's for that shader
 *  			USE THIS FOR BASE LAYER / SPLATTING?
 *  			 ? how to specify this between server/client
 *
 * 			Separate Geometry: rock paths, wall decals, etc.
 * 				create texture atlas for norm, geom, texture
 *
 * 			Teseellation Control Points: eg. erosion, used
 * 			elsewhere
 *
 * 			Multirender: copy list and create new VBO for
 * 				tri's; keep in separate shader (LOD?);
 * 				geometry shader should push all vertices
 * 				outwards (along normal) by epsilon distance
 * 				(OR can we simply leave these shaders for
 * 				last to avoid Z-fighting between base
 * 				layer?)
 * 				USE THIS FOR HIGHLIGHTS & TEMP SHADERS
 *  	> Update tree, undo/redo
 *
 *  		Keep a copy of action done to vertices (easy to
 *  		reverse), selected vertices, removed tris (old
 *  		list) AND replaced tris
 *  		THIS is a change leaf; use a tree of leaves
 *  	> Cached serialization w/ cached sculpting + merging
 *
 *  		Send LOD3 (1 less quality from non-modified
 *  		LOD2); stream LOD2 as we near and quality isn't
 *  		TOO bad (scheduler); LOD1 and LOD0 done on
 *  		client side -- stream specialized data like
 *  		markings and footprints as we get WAY closer;
 *  		send tessellation control tree (IF requested)
 *
 *  		LOD3 object serialization cached; also LOD2
 *
 *  		Send queued updates to terrain to dling users;
 *  		continuously update terrain w/ queued updates
 *  		; make a copy of terrain chunks BEFORE editing,
 *  		then swap (swapbuffer) w/ newest changes when
 *  		not being accessed (SCHEDULER)
 *
 *  	> Vegetation, Erosion, Plate Tectonics, Lake
 *  		Networks, Cave/Dungeons, Mountains, Rocks
 *  	> Select area for some procedural generation and
 *  		noise
 *  	> Sculpt
 *  		
 *  		Use CSG for intersection w/ terrain; including
 *  		subdividing parts to fit closest to bend parts
 *  		(LOD 2); hard corner for LOD 3
 *			Use displacement map (noisy) on inside part of
 *			shape (stored in displacement map shader --
 *			refer to texture atlas for terrain -- NOT atlas
 *			has unique textures AND repeatedly used
 *			textures)
 *			Allow for drawing along terrain to select AREA
 *			of CSG object)
 *
 *			Apply heightmap to area (drawn possibly)
 *			Apply lake network to area
 *			Apply **** history to area (lakes takeover,
 *			lava, plate tectonics)
 *
 *		VEGETATION
 *			
 *			Place a ball in the scene in which all areas of
 *			the terrain which are in view of that ball will
 *			do post-rendering (plants, grass, moss, etc.).
 *			Use a texture atlas? or a vegetation texture
 *			map? to specify where plants are with respect to
 *			viewable position from ball... balls can be
 *			linked together along a chained path.
 *
 *			Automatic: grass area grows where visible to
 *			sun; partition chunk shaders where chunk is
 *			separated (outside portion of chunk, inside
 *			ceiling/cavern part of chunk)
 *
 *		MERGING?
 *		
 *
 *  - pause during procedural generation & add triangles;
 *  undo tree (backstep & resume); auto pause at certain
 *  part; add next X tri's (step x10); should scale for
 *  other things in future
 *  - FIX: overlapping triangles
 *  - add tri's in groups? (allow dynamic groups, and each
 *  group assigned a colour for easy viewing)
 *  - clean: TODO list, code, structure
 *  - optimize: static memory for terrain; pools of
 *  triangles?
 *
 *
 *
 *
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
 *
 *
 *
 *  *IDEAS*
 *  	- sweep hull to sort/store/cache chunks; re-create
 *  	those through delaunay(use polygons to create then
 *  	make tris from that..)(use int-level float
 *  	of initial send); then send next send of
 *  	float level of verts, stored in SAME order
 *  	(no need to send
 *  	  indexing then or specifying the position for each
 *  	  data (eg. color) val.. and use base environment
 *  	  details like base textures etc. to re-create
 *  	  approximation intially), then stream other details
 *  	  (geometry shaders, tessellation, bumpmaps,
 *  	  etc.) individually for further
 *  	  rebuilding the terrain and improving it..
 *  	  You can scale those send rates and quality
 *  	  of sends through priority & max rate
 *  	  levels offered by thread.. Those scales are
 *  	  through an interface that can apply to all
 *  	  threaded objects to manage resources and time.
 *  	  Stream various level of qualities at various
 *  	  distances for variously grouped arrays of chunks
 *  	  at various quality levels..you can profile or
 *  	  dynamically adapt to the best levels for
 *  	  efficiency
 *		
 **/


#include "util.inc.h"
#include <csignal>

#include "extern/GL/glew.h"

#include "libutils/lib_resmgr.h"
#include "libutils/lib_shdmgr.h"
#include "kernel/k_camera.h"


/*
=================================================

	Terrain

	Implements a voxel based terrain system. Terrain handles
	both graphical and physical model of the terrain. A
	voxel based approach allows for easy manipulation in a
	destructable terrain model.

=================================================
*/

struct Voxel;
struct Chunk;
template <class T> struct Point;
template <class T> struct QuadTree;
class Vertex;
struct TerrainSelection;
struct Triangle;
struct TriangleNode;
struct EdgeTriTree;
struct Tri;
class Terrain {
	// GLuint vao, vbo; // TODO: remove these?

public:
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// Terrain map specifications
	static const uint width,//  = 800,
		  	   depth ,// = 800,
			   height;// = 5000; // maximum height
	double dbgID = 0; // TODO: used for debugging terrain
					  // 		model, remove this or
					  // 		implement properly
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	Terrain(GLuint gl); // TODO: how to handle shaders?
	~Terrain();
	void clearTerrain();

	TerrainSelection* selection;
	Tri* terrainPick(glm::vec3 position, glm::vec3 direction);
	void selectTri(Tri*);

	Chunk* headChunk = 0;
	vector<Vertex> vertexBuffer; // all vertices in the terrain
	Voxel* voxelTree; // voxel tree of terrain
	vector<Triangle> debugTriangles; // TODO: remove or implement debug mode properly
	EdgeTriTree* edgeTree;


	/* Rendering
	 *
	 * Loops through chunk hextree and renders each
	 ***/
	void construct();
	void render();
	GLuint gl;

	/* Procedural Generation
	 *
	 * Applies a simple random growth function to each voxel
	 * during the generation of the voxel tree. Triangles
	 * are tessellated seamlessly between chunks.
	 ***/
	void generateTerrain(); // create voxel tree and randomize voxel heights (interpolated between neighbour points)
	Chunk* createChunks(Point<int> point, Chunk* below, Chunk* behind);
	vector<TriangleNode*> addTriangleIntoChunk(Chunk* chunk, Voxel* p0, Voxel* p1, Voxel* p2, bool suppressFailFix = false);
	vector<TriangleNode*> mergeT(vector<TriangleNode*>,vector<TriangleNode*>);
};

template<class T>
struct Point {
	Point() { }
	Point(T x, T y, T z) : x(x), y(y), z(z) { }
	T x; T y; T z;
};


struct Vertex {
	Vertex() { }
	Vertex(float v_x, float v_y, float v_z) : v_x(v_x), v_y(v_y), v_z(v_z) { }
	float v_x, v_y, v_z;
	bool operator ==(Vertex& vertex) {
		int rpt = 1; // rounding point
		return (v_x*rpt == vertex.v_x*rpt &&
				v_y*rpt == vertex.v_y*rpt &&
				v_z*rpt == vertex.v_z*rpt);
	}
	bool between(Vertex& v1, Vertex& v2) {
		return (
			((v_x>=v1.v_x && v_x<=v2.v_x)||(v_x>=v2.v_x && v_x<=v1.v_x)) &&
			((v_y>=v1.v_y && v_y<=v2.v_y)||(v_y>=v2.v_y && v_y<=v1.v_y)) &&
			((v_z>=v1.v_z && v_z<=v2.v_z)||(v_z>=v2.v_z && v_z<=v1.v_z))
		);
	}
};

struct Triangle {
	Triangle(ushort p0, ushort p1, ushort p2) : p0(p0), p1(p1), p2(p2) { }
	ushort p0, p1, p2;
	// uchar metadata;
	bool operator ==(Triangle& triangle) { 
		// TODO: fix this (backface culling)
		return ((p0==triangle.p0||p0==triangle.p1||p0==triangle.p2)&&(p1==triangle.p0||p1==triangle.p1||p1==triangle.p2)&&(p2==triangle.p0||p2==triangle.p1||p2==triangle.p2));
	}
};

struct TriangleNode {
	Chunk* chunk;
	unsigned short triangleID;
	TriangleNode* neighbour_p0p1;
	TriangleNode* neighbour_p0p2;
	TriangleNode* neighbour_p1p2;
};



/* Chunk
 *
 * The 3d multi-level equivalent of a patch
 * Chunks store a set of voxels surrounded by an AABB;
 * chunks contain voxels and triangles (each are stored in
 * contiguous memory and used for VBO's). Triangles are
 * stored as a Triple of indices to the stored voxels.
 * Chunks are apart of a HexTree where each face may be
 * connected to another Chunk. Each chunk contains a list of
 * portals (ptr to portal table) of portals that its
 * connected to.
 *
 * 		Voxel Storage
 *	Voxels are stored in an unordered resizable array
 *
 * 		Triangle Storage
 * 	Triangles contain 3 indices points; the triangles are
 * 	stored in a particular Shader object in LOD[0], as well
 * 	as any further LOD objects in which the triangle is
 * 	still necessary. 
 *
 * 		Adding Triangles
 * 	When triangles are added, some of the voxels may already
 * 	exist in the voxel container; if so then we simply use
 * 	that voxel as the index for that vertex of this
 * 	triangle. If ALL voxels are already inside the voxel
 * 	container then simply skip adding this triangle.
 * 	Sometimes a triangle may contain voxels which are
 * 	outside of the chunk; in this case those voxels are
 * 	projected onto the chunk and the projections are used
 * 	instead. These projections are returned to the caller so
 * 	that the caller may continuously add the triangle as
 * 	multiple parts to other chunks. These split triangles
 * 	are given a metadata value which states that its
 * 	currently shared across chunks, which helps in stitching
 * 	the seams across chunks after changes to the terrain
 *
 * 			 |*   |
 * 		|----|----|
 * 		|   *|*   |
 * 		|    |    |
 * 		|----|----|
 * 	
 *
 *
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
 **/

struct Voxel {
	Voxel() : vertexIndex(0), chunk(0) { }
	bool operator ==(Voxel& voxel) { return voxel.vertexIndex == vertexIndex; }
	Array_Resizable<Voxel*> neighbours;
	ushort vertexIndex; // index of vertex in associated vertexBuffer
	Chunk* chunk;
	Terrain* terrain;
	float getX() { return (terrain->vertexBuffer[vertexIndex].v_x); }
	float getY() { return (terrain->vertexBuffer[vertexIndex].v_y); }
	float getZ() { return (terrain->vertexBuffer[vertexIndex].v_z); }
};
struct Chunk {

	// Chunk specific information
	Chunk(Point<int> position);
	Point<int> worldOffset;
	static const Point<int> chunkSize;

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// Voxel & Triangle storage
	// TODO: do we really need voxelBuffer/triangleBuffer?
	// all of this is stored in VBO
	// vector<Voxel*> voxelBuffer;
	vector<Triangle> triangleBuffer;
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	/* Add Triangles
	 *
	 * Adding triangles into chunks checks first if the
	 * triangle fits entirely into the given chunk;
	 * otherwise it tessellates the triangle across the
	 * chunk's seam. 
	 *
	 * When the triangle needs to be tessellated there are a
	 * number of different cases in which it can be split
	 * across the chunk. These cases require a new set of
	 * vertices to be created along the triangle (ie.
	 * projected along the edges onto the seam of the chunk)
	 ***/
	struct AddTriangleResults {
		Voxel* projected_p1=0;
		Voxel* projected_p2=0;
		Voxel* projected_p1p2=0;
		Voxel* projected_p1Mid=0;
		Voxel* projected_p2Mid=0;
		Voxel* projected_midpoint=0;
		Voxel* outer_midpoint=0;
		unsigned short addedTriangle=0;
		uchar  addResults=0;

		const static uchar TRIANGLE_ADD_SUCCEEDED           = 0;
		const static uchar TRIANGLE_ADD_TWOPOINT_ONESIDE    = 1;
		const static uchar TRIANGLE_ADD_TWOPOINT_TWOSIDE    = 2;
		const static uchar TRIANGLE_ADD_ONEPOINT_ONESIDE    = 3;
		const static uchar TRIANGLE_ADD_ONEPOINT_TWOSIDE    = 4;
		const static uchar TRIANGLE_ADD_FOURPOINT_TWOSIDE   = 5;
		const static uchar TRIANGLE_ADD_FOURPOINT_THREESIDE = 6;
		const static uchar TRIANGLE_ADD_FAILED              = 7;
		const static uchar TRIANGLE_ADD_TWOPOINT_ONESIDE_NOMID = 8;
		const static uchar TRIANGLE_ADD_ONEPOINT_ONESIDE_ONEPROJ = 9;
	};
	AddTriangleResults addTriangle(Voxel* p0, Voxel* p1, Voxel* p2);
	bool isOutsideChunk(Voxel* p);
	Vertex* getVoxelProjection(Voxel* voxel, Voxel* neighbour, uchar* face);
	Vertex* projectVertexOntoFace(Vertex* voxel, Vertex* neighbour, uchar face);
	Vertex* getSeamIntersectionPoint(Vertex* p0, Vertex* p1, Vertex* p2, uchar face);
	Vertex* getSeamIntersectionPoint(Vertex* p0, Vertex* p1, Vertex* p2, uchar face1, uchar face2);
	Vertex* lineIntersectTriangle(float line_x, float line_y, float line_z, float line_dx, float line_dy, float line_dz, Vertex* p0, Vertex* p1, Vertex* p2);
	bool facesMatch(uchar expectedFace1, uchar expectedFace2, uchar face1, uchar face2);

	const static uchar FACE_FRONT   = 0;
	const static uchar FACE_BACK    = 1;
	const static uchar FACE_LEFT    = 2;
	const static uchar FACE_RIGHT   = 3;
	const static uchar FACE_TOP     = 4;
	const static uchar FACE_BOTTOM  = 5;


	void construct();
	void render(float r, float g, float b);
	GLuint vbo, vao, elementBuffer;


	// HexTree
	Chunk* above     = 0;
	Chunk* below     = 0;
	Chunk* left      = 0;
	Chunk* right     = 0;
	Chunk* infront   = 0;
	Chunk* behind    = 0;
	Terrain* terrain = 0;
};

struct Tri {
	Tri(Chunk* chunk, ushort triIndex) : chunk(chunk), triIndex(triIndex) {
		p0 = chunk->triangleBuffer[triIndex].p0;
		p1 = chunk->triangleBuffer[triIndex].p1;
		p2 = chunk->triangleBuffer[triIndex].p2;
		Log(str(format("***New Tri*** {%1%,%2%,%3%}: ")%p0%p1%p2));
	}
	ushort triIndex;
	Chunk* chunk = 0;
	ushort p0, p1, p2;

	ushort getOddPoint(ushort p0, ushort p1);
	static bool oneOrTheOther(ushort p0, ushort p1, ushort match_p0, ushort match_p1);
	Tri** getNeighbourOnEdge(ushort p0, ushort p1);
	void reshapeTriOnEdge(ushort p0, ushort p1, ushort p0_new, ushort p1_new);

	static void assertBadTri(Tri* tri);

	Tri* neighbour_p0p1 = 0;
	Tri* neighbour_p1p2 = 0;
	Tri* neighbour_p2p0 = 0;
};
struct EdgeTriTree {

	static Terrain* terrain;
	EdgeTriTree(Terrain* terrain);
	~EdgeTriTree();

	/* Edge Chunk
	 *
	 * An AABB chunk storing EdgeTriNode's; note that edges
	 * can span across more than one chunk, and hence chunks
	 * store pointers to Edges which may be shared by
	 * multiple chunks
	 *
	 ***/
	struct EdgeTriNode;
	struct EdgeChunk {

		EdgeChunk(Point<int> position);
		Point<int> worldOffset;
		static const Point<int> chunkSize;
		vector<EdgeTriNode*> edges;
		static bool vectorContainsEdgeChunk(vector<EdgeChunk*>* chunks, EdgeChunk* chunk) {
			for ( auto e_chunk : (*chunks) ) {
				if ( e_chunk == chunk ) return true;
			}
			return false;
		}

		static void addMissingEdgeChunks(vector<EdgeChunk*>* chunks, EdgeChunk* chunk, ushort vertexIndex, Terrain* terrain ) {
			
			float x = terrain->vertexBuffer[vertexIndex].v_x;
			float y = terrain->vertexBuffer[vertexIndex].v_y;
			float z = terrain->vertexBuffer[vertexIndex].v_z;
			if ( x <= chunk->worldOffset.x && chunk->left && !EdgeChunk::vectorContainsEdgeChunk( chunks, chunk->left ) ) chunks->push_back( chunk->left );
			if ( y <= chunk->worldOffset.y && chunk->below && !EdgeChunk::vectorContainsEdgeChunk( chunks, chunk->below ) ) chunks->push_back( chunk->below );
			if ( z <= chunk->worldOffset.z && chunk->behind && !EdgeChunk::vectorContainsEdgeChunk( chunks, chunk->behind ) ) chunks->push_back( chunk->behind );

			if ( x >= chunk->worldOffset.x + chunk->chunkSize.x && chunk->right && !EdgeChunk::vectorContainsEdgeChunk( chunks, chunk->right ) ) chunks->push_back( chunk->right );
			if ( y >= chunk->worldOffset.z + chunk->chunkSize.y && chunk->above && !EdgeChunk::vectorContainsEdgeChunk( chunks, chunk->above ) ) chunks->push_back( chunk->above );
			if ( z >= chunk->worldOffset.z + chunk->chunkSize.z && chunk->infront && !EdgeChunk::vectorContainsEdgeChunk( chunks, chunk->infront ) ) chunks->push_back( chunk->infront );
		}


		// HexTree
		EdgeChunk* above     = 0;
		EdgeChunk* below     = 0;
		EdgeChunk* left      = 0;
		EdgeChunk* right     = 0;
		EdgeChunk* infront   = 0;
		EdgeChunk* behind    = 0;

		const static uchar FACE_FRONT   = 0;
		const static uchar FACE_BACK    = 1;
		const static uchar FACE_LEFT    = 2;
		const static uchar FACE_RIGHT   = 3;
		const static uchar FACE_TOP     = 4;
		const static uchar FACE_BOTTOM  = 5;

		float edgeHitPoint(Vertex p0, float dx, float dy, float dz, uchar face);
		bool pointInFace(Vertex hitpoint, uchar face);
	};
	EdgeChunk* headChunk = 0;
	EdgeChunk* createChunks(Point<int> position, EdgeChunk* below, EdgeChunk* behind);

	struct EdgeTriNode {
		ushort p0, p1; // edge

		/* Get Containers
		 * 
		 * A point is inside the container if it is >=
		 * offset but < offset+size; hence [x0,x1)
		 * NOTE: It is important to make a distinction
		 * between which chunk a point belongs to, since a
		 * point is only allowed to belong to ONE chunk
		 */
		static vector<EdgeChunk*> getContainers(ushort p0, ushort p1);
		static vector<EdgeChunk*> getContainer(ushort p0);
		vector<EdgeChunk*> getContainers();
		Tri* triangle_p0p1 = 0; // triangle on p0p1 side
		Tri* triangle_p1p0 = 0; // triangle on p1p0 side

		vector<EdgeTriNode*> p0_edges;
		vector<EdgeTriNode*> p1_edges;

		// vector<Tri*> triangles_p0p1; // triangles on p0p1 side
		// vector<Tri*> triangles_p1p0; // triangles on p1p0 side

		// EdgeTriNode* leftNode  = 0;
		// EdgeTriNode* rightNode = 0;

		// void subdivideTriangles(); // subdivide the edge and re-neighbour the tri's
	};

	/* Edge List
	 *
	 * Edges come in a pair (p0,p1) == (p1,p0) and are
	 * sorted. Edges are stored in a linked list of linked
	 * lists. p0 is ALWAYS less than p1 (if not, then we
	 * switch p0 and p1 in the add process and add the
	 * triangle to the other side of the edge)
	 *
	 * Parent Linked List: sorted list of {p0, linked list},
	 * where we sort by p0 (index of vertex) and store a
	 * linked list with that vertex
	 *
	 * Child Linked List: sort list of {p1, EdgeTriNode},
	 * sorted by p1
	 *
	 * TODO: consider a table which gives ptr to each
	 * half-way point (when list becomes big enough, add
	 * quater-way point, etc.)
	 *
	 ***/
	/*
	struct PointNode {
		ushort p0;
		EdgeTriNode* edgeNode = 0;
		PointNode* leftPoint  = 0;
		PointNode* rightPoint = 0;
	};
	*/


	// PointNode* cachedPoint = 0; // cached ptr to last accessed point node
	// vector<EdgeTriNode*> nodesNeedSubdividing;
	void addTriangle(Chunk* chunk, ushort triIndex);
	void addTriangle(Chunk* chunk, ushort triIndex, ushort p0, ushort p1);
	// TODO: linkedList of linkedList of edges; sort outer
	// and inner lists by ushort index. addTriangle adds to
	// linkedList by switching p0/p1 to make p0<p1; if
	// switched specify we're on other side of edge. Cache
	// lists to last used point and start from there during
	// add process. NOTE: use vertex index, NOT
	// chunk-dependent triangle indices
	// EdgeTri* addTriangle(Tri* tri, ushort p0, ushort p1);
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
	struct SelectionClass {
		const static uchar CLASS_HIGHLIGHT = 1;
		const static uchar CLASS_HIGHLIGHT_NEIGHBOUR = 2;
		int class_id;
		vector<Triangle> triangles;
	};
	vector<SelectionClass*> selections;
	// LinkedList_Line<QuadTree<Voxel>> inner_selection;
	// LinkedList_Circle<QuadTree<Voxel>> inner_neighbours;
	// LinkedList_Circle<QuadTree<Voxel>> mid_neighbours;
	// LinkedList_Circle<QuadTree<Voxel>> outer_neighbours;
};



#endif
