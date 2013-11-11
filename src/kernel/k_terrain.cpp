#include "kernel/k_terrain.h"

// const Point<int> Chunk::chunkSize = Point<int>(113,113,113);
const Point<int> Chunk::chunkSize = Point<int>(501,201,113);

// ============================================== //
Terrain::Terrain(GLuint gl) : gl(gl) {
	generateTerrain();
	construct();
}
// ============================================== //


// ============================================== //
Terrain::~Terrain() {

}
// ============================================== //


// ============================================== //
void Terrain::clearTerrain() {

}
// ============================================== //


// ============================================== //
void Terrain::construct() {


	Chunk* farBackLeft = headChunk;
	Chunk* farLeft;
	Chunk* curChunk;
	while ( farBackLeft ) {
		farLeft = farBackLeft;
		while ( farLeft ) {
			curChunk = farLeft;
			while ( curChunk ) {
				curChunk->construct();
				curChunk = curChunk->right;
			}
			farLeft = farLeft->infront;
		}
		farBackLeft = farBackLeft->above;
	}

}
// ============================================== //


// ============================================== //
void Terrain::render() {


	Chunk* farBackLeft = headChunk;
	Chunk* farLeft;
	Chunk* curChunk;
	float r, g, b;
	r = 0; g = 0; b = 0;
	float incAmount = 0.5f;
	while ( farBackLeft ) {
		farLeft = farBackLeft;
		while ( farLeft ) {
			curChunk = farLeft;
			while ( curChunk ) {
				curChunk->render(r, g, b);
				curChunk = curChunk->right;
			}
			farLeft = farLeft->infront;
			b += incAmount;
			if ( b > 1.0f ) {
				b = 0.0f;
				g += incAmount;
				if ( g > 1.0f ) {
					g = 0.0f;
					r += incAmount;
					if ( r > 1.0f ) {
						r = 0.0f;
					}
				}
			}
		}
		farBackLeft = farBackLeft->above;
	}
	// TODO: draw terrain decal (selection indicator)
	GLuint decal_vbo, decal_ibo;
	glGenBuffers( 1, &decal_vbo );
	glBindBuffer( GL_ARRAY_BUFFER, decal_vbo );
	glBufferData( GL_ARRAY_BUFFER, ( vertexBuffer.size() ) * sizeof(Vertex), vertexBuffer.data(), GL_STATIC_DRAW );

	GLint glVertex   = glGetAttribLocation  ( gl, "in_Position" ) ;
	glEnableVertexAttribArray( glVertex );

	// load data into shader
	glVertexAttribPointer( glVertex, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0 );

	glGenBuffers( 1, &decal_ibo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, decal_ibo );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, debugTriangles.size() * sizeof(Triangle), debugTriangles.data(), GL_STATIC_DRAW );


	GLint glMVP = glGetUniformLocation( gl, "MVP" );

	glm::mat4 mvp = camera.perspectiveView;
	mvp = glm::transpose(mvp);
	glUniformMatrix4fv( glMVP, 1, GL_FALSE, glm::value_ptr(mvp) );
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glUniform3f( glGetUniformLocation( gl, "in_color" ), 1.0f, 0.8f, 0.0f );
	glDrawElements( GL_TRIANGLES, debugTriangles.size()*3, GL_UNSIGNED_SHORT, (void*)0 );
	glDeleteBuffers( 1, &decal_vbo );
	glDeleteBuffers( 1, &decal_ibo );

}
// ============================================== //


	/******************************************************************************/
	/*
							  Procedural Terrain Generation
																				   */
	/******************************************************************************/



// ============================================== //
void Terrain::generateTerrain() {

	// Chunk Generation
	// 	create a chunk hextree along the entire map grid; even if no voxels will exist in
	// 	some of these chunks, its important to set them up (sort of like an empty space
	// 	rather than a null space).
	// 	TODO: consider a dynamically expanding chunk space
	int x_chunks = width  / Chunk::chunkSize.x + 1,
		y_chunks = height / Chunk::chunkSize.y + 1,
		z_chunks = depth  / Chunk::chunkSize.z + 1;

	Point<int> chunkPosition = Point<int>(0,0,0);
	Chunk* chunkStartPoint = headChunk;
	Chunk* chunkCurrent = headChunk;
	Chunk* chunkPreviousRow = headChunk;
	Chunk* chunkPreviousDepth = 0;
	for ( int y = 0; y < y_chunks; ++y ) {
		chunkStartPoint = 0;
		for ( int z = 0; z < z_chunks; ++z ) {
			chunkCurrent = createChunks( chunkPosition, chunkPreviousDepth, chunkPreviousRow ); // create all chunks along X axis
			if ( !chunkStartPoint ) chunkStartPoint = chunkCurrent; // keep the (0,0) of this Y level
			
			chunkPreviousRow = chunkCurrent;
			if ( chunkPreviousDepth ) chunkPreviousDepth = chunkPreviousDepth->infront;
			chunkPosition.z += Chunk::chunkSize.z;
		}

		if ( !headChunk ) headChunk = chunkStartPoint;
		chunkPreviousDepth = chunkStartPoint;
		chunkPreviousRow   = 0;
		chunkPosition.y   += Chunk::chunkSize.y;
		chunkPosition.z    = 0;
	}



	// Voxel Generation
	//
	// Create the voxel tree and setup triangles during the voxel tree generation.
	// Triangles are immediately added (and possibly tessellated) into the chunk hextree.
	// The triangles are then joined together into a tritree, and any triangle who's edge
	// has been split and is shared with a neighbour triangle will split that neighbour
	// triangle to avoid any T-junctions
	//
	// To ease the generation process, voxels are given 4 neighbour points immediately on
	// creation. The neighbours are understood as, [NORTH,EAST,SOUTH,WEST]; hence voxel[1]
	// would be its east neighbour. Neighbours are initially 0, and then changed when
	// neighboured. At the end of the process any empty neighbours are simply deleted, and
	// the whole NESW-compass tree is gone, leaving only the connected tree (including any
	// extra neighbours that may have been made)
	// 
	vector<Voxel*> voxelBuffer; // stored here temporarily (to easily clean up tree afterwards)
	voxelTree = 0;
	Voxel* voxelStartPoint = voxelTree;
	Voxel* voxelCurrent = voxelTree;
	Voxel* voxelPrevRow = 0;
	Voxel* voxelPrev = voxelTree;
	uint neighbourDepth = 40,
		 neighbourRight = 40;
	int  heightRange = 30,
		 heightMin   = 0;
	for ( int forwards = 0; forwards < depth; forwards += neighbourDepth ) {
		voxelStartPoint = 0;
		voxelPrev = 0;
		for ( int right = 0; right < width; right += neighbourRight ) {
			voxelCurrent = new Voxel();
			voxelBuffer.push_back( voxelCurrent );
			voxelCurrent->terrain = this;

			float v_y = 0.0f;
			int sign = 1;//(rand()%3) - 1;
			if ( voxelPrev && voxelPrevRow ) {
				v_y = 0.5*(sign*rand()%heightRange + voxelPrev->getY()) + 0.5*(sign*rand()%heightRange + voxelPrevRow->getY());
			} else if ( voxelPrev ) {
				v_y = sign*rand()%heightRange + voxelPrev->getY();
			} else if ( voxelPrevRow ) {
				v_y = sign*rand()%heightRange + voxelPrevRow->getY();
			} else {
				v_y = sign*rand()%heightRange;
			}
			if ( v_y < heightMin ) v_y = heightMin;

			vertexBuffer.push_back(Vertex(right, v_y, forwards));
			voxelCurrent->vertexIndex = vertexBuffer.size() - 1;
			voxelCurrent->neighbours.add(0);
			voxelCurrent->neighbours.add(0);
			voxelCurrent->neighbours.add(voxelPrevRow);
			voxelCurrent->neighbours.add(voxelPrev);
			if ( !voxelStartPoint ) voxelStartPoint = voxelCurrent;
			if ( voxelPrev ) voxelPrev->neighbours[1] = voxelCurrent;
			if ( voxelPrevRow ) voxelPrevRow->neighbours[0] = voxelCurrent;

			if ( voxelPrevRow ) {
				if ( voxelPrevRow->neighbours[3] ) {
					voxelCurrent->neighbours.add( voxelPrevRow->neighbours[3] );
					voxelPrevRow->neighbours[3]->neighbours.add(voxelCurrent);

					vector<TriangleNode*> addedTriangles;

					++dbgID;
					Log(str(format("Requesting Triangle(1) [%10%]: <%1%,%2%,%3%>, <%4%,%5%,%6%>, <%7%,%8%,%9%>")%voxelCurrent->getX()%voxelCurrent->getY()%voxelCurrent->getZ()%voxelPrevRow->neighbours[3]->getX()%voxelPrevRow->neighbours[3]->getY()%voxelPrevRow->neighbours[3]->getZ()%voxelPrevRow->getX()%voxelPrevRow->getY()%voxelPrevRow->getZ()%dbgID));
					addedTriangles = addTriangleIntoChunk( headChunk, voxelCurrent, voxelPrevRow->neighbours[3], voxelPrevRow );

					++dbgID;
					Log(str(format("Requesting Triangle(2) [%10%]: <%1%,%2%,%3%>, <%4%,%5%,%6%>, <%7%,%8%,%9%>")%voxelCurrent->getX()%voxelCurrent->getY()%voxelCurrent->getZ()%voxelCurrent->neighbours[3]->getX()%voxelCurrent->neighbours[3]->getY()%voxelCurrent->neighbours[3]->getZ()%voxelPrevRow->neighbours[3]->getX()%voxelPrevRow->neighbours[3]->getY()%voxelPrevRow->neighbours[3]->getZ()%dbgID));
					Chunk* cachedChunk = headChunk;
					if ( addedTriangles.size() ) cachedChunk = addedTriangles[0]->chunk;
					addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( cachedChunk, voxelCurrent, voxelCurrent->neighbours[3], voxelPrevRow->neighbours[3] ));
				}

				voxelPrevRow = voxelPrevRow->neighbours[1];
			}
			voxelPrev = voxelCurrent;
			voxelCurrent = voxelCurrent->neighbours[1];
		}
		if ( !voxelTree ) voxelTree = voxelStartPoint;
		voxelPrevRow = voxelStartPoint;
	}

	// clean voxel tree (remove compass-tree)
	for ( Voxel* voxel : voxelBuffer ) {
		while ( voxel->neighbours.has((Voxel*)0) ) {
			voxel->neighbours.remove(0);
		}
	}
}
// ============================================== //


// ============================================== //
Chunk::Chunk(Point<int> position) {
	worldOffset.x = position.x;
	worldOffset.y = position.y;
	worldOffset.z = position.z;

}
// ============================================== //


// ============================================== //
vector<TriangleNode*> Terrain::addTriangleIntoChunk(Chunk* chunk, Voxel* p0, Voxel* p1, Voxel* p2, unsigned long p0_outer = -1, unsigned long p1_outer = -1, unsigned long p2_outer = -1) {

	/* Find corresponding voxel
	 * We can only guarantee a matching chunk for one voxel; in the case that each voxel
	 * is contained within separate voxels then the triangle must be split up in order to
	 * make up the space within each chunk:
	 *
	 * |  * |    |
	 * |----+----|
	 * |   *|  * |
	 * |    |    |
	 * |---------|
	 *
	 **/
	float x = vertexBuffer[p0->vertexIndex].v_x,
		  y = vertexBuffer[p0->vertexIndex].v_y,
		  z = vertexBuffer[p0->vertexIndex].v_z;

	// chunk further ahead?
	if ( x - chunk->worldOffset.x > chunk->chunkSize.x ) return addTriangleIntoChunk(chunk->right,   p0, p1, p2);
	if ( y - chunk->worldOffset.y > chunk->chunkSize.y ) return addTriangleIntoChunk(chunk->above,   p0, p1, p2);
	if ( z - chunk->worldOffset.z > chunk->chunkSize.z ) return addTriangleIntoChunk(chunk->infront, p0, p1, p2);

	// chunk is behind?
	if ( x < chunk->worldOffset.x ) return addTriangleIntoChunk(chunk->left,   p0, p1, p2);
	if ( y < chunk->worldOffset.y ) return addTriangleIntoChunk(chunk->below,  p0, p1, p2);
	if ( z < chunk->worldOffset.z ) return addTriangleIntoChunk(chunk->behind, p0, p1, p2);


	/* Add triangle into this chunk
	 *
	 * Attempt to add triangle into chunk; however if this triangle cannot fit inside the
	 * chunk then the projection details are provided in the return value, as well as the
	 * new projections
	 *
	 * WARNING WARNING WARNING WARNING WARNING
	 * 	when adding triangles, make sure to add with the first vertex inside of the chunk
	 * 	you're expecting, otherwise you could run into an infinite recursive loop of
	 * 	adding triangles
	 * WARNING WARNING WARNING WARNING WARNING
	 *
	 *
	 * Blender Visualization
	 *
	 *
def noTri(messy_str):
	eval("AddTri( " + messy_str.replace('<','').replace('>','') + " )")

def AddTri(x1,z1,y1,x2,z2,y2,x3,z3,y3):
	mesh = D.meshes.new("Tri")
	mesh.from_pydata([(x1,y1,z1),(x2,y2,z2),(x3,y3,z3)], [], [(0,1,2)])
	mesh.update()
	ob = D.objects.new("Tri", mesh)
	ob.data = mesh
	C.scene.objects.link(ob)

def AddAABB(x,y,z,size):
	mesh = D.meshes.new("Chunk")
	mesh.from_pydata([(x,y,z),(x+size,y,z),(x,y+size,z),(x+size,y+size,z),(x,y,z+size),(x+size,y,z+size),(x,y+size,z+size),(x+size,y+size,z+size)],
	[], [ (0,1,3,2), (0,1,5,4), (0,2,6,4), (1,3,7,5), (2,3,7,6), (4,5,7,6) ])
	mesh.update()
	ob = D.objects.new("Chunk", mesh)
	ob.data = mesh
	C.scene.objects.link(ob)


0 (x,y,z)
1 (x+size,y,z)
2 (x,y+size,z)
3 (x+size,y+size,z)
4 (x,y,z+size)
5 (x+size,y,z+size)
6 (x,y+size,z+size)
7 (x+size,y+size,z+size)

(0,1,3,2) BOTTOM
(0,1,5,4) BACK
(0,2,6,4) LEFT
(1,3,7,5) RIGHT
(2,3,7,6) FRONT
(4,5,7,6) TOP

	 **/
	vector<TriangleNode*> addedTriangles;
	Chunk::AddTriangleResults addedResults = chunk->addTriangle(p0, p1, p2); // Tessellated Triangle
	if ( addedResults.addResults == Chunk::AddTriangleResults::TRIANGLE_ADD_SUCCEEDED ) {

		// succeeded in adding triangle
		// debugTriangles.push_back( chunk->triangleBuffer[addedResults.addedTriangle] ); // DEBUG
		TriangleNode* addedTriangle = new TriangleNode();
		addedTriangle->chunk = chunk;
		addedTriangle->triangleID = addedResults.addedTriangle;
		addedTriangles.push_back( addedTriangle );

		ushort edge_p0 = (p0_outer>=0?p0_outer:chunk->triangleBuffer[addedResults.addedTriangle].p0);
		ushort edge_p1 = (p1_outer>=0?p1_outer:chunk->triangleBuffer[addedResults.addedTriangle].p1);
		ushort edge_p2 = (p2_outer>=0?p2_outer:chunk->triangleBuffer[addedResults.addedTriangle].p2);
		edgeTree.addTriangle( chunk, addedResults.addedTriangle, edge_p0, edge_p1 );
		edgeTree.addTriangle( chunk, addedResults.addedTriangle, edge_p1, edge_p2 );
		edgeTree.addTriangle( chunk, addedResults.addedTriangle, edge_p2, edge_p0 );

	} else if ( addedResults.addResults ==
			Chunk::AddTriangleResults::TRIANGLE_ADD_TWOPOINT_ONESIDE ) {

		// 2 outside points projected onto 1 face
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p1, addedResults.projected_p2, p0, p0, p1, p2 ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p1, addedResults.projected_midpoint, addedResults.projected_p1, p0, p1, -1 ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, addedResults.projected_midpoint, addedResults.projected_p2, addedResults.projected_p1 ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p2, addedResults.projected_p2, addedResults.projected_midpoint ));

	} else if ( addedResults.addResults ==
			Chunk::AddTriangleResults::TRIANGLE_ADD_TWOPOINT_TWOSIDE ) {

		// 2 outside points projected onto 2 faces
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p1, addedResults.projected_p2 ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, addedResults.projected_p1, addedResults.projected_p1p2, addedResults.projected_p2 ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p1, addedResults.projected_p1p2, addedResults.projected_p1 ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p1, p2, addedResults.projected_p1p2 ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p2, addedResults.projected_p2, addedResults.projected_p1p2 ));

	} else if ( addedResults.addResults ==
			Chunk::AddTriangleResults::TRIANGLE_ADD_ONEPOINT_ONESIDE ) {

		// 1 outside point projects onto 1 face
		if ( addedResults.projected_p1 ) {
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p1, p2 ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p2, addedResults.projected_p1, addedResults.projected_p1p2 ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p1, addedResults.projected_p1p2, addedResults.projected_p1 ));
		} else {
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, p1, addedResults.projected_p1p2 ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p1p2, addedResults.projected_p2 ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p2, addedResults.projected_p2, addedResults.projected_p1p2 ));
		}

	} else if ( addedResults.addResults ==
			Chunk::AddTriangleResults::TRIANGLE_ADD_ONEPOINT_TWOSIDE ) {

		// 1 outside point projects onto 2 faces
		if ( addedResults.projected_p1 ) {
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p1, p2 ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p2, addedResults.projected_p1, addedResults.projected_p1p2 ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, addedResults.projected_p1p2, addedResults.projected_p1, addedResults.projected_midpoint ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p1, addedResults.projected_midpoint, addedResults.projected_p1 ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p1, addedResults.projected_p1p2, addedResults.projected_midpoint ));
		} else {
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, p1, addedResults.projected_p1p2 ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p1p2, addedResults.projected_p2 ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, addedResults.projected_p2, addedResults.projected_p1p2, addedResults.projected_midpoint ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p2, addedResults.projected_midpoint, addedResults.projected_p1p2 ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p2, addedResults.projected_p2, addedResults.projected_midpoint ));
		}

	} else if ( addedResults.addResults ==
			Chunk::AddTriangleResults::TRIANGLE_ADD_FOURPOINT_TWOSIDE ) {

		// 2 outside points project onto 2 faces; edge between outside points intersects chunk
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p1Mid, addedResults.projected_p2Mid ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p1, addedResults.projected_p1Mid ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p2Mid, addedResults.projected_p2 ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p1, addedResults.projected_p1Mid, addedResults.projected_p1 ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p2, addedResults.projected_p2, addedResults.projected_p2Mid ));

	} else if ( addedResults.addResults ==
			Chunk::AddTriangleResults::TRIANGLE_ADD_FOURPOINT_THREESIDE ) {

		// TODO
		assert(false);
	}

	p0->chunk = chunk;
	p1->chunk = chunk;
	p2->chunk = chunk;
	return addedTriangles;

}
// ============================================== //


// ============================================== //
vector<TriangleNode*> Terrain::mergeT(vector<TriangleNode*> lista, vector<TriangleNode*> list2) {

	for ( auto item : list2 ) {
		lista.push_back(item);
	}
	return lista;
}
// ============================================== //


// ============================================== //
void EdgeTriTree::addTriangle(Chunk* chunk, ushort triIndex) {

	Tri* tri = new Tri(chunk, triIndex);
	ushort p0 = chunk->triangleBuffer[triIndex].p0;
	ushort p1 = chunk->triangleBuffer[triIndex].p1;
	ushort p2 = chunk->triangleBuffer[triIndex].p2;
	addTriangle( tri, p0, p1 );
	addTriangle( tri, p1, p2 );
	addTriangle( tri, p2, p0 );

}
// ============================================== //


// ============================================== //
EdgeTri* addTriangle(Tri* tri, ushort p0, ushort p1) {

	// which side of the edge are we?
	bool p0p1_side = true;
	if ( p0 < p1 ) {
		p0 ^= p1 ^= p0 ^= p1;
		p0p1_side = false;
	}

	// find point node
	PointNode* startPoint = cachedPoint;
	PointNode* nodePoint = 0;
	while ( startPoint ) {
		if ( startPoint->p0 == p0 ) {
			nodePoint = startPoint;
			break;
		} else if ( startPoint->p0 < p0 ) {
			if ( startPoint->rightPoint && startPoint->rightPoint->p0 > p0 ) {
				// node point between these nodes
				nodePoint = new PointNode();
				nodePoint->rightPoint = startPoint->rightPoint;
				nodePoint->rightPoint->leftPoint = nodePoint;
				nodePoint->leftPoint = startPoint;
				startPoint->rightPoint = nodePoint;
				nodePoint->p0 = p0;
				break;
			}
			
			if ( startPoint->rightPoint ) {
				startPoint = startPoint->rightPoint;
			} else {
				// node point on the end
				nodePoint = new PointNode();
				nodePoint->leftPoint = startPoint;
				startPoint->rightPoint = nodePoint;
				nodePoint->p0 = p0;
				break;
			}
		} else {
			if ( startPoint->leftPoint && startPoint->leftPoint->p0 < p0 ) {
				// node point between these nodes
				nodePoint = new PointNode();
				nodePoint->leftPoint = startPoint->leftPoint;
				nodePoint->leftPoint->rightPoint = nodePoint;
				nodePoint->rightPoint = startPoint;
				startPoint->leftPoint = nodePoint;
				nodePoint->p0 = p0;
				break;
			}

			if ( startPoint->leftPoint ) {
				startPoint = startPoint->leftPoint;
			} else {
				// node point on the end
				nodePoint = new PointNode();
				nodePoint->rightPoint = startPoint;
				startPoint->leftPoint = nodePoint;
				nodePoint->p0 = p0;
				break;
			}
		}
	}

	// empty list
	if ( !nodePoint ) {
		nodePoint = new PointNode();
		nodePoint->p0 = p0;
		cachedPoint = nodePoint;
	}
	cachedPoint = nodePoint;


	// find edge adjacent to this point
	EdgeTriNode* startEdge = nodePoint->edgeNode;
	EdgeTriNode* edge = 0;
	while ( startEdge ) {
		if ( startEdge->p1 == p1 ) {
			edge = startEdge;
			break;
		} else if ( startEdge->p1 < p1 ) {
			if ( startEdge->rightNode && startEdge->rightNode->p1 > p1 ) {
				// node edge between these nodes
				edge = new EdgeTriNode();
				edge->rightNode = startEdge->rightNode;
				edge->rightNode->leftNode = edge;
				edge->leftNode = startEdge;
				startEdge->rightNode = edge;
				edge->p0 = p0;
				edge->p1 = p1;
				break;
			}
			
			if ( startEdge->rightNode ) {
				startEdge = startEdge->rightNode;
			} else {
				// node edge on the end
				edge = new EdgeTriNode();
				edge->leftNode = startEdge;
				startEdge->rightNode = edge;
				edge->p0 = p0;
				edge->p1 = p1;
				break;
			}
		} else {
			if ( startEdge->leftNode && startEdge->leftNode->p1 < p1 ) {
				// node edge between these nodes
				edge = new EdgeTriNode();
				edge->leftNode = startEdge->leftNode;
				edge->leftNode->rightNode = edge;
				edge->rightNode = startEdge;
				startEdge->leftNode = edge;
				edge->p0 = p0;
				edge->p1 = p1;
				break;
			}
			
			if ( startEdge->leftNode ) {
				startEdge = startEdge->leftNode;
			} else {
				// node edge on the end
				edge = new EdgeTriNode();
				edge->rightNode = startEdge;
				startEdge->leftNode = edge;
				edge->p0 = p0;
				edge->p1 = p1;
				break;
			}

		}
	}

	// empty list
	if ( !edge ) {
		edge = new EdgeTriNode();
		edge->p0 = p0;
		edge->p1 = p1;
		nodePoint->edgeNode = edge;
	}


	// push triangle to one side of the edge
	if ( p0p1_side ) {
		edge->triangles_p0p1.push_back( tri );
	} else {
		edge->triangles_p1p0.push_back( tri );
	}

	// we cannot subdivide the triangles just yet, so add this edge to the update list
	if ( (edge->triangles_p0p1.size() > 1 || edge->triangles_p1p0.size() > 1) &&
			edge->triangles_p0p1.size() != 0 && edge->triangles_p1p0.size() != 0 ) {
		bool foundEdgeInUpdateList = false;
		for ( auto edgeNode : nodesNeedSubdividing ) {
			if ( edgeNode == edge ) {
				foundEdgeInUpdateList = true;
				break;
			}
		}
		if ( !foundEdgeInUpdateList ) {
			nodesNeedSubdividing.push_back( edge );
		}
	// TODO: neighbour two tris if their edges span this entire edge
		} else if ( edge->triangles_p0p1.size() == 1 && edge->triangles_p1p0.size() == 1 ) {
			// neighbour these tris if their edge spans across the entire edge
			Tri* leftTri  = edge->triangles_p0p1[0];
			Tri* rightTri = edge->triangles_p1p0[0];

			if ( (leftTri->p0 == endPoint && rightTri->p0 == endPoint && ( leftTri->p1 != rightTri->p2 )) ||
				 (leftTri->p0 == endPoint && rightTri->p1 == endPoint && ( leftTri->p1 != rightTri->p0 )) ||
				 (leftTri->p0 == endPoint && rightTri->p2 == endPoint && ( leftTri->p1 != rightTri->p1 )) ||

				 (leftTri->p1 == endPoint && rightTri->p0 == endPoint && ( leftTri->p2 != rightTri->p2 )) ||
				 (leftTri->p1 == endPoint && rightTri->p1 == endPoint && ( leftTri->p2 != rightTri->p0 )) ||
				 (leftTri->p1 == endPoint && rightTri->p2 == endPoint && ( leftTri->p2 != rightTri->p1 )) ||

				 (leftTri->p2 == endPoint && rightTri->p0 == endPoint && ( leftTri->p0 != rightTri->p2 )) ||
				 (leftTri->p2 == endPoint && rightTri->p1 == endPoint && ( leftTri->p0 != rightTri->p0 )) ||
				 (leftTri->p2 == endPoint && rightTri->p2 == endPoint && ( leftTri->p0 != rightTri->p1 )) ) {

				bool foundEdgeInUpdateList = false;
				for ( auto edgeNode : nodesNeedSubdividing ) {
					if ( edgeNode == edge ) {
						foundEdgeInUpdateList = true;
						break;
					}
				}
				nodesNeedSubdividing.push_back( edge );
			}



			if ( tri->p0 == endPoint ) {
				associatedLeftTri = tri;
				leftTri_p0 = tri->p0;
				leftTri_p1 = tri->p1;
				leftTri_p2 = tri->p2;
				left_p1_p  = 1;
				leftTriNeighbour = tri->neighbour_p0p1;
				break;
			} else if ( tri->p1 == endPoint ) {
				associatedLeftTri = tri;
				leftTri_p0 = tri->p1;
				leftTri_p1 = tri->p2;
				leftTri_p2 = tri->p0;
				left_p1_p  = 2;
				leftTriNeighbour = tri->neighbour_p1p2;
				break;
			} else if ( tri->p2 == endPoint ) {
				associatedLeftTri = tri;
				leftTri_p0 = tri->p2;
				leftTri_p1 = tri->p0;
				leftTri_p2 = tri->p1;
				left_p1_p  = 0;
				leftTriNeighbour = tri->neighbour_p2p0;
				break;
			}
		}
	}

}
// ============================================== //


// ============================================== //
void EdgeTriTree::EdgeTriNode::subdivideTriangles() {

	// TODO: subdivide: create new tris for each neighbouring
	//		> loop through on each side, from p0 to (p1X, p1Y) nearest to p0, create new tri on opposite side &
	//		  neighbour tris, create new EdgeTriNode w/ tris (dont add to tree yet)
	//		  	> SIDE: find tri connected to p0, find its p1 (between p0 and p1), set p1 as new p0 point in next
	//		  	  iteration; 

	// TODO: either side is empty? skip
	
	ushort endPoint = p0;
	vector<EdgeTriNode*> edgeSubdivisions;
	while ( endPoint != p1 ) {
		
		// subdivided edge
		EdgeTriNode* edge = new EdgeTriNode();
		edge->p0 = endPoint;

		// find the two triangles in question (on either side)
		Tri* associatedLeftTri  = 0;
		Tri* associatedRightTri = 0;
		Tri* leftTriNeighbour = 0;
		Tri* rightTriNeighbour = 0;
		ushort leftTri_p0,  leftTri_p1,  leftTri_p2,  left_p1_p,  left_index;
		ushort rightTri_p0, rightTri_p1, rightTri_p2, right_p1_p, right_index;
		left_index = 0;
		right_index = 0;
		for ( auto tri : triangles_p0p1 ) {
			if ( tri->p0 == endPoint ) {
				associatedLeftTri = tri;
				leftTri_p0 = tri->p0;
				leftTri_p1 = tri->p1;
				leftTri_p2 = tri->p2;
				left_p1_p  = 1;
				leftTriNeighbour = tri->neighbour_p0p1;
				break;
			} else if ( tri->p1 == endPoint ) {
				associatedLeftTri = tri;
				leftTri_p0 = tri->p1;
				leftTri_p1 = tri->p2;
				leftTri_p2 = tri->p0;
				left_p1_p  = 2;
				leftTriNeighbour = tri->neighbour_p1p2;
				break;
			} else if ( tri->p2 == endPoint ) {
				associatedLeftTri = tri;
				leftTri_p0 = tri->p2;
				leftTri_p1 = tri->p0;
				leftTri_p2 = tri->p1;
				left_p1_p  = 0;
				leftTriNeighbour = tri->neighbour_p2p0;
				break;
			}
			++left_index;
		}
		for ( auto tri : triangles_p1p0 ) {
			// WARNING WARNING WARNING WARNING WARNING WARNING WARNING
			// Remember this is the OTHER side of the edge, which means we must view this triangle in clockwise form for
			// p1
			// WARNING WARNING WARNING WARNING WARNING WARNING WARNING
			if ( tri->p0 == endPoint ) {
				associatedRightTri = tri;
				rightTri_p0 = tri->p0;
				rightTri_p1 = tri->p2;
				rightTri_p2 = tri->p1;
				right_p1_p  = 2;
				rightTriNeighbour = tri->neighbour_p2p0;
				break;
			} else if ( tri->p1 == endPoint ) {
				associatedRightTri = tri;
				rightTri_p0 = tri->p1;
				rightTri_p1 = tri->p0;
				rightTri_p2 = tri->p2;
				right_p1_p  = 0;
				rightTriNeighbour = tri->neighbour_p0p1;
				break;
			} else if ( tri->p2 == endPoint ) {
				associatedRightTri = tri;
				rightTri_p0 = tri->p2;
				rightTri_p1 = tri->p1;
				rightTri_p2 = tri->p0;
				right_p1_p  = 1;
				rightTriNeighbour = tri->neighbour_p1p2;
				break;
			}
			++right_index;
		}

		
		if ( leftTri_p1 == rightTri_p1 ) {
			edge->p1 = leftTri_p1;
			edge->triangles_p0p1.push_back( associatedLeftTri );
			edge->triangles_p1p0.push_back( associatedRightTri );
			*leftTriNeighbour = associatedRightTri;
			*rightTriNeighbour = associatedLeftTri;
			endPoint = leftTri_p1;

			// TODO: save deleted triangles (left/right)
		} else {
			// which p1 is nearest to p0
			// edge: (p1-p0)t + p0 = {left,right}_p1
			Terrain* terrain = associatedLeftTri->chunk->terrain;
			Vertex left_p1 = terrain->vertexBuffer[leftTri_p1]
			Vertex right_p1 = terrain->vertexBuffer[rightTri_p1]
			Vertex endp0 = terrain->vertexBuffer[p0];
			Vertex endp1 = terrain->vertexBuffer[p1];

			glm::vec3 direction = glm::vec3( endp1.v_x - endp0.v_x, endp1.v_y - endp0.v_y, endp1.v_z - endp0.v_z );
			glm::vec3 origin    = glm::vec3( endp0.v_x, endp0.v_y, endp0.v_z );
			glm::vec3 left_vp1  = glm::vec3( left_p1.v_x, left_p1.v_y, left_p1.v_z );
			glm::vec3 right_vp1 = glm::vec3( right_p1.v_x, right_p1.v_y, right_p1.v_z );
			float left_t  = (left_vp1 - origin).length / direction.length;
			float right_t = (right_vp1 - origin).length / direction.length;





			// TODO: subdivide other triangle (save in new triangle's list for adding to chunk; save old triangle for
			// 			deletion; add split triangle into current tri-list for next iteration search)
			if ( left_t < right_t ) { // TODO: merge w/ right/left handling
				// right triangle needs to be subdivided to meet the left triangle's point

				// TODO: create new edge
				edge->p1 = leftTri_p1;

				// TODO: create subdivided triangle for new edge
				// SUBDIVIDED TRIANGLE: resize triangle to remove its new subdivision
				if ( right_p1_p == 0 ) {
					associatedRightTri->chunk->triangleBuffer[associatedRightTri->triIndex].p0 = leftTri_p1;
					associatedRightTri->p0 = leftTri_p1;
				} else if ( right_p1_p == 1 ) {
					associatedRightTri->chunk->triangleBuffer[associatedRightTri->triIndex].p1 = leftTri_p1;
					associatedRightTri->p1 = leftTri_p1;
				} else {
					associatedRightTri->chunk->triangleBuffer[associatedRightTri->triIndex].p2 = leftTri_p1;
					associatedRightTri->p2 = leftTri_p1;
				}

				// SUBDIVIDED TRIANGLE: new triangle to fit within the subdivided edge
				associatedRightTri->chunk->triangleBuffer.push_back( Triangle( rightTri_p0, rightTri_p1, leftTri_p1 ) );
				Tri* triIn = new Tri( associatedRightTri->chunk, associatedRightTri->chunk->triangleBuffer.size() - 1 );
				edge->triangles_p1p0.push_back( triIn );

				// TODO: add & neighbour tri's to edge
				edge->triangles_p0p1.push_back( associatedLeftTri );
				triangles_p0p1.erase( triangles_p0p1.begin()+left_index );
				
				*leftTriNeighbour = triIn;
				if ( right_p1_p == 0 ) {
					rightTriNeighbour = triIn->neighbour_p0p1;
				} else if ( right_p1_p == 1 ) {
					rightTriNeighbour = triIn->neighbour_p1p2;
				} else {
					rightTriNeighbour = triIn->neighbour_p2p0;
				}
				*rightTriNeighbour = associatedLeftTri;

				// TODO: save new triangles, save deleted triangle (left), save new edge
				edgeSubdivisions.push_back( edge );

				// TODO: endpoint is leftTri_p1
				endPoint = leftTri_p1;
			} else { // TODO: merge w/ right/left handling
				// left triangle needs to be subdivided to meet the right triangle's point

				// TODO: create new edge
				edge->p1 = rightTri_p1;

				// TODO: create subdivided triangle for new edge
				// SUBDIVIDED TRIANGLE: resize triangle to remove its new subdivision
				if ( left_p1_p == 0 ) {
					associatedLeftTri->chunk->triangleBuffer[associatedLeftTri->triIndex].p0 = rightTri_p1;
					associatedLeftTri->p0 = rightTri_p1;
				} else if ( left_p1_p == 1 ) {
					associatedLeftTri->chunk->triangleBuffer[associatedLeftTri->triIndex].p1 = rightTri_p1;
					associatedLeftTri->p1 = rightTri_p1;
				} else {
					associatedLeftTri->chunk->triangleBuffer[associatedLeftTri->triIndex].p2 = rightTri_p1;
					associatedLeftTri->p2 = rightTri_p1;
				}

				// SUBDIVIDED TRIANGLE: new triangle to fit within the subdivided edge
				associatedLeftTri->chunk->triangleBuffer.push_back( Triangle( leftTri_p0, rightTri_p1, leftTri_p2 ) );
				Tri* triIn = new Tri( associatedLeftTri->chunk, associatedLeftTri->chunk->triangleBuffer.size() - 1 );
				edge->triangles_p0p1.push_back( triIn );

				// TODO: add & neighbour tri's to edge
				edge->triangles_p1p0.push_back( associatedRightTri );
				triangles_p1p0.erase( triangles_p1p0.begin()+right_index );
				
				*rightTriNeighbour = triIn;
				if ( left_p1_p == 0 ) {
					leftTriNeighbour = triIn->neighbour_p0p1;
				} else if ( left_p1_p == 1 ) {
					leftTriNeighbour = triIn->neighbour_p1p2;
				} else {
					leftTriNeighbour = triIn->neighbour_p2p0;
				}
				*leftTriNeighbour = associatedRightTri;

				// TODO: save new triangles, save deleted triangle (left), save new edge
				edgeSubdivisions.push_back( edge );

				// TODO: endpoint is leftTri_p1
				endPoint = leftTri_p1;

			}

		}


	} 

	// TODO: replace old edge with subdivided edges
	for ( auto edge : edgeSubdivisions ) {
		ushort p0 = edge->p0,
			   p1 = edge->p1;

		// find point node
		PointNode* startPoint = cachedPoint;
		PointNode* nodePoint = 0;
		while ( startPoint ) {
			if ( startPoint->p0 == p0 ) {
				nodePoint = startPoint;
				break;
			} else if ( startPoint->p0 < p0 ) {
				if ( startPoint->rightPoint && startPoint->rightPoint->p0 > p0 ) {
					// node point between these nodes
					nodePoint = new PointNode();
					nodePoint->rightPoint = startPoint->rightPoint;
					nodePoint->rightPoint->leftPoint = nodePoint;
					nodePoint->leftPoint = startPoint;
					startPoint->rightPoint = nodePoint;
					nodePoint->p0 = p0;
					break;
				}

				if ( startPoint->rightPoint ) {
					startPoint = startPoint->rightPoint;
				} else {
					// node point on the end
					nodePoint = new PointNode();
					nodePoint->leftPoint = startPoint;
					startPoint->rightPoint = nodePoint;
					nodePoint->p0 = p0;
					break;
				}
			} else {
				if ( startPoint->leftPoint && startPoint->leftPoint->p0 < p0 ) {
					// node point between these nodes
					nodePoint = new PointNode();
					nodePoint->leftPoint = startPoint->leftPoint;
					nodePoint->leftPoint->rightPoint = nodePoint;
					nodePoint->rightPoint = startPoint;
					startPoint->leftPoint = nodePoint;
					nodePoint->p0 = p0;
					break;
				}

				if ( startPoint->leftPoint ) {
					startPoint = startPoint->leftPoint;
				} else {
					// node point on the end
					nodePoint = new PointNode();
					nodePoint->rightPoint = startPoint;
					startPoint->leftPoint = nodePoint;
					nodePoint->p0 = p0;
					break;
				}
			}
		}

		// empty list
		if ( !nodePoint ) {
			nodePoint = new PointNode();
			nodePoint->p0 = p0;
			cachedPoint = nodePoint;
		}
		cachedPoint = nodePoint;


		// find edge adjacent to this point
		EdgeTriNode* startEdge = nodePoint->edgeNode;
		while ( startEdge ) {
			if ( startEdge->p1 == p1 ) {
				assert(false); // this edge should not yet exist??
				break;
			} else if ( startEdge->p1 < p1 ) {
				if ( startEdge->rightNode && startEdge->rightNode->p1 > p1 ) {
					// node edge between these nodes
					edge->rightNode = startEdge->rightNode;
					edge->rightNode->leftNode = edge;
					edge->leftNode = startEdge;
					startEdge->rightNode = edge;
					break;
				}

				if ( startEdge->rightNode ) {
					startEdge = startEdge->rightNode;
				} else {
					// node edge on the end
					edge->leftNode = startEdge;
					startEdge->rightNode = edge;
					break;
				}
			} else {
				if ( startEdge->leftNode && startEdge->leftNode->p1 < p1 ) {
					// node edge between these nodes
					edge->leftNode = startEdge->leftNode;
					edge->leftNode->rightNode = edge;
					edge->rightNode = startEdge;
					startEdge->leftNode = edge;
					break;
				}

				if ( startEdge->leftNode ) {
					startEdge = startEdge->leftNode;
				} else {
					// node edge on the end
					edge->rightNode = startEdge;
					startEdge->leftNode = edge;
					break;
				}

			}
		}

		// empty list
		if ( !edge ) {
			nodePoint->edgeNode = edge;
		}

	}

}
// ============================================== //


// ============================================== //
Chunk::AddTriangleResults Chunk::addTriangle(Voxel* p0, Voxel* p1, Voxel* p2) {

	/* Attempt to add given triangle to this chunk
	 * 
	 * If the given triangle cannot be added to this chunk (it doesn't fit) then the
	 * point(s) which don't fit within the chunk are projected onto the chunk and returned
	 * in the AddTriangleResults; along with the details behind what sort of triangle
	 * subdivision is necessary to fit it within this chunk. Other projections may be
	 * necessary, like finding points on the edge of the chunk which lay on the triangle;
	 * or the midpoint between projections.
	 **/

	AddTriangleResults results;

	// does this triangle already exist?
	uint index = 0;
	Triangle newTriangle(p0->vertexIndex, p1->vertexIndex, p2->vertexIndex);
	for ( auto existingTriangle : triangleBuffer ) {
		if ( existingTriangle == newTriangle ) {
			results.addedTriangle = index;
			results.addResults = AddTriangleResults::TRIANGLE_ADD_SUCCEEDED;
			return results; // triangle has already been added
		}
		++index;
	}

	/* TODO: remove this
	// have these voxels already been added?
	int _p0=-1, _p1=-1, _p2=-1;
	uint found = 0, index = 0;
	for ( auto voxel : voxelBuffer ) {
		if ( voxel == p0 )      { _p0 = voxel->vertexIndex; ++found; }
		else if ( voxel == p1 ) { _p1 = voxel->vertexIndex; ++found; }
		else if ( voxel == p2 ) { _p2 = voxel->vertexIndex; ++found; }
		if ( found == 3 ) break;
		++index;
	}

	// does the triangle already exist ??
	if ( found == 3 ) {
		index = 0;
		Triangle newTriangle(_p0,_p1,_p2);
		for ( auto existingTriangle : triangleBuffer ) {
			if ( existingTriangle == newTriangle ) {
				results.addedTriangle = index;
				results.addResults = AddTriangleResults::TRIANGLE_ADD_SUCCEEDED;
				return results; // triangle has already been added
			}
			++index;
		}
	}
	*/

	
	// are any of the voxels outside of the chunk?
	bool p1_isOutside = isOutsideChunk(p1);
	bool p2_isOutside = isOutsideChunk(p2);
	if ( p1_isOutside || p2_isOutside ) {

		// vertex points of p0, p1, p2
		Vertex p0end = terrain->vertexBuffer[p0->vertexIndex];
		Vertex p1end = terrain->vertexBuffer[p1->vertexIndex];
		Vertex p2end = terrain->vertexBuffer[p2->vertexIndex];

		// BOTH points outside the chunk
		if ( p1_isOutside && p2_isOutside ) {

			// project p1 along the p0-p1 edge, onto the chunk
			results.projected_p1 = new Voxel();
			uchar facep1;
			Vertex* projectedVertex = getVoxelProjection( p1, p0, &facep1 );
			terrain->vertexBuffer.push_back( Vertex(projectedVertex->v_x, projectedVertex->v_y, projectedVertex->v_z) );
			results.projected_p1->vertexIndex = terrain->vertexBuffer.size() - 1;
			results.projected_p1->terrain = terrain;

			// project p2 along the p0-p2 edge, onto the chunk
			results.projected_p2 = new Voxel();
			uchar facep2;
			projectedVertex = getVoxelProjection( p2, p0, &facep2 );
			terrain->vertexBuffer.push_back( Vertex(projectedVertex->v_x, projectedVertex->v_y, projectedVertex->v_z) );
			results.projected_p2->vertexIndex = terrain->vertexBuffer.size() - 1;
			results.projected_p2->terrain = terrain;

			// both points project onto the same face
			if ( facep1 == facep2 ) {

				// find midpoint between outside points
				Vertex* midpoint = new Vertex();
				midpoint->v_x = (p1end.v_x - p2end.v_x)/2 + p2end.v_x;
				midpoint->v_y = (p1end.v_y - p2end.v_y)/2 + p2end.v_y;
				midpoint->v_z = (p1end.v_z - p2end.v_z)/2 + p2end.v_z;
				results.projected_midpoint = new Voxel();
				terrain->vertexBuffer.push_back( Vertex(midpoint->v_x, midpoint->v_y, midpoint->v_z) );
				results.projected_midpoint->vertexIndex = terrain->vertexBuffer.size() - 1;
				results.projected_midpoint->terrain = terrain;

				results.addResults = AddTriangleResults::TRIANGLE_ADD_TWOPOINT_ONESIDE;
				Log("NEED TO SUBDIVIDE TRIANGLE: two points / one side");
				return results;

			}

			// does p1-p2 edge pass through chunk?
			uchar facep1p2, facep2p1;
			Vertex* projectedVertex_p1p2 = getVoxelProjection( p1, p2, &facep1p2 );
			Vertex* projectedVertex_p2p1 = getVoxelProjection( p2, p1, &facep2p1 );
			if ( projectedVertex_p1p2 && projectedVertex_p2p1 ) {

				// p1-p2 edge passes through chunk
				
				// find point along edge of chunk of projected-p1 face
				results.projected_p1Mid = new Voxel();
				terrain->vertexBuffer.push_back( Vertex(projectedVertex_p1p2->v_x, projectedVertex_p1p2->v_y, projectedVertex_p1p2->v_z) );
				results.projected_p1Mid->vertexIndex = terrain->vertexBuffer.size() - 1;
				results.projected_p1Mid->terrain = terrain;

				// find point along edge of chunk of projected-p2 face
				results.projected_p2Mid = new Voxel();
				terrain->vertexBuffer.push_back( Vertex(projectedVertex_p2p1->v_x, projectedVertex_p2p1->v_y, projectedVertex_p2p1->v_z) );
				results.projected_p2Mid->vertexIndex = terrain->vertexBuffer.size() - 1;
				results.projected_p2Mid->terrain = terrain;

				// TODO: necessary to include _THREESIDE result?
				Log("NEED TO SUBDIVIDE TRIANGLE: four points / two sides...three sides?");
				results.addResults = AddTriangleResults::TRIANGLE_ADD_FOURPOINT_TWOSIDE;
				return results;

			}

			// TODO: we're forcing TWOPOINT_TWOSIDE even if its not the case; which is
			// better, p1mid or p2mid?
			Vertex* p1mid = getSeamIntersectionPoint( &p0end, &p1end, &p2end, facep1 );
			// Vertex* p2mid = getSeamIntersectionPoint( &p0end, &p1end, &p2end, facep2 );

			// find shared midpoint between projected-p1 and projected-p2 along edge of
			// chunk (along faces)
			results.projected_p1p2 = new Voxel();
			terrain->vertexBuffer.push_back( Vertex(p1mid->v_x, p1mid->v_y, p1mid->v_z) );
			results.projected_p1p2->vertexIndex = terrain->vertexBuffer.size() - 1;
			results.projected_p1p2->terrain = terrain;

			results.addResults = AddTriangleResults::TRIANGLE_ADD_TWOPOINT_TWOSIDE;
			Log("NEED TO SUBDIVIDE TRIANGLE: two points / two sides");
			return results;

		} else {

			Voxel* outsideVoxel   = (p1_isOutside?p1:p2);
			Voxel* neighbourVoxel = (p1_isOutside?p2:p1);

			// project outside voxel along p0-pX edge, onto chunk
			Voxel* projection = new Voxel();
			uchar facep0;
			Vertex* projectedVertex = getVoxelProjection( outsideVoxel, p0, &facep0 );
			terrain->vertexBuffer.push_back( Vertex(projectedVertex->v_x, projectedVertex->v_y, projectedVertex->v_z) );
			projection->vertexIndex = terrain->vertexBuffer.size() - 1;
			projection->terrain = terrain;
			if ( p1_isOutside ) results.projected_p1 = projection;
			else results.projected_p2 = projection;

			// project outside voxel along p1-p2 edge, onto chunk
			results.projected_p1p2 = new Voxel();
			uchar facep2;
			projectedVertex = getVoxelProjection( outsideVoxel, neighbourVoxel, &facep2 );
			terrain->vertexBuffer.push_back( Vertex(projectedVertex->v_x, projectedVertex->v_y, projectedVertex->v_z) );
			results.projected_p1p2->vertexIndex = terrain->vertexBuffer.size() - 1;
			results.projected_p1p2->terrain = terrain;


			// both projections lay on the same face
			if ( facep0 == facep2 ) {
				results.addResults = AddTriangleResults::TRIANGLE_ADD_ONEPOINT_ONESIDE;
				Log("NEED TO SUBDIVIDE TRIANGLE: one point / one side");
				return results;
			}

			// find midpoint along seam of chunk between both projections
			Vertex* midpoint = getSeamIntersectionPoint( &p0end, &p1end, &p2end, facep0, facep2 );
			results.projected_midpoint = new Voxel();
			terrain->vertexBuffer.push_back( Vertex(midpoint->v_x, midpoint->v_y, midpoint->v_z) );
			results.projected_midpoint->vertexIndex = terrain->vertexBuffer.size() - 1;
			results.projected_midpoint->terrain = terrain;
			
			results.addResults = AddTriangleResults::TRIANGLE_ADD_ONEPOINT_TWOSIDE;
			return results;


		}
	}


	// Voxels inside of chunk, add triangle normally

	/* TODO: remove this
	// get the indices
	if ( _p0==-1 ) {
		voxelBuffer.push_back(p0);
		_p0 = p0->vertexIndex;

	}
	if ( _p1==-1 ) {
		voxelBuffer.push_back(p1);
		_p1 = p1->vertexIndex;
	}
	if ( _p2==-1 ) {
		voxelBuffer.push_back(p2);
		_p2 = p2->vertexIndex;
	}
	*/

	// add triangle
	// TODO: remove this
	// triangleBuffer.push_back(Triangle(_p0,_p1,_p2));
	triangleBuffer.push_back(Triangle(p0->vertexIndex, p1->vertexIndex, p2->vertexIndex));
	results.addResults = AddTriangleResults::TRIANGLE_ADD_SUCCEEDED;
	results.addedTriangle = triangleBuffer.size() - 1;

	// Log(str(format("Added Triangle(<%1%,%2%,%3%>, <%4%,%5%,%6%>, <%7%,%8%,%9%>) Total (%10%)")%terrain->vertexBuffer[_p0].v_x%terrain->vertexBuffer[_p0].v_y%terrain->vertexBuffer[_p0].v_z%terrain->vertexBuffer[_p1].v_x%terrain->vertexBuffer[_p1].v_y%terrain->vertexBuffer[_p1].v_z%terrain->vertexBuffer[_p2].v_x%terrain->vertexBuffer[_p2].v_y%terrain->vertexBuffer[_p2].v_z%triangleBuffer.size()));
	
	return results;
}
// ============================================== //


// ============================================== //
bool Chunk::isOutsideChunk(Voxel* p) {

	// is this voxel outside the chunk?
	Vertex v = terrain->vertexBuffer[p->vertexIndex];
	return ( v.v_x < worldOffset.x || v.v_x > worldOffset.x + chunkSize.x ||
			 v.v_y < worldOffset.y || v.v_y > worldOffset.y + chunkSize.y ||
			 v.v_z < worldOffset.z || v.v_z > worldOffset.z + chunkSize.z );
}
// ============================================== //


// ============================================== //
Vertex* Chunk::getVoxelProjection(Voxel* voxel, Voxel* neighbour, uchar* face) {

	// attempt to intersect the voxel-neighbour edge on each face of the chunk where the
	// voxel is on the other side; return the projection of the first successful result
	Vertex* v_voxel     = new Vertex();
	v_voxel->v_x = terrain->vertexBuffer[voxel->vertexIndex].v_x;
	v_voxel->v_y = terrain->vertexBuffer[voxel->vertexIndex].v_y;
	v_voxel->v_z = terrain->vertexBuffer[voxel->vertexIndex].v_z;

	Vertex* v_neighbour = new Vertex();
	v_neighbour->v_x = terrain->vertexBuffer[neighbour->vertexIndex].v_x;
	v_neighbour->v_y = terrain->vertexBuffer[neighbour->vertexIndex].v_y;
	v_neighbour->v_z = terrain->vertexBuffer[neighbour->vertexIndex].v_z;

	Vertex* projectedVertex = 0;
	if        ( v_voxel->v_x < worldOffset.x && (projectedVertex = projectVertexOntoFace(v_voxel, v_neighbour, FACE_LEFT)) ) {
		*face = FACE_LEFT; return projectedVertex;
	} else if ( v_voxel->v_y < worldOffset.y && (projectedVertex = projectVertexOntoFace(v_voxel, v_neighbour, FACE_BOTTOM)) ) {
		*face = FACE_BOTTOM; return projectedVertex;
	} else if ( v_voxel->v_z < worldOffset.z && (projectedVertex = projectVertexOntoFace(v_voxel, v_neighbour, FACE_BACK)) ) {
		*face = FACE_BACK; return projectedVertex;
	} else if ( v_voxel->v_x > worldOffset.x + chunkSize.x && (projectedVertex = projectVertexOntoFace(v_voxel, v_neighbour, FACE_RIGHT)) ) {
		*face = FACE_RIGHT; return projectedVertex;
	} else if ( v_voxel->v_y > worldOffset.y + chunkSize.y && (projectedVertex = projectVertexOntoFace(v_voxel, v_neighbour, FACE_TOP)) ) {
		*face = FACE_TOP; return projectedVertex;
	} else if ( v_voxel->v_z > worldOffset.z + chunkSize.z && (projectedVertex = projectVertexOntoFace(v_voxel, v_neighbour, FACE_FRONT)) ) {
		*face = FACE_FRONT; return projectedVertex;
	}
	return 0;

}
// ============================================== //


// ============================================== //
Vertex* Chunk::projectVertexOntoFace(Vertex* voxel, Vertex* neighbour, uchar face) {

	// attempt to project voxel-neighbour edge onto the given face, return the results (0
	// if none)

	// face: ax + by + cz = d
	// edge (parametric): x = et + f,  y = gt + h,  z = it + j
	// intersection: a(et + f) + b(gt + h) + c(it + j) = d
	// 				 t(ae + bg + ci) = d - af - bh - cj
	float f = voxel->v_x, e = neighbour->v_x - f,
		  h = voxel->v_y, g = neighbour->v_y - h,
		  j = voxel->v_z, i = neighbour->v_z - j;
	float a=0, ae=0,
		  b=0, be=0,
		  c=0, ce=0,
		  d=0;
	switch ( face ) {
		case FACE_FRONT:
		case FACE_BACK:
			c = 1;
			if ( face == FACE_FRONT ) ce = 1;
			break;
		case FACE_LEFT:
		case FACE_RIGHT:
			a = 1;
			if ( face == FACE_RIGHT ) ae = 1;
			break;
		case FACE_TOP:
		case FACE_BOTTOM:
			b = 1;
			if ( face == FACE_TOP ) be = 1;
			break;
	}


	// projection
	d = a*worldOffset.x + ae*chunkSize.x + b*worldOffset.y + be*chunkSize.y + c*worldOffset.z + ce*chunkSize.z;
	float t = (d -a*f - b*h - c*j) / (a*e + b*g + c*i);
	float x = e*t + f,
		  y = g*t + h,
		  z = i*t + j;

	// is this within the bounds of the face?
	if ( fabs(t) <= 1.0f &&
		 ( x >= worldOffset.x && x <= worldOffset.x + chunkSize.x ) &&
		 ( y >= worldOffset.y && y <= worldOffset.y + chunkSize.y ) &&
		 ( z >= worldOffset.z && z <= worldOffset.z + chunkSize.z ) ) {
		return new Vertex(x,y,z);
	}

	return 0;

}
// ============================================== //


// ============================================== //
Vertex* Chunk::getSeamIntersectionPoint(Vertex* p0, Vertex* p1, Vertex* p2, uchar face) {

	// plane: ax + by + cz = d
	// edge (parametric): x = et + f,  y = gt + h,  z = it + j
	// intersection: a(et + f) + b(gt + h) + c(it + j) = d
	// 				 t(ae + bg + ci) = d - af - bh - cj
	float e, f, g, h, i, j; // edge parameters

	Vertex* intersection = 0;
	// go through each edge, check if this face has that edge
	if        ( face == FACE_TOP || face == FACE_LEFT ) { // topleft
		e = 0;  f = worldOffset.x;
		g = 0;  h = worldOffset.y + chunkSize.y;
		i = 1;  j = worldOffset.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} 
	if ( face == FACE_TOP || face == FACE_RIGHT ) { // topright
		e = 0;  f = worldOffset.x + chunkSize.x;
		g = 0;  h = worldOffset.y + chunkSize.y;
		i = 1;  j = worldOffset.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} 
	if ( face == FACE_BOTTOM || face == FACE_RIGHT ) { // bottomright
		e = 0;  f = worldOffset.x + chunkSize.x;
		g = 0;  h = worldOffset.y;
		i = 1;  j = worldOffset.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} 
	if ( face == FACE_BOTTOM || face == FACE_LEFT ) { // bottomleft
		e = 0;  f = worldOffset.x;
		g = 0;  h = worldOffset.y;
		i = 1;  j = worldOffset.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} 
	if ( face == FACE_TOP || face == FACE_FRONT ) { // topfront
		e = 1;  f = worldOffset.x;
		g = 0;  h = worldOffset.y + chunkSize.y;
		i = 0;  j = worldOffset.z + chunkSize.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} 
	if ( face == FACE_TOP || face == FACE_BACK ) { // topback
		e = 1;  f = worldOffset.x;
		g = 0;  h = worldOffset.y + chunkSize.y;
		i = 0;  j = worldOffset.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} 
	if ( face == FACE_BOTTOM || face == FACE_FRONT ) { // bottomfront
		e = 1;  f = worldOffset.x;
		g = 0;  h = worldOffset.y;
		i = 0;  j = worldOffset.z + chunkSize.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} 
	if ( face == FACE_BOTTOM || face == FACE_BACK ) { // bottomback
		e = 1;  f = worldOffset.x;
		g = 0;  h = worldOffset.y;
		i = 0;  j = worldOffset.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} 
	if ( face == FACE_FRONT || face == FACE_LEFT ) { // frontleft
		e = 0;  f = worldOffset.x;
		g = 1;  h = worldOffset.y;
		i = 0;  j = worldOffset.z + chunkSize.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} 
	if ( face == FACE_FRONT || face == FACE_RIGHT ) { // frontright
		e = 0;  f = worldOffset.x + chunkSize.x;
		g = 1;  h = worldOffset.y;
		i = 0;  j = worldOffset.z + chunkSize.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} 
	if ( face == FACE_BACK || face == FACE_LEFT ) { // backleft
		e = 0;  f = worldOffset.x;
		g = 1;  h = worldOffset.y;
		i = 0;  j = worldOffset.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} 
	if ( face == FACE_BACK || face == FACE_RIGHT ) { // backright
		e = 0;  f = worldOffset.x + chunkSize.x;
		g = 1;  h = worldOffset.y;
		i = 0;  j = worldOffset.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	}

	return 0;

}
// ============================================== //


// ============================================== //
bool Chunk::facesMatch(uchar expectedFace1, uchar expectedFace2, uchar face1, uchar face2) {
	return ( ( face1 == expectedFace1 && face2 == expectedFace2 ) ||
			 ( face2 == expectedFace1 && face1 == expectedFace2 ) );
}
// ============================================== //


// ============================================== //
Vertex* Chunk::getSeamIntersectionPoint(Vertex* p0, Vertex* p1, Vertex* p2, uchar face1, uchar face2) {

	float e, f, g, h, i, j; // edge parameters

	Vertex* intersection = 0;
	// go through each edge, check if the seam between the two faces contains it
	if        ( facesMatch( FACE_TOP, FACE_LEFT, face1, face2 ) ) { // topleft
		e = 0;  f = worldOffset.x;
		g = 0;  h = worldOffset.y + chunkSize.y;
		i = 1;  j = worldOffset.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} if ( facesMatch( FACE_TOP, FACE_RIGHT, face1, face2 ) ) { // topright
		e = 0;  f = worldOffset.x + chunkSize.x;
		g = 0;  h = worldOffset.y + chunkSize.y;
		i = 1;  j = worldOffset.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} if ( facesMatch( FACE_BOTTOM, FACE_RIGHT, face1, face2 ) ) { // bottomright
		e = 0;  f = worldOffset.x + chunkSize.x;
		g = 0;  h = worldOffset.y;
		i = 1;  j = worldOffset.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} if ( facesMatch( FACE_BOTTOM, FACE_LEFT, face1, face2 ) ) { // bottomleft
		e = 0;  f = worldOffset.x;
		g = 0;  h = worldOffset.y;
		i = 1;  j = worldOffset.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} if ( facesMatch( FACE_TOP, FACE_FRONT, face1, face2 ) ) { // topfront
		e = 1;  f = worldOffset.x;
		g = 0;  h = worldOffset.y + chunkSize.y;
		i = 0;  j = worldOffset.z + chunkSize.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} if ( facesMatch( FACE_TOP, FACE_BACK, face1, face2 ) ) { // topback
		e = 1;  f = worldOffset.x;
		g = 0;  h = worldOffset.y + chunkSize.y;
		i = 0;  j = worldOffset.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} if ( facesMatch( FACE_BOTTOM, FACE_FRONT, face1, face2 ) ) { // bottomfront
		e = 1;  f = worldOffset.x;
		g = 0;  h = worldOffset.y;
		i = 0;  j = worldOffset.z + chunkSize.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} if ( facesMatch( FACE_BOTTOM, FACE_BACK, face1, face2 ) ) { // bottomback
		e = 1;  f = worldOffset.x;
		g = 0;  h = worldOffset.y;
		i = 0;  j = worldOffset.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} if ( facesMatch( FACE_FRONT, FACE_LEFT, face1, face2 ) ) { // frontleft
		e = 0;  f = worldOffset.x;
		g = 1;  h = worldOffset.y;
		i = 0;  j = worldOffset.z + chunkSize.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} if ( facesMatch( FACE_FRONT, FACE_RIGHT, face1, face2 ) ) { // frontright
		e = 0;  f = worldOffset.x + chunkSize.x;
		g = 1;  h = worldOffset.y;
		i = 0;  j = worldOffset.z + chunkSize.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} if ( facesMatch( FACE_BACK, FACE_LEFT, face1, face2 ) ) { // backleft
		e = 0;  f = worldOffset.x;
		g = 1;  h = worldOffset.y;
		i = 0;  j = worldOffset.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	} if ( facesMatch( FACE_BACK, FACE_RIGHT, face1, face2 ) ) { // backright
		e = 0;  f = worldOffset.x + chunkSize.x;
		g = 1;  h = worldOffset.y;
		i = 0;  j = worldOffset.z;
		intersection = lineIntersectTriangle( f, h, j, e, g, i, p0, p1, p2 );
		if ( intersection ) return intersection;
	}

	return 0;

}
// ============================================== //


// ============================================== //
Vertex* Chunk::lineIntersectTriangle(float line_x, float line_y, float line_z, float line_dx, float line_dy, float line_dz, Vertex* p0, Vertex* p1, Vertex* p2) {


	glm::vec3 lineO = glm::vec3( line_x, line_y, line_z );
	glm::vec3 lineD = glm::vec3( line_dx, line_dy, line_dz );
	glm::vec3 v0 = glm::vec3( p0->v_x, p0->v_y, p0->v_z );
	glm::vec3 v1 = glm::vec3( p1->v_x, p1->v_y, p1->v_z );
	glm::vec3 v2 = glm::vec3( p2->v_x, p2->v_y, p2->v_z );
	glm::vec3 e1 = v1 - v0;
	glm::vec3 e2 = v2 - v0;


#define EPSILON 0.000001
	glm::vec3 P, Q, T;
	float det, inv_det, u, v;
	float t;

	//Begin calculating determinant - also used to calculate u parameter
	P = glm::cross( lineD, e2 );

	//if determinant is near zero, ray lies in plane of triangle
	det = glm::dot( e1, P );

	//NOT CULLING
	if(det > -EPSILON && det < EPSILON) return 0;
	inv_det = 1.f / det;

	//calculate distance from V0 to ray origin
	T = lineO - v0;

	//Calculate u parameter and test bound
	u = glm::dot(T, P) * inv_det;

	//The intersection lies outside of the triangle
	if(u < 0.f || u > 1.f) return 0;

	//Prepare to test v parameter
	Q = glm::cross( T, e1 );

	//Calculate V parameter and test bound
	v = glm::dot( lineD, Q ) * inv_det;

	//The intersection lies outside of the triangle
	if(v < 0.f || u + v  > 1.f) return 0;

	t = glm::dot( e2, Q ) * inv_det;
	if ( t > chunkSize.x ) return 0; // TODO: t too big?

	Vertex* hit = new Vertex();
	hit->v_x = lineO.x + line_dx*t;
	hit->v_y = lineO.y + line_dy*t;
	hit->v_z = lineO.z + line_dz*t;
	return hit;

}
// ============================================== //


// ============================================== //
void Chunk::construct() {

	/* TODO: remove this
	for ( Voxel* voxel : voxelBuffer ) {
		while ( voxel->neighbours.has((Voxel*)0) ) {
			voxel->neighbours.remove(0);
		}
	}
	*/
	
	glUseProgram(terrain->gl);
	// TODO: delete vao (in case its already setup for another program)
	
	// enable display list
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );

	// setup buffer object
	glGenBuffers( 1, &vbo );
	glBindBuffer( GL_ARRAY_BUFFER, vbo );
	glBufferData( GL_ARRAY_BUFFER, ( terrain->vertexBuffer.size() ) * sizeof(Vertex), terrain->vertexBuffer.data(), GL_STATIC_DRAW );

	GLint glVertex = glGetAttribLocation( terrain->gl, "in_Position" );
	glEnableVertexAttribArray( glVertex );

	// load data into shader
	glVertexAttribPointer( glVertex, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0 );


	// Generate a buffer for the indices
	glGenBuffers(1, &elementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangleBuffer.size() * sizeof(Triangle), triangleBuffer.data(), GL_STATIC_DRAW);

	// cleanup
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray( 0 );

}
// ============================================== //


// ============================================== //
void Chunk::render(float r, float g, float b) {

	if ( triangleBuffer.size() == 0 ) return;
	
	glUseProgram(terrain->gl);
	glBindVertexArray(vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);

	GLint glMVP = glGetUniformLocation( terrain->gl, "MVP" );

	glm::mat4 mvp = camera.perspectiveView;
	mvp = glm::transpose(mvp);
	glUniformMatrix4fv( glMVP, 1, GL_FALSE, glm::value_ptr(mvp) );

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glUniform3f( glGetUniformLocation( terrain->gl, "in_color" ), r, g, b );
	glDrawElements( GL_TRIANGLES, triangleBuffer.size()*3, GL_UNSIGNED_SHORT, (void*)0 );

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glUniform3f( glGetUniformLocation( terrain->gl, "in_color" ), 1.0f, 1.0f, 1.0f );
	glDrawElements( GL_TRIANGLES, triangleBuffer.size()*3, GL_UNSIGNED_SHORT, (void*)0 );

	glBindVertexArray(0);
}
// ============================================== //


// ============================================== //
Chunk* Terrain::createChunks(Point<int> position, Chunk* below, Chunk* behind) {

	Chunk* chunk = new Chunk(position);
	chunk->terrain = this;

	if ( below ) {
		chunk->below = below;
		chunk->below->above = chunk;
	}

	if ( behind ) {
		chunk->behind = behind;
		chunk->behind->infront = chunk;
	}

	if ( position.x + Chunk::chunkSize.x < width ) {
		position.x += Chunk::chunkSize.x;
		Chunk* belowRight  = (below  ? below->right : 0);
		Chunk* behindRight = (behind ? behind->right : 0);
		chunk->right = createChunks( position, belowRight, behindRight );
		chunk->right->left = chunk;
	}

	return chunk;
}
// ============================================== //


// ============================================== //
TerrainSelection* Terrain::terrainPick(glm::vec3 position, glm::vec3 direction) {

	// TODO: check ray against pages, then chunks, then move along line_linkedlist for
	// intersecting quad
	
	/*
	// find intersecting quad
	QuadTree<TerrainVertexBuffer>* far_bottom_left = verticesQuadTree;
	QuadTree<TerrainVertexBuffer>* bottom_left = verticesQuadTree;
	while ( far_bottom_left->forward ) {

		bottom_left = far_bottom_left;
		while ( bottom_left->forward && bottom_left->right && bottom_left->right->forward ) {

			// check this quad
			// TODO

			bottom_left = bottom_left->right;
		}
		far_bottom_left = far_bottom_left->forward;
	}
	*/

	return 0;
}
// ============================================== //

