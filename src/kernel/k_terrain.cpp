#include "kernel/k_terrain.h"

// const Point<int> Chunk::chunkSize = Point<int>(113,113,113);
// const Point<int> EdgeTriTree::EdgeChunk::chunkSize = Point<int>(113,113,113);
#define CHUNK_SIZE 100
const Point<int> Chunk::chunkSize = Point<int>(CHUNK_SIZE,CHUNK_SIZE,CHUNK_SIZE);
const Point<int> EdgeTriTree::EdgeChunk::chunkSize = Point<int>(CHUNK_SIZE,CHUNK_SIZE,CHUNK_SIZE);
// const Point<int> Chunk::chunkSize = Point<int>(100,100,100);
// const Point<int> EdgeTriTree::EdgeChunk::chunkSize = Point<int>(100,100,100);

const uint Terrain::width = 700;
const uint Terrain::depth = 700;
const uint Terrain::height = 5000;

Terrain* EdgeTriTree::terrain = 0;

// ============================================== //
Terrain::Terrain(GLuint gl) : gl(gl) {

	edgeTree = new EdgeTriTree(this);
	journal = new Journal();
	generateTerrain();
	activeJournalEntry = journal->entries.size() - 1;
	construct();
	selection = new TerrainSelection();

	return;


	EdgeTriTree::EdgeChunk* curFarBackLeft = edgeTree->headChunk;
	EdgeTriTree::EdgeChunk* curFarLeft;
	EdgeTriTree::EdgeChunk* curNode;
	while ( curFarBackLeft ) {
		curFarLeft = curFarBackLeft;
		while ( curFarLeft ) {
			curNode = curFarLeft;
			while ( curNode ) {

				// Check this Chunk; sort all of the tri's accordingly
				vector<pair<Tri*,uchar>> triangles; // <Tri, neighbour_count>
				for ( auto edge : curNode->edges ) {
					if ( edge->triangle_p0p1 ) {
						bool foundTri = false;
						for ( auto e_tri : triangles ) {
							if ( e_tri.first == edge->triangle_p0p1 ) {
								foundTri = true;
								break;
							}
						}
						if ( !foundTri ) {
							uchar neighbour_count = 0;
							if ( edge->triangle_p0p1->neighbour_p0p1 ) ++neighbour_count;
							if ( edge->triangle_p0p1->neighbour_p1p2 ) ++neighbour_count;
							if ( edge->triangle_p0p1->neighbour_p2p0 ) ++neighbour_count;
							triangles.push_back({ edge->triangle_p0p1, neighbour_count });
						}
					}
					if ( edge->triangle_p1p0 ) {
						bool foundTri = false;
						for ( auto e_tri : triangles ) {
							if ( e_tri.first == edge->triangle_p1p0 ) {
								foundTri = true;
								break;
							}
						}
						if ( !foundTri ) {
							uchar neighbour_count = 0;
							if ( edge->triangle_p1p0->neighbour_p0p1 ) ++neighbour_count;
							if ( edge->triangle_p1p0->neighbour_p1p2 ) ++neighbour_count;
							if ( edge->triangle_p1p0->neighbour_p2p0 ) ++neighbour_count;
							triangles.push_back({ edge->triangle_p1p0, neighbour_count });
						}
					}
				}

				// Add triangles to selection whose neighbour counts are below 3
				TerrainSelection::SelectionClass* triangles_n0 = new TerrainSelection::SelectionClass();
				TerrainSelection::SelectionClass* triangles_n1 = new TerrainSelection::SelectionClass();
				TerrainSelection::SelectionClass* triangles_n2 = new TerrainSelection::SelectionClass();
				TerrainSelection::SelectionClass* triangles_n3 = new TerrainSelection::SelectionClass();
				triangles_n0->class_id = TerrainSelection::SelectionClass::CLASS_HIGHLIGHT_NEIGHBOURCOUNT0;
				triangles_n1->class_id = TerrainSelection::SelectionClass::CLASS_HIGHLIGHT_NEIGHBOURCOUNT1;
				triangles_n2->class_id = TerrainSelection::SelectionClass::CLASS_HIGHLIGHT_NEIGHBOURCOUNT2;
				triangles_n3->class_id = TerrainSelection::SelectionClass::CLASS_HIGHLIGHT_NEIGHBOURCOUNT3;
				for ( auto triangle : triangles ) {
					Log(str(format("Triangle neighbourcount: [%1%]: %2%")%triangle.first%triangle.second));
					if ( triangle.second < 3 ) {
						if ( triangle.second == 0 ) triangles_n0->triangles.push_back(triangle.first->chunk->triangleBuffer[triangle.first->triIndex]);
						else if ( triangle.second == 1 ) triangles_n1->triangles.push_back( triangle.first->chunk->triangleBuffer[triangle.first->triIndex] );
						else if ( triangle.second == 2 ) triangles_n2->triangles.push_back( triangle.first->chunk->triangleBuffer[triangle.first->triIndex] );
					} else {
						triangles_n3->triangles.push_back( triangle.first->chunk->triangleBuffer[triangle.first->triIndex] );
					}
				}
				if ( !triangles_n0->triangles.empty() ) selection->selections.push_back( triangles_n0 );
				if ( !triangles_n1->triangles.empty() ) selection->selections.push_back( triangles_n1 );
				if ( !triangles_n2->triangles.empty() ) selection->selections.push_back( triangles_n2 );
				if ( !triangles_n3->triangles.empty() ) selection->selections.push_back( triangles_n3 );

				curNode = curNode->right;
			}

			curFarLeft = curFarLeft->infront;
		}
		curFarBackLeft = curFarBackLeft->above; 
	}
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

	glUseProgram(gl);
	glGenBuffers( 1, &vbo );
	glBindBuffer( GL_ARRAY_BUFFER, vbo );
	glBufferData( GL_ARRAY_BUFFER, ( vertexBuffer.size() ) * sizeof(Vertex), vertexBuffer.data(), GL_DYNAMIC_DRAW ); // TODO: dynamic drawing? multiple VBO's per LOD & page?




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
	float incAmount = 0.3f;
	while ( farBackLeft ) {
		farLeft = farBackLeft;
		while ( farLeft ) {
			curChunk = farLeft;
			while ( curChunk ) {
				// r=0; g=0; b=0; // FIXME: disable showing chunks for now
				curChunk->render(r, g, b);
				curChunk = curChunk->right;
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
			farLeft = farLeft->infront;
		}
		farBackLeft = farBackLeft->above;
	}

	// Draw selected triangles
	if ( !selection->selections.empty() ) {
		for ( auto selectionClass : selection->selections ) {

			glUseProgram(gl);
			GLuint decal_vbo, decal_ibo;
			glGenBuffers( 1, &decal_vbo );
			glBindBuffer( GL_ARRAY_BUFFER, decal_vbo );
			glBufferData( GL_ARRAY_BUFFER, ( vertexBuffer.size() ) * sizeof(Vertex), vertexBuffer.data(), GL_STATIC_DRAW ); // TODO: this is NOT okay..

			GLint glVertex   = glGetAttribLocation  ( gl, "in_Position" ) ;
			glEnableVertexAttribArray( glVertex );

			// load data into shader
			glVertexAttribPointer( glVertex, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0 );

			glGenBuffers( 1, &decal_ibo );
			glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, decal_ibo );
			glBufferData( GL_ELEMENT_ARRAY_BUFFER, selectionClass->triangles.size() * sizeof(iTriangle), selectionClass->triangles.data(), GL_STATIC_DRAW );


			GLint glMVP = glGetUniformLocation( gl, "MVP" );

			glm::mat4 mvp = camera.perspectiveView;
			mvp = glm::transpose(mvp);
			glUniformMatrix4fv( glMVP, 1, GL_FALSE, glm::value_ptr(mvp) );
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			if ( selectionClass->class_id == TerrainSelection::SelectionClass::CLASS_HIGHLIGHT ) {
				glUniform3f( glGetUniformLocation( gl, "in_color" ), 1.0f, 0.8f, 0.0f );
			} else if ( selectionClass->class_id == TerrainSelection::SelectionClass::CLASS_HIGHLIGHT_NEIGHBOUR ) {
				glUniform3f( glGetUniformLocation( gl, "in_color" ), 9.0f, 0.4f, 0.0f );
			} else if ( selectionClass->class_id == TerrainSelection::SelectionClass::CLASS_HIGHLIGHT_NEIGHBOURCOUNT0 ) {
				glUniform3f( glGetUniformLocation( gl, "in_color" ), 1.0f, 0.0f, 0.0f );
			} else if ( selectionClass->class_id == TerrainSelection::SelectionClass::CLASS_HIGHLIGHT_NEIGHBOURCOUNT1 ) {
				glUniform3f( glGetUniformLocation( gl, "in_color" ), 0.0f, 1.0f, 0.0f );
			} else if ( selectionClass->class_id == TerrainSelection::SelectionClass::CLASS_HIGHLIGHT_NEIGHBOURCOUNT2 ) {
				glUniform3f( glGetUniformLocation( gl, "in_color" ), 0.0f, 0.0f, 1.0f );
			} else if ( selectionClass->class_id == TerrainSelection::SelectionClass::CLASS_HIGHLIGHT_NEIGHBOURCOUNT3 ) {
				glUniform3f( glGetUniformLocation( gl, "in_color" ), 0.5f, 0.0f, 0.5f );
			} else {
				glUniform3f( glGetUniformLocation( gl, "in_color" ), 0.7f, 0.0f, 0.3f );
			}
			glDrawElements( GL_TRIANGLES, selectionClass->triangles.size()*3, GL_UNSIGNED_SHORT, (void*)0 );
			glDeleteBuffers( 1, &decal_vbo );
			glDeleteBuffers( 1, &decal_ibo );
		}
	}

	/*

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
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, debugTriangles.size() * sizeof(iTriangle), debugTriangles.data(), GL_STATIC_DRAW );


	GLint glMVP = glGetUniformLocation( gl, "MVP" );

	glm::mat4 mvp = -1 * camera.perspectiveView;
	mvp = glm::transpose(mvp);
	glUniformMatrix4fv( glMVP, 1, GL_FALSE, glm::value_ptr(mvp) );
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glUniform3f( glGetUniformLocation( gl, "in_color" ), 1.0f, 0.8f, 0.0f );
	glDrawElements( GL_TRIANGLES, debugTriangles.size()*3, GL_UNSIGNED_SHORT, (void*)0 );
	glDeleteBuffers( 1, &decal_vbo );
	glDeleteBuffers( 1, &decal_ibo );
	*/

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
	int  heightRange = 60,
		 heightMin   = 0;
	for ( int forwards = 0; forwards < depth; forwards += neighbourDepth ) {
		voxelStartPoint = 0;
		voxelPrev = 0;
		for ( int right = 0; right < width; right += neighbourRight ) {
			voxelCurrent = new Voxel();
			voxelBuffer.push_back( voxelCurrent );
			voxelCurrent->terrain = this;

			float v_y = 0.0f;
			int sign = (rand()%3) - 1;
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
vector<TriangleNode*> Terrain::addTriangleIntoChunk(Chunk* chunk, Voxel* p0, Voxel* p1, Voxel* p2, bool suppressFailFix) {

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
	if ( x - chunk->worldOffset.x > chunk->chunkSize.x ) return addTriangleIntoChunk(chunk->right,   p0, p1, p2, suppressFailFix);
	if ( y - chunk->worldOffset.y > chunk->chunkSize.y ) return addTriangleIntoChunk(chunk->above,   p0, p1, p2, suppressFailFix);
	if ( z - chunk->worldOffset.z > chunk->chunkSize.z ) return addTriangleIntoChunk(chunk->infront, p0, p1, p2, suppressFailFix);

	// chunk is behind?
	if ( x < chunk->worldOffset.x ) return addTriangleIntoChunk(chunk->left,   p0, p1, p2, suppressFailFix);
	if ( y < chunk->worldOffset.y ) return addTriangleIntoChunk(chunk->below,  p0, p1, p2, suppressFailFix);
	if ( z < chunk->worldOffset.z ) return addTriangleIntoChunk(chunk->behind, p0, p1, p2, suppressFailFix);


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
	Environment* env = new Environment();
	env->terrain = this;
	env->journalEntry = new JournalEntry();
	Chunk::AddTriangleResults addedResults = chunk->addTriangle(p0, p1, p2, env); // Tessellated Triangle
	if ( addedResults.addResults == Chunk::AddTriangleResults::TRIANGLE_ADD_FAILED ) {
		Log(str(format("	Failed to add Triangle: {%1%,%2%,%3%}")%p0->vertexIndex%p1->vertexIndex%p2->vertexIndex));
		/* NOTE: adding a triangle could fail initially since p0 may lay between two containers, and we may be looking
		 * at the wrong container.. consider the following case,
		 |--------|--------| p0 may lay between two containers, and the container being considered here
		 | *      |        | may be the right one, where we obviously instead want the other one.. The
		 |        *        | trick is to circle through the vertices, since one of the other vertices
		 |        |        | will be in the chunk that we actually want
		 | *      |        |
		 |--------|--------|
	
		 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		 WARNING WARNING WARNING WARNING WARNING WARNING!
TODO: BUGS
	- what if each vertex lays on the seam?
	- in the case that the other vertices are outside of the desired chunk, they will properly tessellate within their
		own chunks, however they will tessellate into the outside chunk (p0) afterwards with the projected voxels, and
		hence all the voxels will lay on the seam (hence the above bug will be extremely common)
FIXME: Bugfix ideas
	- when picking the chunk for p0, use p1 and p2 as hints on the direction in case p0 is on the seam (check for each
		side in which p0 lays on that given seam)
		
		Case 1 (p1, p2 in C): use C
		Case 2 (p0, p1 or p0, p2 in C): use C
		Case 3 (p0, p1 in C; p0, p2 in C'): add into either, let tessellation handle the rest
		 WARNING WARNING WARNING WARNING WARNING WARNING!
		 !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		*/

		if ( p0->vertexIndex == p1->vertexIndex ||
			 p0->vertexIndex == p2->vertexIndex ||
			 p1->vertexIndex == p2->vertexIndex ) {
			// intentionally blank
		} else {
			if ( !suppressFailFix ) {
				addedTriangles = addTriangleIntoChunk(chunk, p1, p2, p0, true);
			}
			if ( addedTriangles.empty() && !suppressFailFix ) {
				// TODO: I've commented this out because it causes issues...this shouldn't be a problem because if its
				// working without this then clearly this is being triggered when it shouldn't be... .empty() causing
				// issues? but there MUST be cases (although very rare) where this case is necessary
				// addedTriangles = addTriangleIntoChunk(chunk, p2, p0, p1, true);
			}
			if ( addedTriangles.empty() ) {
				Log(str(format("	Failed attempted fix..: {%1%,%2%,%3%}")%p0->vertexIndex%p1->vertexIndex%p2->vertexIndex));
			}
			/*
			   if ( p0->vertexIndex != p1->vertexIndex &&
			   p0->vertexIndex != p2->vertexIndex &&
			   p1->vertexIndex != p2->vertexIndex ) {
			   chunk->triangleBuffer.push_back(iTriangle(p0->vertexIndex, p1->vertexIndex, p2->vertexIndex));
			   ushort bad_tri = chunk->triangleBuffer.size() - 1;
			   edgeTree->addTriangle( chunk, bad_tri );
			   }
			   */
		}
		delete env;
		return addedTriangles;
	}
	if ( addedResults.addResults == Chunk::AddTriangleResults::TRIANGLE_ADD_SUCCEEDED ) {

		// succeeded in adding triangle
		TriangleNode* addedTriangle = new TriangleNode();
		addedTriangle->chunk = chunk;
		addedTriangle->triangleID = addedResults.addedTriangle;
		addedTriangles.push_back( addedTriangle );

		Log(str(format("	Add-Success <%1%,%2%,%3%> <%4%, %5%, %6%>, <%7%, %8%, %9%>") % 
					p0->getX() % p0->getY() % p0->getZ() %
					p1->getX() % p1->getY() % p1->getZ() %
					p2->getX() % p2->getY() % p2->getZ() ));
		assert( !(p0==p1 || p0==p2 || p1==p2) );
		iTriangle tr = addedTriangle->chunk->triangleBuffer[addedTriangle->triangleID];
		// TODO: triangles being formed with same vertices
		if ( tr.p0==tr.p1 || tr.p0==tr.p2 || tr.p1==tr.p2 ) {
			assert(false);
			addedTriangle->chunk->triangleBuffer.erase( addedTriangle->chunk->triangleBuffer.begin() + addedTriangle->triangleID );
		} else {
			bool edgeAddSuccess = edgeTree->addTriangle( chunk, addedResults.addedTriangle, env );
			journal->entries.push_back( env->journalEntry );
		}

	} else if ( addedResults.addResults == 
			Chunk::AddTriangleResults::TRIANGLE_ADD_NOT_NECESSARY ) {
		Log(str(format("	Not necessary to add this triangle.. Probably already added")));
	} else if ( addedResults.addResults == 
			Chunk::AddTriangleResults::TRIANGLE_ADD_FAILED_BADTRI ) {
		Log(str(format("	BAD TRIANGLE!!! Not a real tri? (shared verts)")));
	} else if ( addedResults.addResults ==
			Chunk::AddTriangleResults::TRIANGLE_ADD_TWOPOINT_ONESIDE_NOMID ) {

		Log(str(format("	Add-Twopoint Oneside (nomid) <%1%,%2%,%3%> <%4%, %5%, %6%>, <%7%, %8%, %9%>  { p1' <%10%, %11%, %12%>, p2' <%13%, %14%, %15%> ") % 
					p0->getX() % p0->getY() % p0->getZ() %
					p1->getX() % p1->getY() % p1->getZ() %
					p2->getX() % p2->getY() % p2->getZ() %
					addedResults.projected_p1->getX() % addedResults.projected_p1->getY() % addedResults.projected_p1->getZ() %
					addedResults.projected_p2->getX() % addedResults.projected_p2->getY() % addedResults.projected_p2->getZ() ));
		// 2 outside points projected onto 1 face
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p1, addedResults.projected_p2, suppressFailFix ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p1, p2, addedResults.projected_p1, suppressFailFix ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p2, addedResults.projected_p2, addedResults.projected_p1, suppressFailFix ));

	} else if ( addedResults.addResults ==
			Chunk::AddTriangleResults::TRIANGLE_ADD_TWOPOINT_ONESIDE ) {

		Log(str(format("	Add-Twopoint Oneside <%1%,%2%,%3%> <%4%, %5%, %6%>, <%7%, %8%, %9%>  { p1' <%10%, %11%, %12%>, p2' <%13%, %14%, %15%>, mid <%16%, %17%, %18%> ") % 
					p0->getX() % p0->getY() % p0->getZ() %
					p1->getX() % p1->getY() % p1->getZ() %
					p2->getX() % p2->getY() % p2->getZ() %
					addedResults.projected_p1->getX() % addedResults.projected_p1->getY() % addedResults.projected_p1->getZ() %
					addedResults.projected_p2->getX() % addedResults.projected_p2->getY() % addedResults.projected_p2->getZ() %
					addedResults.projected_midpoint->getX() % addedResults.projected_midpoint->getY() % addedResults.projected_midpoint->getZ()));
		// 2 outside points projected onto 1 face
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p1, addedResults.projected_p2, suppressFailFix ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p1, addedResults.projected_midpoint, addedResults.projected_p1, suppressFailFix ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, addedResults.projected_midpoint, addedResults.projected_p2, addedResults.projected_p1, suppressFailFix ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p2, addedResults.projected_p2, addedResults.projected_midpoint, suppressFailFix ));

	} else if ( addedResults.addResults ==
			Chunk::AddTriangleResults::TRIANGLE_ADD_TWOPOINT_TWOSIDE ) {

		Log(str(format("	Add-Twopoint Twoside <%1%,%2%,%3%> <%4%, %5%, %6%>, <%7%, %8%, %9%>  { p1' <%10%, %11%, %12%>, p2' <%13%, %14%, %15%>, p1p2 <%16%, %17%, %18%> ") % 
					p0->getX() % p0->getY() % p0->getZ() %
					p1->getX() % p1->getY() % p1->getZ() %
					p2->getX() % p2->getY() % p2->getZ() %
					addedResults.projected_p1->getX() % addedResults.projected_p1->getY() % addedResults.projected_p1->getZ() %
					addedResults.projected_p2->getX() % addedResults.projected_p2->getY() % addedResults.projected_p2->getZ() %
					addedResults.projected_p1p2->getX()% addedResults.projected_p1p2->getY()% addedResults.projected_p1p2->getZ()));
		// 2 outside points projected onto 2 faces
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p1, addedResults.projected_p2, suppressFailFix ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, addedResults.projected_p1, addedResults.projected_p1p2, addedResults.projected_p2, suppressFailFix ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p1, addedResults.projected_p1p2, addedResults.projected_p1, suppressFailFix ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p1, p2, addedResults.projected_p1p2,
					suppressFailFix ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p2, addedResults.projected_p2, addedResults.projected_p1p2, suppressFailFix ));

	} else if ( addedResults.addResults ==
			Chunk::AddTriangleResults::TRIANGLE_ADD_ONEPOINT_ONESIDE ) {

		Log(str(format("	Add-Onepoint Oneside <%1%,%2%,%3%> <%4%, %5%, %6%>, <%7%, %8%, %9%>  { %10% <%11%, %12%, %13%>, p1p2 <%14%, %15%, %16%> ") % 
					p0->getX() % p0->getY() % p0->getZ() %
					p1->getX() % p1->getY() % p1->getZ() %
					p2->getX() % p2->getY() % p2->getZ() %
					(addedResults.projected_p1? "p1'" : "p2'") %
					(addedResults.projected_p1? addedResults.projected_p1->getX() : addedResults.projected_p2->getX()) %
					(addedResults.projected_p1? addedResults.projected_p1->getY() : addedResults.projected_p2->getY()) %
					(addedResults.projected_p1? addedResults.projected_p1->getZ() : addedResults.projected_p2->getZ()) %
					addedResults.projected_p1p2->getX() % addedResults.projected_p1p2->getY() % addedResults.projected_p1p2->getZ()));
		// 1 outside point projects onto 1 face
		if ( addedResults.projected_p1 ) {
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p1, p2, suppressFailFix ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p2, addedResults.projected_p1, addedResults.projected_p1p2, suppressFailFix ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p1, addedResults.projected_p1p2, addedResults.projected_p1, suppressFailFix ));
		} else {
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, p1, addedResults.projected_p1p2, suppressFailFix ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p1p2, addedResults.projected_p2, suppressFailFix ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p2, addedResults.projected_p2, addedResults.projected_p1p2, suppressFailFix ));
		}

	} else if ( addedResults.addResults ==
			Chunk::AddTriangleResults::TRIANGLE_ADD_ONEPOINT_TWOSIDE ) {


		Log(str(format("	Add-Onepoint Twoside <%1%,%2%,%3%> <%4%, %5%, %6%>, <%7%, %8%, %9%>  { %10% <%11%, %12%, %13%>, p1p2 <%14%, %15%, %16%>, mid <%17%, %18%, %19%> ") % 
					p0->getX() % p0->getY() % p0->getZ() %
					p1->getX() % p1->getY() % p1->getZ() %
					p2->getX() % p2->getY() % p2->getZ() %
					(addedResults.projected_p1? "p1'" : "p2'") %
					(addedResults.projected_p1? addedResults.projected_p1->getX() : addedResults.projected_p2->getX()) %
					(addedResults.projected_p1? addedResults.projected_p1->getY() : addedResults.projected_p2->getY()) %
					(addedResults.projected_p1? addedResults.projected_p1->getZ() : addedResults.projected_p2->getZ()) %
					addedResults.projected_p1p2->getX() % addedResults.projected_p1p2->getY() % addedResults.projected_p1p2->getZ() %
					addedResults.projected_midpoint->getX() % addedResults.projected_midpoint->getY() % addedResults.projected_midpoint->getZ()));
		// 1 outside point projects onto 2 faces
		if ( addedResults.projected_p1 ) {
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p1, p2, suppressFailFix ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p2, addedResults.projected_p1, addedResults.projected_p1p2, suppressFailFix ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, addedResults.projected_p1p2, addedResults.projected_p1, addedResults.projected_midpoint, suppressFailFix ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p1, addedResults.projected_midpoint, addedResults.projected_p1, suppressFailFix ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p1, addedResults.projected_p1p2, addedResults.projected_midpoint, suppressFailFix ));
		} else {
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, p1, addedResults.projected_p1p2, suppressFailFix ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p1p2, addedResults.projected_p2, suppressFailFix ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, addedResults.projected_p2, addedResults.projected_p1p2, addedResults.projected_midpoint, suppressFailFix ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p2, addedResults.projected_midpoint, addedResults.projected_p1p2, suppressFailFix ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p2, addedResults.projected_p2, addedResults.projected_midpoint, suppressFailFix ));
		}

	} else if ( addedResults.addResults ==
			Chunk::AddTriangleResults::TRIANGLE_ADD_ONEPOINT_ONESIDE_ONEPROJ ) {

		Log(str(format("	Add-Onepoint Oneside / One Projection <%1%,%2%,%3%> <%4%, %5%, %6%>, <%7%, %8%, %9%>  { %10% <%11%, %12%, %13%> ") % 
					p0->getX() % p0->getY() % p0->getZ() %
					p1->getX() % p1->getY() % p1->getZ() %
					p2->getX() % p2->getY() % p2->getZ() %
					(addedResults.projected_p1? "p1'" : (addedResults.projected_p2? "p2'" : "p1p2") ) %
					(addedResults.projected_p1? addedResults.projected_p1->getX() :
					 (addedResults.projected_p2?addedResults.projected_p2->getX() :
					  addedResults.projected_p1p2->getX())) %
					(addedResults.projected_p1? addedResults.projected_p1->getY() :
					 (addedResults.projected_p2?addedResults.projected_p2->getY() :
					  addedResults.projected_p1p2->getY())) %
					(addedResults.projected_p1? addedResults.projected_p1->getZ() :
					 (addedResults.projected_p2?addedResults.projected_p2->getZ() :
					  addedResults.projected_p1p2->getZ())) ));
		if ( addedResults.projected_p1 ) {

			Log("	(p1' tri)");
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p1, p2, suppressFailFix ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p1, p2, addedResults.projected_p1, suppressFailFix ));
		} else if ( addedResults.projected_p2 ) {

			Log("	(p2' tri)");
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, p1, addedResults.projected_p2, suppressFailFix ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p2, addedResults.projected_p2, p1, suppressFailFix ));
		} else {

			Log("	(p1p2 tri)");
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p2, p0, addedResults.projected_p1p2, suppressFailFix ));
			addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p1, addedResults.projected_p1p2, p0, suppressFailFix ));
		}

	} else if ( addedResults.addResults ==
			Chunk::AddTriangleResults::TRIANGLE_ADD_FOURPOINT_TWOSIDE ) {


		Log(str(format("	Add-Fourpoint Twoside <%1%,%2%,%3%> <%4%, %5%, %6%>, <%7%, %8%, %9%>  { p1' <%10%, %11%, %12%>, p1mid <%13%, %14%, %15%>, p2' <%16%, %17%, %18%>, p2mid <%19%, %20%, %21%> ") % 
					p0->getX() % p0->getY() % p0->getZ() %
					p1->getX() % p1->getY() % p1->getZ() %
					p2->getX() % p2->getY() % p2->getZ() %
					addedResults.projected_p1->getX() % addedResults.projected_p1->getY() % addedResults.projected_p1->getZ() % 
					addedResults.projected_p1Mid->getX() % addedResults.projected_p1Mid->getY() % addedResults.projected_p1Mid->getZ() %
					addedResults.projected_p2->getX() % addedResults.projected_p2->getY() % addedResults.projected_p2->getZ() %
					addedResults.projected_p2Mid->getX() % addedResults.projected_p2Mid->getY() % addedResults.projected_p2Mid->getZ() ));
		// 2 outside points project onto 2 faces; edge between outside points intersects chunk
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p1Mid, addedResults.projected_p2Mid, suppressFailFix ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p1, addedResults.projected_p1Mid, suppressFailFix ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p0, addedResults.projected_p2Mid, addedResults.projected_p2, suppressFailFix ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p1, addedResults.projected_p1Mid, addedResults.projected_p1, suppressFailFix ));
		addedTriangles = mergeT(addedTriangles, addTriangleIntoChunk( chunk, p2, addedResults.projected_p2, addedResults.projected_p2Mid, suppressFailFix ));

	} else if ( addedResults.addResults ==
			Chunk::AddTriangleResults::TRIANGLE_ADD_FOURPOINT_THREESIDE ) {

		// TODO
		assert(false);
	} else {
		assert(false);
	}

	p0->chunk = chunk;
	p1->chunk = chunk;
	p2->chunk = chunk;
	delete env;
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
EdgeTriTree::EdgeChunk::EdgeChunk(Point<int> position) : worldOffset(position) {

}
// ============================================== //


// ============================================== //
EdgeTriTree::EdgeTriTree(Terrain* terrain) {// : terrain(terrain) {
	this->terrain = terrain;
	EdgeTriTree::terrain = terrain;

	// Chunk Generation
	// 	create a chunk hextree along the entire map grid; even if no voxels will exist in
	// 	some of these chunks, its important to set them up (sort of like an empty space
	// 	rather than a null space).
	int x_chunks = terrain->width  / EdgeChunk::chunkSize.x + 1,
		y_chunks = terrain->height / EdgeChunk::chunkSize.y + 1,
		z_chunks = terrain->depth  / EdgeChunk::chunkSize.z + 1;


	// TODO: hextree initialization is the same here as other hextree's; abstract it
	Point<int> chunkPosition = Point<int>(0,0,0);
	EdgeChunk* chunkStartPoint = headChunk;
	EdgeChunk* chunkCurrent = headChunk;
	EdgeChunk* chunkPreviousRow = headChunk;
	EdgeChunk* chunkPreviousDepth = 0;
	for ( int y = 0; y < y_chunks; ++y ) {
		chunkStartPoint = 0;
		for ( int z = 0; z < z_chunks; ++z ) {

			chunkCurrent = createChunks( chunkPosition, chunkPreviousDepth, chunkPreviousRow ); // create all chunks along X axis
			if ( !chunkStartPoint ) chunkStartPoint = chunkCurrent; // keep the (0,0) of this Y level
			
			chunkPreviousRow = chunkCurrent;
			if ( chunkPreviousDepth ) chunkPreviousDepth = chunkPreviousDepth->infront;
			chunkPosition.z += EdgeChunk::chunkSize.z;

		}

		if ( !headChunk ) headChunk = chunkStartPoint;
		chunkPreviousDepth = chunkStartPoint;
		chunkPreviousRow   = 0;
		chunkPosition.y   += EdgeChunk::chunkSize.y;
		chunkPosition.z    = 0;
	}

}
// ============================================== //


// ============================================== //
EdgeTriTree::EdgeChunk* EdgeTriTree::createChunks(Point<int> position, EdgeChunk* below, EdgeChunk* behind) {


	EdgeChunk* chunk = new EdgeChunk(position);

	if ( below ) {
		chunk->below = below;
		chunk->below->above = chunk;
	}

	if ( behind ) {
		chunk->behind = behind;
		chunk->behind->infront = chunk;
	}

	if ( position.x + EdgeChunk::chunkSize.x < terrain->width ) {
		position.x += EdgeChunk::chunkSize.x;
		EdgeChunk* belowRight  = (below  ? below->right : 0);
		EdgeChunk* behindRight = (behind ? behind->right : 0);
		chunk->right = createChunks( position, belowRight, behindRight );
		chunk->right->left = chunk;
	}

	return chunk;
}
// ============================================== //


// ============================================== //
vector<EdgeTriTree::EdgeChunk*> EdgeTriTree::EdgeTriNode::getContainers(ushort p0, ushort p1) {

	Vertex vp0 = EdgeTriTree::terrain->vertexBuffer[p0];
	Vertex vp1 = EdgeTriTree::terrain->vertexBuffer[p1];
	Vertex direction = Vertex( vp1.v_x - vp0.v_x, vp1.v_y - vp0.v_y, vp1.v_z - vp0.v_z );
	glm::vec3 dir_vec = glm::normalize(glm::vec3( vp1.v_x - vp0.v_x, vp1.v_y - vp0.v_y, vp1.v_z - vp0.v_z ));

	// NOTE: t will be calculated as a point along the line of (v0-v1).. t MUST be between (0,1] in order to fit on the
	// line


	// find p0 chunk container
	EdgeChunk* p0Chunk = EdgeTriTree::terrain->edgeTree->headChunk;
	while ( true ) {
		if ( vp0.v_x >= p0Chunk->worldOffset.x + p0Chunk->chunkSize.x && p0Chunk->right ) { p0Chunk = p0Chunk->right; continue; }
		if ( vp0.v_y >= p0Chunk->worldOffset.y + p0Chunk->chunkSize.y && p0Chunk->above ) { p0Chunk = p0Chunk->above; continue; }
		if ( vp0.v_z >= p0Chunk->worldOffset.z + p0Chunk->chunkSize.z && p0Chunk->infront ) { p0Chunk = p0Chunk->infront; continue; }

		if ( vp0.v_x < p0Chunk->worldOffset.x && p0Chunk->left ) { p0Chunk = p0Chunk->left; continue; }
		if ( vp0.v_y < p0Chunk->worldOffset.y && p0Chunk->below ) { p0Chunk = p0Chunk->below; continue; }
		if ( vp0.v_z < p0Chunk->worldOffset.z && p0Chunk->behind ) { p0Chunk = p0Chunk->behind; continue; }

		break;
	}

	// find p1 chunk container
	EdgeChunk* p1Chunk = p0Chunk;
	char d_right = 0; // distance between p0 and p1 chunks (in terms of chunks)
	char d_up = 0;
	char d_ahead = 0;
	while ( true ) {
		if ( vp1.v_x >= p1Chunk->worldOffset.x + p1Chunk->chunkSize.x && p1Chunk->right ) { p1Chunk = p1Chunk->right; ++d_right; continue; }
		if ( vp1.v_y >= p1Chunk->worldOffset.y + p1Chunk->chunkSize.y && p1Chunk->above ) { p1Chunk = p1Chunk->above; ++d_up; continue; }
		if ( vp1.v_z >= p1Chunk->worldOffset.z + p1Chunk->chunkSize.z && p1Chunk->infront ) { p1Chunk = p1Chunk->infront; ++d_ahead; continue; }

		if ( vp1.v_x < p1Chunk->worldOffset.x && p1Chunk->left ) { p1Chunk = p1Chunk->left; --d_right; continue; }
		if ( vp1.v_y < p1Chunk->worldOffset.y && p1Chunk->below ) { p1Chunk = p1Chunk->below; --d_up; continue; }
		if ( vp1.v_z < p1Chunk->worldOffset.z && p1Chunk->behind ) { p1Chunk = p1Chunk->behind; --d_ahead; continue; }

		break;
	}


	// traverse from p0 to p1 and add all of the chunks along the path
	vector<EdgeChunk*> containers;
	containers.push_back( p0Chunk );
	if ( p0Chunk != p1Chunk ) containers.push_back( p1Chunk );
	EdgeChunk* midChunk = p0Chunk;
	while ( d_right != 0 && d_up != 0 && d_ahead != 0 ) {

		// plane: ax + by + cz = d
		// edge (parametric): x = et + f,  y = gt + h,  z = it + j
		// intersection: a(et + f) + b(gt + h) + c(it + j) = d
		// 				 t(ae + bg + ci) = d - af - bh - cj

		float t;
		Vertex hitpoint;
		// TODO: function which checks if t leads to a hitpoint on the OUTSIDE bounds of chunk
		// ... need to check t hitpoint against bounds of chunk..if it hits the outside bounds then try moving forward
		// by epsilon repeatedly until we hit a threshold t value
		auto fit_t_in_chunk = []( Vertex& v, EdgeChunk* chunk, Vertex& direction, float t ) -> float {
			return t; // TODO: FIXME
			// Check if the line [v,[v+dir]] fits within the outer bounds of this chunk. Since a chunk contains a point
			// within its [x0,x') bounds, then we are concerned with the x' value which is really just the ceiling of x'
			// minus epsilon (a point extremely close to the rounded value of x'). This function will check if the
			// hitpoint is within the bounds of the chunk, and repeatedly increase the t value until either we are
			// within the chunk or the t value is too high
			float max_t = t+0.2f; // threshold level slightly ahead of current point
			float inc_t = 0.01f;
			while ( t <= max_t ) {
				Vertex hitpoint = Vertex( v.v_x + t*direction.v_x, v.v_y + t*direction.v_y, v.v_z + t*direction.v_z );
				if ( hitpoint.v_x < chunk->worldOffset.x ||
					 hitpoint.v_y < chunk->worldOffset.y ||
					 hitpoint.v_z < chunk->worldOffset.z ) {
					// we've gone too far..clearly this chunk does not contain our vert
					return 0;
				}

				if ( hitpoint.v_x < chunk->worldOffset.x + chunk->chunkSize.x &&
					 hitpoint.v_y < chunk->worldOffset.y + chunk->chunkSize.y &&
					 hitpoint.v_z < chunk->worldOffset.z + chunk->chunkSize.z ) {
					// the t value is accepted
					return t;
				}

				// need to go further along the line to fit within the bounds
				t += inc_t;
			}

			return 0;
		};

		if ( d_up > 0 ) { // above
			// check if above chunk is hit
			t = midChunk->above->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, EdgeChunk::FACE_BOTTOM );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, midChunk->above, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && midChunk->above->pointInFace( hitpoint, EdgeChunk::FACE_BOTTOM ) ) {
				midChunk = midChunk->above;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				--d_up;
				continue;
			}
		}
		if ( d_up < 0 ) { // below
			t = midChunk->below->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, EdgeChunk::FACE_TOP );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, midChunk->below, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && midChunk->below->pointInFace( hitpoint, EdgeChunk::FACE_TOP ) ) {
				midChunk = midChunk->below;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				++d_up;
				continue;
			}
		}

		if ( d_ahead > 0 ) { // infront
			t = midChunk->infront->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, EdgeChunk::FACE_BACK );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, midChunk->infront, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && midChunk->infront->pointInFace( hitpoint, EdgeChunk::FACE_BACK ) ) {
				midChunk = midChunk->infront;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				--d_ahead;
				continue;
			}
		}
		if ( d_ahead < 0 ) { // behind
			t = midChunk->behind->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, EdgeChunk::FACE_FRONT );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, midChunk->behind, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && midChunk->behind->pointInFace( hitpoint, EdgeChunk::FACE_FRONT ) ) {
				midChunk = midChunk->behind;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				++d_ahead;
				continue;
			}
		}

		if ( d_right > 0 ) { // right
			t = midChunk->right->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, EdgeChunk::FACE_LEFT );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, midChunk->right, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && midChunk->right->pointInFace( hitpoint, EdgeChunk::FACE_LEFT ) ) {
				midChunk = midChunk->right;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				--d_right;
				continue;
			}
		}
		if ( d_right < 0 ) { // left
			t = midChunk->left->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, EdgeChunk::FACE_RIGHT );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, midChunk->left, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && midChunk->left->pointInFace( hitpoint, EdgeChunk::FACE_RIGHT ) ) {
				midChunk = midChunk->left;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				++d_right;
				continue;
			}
		}

		//
		// Edge Segments
		///////////////////
		// since a point may lay in only one chunk, and the chunks contain points which are within its [x0,x') bounds,
		// then its possible for a line to go from the current chunk to a chunk which is adjacent to that chunk and
		// connects along the chunks edge or corner. A may then have 26 neighbours which must be considered


		//
		// Top Edges (4)
		//

		if ( d_up > 0 && d_ahead > 0 ) { // up-front
			EdgeChunk* chunk_test = midChunk->above->infront;
			uchar face_test       = EdgeChunk::FACE_BOTTOM;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				--d_up;
				--d_ahead;
				continue;
			}
		}

		if ( d_up > 0 && d_right > 0 ) { // up-right
			EdgeChunk* chunk_test = midChunk->above->right;
			uchar      face_test  = EdgeChunk::FACE_BOTTOM;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				--d_up;
				--d_right;
				continue;
			}
		}


		if ( d_up > 0 && d_ahead < 0 ) { // up-back
			EdgeChunk* chunk_test = midChunk->above->behind;
			uchar face_test       = EdgeChunk::FACE_BOTTOM;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				--d_up;
				++d_ahead;
				continue;
			}
		}

		if ( d_up > 0 && d_right < 0 ) { // up-left
			EdgeChunk* chunk_test = midChunk->above->left;
			uchar      face_test  = EdgeChunk::FACE_BOTTOM;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				--d_up;
				++d_right;
				continue;
			}
		}




		//
		// Middle Edges (4)
		//


		if ( d_ahead > 0 && d_right > 0 ) { // front-right
			EdgeChunk* chunk_test = midChunk->infront->right;
			uchar      face_test  = EdgeChunk::FACE_BACK;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				--d_ahead;
				--d_right;
				continue;
			}
		}


		if ( d_ahead > 0 && d_right < 0 ) { // front-left
			EdgeChunk* chunk_test = midChunk->infront->left;
			uchar      face_test  = EdgeChunk::FACE_BACK;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				--d_ahead;
				++d_right;
				continue;
			}
		}


		if ( d_ahead < 0 && d_right > 0 ) { // back-right
			EdgeChunk* chunk_test = midChunk->behind->right;
			uchar      face_test  = EdgeChunk::FACE_FRONT;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				++d_ahead;
				--d_right;
				continue;
			}
		}


		if ( d_ahead < 0 && d_right < 0 ) { // back-left
			EdgeChunk* chunk_test = midChunk->behind->left;
			uchar      face_test  = EdgeChunk::FACE_FRONT;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				++d_ahead;
				++d_right;
				continue;
			}
		}


		//
		// Bottom Edges (4)
		//

		if ( d_up < 0 && d_ahead > 0 ) { // bottom-front
			EdgeChunk* chunk_test = midChunk->below->infront;
			uchar face_test       = EdgeChunk::FACE_TOP;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				++d_up;
				--d_ahead;
				continue;
			}
		}

		if ( d_up < 0 && d_right > 0 ) { // bottom-right
			EdgeChunk* chunk_test = midChunk->below->right;
			uchar      face_test  = EdgeChunk::FACE_TOP;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				++d_up;
				--d_right;
				continue;
			}
		}


		if ( d_up < 0 && d_ahead < 0 ) { // bottom-back
			EdgeChunk* chunk_test = midChunk->below->behind;
			uchar face_test       = EdgeChunk::FACE_TOP;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				++d_up;
				++d_ahead;
				continue;
			}
		}

		if ( d_up < 0 && d_right < 0 ) { // bottom-left
			EdgeChunk* chunk_test = midChunk->below->left;
			uchar      face_test  = EdgeChunk::FACE_TOP;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				++d_up;
				++d_right;
				continue;
			}
		}


		//
		// Corner Segments
		///////////////////



		//
		// Top Corners (4)
		//

		if ( d_up > 0 && d_ahead > 0 && d_right > 0 ) { // top-front-right
			EdgeChunk* chunk_test = midChunk->above->infront->right;
			uchar face_test       = EdgeChunk::FACE_BOTTOM;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				--d_up;
				--d_ahead;
				--d_right;
				continue;
			}
		}


		if ( d_up > 0 && d_ahead > 0 && d_right < 0 ) { // top-front-left
			EdgeChunk* chunk_test = midChunk->above->infront->left;
			uchar face_test       = EdgeChunk::FACE_BOTTOM;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				--d_up;
				--d_ahead;
				++d_right;
				continue;
			}
		}


		if ( d_up > 0 && d_ahead < 0 && d_right > 0 ) { // top-back-right
			EdgeChunk* chunk_test = midChunk->above->behind->right;
			uchar face_test       = EdgeChunk::FACE_BOTTOM;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				--d_up;
				++d_ahead;
				--d_right;
				continue;
			}
		}


		if ( d_up > 0 && d_ahead < 0 && d_right < 0 ) { // top-back-left
			EdgeChunk* chunk_test = midChunk->above->behind->left;
			uchar face_test       = EdgeChunk::FACE_BOTTOM;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				--d_up;
				++d_ahead;
				++d_right;
				continue;
			}
		}


		//
		// Bottom Corners (4)
		//

		if ( d_up < 0 && d_ahead > 0 && d_right > 0 ) { // bottom-front-right
			EdgeChunk* chunk_test = midChunk->below->infront->right;
			uchar face_test       = EdgeChunk::FACE_TOP;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				++d_up;
				--d_ahead;
				--d_right;
				continue;
			}
		}


		if ( d_up < 0 && d_ahead > 0 && d_right < 0 ) { // bottom-front-left
			EdgeChunk* chunk_test = midChunk->below->infront->left;
			uchar face_test       = EdgeChunk::FACE_TOP;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				++d_up;
				--d_ahead;
				++d_right;
				continue;
			}
		}


		if ( d_up < 0 && d_ahead < 0 && d_right > 0 ) { // bottom-back-right
			EdgeChunk* chunk_test = midChunk->below->behind->right;
			uchar face_test       = EdgeChunk::FACE_TOP;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				++d_up;
				++d_ahead;
				--d_right;
				continue;
			}
		}


		if ( d_up < 0 && d_ahead < 0 && d_right < 0 ) { // bottom-back-left
			EdgeChunk* chunk_test = midChunk->below->behind->left;
			uchar face_test       = EdgeChunk::FACE_TOP;
			t = chunk_test->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, face_test );
			if ( t == 0 ) t = fit_t_in_chunk( vp0, chunk_test, direction, t );

			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t >= 0 && chunk_test->pointInFace( hitpoint, face_test ) ) {
				midChunk = chunk_test;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				++d_up;
				++d_ahead;
				++d_right;
				continue;
			}
		}




		// TODO: get rid of EPSILON?
		// TODO: abstract the calculations

		// it MUST hit at least one neighbour
		assert(false);
	}

	EdgeChunk::addMissingEdgeChunks( &containers, p0Chunk, p0, EdgeTriTree::terrain );
	EdgeChunk::addMissingEdgeChunks( &containers, p1Chunk, p1, EdgeTriTree::terrain );

	return containers;
}
// ============================================== //


// ============================================== //
vector<EdgeTriTree::EdgeChunk*> EdgeTriTree::EdgeTriNode::getContainers() {
	return getContainers( p0, p1 );
}
// ============================================== //


// ============================================== //
Tri** EdgeTriTree::EdgeTriNode::getTriSide(Tri* tri) {
	if ( ( p0 == tri->p0 && p1 == tri->p1 ) ||
		 ( p0 == tri->p1 && p1 == tri->p2 ) ||
		 ( p0 == tri->p2 && p1 == tri->p0 ) ) {
		return &triangle_p0p1;
	} else if (
		 ( p1 == tri->p0 && p0 == tri->p1 ) ||
		 ( p1 == tri->p1 && p0 == tri->p2 ) ||
		 ( p1 == tri->p2 && p0 == tri->p0 ) ) {
		return &triangle_p1p0;
	} else {
		return 0;
	}
}
// ============================================== //


// ============================================== //
bool Tri::oneOrTheOther(ushort p0, ushort p1, ushort match_p0, ushort match_p1) {
	return ((match_p0 == p0 && match_p1 == p1) || (match_p1 == p0 && match_p0 == p1));
}
// ============================================== //


// ============================================== //
ushort Tri::getOddPoint(ushort p0, ushort p1) {
	if (oneOrTheOther(this->p0, this->p1, p0, p1)) {
		return this->p2;
	} else if (oneOrTheOther(this->p0, this->p2, p0, p1)) {
		return this->p1;
	} else if (oneOrTheOther(this->p1, this->p2, p0, p1)) {
		return this->p0;
	} else {
		assert(false);
	}
}
// ============================================== //


// ============================================== //
Tri** Tri::getNeighbourOnEdge(ushort p0, ushort p1) {
	if (oneOrTheOther(this->p0, this->p1, p0, p1)) {
		return &neighbour_p0p1;
	} else if (oneOrTheOther(this->p0, this->p2, p0, p1)) {
		return &neighbour_p2p0;
	} else {
		return &neighbour_p1p2;
	}
}
// ============================================== //


// ============================================== //
vector<EdgeTriTree::EdgeChunk*> EdgeTriTree::EdgeTriNode::getContainer(ushort p0) {

	// find p0 chunk container
	EdgeChunk* p0Chunk = EdgeTriTree::terrain->edgeTree->headChunk;
	Vertex vp0 = terrain->vertexBuffer[p0];
	while ( true ) {
		if ( vp0.v_x >= p0Chunk->worldOffset.x + p0Chunk->chunkSize.x && p0Chunk->right ) { p0Chunk = p0Chunk->right; continue; }
		if ( vp0.v_y >= p0Chunk->worldOffset.y + p0Chunk->chunkSize.y && p0Chunk->above ) { p0Chunk = p0Chunk->above; continue; }
		if ( vp0.v_z >= p0Chunk->worldOffset.z + p0Chunk->chunkSize.z && p0Chunk->infront ) { p0Chunk = p0Chunk->infront; continue; }

		if ( vp0.v_x < p0Chunk->worldOffset.x && p0Chunk->left ) { p0Chunk = p0Chunk->left; continue; }
		if ( vp0.v_y < p0Chunk->worldOffset.y && p0Chunk->below ) { p0Chunk = p0Chunk->below; continue; }
		if ( vp0.v_z < p0Chunk->worldOffset.z && p0Chunk->behind ) { p0Chunk = p0Chunk->behind; continue; }

		break;
	}

	vector<EdgeChunk*> chunks;
	chunks.push_back( p0Chunk );
	EdgeChunk::addMissingEdgeChunks( &chunks, p0Chunk, p0, EdgeTriTree::terrain );

	return chunks;
}
// ============================================== //


// ============================================== //
void Tri::assertBadTri(Tri* tri) {
	// Check each neighbour that they've also neighboured us back
	// Also check that none of our neighboured tri's (nor the neighbours of our neighboured tri's) are the same..ie they
	// all must be unique from one another
	if ( tri->neighbour_p0p1 ) {
		assert( tri->neighbour_p0p1->neighbour_p0p1 == tri ||
				tri->neighbour_p0p1->neighbour_p1p2 == tri ||
				tri->neighbour_p0p1->neighbour_p2p0 == tri );
		assert( tri->neighbour_p0p1->neighbour_p0p1 == 0 || tri->neighbour_p0p1->neighbour_p0p1 != tri->neighbour_p0p1->neighbour_p1p2 );
		assert( tri->neighbour_p0p1->neighbour_p0p1 == 0 || tri->neighbour_p0p1->neighbour_p0p1 != tri->neighbour_p0p1->neighbour_p2p0 );
		assert( tri->neighbour_p0p1->neighbour_p1p2 == 0 || tri->neighbour_p0p1->neighbour_p1p2 != tri->neighbour_p0p1->neighbour_p2p0 );
		assert( ( tri->neighbour_p0p1->p0 == tri->p0 && tri->neighbour_p0p1->p1 == tri->p1 ) ||
				( tri->neighbour_p0p1->p1 == tri->p0 && tri->neighbour_p0p1->p2 == tri->p1 ) ||
				( tri->neighbour_p0p1->p2 == tri->p0 && tri->neighbour_p0p1->p0 == tri->p1 ) ||
				( tri->neighbour_p0p1->p0 == tri->p0 && tri->neighbour_p0p1->p2 == tri->p1 ) ||
				( tri->neighbour_p0p1->p1 == tri->p0 && tri->neighbour_p0p1->p0 == tri->p1 ) ||
				( tri->neighbour_p0p1->p2 == tri->p0 && tri->neighbour_p0p1->p1 == tri->p1 ) ); // connects on same edge
	}
	if ( tri->neighbour_p1p2 ) {
		assert( tri->neighbour_p1p2->neighbour_p0p1 == tri ||
				tri->neighbour_p1p2->neighbour_p1p2 == tri ||
				tri->neighbour_p1p2->neighbour_p2p0 == tri );
		assert( tri->neighbour_p1p2->neighbour_p0p1 == 0 || tri->neighbour_p1p2->neighbour_p0p1 != tri->neighbour_p1p2->neighbour_p1p2 );
		assert( tri->neighbour_p1p2->neighbour_p0p1 == 0 || tri->neighbour_p1p2->neighbour_p0p1 != tri->neighbour_p1p2->neighbour_p2p0 );
		assert( tri->neighbour_p1p2->neighbour_p1p2 == 0 || tri->neighbour_p1p2->neighbour_p1p2 != tri->neighbour_p1p2->neighbour_p2p0 );
		assert( ( tri->neighbour_p1p2->p0 == tri->p1 && tri->neighbour_p1p2->p1 == tri->p2 ) ||
				( tri->neighbour_p1p2->p1 == tri->p1 && tri->neighbour_p1p2->p2 == tri->p2 ) ||
				( tri->neighbour_p1p2->p2 == tri->p1 && tri->neighbour_p1p2->p0 == tri->p2 ) ||
				( tri->neighbour_p1p2->p0 == tri->p1 && tri->neighbour_p1p2->p2 == tri->p2 ) ||
				( tri->neighbour_p1p2->p1 == tri->p1 && tri->neighbour_p1p2->p0 == tri->p2 ) ||
				( tri->neighbour_p1p2->p2 == tri->p1 && tri->neighbour_p1p2->p1 == tri->p2 ) ); // connects on same edge
	}
	if ( tri->neighbour_p2p0 ) {
		assert( tri->neighbour_p2p0->neighbour_p0p1 == tri ||
				tri->neighbour_p2p0->neighbour_p1p2 == tri ||
				tri->neighbour_p2p0->neighbour_p2p0 == tri );
		assert( tri->neighbour_p2p0->neighbour_p0p1 == 0 || tri->neighbour_p2p0->neighbour_p0p1 != tri->neighbour_p2p0->neighbour_p1p2 );
		assert( tri->neighbour_p2p0->neighbour_p0p1 == 0 || tri->neighbour_p2p0->neighbour_p0p1 != tri->neighbour_p2p0->neighbour_p2p0 );
		assert( tri->neighbour_p2p0->neighbour_p1p2 == 0 || tri->neighbour_p2p0->neighbour_p1p2 != tri->neighbour_p2p0->neighbour_p2p0 );
		assert( ( tri->neighbour_p2p0->p0 == tri->p2 && tri->neighbour_p2p0->p1 == tri->p0 ) ||
				( tri->neighbour_p2p0->p1 == tri->p2 && tri->neighbour_p2p0->p2 == tri->p0 ) ||
				( tri->neighbour_p2p0->p2 == tri->p2 && tri->neighbour_p2p0->p0 == tri->p0 ) ||
				( tri->neighbour_p2p0->p0 == tri->p2 && tri->neighbour_p2p0->p2 == tri->p0 ) ||
				( tri->neighbour_p2p0->p1 == tri->p2 && tri->neighbour_p2p0->p0 == tri->p0 ) ||
				( tri->neighbour_p2p0->p2 == tri->p2 && tri->neighbour_p2p0->p1 == tri->p0 ) ); // connects on same edge
	}

	assert( tri->neighbour_p0p1 == 0 || tri->neighbour_p0p1 != tri->neighbour_p1p2 );
	assert( tri->neighbour_p0p1 == 0 || tri->neighbour_p0p1 != tri->neighbour_p2p0 );
	assert( tri->neighbour_p1p2 == 0 || tri->neighbour_p1p2 != tri->neighbour_p2p0 );
}
// ============================================== //


// ============================================== //
void Tri::reshapeTriOnEdge(ushort p0, ushort p1, ushort p0_new, ushort p1_new) {
	if ( this->p0 == p0 ) {
		this->p0 = p0_new;
		if ( this->p1 == p1 ) {
			this->p1 = p1_new;
		} else {
			this->p2 = p1_new;
		}
	} else if ( this->p1 == p0 ) {
		this->p1 = p0_new;
		if ( this->p0 == p1 ) {
			this->p0 = p1_new;
		} else {
			this->p2 = p1_new;
		}
	} else {
		this->p2 = p0_new;
		if ( this->p0 == p1 ) {
			this->p0 = p1_new;
		} else {
			this->p1 = p1_new;
		}
	}
	this->chunk->triangleBuffer[this->triIndex].p0 = this->p0;
	this->chunk->triangleBuffer[this->triIndex].p1 = this->p1;
	this->chunk->triangleBuffer[this->triIndex].p2 = this->p2;
	Log(str(format("***Reshaped Tri*** {%1%,%2%,%3%}")%this->p0%this->p1%this->p2));
}
// ============================================== //


// ============================================== //
bool EdgeTriTree::addTriangle(Chunk* chunk, ushort triIndex, Environment* env) {

	
	/* EdgeOfTri
	 *
	 * The triangle has 3 sides to it; each side needs to be added into the edge tree. The cases during this add phase
	 * can be very specific for each edge (eg. one edge needs to be added to the tree, another edge has been found as a
	 * smaller scale of an existing edge, the last edge is found as a larger scale of an existing edge). This struct
	 * defines all the necessary information for each edge, which can later be handled accordingly (subdividing the
	 * triangle, re-neighbouring, etc.). If the added edge affects an existing edge/triangle then all necessary
	 * manipulations on the existing edge/triangle will be handled in here (eg. edge subdivision, splitting &
	 * re-neighbouring triangles)
	 ***/
	struct EdgeOfTri {

		void assertEdgeN(ushort pt, EdgeTriNode* neighbour) {
			assert(neighbour->p0 == pt || neighbour->p1 == pt);
		}

		void assertBadNeighbours(EdgeTriNode* edge) {
			return; // FIXME: test speeds
			vector<pair<ushort, vector<EdgeTriNode*>*>> neighbouringChecks;
			neighbouringChecks.push_back({ edge->p0, &edge->p0_edges });
			neighbouringChecks.push_back({ edge->p1, &edge->p1_edges });

			// Check that the attached tri's are all good
			assert( (edge->triangle_p0p1==0) ||
					(edge->triangle_p0p1->p0 == edge->p0 && edge->triangle_p0p1->p1 == edge->p1) ||
					(edge->triangle_p0p1->p1 == edge->p0 && edge->triangle_p0p1->p2 == edge->p1) ||
					(edge->triangle_p0p1->p2 == edge->p0 && edge->triangle_p0p1->p0 == edge->p1) );
			assert( (edge->triangle_p1p0==0) ||
					(edge->triangle_p1p0->p1 == edge->p0 && edge->triangle_p1p0->p0 == edge->p1) ||
					(edge->triangle_p1p0->p2 == edge->p0 && edge->triangle_p1p0->p1 == edge->p1) ||
					(edge->triangle_p1p0->p0 == edge->p0 && edge->triangle_p1p0->p2 == edge->p1) );

			for ( auto neighbouringCheck : neighbouringChecks ) {
				ushort pt = neighbouringCheck.first;
				vector<EdgeTriNode*>* neighbours = neighbouringCheck.second;
				int i=0;
				for ( auto neighbour : (*neighbours) ) {
					if ( neighbour == edge ) {
						assert(false); // Neighboured ourselves!?
					}
					if ( neighbour->p0 == pt ) {
						bool foundOurselves = false;
						for ( auto some_edge : neighbour->p0_edges ) {
							if ( some_edge == edge ) {
								foundOurselves = true;
								break;
							}
						}
						if ( !foundOurselves ) assert(false);
					} else if ( neighbour->p1 == pt ) {
						bool foundOurselves = false;
						for ( auto some_edge : neighbour->p1_edges ) {
							if ( some_edge == edge ) {
								foundOurselves = true;
								break;
							}
						}
						if ( !foundOurselves ) assert(false);
					} else {
						assert(false); // we have a neighbour who's pt doesnt match ours
					}

					// check for duplicates
					int j=0;
					for ( auto other_neighbour : (*neighbours) ) {
						if (j>i) {
							if ( neighbour == other_neighbour ) {
								assert(false); // Found duplicate!
							}
						}

						++j;
					}

					++i;
				}
				i=0;
			}


		}

		void touchingEdge(EdgeTriNode* edge) {
			// TODO: remove this..
		}

		// retrieve the edge(s) for this side of the Tri
		//  > sets up edges (edge of tri being added)
		//  	NOTE: added triangle ALWAYS belongs on p0p1 side of edge
		//  > creates any necessary Tri's to subdivide existing Triangle for creating our new edge
		EdgeOfTri(ushort p0, ushort p1, Chunk* chunk) {

			EdgeTriNode* nmatch; ushort pt; // Used for asserts
			Terrain* terrain = chunk->terrain;

			Vertex vp0 = terrain->vertexBuffer[p0];
			Vertex vp1 = terrain->vertexBuffer[p1];
			containers = EdgeTriNode::getContainers(p0, p1); // all containers containing the edge(s)

			journalEntry = new JournalEntry();

			Log(str(format("	EdgeOfTri( {%1%,%2%} <%3%, %4%, %5%>, <%6%, %7%, %8%> )") % p0 % p1 %
						vp0.v_x % vp0.v_y % vp0.v_z %
						vp1.v_x % vp1.v_y % vp1.v_z ));


			// ======================================================
			// EXISTING EDGES
			// ======================================================
			// check all edges in containers to see if there are any potentially matching edges
			// 	ie. this edge is a subdivided portion of another edge, or vice versa
			Vertex direction = Vertex( vp1.v_x - vp0.v_x, vp1.v_y - vp0.v_y, vp1.v_z - vp0.v_z );
			glm::vec3 dir_vec = glm::normalize(glm::vec3( vp1.v_x - vp0.v_x, vp1.v_y - vp0.v_y, vp1.v_z - vp0.v_z ));
			vector<EdgeTriNode*> potentialMatches;
			// Log(str(format("		dir_vec: <%1%,%2%,%3%>")%dir_vec.x%dir_vec.y%dir_vec.z));
			for ( auto chunk : containers ) {
				for ( auto e_edge : chunk->edges ) {
					assertBadNeighbours(e_edge);
					Vertex e_p0  = terrain->vertexBuffer[e_edge->p0];
					Vertex e_p1  = terrain->vertexBuffer[e_edge->p1];

					// Edges are the same direction?
					glm::vec3 e_dir_vec = glm::normalize(glm::vec3( e_p1.v_x - e_p0.v_x, e_p1.v_y - e_p0.v_y, e_p1.v_z - e_p0.v_z ));
					glm::vec3 e_dir_vec_n = e_dir_vec * -1.0f;
					// Log(str(format("		considing? (%1%,%2%) :: e_dir_vec == <%3%,%4%,%5%> :: -1<%6%,%7%,%8%>")%e_edge->p0%e_edge->p1%e_dir_vec.x%e_dir_vec.y%e_dir_vec.z%e_dir_vec_n.x%e_dir_vec_n.y%e_dir_vec_n.z));
#define VEC_MATCH_ERR 0.0001f // NOTE: 0.01 isn't enough
					if ( glm::distance(dir_vec, e_dir_vec) < VEC_MATCH_ERR || glm::distance(dir_vec, e_dir_vec*-1.0f) < VEC_MATCH_ERR ) {
						// Log(str(format("		same direction: (%1%,%2%)   (distance: %3%) (-distance: %4%)")%e_edge->p0%e_edge->p1%
						// 		glm::distance(dir_vec, e_dir_vec)%glm::distance(dir_vec, e_dir_vec*-1.0f)	));
						assert(glm::distance(dir_vec, e_dir_vec*-1.0f) >= 0.0f);
						assert(glm::distance(dir_vec, e_dir_vec) >= 0.0f);

						// Edges are parallel, or apart of the same continuous line?
						bool possibleMatch = false;
						if ( e_edge->p1 != p0 ) {
							e_dir_vec = glm::normalize(glm::vec3( e_p1.v_x - vp0.v_x, e_p1.v_y - vp0.v_y, e_p1.v_z - vp0.v_z ));
							e_dir_vec_n = e_dir_vec * -1.0f;
							// Log(str(format("			e_dir_vec: <%1%,%2%,%3%>")%e_dir_vec.x%e_dir_vec.y%e_dir_vec.z));
							// Log(str(format("			dir_vec: <%1%,%2%,%3%>")%dir_vec.x%dir_vec.y%dir_vec.z));
							// Log(str(format("			distance: %1%")%(glm::distance(dir_vec,e_dir_vec))));
							// Log(str(format("			-distance: %1%")%(glm::distance(dir_vec,(e_dir_vec*-1.0f)))));
							if ( dir_vec == e_dir_vec || dir_vec == e_dir_vec_n ) possibleMatch = true;
							else if (glm::distance(dir_vec, e_dir_vec) < VEC_MATCH_ERR || glm::distance(dir_vec, e_dir_vec_n) < VEC_MATCH_ERR) {
								// TODO: there is room for tiny floating point errors..this causes issues in matching
								// edges
								Log(str(format("		POTENTIAL MATCH HAD TO BE RECONSIDERED FROM NORMALIZATION / FLOATING ERROR")));
								possibleMatch=true;
							}
						} else {
							possibleMatch = true;
						}
						
						if ( possibleMatch ) {

							// Have we already recorded this edge?
							bool alreadyAdded = false;
							for ( auto ep_edge : potentialMatches ) {
								if ( ep_edge == e_edge ) {
									alreadyAdded = true;
									break;
								}
							}

							if ( !alreadyAdded ) {
								Log(str(format("	Potential Match: %1% <%2%, %3%, %4%>, %5% <%6%, %7%, %8%>") %
											e_edge->p0 % e_p0.v_x % e_p0.v_y % e_p0.v_z %
											e_edge->p1 % e_p1.v_x % e_p1.v_y % e_p1.v_z ));
								potentialMatches.push_back( e_edge );
							}
						}

					} else {
						float ex = e_dir_vec.x;
						float ey = e_dir_vec.y;
						float ez = e_dir_vec.z;
						float enx = e_dir_vec_n.x;
						float eny = e_dir_vec_n.y;
						float enz = e_dir_vec_n.z;
						if (ex == -0) ex = 0;
						if (ey == -0) ey = 0;
						if (ez == -0) ez = 0;
						if (enx == -0) enx = 0;
						if (eny == -0) eny = 0;
						if (enz == -0) enz = 0;
						assert(!(
							(e_dir_vec.x == dir_vec.x && e_dir_vec.y == dir_vec.y && e_dir_vec.z == dir_vec.z) ||
							(e_dir_vec_n.x == dir_vec.x && e_dir_vec_n.y == dir_vec.y && e_dir_vec_n.z == dir_vec.z) ));
						assert(!(
							(ex == dir_vec.x && ey == dir_vec.y && ez == dir_vec.z) ||
							(enx == dir_vec.x && eny == dir_vec.y && enz == dir_vec.z) ));
					}
				}
			}


			// ===================================================
			// GET EDGE(S)
			// ===================================================
			// Potential Matches lay along the same continuous line; is either edge within the other one?
			bool pleaseFillHoles = false; // if added edges MAY have holes between them, fill those with new edges
			for ( auto e_edge : potentialMatches ) {

				Vertex e_p0  = terrain->vertexBuffer[e_edge->p0];
				Vertex e_p1  = terrain->vertexBuffer[e_edge->p1];

				// Log(str(format("	--> {%1%,%2%} <%3%, %4%, %5%> <%6%, %7%, %8%>") % e_edge->p0 % e_edge->p1 %
				// 		e_p0.v_x % e_p0.v_y % e_p0.v_z %
				// 		e_p1.v_x % e_p1.v_y % e_p1.v_z ));
				int whichCase = -1; // TODO: removing whichCase assignment in if statements causes crash..
				try {
					if ( ((whichCase = 1) && Tri::oneOrTheOther( p0, p1, e_edge->p0, e_edge->p1 )) ) {

						// Log(str(format("	MATCH: Same edge!")));
						assert( edges.size() == 0 );
						// This edge is exactly the one we're looking for
						InnerEdgeOfTri* matchedEdge = new InnerEdgeOfTri();
						matchedEdge->edge = e_edge;
						matchedEdge->flippedEdge = ( p0 != e_edge->p0 );
						edges.push_back( matchedEdge );
						break;

					} else if ( ((whichCase = 2) && vp0.between( e_p0, e_p1 ) && vp1.between( e_p0, e_p1 )) ) {
						// The edge is a subdivision of this existing edge

						// Log(str(format("	MATCH: adding edge which is subdivision of existing edge; (%1%)<%2%, %3%, %4%> <= (%5%)<%6%, %7%, %8%> <= (%9%)<%10%, %11%, %12%> <= (%13%)<%14%, %15%, %16%>")%
						// 			e_edge->p0 % e_p0.v_x % e_p0.v_y % e_p0.v_z %
						// 			p0 % vp0.v_x % vp0.v_y % vp0.v_z %
						// 			p1 % vp1.v_x % vp1.v_y % vp1.v_z %
						// 			e_edge->p1 % e_p1.v_x % e_p1.v_y % e_p1.v_z ));
						assert( edges.size() == 0 );
						// Remove existing edge from all containers
						vector<EdgeChunk*> e_containers = e_edge->getContainers();
						for ( auto container : e_containers ) {
							int i = 0;
							for ( auto some_edge : container->edges ) {
								if ( some_edge == e_edge ) {
									break;
								}
								++i;
							}
							container->edges.erase( container->edges.begin() + i );
						}


						// Disconnect edge from graph
						assertBadNeighbours( e_edge );
						vector<vector<EdgeTriNode*>*> e_edge_endPoints;
						e_edge_endPoints.push_back( &e_edge->p0_edges );
						e_edge_endPoints.push_back( &e_edge->p1_edges );
						ushort e_edge_endPoints_pRef = e_edge->p0;
						ushort dbgA=0, dbgB=0;
						// Log(str(format("		Disconnecting {%1%,%2%} edge")% e_edge->p0 % e_edge->p1));
						for ( auto e_edge_endPoint : e_edge_endPoints ) {
							for ( auto neighbour_edge : (*e_edge_endPoint) ) {

								assertBadNeighbours( neighbour_edge );

								// Disconnect from either side of the neighbour edge
								int i = 0;
								bool disconnected = false;
								for ( auto some_edge : neighbour_edge->p0_edges ) {
									if ( some_edge == e_edge ) {
										touchingEdge(neighbour_edge);
										neighbour_edge->p0_edges.erase( neighbour_edge->p0_edges.begin() + i );
										disconnected = true;
										break;
									}
									++i;
								}
								if (disconnected) {
									// Log(str(format("			Disconnected from {%1%,%2%}")% neighbour_edge->p0 % neighbour_edge->p1));
								} else {

									i = 0;
									for ( auto some_edge : neighbour_edge->p1_edges ) {
										if ( some_edge == e_edge ) {
											touchingEdge(neighbour_edge);
											neighbour_edge->p1_edges.erase( neighbour_edge->p1_edges.begin() + i );
											disconnected = true;
											break;
										}
										++i;
									}
									assert( disconnected );
									// Log(str(format("			Disconnected from {%1%,%2%}")% neighbour_edge->p0 % neighbour_edge->p1));
								}


								/*
								   vector<EdgeTriNode*>* neighbourConnection = ( neighbour_edge->p0 == e_edge_endPoints_pRef ?
								   &neighbour_edge->p0_edges : &neighbour_edge->p1_edges );

								   for ( auto some_edge : *neighbourConnection ) {
								   if ( some_edge == e_edge ) {
								   break;
								   }
								   ++i;
								   }
								   neighbourConnection->erase( neighbourConnection->begin() + i );
								 */

								++dbgB;
							}
							e_edge_endPoints_pRef = e_edge->p1;
							++dbgA;
							dbgB=0;
						}


						// Create new edge
						EdgeTriNode* edge = new EdgeTriNode();
						edge->p0 = p0;
						edge->p1 = p1;

						InnerEdgeOfTri* newEdge = new InnerEdgeOfTri();
						newEdge->edge = edge;

						// Is the edge flipped (match existing edge)
						// p0 between p1 and existing p1 --> flipped
						newEdge->flippedEdge = false;
						if ( vp0.between( vp1, e_p1 ) ) {
							newEdge->flippedEdge = true;
						}
						edges.push_back( newEdge );


						// ======================================================
						// SUBDIVIDE TRIANGLE (existing)
						// ======================================================
						// Existing triangle needs to be subdivided along its edge to match our new edge
						// Case 1 (2 cuts): Cut triangle from p0->p1, p1->ep1  [ep0==p0]
						// Case 2 (2 cuts): Cut triangle from ep0->p0, p0->p1  [ep1==p1]
						// Case 3 (3 cuts): Cut triangle from ep0->p0, p0->p1, p1->ep1 [no matches]
						// NOTE: because the existing edge is being cut, those cut pieces are not actually apart of the edge
						// in which we are concerned with in adding our triangle; hence, those edges do not need to be added
						// to this edge list

						if ( (newEdge->flippedEdge && p0 == e_edge->p1) || p0 == e_edge->p0 ) {
							// Case 1 (2 cuts); p0->p1, {p1->ep1,p1->ep0}

							Log(str(format("		Subdivision (Case 1) : p0->p1, p1->{ep1}")));
							Tri* e_tri = (newEdge->flippedEdge? e_edge->triangle_p0p1 : e_edge->triangle_p1p0);


							Vertex evp0 = terrain->vertexBuffer[e_tri->p0];
							Vertex evp1 = terrain->vertexBuffer[e_tri->p1];
							Vertex evp2 = terrain->vertexBuffer[e_tri->p2];
							ushort p2 = e_tri->getOddPoint(e_edge->p0, e_edge->p1);
							Vertex vp2 = terrain->vertexBuffer[p2];
							// Log(str(format("			p2 {%1%}: <%2%, %3%, %4%>") % p2 % vp2.v_x % vp2.v_y % vp2.v_z));

							// Create edge for end segment
							EdgeTriNode* edge_end = new EdgeTriNode();
							edge_end->p0 = p1;
							edge_end->p1 = (newEdge->flippedEdge? e_edge->p0 : e_edge->p1);

							// Create edge from p1->(e_p2)
							EdgeTriNode* edge_split = new EdgeTriNode();
							edge_split->p0 = p1;
							edge_split->p1 = p2;


							// Connect Edges
							// ----------------
							// Neighbour edge/edge_end/edge_split edges
							edge->p1_edges.push_back(edge_end);
							assertEdgeN( edge->p1, edge_end );
							edge->p1_edges.push_back(edge_split);
							assertEdgeN( edge->p1, edge_split );
							// Log(str(format("			Neighboured: Edge(p1) {%1%,%2%} w/ EdgeEnd {%3%,%4%}")% 
							// 			edge->p0 % edge->p1 % edge_end->p0 % edge_end->p0 ));
							// Log(str(format("			Neighboured: Edge(p1) {%1%,%2%} w/ EdgeSplit {%3%,%4%}")% 
							// 			edge->p0 % edge->p1 % edge_split->p0 % edge_split->p0 ));

							edge_end->p0_edges.push_back(edge);
							assertEdgeN( edge_end->p0, edge );
							edge_end->p0_edges.push_back(edge_split);
							assertEdgeN( edge_end->p0, edge_split );
							// Log(str(format("			Neighboured: EdgeEnd(p0) {%1%,%2%} w/ Edge {%3%,%4%}")% 
							// 			edge_end->p0 % edge_end->p1 % edge->p0 % edge->p1 ));
							// Log(str(format("			Neighboured: EdgeEnd(p0) {%1%,%2%} w/ EdgeSplit {%3%,%4%}")% 
							// 			edge_end->p0 % edge_end->p1 % edge_split->p0 % edge_split->p1 ));

							edge_split->p0_edges.push_back(edge);
							assertEdgeN( edge_split->p0, edge );
							// Log(str(format("			Neighboured: EdgeSplit(p0) {%1%,%2%} w/ Edge {%3%,%4%}")% 
							// 			edge_split->p0 % edge_split->p1 % edge->p0 % edge->p1 ));
							edge_split->p0_edges.push_back(edge_end);
							assertEdgeN( edge_split->p0, edge_end );
							// Log(str(format("			Neighboured: EdgeSplit(p0) {%1%,%2%} w/ EdgeEnd {%3%,%4%}")% 
							// 			edge_split->p0 % edge_split->p1 % edge_end->p0 % edge_end->p1 ));

							assertBadNeighbours(edge);
							assertBadNeighbours(edge_end);
							assertBadNeighbours(edge_split);

							// Find p0p2/p1p2 edge to help in re-neighbouring the edge_split edge
							EdgeTriNode* edge_p0p2 = 0;
							EdgeTriNode* edge_p1p2 = 0;
							bool edge_p0p2_flipped = false;
							bool edge_p1p2_flipped = false;
							ushort edge_p1_expected = (newEdge->flippedEdge? e_edge->p0 : e_edge->p1);
							vector<EdgeTriNode*>* edge_p0p2_neighbouringElement = 0;
							vector<EdgeTriNode*>* edge_p1p2_neighbouringElement = 0;
							assertBadNeighbours(edge);
							for ( auto neighbourEdge : (newEdge->flippedEdge? e_edge->p1_edges : e_edge->p0_edges) ) {
								if ( !edge_p0p2 && Tri::oneOrTheOther( neighbourEdge->p0, neighbourEdge->p1, p0, p2 ) ) {
									edge_p0p2 = neighbourEdge;
									edge_p0p2_flipped = (neighbourEdge->p1 == p0);
									edge_p0p2_neighbouringElement = ( edge_p0p2_flipped ? &edge_p0p2->p0_edges : &edge_p0p2->p1_edges );
									Log(str(format("			Found edge_p0p2 {%1%,%2%} Flipped/NeighbouringEl: [%3%](%4%)")%
												neighbourEdge->p0 % neighbourEdge->p1 % (edge_p0p2_flipped?"Flipped":"NotFlipped") % (edge_p0p2_flipped?"p0":"p1")));

									// edge_split->p1_edges.push_back(edge_p0p2);
									assertEdgeN( edge_split->p1, edge_p0p2 );
									// NOTE: p0p2 must neighbour edge_split AFTERWARDS; otherwise it will neighbour it
									// during the neighbouring section below
									// edge_p0p2_neighbouringElement->push_back(edge_split);
									assertEdgeN( (edge_p0p2_flipped?edge_p0p2->p0:edge_p0p2->p1), edge_split );
									assertBadNeighbours(edge);

									break;
								}
							}
							for ( auto neighbourEdge : (newEdge->flippedEdge? e_edge->p0_edges : e_edge->p1_edges) ) {
								if ( !edge_p1p2 && Tri::oneOrTheOther( neighbourEdge->p0, neighbourEdge->p1, edge_p1_expected, p2 ) ) {
									edge_p1p2 = neighbourEdge;
									edge_p1p2_flipped = (neighbourEdge->p1 == edge_p1_expected);
									edge_p1p2_neighbouringElement = ( edge_p1p2_flipped ? &edge_p1p2->p0_edges : &edge_p1p2->p1_edges );
									(*(edge_p1p2_flipped? &edge_p1p2->triangle_p0p1 : &edge_p1p2->triangle_p1p0)) = 0; // unattach triangle (to-be changed)
									Log(str(format("			Found edge_p1p2 {%1%,%2%} Flipped/NeighbouringEl: [%3%](%4%)")%
												neighbourEdge->p0 % neighbourEdge->p1 % (edge_p1p2_flipped?"Flipped":"NotFlipped") % (edge_p1p2_flipped?"p0":"p1")));

									// NOTE: the neighbouring done below neighbours ALL attached elements of p0p2(p1) to
									// edge_split, and vice-versa; this include p1p2 and hence this neighbouring is
									// unecessary here
									// edge_split->p1_edges.push_back(edge_p1p2);
									assertEdgeN( edge_split->p1, edge_p1p2 );
									// edge_p1p2_neighbouringElement->push_back(edge_split);
									assertEdgeN( (edge_p1p2_flipped?edge_p1p2->p0:edge_p1p2->p1), edge_split );
									assertBadNeighbours(edge);

									break;
								}
							}
							assert( edge_p0p2 ); // TODO: cannot find required neighbour edge of triangle which is attached
							//			p0p1 edge. 
							assert( edge_p1p2 );
							assert( edge_p0p2_neighbouringElement );
							assert( edge_p1p2_neighbouringElement );

							assertBadNeighbours(edge_p0p2);
							assertBadNeighbours(edge_p1p2);

							assertBadNeighbours(edge);
							assertBadNeighbours(edge_end);
							assertBadNeighbours(edge_split);

							// Copy neighbours from existing edge
							vector<tuple<vector<EdgeTriNode*>*, EdgeTriNode*, ushort>> edge_neighbours; // neighbours: {source, destination_node, point}
							edge_neighbours.push_back(make_tuple( (newEdge->flippedEdge? &e_edge->p1_edges : &e_edge->p0_edges), edge, edge->p0 ));
							edge_neighbours.push_back(make_tuple( (newEdge->flippedEdge? &e_edge->p0_edges : &e_edge->p1_edges), edge_end, edge_end->p1 ));
							edge_neighbours.push_back(make_tuple( edge_p0p2_neighbouringElement, edge_split, edge_split->p1 ));
							int j=0;
							for ( auto neighbouringElement : edge_neighbours ) {
								for ( auto neighbour : (*get<0>(neighbouringElement)) ) {
									EdgeTriNode* this_edge = get<1>(neighbouringElement);
									ushort pt = get<2>(neighbouringElement);
									(this_edge->p0 == pt ? this_edge->p0_edges : this_edge->p1_edges).push_back( neighbour );
									(neighbour->p0 == pt ? neighbour->p0_edges : neighbour->p1_edges).push_back( this_edge );

									string srcEl;
									string dstEl;
									if (j==0) { srcEl = "e_edge_neighbour"; dstEl = "edge(p0)"; }
									else if (j==1) { srcEl = "e_edge_neighbour"; dstEl = "edge_end(p1)"; }
									else { srcEl = "p0p2"; dstEl = "edge_split(p1)"; }
									Log(str(format("			Neighboured: %1% {%2%,%3%} w/ %4% {%5%,%6%}")% 
												srcEl % neighbour->p0 % neighbour->p1 % dstEl % this_edge->p0 % this_edge->p1 ));

									assertBadNeighbours(neighbour);
									assertBadNeighbours(this_edge);
								}
								++j;
							}
							edge_split->p1_edges.push_back(edge_p0p2);
							edge_p0p2_neighbouringElement->push_back(edge_split);
							Log(str(format("			Neighboured: EdgeSplit(p1) {%1%,%2%} w/ edge_p0p2 {%3%,%4%}")% 
										edge_split->p0 % edge_split->p1 % edge_p0p2->p0 % edge_p0p2->p1 ));
							Log(str(format("			Neighboured: edge_p0p2 {%1%,%2%} w/ EdgeSplit {%3%,%4%}")% 
										edge_p0p2->p0 % edge_p0p2->p1 % edge_split->p0 % edge_split->p1 ));

							assertBadNeighbours(edge);
							assertBadNeighbours(edge_end);
							assertBadNeighbours(edge_split);
							assertBadNeighbours( edge_p0p2 );
							assertBadNeighbours( edge_p1p2 );

							// attach to matching edges with p1
							vector<EdgeChunk*> p1Containers = EdgeTriNode::getContainer( p1 ); // TODO: p1Container is last in containers?
							EdgeChunk* p1Container = p1Containers.front(); // NOTE: any edges which lay in one of these containers, will also lay in all of the others; hence we only need to work with one of them
							Log(str(format("			Searching for Connections for {%1%,%2%}(p1)")% p0 % p1));
							for ( auto some_edge : p1Container->edges ) {
								if ( some_edge == edge_end ||
									 some_edge == edge_split ||
									 some_edge == edge_p0p2 ||
									 some_edge == edge_p1p2 ) continue;
								if ( some_edge->p0 == p1 ) {
									some_edge->p0_edges.push_back( edge );
									assertEdgeN(some_edge->p0, edge);
									edge->p1_edges.push_back( some_edge );
									assertEdgeN(edge->p1, some_edge);
									assertBadNeighbours(edge);
									assertBadNeighbours(some_edge);
									Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
								} else if ( some_edge->p1 == p1 ) {
									some_edge->p1_edges.push_back( edge );
									assertEdgeN(some_edge->p1, edge);
									edge->p1_edges.push_back( some_edge );
									assertEdgeN(edge->p1, some_edge);
									assertBadNeighbours(edge);
									assertBadNeighbours(some_edge);
									Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
								}
							}
							assertBadNeighbours(edge);


							// Add Edges (containers)
							// --------------------------
							// NOTE: do not need to add edge end to edges; since the triangle we're adding only fits on edge
							// and not edge_end
							vector<EdgeTriNode*> edges_containering; // edges in which we need to add to their respective containers
							edges_containering.push_back( edge_end );
							edges_containering.push_back( edge_split );
							edges_containering.push_back( edge );
							for ( auto edgeToContainer : edges_containering ) {
								vector<EdgeChunk*> edgeContainers = edgeToContainer->getContainers();
								for ( auto container : edgeContainers ) {
									container->edges.push_back( edgeToContainer );
								}

								journalEntry->operations.push_back( JournalEntry::AddEdge( edgeToContainer->p0, edgeToContainer->p1 ) );
							}


							// Subdivide Existing Triangle
							// ----------------------------

							// Subdivide (upper tri): {p1->ep1,p1->ep0}
							Log(str(format("	Subdivided Tri: <%1%, %2%, %3%>, <%4%, %5%, %6%>, <%7%, %8%, %9%>") %
										vp1.v_x % vp1.v_y % vp1.v_z %
										terrain->vertexBuffer[p2].v_x % terrain->vertexBuffer[p2].v_y % terrain->vertexBuffer[p2].v_z %
										e_p1.v_x % e_p1.v_y % e_p1.v_z ));
							e_tri->chunk->triangleBuffer.push_back( iTriangle( p1, p2, (newEdge->flippedEdge? e_edge->p0 : e_edge->p1) ) );
							ushort subTri_outi = e_tri->chunk->triangleBuffer.size() - 1;
							Tri* subTri_out = new Tri( e_tri->chunk, subTri_outi );
							Tri::assertBadTri(subTri_out);

							journalEntry->operations.push_back( JournalEntry::AddTri( subTri_outi, e_tri->chunk->id,
										subTri_out->p0, subTri_out->p1, subTri_out->p2 ) );

							// Subdivide (lower tri): {p0->p1} 
							// NOTE: using the same existing Tri since we refer to iTriangles by their index, we do NOT want
							// to remove a triangle ever from the list
							ushort e_tri_oldp0 = e_tri->p0,
								   e_tri_oldp1 = e_tri->p1,
								   e_tri_oldp2 = e_tri->p2;
							Log(str(format("	About to reshape tri: <%1%,%2%,%3%>")%e_tri->p0%e_tri->p1%e_tri->p2));
							e_tri->reshapeTriOnEdge( p0, (newEdge->flippedEdge? e_edge->p0 : e_edge->p1), p0, p1 );
							journalEntry->operations.push_back( JournalEntry::ReshapeTri( e_tri->triIndex, e_tri->chunk->id,
										e_tri_oldp0, e_tri_oldp1, e_tri_oldp2,
										e_tri->p0, e_tri->p1, e_tri->p2 ) );


							// Attach Tri's to Edges
							// -----------------------

							// Attach (upper tri)
							assert(!edge_end->triangle_p1p0);
							assert(!edge_split->triangle_p0p1);
							edge_end->triangle_p1p0 = subTri_out; // TODO: used to be p0p1
							edge_split->triangle_p0p1 = subTri_out;

							if ( (subTri_out->p0 == edge_p1p2->p0 && subTri_out->p1 == edge_p1p2->p1) ||
								 (subTri_out->p1 == edge_p1p2->p0 && subTri_out->p2 == edge_p1p2->p1) ||
								 (subTri_out->p2 == edge_p1p2->p0 && subTri_out->p0 == edge_p1p2->p1) ) {
								assert(!edge_p1p2->triangle_p0p1);
								edge_p1p2->triangle_p0p1 = subTri_out;
							} else if ( (subTri_out->p0 == edge_p1p2->p1 && subTri_out->p1 == edge_p1p2->p0) ||
								 		(subTri_out->p1 == edge_p1p2->p1 && subTri_out->p2 == edge_p1p2->p0) ||
								 		(subTri_out->p2 == edge_p1p2->p1 && subTri_out->p0 == edge_p1p2->p0) ) {
								assert(!edge_p1p2->triangle_p1p0);
								edge_p1p2->triangle_p1p0 = subTri_out;
							} else {
								assert(false);
								Tri** edge_p1p2_subTriSide = (edge_p1p2_flipped? &edge_p1p2->triangle_p0p1 : &edge_p1p2->triangle_p1p0 );
								assert(!(*edge_p1p2_subTriSide));
								(*edge_p1p2_subTriSide) = subTri_out;
							}


							// Attach (lower tri)
							assert(!edge->triangle_p1p0);
							assert(!edge_split->triangle_p1p0);
							edge->triangle_p1p0 = e_tri;
							edge_split->triangle_p1p0 = e_tri;
							Tri** edge_p0p2_triSide = (edge_p0p2_flipped? &edge_p0p2->triangle_p1p0 : &edge_p0p2->triangle_p0p1 );
							assert(!(*edge_p0p2_triSide) || (*edge_p0p2_triSide == e_tri));
							(*edge_p0p2_triSide) = e_tri;

							assertBadNeighbours(edge);
							assertBadNeighbours(edge_end);
							assertBadNeighbours(edge_split);
							assertBadNeighbours(edge_p1p2);
							assertBadNeighbours(edge_p0p2);


							// Neighbour the Tri's
							// ------------------------
							// NOTE: tri does not need to re-neighbour with the bottom since its only been resized from the
							// top

							// Neighbour subTri_out w/ top
							ushort ref_ep1 = (newEdge->flippedEdge?e_edge->p0:e_edge->p1);
							Tri** neighbourTri_ep1p2 = e_tri->getNeighbourOnEdge( p1, p2 ); // NOTE: p1 instead of ref_ep1 since e_tri has been resized
							if ( (*neighbourTri_ep1p2) ) {
								(*subTri_out->getNeighbourOnEdge(ref_ep1, p2)) = (*neighbourTri_ep1p2);
								(*(*neighbourTri_ep1p2)->getNeighbourOnEdge( ref_ep1, p2 )) = subTri_out;
							}

							// Neighbour subTri_out w/ tri
							(*neighbourTri_ep1p2) = subTri_out;
							(*subTri_out->getNeighbourOnEdge(p1, p2)) = e_tri;
							Tri::assertBadTri(subTri_out);
							Tri::assertBadTri(e_tri);

							// TODO: using reference Tri**, however we rewrite the top Tri neighbour in e_tri after setting
							// subTri to e_tri's neighbour. Will this affect subTri's neighbouring?

							Log(str(format("	Deleting e_edge {%1%,%2%}")%e_edge->p0%e_edge->p1));
							journalEntry->operations.push_back( JournalEntry::RemoveEdge( e_edge->p0, e_edge->p1 ) );
							delete e_edge;
						} else if ( (newEdge->flippedEdge && p1 == e_edge->p0) || p1 == e_edge->p1 ) {
							// Case 2 (2 cuts); {ep0->p0,ep1->p0}, p0->p1

							Log(str(format("		Subdivision (Case 2) : {ep0}->p0, p0->p1")));
							Tri* e_tri = (newEdge->flippedEdge? e_edge->triangle_p0p1 : e_edge->triangle_p1p0);
							Vertex evp0 = terrain->vertexBuffer[e_tri->p0];
							Vertex evp1 = terrain->vertexBuffer[e_tri->p1];
							Vertex evp2 = terrain->vertexBuffer[e_tri->p2];
							ushort p2 = e_tri->getOddPoint(e_edge->p0, e_edge->p1);
							Vertex vp2 = terrain->vertexBuffer[p2];
							Log(str(format("			p2 {%1%}: <%2%, %3%, %4%>") % p2 % vp2.v_x % vp2.v_y % vp2.v_z));

							// Create edge for end segment
							EdgeTriNode* edge_end = new EdgeTriNode();
							edge_end->p0 = (newEdge->flippedEdge? e_edge->p1 : e_edge->p0);
							edge_end->p1 = p0;

							// Create edge from p0->(e_p2)
							EdgeTriNode* edge_split = new EdgeTriNode();
							edge_split->p0 = p0;
							edge_split->p1 = p2;


							// Connect Edges
							// ----------------
							// Neighbour edge/edge_end/edge_split edges
							edge->p0_edges.push_back(edge_end);
							assertEdgeN( edge->p0, edge_end );
							edge->p0_edges.push_back(edge_split);
							assertEdgeN( edge->p0, edge_split );
							Log(str(format("			Neighboured: Edge(p0) {%1%,%2%} w/ EdgeEnd {%3%,%4%}")% 
										edge->p0 % edge->p1 % edge_end->p0 % edge_end->p0 ));
							Log(str(format("			Neighboured: Edge(p0) {%1%,%2%} w/ EdgeSplit {%3%,%4%}")% 
										edge->p0 % edge->p1 % edge_split->p0 % edge_split->p0 ));

							edge_end->p1_edges.push_back(edge);
							assertEdgeN( edge_end->p1, edge );
							edge_end->p1_edges.push_back(edge_split);
							assertEdgeN( edge_end->p1, edge_split );
							Log(str(format("			Neighboured: EdgeEnd(p1) {%1%,%2%} w/ Edge {%3%,%4%}")% 
										edge_end->p0 % edge_end->p1 % edge->p0 % edge->p1 ));
							Log(str(format("			Neighboured: EdgeEnd(p1) {%1%,%2%} w/ EdgeSplit {%3%,%4%}")% 
										edge_end->p0 % edge_end->p1 % edge_split->p0 % edge_split->p1 ));

							edge_split->p0_edges.push_back(edge);
							assertEdgeN( edge_split->p0, edge );
							edge_split->p0_edges.push_back(edge_end);
							assertEdgeN( edge_split->p0, edge_end );
							Log(str(format("			Neighboured: EdgeSplit(p0) {%1%,%2%} w/ Edge {%3%,%4%}")% 
										edge_split->p0 % edge_split->p1 % edge->p0 % edge->p1 ));
							Log(str(format("			Neighboured: EdgeSplit(p0) {%1%,%2%} w/ EdgeEnd {%3%,%4%}")% 
										edge_split->p0 % edge_split->p1 % edge_end->p0 % edge_end->p1 ));


							assertBadNeighbours(edge);
							assertBadNeighbours(edge_split);
							assertBadNeighbours(edge_end);

							// Find p1p2 edge to help in re-neighbouring the edge_split edge
							EdgeTriNode* edge_p0p2 = 0;
							EdgeTriNode* edge_p1p2 = 0;
							bool edge_p0p2_flipped = false;
							bool edge_p1p2_flipped = false;
							ushort edge_p0_expected = (newEdge->flippedEdge? e_edge->p1 : e_edge->p0);
							vector<EdgeTriNode*>* edge_p0p2_neighbouringElement = 0;
							vector<EdgeTriNode*>* edge_p1p2_neighbouringElement = 0;
							for ( auto neighbourEdge : (newEdge->flippedEdge? e_edge->p1_edges : e_edge->p0_edges) ) {
								if ( !edge_p0p2 && Tri::oneOrTheOther( neighbourEdge->p0, neighbourEdge->p1, edge_p0_expected, p2 ) ) {
									edge_p0p2 = neighbourEdge;
									edge_p0p2_flipped = (neighbourEdge->p1 == edge_p0_expected);
									(*(edge_p0p2_flipped? &edge_p0p2->triangle_p1p0 : &edge_p0p2->triangle_p0p1)) = 0; // unattach triangle (to-be changed)
									edge_p0p2_neighbouringElement = ( edge_p0p2_flipped ? &edge_p0p2->p0_edges : &edge_p0p2->p1_edges );
									Log(str(format("			Found edge_p0p2 {%1%,%2%} Flipped/NeighbouringEl: [%3%](%4%)")%
												neighbourEdge->p0 % neighbourEdge->p1 % (edge_p0p2_flipped?"Flipped":"NotFlipped") % (edge_p0p2_flipped?"p0":"p1")));

									// edge_split->p1_edges.push_back(edge_p0p2);
									assertEdgeN( edge_split->p1, edge_p0p2 );
									// edge_p0p2_neighbouringElement->push_back(edge_split);
									assertEdgeN( (edge_p0p2_flipped?edge_p0p2->p0:edge_p0p2->p1), edge_split );

									break;
								}
							}
							for ( auto neighbourEdge : (newEdge->flippedEdge? e_edge->p0_edges : e_edge->p1_edges) ) {
								if ( !edge_p1p2 && Tri::oneOrTheOther( neighbourEdge->p0, neighbourEdge->p1, p1, p2 ) ) {
									edge_p1p2 = neighbourEdge;
									edge_p1p2_flipped = (neighbourEdge->p1 == p1);
									edge_p1p2_neighbouringElement = ( edge_p1p2_flipped ? &edge_p1p2->p0_edges : &edge_p1p2->p1_edges );
									Log(str(format("			Found edge_p1p2 {%1%,%2%} Flipped/NeighbouringEl: [%3%](%4%)")%
												neighbourEdge->p0 % neighbourEdge->p1 % (edge_p1p2_flipped?"Flipped":"NotFlipped") % (edge_p1p2_flipped?"p0":"p1")));

									// edge_split->p1_edges.push_back(edge_p1p2);
									assertEdgeN( edge_split->p1, edge_p1p2 );
									// edge_p1p2_neighbouringElement->push_back(edge_split); 
									assertEdgeN( (edge_p1p2_flipped?edge_p1p2->p0:edge_p1p2->p1), edge_split );

									break;
								}
							}
							assert( edge_p0p2 );
							assert( edge_p1p2 );
							assert( edge_p0p2_neighbouringElement );
							assert( edge_p1p2_neighbouringElement );

							assertBadNeighbours(edge_p0p2);
							assertBadNeighbours(edge_p1p2);

							// Copy neighbours from existing edge
							vector<tuple<vector<EdgeTriNode*>*, EdgeTriNode*, ushort>> edge_neighbours; // neighbours: {source, destination_node, point}

							edge_neighbours.push_back(make_tuple( (newEdge->flippedEdge? &e_edge->p0_edges : &e_edge->p1_edges), edge, edge->p1 ));
							edge_neighbours.push_back(make_tuple( (newEdge->flippedEdge? &e_edge->p1_edges : &e_edge->p0_edges), edge_end, edge_end->p0 ));
							edge_neighbours.push_back(make_tuple( edge_p1p2_neighbouringElement, edge_split, edge_split->p1 ));
							int j=0;
							for ( auto neighbouringElement : edge_neighbours ) {
								for ( auto neighbour : (*get<0>(neighbouringElement)) ) {
									EdgeTriNode* this_edge = get<1>(neighbouringElement);
									ushort pt = get<2>(neighbouringElement);
									(this_edge->p0 == pt ? this_edge->p0_edges : this_edge->p1_edges).push_back( neighbour );
									(neighbour->p0 == pt ? neighbour->p0_edges : neighbour->p1_edges).push_back( this_edge );

									string srcEl;
									string dstEl;
									if (j==0) { srcEl = "e_edge_neighbour"; dstEl = "edge(p1)"; }
									else if (j==1) { srcEl = "e_edge_neighbour"; dstEl = "edge_end(p0)"; }
									else { srcEl = "p1p2"; dstEl = "edge_split(p1)"; }
									Log(str(format("			Neighboured: %1% {%2%,%3%} w/ %4% {%5%,%6%}")% 
												srcEl % neighbour->p0 % neighbour->p1 % dstEl % this_edge->p0 % this_edge->p1 ));

									assertBadNeighbours( this_edge );
									assertBadNeighbours( neighbour );
								}
								++j;
							}
							edge_split->p1_edges.push_back(edge_p1p2);
							edge_p1p2_neighbouringElement->push_back(edge_split);

							Log(str(format("			Neighboured: EdgeSplit(p1) {%1%,%2%} w/ edge_p1p2 {%3%,%4%}")% 
										edge_split->p0 % edge_split->p1 % edge_p1p2->p0 % edge_p1p2->p1 ));
							Log(str(format("			Neighboured: edge_p1p2 {%1%,%2%} w/ EdgeSplit {%3%,%4%}")% 
										edge_p1p2->p0 % edge_p1p2->p1 % edge_split->p0 % edge_split->p1 ));


							/* TODO: NOTE e_edge is being deleted...is this necessary???
							// Re-attach e_edge; this has been disconnected from its neighbours, but still remains attached.
							// Hence it needs to detach from the neighbour of its resized point, and re-attached to the
							// neighbour of its untouched point
							if ( newEdge->flippedEdge ) {

							} else {
							// Detach p1_edges; re-attach edge_split/edge_end
							e_edge->p1_edges.clear();
							e_edge->p1_edges.push_back( edge_split );
							e_edge->p1_edges.push_back( edge_end );

							// TODO: re-attach to p0_edges (those which are not edge_split/edge_end/edge)
							for ( auto neighbour : e_edge->p0_edges ) {
							if ( neighbour == edge_split || neighbour == 
							}
							e_edge->p0_edges.push_back( 
							}
							 */

							assertBadNeighbours(edge);
							assertBadNeighbours(edge_end);
							assertBadNeighbours(edge_split);
							assertBadNeighbours(edge_end);
							assertBadNeighbours(edge_p0p2);
							assertBadNeighbours(edge_p1p2);


							// attach to matching edges with p0
							vector<EdgeChunk*> p0Containers = EdgeTriNode::getContainer( p0 ); // TODO: p0Container is last in containers?
							EdgeChunk* p0Container = p0Containers.front(); // NOTE: each container here will contain the same set of edges on the seam as the first one
							Log(str(format("			Searching for Connections for {%1%,%2%}(p0)")% p0 % p1));
							for ( auto some_edge : p0Container->edges ) {
								if ( some_edge == edge_end ||
									 some_edge == edge_split ||
									 some_edge == edge_p0p2 ||
									 some_edge == edge_p1p2 ) continue;
								if ( some_edge->p0 == p0 ) {
									some_edge->p0_edges.push_back( edge );
									assertEdgeN(some_edge->p0, edge);
									edge->p0_edges.push_back( some_edge );
									assertEdgeN(edge->p0, some_edge);
									assertBadNeighbours(edge);
									assertBadNeighbours(some_edge);
									Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
								} else if ( some_edge->p1 == p0 ) {
									some_edge->p1_edges.push_back( edge );
									assertEdgeN(some_edge->p1, edge);
									edge->p0_edges.push_back( some_edge );
									assertEdgeN(edge->p0, some_edge);
									assertBadNeighbours(edge);
									assertBadNeighbours(some_edge);
									Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
								}
							}
							assertBadNeighbours(edge);


							// Add Edges (containers)
							// --------------------------
							// NOTE: do not need to add edge end to edges; since the triangle we're adding only fits on edge
							// and not edge_end
							vector<EdgeTriNode*> edges_containering; // edges in which we need to add to their respective containers
							edges_containering.push_back( edge_end );
							edges_containering.push_back( edge_split );
							edges_containering.push_back( edge );
							for ( auto edgeToContainer : edges_containering ) {
								vector<EdgeChunk*> edgeContainers = edgeToContainer->getContainers();
								for ( auto container : edgeContainers ) {
									container->edges.push_back( edgeToContainer );
								}
								journalEntry->operations.push_back( JournalEntry::AddEdge( 
											edgeToContainer->p0, edgeToContainer->p1 ) );
							}


							// Subdivide Existing iTriangle
							// ----------------------------

							// Subdivide (lower tri): {ep0->p0,ep1->p0}
							Log(str(format("	Subdivided Tri: <%1%, %2%, %3%>, <%4%, %5%, %6%>, <%7%, %8%, %9%>") %
										terrain->vertexBuffer[p2].v_x % terrain->vertexBuffer[p2].v_y % terrain->vertexBuffer[p2].v_z %
										vp0.v_x % vp0.v_y % vp0.v_z %
										e_p0.v_x % e_p0.v_y % e_p0.v_z ));
							e_tri->chunk->triangleBuffer.push_back( iTriangle( (newEdge->flippedEdge? e_edge->p1 : e_edge->p0), p2, p0 ) );
							ushort subTri_outi = e_tri->chunk->triangleBuffer.size() - 1;
							Tri* subTri_out = new Tri( e_tri->chunk, subTri_outi );
							Tri::assertBadTri(subTri_out);

							journalEntry->operations.push_back( JournalEntry::AddTri(
										subTri_outi, e_tri->chunk->id,
										subTri_out->p0, subTri_out->p1, subTri_out->p2 ) );

							// Subdivide (lower tri): {p0->p1} 
							// NOTE: using the same existing Tri since we refer to Triangles by their index, we do NOT want
							// to remove a triangle ever from the list

							ushort e_tri_oldp0 = e_tri->p0,
								   e_tri_oldp1 = e_tri->p1,
								   e_tri_oldp2 = e_tri->p2;
							Log(str(format("	About to reshape tri: <%1%,%2%,%3%>")%e_tri->p0%e_tri->p1%e_tri->p2));
							e_tri->reshapeTriOnEdge( (newEdge->flippedEdge? e_edge->p1 : e_edge->p0), p1, p0, p1 );
							journalEntry->operations.push_back( JournalEntry::ReshapeTri( e_tri->triIndex, e_tri->chunk->id,
										e_tri_oldp0, e_tri_oldp1, e_tri_oldp2,
										e_tri->p0, e_tri->p1, e_tri->p2 ) );



							// Attach Tri's to Edges
							// -----------------------

							// Attach (lower tri)
							assert(!edge_end->triangle_p1p0);
							assert(!edge_split->triangle_p1p0);
							edge_end->triangle_p1p0 = subTri_out;
							edge_split->triangle_p1p0 = subTri_out;

							if ( (subTri_out->p0 == edge_p0p2->p0 && subTri_out->p1 == edge_p0p2->p1) ||
								 (subTri_out->p1 == edge_p0p2->p0 && subTri_out->p2 == edge_p0p2->p1) ||
								 (subTri_out->p2 == edge_p0p2->p0 && subTri_out->p0 == edge_p0p2->p1) ) {
								assert(!edge_p0p2->triangle_p0p1);
								edge_p0p2->triangle_p0p1 = subTri_out;
							} else if ( (subTri_out->p0 == edge_p0p2->p1 && subTri_out->p1 == edge_p0p2->p0) ||
								 		(subTri_out->p1 == edge_p0p2->p1 && subTri_out->p2 == edge_p0p2->p0) ||
								 		(subTri_out->p2 == edge_p0p2->p1 && subTri_out->p0 == edge_p0p2->p0) ) {
								assert(!edge_p0p2->triangle_p1p0);
								edge_p0p2->triangle_p1p0 = subTri_out;
							} else {
								assert(false);
								Tri** edge_p0p2_subTriSide = (edge_p0p2_flipped? &edge_p0p2->triangle_p1p0 : &edge_p0p2->triangle_p0p1 );
								assert(!(*edge_p0p2_subTriSide));
								(*edge_p0p2_subTriSide) = subTri_out;
							}

							// Attach (upper tri)
							assert(!edge->triangle_p1p0);
							assert(!edge_split->triangle_p0p1);
							edge->triangle_p1p0 = e_tri;
							edge_split->triangle_p0p1 = e_tri;

							if ( (e_tri->p0 == edge_p1p2->p0 && e_tri->p1 == edge_p1p2->p1) ||
								 (e_tri->p1 == edge_p1p2->p0 && e_tri->p2 == edge_p1p2->p1) ||
								 (e_tri->p2 == edge_p1p2->p0 && e_tri->p0 == edge_p1p2->p1) ) {
								assert(!edge_p1p2->triangle_p0p1 || edge_p1p2->triangle_p0p1 == e_tri);
								edge_p1p2->triangle_p0p1 = e_tri;
							} else if ( (e_tri->p0 == edge_p1p2->p1 && e_tri->p1 == edge_p1p2->p0) ||
								 		(e_tri->p1 == edge_p1p2->p1 && e_tri->p2 == edge_p1p2->p0) ||
								 		(e_tri->p2 == edge_p1p2->p1 && e_tri->p0 == edge_p1p2->p0) ) {
								assert(!edge_p1p2->triangle_p1p0 || edge_p1p2->triangle_p1p0 == e_tri);
								edge_p1p2->triangle_p1p0 = e_tri;
							} else {
								assert(false);
								Tri** edge_p1p2_triSide = (edge_p1p2_flipped? &edge_p1p2->triangle_p1p0 : &edge_p1p2->triangle_p0p1 );
								assert(!(*edge_p1p2_triSide));
								(*edge_p1p2_triSide) = e_tri;
							}

							assertBadNeighbours(edge_end);
							assertBadNeighbours(edge_split);
							assertBadNeighbours(edge_p1p2);
							assertBadNeighbours(edge_p0p2);

							// Neighbour the Tri's
							// ------------------------
							// NOTE: tri does not need to re-neighbour with the bottom since its only been resized from the
							// top

							// Neighbour subTri_out w/ bottom
							ushort ref_ep0 = (newEdge->flippedEdge?e_edge->p1:e_edge->p0);
							Tri** neighbourTri_ep0p2 = e_tri->getNeighbourOnEdge( p0, p2 );
							if ( (*neighbourTri_ep0p2) ) {
								(*subTri_out->getNeighbourOnEdge(ref_ep0, p2)) = (*neighbourTri_ep0p2);
								(*(*neighbourTri_ep0p2)->getNeighbourOnEdge( ref_ep0, p2 )) = subTri_out;
								Tri::assertBadTri(subTri_out);
							}

							// Neighbour subTri_out w/ tri
							(*neighbourTri_ep0p2) = subTri_out;
							(*subTri_out->getNeighbourOnEdge(p0, p2)) = e_tri;
							Tri::assertBadTri(subTri_out);
							Tri::assertBadTri(e_tri);

							// TODO: using reference Tri**, however we rewrite the top Tri neighbour in e_tri after setting
							// subTri to e_tri's neighbour. Will this affect subTri's neighbouring?

							Log(str(format("	Deleting e_edge {%1%,%2%}")%e_edge->p0%e_edge->p1));
							journalEntry->operations.push_back( JournalEntry::RemoveEdge( e_edge->p0, e_edge->p1 ) );
							delete e_edge;
						} else {
							// Case 3 (3 cuts); ep0->p0, p0->p1, p1->ep1

							Log(str(format("		Subdivision (Case 3)")));
							Tri* e_tri = (newEdge->flippedEdge? e_edge->triangle_p0p1 : e_edge->triangle_p1p0);
							Vertex evp0 = terrain->vertexBuffer[e_tri->p0];
							Vertex evp1 = terrain->vertexBuffer[e_tri->p1];
							Vertex evp2 = terrain->vertexBuffer[e_tri->p2];
							ushort p2 = e_tri->getOddPoint(e_edge->p0, e_edge->p1);
							Vertex vp2 = terrain->vertexBuffer[p2];
							Log(str(format("			p2 {%1%}: <%2%, %3%, %4%>") % p2 % vp2.v_x % vp2.v_y % vp2.v_z));

							// Create edge for end segments
							EdgeTriNode* edge_top = new EdgeTriNode();
							edge_top->p0 = p1;
							edge_top->p1 = (newEdge->flippedEdge? e_edge->p0 : e_edge->p1);

							EdgeTriNode* edge_bot = new EdgeTriNode();
							edge_bot->p0 = (newEdge->flippedEdge? e_edge->p1 : e_edge->p0);
							edge_bot->p1 = p0;

							// Create split edges
							EdgeTriNode* edge_topsplit = new EdgeTriNode();
							edge_topsplit->p0 = p1;
							edge_topsplit->p1 = p2;

							EdgeTriNode* edge_botsplit = new EdgeTriNode();
							edge_botsplit->p0 = p0;
							edge_botsplit->p1 = p2;

							// Connect Edges
							// ---------------
							// Neighbour all edges
							edge_top->p0_edges.push_back(edge_topsplit);
							assertEdgeN( edge_top->p0, edge_topsplit );
							edge_top->p0_edges.push_back(edge);
							assertEdgeN( edge_top->p0, edge );
							Log(str(format("			Neighboured: EdgeTop(p0) {%1%,%2%} w/ EdgeTopSplit {%3%,%4%}")% 
										edge_top->p0 % edge_top->p1 % edge_topsplit->p0 % edge_topsplit->p1 ));
							Log(str(format("			Neighboured: EdgeTop(p0) {%1%,%2%} w/ Edge {%3%,%4%}")% 
										edge_top->p0 % edge_top->p1 % edge->p0 % edge->p1 ));

							edge_topsplit->p0_edges.push_back(edge);
							assertEdgeN( edge_topsplit->p0, edge );
							edge_topsplit->p0_edges.push_back(edge_top);
							assertEdgeN( edge_topsplit->p0, edge_top );
							Log(str(format("			Neighboured: EdgeTopSplit(p0) {%1%,%2%} w/ Edge {%3%,%4%}")% 
										edge_topsplit->p0 % edge_topsplit->p1 % edge->p0 % edge->p1 ));
							Log(str(format("			Neighboured: EdgeTopSplit(p0) {%1%,%2%} w/ EdgeTop {%3%,%4%}")% 
										edge_topsplit->p0 % edge_topsplit->p1 % edge_top->p0 % edge_top->p1 ));

							edge->p1_edges.push_back(edge_topsplit);
							assertEdgeN( edge->p1, edge_topsplit );
							edge->p1_edges.push_back(edge_top);
							assertEdgeN( edge->p1, edge_top );
							Log(str(format("			Neighboured: Edge(p1) {%1%,%2%} w/ EdgeTopSplit {%3%,%4%}")% 
										edge->p0 % edge->p1 % edge_topsplit->p0 % edge_topsplit->p1 ));
							Log(str(format("			Neighboured: Edge(p1) {%1%,%2%} w/ EdgeTop {%3%,%4%}")% 
										edge->p0 % edge->p1 % edge_top->p0 % edge_top->p1 ));

							edge_bot->p1_edges.push_back(edge);
							assertEdgeN( edge_bot->p1, edge );
							edge_bot->p1_edges.push_back(edge_botsplit);
							assertEdgeN( edge_bot->p1, edge_botsplit );
							Log(str(format("			Neighboured: EdgeBot(p1) {%1%,%2%} w/ Edge {%3%,%4%}")% 
										edge_bot->p0 % edge_bot->p1 % edge->p0 % edge->p1 ));
							Log(str(format("			Neighboured: EdgeBot(p1) {%1%,%2%} w/ EdgeBotSplit {%3%,%4%}")% 
										edge_bot->p0 % edge_bot->p1 % edge_botsplit->p0 % edge_botsplit->p1 ));

							edge_botsplit->p0_edges.push_back(edge);
							assertEdgeN( edge_botsplit->p0, edge );
							edge_botsplit->p0_edges.push_back(edge_bot);
							assertEdgeN( edge_botsplit->p0, edge_bot );
							Log(str(format("			Neighboured: EdgeBotSplit(p0) {%1%,%2%} w/ Edge {%3%,%4%}")% 
										edge_botsplit->p0 % edge_botsplit->p1 % edge->p0 % edge->p1 ));
							Log(str(format("			Neighboured: EdgeBotSplit(p0) {%1%,%2%} w/ EdgeBot {%3%,%4%}")% 
										edge_botsplit->p0 % edge_botsplit->p1 % edge_bot->p0 % edge_bot->p1 ));

							edge->p0_edges.push_back(edge_botsplit);
							assertEdgeN( edge->p0, edge_botsplit );
							edge->p0_edges.push_back(edge_bot);
							assertEdgeN( edge->p0, edge_bot );
							Log(str(format("			Neighboured: Edge(p0) {%1%,%2%} w/ EdgeBotSplit {%3%,%4%}")% 
										edge->p0 % edge->p1 % edge_botsplit->p0 % edge_botsplit->p1 ));
							Log(str(format("			Neighboured: Edge(p0) {%1%,%2%} w/ EdgeBot {%3%,%4%}")% 
										edge->p0 % edge->p1 % edge_bot->p0 % edge_bot->p1 ));

							// edge_topsplit->p1_edges.push_back(edge_botsplit); // skip this since it'll be added in
							// the neighbouring below
							// assertEdgeN( edge_topsplit->p1, edge_botsplit );
							// edge_botsplit->p1_edges.push_back(edge_topsplit);
							// assertEdgeN( edge_botsplit->p1, edge_topsplit );
							// Log(str(format("			Neighboured: EdgeTopSplit(p1) {%1%,%2%} w/ EdgeBotSplit {%3%,%4%}")% 
							// 			edge_topsplit->p0 % edge_topsplit->p1 % edge_botsplit->p0 % edge_botsplit->p1 ));
							// Log(str(format("			Neighboured: EdgeBotSplit(p1) {%1%,%2%} w/ EdgeTopSplit {%3%,%4%}")% 
							// 			edge_botsplit->p0 % edge_botsplit->p1 % edge_topsplit->p0 % edge_topsplit->p1 ));

							assertBadNeighbours(edge);
							assertBadNeighbours(edge_top);
							assertBadNeighbours(edge_topsplit);
							assertBadNeighbours(edge_bot);
							assertBadNeighbours(edge_botsplit);

							// Find p0p2/p1p2 edge to help in re-neighbouring the edge_split edge
							EdgeTriNode* edge_p0p2 = 0;
							EdgeTriNode* edge_p1p2 = 0;
							bool edge_p0p2_flipped = false;
							bool edge_p1p2_flipped = false;
							ushort edge_p0_expected = (newEdge->flippedEdge? e_edge->p1 : e_edge->p0);
							ushort edge_p1_expected = (newEdge->flippedEdge? e_edge->p0 : e_edge->p1);
							vector<EdgeTriNode*>* edge_p0p2_neighbouringElement = 0;
							vector<EdgeTriNode*>* edge_p1p2_neighbouringElement = 0;
							for ( auto neighbourEdge : (newEdge->flippedEdge? e_edge->p1_edges : e_edge->p0_edges) ) {
								if ( !edge_p0p2 && Tri::oneOrTheOther( neighbourEdge->p0, neighbourEdge->p1, edge_p0_expected, p2 ) ) {
									edge_p0p2 = neighbourEdge;
									edge_p0p2_flipped = (neighbourEdge->p1 == edge_p0_expected);
									(*(edge_p0p2_flipped? &edge_p0p2->triangle_p0p1 : &edge_p0p2->triangle_p1p0)) = 0; // unattach triangle (to-be changed)
									edge_p0p2_neighbouringElement = ( edge_p0p2_flipped ? &edge_p0p2->p0_edges : &edge_p0p2->p1_edges );

									Log(str(format("			Found edge_p0p2 {%1%,%2%} Flipped/NeighbouringEl: [%3%](%4%)")%
												neighbourEdge->p0 % neighbourEdge->p1 % (edge_p0p2_flipped?"Flipped":"NotFlipped") % (edge_p0p2_flipped?"p0":"p1")));

									// edge_botsplit->p1_edges.push_back(edge_p0p2);
									// assertEdgeN( edge_botsplit->p1, edge_p0p2 );
									// edge_topsplit->p1_edges.push_back(edge_p0p2);
									// assertEdgeN( edge_topsplit->p1, edge_p0p2 );

									// edge_p0p2_neighbouringElement->push_back(edge_botsplit);
									// assertEdgeN( (edge_p0p2_flipped?edge_p0p2->p0:edge_p0p2->p1), edge_botsplit );
									// edge_p0p2_neighbouringElement->push_back(edge_topsplit); 
									// assertEdgeN( (edge_p0p2_flipped?edge_p0p2->p0:edge_p0p2->p1), edge_topsplit );

									break;
								}
							}
							for ( auto neighbourEdge : (newEdge->flippedEdge? e_edge->p0_edges : e_edge->p1_edges) ) {
								if ( !edge_p1p2 && Tri::oneOrTheOther( neighbourEdge->p0, neighbourEdge->p1, edge_p1_expected, p2 ) ) {
									edge_p1p2 = neighbourEdge;
									edge_p1p2_flipped = (neighbourEdge->p0 == edge_p1_expected); // FIXME: changed from neighbourEdge->p1 (should be p0?)
									(*(edge_p1p2_flipped? &edge_p1p2->triangle_p0p1 : &edge_p1p2->triangle_p1p0)) = 0; // unattach triangle (to-be changed)
									edge_p1p2_neighbouringElement = ( edge_p1p2_flipped ? &edge_p1p2->p1_edges : &edge_p1p2->p0_edges ); // FIXME: switch p1_edges and p0_edges ?

									Log(str(format("			Found edge_p1p2 {%1%,%2%} Flipped/NeighbouringEl: [%3%](%4%)")%
												neighbourEdge->p0 % neighbourEdge->p1 % (edge_p1p2_flipped?"Flipped":"NotFlipped") % (edge_p1p2_flipped?"p0":"p1")));

									// edge_botsplit->p1_edges.push_back(edge_p1p2);
									// assertEdgeN( edge_botsplit->p1, edge_p1p2 );
									// edge_topsplit->p1_edges.push_back(edge_p1p2);
									// assertEdgeN( edge_topsplit->p1, edge_p1p2 );

									// edge_p1p2_neighbouringElement->push_back(edge_botsplit);
									// assertEdgeN( (edge_p1p2_flipped?edge_p1p2->p0:edge_p1p2->p1), edge_botsplit );
									// edge_p1p2_neighbouringElement->push_back(edge_topsplit); 
									// assertEdgeN( (edge_p1p2_flipped?edge_p1p2->p0:edge_p1p2->p1), edge_topsplit );

									break;
								}
							}
							assert( edge_p0p2 );
							assert( edge_p1p2 );
							assert( edge_p0p2_neighbouringElement );
							assert( edge_p1p2_neighbouringElement );

							assertBadNeighbours(edge_p1p2);
							assertBadNeighbours(edge_p0p2);
							assertBadNeighbours(edge_topsplit);
							assertBadNeighbours(edge_botsplit);


							// Copy neighbours from existing edge
							vector<tuple<vector<EdgeTriNode*>*, EdgeTriNode*, ushort>> edge_neighbours; // neighbours: {source, destination_node, point}
							edge_neighbours.push_back(make_tuple( (newEdge->flippedEdge? &e_edge->p1_edges : &e_edge->p0_edges), edge_bot, edge_bot->p0 ));
							edge_neighbours.push_back(make_tuple( (newEdge->flippedEdge? &e_edge->p0_edges : &e_edge->p1_edges), edge_top, edge_top->p1 ));
							edge_neighbours.push_back(make_tuple( edge_p0p2_neighbouringElement, edge_botsplit, edge_botsplit->p1 ));
							edge_neighbours.push_back(make_tuple( edge_p1p2_neighbouringElement, edge_topsplit, edge_topsplit->p1 ));

							int j=0;
							for ( auto neighbouringElement : edge_neighbours ) {
								for ( auto neighbour : (*get<0>(neighbouringElement)) ) {
									EdgeTriNode* this_edge = get<1>(neighbouringElement);
									ushort pt = get<2>(neighbouringElement);
									(this_edge->p0 == pt ? this_edge->p0_edges : this_edge->p1_edges).push_back( neighbour );
									(neighbour->p0 == pt ? neighbour->p0_edges : neighbour->p1_edges).push_back( this_edge );

									string srcEl;
									string dstEl;
									if (j==0) { srcEl = "e_edge_neighbour"; dstEl = "edge_bot(p0)"; }
									else if (j==1) { srcEl = "e_edge_neighbour"; dstEl = "edge_top(p1)"; }
									else if (j==2) { srcEl = "p0p2"; dstEl = "edge_botsplit(p1)"; }
									else { srcEl = "p1p2"; dstEl = "edge_topsplit(p1)"; }
									Log(str(format("			Neighboured: %1% {%2%,%3%} w/ %4% {%5%,%6%}")% 
												srcEl % neighbour->p0 % neighbour->p1 % dstEl % this_edge->p0 % this_edge->p1 ));

									assertBadNeighbours( this_edge );
									assertBadNeighbours( neighbour );
								}
								++j;
							}
							edge_botsplit->p1_edges.push_back(edge_p0p2);
							edge_p0p2_neighbouringElement->push_back(edge_botsplit);
							edge_topsplit->p1_edges.push_back(edge_p1p2);
							edge_p1p2_neighbouringElement->push_back(edge_topsplit); 

							Log(str(format("			Neighboured: EdgeBotSplit(p1) {%1%,%2%} w/ edge_p0p2 {%3%,%4%}")% 
										edge_botsplit->p0 % edge_botsplit->p1 % edge_p0p2->p0 % edge_p0p2->p1 ));
							Log(str(format("			Neighboured: EdgeTopSplit(p1) {%1%,%2%} w/ edge_p1p2 {%3%,%4%}")% 
										edge_topsplit->p0 % edge_topsplit->p1 % edge_p1p2->p0 % edge_p1p2->p1 ));


							assertBadNeighbours(edge_bot);
							assertBadNeighbours(edge_top);
							assertBadNeighbours(edge_botsplit);
							assertBadNeighbours(edge_topsplit);
							assertBadNeighbours(edge_p0p2);
							assertBadNeighbours(edge_p1p2);

							// TODO: Is this necessary? It was previously left out..
							// attach to matching edges with p0
							vector<EdgeChunk*> p0Containers = EdgeTriNode::getContainer( p0 ); // TODO: p0Container is last in containers?
							EdgeChunk* p0Container = p0Containers.front(); // NOTE: first container contains all elements we're concerned with, including ALL elements on the seam
							Log(str(format("			Searching for Connections for {%1%,%2%}(p0)")% p0 % p1));
							for ( auto some_edge : p0Container->edges ) {
								if ( some_edge == edge_bot ||
									 some_edge == edge_top ||
									 some_edge == edge_topsplit ||
									 some_edge == edge_botsplit ||
									 some_edge == edge_p0p2 ||
									 some_edge == edge_p1p2 ) continue;
								if ( some_edge->p0 == p0 ) {
									some_edge->p0_edges.push_back( edge );
									assertEdgeN(some_edge->p0, edge);
									edge->p0_edges.push_back( some_edge );
									assertEdgeN(edge->p0, some_edge);
									assertBadNeighbours(edge);
									assertBadNeighbours(some_edge);
									Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
								} else if ( some_edge->p1 == p0 ) {
									some_edge->p1_edges.push_back( edge );
									assertEdgeN(some_edge->p1, edge);
									edge->p0_edges.push_back( some_edge );
									assertEdgeN(edge->p0, some_edge);
									assertBadNeighbours(edge);
									assertBadNeighbours(some_edge);
									Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
								}
							}
							assertBadNeighbours(edge);

							// attach to matching edges with p1
							vector<EdgeChunk*> p1Containers = EdgeTriNode::getContainer( p1 ); // TODO: p1Container is last in containers?
							EdgeChunk* p1Container = p1Containers.front();
							Log(str(format("			Searching for Connections for {%1%,%2%}(p1)")% p0 % p1));
							for ( auto some_edge : p1Container->edges ) {
								if ( some_edge == edge_bot ||
									 some_edge == edge_top ||
									 some_edge == edge_topsplit ||
									 some_edge == edge_botsplit ||
									 some_edge == edge_p0p2 ||
									 some_edge == edge_p1p2 ) continue;
								if ( some_edge->p0 == p1 ) {
									some_edge->p0_edges.push_back( edge );
									assertEdgeN(some_edge->p0, edge);
									edge->p1_edges.push_back( some_edge );
									assertEdgeN(edge->p1, some_edge);
									assertBadNeighbours(edge);
									assertBadNeighbours(some_edge);
									Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
								} else if ( some_edge->p1 == p1 ) {
									some_edge->p1_edges.push_back( edge );
									assertEdgeN(some_edge->p1, edge);
									edge->p1_edges.push_back( some_edge );
									assertEdgeN(edge->p1, some_edge);
									assertBadNeighbours(edge);
									assertBadNeighbours(some_edge);
									Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
								}
							}
							assertBadNeighbours(edge);



							// Add Edges (containers)
							// --------------------------
							// NOTE: do not need to add edge end to edges; since the triangle we're adding only fits on edge
							// and not edge_end
							vector<EdgeTriNode*> edges_containering; // edges in which we need to add to their respective containers
							edges_containering.push_back( edge_top );
							edges_containering.push_back( edge_bot );
							edges_containering.push_back( edge_botsplit );
							edges_containering.push_back( edge_topsplit );
							edges_containering.push_back( edge );
							for ( auto edgeToContainer : edges_containering ) {
								vector<EdgeChunk*> edgeContainers = edgeToContainer->getContainers();
								for ( auto container : edgeContainers ) {
									container->edges.push_back( edgeToContainer );
								}
								journalEntry->operations.push_back( JournalEntry::AddEdge( 
											edgeToContainer->p0, edgeToContainer->p1 ) );
							}


							// Subdivide Existing Triangle
							// ----------------------------

							// Subdivide (upper tri): {p1->ep1,p1->ep0}
							Log(str(format("	Subdivided Tri: <%1%, %2%, %3%>, <%4%, %5%, %6%>, <%7%, %8%, %9%>") %
										vp1.v_x % vp1.v_y % vp1.v_z %
										terrain->vertexBuffer[p2].v_x % terrain->vertexBuffer[p2].v_y % terrain->vertexBuffer[p2].v_z %
										(newEdge->flippedEdge?e_p0.v_x:e_p1.v_x) % (newEdge->flippedEdge?e_p0.v_y:e_p1.v_y) % (newEdge->flippedEdge?e_p0.v_z:e_p1.v_z) ));
							e_tri->chunk->triangleBuffer.push_back( iTriangle( p1, p2, (newEdge->flippedEdge? e_edge->p0 : e_edge->p1) ) );
							ushort subTri_topi = e_tri->chunk->triangleBuffer.size() - 1;
							Tri* subTri_top = new Tri( e_tri->chunk, subTri_topi );

							journalEntry->operations.push_back( JournalEntry::AddTri( 
										subTri_topi, e_tri->chunk->id,
										subTri_top->p0, subTri_top->p1, subTri_top->p2 ) );

							// Subdivide (lower tri): {ep0->p0,ep1->p0}
							Log(str(format("	Subdivided Tri: <%1%, %2%, %3%>, <%4%, %5%, %6%>, <%7%, %8%, %9%>") %
										(newEdge->flippedEdge?e_p1.v_x:e_p0.v_x) % (newEdge->flippedEdge?e_p1.v_y:e_p0.v_y) % (newEdge->flippedEdge?e_p1.v_z:e_p0.v_z) %
										terrain->vertexBuffer[p2].v_x % terrain->vertexBuffer[p2].v_y % terrain->vertexBuffer[p2].v_z %
										vp0.v_x % vp0.v_y % vp0.v_z));
							e_tri->chunk->triangleBuffer.push_back( iTriangle( (newEdge->flippedEdge? e_edge->p1 : e_edge->p0), p2, p0 ) );
							ushort subTri_boti = e_tri->chunk->triangleBuffer.size() - 1;
							Tri* subTri_bot = new Tri( e_tri->chunk, subTri_boti );

							journalEntry->operations.push_back( JournalEntry::AddTri( 
										subTri_boti, e_tri->chunk->id,
										subTri_bot->p0, subTri_bot->p1, subTri_bot->p2 ) );

							// Subdivide (mid tri): {p0->p1} 
							// NOTE: using the same existing Tri since we refer to Triangles by their index, we do NOT want
							// to remove a triangle ever from the list
							Log(str(format("	Reshaping Tri: <%1%, %2%, %3%>, <%4%, %5%, %6%>") %
										vp0.v_x % vp0.v_y % vp0.v_z % vp1.v_x % vp1.v_y % vp1.v_z ));

							ushort e_tri_oldp0 = e_tri->p0,
								   e_tri_oldp1 = e_tri->p1,
								   e_tri_oldp2 = e_tri->p2;
							Log(str(format("	About to reshape tri: <%1%,%2%,%3%>")%e_tri->p0%e_tri->p1%e_tri->p2));
							e_tri->reshapeTriOnEdge( (newEdge->flippedEdge? e_edge->p1 : e_edge->p0), (newEdge->flippedEdge? e_edge->p0 : e_edge->p1), p0, p1 );
							journalEntry->operations.push_back( JournalEntry::ReshapeTri( e_tri->triIndex, e_tri->chunk->id,
										e_tri_oldp0, e_tri_oldp1, e_tri_oldp2,
										e_tri->p0, e_tri->p1, e_tri->p2 ) );


							// Attach Tri's to Edges
							// -----------------------

							// Attach (upper tri)
							edge_top->triangle_p1p0 = subTri_top;
							edge_topsplit->triangle_p0p1 = subTri_top;
							Tri** edge_p1p2_subTriSide = (edge_p1p2_flipped? &edge_p1p2->triangle_p1p0 : &edge_p1p2->triangle_p0p1 );
							(*edge_p1p2_subTriSide) = subTri_top;
							assert( ( edge_p1p2->triangle_p0p1 == subTri_top && (
										( edge_p1p2->p0 == subTri_top->p0 && edge_p1p2->p1 == subTri_top->p1 ) ||
										( edge_p1p2->p0 == subTri_top->p1 && edge_p1p2->p1 == subTri_top->p2 ) ||
										( edge_p1p2->p0 == subTri_top->p2 && edge_p1p2->p1 == subTri_top->p0 ) ) ) ||
									( edge_p1p2->triangle_p1p0 == subTri_top && (
										( edge_p1p2->p1 == subTri_top->p0 && edge_p1p2->p0 == subTri_top->p1 ) ||
										( edge_p1p2->p1 == subTri_top->p1 && edge_p1p2->p0 == subTri_top->p2 ) ||
										( edge_p1p2->p1 == subTri_top->p2 && edge_p1p2->p0 == subTri_top->p0 ) ) ) );

							// Attach (lower tri)
							edge_bot->triangle_p1p0 = subTri_bot;
							edge_botsplit->triangle_p1p0 = subTri_bot;
							Tri** edge_p0p2_subTriSide = (edge_p0p2_flipped? &edge_p0p2->triangle_p1p0 : &edge_p0p2->triangle_p0p1 );
							(*edge_p0p2_subTriSide) = subTri_bot;
							assert( ( edge_p0p2->triangle_p0p1 == subTri_bot && (
										( edge_p0p2->p0 == subTri_bot->p0 && edge_p0p2->p1 == subTri_bot->p1 ) ||
										( edge_p0p2->p0 == subTri_bot->p1 && edge_p0p2->p1 == subTri_bot->p2 ) ||
										( edge_p0p2->p0 == subTri_bot->p2 && edge_p0p2->p1 == subTri_bot->p0 ) ) ) ||
									( edge_p0p2->triangle_p1p0 == subTri_bot && (
										( edge_p0p2->p1 == subTri_bot->p0 && edge_p0p2->p0 == subTri_bot->p1 ) ||
										( edge_p0p2->p1 == subTri_bot->p1 && edge_p0p2->p0 == subTri_bot->p2 ) ||
										( edge_p0p2->p1 == subTri_bot->p2 && edge_p0p2->p0 == subTri_bot->p0 ) ) ) );

							// Attach (middle tri)
							assert(!edge->triangle_p1p0);
							assert(!edge_botsplit->triangle_p0p1);
							assert(!edge_topsplit->triangle_p1p0);
							edge->triangle_p1p0 = e_tri;
							edge_botsplit->triangle_p0p1 = e_tri;
							edge_topsplit->triangle_p1p0 = e_tri;

							assertBadNeighbours(edge);
							assertBadNeighbours(edge_top);
							assertBadNeighbours(edge_bot);
							assertBadNeighbours(edge_topsplit);
							assertBadNeighbours(edge_botsplit);
							assertBadNeighbours(edge_p1p2);
							assertBadNeighbours(edge_p0p2);


							// Neighbour the Tri's
							// ------------------------

							// Neighbour subTri_top w/ top
							ushort ref_ep1 = (newEdge->flippedEdge?e_edge->p0:e_edge->p1);
							Tri** neighbourTri_ep1p2 = e_tri->getNeighbourOnEdge( p1, p2 );
							if ( (*neighbourTri_ep1p2) ) {
								(*subTri_top->getNeighbourOnEdge(ref_ep1, p2)) = (*neighbourTri_ep1p2);
								(*(*neighbourTri_ep1p2)->getNeighbourOnEdge( ref_ep1, p2 )) = subTri_top;
							}

							// Neighbour subTri_top w/ tri
							(*neighbourTri_ep1p2) = subTri_top;
							(*subTri_top->getNeighbourOnEdge(p1, p2)) = e_tri;


							// Neighbour subTri_bot w/ bot
							ushort ref_ep0 = (newEdge->flippedEdge?e_edge->p1:e_edge->p0);
							Tri** neighbourTri_ep0p2 = e_tri->getNeighbourOnEdge( p0, p2 );
							if ( (*neighbourTri_ep0p2) ) {
								(*subTri_bot->getNeighbourOnEdge(ref_ep0, p2)) = (*neighbourTri_ep0p2);
								(*(*neighbourTri_ep0p2)->getNeighbourOnEdge( ref_ep0, p2 )) = subTri_bot;
							}

							// Neighbour subTri_bot w/ tri
							(*neighbourTri_ep0p2) = subTri_bot;
							(*subTri_bot->getNeighbourOnEdge(p0, p2)) = e_tri;
							Tri::assertBadTri(subTri_top);
							Tri::assertBadTri(subTri_bot);
							Tri::assertBadTri(e_tri);

							// TODO: using reference Tri**, however we rewrite the top/bot Tri neighbour in e_tri after setting
							// subTri to e_tri's neighbour. Will this affect subTri's neighbouring?

							Log(str(format("	Deleting e_edge {%1%,%2%}")%e_edge->p0%e_edge->p1));
							journalEntry->operations.push_back( JournalEntry::RemoveEdge( e_edge->p0, e_edge->p1 ) );
							delete e_edge;
						}


						newEdge->flippedEdge = false; // TODO: we only need to know if the edge WAS flipped, but as far
						// as the caller is concerned, this new edge is NOT flipped
						break;
					} else if ( (whichCase = 3) && e_p0.between( vp0, vp1 ) && e_p1.between( vp0, vp1 ) ) {
						// Existing edge is a subdivided version of our new edge

						/* Subdivided edges along this edge
						 * There may be more edges which are along this edge, simply continue looping along the potential
						 * edges and add each. If we reach this, then there's no chance to enter the other 2 cases, and will
						 * only enter this case again. Note that its possible for there to be a discontinuous path between
						 * the edges which lay on this edge; hence we need to later check for holes and gaps on our edge and
						 * create new edges to fill in
						 */
						Log("	case3");
						Log(str(format("		MATCH: Subdivided version of our edge; (%1%)<%2%, %3%, %4%>, (%5%)<%6%, %7%, %8%>") %
									e_edge->p0 % e_p0.v_x % e_p0.v_y % e_p0.v_z %
									e_edge->p1 % e_p1.v_x % e_p1.v_y % e_p1.v_z ));
						pleaseFillHoles = true;

						// Include existing (subdivided) edge
						bool flippedEdge = (p0 == e_edge->p1 || p1 == e_edge->p0 || !e_p0.between(vp0, e_p1));
						InnerEdgeOfTri* e_newEdge = new InnerEdgeOfTri();
						e_newEdge->edge = e_edge;
						e_newEdge->flippedEdge = flippedEdge;

						// Insert in order along p0p1
						int i = 0;
						Vertex expected_ep0 = (flippedEdge? e_p1 : e_p0);
						Vertex expected_ep1 = (flippedEdge? e_p0 : e_p1);
						for ( auto otherEdge : edges ) {
							ushort expected_op0 = (otherEdge->flippedEdge ? otherEdge->edge->p1 : otherEdge->edge->p0);
							ushort expected_op1 = (otherEdge->flippedEdge ? otherEdge->edge->p0 : otherEdge->edge->p1);
							if ( chunk->terrain->vertexBuffer[expected_op0].between( expected_ep1, chunk->terrain->vertexBuffer[expected_op1] ) ) {
								break; // this edge comes before ith edge
							}
							++i;
						}
						edges.insert( edges.begin()+i, e_newEdge );

						// NOTE: DO NOT DO NOT DO NOT BREAK!!!!!
					}
				} catch(exception& e) {
					Log(str(format("Bug at %1%")%whichCase));
				}
			}

			if ( pleaseFillHoles ) {
				// pleaseFillHoles? find empty spaces between p0 and p1, add as separate edges & neighbour


				Log(str(format("	Please Fill Holes..")));
				// Find edges from end-points which match our end points; then neighbour edges
				vector<EdgeChunk*> p0Containers = EdgeTriNode::getContainer( p0 ); // TODO: p0Container is first in containers?
				vector<EdgeChunk*> p1Containers = EdgeTriNode::getContainer( p1 ); // TODO: p1Container is last in containers?
				EdgeChunk* p0Container = p0Containers.front();
				EdgeChunk* p1Container = p1Containers.front();

				ushort ip0 = p0;
				ushort edgeIndex = 0;
				while (ip0 != p1) {
					if ( edgeIndex >= edges.size() ) {
						// Gap (end): fill in (ip0,p1)
						// fill gap, attach edge

						EdgeTriNode* gap = new EdgeTriNode();
						gap->p0 = ip0;
						gap->p1 = p1;
						InnerEdgeOfTri* e_newEdge = new InnerEdgeOfTri();
						e_newEdge->edge = gap;
						e_newEdge->flippedEdge = false;
						Log(str(format("		Created Gap: {%1%,%2%}")% ip0 % p1));

						// attach p1
						EdgeTriNode* prevEdge = edges[edgeIndex-1]->edge;
						gap->p0_edges.push_back( prevEdge );
						assertEdgeN(gap->p0, prevEdge);
						(edges[edgeIndex-1]->flippedEdge? prevEdge->p0_edges : prevEdge->p1_edges).push_back(gap);
						assertEdgeN((edges[edgeIndex-1]->flippedEdge? prevEdge->p0 : prevEdge->p1), gap);
						Log(str(format("			Neighboured: Gap(p0) {%1%,%2%} w/ PrevEdge {%3%,%4%}")% 
									gap->p0 % gap->p1 % prevEdge->p0 % prevEdge->p1 ));
						Log(str(format("			Neighboured: prevEdge {%1%,%2%} w/ Gap {%3%,%4%}")% 
									prevEdge->p0 % prevEdge->p1 % gap->p0 % gap->p1 ));

						Log(str(format("			Searching for Connections for {%1%,%2%}(p1)")% ip0 % p1));
						for ( auto some_edge : p1Container->edges ) {
							if ( some_edge->p0 == p1 ) {
								some_edge->p0_edges.push_back( gap );
								assertEdgeN(some_edge->p0, gap);
								gap->p1_edges.push_back( some_edge );
								assertEdgeN(gap->p1, some_edge);
								Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
							} else if ( some_edge->p1 == p1 ) {
								some_edge->p1_edges.push_back( gap );
								assertEdgeN(some_edge->p1, gap);
								gap->p1_edges.push_back( some_edge );
								assertEdgeN(gap->p1, some_edge);
								Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
							}
						}

						assertBadNeighbours(gap);
						assertBadNeighbours(prevEdge);
						
						edges.insert( edges.begin()+edgeIndex, e_newEdge );
						++edgeIndex; // make up for added edge

						// Insert edge into container
						// TODO: efficiency, check each container in containers if this edge is within it
						vector<EdgeChunk*> gapContainers = gap->getContainers();
						for ( auto container : gapContainers ) {
							container->edges.push_back( gap );
						}

						journalEntry->operations.push_back( JournalEntry::AddEdge( gap->p0, gap->p1 ) );

						break;
					}

					EdgeTriNode* nextEdge = edges[edgeIndex]->edge;
					ushort expected_p0 = (edges[edgeIndex]->flippedEdge? nextEdge->p1 : nextEdge->p0);
					ushort expected_p1 = (edges[edgeIndex]->flippedEdge? nextEdge->p0 : nextEdge->p1);
					if ( ip0 != expected_p0 ) {
						// Gap: fill in (ip0,expected_p0)

						EdgeTriNode* gap = new EdgeTriNode();
						gap->p0 = ip0;
						gap->p1 = expected_p0;
						InnerEdgeOfTri* e_newEdge = new InnerEdgeOfTri();
						e_newEdge->edge = gap;
						e_newEdge->flippedEdge = false;
						Log(str(format("		Created Gap: {%1%,%2%}")% ip0 % expected_p0));

						// Attach to other edges
						if ( ip0 == p0 ) {
							// Search through ALL edges in p0Container for connection
							// Connected edge to graph
							Log(str(format("			Searching for Connections for {%1%,%2%}(p0)")% ip0 % expected_p0));
							for ( auto some_edge : p0Container->edges ) {
								if ( some_edge->p0 == p0 ) {
									some_edge->p0_edges.push_back( gap );
									assertEdgeN(some_edge->p0, gap);
									gap->p0_edges.push_back( some_edge );
									assertEdgeN(gap->p0, some_edge);
									assertBadNeighbours(some_edge);
									assertBadNeighbours(gap);
									Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
								} else if ( some_edge->p1 == p0 ) {
									some_edge->p1_edges.push_back( gap );
									assertEdgeN(some_edge->p1, gap);
									gap->p0_edges.push_back( some_edge );
									assertEdgeN(gap->p0, some_edge);
									assertBadNeighbours(some_edge);
									assertBadNeighbours(gap);
									Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
								}
							}
						} else {
							EdgeTriNode* prevEdge = edges[edgeIndex-1]->edge;
							gap->p0_edges.push_back( prevEdge );
							assertEdgeN(gap->p0, prevEdge);
							(edges[edgeIndex-1]->flippedEdge? prevEdge->p0_edges : prevEdge->p1_edges).push_back(gap);
							assertEdgeN((edges[edgeIndex-1]->flippedEdge? prevEdge->p0 : prevEdge->p1), gap);
							assertBadNeighbours(gap);
							assertBadNeighbours(prevEdge);
							Log(str(format("			Neighboured: Gap(p0) {%1%,%2%} w/ PrevEdge {%3%,%4%}")% 
										gap->p0 % gap->p1 % prevEdge->p0 % prevEdge->p1 ));
							Log(str(format("			Neighboured: prevEdge {%1%,%2%} w/ Gap {%3%,%4%}")% 
										prevEdge->p0 % prevEdge->p1 % gap->p0 % gap->p1 ));
						}
						gap->p1_edges.push_back( nextEdge );
						assertEdgeN(gap->p1, nextEdge);
						(edges[edgeIndex]->flippedEdge? nextEdge->p1_edges : nextEdge->p0_edges).push_back(gap);
						assertEdgeN((edges[edgeIndex]->flippedEdge? nextEdge->p1 : nextEdge->p0), gap);
						assertBadNeighbours(gap);
						assertBadNeighbours(nextEdge);
						Log(str(format("			Neighboured: Gap(p1) {%1%,%2%} w/ NextEdge {%3%,%4%}")% 
									gap->p0 % gap->p1 % nextEdge->p0 % nextEdge->p1 ));
						Log(str(format("			Neighboured: NextEdge {%1%,%2%} w/ Gap {%3%,%4%}")% 
									nextEdge->p0 % nextEdge->p1 % gap->p0 % gap->p1 ));


						edges.insert( edges.begin()+edgeIndex, e_newEdge );
						++edgeIndex; // make up for added edge

						// Insert edge into container
						// TODO: efficiency, check each container in containers if this edge is within it
						vector<EdgeChunk*> gapContainers = gap->getContainers();
						for ( auto container : gapContainers ) {
							container->edges.push_back( gap );
						}

						journalEntry->operations.push_back( JournalEntry::AddEdge( gap->p0, gap->p1 ) );
					} 

					ip0 = expected_p1;
					++edgeIndex;
				}
			}

			if ( edges.empty() ) {
				// edges empty? add new edge


				Log(str(format("	NO EDGES: Creating requested edge..")));
				// Find edges from end-points which match our end points; then neighbour edges
				vector<EdgeChunk*> p0Containers = EdgeTriNode::getContainer( p0 ); // TODO: p0Container is first in containers?
				vector<EdgeChunk*> p1Containers = EdgeTriNode::getContainer( p1 ); // TODO: p1Container is last in containers?
				EdgeChunk* p0Container = p0Containers.front();
				EdgeChunk* p1Container = p1Containers.front();


				EdgeTriNode* edge = new EdgeTriNode();
				edge->p0 = p0;
				edge->p1 = p1;
				InnerEdgeOfTri* e_newEdge = new InnerEdgeOfTri();
				e_newEdge->edge = edge;
				e_newEdge->flippedEdge = false;
				edges.push_back( e_newEdge );

				// Connected edge to graph
				Log(str(format("		Searching for Connections for {%1%,%2%}(p0)")% p0 % p1));
				for ( auto some_edge : p0Container->edges ) {
					if ( some_edge->p0 == p0 ) {
						some_edge->p0_edges.push_back( edge );
						assertEdgeN(some_edge->p0, edge);
						edge->p0_edges.push_back( some_edge );
						assertEdgeN(edge->p0, some_edge);
						assertBadNeighbours(edge);
						assertBadNeighbours(some_edge);
						Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
					} else if ( some_edge->p1 == p0 ) {
						some_edge->p1_edges.push_back( edge );
						assertEdgeN(some_edge->p1, edge);
						edge->p0_edges.push_back( some_edge );
						assertEdgeN(edge->p0, some_edge);
						assertBadNeighbours(edge);
						assertBadNeighbours(some_edge);
						Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
					}
				}
				Log(str(format("		Searching for Connections for {%1%,%2%}(p1)")% p0 % p1));
				for ( auto some_edge : p1Container->edges ) {
					if ( some_edge->p0 == p1 ) {
						some_edge->p0_edges.push_back( edge );
						assertEdgeN(some_edge->p0, edge);
						edge->p1_edges.push_back( some_edge );
						assertEdgeN(edge->p1, some_edge);
						assertBadNeighbours(edge);
						assertBadNeighbours(some_edge);
						Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
					} else if ( some_edge->p1 == p1 ) {
						some_edge->p1_edges.push_back( edge );
						assertEdgeN(some_edge->p1, edge);
						edge->p1_edges.push_back( some_edge );
						assertEdgeN(edge->p1, some_edge);
						assertBadNeighbours(edge);
						assertBadNeighbours(some_edge);
						Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
					}
				}
				assertBadNeighbours(edge);


				// Add edge to each chunk
				for ( auto container : containers ) {
					container->edges.push_back( edge );

					bool foundEdge = false;
					for ( auto e_edge : container->edges ) {
						if ( e_edge == edge ) {
							foundEdge = true;
							Log(str(format("		Edge has definitely been added to container: (%1%,%2%,%3%) [%4%]")%
										container->worldOffset.x % container->worldOffset.y % container->worldOffset.z %
										container ));
							break;
						}
					}
					if ( !foundEdge ) raise(SIGTRAP);
				}

				journalEntry->operations.push_back( JournalEntry::AddEdge( edge->p0, edge->p1 ) );

			}
		}

		struct InnerEdgeOfTri {
			EdgeTriNode* edge;
			bool flippedEdge; // flipped specified p0p1?
		};

		vector<InnerEdgeOfTri*> edges; // the edge(s) along this side of the original triangle
		vector<EdgeChunk*> containers; // containers for all edges
		JournalEntry* journalEntry; // the journal of adding this edge (later merged with other journal entries from the
									// other edges)
						// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
						// WARNING WARNING WARNING WARNING WARNING WARNING!
						// TODO: What if e_tri doesn't exist (triangle removed but edge has been left in place?)
						// WARNING WARNING WARNING WARNING WARNING WARNING!
						// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	};
	EdgeOfTri* triEdge_p0p1;
	EdgeOfTri* triEdge_p1p2;
	EdgeOfTri* triEdge_p2p0;

	// Initialize each EdgeOfTri
	iTriangle triangle = chunk->triangleBuffer[triIndex];
	assert( triangle.p0 != triangle.p1 );
	assert( triangle.p0 != triangle.p2 );
	assert( triangle.p1 != triangle.p2 );
	Vertex v0 = chunk->terrain->vertexBuffer[triangle.p0];
	Vertex v1 = chunk->terrain->vertexBuffer[triangle.p1];
	Vertex v2 = chunk->terrain->vertexBuffer[triangle.p2];

	Log(str(format("EdgeTriTree:: Adding Triangle: (%1%)<%2%, %3%, %4%>, (%5%)<%6%, %7%, %8%>, (%9%)<%10%, %11%, %12%>") %
				triangle.p0 % v0.v_x % v0.v_y % v0.v_z %
				triangle.p1 % v1.v_x % v1.v_y % v1.v_z %
				triangle.p2 % v2.v_x % v2.v_y % v2.v_z ));

	// Is this a proper triangle?
	/*
	 * NOTE: this has caused issues! Each determined line has turned out to be a necessary component of the terrain
	 * TODO: remove this? is this at all necessary?
	glm::vec3 v0v1 = glm::normalize( glm::vec3( v1.v_x - v0.v_x, v1.v_y - v0.v_y, v1.v_z - v0.v_z ) );
	glm::vec3 v0v2 = glm::normalize( glm::vec3( v2.v_x - v0.v_x, v2.v_y - v0.v_y, v2.v_z - v0.v_z ) );
	glm::vec3 v1v0 = glm::normalize( glm::vec3( v0.v_x - v1.v_x, v0.v_y - v1.v_y, v0.v_z - v1.v_z ) );
	glm::vec3 v1v2 = glm::normalize( glm::vec3( v2.v_x - v1.v_x, v2.v_y - v1.v_y, v2.v_z - v1.v_z ) );
	glm::vec3 v2v0 = glm::normalize( glm::vec3( v0.v_x - v2.v_x, v0.v_y - v2.v_y, v0.v_z - v2.v_z ) );
	glm::vec3 v2v1 = glm::normalize( glm::vec3( v1.v_x - v2.v_x, v1.v_y - v2.v_y, v1.v_z - v2.v_z ) );
	Vertex _v0v1 = Vertex( v0v1.x, v0v1.y, v0v1.z );
	Vertex _v0v2 = Vertex( v0v2.x, v0v2.y, v0v2.z );
	Vertex _v1v0 = Vertex( v1v0.x, v1v0.y, v1v0.z );
	Vertex _v1v2 = Vertex( v1v2.x, v1v2.y, v1v2.z );
	Vertex _v2v0 = Vertex( v2v0.x, v2v0.y, v2v0.z );
	Vertex _v2v1 = Vertex( v2v1.x, v2v1.y, v2v1.z );
	if ( _v0v1 == _v1v2 || _v0v1 == _v2v1 ||
		 _v0v2 == _v2v1 || _v0v2 == _v1v2 ||
		 _v1v0 == _v0v2 || _v1v0 == _v2v0 ) {
		Log(str(format("	THIS TRIANGLE WAS BAD BAD BAD!!! IT WAS A LINE!!!")));
		// env->terrain->undo( env->journalEntry ); // TODO: get this to work
		env->journalEntry->operations.clear();
		return false;
	}
	*/

	triEdge_p0p1 = new EdgeOfTri( triangle.p0, triangle.p1, chunk );
	triEdge_p1p2 = new EdgeOfTri( triangle.p1, triangle.p2, chunk );
	triEdge_p2p0 = new EdgeOfTri( triangle.p2, triangle.p0, chunk );

	if ( triEdge_p0p1->edges.size() == 1 &&
		 triEdge_p1p2->edges.size() == 1 &&
		 triEdge_p2p0->edges.size() == 1 ) {
		// One triangle; no subdividing necessary

		Log(str(format("	Triangle: NO Subdivision necessary... adding one")));
		Tri* newTri = new Tri(chunk, triIndex);
		// TODO: anything necessary? (added to edges; neighboured; added in chunks)

		// NOTE: flippedEdge doesn't imply that the edge is actually flipped; instead its existing edge match could be
		// the flipped version of this. Also the edge could be actually flipped, if we've found an exact match existing
		// edge which is a flipped version of ours. This is why its necessary to check each point for the correct
		// attachment
		EdgeTriNode* edge_p0p1 = triEdge_p0p1->edges.front()->edge;
		EdgeTriNode* edge_p1p2 = triEdge_p1p2->edges.front()->edge;
		EdgeTriNode* edge_p2p0 = triEdge_p2p0->edges.front()->edge;

		assert( edge_p0p1->p0_edges.size() );
		assert( edge_p0p1->p1_edges.size() );
		assert( edge_p1p2->p0_edges.size() );
		assert( edge_p1p2->p1_edges.size() );
		assert( edge_p2p0->p0_edges.size() );
		assert( edge_p2p0->p1_edges.size() );

		triEdge_p0p1->assertBadNeighbours(triEdge_p0p1->edges.front()->edge);
		triEdge_p1p2->assertBadNeighbours(triEdge_p1p2->edges.front()->edge);
		triEdge_p2p0->assertBadNeighbours(triEdge_p2p0->edges.front()->edge);

		// Neighbour the tri
		vector<pair<EdgeOfTri::InnerEdgeOfTri*,Tri**>> edges;
		edges.push_back({ triEdge_p0p1->edges.front(), &newTri->neighbour_p0p1 });
		edges.push_back({ triEdge_p1p2->edges.front(), &newTri->neighbour_p1p2 });
		edges.push_back({ triEdge_p2p0->edges.front(), &newTri->neighbour_p2p0 });
		for ( auto edge_pair : edges ) {
			EdgeTriNode* edge = edge_pair.first->edge;
			Log(str(format("	Adding Tri to Edge: (%1%,%2%)[%3%]")%edge->p0%edge->p1%edge));
			Tri** sideToAttach = edge->getTriSide( newTri );
			assert(sideToAttach);
			assert(!(*sideToAttach));
			(*sideToAttach) = newTri;
			if ( sideToAttach == &edge->triangle_p0p1 ) {
				(*edge_pair.second) = edge->triangle_p1p0;
				if (edge->triangle_p1p0) (*edge->triangle_p1p0->getNeighbourOnEdge(edge->p0, edge->p1)) = newTri;
			} else {
				(*edge_pair.second) = edge->triangle_p0p1;
				if (edge->triangle_p0p1) (*edge->triangle_p0p1->getNeighbourOnEdge(edge->p0, edge->p1)) = newTri;
			}
			

			// if ( edge_pair.first->flippedEdge ) {
			// 	assert(edge->triangle_p1p0==0);
			// 	edge->triangle_p1p0 = newTri;
			// 	(*edge_pair.second) = edge->triangle_p0p1;
			// 	if (edge->triangle_p0p1) (*edge->triangle_p0p1->getNeighbourOnEdge(edge->p0, edge->p1)) = newTri;
			// } else {
			// 	assert(edge->triangle_p0p1==0);
			// 	edge->triangle_p0p1 = newTri;
			// 	(*edge_pair.second) = edge->triangle_p1p0;
			// 	if (edge->triangle_p1p0) (*edge->triangle_p1p0->getNeighbourOnEdge(edge->p0, edge->p1)) = newTri;
			// }
			assert( ( edge->triangle_p0p1 == newTri && (
							( edge->p0 == newTri->p0 && edge->p1 == newTri->p1 ) ||
							( edge->p0 == newTri->p1 && edge->p1 == newTri->p2 ) ||
							( edge->p0 == newTri->p2 && edge->p1 == newTri->p0 ) ) ) ||
					( edge->triangle_p1p0 == newTri && (
							( edge->p1 == newTri->p0 && edge->p0 == newTri->p1 ) ||
							( edge->p1 == newTri->p1 && edge->p0 == newTri->p2 ) ||
							( edge->p1 == newTri->p2 && edge->p0 == newTri->p0 ) ) ) );
		}
		triEdge_p0p1->assertBadNeighbours(triEdge_p0p1->edges.front()->edge);
		triEdge_p1p2->assertBadNeighbours(triEdge_p1p2->edges.front()->edge);
		triEdge_p2p0->assertBadNeighbours(triEdge_p2p0->edges.front()->edge);
		Tri::assertBadTri(newTri);

		struct PointOnTri {
			PointOnTri(ushort pt) {

				Log(str(format("	PointOnTri(%1%)")%pt));
				Vertex p0 = terrain->vertexBuffer[pt];

				// find containers which contain this point
				vector<EdgeChunk*> containers = EdgeTriNode::getContainer(pt);
				EdgeChunk* container = containers.front(); // NOTE: if we get multiple containers then we're laying on the seam; if we're on the seam then any edges of concern must be along/across the seam, and therefore in both containers as well

				// find all edges in which this point is between
				vector<EdgeTriNode*> edges;
				for ( auto some_edge : container->edges ) {
					if (pt == some_edge->p0 || pt == some_edge->p1) continue;
					Vertex e_p0 = terrain->vertexBuffer[some_edge->p0];
					Vertex e_p1 = terrain->vertexBuffer[some_edge->p1];

					// is this vertex between the edge endpoints
					if ( p0.between(e_p0, e_p1) ) {
						Log(str(format("			between edge {%1%,%2%} <%3%,%4%,%5%> <%6%,%7%,%8%> :: <%9%,%10%,%11%>")%
								some_edge->p0 % some_edge->p1 %
								e_p0.v_x % e_p0.v_y % e_p0.v_z %
								e_p1.v_x % e_p1.v_y % e_p1.v_z %
								p0.v_x   % p0.v_y   % p0.v_z ));

						// filter edges in which this point lays on the line BUT is not the same point as the edge endpoints
						float dx = e_p1.v_x - e_p0.v_x;
						float dy = e_p1.v_y - e_p0.v_y;
						float dz = e_p1.v_z - e_p0.v_z;

						float tx = dx / (e_p1.v_x - p0.v_x);
						float ty = dy / (e_p1.v_y - p0.v_y);
						float tz = dz / (e_p1.v_z - p0.v_z);
						float t = tx;
						if (tx == 0 || ty == 0 || tz == 0) {
							if (tx == 0 && ty == 0 && tz == 0) {
								assert(false);
							} else if (tx == 0 && ty == 0) {

							} else if (tx == 0 && tz == 0) {

							} else if (ty == 0 && tz == 0) {

							} else if (tx == 0) {
								if (ty != tz) continue;
							} else if (ty == 0) {
								if (tx != tz) continue;
							} else if (tz == 0) {
								if (tx != ty) continue;
							}
						} else {
							if (tx != ty || tx != tz || ty != tz) continue;
						}

						edges.push_back( some_edge );
					}
				}

				if (edges.empty()) return;
				assert(edges.size()==1);
				EdgeTriNode* edge = edges.front();
				assert( ( edge->triangle_p0p1 == 0 || edge->triangle_p1p0 == 0 ) &&
					    ( edge->triangle_p0p1 != 0 || edge->triangle_p1p0 != 0 ) );

				Tri* tri = (edge->triangle_p0p1 ? edge->triangle_p0p1 : edge->triangle_p1p0);
				Log(str(format("***** RESIZE TRIANGLE: {%1%,%2%,%3%} ******")%tri->p0%tri->p1%tri->p2));
				// TODO: resize main tri
				// TODO: add new tri to make up for gap
			}
		};

		// TODO: is there a point to PointOnTri?
		PointOnTri* tp0 = new PointOnTri(triangle.p0);
		PointOnTri* tp1 = new PointOnTri(triangle.p1);
		PointOnTri* tp2 = new PointOnTri(triangle.p2);

		// merge journal entry with entries from edges
		vector<EdgeOfTri*> triEdges;
		triEdges.push_back( triEdge_p0p1 );
		triEdges.push_back( triEdge_p1p2 );
		triEdges.push_back( triEdge_p2p0 );
		for ( auto triEdge : triEdges ) {
			// Add journal entries in order
			// 	- vert
			// 	- edge
			// 	- tri
			vector<pair<vector<JournalEntry::JournalEntryOp*>*, vector<uchar>>> operationsToAdd;
			operationsToAdd.push_back({ &triEdge->journalEntry->operations, { 
					JournalEntry::JournalEntryOp::JOURNAL_ENTRY_REMOVEEDGE } }); // Edges (remove)
			operationsToAdd.push_back({ &triEdge->journalEntry->operations, { 
					JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDTRI,
					JournalEntry::JournalEntryOp::JOURNAL_ENTRY_REMOVETRI,
					JournalEntry::JournalEntryOp::JOURNAL_ENTRY_RESHAPETRI } }); // Tri
			operationsToAdd.push_back({ &triEdge->journalEntry->operations, { 
					JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDEDGE } }); // Edges (add)
			operationsToAdd.push_back({ &triEdge->journalEntry->operations, { 
					JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDVERT } }); // Verts
			Log(str(format("		Setting up journal: %1%")%env->journalEntry));
			Log(str(format("			SubJournal [%1%]:")%triEdge->journalEntry));
			for ( auto operation : triEdge->journalEntry->operations ) {
				string entry_string;
				if ( operation->entry_type == 0 ) entry_string = "ADDTRI";
				else if ( operation->entry_type == 1 ) entry_string = "RESHAPETRI";
				else if ( operation->entry_type == 2 ) entry_string = "REMOVETRI";
				else if ( operation->entry_type == 3 ) entry_string = "ADDVERT";
				else if ( operation->entry_type == 4 ) entry_string = "ADDEDGE";
				else if ( operation->entry_type == 5 ) entry_string = "REMOVEEDGE";
				else raise(SIGTRAP);
				Log(str(format("				Entry: %1%")%entry_string));
			}
			for ( auto operations : operationsToAdd ) {
				for ( auto journalEntryOperation : (*operations.first) ) {
					bool allowable = false;
					for ( auto acceptableEntry : operations.second ) {
						string entry_string;
						if ( journalEntryOperation->entry_type == 0 ) entry_string = "ADDTRI";
						else if ( journalEntryOperation->entry_type == 1 ) entry_string = "RESHAPETRI";
						else if ( journalEntryOperation->entry_type == 2 ) entry_string = "REMOVETRI";
						else if ( journalEntryOperation->entry_type == 3 ) entry_string = "ADDVERT";
						else if ( journalEntryOperation->entry_type == 4 ) entry_string = "ADDEDGE";
						else if ( journalEntryOperation->entry_type == 5 ) entry_string = "REMOVEEDGE";
						string test_entry;
						if ( acceptableEntry == 0 ) test_entry = "ADDTRI";
						else if ( acceptableEntry == 1 ) test_entry = "RESHAPETRI";
						else if ( acceptableEntry == 2 ) test_entry = "REMOVETRI";
						else if ( acceptableEntry == 3 ) test_entry = "ADDVERT";
						else if ( acceptableEntry == 4 ) test_entry = "ADDEDGE";
						else if ( acceptableEntry == 5 ) test_entry = "REMOVEEDGE";
						if ( acceptableEntry == journalEntryOperation->entry_type ) {
							Log(str(format("			%1% == %2%")%entry_string%test_entry));
							allowable = true;
							break;
						}
						Log(str(format("			%1% != %2%")%entry_string%test_entry));
					}
					if ( !allowable ) continue;

					string entry_string;
					if ( journalEntryOperation->entry_type == 0 ) entry_string = "ADDTRI";
					else if ( journalEntryOperation->entry_type == 1 ) entry_string = "RESHAPETRI";
					else if ( journalEntryOperation->entry_type == 2 ) entry_string = "REMOVETRI";
					else if ( journalEntryOperation->entry_type == 3 ) entry_string = "ADDVERT";
					else if ( journalEntryOperation->entry_type == 4 ) entry_string = "ADDEDGE";
					else if ( journalEntryOperation->entry_type == 5 ) entry_string = "REMOVEEDGE";
					else raise(SIGTRAP);
					Log(str(format("			Added journal entry operation: %1%")%entry_string));
					env->journalEntry->operations.insert( env->journalEntry->operations.begin() + 0, journalEntryOperation );
				}
			}
			delete triEdge->journalEntry;
		}
		Log(str(format("		Journal [%1%]:")%env->journalEntry));
		for ( auto operation : env->journalEntry->operations ) {
			string entry_string;
			if ( operation->entry_type == 0 ) entry_string = "ADDTRI";
			else if ( operation->entry_type == 1 ) entry_string = "RESHAPETRI";
			else if ( operation->entry_type == 2 ) entry_string = "REMOVETRI";
			else if ( operation->entry_type == 3 ) entry_string = "ADDVERT";
			else if ( operation->entry_type == 4 ) entry_string = "ADDEDGE";
			else if ( operation->entry_type == 5 ) entry_string = "REMOVEEDGE";
			else raise(SIGTRAP);
			Log(str(format("			Entry: %1%")%entry_string));
		}
		delete triEdge_p0p1;
		delete triEdge_p1p2;
		delete triEdge_p2p0;

		env->journalEntry->operations.push_back( JournalEntry::AddTri( triIndex, chunk->id,
					triangle.p0, triangle.p1, triangle.p2 ) );

		return true;

	} else if ( triEdge_p0p1->edges.size() == 1 &&
		 triEdge_p2p0->edges.size() > 1 ) {
		// Setup triangles from each side's edge
		// ----------------------------
		// NOTE: because of the nature of the default tessellation system, one requirement is that if p0p1 has 1 edge,
		// then p2p0 MUST have 1 edge. Otherwise the tessellation process must be changed entirely
		//
		// p1p2{i<n} -> p1p2(p0,p1) to p0p1(p0)
		// p2p0{i<=n} -> p2p0(p0,p1) to p1p2.last(p0)

		Log(str(format("	Triangle: Tessellation Case (1)")));

		// remove existing triangle in favour of new ones
		chunk->triangleBuffer.erase( chunk->triangleBuffer.begin() + triIndex );

		// merge journal entry with entries from edges
		vector<EdgeOfTri*> triEdges;
		triEdges.push_back( triEdge_p0p1 );
		triEdges.push_back( triEdge_p1p2 );
		triEdges.push_back( triEdge_p2p0 );
		for ( auto triEdge : triEdges ) {
			// Add journal entries in order
			// 	- vert
			// 	- edge
			// 	- tri
			vector<pair<vector<JournalEntry::JournalEntryOp*>*, vector<ushort>>> operationsToAdd;
			operationsToAdd.push_back({ &triEdge->journalEntry->operations, { 
					JournalEntry::JournalEntryOp::JOURNAL_ENTRY_REMOVEEDGE } }); // Edges (remove)
			operationsToAdd.push_back({ &triEdge->journalEntry->operations, { 
					JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDTRI,
					JournalEntry::JournalEntryOp::JOURNAL_ENTRY_REMOVETRI,
					JournalEntry::JournalEntryOp::JOURNAL_ENTRY_RESHAPETRI } }); // Tri
			operationsToAdd.push_back({ &triEdge->journalEntry->operations, { 
					JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDEDGE, } }); // Edges (add)
			operationsToAdd.push_back({ &triEdge->journalEntry->operations, { 
					JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDVERT } }); // Verts
			Log(str(format("		Setting up journal: %1%")%env->journalEntry));
			Log(str(format("			SubJournal [%1%]:")%triEdge->journalEntry));
			for ( auto operation : triEdge->journalEntry->operations ) {
				string entry_string;
				if ( operation->entry_type == 0 ) entry_string = "ADDTRI";
				else if ( operation->entry_type == 1 ) entry_string = "RESHAPETRI";
				else if ( operation->entry_type == 2 ) entry_string = "REMOVETRI";
				else if ( operation->entry_type == 3 ) entry_string = "ADDVERT";
				else if ( operation->entry_type == 4 ) entry_string = "ADDEDGE";
				else if ( operation->entry_type == 5 ) entry_string = "REMOVEEDGE";
				else raise(SIGTRAP);
				Log(str(format("				Entry: %1%")%entry_string));
			}
			for ( auto operations : operationsToAdd ) {
				for ( auto journalEntryOperation : (*operations.first) ) {
					bool allowable = false;
					for ( auto acceptableEntry : operations.second ) {
						if ( acceptableEntry == journalEntryOperation->entry_type ) {
							allowable = true;
							break;
						}
					}
					if ( !allowable ) continue;

					string entry_string;
					if ( journalEntryOperation->entry_type == 0 ) entry_string = "ADDTRI";
					else if ( journalEntryOperation->entry_type == 1 ) entry_string = "RESHAPETRI";
					else if ( journalEntryOperation->entry_type == 2 ) entry_string = "REMOVETRI";
					else if ( journalEntryOperation->entry_type == 3 ) entry_string = "ADDVERT";
					else if ( journalEntryOperation->entry_type == 4 ) entry_string = "ADDEDGE";
					else if ( journalEntryOperation->entry_type == 5 ) entry_string = "REMOVEEDGE";
					else raise(SIGTRAP);
					Log(str(format("			Added journal entry operation: %1%")%entry_string));
					env->journalEntry->operations.insert( env->journalEntry->operations.begin() + 0, journalEntryOperation );
				}
			}
			delete triEdge->journalEntry;
		}
		Log(str(format("		Journal [%1%]:")%env->journalEntry));
		for ( auto operation : env->journalEntry->operations ) {
			string entry_string;
			if ( operation->entry_type == 0 ) entry_string = "ADDTRI";
			else if ( operation->entry_type == 1 ) entry_string = "RESHAPETRI";
			else if ( operation->entry_type == 2 ) entry_string = "REMOVETRI";
			else if ( operation->entry_type == 3 ) entry_string = "ADDVERT";
			else if ( operation->entry_type == 4 ) entry_string = "ADDEDGE";
			else if ( operation->entry_type == 5 ) entry_string = "REMOVEEDGE";
			else raise(SIGTRAP);
			Log(str(format("			Entry: %1%")%entry_string));
		}

		// Setup triangles from p1p2 side to p0p1 edge
		EdgeOfTri::InnerEdgeOfTri* nextEdge = triEdge_p0p1->edges.front();
		int i=0;
		for ( auto in_edge : triEdge_p1p2->edges ) {
			if ( in_edge == triEdge_p1p2->edges.back() ) break;
			ushort expected_p0 = (in_edge->flippedEdge? in_edge->edge->p1 : in_edge->edge->p0);
			ushort expected_p1 = (in_edge->flippedEdge? in_edge->edge->p0 : in_edge->edge->p1);
			ushort p2 = (nextEdge->flippedEdge? nextEdge->edge->p1 : nextEdge->edge->p0);
			if (expected_p0 == expected_p1 || expected_p0 == p2 || expected_p1 == p2) continue;
			chunk->triangleBuffer.push_back( iTriangle( expected_p0, expected_p1, p2 ) );
			Log(str(format("		LOOKING TO ADD TRI: <%1%,%2%,%3%> (%4%)")%
						chunk->triangleBuffer.back().p0%
						chunk->triangleBuffer.back().p1%
						chunk->triangleBuffer.back().p2%i));
			addTriangle( chunk, chunk->triangleBuffer.size() - 1, env );
			++i;
		}

		// Setup triangles from p2p0 to last edge on p1p2
		nextEdge = triEdge_p1p2->edges.back();
		for ( auto in_edge : triEdge_p2p0->edges ) {
			ushort expected_p0 = (in_edge->flippedEdge? in_edge->edge->p1 : in_edge->edge->p0);
			ushort expected_p1 = (in_edge->flippedEdge? in_edge->edge->p0 : in_edge->edge->p1);
			ushort p2 = (nextEdge->flippedEdge? nextEdge->edge->p1 : nextEdge->edge->p0);
			if (expected_p0 == expected_p1 || expected_p0 == p2 || expected_p1 == p2) continue;
			chunk->triangleBuffer.push_back( iTriangle( expected_p0, expected_p1, p2 ) );
			Log(str(format("		LOOKING TO ADD TRI: <%1%,%2%,%3%> [%4%]")%
						chunk->triangleBuffer.back().p0%
						chunk->triangleBuffer.back().p1%
						chunk->triangleBuffer.back().p2%i));
			addTriangle( chunk, chunk->triangleBuffer.size() - 1, env );
			++i;
		}

		// re-sort the journal
		vector<pair<vector<JournalEntry::JournalEntryOp*>*, vector<ushort>>> operationsToAdd;
		operationsToAdd.push_back({&env->journalEntry->operations,{ // Edges (remove)
				JournalEntry::JournalEntryOp::JOURNAL_ENTRY_REMOVEEDGE }});
		operationsToAdd.push_back({&env->journalEntry->operations,{ // Tri
				JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDTRI,
				JournalEntry::JournalEntryOp::JOURNAL_ENTRY_REMOVETRI,
				JournalEntry::JournalEntryOp::JOURNAL_ENTRY_RESHAPETRI }});
		operationsToAdd.push_back({&env->journalEntry->operations,{ // Edges (add)
				JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDEDGE }});
		operationsToAdd.push_back({&env->journalEntry->operations,{ // Verts
				JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDVERT }});
		JournalEntry* cleanJournal = new JournalEntry();
		Log(str(format("		Re-sorting journal: %1% TO %2%")%env->journalEntry%cleanJournal));
		for ( auto operations : operationsToAdd ) {
			for ( auto journalEntryOperation : (*operations.first) ) {
					bool allowable = false;
					for ( auto acceptableEntry : operations.second ) {
						if ( acceptableEntry == journalEntryOperation->entry_type ) {
							allowable = true;
							break;
						}
					}
					if ( !allowable ) continue;

					string entry_string;
					if ( journalEntryOperation->entry_type == 0 ) entry_string = "ADDTRI";
					else if ( journalEntryOperation->entry_type == 1 ) entry_string = "RESHAPETRI";
					else if ( journalEntryOperation->entry_type == 2 ) entry_string = "REMOVETRI";
					else if ( journalEntryOperation->entry_type == 3 ) entry_string = "ADDVERT";
					else if ( journalEntryOperation->entry_type == 4 ) entry_string = "ADDEDGE";
					else if ( journalEntryOperation->entry_type == 5 ) entry_string = "REMOVEEDGE";
					else raise(SIGTRAP);
					Log(str(format("			Added journal entry operation: %1%")%entry_string));
					cleanJournal->operations.insert( cleanJournal->operations.begin() + 0, journalEntryOperation );
			}
		}

		// copy over clean journal
		env->journalEntry->operations.clear();
		for ( auto operation : cleanJournal->operations ) {
			env->journalEntry->operations.push_back( operation );
		}

		return false;
	} else {

		// Setup triangles from each side's edges
		// ---------------------------
		// p0p1{i<n} -> p2p0: p0p1(p0,p1) to p2p0.last(p0)
		// p1p2{i<=n} -> p0p1: p1p2(p0,p1) to p0p1.last(p0)
		// p2p0{i<n} -> p0p1: p2p0(p0,p1) to p0p1.last(p0)

		Log(str(format("	Triangle: Tessellation Case (2)")));

		// remove existing triangle in favour of new ones
		chunk->triangleBuffer.erase( chunk->triangleBuffer.begin() + triIndex );

		// merge journal entry with entries from edges
		vector<EdgeOfTri*> triEdges;
		triEdges.push_back( triEdge_p0p1 );
		triEdges.push_back( triEdge_p1p2 );
		triEdges.push_back( triEdge_p2p0 );
		for ( auto triEdge : triEdges ) {
			// Add journal entries in order
			// 	- vert
			// 	- edge
			// 	- tri
			vector<pair<vector<JournalEntry::JournalEntryOp*>*, vector<ushort>>> operationsToAdd;
			operationsToAdd.push_back({ &triEdge->journalEntry->operations, { 
					JournalEntry::JournalEntryOp::JOURNAL_ENTRY_REMOVEEDGE } }); // Edges (remove)
			operationsToAdd.push_back({ &triEdge->journalEntry->operations, { 
					JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDTRI,
					JournalEntry::JournalEntryOp::JOURNAL_ENTRY_REMOVETRI,
					JournalEntry::JournalEntryOp::JOURNAL_ENTRY_RESHAPETRI } }); // Tri
			operationsToAdd.push_back({ &triEdge->journalEntry->operations, { 
					JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDEDGE  } }); // Edges (add)
			operationsToAdd.push_back({ &triEdge->journalEntry->operations, { 
					JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDVERT } }); // Verts
			Log(str(format("		Setting up journal: %1%")%env->journalEntry));
			Log(str(format("			SubJournal [%1%]:")%triEdge->journalEntry));
			for ( auto operation : triEdge->journalEntry->operations ) {
				string entry_string;
				if ( operation->entry_type == 0 ) entry_string = "ADDTRI";
				else if ( operation->entry_type == 1 ) entry_string = "RESHAPETRI";
				else if ( operation->entry_type == 2 ) entry_string = "REMOVETRI";
				else if ( operation->entry_type == 3 ) entry_string = "ADDVERT";
				else if ( operation->entry_type == 4 ) entry_string = "ADDEDGE";
				else if ( operation->entry_type == 5 ) entry_string = "REMOVEEDGE";
				else raise(SIGTRAP);
				Log(str(format("				Entry: %1%")%entry_string));
			}
			for ( auto operations : operationsToAdd ) {
				for ( auto journalEntryOperation : (*operations.first) ) {
					bool allowable = false;
					for ( auto acceptableEntry : operations.second ) {
						if ( acceptableEntry == journalEntryOperation->entry_type ) {
							allowable = true;
							break;
						}
					}
					if ( !allowable ) continue;

					string entry_string;
					if ( journalEntryOperation->entry_type == 0 ) entry_string = "ADDTRI";
					else if ( journalEntryOperation->entry_type == 1 ) entry_string = "RESHAPETRI";
					else if ( journalEntryOperation->entry_type == 2 ) entry_string = "REMOVETRI";
					else if ( journalEntryOperation->entry_type == 3 ) entry_string = "ADDVERT";
					else if ( journalEntryOperation->entry_type == 4 ) entry_string = "ADDEDGE";
					else if ( journalEntryOperation->entry_type == 5 ) entry_string = "REMOVEEDGE";
					else raise(SIGTRAP);
					Log(str(format("			Added journal entry operation: %1%")%entry_string));
					env->journalEntry->operations.insert( env->journalEntry->operations.begin() + 0, journalEntryOperation );
				}
			}
			delete triEdge->journalEntry;
		}
		Log(str(format("		Journal [%1%]:")%env->journalEntry));
		for ( auto operation : env->journalEntry->operations ) {
			string entry_string;
			if ( operation->entry_type == 0 ) entry_string = "ADDTRI";
			else if ( operation->entry_type == 1 ) entry_string = "RESHAPETRI";
			else if ( operation->entry_type == 2 ) entry_string = "REMOVETRI";
			else if ( operation->entry_type == 3 ) entry_string = "ADDVERT";
			else if ( operation->entry_type == 4 ) entry_string = "ADDEDGE";
			else if ( operation->entry_type == 5 ) entry_string = "REMOVEEDGE";
			else raise(SIGTRAP);
			Log(str(format("			Entry: %1%")%entry_string));
		}

		// Setup triangles from p0p1 side to last edge on p2p0
		EdgeOfTri::InnerEdgeOfTri* nextEdge = triEdge_p2p0->edges.back();
		for ( auto in_edge : triEdge_p0p1->edges ) {
			if ( in_edge == triEdge_p0p1->edges.back() ) break;
			ushort expected_p0 = (in_edge->flippedEdge? in_edge->edge->p1 : in_edge->edge->p0);
			ushort expected_p1 = (in_edge->flippedEdge? in_edge->edge->p0 : in_edge->edge->p1);
			ushort p2 = (nextEdge->flippedEdge? nextEdge->edge->p1 : nextEdge->edge->p0);
			if (expected_p0 == expected_p1 || expected_p0 == p2 || expected_p1 == p2) continue;
			chunk->triangleBuffer.push_back( iTriangle( expected_p0, expected_p1, p2 ) );
			Log(str(format("		LOOKING TO ADD TRI: <%1%,%2%,%3%>")%
						chunk->triangleBuffer.back().p0%
						chunk->triangleBuffer.back().p1%
						chunk->triangleBuffer.back().p2));
			addTriangle( chunk, chunk->triangleBuffer.size() - 1, env );
		}

		// Setup triangles from p1p2 side to last edge on p0p1
		nextEdge = triEdge_p0p1->edges.back();
		for ( auto in_edge : triEdge_p1p2->edges ) {
			ushort expected_p0 = (in_edge->flippedEdge? in_edge->edge->p1 : in_edge->edge->p0);
			ushort expected_p1 = (in_edge->flippedEdge? in_edge->edge->p0 : in_edge->edge->p1);
			ushort p2 = (nextEdge->flippedEdge? nextEdge->edge->p1 : nextEdge->edge->p0);
			if (expected_p0 == expected_p1 || expected_p0 == p2 || expected_p1 == p2) continue;
			chunk->triangleBuffer.push_back( iTriangle( expected_p0, expected_p1, p2 ) );
			Log(str(format("		LOOKING TO ADD TRI: <%1%,%2%,%3%>")%
						chunk->triangleBuffer.back().p0%
						chunk->triangleBuffer.back().p1%
						chunk->triangleBuffer.back().p2));
			addTriangle( chunk, chunk->triangleBuffer.size() - 1, env );
		}

		// Setup triangles from p2p0 side to last edge on p0p1
		for ( auto in_edge : triEdge_p2p0->edges ) {
			if ( in_edge == triEdge_p2p0->edges.back() ) break;
			ushort expected_p0 = (in_edge->flippedEdge? in_edge->edge->p1 : in_edge->edge->p0);
			ushort expected_p1 = (in_edge->flippedEdge? in_edge->edge->p0 : in_edge->edge->p1);
			ushort p2 = (nextEdge->flippedEdge? nextEdge->edge->p1 : nextEdge->edge->p0);
			if (expected_p0 == expected_p1 || expected_p0 == p2 || expected_p1 == p2) continue;
			chunk->triangleBuffer.push_back( iTriangle( expected_p0, expected_p1, p2 ) );
			Log(str(format("		LOOKING TO ADD TRI: <%1%,%2%,%3%>")%
						chunk->triangleBuffer.back().p0%
						chunk->triangleBuffer.back().p1%
						chunk->triangleBuffer.back().p2));
			addTriangle( chunk, chunk->triangleBuffer.size() - 1, env );
		}


		return false;
	}
}
// ============================================== //


// ============================================== //
float EdgeTriTree::EdgeChunk::edgeHitPoint(Vertex p0, float dx, float dy, float dz, uchar face) {

	// plane: ax + by + cz = d
	// edge (parametric): x = et + f,  y = gt + h,  z = it + j
	// intersection: a(et + f) + b(gt + h) + c(it + j) = d
	// 				 t(ae + bg + ci) = d - af - bh - cj
	float t=0, d;
	
	if ( face == FACE_TOP ) {
		d = worldOffset.y + chunkSize.y;
		t = ( d - p0.v_y ) / ( dy );
	} else if ( face == FACE_BOTTOM ) {
		d = worldOffset.y;
		t = ( d - p0.v_y ) / ( dy );
	} else if ( face == FACE_RIGHT ) {
		d = worldOffset.x + chunkSize.x;
		t = ( d - p0.v_x ) / ( dx );
	} else if ( face == FACE_LEFT ) {
		d = worldOffset.x;
		t = ( d - p0.v_x ) / ( dx );
	} else if ( face == FACE_FRONT ) {
		d = worldOffset.z + chunkSize.z;
		t = ( d - p0.v_z ) / ( dz );
	} else if ( face == FACE_BACK ) {
		d = worldOffset.z;
		t = ( d - p0.v_z ) / ( dz );
	}

	return t;
}
// ============================================== //


// ============================================== //
bool EdgeTriTree::EdgeChunk::pointInFace(Vertex hitpoint, uchar face) {

	if ( face == FACE_TOP ) {
		return ( hitpoint.v_x <= worldOffset.x + chunkSize.x &&
				 hitpoint.v_x >= worldOffset.x &&
				 hitpoint.v_y == worldOffset.y + chunkSize.y &&
				 hitpoint.v_z <= worldOffset.z + chunkSize.z &&
				 hitpoint.v_z >= worldOffset.z );
	} else if ( face == FACE_BOTTOM ) {
		return ( hitpoint.v_x <= worldOffset.x + chunkSize.x &&
				 hitpoint.v_x >= worldOffset.x &&
				 hitpoint.v_y == worldOffset.y &&
				 hitpoint.v_z <= worldOffset.z + chunkSize.z &&
				 hitpoint.v_z >= worldOffset.z );
	} else if ( face == FACE_FRONT ) {
		return ( hitpoint.v_x <= worldOffset.x + chunkSize.x &&
				 hitpoint.v_x >= worldOffset.x &&
				 hitpoint.v_y <= worldOffset.y + chunkSize.y &&
				 hitpoint.v_y >= worldOffset.y &&
				 hitpoint.v_z == worldOffset.z + chunkSize.z );
	} else if ( face == FACE_BACK ) {
		return ( hitpoint.v_x <= worldOffset.x + chunkSize.x &&
				 hitpoint.v_x >= worldOffset.x &&
				 hitpoint.v_y <= worldOffset.y + chunkSize.y &&
				 hitpoint.v_y >= worldOffset.y &&
				 hitpoint.v_z == worldOffset.z );
	} else if ( face == FACE_RIGHT ) {
		return ( hitpoint.v_x == worldOffset.x + chunkSize.x &&
				 hitpoint.v_y <= worldOffset.y + chunkSize.y &&
				 hitpoint.v_y >= worldOffset.y &&
				 hitpoint.v_z <= worldOffset.z + chunkSize.z &&
				 hitpoint.v_z >= worldOffset.z );
	} else if ( face == FACE_LEFT ) {
		return ( hitpoint.v_x == worldOffset.x &&
				 hitpoint.v_y <= worldOffset.y + chunkSize.y &&
				 hitpoint.v_y >= worldOffset.y &&
				 hitpoint.v_z <= worldOffset.z + chunkSize.z &&
				 hitpoint.v_z >= worldOffset.z );
	}

	return false;
}
// ============================================== //


// ============================================== //
Chunk::AddTriangleResults Chunk::addTriangle(Voxel* p0, Voxel* p1, Voxel* p2, Environment* env) {

	/* Attempt to add given triangle to this chunk
	 * 
	 * If the given triangle cannot be added to this chunk (it doesn't fit) then the
	 * point(s) which don't fit within the chunk are projected onto the chunk and returned
	 * in the AddTriangleResults; along with the details behind what sort of triangle
	 * subdivision is necessary to fit it within this chunk. Other projections may be
	 * necessary, like finding points on the edge of the chunk which lay on the triangle;
	 * or the midpoint between projections.
	 **/

	try {
	Log(str(format("Chunk::addTriangle {%1%,%2%,%3%}: <%4%,%5%,%6%> <%7%,%8%,%9%> <%10%,%11%,%12%>") %
			p0->vertexIndex % p1->vertexIndex % p2->vertexIndex %
			p0->getX() % p0->getY() % p0->getZ() % 
			p1->getX() % p1->getY() % p1->getZ() % 
			p2->getX() % p2->getY() % p2->getZ() ));
	} catch(exception& e) {
		Log("Chunk::addTriangle ...?");
	}
	AddTriangleResults results;

	if ( p0->vertexIndex == p1->vertexIndex ||
		 p0->vertexIndex == p2->vertexIndex ||
		 p1->vertexIndex == p2->vertexIndex ) {
		results.addResults = AddTriangleResults::TRIANGLE_ADD_FAILED;
		return results;
	}

	// does this triangle already exist?
	uint index = 0;
	iTriangle newTriangle(p0->vertexIndex, p1->vertexIndex, p2->vertexIndex);
	for ( auto existingTriangle : triangleBuffer ) {
		if ( existingTriangle == newTriangle ) {
			results.addedTriangle = index;
			results.addResults = AddTriangleResults::TRIANGLE_ADD_NOT_NECESSARY;
			return results; // triangle has already been added
		}
		++index;
	}

	auto anyMatchingPoints = []( ushort p0, ushort p1, ushort p2, AddTriangleResults* points ) -> bool {
		auto matchingPoints = []( ushort p, AddTriangleResults* points ) -> bool {
			return ( 
			 ( points->projected_p1       && p == points->projected_p1->vertexIndex ) ||
			 ( points->projected_p2       && p == points->projected_p2->vertexIndex ) ||
			 ( points->projected_p1p2     && p == points->projected_p1p2->vertexIndex ) ||
			 ( points->projected_p1Mid    && p == points->projected_p1Mid->vertexIndex ) ||
			 ( points->projected_p2Mid    && p == points->projected_p2Mid->vertexIndex ) ||
			 ( points->projected_midpoint && p == points->projected_midpoint->vertexIndex ) ||
			 ( points->outer_midpoint     && p == points->outer_midpoint->vertexIndex ) );
		};
		auto pointsMatchOthers = []( AddTriangleResults* points ) -> bool {
			ushort v;
			if ( points->projected_p1 ) {
				v = points->projected_p1->vertexIndex;
				if (
					( points->projected_p2       && v == points->projected_p2->vertexIndex ) ||
					( points->projected_p1p2     && v == points->projected_p1p2->vertexIndex ) ||
					( points->projected_p1Mid    && v == points->projected_p1Mid->vertexIndex ) ||
					( points->projected_p2Mid    && v == points->projected_p2Mid->vertexIndex ) ||
					( points->projected_midpoint && v == points->projected_midpoint->vertexIndex ) ||
					( points->outer_midpoint     && v == points->outer_midpoint->vertexIndex ) ) return true;
			}
			if ( points->projected_p2 ) {
				v = points->projected_p2->vertexIndex;
				if (
					( points->projected_p1       && v == points->projected_p1->vertexIndex ) ||
					( points->projected_p1p2     && v == points->projected_p1p2->vertexIndex ) ||
					( points->projected_p1Mid    && v == points->projected_p1Mid->vertexIndex ) ||
					( points->projected_p2Mid    && v == points->projected_p2Mid->vertexIndex ) ||
					( points->projected_midpoint && v == points->projected_midpoint->vertexIndex ) ||
					( points->outer_midpoint     && v == points->outer_midpoint->vertexIndex ) ) return true;
			}
			if ( points->projected_p1p2 ) {
				v = points->projected_p1p2->vertexIndex;
				if (
					( points->projected_p1       && v == points->projected_p1->vertexIndex ) ||
					( points->projected_p2       && v == points->projected_p2->vertexIndex ) ||
					( points->projected_p1Mid    && v == points->projected_p1Mid->vertexIndex ) ||
					( points->projected_p2Mid    && v == points->projected_p2Mid->vertexIndex ) ||
					( points->projected_midpoint && v == points->projected_midpoint->vertexIndex ) ||
					( points->outer_midpoint     && v == points->outer_midpoint->vertexIndex ) ) return true;
			}
			if ( points->projected_p1Mid ) {
				v = points->projected_p1Mid->vertexIndex;
				if (
					( points->projected_p1       && v == points->projected_p1->vertexIndex ) ||
					( points->projected_p2       && v == points->projected_p2->vertexIndex ) ||
					( points->projected_p1p2     && v == points->projected_p1p2->vertexIndex ) ||
					( points->projected_p2Mid    && v == points->projected_p2Mid->vertexIndex ) ||
					( points->projected_midpoint && v == points->projected_midpoint->vertexIndex ) ||
					( points->outer_midpoint     && v == points->outer_midpoint->vertexIndex ) ) return true;
			}
			if ( points->projected_p2Mid ) {
				v = points->projected_p2Mid->vertexIndex;
				if (
					( points->projected_p1       && v == points->projected_p1->vertexIndex ) ||
					( points->projected_p2       && v == points->projected_p2->vertexIndex ) ||
					( points->projected_p1p2     && v == points->projected_p1p2->vertexIndex ) ||
					( points->projected_p1Mid    && v == points->projected_p1Mid->vertexIndex ) ||
					( points->projected_midpoint && v == points->projected_midpoint->vertexIndex ) ||
					( points->outer_midpoint     && v == points->outer_midpoint->vertexIndex ) ) return true;
			}
			if ( points->projected_midpoint ) {
				v = points->projected_midpoint->vertexIndex;
				if (
					( points->projected_p1       && v == points->projected_p1->vertexIndex ) ||
					( points->projected_p2       && v == points->projected_p2->vertexIndex ) ||
					( points->projected_p1p2     && v == points->projected_p1p2->vertexIndex ) ||
					( points->projected_p1Mid    && v == points->projected_p1Mid->vertexIndex ) ||
					( points->projected_p2Mid    && v == points->projected_p2Mid->vertexIndex ) ||
					( points->outer_midpoint     && v == points->outer_midpoint->vertexIndex ) ) return true;
			}
			if ( points->outer_midpoint ) {
				v = points->outer_midpoint->vertexIndex;
				if (
					( points->projected_p1       && v == points->projected_p1->vertexIndex ) ||
					( points->projected_p2       && v == points->projected_p2->vertexIndex ) ||
					( points->projected_p1p2     && v == points->projected_p1p2->vertexIndex ) ||
					( points->projected_p1Mid    && v == points->projected_p1Mid->vertexIndex ) ||
					( points->projected_p2Mid    && v == points->projected_p2Mid->vertexIndex ) ||
					( points->projected_midpoint && v == points->projected_midpoint->vertexIndex ) ) return true;
			}

			return false;

		};

		return ( p0 == p1 || p0 == p2 || p1 == p2 ||
				 matchingPoints( p0, points ) || matchingPoints( p1, points ) || matchingPoints( p2, points ) ||
				 pointsMatchOthers( points ) );

	};

	
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

			uchar face;
			pair<Vertex*,Vertex*>* pairedProjections = getVoxelsProjections( p1, p2, p0, &face );
			if ( pairedProjections ) {

				// both points project onto the same face

				results.projected_p1 = new Voxel();
				Vertex* projectedVertex = pairedProjections->first;
				Vertex new_vert =  Vertex(projectedVertex->v_x, projectedVertex->v_y, projectedVertex->v_z);
				int i=0;
				for( auto vertex : terrain->vertexBuffer ) {
					if ( vertex == new_vert ) {
						Log(str(format("Borrowed vertex elsewhere.. %1%")%i));
						results.projected_p1->vertexIndex = i;
						break;
					}
					++i;
				}
				if ( i >= terrain->vertexBuffer.size() ) {
					terrain->vertexBuffer.push_back( new_vert );
					results.projected_p1->vertexIndex = terrain->vertexBuffer.size() - 1;
					env->journalEntry->operations.push_back( JournalEntry::AddVert( results.projected_p1->vertexIndex,
								projectedVertex->v_x, projectedVertex->v_y, projectedVertex->v_z ) );
				}

				if ( p1->vertexIndex == results.projected_p1->vertexIndex ||
						p0->vertexIndex == results.projected_p1->vertexIndex ) {
					delete results.projected_p1;
					results.projected_p1 = 0;
				} else {
					results.projected_p1->terrain = terrain;
				}

				if ( anyMatchingPoints( p0->vertexIndex, p1->vertexIndex, p2->vertexIndex, &results ) ) {
					raise(SIGTRAP); // NOTE: x1 this was just a line..
					// 	Log("i");
					results.addResults = AddTriangleResults::TRIANGLE_ADD_NOT_NECESSARY;
					return results;
				}

				results.projected_p2 = new Voxel();
				projectedVertex = pairedProjections->second;
				new_vert =  Vertex(projectedVertex->v_x, projectedVertex->v_y, projectedVertex->v_z);
				i=0;
				for( auto vertex : terrain->vertexBuffer ) {
					if ( vertex == new_vert ) {
						Log(str(format("Borrowed vertex elsewhere.. %1%")%i));
						results.projected_p2->vertexIndex = i;
						break;
					}
					++i;
				}
				if ( i >= terrain->vertexBuffer.size() ) {
					terrain->vertexBuffer.push_back( new_vert );
					results.projected_p2->vertexIndex = terrain->vertexBuffer.size() - 1;
					env->journalEntry->operations.push_back( JournalEntry::AddVert( results.projected_p2->vertexIndex,
								projectedVertex->v_x, projectedVertex->v_y, projectedVertex->v_z ) );
				}

				if ( p2->vertexIndex == results.projected_p2->vertexIndex ||
						p0->vertexIndex == results.projected_p2->vertexIndex ) {
					delete results.projected_p2;
					results.projected_p2 = 0;
				} else {
					results.projected_p2->terrain = terrain;
				}

				if ( anyMatchingPoints( p0->vertexIndex, p1->vertexIndex, p2->vertexIndex, &results ) ) {
					results.addResults = AddTriangleResults::TRIANGLE_ADD_NOT_NECESSARY;
					Log("FAILED TO ADD TRI: TRI IS A LINE!!! BAD TRI!");
					return results;
					// 	Log("i");
					// 	results.addResults = AddTriangleResults::TRIANGLE_ADD_FAILED_BADTRI;
					// 	return results;
				}



				if ( !results.projected_p1 || !results.projected_p2 ) {
					results.addResults = AddTriangleResults::TRIANGLE_ADD_FAILED;
					Log("FAILED TO ADD TRI: probably just need to add from another vert");
					return results;
				}



				// find midpoint between outside points
				Vertex* midpoint = new Vertex();
				midpoint->v_x = (p1end.v_x - p2end.v_x)/2 + p2end.v_x;
				midpoint->v_y = (p1end.v_y - p2end.v_y)/2 + p2end.v_y;
				midpoint->v_z = (p1end.v_z - p2end.v_z)/2 + p2end.v_z;
				results.projected_midpoint = new Voxel();

				new_vert =  Vertex(midpoint->v_x, midpoint->v_y, midpoint->v_z);
				i=0;
				for( auto vertex : terrain->vertexBuffer ) {
					if ( vertex == new_vert ) {
						Log(str(format("Borrowed vertex elsewhere.. %1%")%i));
						// assert(false);
						results.projected_midpoint->vertexIndex = i;
						break;
					}
					++i;
				}
				if ( i >= terrain->vertexBuffer.size() ) {
					terrain->vertexBuffer.push_back( new_vert );
					results.projected_midpoint->vertexIndex = terrain->vertexBuffer.size() - 1;
					env->journalEntry->operations.push_back( JournalEntry::AddVert( results.projected_midpoint->vertexIndex,
								midpoint->v_x, midpoint->v_y, midpoint->v_z ) );
				}

				if ( ( results.projected_p1 && results.projected_p1->vertexIndex == results.projected_midpoint->vertexIndex ) ||
						( results.projected_p2 && results.projected_p2->vertexIndex == results.projected_midpoint->vertexIndex ) ||
						( results.projected_p1p2 && results.projected_p1p2->vertexIndex == results.projected_midpoint->vertexIndex ) ) {
					delete results.projected_midpoint;
					results.projected_midpoint = 0;
					results.addResults = AddTriangleResults::TRIANGLE_ADD_TWOPOINT_ONESIDE_NOMID;
					return results;
				} else if ( ( p1->vertexIndex == results.projected_midpoint->vertexIndex ) ||
						( p2->vertexIndex == results.projected_midpoint->vertexIndex ) ) {
					delete results.projected_midpoint;
					results.projected_midpoint = 0;
					results.addResults = AddTriangleResults::TRIANGLE_ADD_TWOPOINT_ONESIDE_NOMID;
					return results;
				}

				if ( anyMatchingPoints( p0->vertexIndex, p1->vertexIndex, p2->vertexIndex, &results ) ) {
					raise(SIGTRAP);
					// 	Log("g");
					// 	results.addResults = AddTriangleResults::TRIANGLE_ADD_FAILED_BADTRI;
					// 	return results;
				}


				results.projected_midpoint->terrain = terrain;

				results.addResults = AddTriangleResults::TRIANGLE_ADD_TWOPOINT_ONESIDE;
				Log("NEED TO SUBDIVIDE TRIANGLE: two points / one side");
				return results;
			}


			// project p1 along the p0-p1 edge, onto the chunk
			results.projected_p1 = new Voxel();
			uchar facep1;
			Vertex* projectedVertex = getVoxelProjection( p1, p0, &facep1 );
			if ( projectedVertex == 0 ) {
				results.addResults = AddTriangleResults::TRIANGLE_ADD_FAILED;
				return results;
			}

			Vertex new_vert =  Vertex(projectedVertex->v_x, projectedVertex->v_y, projectedVertex->v_z);
			int i=0;
			for( auto vertex : terrain->vertexBuffer ) {
				if ( vertex == new_vert ) {
					Log(str(format("Borrowed vertex elsewhere.. %1%")%i));
					// assert(false);
					results.projected_p1->vertexIndex = i;
					break;
				}
				++i;
			}
			if ( i >= terrain->vertexBuffer.size() ) {
				terrain->vertexBuffer.push_back( new_vert );
				results.projected_p1->vertexIndex = terrain->vertexBuffer.size() - 1;
				env->journalEntry->operations.push_back( JournalEntry::AddVert( results.projected_p1->vertexIndex,
							projectedVertex->v_x, projectedVertex->v_y, projectedVertex->v_z ) );
			}

			if ( p1->vertexIndex == results.projected_p1->vertexIndex ||
					p0->vertexIndex == results.projected_p1->vertexIndex ) {
				delete results.projected_p1;
				results.projected_p1 = 0;
			} else {
				results.projected_p1->terrain = terrain;
			}

			if ( anyMatchingPoints( p0->vertexIndex, p1->vertexIndex, p2->vertexIndex, &results ) ) {
				raise(SIGTRAP);
				// 	Log("i");
				// 	results.addResults = AddTriangleResults::TRIANGLE_ADD_FAILED_BADTRI;
				// 	return results;
			}


			// project p2 along the p0-p2 edge, onto the chunk
			results.projected_p2 = new Voxel();
			uchar facep2;
			projectedVertex = getVoxelProjection( p2, p0, &facep2 );
			if ( projectedVertex == 0 ) {
				results.addResults = AddTriangleResults::TRIANGLE_ADD_FAILED;
				return results;
			}

			new_vert =  Vertex(projectedVertex->v_x, projectedVertex->v_y, projectedVertex->v_z);
			i=0;
			for( auto vertex : terrain->vertexBuffer ) {
				if ( vertex == new_vert ) {
					Log(str(format("Borrowed vertex elsewhere.. %1%")%i));
					// assert(false);
					results.projected_p2->vertexIndex = i;
					break;
				}
				++i;
			}
			if ( i >= terrain->vertexBuffer.size() ) {
				terrain->vertexBuffer.push_back( new_vert );
				results.projected_p2->vertexIndex = terrain->vertexBuffer.size() - 1;
				env->journalEntry->operations.push_back( JournalEntry::AddVert( results.projected_p2->vertexIndex,
							projectedVertex->v_x, projectedVertex->v_y, projectedVertex->v_z ) );
			}

			if ( p2->vertexIndex == results.projected_p2->vertexIndex ||
					p0->vertexIndex == results.projected_p2->vertexIndex ) {
				delete results.projected_p2;
				results.projected_p2 = 0;
			} else {
				results.projected_p2->terrain = terrain;
			}

			if ( (results.projected_p1 && results.projected_p2) && results.projected_p1->vertexIndex == results.projected_p2->vertexIndex ) {
				// this is a line..
				delete results.projected_p1;
				delete results.projected_p2;
				results.projected_p1 = 0;
				results.projected_p2 = 0;
				results.addResults = AddTriangleResults::TRIANGLE_ADD_NOT_NECESSARY;
				return results;
			}

			if ( anyMatchingPoints( p0->vertexIndex, p1->vertexIndex, p2->vertexIndex, &results ) ) {
				raise(SIGTRAP); // NOTE: x1 this was just a line
				results.addResults = AddTriangleResults::TRIANGLE_ADD_NOT_NECESSARY;
				return results;
				// 	Log("h");
				// 	results.addResults = AddTriangleResults::TRIANGLE_ADD_FAILED_BADTRI;
				// 	return results;
			}

			if ( !results.projected_p1 && !results.projected_p2 ) {
				// simple triangle, no subdivision
				triangleBuffer.push_back(iTriangle(p0->vertexIndex, p1->vertexIndex, p2->vertexIndex));
				results.addResults = AddTriangleResults::TRIANGLE_ADD_SUCCEEDED;
				results.addedTriangle = triangleBuffer.size() - 1;
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

				Vertex new_vert =  Vertex(projectedVertex_p1p2->v_x, projectedVertex_p1p2->v_y, projectedVertex_p1p2->v_z);
				i=0;
				for( auto vertex : terrain->vertexBuffer ) {
					if ( vertex == new_vert ) {
						Log(str(format("Borrowed vertex elsewhere.. %1%")%i));
						// assert(false);
						results.projected_p1Mid->vertexIndex = i;
						break;
					}
					++i;
				}
				if ( i >= terrain->vertexBuffer.size() ) {
					terrain->vertexBuffer.push_back( new_vert );
					results.projected_p1Mid->vertexIndex = terrain->vertexBuffer.size() - 1;
					env->journalEntry->operations.push_back( JournalEntry::AddVert( results.projected_p1Mid->vertexIndex,
								projectedVertex_p1p2->v_x, projectedVertex_p1p2->v_y, projectedVertex_p1p2->v_z ) );
				}

				if ( anyMatchingPoints( p0->vertexIndex, p1->vertexIndex, p2->vertexIndex, &results ) ) {
					Log("f");
					results.addResults = AddTriangleResults::TRIANGLE_ADD_FAILED_BADTRI;
					return results;
				}


				results.projected_p1Mid->terrain = terrain;

				if ( !results.projected_p1 ) {
					results.projected_p1p2 = results.projected_p1Mid;
					results.projected_p1Mid = 0;
					results.addResults = AddTriangleResults::TRIANGLE_ADD_ONEPOINT_ONESIDE;
					return results;
				}

				// find point along edge of chunk of projected-p2 face
				results.projected_p2Mid = new Voxel();

				new_vert =  Vertex(projectedVertex_p2p1->v_x, projectedVertex_p2p1->v_y, projectedVertex_p2p1->v_z);
				i=0;
				for( auto vertex : terrain->vertexBuffer ) {
					if ( vertex == new_vert ) {
						Log(str(format("Borrowed vertex elsewhere.. %1%")%i));
						// assert(false);
						results.projected_p2Mid->vertexIndex = i;
						break;
					}
					++i;
				}
				if ( i >= terrain->vertexBuffer.size() ) {
					terrain->vertexBuffer.push_back( new_vert );
					results.projected_p2Mid->vertexIndex = terrain->vertexBuffer.size() - 1;
					env->journalEntry->operations.push_back( JournalEntry::AddVert( results.projected_p2Mid->vertexIndex,
								projectedVertex_p1p2->v_x, projectedVertex_p1p2->v_y, projectedVertex_p1p2->v_z ) );
				}

				if ( anyMatchingPoints( p0->vertexIndex, p1->vertexIndex, p2->vertexIndex, &results ) ) {
					Log("e");
					results.addResults = AddTriangleResults::TRIANGLE_ADD_FAILED_BADTRI;
					return results;
				}


				results.projected_p2Mid->terrain = terrain;

				if ( !results.projected_p2 ) {
					delete results.projected_p1Mid;
					results.projected_p1p2 = results.projected_p2Mid;
					results.projected_p2Mid = 0;
					results.addResults = AddTriangleResults::TRIANGLE_ADD_ONEPOINT_ONESIDE;
					return results;
				}

				// TODO: necessary to include _THREESIDE result?
				Log("NEED TO SUBDIVIDE TRIANGLE: four points / two sides...three sides?");
				results.addResults = AddTriangleResults::TRIANGLE_ADD_FOURPOINT_TWOSIDE;
				return results;

			}

			// TODO: we're forcing TWOPOINT_TWOSIDE even if its not the case; which is
			// better, p1mid or p2mid?
			Vertex* p1mid;
			Vertex* p2mid;
			Vertex* mid;
			if ( !results.projected_p1 ) {
				p1mid = getSeamIntersectionPoint( &p0end, &p1end, &p2end, facep1 );
				mid = p1mid;
			} else if ( !results.projected_p2 ) {
				p2mid = getSeamIntersectionPoint( &p0end, &p1end, &p2end, facep2 );
				mid = p2mid;
			} else {
				p1mid = getSeamIntersectionPoint( &p0end, &p1end, &p2end, facep1 ); // TODO: returns 0 in some cases!?

				// In some cases 
				if ( !p1mid ) {
					for ( int i = 0; i <= 5; ++i ) {
						p1mid = getSeamIntersectionPoint( &p0end, &p1end, &p2end, i );
						if ( p1mid ) break;
					}
				}
				mid = p1mid;
			}


			if ( results.projected_p1 &&
				mid->v_x == results.projected_p1->getX() &&
				mid->v_y == results.projected_p1->getY() &&
				mid->v_z == results.projected_p1->getZ() ) {
				Log("	projected p1p2 is the SAME as p1'");
				if (!results.projected_p2) {
					results.addResults = AddTriangleResults::TRIANGLE_ADD_ONEPOINT_ONESIDE_ONEPROJ;
					return results;
				}
				results.addResults = AddTriangleResults::TRIANGLE_ADD_TWOPOINT_ONESIDE_NOMID;
				return results;
			} else if ( results.projected_p2 &&
				mid->v_x == results.projected_p2->getX() &&
				mid->v_y == results.projected_p2->getY() &&
				mid->v_z == results.projected_p2->getZ() ) {
				Log("	projected p1p2 is the SAME as p2'");
				if (!results.projected_p1) {
					results.addResults = AddTriangleResults::TRIANGLE_ADD_ONEPOINT_ONESIDE_ONEPROJ;
					return results;
				}
				results.addResults = AddTriangleResults::TRIANGLE_ADD_TWOPOINT_ONESIDE_NOMID;
				return results;
			}
			// Vertex* p2mid = getSeamIntersectionPoint( &p0end, &p1end, &p2end, facep2 );

			// find shared midpoint between projected-p1 and projected-p2 along edge of
			// chunk (along faces)
			results.projected_p1p2 = new Voxel();

			new_vert =  Vertex(mid->v_x, mid->v_y, mid->v_z);
			i=0;
			for( auto vertex : terrain->vertexBuffer ) {
				if ( vertex == new_vert ) {
					Log(str(format("Borrowed vertex elsewhere.. %1%")%i));
					// assert(false);
					results.projected_p1p2->vertexIndex = i;
					break;
				}
				++i;
			}
			if ( i >= terrain->vertexBuffer.size() ) {
				terrain->vertexBuffer.push_back( new_vert );
				results.projected_p1p2->vertexIndex = terrain->vertexBuffer.size() - 1;
				env->journalEntry->operations.push_back( JournalEntry::AddVert( results.projected_p1p2->vertexIndex,
							mid->v_x, mid->v_y, mid->v_z ) );
			}

			if ( p1->vertexIndex == results.projected_p1p2->vertexIndex ||
				 p2->vertexIndex == results.projected_p1p2->vertexIndex ) {
				// p1 or p2 is laying on its projection
				delete results.projected_p1p2;
				results.projected_p1p2 = 0;

				if (!results.projected_p1 || !results.projected_p2) {
					results.addResults = AddTriangleResults::TRIANGLE_ADD_ONEPOINT_ONESIDE_ONEPROJ;
					return results;
				}

				results.addResults = AddTriangleResults::TRIANGLE_ADD_TWOPOINT_ONESIDE_NOMID;
				Log("NEED TO SUBDIVIDE TRIANGLE: two points / one side / no mid");
				return results;
			} else if ( ( results.projected_p1 && results.projected_p1->vertexIndex == results.projected_p1p2->vertexIndex ) ||
						( results.projected_p2 && results.projected_p2->vertexIndex == results.projected_p1p2->vertexIndex ) ) {
				delete results.projected_p1p2;
				results.projected_p1p2 = 0;

				if (!results.projected_p1 || !results.projected_p2) {
					results.addResults = AddTriangleResults::TRIANGLE_ADD_ONEPOINT_ONESIDE_ONEPROJ;
					return results;
				}

				results.addResults = AddTriangleResults::TRIANGLE_ADD_TWOPOINT_ONESIDE_NOMID;
				Log("NEED TO SUBDIVIDE TRIANGLE: two points / one side / no mid");
				return results;
			}

			if ( anyMatchingPoints( p0->vertexIndex, p1->vertexIndex, p2->vertexIndex, &results ) ) {
				raise(SIGTRAP);
			// 	Log("d");
			// 	results.addResults = AddTriangleResults::TRIANGLE_ADD_FAILED_BADTRI;
			// 	return results;
			}


			results.projected_p1p2->terrain = terrain;

			if (!results.projected_p1 || !results.projected_p2) {
				results.addResults = AddTriangleResults::TRIANGLE_ADD_ONEPOINT_ONESIDE_ONEPROJ;
				return results;
			}

			results.addResults = AddTriangleResults::TRIANGLE_ADD_TWOPOINT_TWOSIDE;
			Log("NEED TO SUBDIVIDE TRIANGLE: two points / two sides");
			return results;

		} else {

			results.addResults = AddTriangleResults::TRIANGLE_ADD_ONEPOINT_TWOSIDE; // this may change.. hence its initialized up here to allow for possible changes
			Voxel* outsideVoxel   = (p1_isOutside?p1:p2);
			Voxel* neighbourVoxel = (p1_isOutside?p2:p1);

			// project outside voxel along p0-pX edge, onto chunk
			uchar facep0;
			uchar facep2;
			uchar face;
			pair<Vertex*,Vertex*>* voxelProjections = getVoxelProjections( outsideVoxel, p0, neighbourVoxel, &face );
			if ( voxelProjections ) {

				Vertex* projectedVertex0 = voxelProjections->first;
				Vertex* projectedVertexN = voxelProjections->second;

				Voxel* projection = new Voxel();
				Vertex new_vert =  Vertex(projectedVertex0->v_x, projectedVertex0->v_y, projectedVertex0->v_z);
				int i=0;
				for( auto vertex : terrain->vertexBuffer ) {
					if ( vertex == new_vert ) {
						Log(str(format("Borrowed vertex elsewhere.. %1%")%i));
						projection->vertexIndex = i;
						break;
					}
					++i;
				}
				if ( i >= terrain->vertexBuffer.size() ) {
					terrain->vertexBuffer.push_back( new_vert );
					projection->vertexIndex = terrain->vertexBuffer.size() - 1;
					env->journalEntry->operations.push_back( JournalEntry::AddVert( projection->vertexIndex,
								projectedVertex0->v_x, projectedVertex0->v_y, projectedVertex0->v_z ) );
				}

				if ( projection->vertexIndex == p0->vertexIndex ||
						projection->vertexIndex == p1->vertexIndex ||
						projection->vertexIndex == p2->vertexIndex ) {
					projectedVertex0 = 0;
				} else {
					projection->terrain = terrain;
					if ( p1_isOutside ) results.projected_p1 = projection;
					else results.projected_p2 = projection;
				}

				// project outside voxel along p1-p2 edge, onto chunk
				results.projected_p1p2 = new Voxel();
				new_vert = Vertex(projectedVertexN->v_x, projectedVertexN->v_y, projectedVertexN->v_z);
				i=0;
				for( auto vertex : terrain->vertexBuffer ) {
					if ( vertex == new_vert ) {
						Log(str(format("Borrowed vertex elsewhere.. %1%")%i));
						// assert(false);
						results.projected_p1p2->vertexIndex = i;
						break;
					}
					++i;
				}
				if ( i >= terrain->vertexBuffer.size() ) {
					terrain->vertexBuffer.push_back( new_vert );
					results.projected_p1p2->vertexIndex = terrain->vertexBuffer.size() - 1;
					env->journalEntry->operations.push_back( JournalEntry::AddVert( results.projected_p1p2->vertexIndex,
								projectedVertexN->v_x, projectedVertexN->v_y, projectedVertexN->v_z ) );
				}

				if ( results.projected_p1p2->vertexIndex == p0->vertexIndex ||
						results.projected_p1p2->vertexIndex == p1->vertexIndex ||
						results.projected_p1p2->vertexIndex == p2->vertexIndex ||
						(results.projected_p1 && results.projected_p1p2->vertexIndex == results.projected_p1->vertexIndex) ||
						(results.projected_p2 && results.projected_p1p2->vertexIndex == results.projected_p2->vertexIndex) ) {
					delete results.projected_p1p2;
					results.projected_p1p2 = 0;
					projectedVertexN = 0;
				} else {
					results.projected_p1p2->terrain = terrain;
				}



				// in case the projection(s) were false
				if ( projectedVertex0 == 0 && projectedVertexN == 0 ) {
					// projection from outside vertex to both inside vertices are just themselves
					// NOTE: this means it fails in adding the triangle to this chunk, however it should fit in the outside
					// vertex's chunk, given that the two inside vertices lay on the seam
					Log("	Failed...probably just needs to use the other vertex's chunk");
					results.addResults = AddTriangleResults::TRIANGLE_ADD_FAILED;
					return results;
				}


				// in case one of the inside voxels lays on the seam of the chunk, the outside voxel will likely project
				// directly onto the same voxel as that one...hence we only have 1 unique projection, and no midpoint
				// necessary
				if ( projectedVertex0 == 0 || projectedVertexN == 0 ) {
					results.addResults = AddTriangleResults::TRIANGLE_ADD_ONEPOINT_ONESIDE_ONEPROJ;
					Log("NEED TO SUBDIVIDE TRIANGLE: one point / one side - one projection");
					return results;
				}


				if ( !results.projected_p1p2 ) {
					results.addResults = AddTriangleResults::TRIANGLE_ADD_ONEPOINT_ONESIDE_ONEPROJ;
					Log("NEED TO SUBDIVIDE TRIANGLE: one point / one side / one projection");
					return results;
				}

				results.addResults = AddTriangleResults::TRIANGLE_ADD_ONEPOINT_ONESIDE;
				Log("NEED TO SUBDIVIDE TRIANGLE: one point / one side");
				return results;
			}

			Vertex* projectedVertex0 = getVoxelProjection( outsideVoxel, p0, &facep0 );
			Vertex* projectedVertexN = getVoxelProjection( outsideVoxel, neighbourVoxel, &facep2 );
			if ( projectedVertex0 == 0 && projectedVertexN == 0 ) {
				// projection from outside vertex to both inside vertices are just themselves
				// NOTE: this means it fails in adding the triangle to this chunk, however it should fit in the outside
				// vertex's chunk, given that the two inside vertices lay on the seam
				Log("	Failed...probably just needs to use the other vertex's chunk");
				results.addResults = AddTriangleResults::TRIANGLE_ADD_FAILED;
				return results;
			}

			int i=0;
			if ( projectedVertex0 ) {

				Voxel* projection = new Voxel();
				Vertex new_vert =  Vertex(projectedVertex0->v_x, projectedVertex0->v_y, projectedVertex0->v_z);
				for( auto vertex : terrain->vertexBuffer ) {
					if ( vertex == new_vert ) {
						Log(str(format("Borrowed vertex elsewhere.. %1%")%i));
						projection->vertexIndex = i;
						break;
					}
					++i;
				}
				if ( i >= terrain->vertexBuffer.size() ) {
					terrain->vertexBuffer.push_back( new_vert );
					projection->vertexIndex = terrain->vertexBuffer.size() - 1;
					env->journalEntry->operations.push_back( JournalEntry::AddVert( projection->vertexIndex,
								projectedVertex0->v_x, projectedVertex0->v_y, projectedVertex0->v_z ) );
				}

				if ( projection->vertexIndex == p0->vertexIndex ||
					 projection->vertexIndex == p1->vertexIndex ||
					 projection->vertexIndex == p2->vertexIndex ) {
					projectedVertex0 = 0;
				} else {
					projection->terrain = terrain;
					if ( p1_isOutside ) results.projected_p1 = projection;
					else results.projected_p2 = projection;
				}
			}

			if ( projectedVertexN ) {

				// project outside voxel along p1-p2 edge, onto chunk
				results.projected_p1p2 = new Voxel();
				Vertex new_vert = Vertex(projectedVertexN->v_x, projectedVertexN->v_y, projectedVertexN->v_z);
				i=0;
				for( auto vertex : terrain->vertexBuffer ) {
					if ( vertex == new_vert ) {
						Log(str(format("Borrowed vertex elsewhere.. %1%")%i));
						// assert(false);
						results.projected_p1p2->vertexIndex = i;
						break;
					}
					++i;
				}
				if ( i >= terrain->vertexBuffer.size() ) {
					terrain->vertexBuffer.push_back( new_vert );
					results.projected_p1p2->vertexIndex = terrain->vertexBuffer.size() - 1;
					env->journalEntry->operations.push_back( JournalEntry::AddVert( results.projected_p1p2->vertexIndex,
								projectedVertexN->v_x, projectedVertexN->v_y, projectedVertexN->v_z ) );
				}

				if ( results.projected_p1p2->vertexIndex == p0->vertexIndex ||
					 results.projected_p1p2->vertexIndex == p1->vertexIndex ||
					 results.projected_p1p2->vertexIndex == p2->vertexIndex ||
					 (results.projected_p1 && results.projected_p1p2->vertexIndex == results.projected_p1->vertexIndex) ||
					 (results.projected_p2 && results.projected_p1p2->vertexIndex == results.projected_p2->vertexIndex) ) {
					delete results.projected_p1p2;
					results.projected_p1p2 = 0;
					projectedVertexN = 0;
				} else {
					results.projected_p1p2->terrain = terrain;
				}
			}

			// in case the projection(s) were false
			if ( projectedVertex0 == 0 && projectedVertexN == 0 ) {
				// projection from outside vertex to both inside vertices are just themselves
				// NOTE: this means it fails in adding the triangle to this chunk, however it should fit in the outside
				// vertex's chunk, given that the two inside vertices lay on the seam
				Log("	Failed...probably just needs to use the other vertex's chunk");
				results.addResults = AddTriangleResults::TRIANGLE_ADD_FAILED;
				return results;
			}


			// in case one of the inside voxels lays on the seam of the chunk, the outside voxel will likely project
			// directly onto the same voxel as that one...hence we only have 1 unique projection, and no midpoint
			// necessary
			if ( projectedVertex0 == 0 || projectedVertexN == 0 ) {
				results.addResults = AddTriangleResults::TRIANGLE_ADD_ONEPOINT_ONESIDE_ONEPROJ;
				Log("NEED TO SUBDIVIDE TRIANGLE: one point / one side - one projection");
				return results;
			}


			// find midpoint along seam of chunk between both projections
			Vertex* midpoint = getSeamIntersectionPoint( &p0end, &p1end, &p2end, facep0, facep2 ); // TODO: sometimes returns 0 !?
			results.projected_midpoint = new Voxel();

			if ( midpoint == 0 ) {
				raise(SIGTRAP);
				Log("c");
				results.addResults = AddTriangleResults::TRIANGLE_ADD_FAILED_BADTRI;
				return results;
			}

			Vertex new_vert = Vertex(midpoint->v_x, midpoint->v_y, midpoint->v_z);
			i=0;
			for( auto vertex : terrain->vertexBuffer ) {
				if ( vertex == new_vert ) {
					Log(str(format("Borrowed vertex elsewhere.. %1%")%i));
					// assert(false);
					results.projected_midpoint->vertexIndex = i;
					break;
				}
				++i;
			}
			if ( i >= terrain->vertexBuffer.size() ) {
				terrain->vertexBuffer.push_back( new_vert );
				results.projected_midpoint->vertexIndex = terrain->vertexBuffer.size() - 1;
				env->journalEntry->operations.push_back( JournalEntry::AddVert( results.projected_midpoint->vertexIndex,
							midpoint->v_x, midpoint->v_y, midpoint->v_z ) );
			}

			if ( ( results.projected_p1 && results.projected_p1->vertexIndex == results.projected_midpoint->vertexIndex ) || 
				 ( results.projected_p2 && results.projected_p2->vertexIndex == results.projected_midpoint->vertexIndex ) ||
				 ( results.projected_p1p2 && results.projected_p1p2->vertexIndex == results.projected_midpoint->vertexIndex ) ) {
				delete results.projected_midpoint;
				results.projected_midpoint = 0;

				if ( !results.projected_p1p2 ) {
					results.addResults = AddTriangleResults::TRIANGLE_ADD_ONEPOINT_ONESIDE_ONEPROJ;
					Log("NEED TO SUBDIVIDE TRIANGLE: one point / one side / one projection");
					return results;
				}

				results.addResults = AddTriangleResults::TRIANGLE_ADD_ONEPOINT_ONESIDE;
				return results;
			}

			if ( anyMatchingPoints( p0->vertexIndex, p1->vertexIndex, p2->vertexIndex, &results ) ) {
				raise(SIGTRAP);
			// 	Log("b");
			// 	results.addResults = AddTriangleResults::TRIANGLE_ADD_FAILED_BADTRI;
			// 	return results;
			}


			results.projected_midpoint->terrain = terrain;
			
			return results;


		}
	}


	// add triangle
	if (p0->vertexIndex == p1->vertexIndex || p0->vertexIndex == p2->vertexIndex || p1->vertexIndex == p2->vertexIndex) {
		raise(SIGTRAP);
		// two vertices are the SAME.. hence this is not a triangle
		Log("a");
		results.addResults = AddTriangleResults::TRIANGLE_ADD_FAILED_BADTRI;
		return results;
	}
	triangleBuffer.push_back(iTriangle(p0->vertexIndex, p1->vertexIndex, p2->vertexIndex));
	results.addResults = AddTriangleResults::TRIANGLE_ADD_SUCCEEDED;
	results.addedTriangle = triangleBuffer.size() - 1;

	// env->journalEntry->operations.push_back( JournalEntry::AddTri( results.addedTriangle, this->id,
	// 			p0->vertexIndex, p1->vertexIndex, p2->vertexIndex ) );

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
	Log(str(format("		Projecting {%1%} -> {%2%} :: <%3%,%4%,%5%> -> <%6%,%7%,%8%> {[%9%:%10%],[%11%:%12%],[%13%:%14%]}")%voxel->vertexIndex%neighbour->vertexIndex%
				v_voxel->v_x % v_voxel->v_y % v_voxel->v_z %
				v_neighbour->v_x % v_neighbour->v_y % v_neighbour->v_z %
				worldOffset.x % (worldOffset.x + chunkSize.x) %
				worldOffset.y % (worldOffset.y + chunkSize.y) %
				worldOffset.z % (worldOffset.z + chunkSize.z) ));
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
pair<Vertex*,Vertex*>* Chunk::getVoxelsProjections(Voxel* voxel1, Voxel* voxel2, Voxel* neighbour, uchar* face) {

	// attempt to intersect the voxel-neighbour edge on each face of the chunk where the
	// voxel is on the other side; return the projection of the first successful result
	Vertex* v_voxel1     = new Vertex();
	v_voxel1->v_x = terrain->vertexBuffer[voxel1->vertexIndex].v_x;
	v_voxel1->v_y = terrain->vertexBuffer[voxel1->vertexIndex].v_y;
	v_voxel1->v_z = terrain->vertexBuffer[voxel1->vertexIndex].v_z;

	Vertex* v_voxel2     = new Vertex();
	v_voxel2->v_x = terrain->vertexBuffer[voxel2->vertexIndex].v_x;
	v_voxel2->v_y = terrain->vertexBuffer[voxel2->vertexIndex].v_y;
	v_voxel2->v_z = terrain->vertexBuffer[voxel2->vertexIndex].v_z;

	Vertex* v_neighbour = new Vertex();
	v_neighbour->v_x = terrain->vertexBuffer[neighbour->vertexIndex].v_x;
	v_neighbour->v_y = terrain->vertexBuffer[neighbour->vertexIndex].v_y;
	v_neighbour->v_z = terrain->vertexBuffer[neighbour->vertexIndex].v_z;

	Vertex* projectedVertex1 = 0;
	Vertex* projectedVertex2 = 0;
	if        ( v_voxel1->v_x < worldOffset.x && (projectedVertex1 = projectVertexOntoFace(v_voxel1, v_neighbour, FACE_LEFT)) &&
			    v_voxel2->v_x < worldOffset.x && (projectedVertex2 = projectVertexOntoFace(v_voxel2, v_neighbour, FACE_LEFT)) ) {
		*face = FACE_LEFT; return new pair<Vertex*,Vertex*>{projectedVertex1, projectedVertex2};
	} else if ( v_voxel1->v_y < worldOffset.y && (projectedVertex1 = projectVertexOntoFace(v_voxel1, v_neighbour, FACE_BOTTOM)) &&
			    v_voxel2->v_y < worldOffset.y && (projectedVertex2 = projectVertexOntoFace(v_voxel2, v_neighbour, FACE_BOTTOM)) ) {
		*face = FACE_BOTTOM; return new pair<Vertex*,Vertex*>{projectedVertex1, projectedVertex2};
	} else if ( v_voxel1->v_z < worldOffset.z && (projectedVertex1 = projectVertexOntoFace(v_voxel1, v_neighbour, FACE_BACK)) &&
			    v_voxel2->v_z < worldOffset.z && (projectedVertex2 = projectVertexOntoFace(v_voxel2, v_neighbour, FACE_BACK)) ) {
		*face = FACE_BACK; return new pair<Vertex*,Vertex*>{projectedVertex1, projectedVertex2};
	} else if ( v_voxel1->v_x > worldOffset.x + chunkSize.x && (projectedVertex1 = projectVertexOntoFace(v_voxel1, v_neighbour, FACE_RIGHT)) &&
			    v_voxel2->v_x > worldOffset.x + chunkSize.x && (projectedVertex2 = projectVertexOntoFace(v_voxel2, v_neighbour, FACE_RIGHT)) ) {
		*face = FACE_RIGHT; return new pair<Vertex*,Vertex*>{projectedVertex1, projectedVertex2};
	} else if ( v_voxel1->v_y > worldOffset.y + chunkSize.y && (projectedVertex1 = projectVertexOntoFace(v_voxel1, v_neighbour, FACE_TOP)) &&
			    v_voxel2->v_y > worldOffset.y + chunkSize.y && (projectedVertex2 = projectVertexOntoFace(v_voxel2, v_neighbour, FACE_TOP)) ) {
		*face = FACE_TOP; return new pair<Vertex*,Vertex*>{projectedVertex1, projectedVertex2};
	} else if ( v_voxel1->v_z > worldOffset.z + chunkSize.z && (projectedVertex1 = projectVertexOntoFace(v_voxel1, v_neighbour, FACE_FRONT)) &&
			    v_voxel2->v_z > worldOffset.z + chunkSize.z && (projectedVertex2 = projectVertexOntoFace(v_voxel2, v_neighbour, FACE_FRONT)) ) {
		*face = FACE_FRONT; return new pair<Vertex*,Vertex*>{projectedVertex1, projectedVertex2};
	}
	return 0;

}
// ============================================== //


// ============================================== //
pair<Vertex*,Vertex*>* Chunk::getVoxelProjections(Voxel* voxel, Voxel* neighbour1, Voxel* neighbour2, uchar* face) {

	Vertex* v_voxel     = new Vertex();
	v_voxel->v_x = terrain->vertexBuffer[voxel->vertexIndex].v_x;
	v_voxel->v_y = terrain->vertexBuffer[voxel->vertexIndex].v_y;
	v_voxel->v_z = terrain->vertexBuffer[voxel->vertexIndex].v_z;


	Vertex* v_neighbour1 = new Vertex();
	v_neighbour1->v_x = terrain->vertexBuffer[neighbour1->vertexIndex].v_x;
	v_neighbour1->v_y = terrain->vertexBuffer[neighbour1->vertexIndex].v_y;
	v_neighbour1->v_z = terrain->vertexBuffer[neighbour1->vertexIndex].v_z;

	Vertex* v_neighbour2 = new Vertex();
	v_neighbour2->v_x = terrain->vertexBuffer[neighbour2->vertexIndex].v_x;
	v_neighbour2->v_y = terrain->vertexBuffer[neighbour2->vertexIndex].v_y;
	v_neighbour2->v_z = terrain->vertexBuffer[neighbour2->vertexIndex].v_z;


	Vertex* projectedVertex1 = 0;
	Vertex* projectedVertex2 = 0;
	if        ( v_voxel->v_x < worldOffset.x && (projectedVertex1 = projectVertexOntoFace(v_voxel, v_neighbour1, FACE_LEFT)) &&
			    v_voxel->v_x < worldOffset.x && (projectedVertex2 = projectVertexOntoFace(v_voxel, v_neighbour2, FACE_LEFT)) ) {
		*face = FACE_LEFT; return new pair<Vertex*,Vertex*>{projectedVertex1, projectedVertex2};
	} else if ( v_voxel->v_y < worldOffset.y && (projectedVertex1 = projectVertexOntoFace(v_voxel, v_neighbour1, FACE_BOTTOM)) &&
			    v_voxel->v_y < worldOffset.y && (projectedVertex2 = projectVertexOntoFace(v_voxel, v_neighbour2, FACE_BOTTOM)) ) {
		*face = FACE_BOTTOM; return new pair<Vertex*,Vertex*>{projectedVertex1, projectedVertex2};
	} else if ( v_voxel->v_z < worldOffset.z && (projectedVertex1 = projectVertexOntoFace(v_voxel, v_neighbour1, FACE_BACK)) &&
			    v_voxel->v_z < worldOffset.z && (projectedVertex2 = projectVertexOntoFace(v_voxel, v_neighbour2, FACE_BACK)) ) {
		*face = FACE_BACK; return new pair<Vertex*,Vertex*>{projectedVertex1, projectedVertex2};
	} else if ( v_voxel->v_x > worldOffset.x + chunkSize.x && (projectedVertex1 = projectVertexOntoFace(v_voxel, v_neighbour1, FACE_RIGHT)) &&
			    v_voxel->v_x > worldOffset.x + chunkSize.x && (projectedVertex2 = projectVertexOntoFace(v_voxel, v_neighbour2, FACE_RIGHT)) ) {
		*face = FACE_RIGHT; return new pair<Vertex*,Vertex*>{projectedVertex1, projectedVertex2};
	} else if ( v_voxel->v_y > worldOffset.y + chunkSize.y && (projectedVertex1 = projectVertexOntoFace(v_voxel, v_neighbour1, FACE_TOP)) &&
			    v_voxel->v_y > worldOffset.y + chunkSize.y && (projectedVertex2 = projectVertexOntoFace(v_voxel, v_neighbour2, FACE_TOP)) ) {
		*face = FACE_TOP; return new pair<Vertex*,Vertex*>{projectedVertex1, projectedVertex2};
	} else if ( v_voxel->v_z > worldOffset.z + chunkSize.z && (projectedVertex1 = projectVertexOntoFace(v_voxel, v_neighbour1, FACE_FRONT)) &&
			    v_voxel->v_z > worldOffset.z + chunkSize.z && (projectedVertex2 = projectVertexOntoFace(v_voxel, v_neighbour2, FACE_FRONT)) ) {
		*face = FACE_FRONT; return new pair<Vertex*,Vertex*>{projectedVertex1, projectedVertex2};
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
	float t = (d -a*f - b*h - c*j) / (a*e + b*g + c*i); // TODO: what if denominator is 0?
	float x = e*t + f,
		  y = g*t + h,
		  z = i*t + j;

	// is this within the bounds of the face?
	if ( x == neighbour->v_x && y == neighbour->v_y && z == neighbour->v_z ) {
		Log(str(format("		Projected to SAME as neighbour: <%1%,%2%,%3%>")%x%y%z));
	}
	if ( fabs(t) <= 1.0f &&
		 ( x >= worldOffset.x && x <= worldOffset.x + chunkSize.x ) &&
		 ( y >= worldOffset.y && y <= worldOffset.y + chunkSize.y ) &&
		 ( z >= worldOffset.z && z <= worldOffset.z + chunkSize.z ) &&
		 !(x == neighbour->v_x && y == neighbour->v_y && z == neighbour->v_z )) {
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

	// FIXME: try ALL other possibilities in case the triangle hit the chunk in a weird way
	float wx = worldOffset.x;
	float wy = worldOffset.y;
	float wz = worldOffset.z;
	float wxe = worldOffset.x + chunkSize.x;
	float wye = worldOffset.y + chunkSize.y;
	float wze = worldOffset.z + chunkSize.z;
	if ( (intersection = lineIntersectTriangle( wx, wy, wz, 0, 0, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wy, wz, 0, 1, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wy, wz, 0, 1, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wy, wz, 1, 0, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wy, wz, 1, 0, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wy, wz, 1, 1, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wy, wz, 1, 1, 1, p0, p1, p2 )) ) return intersection;

	if ( (intersection = lineIntersectTriangle( wx, wy, wze, 0, 0, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wy, wze, 0, 1, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wy, wze, 0, 1, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wy, wze, 1, 0, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wy, wze, 1, 0, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wy, wze, 1, 1, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wy, wze, 1, 1, 1, p0, p1, p2 )) ) return intersection;

	if ( (intersection = lineIntersectTriangle( wx, wye, wz, 0, 0, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wye, wz, 0, 1, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wye, wz, 0, 1, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wye, wz, 1, 0, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wye, wz, 1, 0, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wye, wz, 1, 1, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wye, wz, 1, 1, 1, p0, p1, p2 )) ) return intersection;

	if ( (intersection = lineIntersectTriangle( wx, wye, wze, 0, 0, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wye, wze, 0, 1, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wye, wze, 0, 1, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wye, wze, 1, 0, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wye, wze, 1, 0, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wye, wze, 1, 1, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wx, wye, wze, 1, 1, 1, p0, p1, p2 )) ) return intersection;

	if ( (intersection = lineIntersectTriangle( wxe, wy, wz, 0, 0, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wy, wz, 0, 1, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wy, wz, 0, 1, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wy, wz, 1, 0, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wy, wz, 1, 0, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wy, wz, 1, 1, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wy, wz, 1, 1, 1, p0, p1, p2 )) ) return intersection;

	if ( (intersection = lineIntersectTriangle( wxe, wy, wze, 0, 0, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wy, wze, 0, 1, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wy, wze, 0, 1, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wy, wze, 1, 0, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wy, wze, 1, 0, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wy, wze, 1, 1, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wy, wze, 1, 1, 1, p0, p1, p2 )) ) return intersection;

	if ( (intersection = lineIntersectTriangle( wxe, wye, wz, 0, 0, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wye, wz, 0, 1, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wye, wz, 0, 1, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wye, wz, 1, 0, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wye, wz, 1, 0, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wye, wz, 1, 1, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wye, wz, 1, 1, 1, p0, p1, p2 )) ) return intersection;

	if ( (intersection = lineIntersectTriangle( wxe, wye, wze, 0, 0, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wye, wze, 0, 1, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wye, wze, 0, 1, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wye, wze, 1, 0, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wye, wze, 1, 0, 1, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wye, wze, 1, 1, 0, p0, p1, p2 )) ) return intersection;
	if ( (intersection = lineIntersectTriangle( wxe, wye, wze, 1, 1, 1, p0, p1, p2 )) ) return intersection;


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
	if ( u < 0.f && fabs(u) < EPSILON ) u = 0; // JB: close enough..

	//Prepare to test v parameter
	Q = glm::cross( T, e1 );

	//Calculate V parameter and test bound
	v = glm::dot( lineD, Q ) * inv_det;
	if ( v < 0.f && fabs(v) < EPSILON ) v = 0; // JB: close enough..

	//The intersection lies outside of the triangle
	if(v < 0.f || u + v  > 1.f) return 0;

	t = glm::dot( e2, Q ) * inv_det;
	if ( t < 0 || t > chunkSize.x || t > chunkSize.y || t > chunkSize.z ) {
		// TODO: this setup appears buggy in some situations....seriously why is this such a problem ?? Can we assume
		// that if we've gotten this far then the line hits inside the triangle, except that t is wrong??

		glm::vec3 plane_norm = glm::cross( e1, e2 );
		float right_side = glm::dot( plane_norm, v0 );
		t = (right_side - (glm::dot(plane_norm, lineO))) / (glm::dot(plane_norm, lineD));
		if (  t < 0 || t > chunkSize.x || t > chunkSize.y || t > chunkSize.z ) {
			// TODO: perhaps the lines intersect?
			// LINE: x = L0x + tLx
			// 		 y = L0y + tLy
			// 		 z = L0z + tLz
			// E: x = e0x + sex
			// 	  y = e0y + sey
			// 	  z = e0z + sez
			//
			// x = L0x + tLx = e0x + sex
			// 	   L0x - e0x = sex - tLx
			// 	   tLx = sex + e0x - L0x
			// 	   t = (sex + e0x - L0x) / Lx
			// y = L0y + tLy = e0y + sey
			// 	   L0y - e0y = sey - tLy
			// 	   L0y - e0y = sey - ((Ly/Lx)(sex + e0x - L0x))
			// 	   L0y - e0y = sey - (Ly/Lx)(sex) - (Ly/Lx)(e0x - L0x)
			// 	   L0y - e0y = s(ey - (Ly/Lx)ex) - (Ly/Lx)(e0x - L0x)
			// 	   L0y - e0y + (Ly/Lx)(e0x - L0x) = s(ey - (Ly/Lx)ex)
			// 	   s = (L0y - e0y + (Ly/Lx)(e0x - L0x)) / (ey - (Ly/Lx)ex)
			// z = L0z + tLz
			//
			// NOTE: check if hitpoint is outside of chunk; if so then find vector v = (chunk_center - hit) and
			// repeatedly add tv (small t) to hit until its inside chunk or t surpasses threshold



			// TODO: the lines may not intersect precisely, but are close enough to be considered intersecting
			// line_pt = 

			// TODO: if line hits, may need to offset point slightly to fit inside chunk


			// TODO: either e1 or e2
			
			// e1 intersect with line..
			// (v0 + t2*e1) = lineO + t1*lineD  (ASSUME lineD.x == 0)
			// (x):  t2 = (lineO.x - v0.x) / e1.x
			if ( lineD.x == 0 ) {
				float t2X = (lineO.x - v0.x) / e1.x;
			} else if ( lineD.y == 0 ) {

			}
			return 0;
		}

		Log(str(format(" OMG IT HIT!!! %1%")%t));
		Vertex* hit = new Vertex();
		hit->v_x = lineO.x + line_dx*t;
		hit->v_y = lineO.y + line_dy*t;
		hit->v_z = lineO.z + line_dz*t;
		return hit;
		return 0; // TODO: t too big?
	}

	Vertex* hit = new Vertex();
	hit->v_x = lineO.x + line_dx*t;
	hit->v_y = lineO.y + line_dy*t;
	hit->v_z = lineO.z + line_dz*t;
	return hit;

}
// ============================================== //


// ============================================== //
void Chunk::construct() {

	
	glUseProgram(terrain->gl);
	// TODO: delete vao (in case its already setup for another program)
	
	// enable display list
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );

	// setup buffer object
	glBindBuffer( GL_ARRAY_BUFFER, terrain->vbo );

	GLint glVertex = glGetAttribLocation( terrain->gl, "in_Position" );
	glEnableVertexAttribArray( glVertex );

	// load data into shader
	glVertexAttribPointer( glVertex, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0 );


	// Generate a buffer for the indices
	glGenBuffers(1, &elementBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangleBuffer.size() * sizeof(iTriangle), triangleBuffer.data(), GL_DYNAMIC_DRAW);

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
	chunk->id = chunkList.size();
	chunkList.push_back(chunk);

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
Tri* Terrain::terrainPick(glm::vec3 position, glm::vec3 direction) {

	// TODO: check ray against pages, then chunks, then move along line_linkedlist for
	// intersecting quad
	
	vector<pair<Tri*,float>> hits;

	// find intersecting quad
	EdgeTriTree::EdgeChunk* far_bottom_back_left = edgeTree->headChunk;
	EdgeTriTree::EdgeChunk* bottom_back_left;
	EdgeTriTree::EdgeChunk* cur_chunk;
	while ( far_bottom_back_left ) {

		bottom_back_left = far_bottom_back_left;
		while ( bottom_back_left ) {

			cur_chunk = bottom_back_left;
			while ( cur_chunk ) {

				// TODO: check for intersection of this chunk

				// check for all Triangles which are hit in this chunk
				for ( auto edge : cur_chunk->edges ) {
					vector<Tri*> triangles;
					if ( edge->triangle_p0p1 ) triangles.push_back( edge->triangle_p0p1 );
					if ( edge->triangle_p0p1 ) triangles.push_back( edge->triangle_p0p1 );
					if ( edge->triangle_p1p0 ) triangles.push_back( edge->triangle_p1p0 );
					if ( edge->triangle_p1p0 ) triangles.push_back( edge->triangle_p1p0 );
					int i=0;
					for ( auto tri : triangles ) {
						++i;
						// check if tri hits..
						Vertex pv0 = vertexBuffer[tri->p0];
						Vertex pv1 = vertexBuffer[tri->p1];
						Vertex pv2 = vertexBuffer[tri->p2];

						// glm::mat4 mvp = camera.perspectiveView;
						// mvp = glm::transpose(mvp);
						glm::vec4 _v0 = glm::vec4( pv0.v_x, pv0.v_y, pv0.v_z, 1.0f );
						glm::vec4 _v1 = glm::vec4( pv1.v_x, pv1.v_y, pv1.v_z, 1.0f );
						glm::vec4 _v2 = glm::vec4( pv2.v_x, pv2.v_y, pv2.v_z, 1.0f );


						glm::vec3 v0;
						glm::vec3 v1;
						glm::vec3 v2;
						if (i%2==0) {
							v0 = _v0.xyz;
							v1 = _v2.xyz;
							v2 = _v1.xyz;
						} else {
							v0 = _v0.xyz;
							v1 = _v1.xyz;
							v2 = _v2.xyz;
						}


						glm::vec3 u = v1 - v0;
						glm::vec3 v = v2 - v0;
						glm::vec3 norm = glm::cross( u, v );

						if ( norm == glm::vec3(0.0f) ) {
							continue; // triangle is degenerate
						}


						glm::vec3 triRayDir = position - v0;
						float triRayNormDot = -1 * glm::dot( norm, triRayDir );
						float rayNormDot    = glm::dot( norm, direction );
						// if (rayNormDot < 0) rayNormDot *= -1; // JB: double sided triangle?
						float r             = triRayNormDot / rayNormDot;
						if ( fabs(rayNormDot) < 0.0000001 ) {
							Log("fabs == 0?");
							if ( triRayNormDot == 0 ) {
								hits.push_back({ tri, 0 });
							}
							continue; // ray disjoint from plane
						}


						if ( r < 0.0f ) continue; // ray points away from triangle
						glm::vec3 intersection = position + r * direction; // intersection of ray & plane

						// is intersection inside triangle?
						float uu, uv, vv, wu, wv, D;
						uu = glm::dot( u, u );
						uv = glm::dot( u, v );
						vv = glm::dot( v, v );
						glm::vec3 w  = intersection - v0;
						wu = glm::dot( w, u );
						wv = glm::dot( w, v );
						D = uv * uv - uu * vv;

						float s, t;
						s = ( uv * wv - vv * wu ) / D;
						if ( s < 0.0 || s > 1.0 ) {
							// Log("  outside s");
							continue; // intersection outside triangle
						}
						t = ( uv * wu - uu * wv ) / D;
						if ( t < 0.0 || ( s + t ) > 1.0 ) {
							// Log("  outside t");
							continue; // intersection outside triangle
						}
						float distance = glm::distance( intersection, position );
						hits.push_back({ tri, distance });
					}
				}

				cur_chunk = cur_chunk->right;
			}

			bottom_back_left = bottom_back_left->infront;
		}

		far_bottom_back_left = far_bottom_back_left->above;
	}

	if ( hits.empty() ) {
		return 0;
	}

	Tri* closest_tri = hits.front().first;
	float closest_hit = hits.front().second;
	for ( auto hit : hits ) {
		if ( hit.second < closest_hit ) {
			closest_tri = hit.first;
			closest_hit = hit.second;
		}
	}

	return closest_tri;
}
// ============================================== //


// ============================================== //
void Terrain::selectTri(Tri* tri) {
	int i=0;
	for ( auto selectionClass : selection->selections ) {
		if ( selectionClass->class_id == TerrainSelection::SelectionClass::CLASS_HIGHLIGHT ||
			 selectionClass->class_id == TerrainSelection::SelectionClass::CLASS_HIGHLIGHT_NEIGHBOUR ) {
			delete selectionClass;
			selection->selections.erase( selection->selections.begin() + i );
		} else {
			++i;
		}
	}

	iTriangle triangle = tri->chunk->triangleBuffer[tri->triIndex];

	// Select main triangles
	TerrainSelection::SelectionClass* selectionClass = new TerrainSelection::SelectionClass();
	selectionClass->class_id = TerrainSelection::SelectionClass::CLASS_HIGHLIGHT;
	selectionClass->refChunk = tri->chunk;
	selectionClass->refTri_id = tri->triIndex;
	selectionClass->triangles.push_back( triangle );
	selection->selections.push_back( selectionClass );

	// Select neighboured tri's
	selectionClass = new TerrainSelection::SelectionClass();
	selectionClass->class_id = TerrainSelection::SelectionClass::CLASS_HIGHLIGHT_NEIGHBOUR;
	if ( tri->neighbour_p0p1 ) selectionClass->triangles.push_back( tri->neighbour_p0p1->chunk->triangleBuffer[tri->neighbour_p0p1->triIndex] );
	if ( tri->neighbour_p1p2 ) selectionClass->triangles.push_back( tri->neighbour_p1p2->chunk->triangleBuffer[tri->neighbour_p1p2->triIndex] );
	if ( tri->neighbour_p2p0 ) selectionClass->triangles.push_back( tri->neighbour_p2p0->chunk->triangleBuffer[tri->neighbour_p2p0->triIndex] );
	if ( !selectionClass->triangles.empty() ) selection->selections.push_back( selectionClass );
}
// ============================================== //


// ============================================== //
void Terrain::CSG(glm::vec3 position, glm::vec3 intersection, Tri* hitTri) {

	vector<Tri*> trisInCSG;
	float radius = 60;
	std::function<void(Tri*)> checkTri = [&](Tri* tri)->void{
		Vertex v = vertexBuffer[tri->p0];
		glm::vec3 p0 = glm::vec3( v.v_x, v.v_y, v.v_z );
		v = vertexBuffer[tri->p1];
		glm::vec3 p1 = glm::vec3( v.v_x, v.v_y, v.v_z );
		v = vertexBuffer[tri->p2];
		glm::vec3 p2 = glm::vec3( v.v_x, v.v_y, v.v_z );

		if ( glm::distance( intersection, p0 ) <= radius ||
			 glm::distance( intersection, p1 ) <= radius ||
			 glm::distance( intersection, p2 ) <= radius ) {
			trisInCSG.push_back( tri );

			vector<Tri*> neighbours;
			if ( tri->neighbour_p0p1 ) neighbours.push_back( tri->neighbour_p0p1 );
			if ( tri->neighbour_p1p2 ) neighbours.push_back( tri->neighbour_p1p2 );
			if ( tri->neighbour_p2p0 ) neighbours.push_back( tri->neighbour_p2p0 );
			for ( auto neighbour : neighbours ) {
				bool found_tri = false;
				for ( auto e_tri : trisInCSG ) {
					if ( e_tri == neighbour ) {
						found_tri = true;
						break;
					}
				}
				if ( !found_tri ) {
					checkTri( neighbour );
				}
			}
		}
	};
	checkTri(hitTri);

	TerrainSelection::SelectionClass* selectionClass = new TerrainSelection::SelectionClass();
	selectionClass->class_id = TerrainSelection::SelectionClass::CLASS_HIGHLIGHT_NEIGHBOUR;
	selectionClass->refChunk = hitTri->chunk;
	selectionClass->refTri_id = hitTri->triIndex;
	for ( auto tri : trisInCSG ) {
		selectionClass->triangles.push_back( tri->chunk->triangleBuffer[tri->triIndex] );
	}
	selection->selections.push_back( selectionClass );


	// TODO: select ALL tri's (separate from those not linked with hitTri)
	// TODO: find opening normal (norm of CSG object; dot(norm, triNorm) < 0 --> remove tri of CSG obj)

	// TODO: tri -> plane ;; sphere / plane -> circle ;; circle -> tri ;; discrete points (error threshold)
	// NOTE: plug sphere equation INTO plane equation!
	Triangle oneTri( vertexBuffer[hitTri->p0], vertexBuffer[hitTri->p1], vertexBuffer[hitTri->p2] );
	Plane* triPlane = new Plane( oneTri );
	Sphere* sphere = new Sphere();
	Circle* circle = intersect( triPlane, sphere );
}
// ============================================== //


	/******************************************************************************/
	/*
							  Terrain Journaling System
																				  */
	/******************************************************************************/


// ============================================== //
void Terrain::operation_removeTri(JournalEntry::JournalEntryOp* operation) {
	Log(str(format("	Deleting a tri.. (%1%) <%2%,%3%,%4%>")%operation->id[0]%
				operation->id[2]%operation->id[3]%operation->id[4]));

	// remove a tri
	Chunk* chunk = chunkList[operation->id[1]];
	Tri* tri = 0;
	vector<EdgeTriTree::EdgeChunk*> containers = EdgeTriTree::EdgeTriNode::getContainers( operation->id[2], operation->id[3] );
	EdgeTriTree::EdgeTriNode* edge = 0;

	// find first edge of triangle
	for ( auto container : containers ) {
		for ( auto e_edge : container->edges ) {
			if ( Tri::oneOrTheOther( operation->id[2], operation->id[3], e_edge->p0, e_edge->p1 ) ) {
				edge = e_edge;
				break;
			}
		}
		if ( edge ) break;
	}

	// find triangle on edge
	if ( edge->triangle_p0p1 && edge->triangle_p0p1->getOddPoint( operation->id[2], operation->id[3] ) ==
			operation->id[4] ) {
		tri = edge->triangle_p0p1;
		edge->triangle_p0p1 = 0;
	} else {
		tri = edge->triangle_p1p0;
		edge->triangle_p1p0 = 0;
		assert( tri->getOddPoint( operation->id[2], operation->id[3] ) == operation->id[4] );
	}

	// un-attach from edges
	for ( auto e_edge : edge->p0_edges ) {
		if ( tri->oneOrTheOther( edge->p0, operation->id[4], e_edge->p0, e_edge->p1 ) ) {
			if ( e_edge->triangle_p0p1 &&
					e_edge->triangle_p0p1->triIndex == tri->triIndex && e_edge->triangle_p0p1->chunk == tri->chunk ) {
				assert(e_edge->triangle_p0p1 == tri);
				e_edge->triangle_p0p1 = 0;
			} else {
				assert( e_edge->triangle_p1p0 && e_edge->triangle_p1p0->triIndex == tri->triIndex );
				assert(e_edge->triangle_p1p0 == tri);
				e_edge->triangle_p1p0 = 0;
			}
		}
	}
	for ( auto e_edge : edge->p1_edges ) {
		if ( tri->oneOrTheOther( edge->p1, operation->id[4], e_edge->p0, e_edge->p1 ) ) {
			if ( e_edge->triangle_p0p1 &&
					e_edge->triangle_p0p1->triIndex == tri->triIndex && e_edge->triangle_p0p1->chunk == tri->chunk ) {
				assert(e_edge->triangle_p0p1 == tri);
				e_edge->triangle_p0p1 = 0;
			} else {
				assert( e_edge->triangle_p1p0 && e_edge->triangle_p1p0->triIndex == tri->triIndex );
				assert(e_edge->triangle_p1p0 == tri);
				e_edge->triangle_p1p0 = 0;
			}
		}
	}


	// un-neighbour tri
	if ( tri->neighbour_p0p1 ) (*tri->neighbour_p0p1->getNeighbourOnEdge( tri->p0, tri->p1 )) = 0;
	if ( tri->neighbour_p1p2 ) (*tri->neighbour_p1p2->getNeighbourOnEdge( tri->p1, tri->p2 )) = 0;
	if ( tri->neighbour_p2p0 ) (*tri->neighbour_p2p0->getNeighbourOnEdge( tri->p2, tri->p0 )) = 0;


	// remove triangle
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, chunk->elementBuffer );
	void * data = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
	chunk->triangleBuffer.erase(chunk->triangleBuffer.begin() + operation->id[0]);
	memcpy( data, chunk->triangleBuffer.data(), chunk->triangleBuffer.size()*sizeof(chunk->triangleBuffer[0]) );
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

}
// ============================================== //


// ============================================== //
void Terrain::operation_addTri(JournalEntry::JournalEntryOp* operation) {
	Log(str(format("	Adding a tri.. (%1%) <%2%,%3%,%4%>")%operation->id[0]%
				operation->id[2]%operation->id[3]%operation->id[4]));

	// Add a tri
	Chunk* chunk = chunkList[operation->id[1]];
	Tri* tri;
	vector<EdgeTriTree::EdgeChunk*> containers = EdgeTriTree::EdgeTriNode::getContainers( operation->id[2], operation->id[3] );
	EdgeTriTree::EdgeTriNode* edge = 0;

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, chunk->elementBuffer );
	void * data = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
	// NOTE: cannot insert in case this index is bigger than the size of the triangleBuffer..
	while ( operation->id[0] > chunk->triangleBuffer.size() ) {
		// create placeholder at index
		chunk->triangleBuffer.push_back( iTriangle(0,0,0) );
	}

	if ( operation->id[0] == chunk->triangleBuffer.size() ) {
		chunk->triangleBuffer.push_back( iTriangle( operation->id[2], operation->id[3], operation->id[4] ) );
	} else {
		chunk->triangleBuffer.at(operation->id[0]) = iTriangle( operation->id[2], operation->id[3], operation->id[4] );
	}

	// chunk->triangleBuffer.insert(chunk->triangleBuffer.begin() + operation->id[0],
	// 		iTriangle( operation->id[2], operation->id[3], operation->id[4] ));
	memcpy( data, chunk->triangleBuffer.data(), chunk->triangleBuffer.size()*sizeof(chunk->triangleBuffer[0]) );
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
	tri = new Tri( chunk, operation->id[0] );

	// find first edge of triangle
	for ( auto container : containers ) {
		for ( auto e_edge : container->edges ) {
			if ( Tri::oneOrTheOther( operation->id[2], operation->id[3], e_edge->p0, e_edge->p1 ) ) {
				edge = e_edge;
				break;
			}
		}
		if ( edge ) break;
	}

	// attach tri to edge
	bool triFlipped;
	if ( !edge->triangle_p0p1 && edge->p0 == operation->id[2] && edge->p1 == operation->id[3] ) {
		triFlipped = false;
		edge->triangle_p0p1 = tri;
		tri->neighbour_p0p1 = edge->triangle_p1p0;
		Log(str(format("		Attached tri to edge {%1%,%2%}")%edge->p0%edge->p1));
	} else {
		triFlipped = true;
		assert(!edge->triangle_p1p0);
		edge->triangle_p1p0 = tri;
		assert( edge->p1 == operation->id[2] && edge->p0 == operation->id[3] );
		tri->neighbour_p0p1 = edge->triangle_p0p1;
		Log(str(format("		Attached tri to edge {%1%,%2%}")%edge->p0%edge->p1));
	}

	// attach tri to other edges
	for ( auto e_edge : edge->p0_edges ) {
		if ( tri->oneOrTheOther( edge->p0, operation->id[4], e_edge->p0, e_edge->p1 ) ) {
			Tri** triSide = e_edge->getTriSide(tri);
			assert(triSide);
			assert(!(*triSide));
			(*triSide) = tri;
			Log(str(format("		Attached tri to edge {%1%,%2%}")%e_edge->p0%e_edge->p1));

			if (triSide == &e_edge->triangle_p0p1) {
				if (triFlipped) tri->neighbour_p1p2 = e_edge->triangle_p1p0;
				else tri->neighbour_p2p0 = e_edge->triangle_p1p0;
			} else {
				if (triFlipped) tri->neighbour_p1p2 = e_edge->triangle_p0p1;
				else tri->neighbour_p2p0 = e_edge->triangle_p0p1;
			}


			// if ( !e_edge->triangle_p0p1 && e_edge->p0 == edge->p0 && e_edge->p1 == operation->id[4] ) {
			// 	e_edge->triangle_p0p1 = tri;
			// 	if (triFlipped) tri->neighbour_p1p2 = e_edge->triangle_p1p0;
			// 	else tri->neighbour_p2p0 = e_edge->triangle_p1p0;
			// 	Log(str(format("		Attached tri to edge {%1%,%2%}")%e_edge->p0%e_edge->p1));
			// } else {
			// 	assert(!e_edge->triangle_p1p0);
			// 	e_edge->triangle_p1p0 = tri;
			// 	assert( e_edge->p1 == edge->p0 && e_edge->p0 == operation->id[4] );
			// 	if (triFlipped) tri->neighbour_p1p2 = e_edge->triangle_p0p1;
			// 	else tri->neighbour_p2p0 = e_edge->triangle_p0p1;
			// 	Log(str(format("		Attached tri to edge {%1%,%2%}")%e_edge->p0%e_edge->p1));
			// }
			break;
		}
	}
	for ( auto e_edge : edge->p1_edges ) {
		if ( tri->oneOrTheOther( edge->p1, operation->id[4], e_edge->p0, e_edge->p1 ) ) {
			Tri** triSide = e_edge->getTriSide(tri);
			assert(triSide);
			assert(!(*triSide));
			(*triSide) = tri;
			Log(str(format("		Attached tri to edge {%1%,%2%}")%e_edge->p0%e_edge->p1));

			if (triSide == &e_edge->triangle_p0p1) {
				if (triFlipped) tri->neighbour_p2p0 = e_edge->triangle_p1p0;
				else tri->neighbour_p1p2 = e_edge->triangle_p1p0;
			} else {
				if (triFlipped) tri->neighbour_p2p0 = e_edge->triangle_p0p1;
				else tri->neighbour_p1p2 = e_edge->triangle_p0p1;
			}


			// if ( !e_edge->triangle_p0p1 && e_edge->p0 == edge->p1 && e_edge->p1 == operation->id[4] ) {
			// 	e_edge->triangle_p0p1 = tri;
			// 	if (triFlipped) tri->neighbour_p2p0 = e_edge->triangle_p1p0;
			// 	else tri->neighbour_p1p2 = e_edge->triangle_p1p0;
			// 	Log(str(format("		Attached tri to edge {%1%,%2%}")%e_edge->p0%e_edge->p1));
			// } else {
			// 	assert(!e_edge->triangle_p1p0);
			// 	e_edge->triangle_p1p0 = tri;
			// 	assert( e_edge->p1 == edge->p1 && e_edge->p0 == operation->id[4] );
			// 	if (triFlipped) tri->neighbour_p2p0 = e_edge->triangle_p0p1;
			// 	else tri->neighbour_p1p2 = e_edge->triangle_p0p1;
			// 	Log(str(format("		Attached tri to edge {%1%,%2%}")%e_edge->p0%e_edge->p1));
			// }
			break;
		}
	}


	// neighbour tri
	if ( tri->neighbour_p0p1 ) (*tri->neighbour_p0p1->getNeighbourOnEdge( tri->p0, tri->p1 )) = tri;
	if ( tri->neighbour_p1p2 ) (*tri->neighbour_p1p2->getNeighbourOnEdge( tri->p1, tri->p2 )) = tri;
	if ( tri->neighbour_p2p0 ) (*tri->neighbour_p2p0->getNeighbourOnEdge( tri->p2, tri->p0 )) = tri;



}
// ============================================== //


// ============================================== //
void Terrain::operation_reshapeTri(JournalEntry::JournalEntryOp* operation) {
	Log(str(format("	Reshaping a tri.. <%1%,%2%,%3%> TO <%4%,%5%,%6%>")%
				operation->id[2]%operation->id[3]%operation->id[4]%
				operation->id[5]%operation->id[6]%operation->id[7]));

	// A tri could be reshaped on 1 or 2 verts
	// Need to find all edges (2 or 3) affected by change; those that contain the changed verts
	// For each edge: remove tri, un-neighbour tri; find replacement edge, add tri, neighbour tri
	// Update tri & triangle

	// Find all edges which are affected by changed verts
	// AffectedEdge: {old edge-pair, new edge-pair}(edge, containers)
	typedef pair<pair<EdgeTriTree::EdgeTriNode*, vector<EdgeTriTree::EdgeChunk*>>,
			pair<EdgeTriTree::EdgeTriNode*, vector<EdgeTriTree::EdgeChunk*>>> AffectedEdge ;
	uchar changed_edges = 0;
	const static uchar CHANGED_EDGE_p0p1 = 1<<0;
	const static uchar CHANGED_EDGE_p1p2 = 1<<1;
	const static uchar CHANGED_EDGE_p2p0 = 1<<2;
	AffectedEdge* changed_edge_p0p1 = 0;
	AffectedEdge* changed_edge_p1p2 = 0;
	AffectedEdge* changed_edge_p2p0 = 0;
	ushort old_p0 = operation->id[2];
	ushort old_p1 = operation->id[3];
	ushort old_p2 = operation->id[4];
	ushort new_p0 = operation->id[5];
	ushort new_p1 = operation->id[6];
	ushort new_p2 = operation->id[7];
	Tri* tri = 0;
	if ( old_p0 != new_p0 ) {
		// p0 changed
		changed_edges |= CHANGED_EDGE_p0p1;
		changed_edges |= CHANGED_EDGE_p2p0;
	}
	if ( old_p1 != new_p1 ) {
		// p1 changed
		changed_edges |= CHANGED_EDGE_p0p1;
		changed_edges |= CHANGED_EDGE_p1p2;
	}
	if ( old_p2 != new_p2 ) {
		// p2 changed
		changed_edges |= CHANGED_EDGE_p1p2;
		changed_edges |= CHANGED_EDGE_p2p0;
	}

	if ( changed_edges & CHANGED_EDGE_p0p1 ) {

		EdgeTriTree::EdgeTriNode* old_edge;
		EdgeTriTree::EdgeTriNode* new_edge;
		vector<EdgeTriTree::EdgeChunk*> old_edge_containers, new_edge_containers;

		old_edge_containers = EdgeTriTree::EdgeTriNode::getContainers( old_p0, old_p1 );
		new_edge_containers = EdgeTriTree::EdgeTriNode::getContainers( new_p0, new_p1 );
		for ( auto container : old_edge_containers ) {
			for ( auto e_edge : container->edges ) {
				if ( Tri::oneOrTheOther( old_p0, old_p1, e_edge->p0, e_edge->p1 ) ) {
					old_edge = e_edge;
					break;
				}
			}
			if ( old_edge ) break;
		}
		for ( auto container : new_edge_containers ) {
			for ( auto e_edge : container->edges ) {
				if ( Tri::oneOrTheOther( new_p0, new_p1, e_edge->p0, e_edge->p1 ) ) {
					new_edge = e_edge;
					if ( new_edge->triangle_p0p1 &&
							new_edge->triangle_p0p1->getOddPoint( new_p0, new_p1 ) == new_p2 ) {
						tri = new_edge->triangle_p0p1;
					} else {
						tri = new_edge->triangle_p1p0;
						assert( tri->getOddPoint( new_p0, new_p1 ) == new_p2 );
					}
					break;
				}
			}
			if ( old_edge ) break;
		}
		changed_edge_p0p1 = new AffectedEdge({
				{ old_edge, old_edge_containers }, // old edge-pair
				{ new_edge, new_edge_containers }  // new edge-pair
				});

	}
	if ( changed_edges & CHANGED_EDGE_p1p2 ) {

		EdgeTriTree::EdgeTriNode* old_edge;
		EdgeTriTree::EdgeTriNode* new_edge;
		vector<EdgeTriTree::EdgeChunk*> old_edge_containers, new_edge_containers;

		old_edge_containers = EdgeTriTree::EdgeTriNode::getContainers( old_p1, old_p2 );
		new_edge_containers = EdgeTriTree::EdgeTriNode::getContainers( new_p1, new_p2 );
		for ( auto container : old_edge_containers ) {
			for ( auto e_edge : container->edges ) {
				if ( Tri::oneOrTheOther( old_p1, old_p2, e_edge->p0, e_edge->p1 ) ) {
					old_edge = e_edge;
					break;
				}
			}
			if ( old_edge ) break;
		}
		for ( auto container : new_edge_containers ) {
			for ( auto e_edge : container->edges ) {
				if ( Tri::oneOrTheOther( new_p1, new_p2, e_edge->p0, e_edge->p1 ) ) {
					new_edge = e_edge;
					if ( !tri ) {
						if ( new_edge->triangle_p0p1 &&
								new_edge->triangle_p0p1->getOddPoint( new_p1, new_p2 ) == new_p0 ) {
							tri = new_edge->triangle_p0p1;
						} else {
							tri = new_edge->triangle_p1p0;
							assert( tri->getOddPoint( new_p1, new_p2 ) == new_p0 );
						}
					}
					break;
				}
			}
			if ( old_edge ) break;
		}
		changed_edge_p1p2 = new AffectedEdge({
				{ old_edge, old_edge_containers }, // old edge-pair
				{ new_edge, new_edge_containers }  // new edge-pair
				});

	}
	if ( changed_edges & CHANGED_EDGE_p2p0 ) {

		EdgeTriTree::EdgeTriNode* old_edge;
		EdgeTriTree::EdgeTriNode* new_edge;
		vector<EdgeTriTree::EdgeChunk*> old_edge_containers, new_edge_containers;

		old_edge_containers = EdgeTriTree::EdgeTriNode::getContainers( old_p2, old_p0 );
		new_edge_containers = EdgeTriTree::EdgeTriNode::getContainers( new_p2, new_p0 );
		for ( auto container : old_edge_containers ) {
			for ( auto e_edge : container->edges ) {
				if ( Tri::oneOrTheOther( old_p2, old_p0, e_edge->p0, e_edge->p1 ) ) {
					old_edge = e_edge;
					break;
				}
			}
			if ( old_edge ) break;
		}
		for ( auto container : new_edge_containers ) {
			for ( auto e_edge : container->edges ) {
				if ( Tri::oneOrTheOther( new_p2, new_p0, e_edge->p0, e_edge->p1 ) ) {
					new_edge = e_edge;
					if ( !tri ) {
						if ( new_edge->triangle_p0p1 &&
								new_edge->triangle_p0p1->getOddPoint( new_p2, new_p0 ) == new_p1 ) {
							tri = new_edge->triangle_p0p1;
						} else {
							tri = new_edge->triangle_p1p0;
							assert( tri->getOddPoint( new_p2, new_p0 ) == new_p1 );
						}
					}
					break;
				}
			}
			if ( old_edge ) break;
		}
		changed_edge_p2p0 = new AffectedEdge({
				{ old_edge, old_edge_containers }, // old edge-pair
				{ new_edge, new_edge_containers }  // new edge-pair
				});

	}

	assert(tri);

	// For each edge: remove tri, un-neighbour tri; find replacement edge, add tri, neighbour tri
	if ( changed_edge_p0p1 ) {
		// Remove tri from edge, un-neighbour tri
		EdgeTriTree::EdgeTriNode* old_edge = changed_edge_p0p1->first.first;
		EdgeTriTree::EdgeTriNode* new_edge = changed_edge_p0p1->second.first;

		// un-attach tri from new edge
		if ( new_edge->triangle_p0p1 == tri ) {
			new_edge->triangle_p0p1 = 0;
		} else {
			assert(new_edge->triangle_p1p0 == tri);
			new_edge->triangle_p1p0 = 0;
		}

		// un-neighbour tri on edge
		if (*tri->getNeighbourOnEdge( new_p0, new_p1 )) {
			(*(*tri->getNeighbourOnEdge( new_p0, new_p1 ))->getNeighbourOnEdge( new_p0, new_p1 )) = 0;
			(*tri->getNeighbourOnEdge( new_p0, new_p1 )) = 0;
		}

		// attach tri to old edge, neighbour tri
		if (  ( old_p0 == old_edge->p0 && old_p1 == old_edge->p1 ) ||
				( old_p1 == old_edge->p0 && old_p2 == old_edge->p1 ) ||
				( old_p2 == old_edge->p0 && old_p0 == old_edge->p1 ) ) {
			old_edge->triangle_p0p1 = tri;
			if ( old_edge->triangle_p1p0 ) {
				(*tri->getNeighbourOnEdge( old_edge->p0, old_edge->p1 )) = old_edge->triangle_p1p0;
				(*(*tri->getNeighbourOnEdge( old_edge->p0, old_edge->p1 ))->getNeighbourOnEdge( old_edge->p0, old_edge->p1 )) = tri;
			}
		} else {
			assert( ( old_p0 == old_edge->p1 && old_p1 == old_edge->p0 ) ||
					( old_p1 == old_edge->p1 && old_p2 == old_edge->p0 ) ||
					( old_p2 == old_edge->p1 && old_p0 == old_edge->p0 ) );
			old_edge->triangle_p1p0 = tri;
			if ( old_edge->triangle_p0p1 ) {
				(*tri->getNeighbourOnEdge( old_edge->p0, old_edge->p1 )) = old_edge->triangle_p0p1;
				(*(*tri->getNeighbourOnEdge( old_edge->p0, old_edge->p1 ))->getNeighbourOnEdge( old_edge->p0, old_edge->p1 )) = tri;
			}
		}

	}
	if ( changed_edge_p1p2 ) {
		// Remove tri from edge, un-neighbour tri
		EdgeTriTree::EdgeTriNode* old_edge = changed_edge_p1p2->first.first;
		EdgeTriTree::EdgeTriNode* new_edge = changed_edge_p1p2->second.first;

		// un-attach tri from new edge
		if ( new_edge->triangle_p0p1 == tri ) {
			new_edge->triangle_p0p1 = 0;
		} else {
			assert(new_edge->triangle_p1p0 == tri);
			new_edge->triangle_p1p0 = 0;
		}

		// un-neighbour tri on edge
		if (*tri->getNeighbourOnEdge( new_p0, new_p1 )) {
			(*(*tri->getNeighbourOnEdge( new_p0, new_p1 ))->getNeighbourOnEdge( new_p0, new_p1 )) = 0;
			(*tri->getNeighbourOnEdge( new_p0, new_p1 )) = 0;
		}

		// attach tri to old edge, neighbour tri
		if (  ( old_p0 == old_edge->p0 && old_p1 == old_edge->p1 ) ||
				( old_p1 == old_edge->p0 && old_p2 == old_edge->p1 ) ||
				( old_p2 == old_edge->p0 && old_p0 == old_edge->p1 ) ) {
			old_edge->triangle_p0p1 = tri;
			if ( old_edge->triangle_p1p0 ) {
				(*tri->getNeighbourOnEdge( old_edge->p0, old_edge->p1 )) = old_edge->triangle_p1p0;
				(*(*tri->getNeighbourOnEdge( old_edge->p0, old_edge->p1 ))->getNeighbourOnEdge( old_edge->p0, old_edge->p1 )) = tri;
			}
		} else {
			assert( ( old_p0 == old_edge->p1 && old_p1 == old_edge->p0 ) ||
					( old_p1 == old_edge->p1 && old_p2 == old_edge->p0 ) ||
					( old_p2 == old_edge->p1 && old_p0 == old_edge->p0 ) );
			old_edge->triangle_p1p0 = tri;
			if ( old_edge->triangle_p0p1 ) {
				(*tri->getNeighbourOnEdge( old_edge->p0, old_edge->p1 )) = old_edge->triangle_p0p1;
				(*(*tri->getNeighbourOnEdge( old_edge->p0, old_edge->p1 ))->getNeighbourOnEdge( old_edge->p0, old_edge->p1 )) = tri;
			}
		}

	}
	if ( changed_edge_p2p0 ) {
		// Remove tri from edge, un-neighbour tri
		EdgeTriTree::EdgeTriNode* old_edge = changed_edge_p2p0->first.first;
		EdgeTriTree::EdgeTriNode* new_edge = changed_edge_p2p0->second.first;

		// un-attach tri from new edge
		if ( new_edge->triangle_p0p1 == tri ) {
			new_edge->triangle_p0p1 = 0;
		} else {
			assert(new_edge->triangle_p1p0 == tri);
			new_edge->triangle_p1p0 = 0;
		}

		// un-neighbour tri on edge
		if (*tri->getNeighbourOnEdge( new_p0, new_p1 )) {
			(*(*tri->getNeighbourOnEdge( new_p0, new_p1 ))->getNeighbourOnEdge( new_p0, new_p1 )) = 0;
			(*tri->getNeighbourOnEdge( new_p0, new_p1 )) = 0;
		}

		// attach tri to old edge, neighbour tri
		if (  ( old_p0 == old_edge->p0 && old_p1 == old_edge->p1 ) ||
				( old_p1 == old_edge->p0 && old_p2 == old_edge->p1 ) ||
				( old_p2 == old_edge->p0 && old_p0 == old_edge->p1 ) ) {
			old_edge->triangle_p0p1 = tri;
			if ( old_edge->triangle_p1p0 ) {
				(*tri->getNeighbourOnEdge( old_edge->p0, old_edge->p1 )) = old_edge->triangle_p1p0;
				(*(*tri->getNeighbourOnEdge( old_edge->p0, old_edge->p1 ))->getNeighbourOnEdge( old_edge->p0, old_edge->p1 )) = tri;
			}
		} else {
			assert( ( old_p0 == old_edge->p1 && old_p1 == old_edge->p0 ) ||
					( old_p1 == old_edge->p1 && old_p2 == old_edge->p0 ) ||
					( old_p2 == old_edge->p1 && old_p0 == old_edge->p0 ) );
			old_edge->triangle_p1p0 = tri;
			if ( old_edge->triangle_p0p1 ) {
				(*tri->getNeighbourOnEdge( old_edge->p0, old_edge->p1 )) = old_edge->triangle_p0p1;
				(*(*tri->getNeighbourOnEdge( old_edge->p0, old_edge->p1 ))->getNeighbourOnEdge( old_edge->p0, old_edge->p1 )) = tri;
			}
		}

	}

	// Reshape the tri
	tri->p0 = old_p0;
	tri->p1 = old_p1;
	tri->p2 = old_p2;
	Chunk* chunk = chunkList[operation->id[1]];
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, chunk->elementBuffer );
	void * data = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
	chunk->triangleBuffer.at( operation->id[0] ) = iTriangle( old_p0, old_p1, old_p2 );
	memcpy( data, chunk->triangleBuffer.data(), chunk->triangleBuffer.size()*sizeof(chunk->triangleBuffer[0]) );
	glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);




}
// ============================================== //


// ============================================== //
void Terrain::operation_addVert(JournalEntry::JournalEntryOp* operation) {
	Log(str(format("	Adding a vert..")));

	// adding vert
	glBindBuffer( GL_ARRAY_BUFFER, vbo );
	void * data = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	vertexBuffer.insert( vertexBuffer.begin() + operation->id[0], Vertex( 
				operation->id[1], operation->id[2], operation->id[3] ) );
	memcpy( data, vertexBuffer.data(), vertexBuffer.size()*sizeof(vertexBuffer[0]) );
	glUnmapBuffer(GL_ARRAY_BUFFER);
}
// ============================================== //


// ============================================== //
void Terrain::operation_removeVert(JournalEntry::JournalEntryOp* operation) {
	Log(str(format("	Deleting a vert..")));

	// delete vert
	glBindBuffer( GL_ARRAY_BUFFER, vbo );
	void * data = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
	vertexBuffer.erase(vertexBuffer.begin() + operation->id[0]);
	memcpy( data, vertexBuffer.data(), vertexBuffer.size()*sizeof(vertexBuffer[0]) );
	glUnmapBuffer(GL_ARRAY_BUFFER);
}
// ============================================== //


// ============================================== //
void Terrain::operation_addEdge(JournalEntry::JournalEntryOp* operation) {
	Log(str(format("	Adding an edge.. (%1%,%2%)")%operation->id[0]%operation->id[1]));
	ushort p0 = operation->id[0];
	ushort p1 = operation->id[1];

				vector<EdgeTriTree::EdgeChunk*> containers   = EdgeTriTree::EdgeTriNode::getContainers(p0, p1); // all containers containing the edge(s)
				vector<EdgeTriTree::EdgeChunk*> p0Containers = EdgeTriTree::EdgeTriNode::getContainer( p0 ); 
				vector<EdgeTriTree::EdgeChunk*> p1Containers = EdgeTriTree::EdgeTriNode::getContainer( p1 );
				EdgeTriTree::EdgeChunk* p0Container = p0Containers.front();
				EdgeTriTree::EdgeChunk* p1Container = p1Containers.front();


				EdgeTriTree::EdgeTriNode* edge = new EdgeTriTree::EdgeTriNode();
				edge->p0 = p0;
				edge->p1 = p1;

				// Connected edge to graph
				Log(str(format("		Searching for Connections for {%1%,%2%}(p0)")% p0 % p1));
				for ( auto some_edge : p0Container->edges ) {
					if ( some_edge->p0 == p0 ) {
						some_edge->p0_edges.push_back( edge );
						edge->p0_edges.push_back( some_edge );
						Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
					} else if ( some_edge->p1 == p0 ) {
						some_edge->p1_edges.push_back( edge );
						edge->p0_edges.push_back( some_edge );
						Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
					}
				}
				Log(str(format("		Searching for Connections for {%1%,%2%}(p1)")% p0 % p1));
				for ( auto some_edge : p1Container->edges ) {
					if ( some_edge->p0 == p1 ) {
						some_edge->p0_edges.push_back( edge );
						edge->p1_edges.push_back( some_edge );
						Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
					} else if ( some_edge->p1 == p1 ) {
						some_edge->p1_edges.push_back( edge );
						edge->p1_edges.push_back( some_edge );
						Log(str(format("			Connected with: {%1%,%2%}")% some_edge->p0 % some_edge->p1));
					}
				}


				// Add edge to each chunk
				for ( auto container : containers ) {
					container->edges.push_back( edge );

					bool foundEdge = false;
					for ( auto e_edge : container->edges ) {
						if ( e_edge == edge ) {
							foundEdge = true;
							Log(str(format("		Edge has definitely been added to container: (%1%,%2%,%3%) [%4%]")%
										container->worldOffset.x % container->worldOffset.y % container->worldOffset.z %
										container ));
							break;
						}
					}
					if ( !foundEdge ) raise(SIGTRAP);
				}
				/*
	// Add edge
	vector<EdgeTriTree::EdgeChunk*> containers = EdgeTriTree::EdgeTriNode::getContainers( operation->id[0], operation->id[1] );
	EdgeTriTree::EdgeTriNode* edge = new EdgeTriTree::EdgeTriNode();
	edge->p0 = operation->id[0];
	edge->p1 = operation->id[1];

	// add edge to containers; attach to other edges
	int count = 0;
	for ( auto container : containers ) {
		for ( auto e_edge : container->edges ) {
			if ( Tri::oneOrTheOther( e_edge->p0, e_edge->p1, edge->p0, edge->p1 ) ) {
				if ( e_edge->p0 == edge->p0 ) {
					edge->p0_edges.push_back( e_edge );
					e_edge->p0_edges.push_back( edge );
				} else if ( e_edge->p0 == edge->p1 ) {
					edge->p1_edges.push_back( e_edge );
					e_edge->p0_edges.push_back( edge );
				} else if ( e_edge->p1 == edge->p0 ) {
					edge->p0_edges.push_back( e_edge );
					e_edge->p1_edges.push_back( edge );
				} else if ( e_edge->p1 == edge->p1 ) {
					edge->p1_edges.push_back( e_edge );
					e_edge->p1_edges.push_back( edge );
				} else {
					raise(SIGTRAP);
				}
				++count;
			} else {
				Log(str(format("		edge NOT neighboured with (%1%,%2%)")%operation->id[0]%operation->id[1]));
			}
		}
		container->edges.push_back( edge );
	}
	if ( count < 2 ) raise(SIGTRAP);

	*/

}
// ============================================== //


// ============================================== //
void Terrain::operation_removeEdge(JournalEntry::JournalEntryOp* operation) {
	Log(str(format("	Deleting an edge..  (%1%,%2%)")%operation->id[0]%operation->id[1]));

	// delete edge
	vector<EdgeTriTree::EdgeChunk*> containers = EdgeTriTree::EdgeTriNode::getContainers( operation->id[0], operation->id[1] );
	EdgeTriTree::EdgeTriNode* edge = 0;

	// find edge and remove from each container
	for ( auto container : containers ) {
		int position = 0;
		for ( auto e_edge : container->edges ) {
			if ( Tri::oneOrTheOther( operation->id[0], operation->id[1], e_edge->p0, e_edge->p1 ) ) {
				if ( !edge ) edge = e_edge;
				container->edges.erase( container->edges.begin() + position );
				break;
			}
			++position;
		}
	}

	// unattach edge from edge-tree
	vector<vector<EdgeTriTree::EdgeTriNode*>*> neighbours_list;
	neighbours_list.push_back(&edge->p0_edges);
	neighbours_list.push_back(&edge->p1_edges);
	for ( auto neighbours : neighbours_list ) {
		for ( int i = 0; i < (*neighbours).size(); ++i ) {
			// NOTE: using (auto neighbour : (*neighbours)) caused major problems
			EdgeTriTree::EdgeTriNode* neighbour = (*neighbours)[i];
			int position = 0;
			bool removed = false;
			for ( auto e_edge : neighbour->p0_edges ) {
				if ( e_edge == edge ) {
					neighbour->p0_edges.erase( neighbour->p0_edges.begin() + position );
					break;
				}
				++position;
			}

			if ( !removed ) {
				position = 0;
				for ( auto e_edge : neighbour->p1_edges ) {
					if ( e_edge == edge ) {
						neighbour->p1_edges.erase( neighbour->p1_edges.begin() + position );
						break;
					}
					++position;
				}
			}
		}
	}

	delete edge;


}
// ============================================== //


// ============================================== //
void Terrain::undo(JournalEntry* entry) {

	// undo each operation (in reverse order)
	Log("Undo-ing Journal Entry");
	for ( int iOp = entry->operations.size() - 1; iOp >= 0; --iOp ) {
		JournalEntry::JournalEntryOp* operation = entry->operations[iOp];

		if ( operation->entry_type == JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDVERT ) {

			operation_removeVert( operation );

		} else if ( operation->entry_type == JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDEDGE ) {

			operation_removeEdge( operation );

		} else if ( operation->entry_type == JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDTRI ) {

			operation_removeTri(operation);

		} else if ( operation->entry_type == JournalEntry::JournalEntryOp::JOURNAL_ENTRY_REMOVETRI ) {

			operation_addTri(operation);

		} else if ( operation->entry_type == JournalEntry::JournalEntryOp::JOURNAL_ENTRY_REMOVEEDGE ) {

			operation_addEdge( operation );

		} else if ( operation->entry_type == JournalEntry::JournalEntryOp::JOURNAL_ENTRY_RESHAPETRI ) {

			operation_reshapeTri( operation );

		} else {
			raise(SIGTRAP); // um..what?
		}
	}

}
// ============================================== //


// ============================================== //
void Terrain::redo(JournalEntry* entry) {

	// redo each operation
	Log("Redo-ing Journal Entry");
	for ( auto operation : entry->operations ) {
		if ( operation->entry_type == JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDVERT ) {
			operation_addVert( operation );
		} else if ( operation->entry_type == JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDEDGE ) {
			operation_addEdge( operation );
		} else if ( operation->entry_type == JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDTRI ) {
			operation_addTri( operation );
		} else if ( operation->entry_type == JournalEntry::JournalEntryOp::JOURNAL_ENTRY_REMOVETRI ) {
			operation_removeTri( operation );
		} else if ( operation->entry_type == JournalEntry::JournalEntryOp::JOURNAL_ENTRY_REMOVEEDGE ) {
			operation_removeEdge( operation );
		} else if ( operation->entry_type == JournalEntry::JournalEntryOp::JOURNAL_ENTRY_RESHAPETRI ) {
			JournalEntry::JournalEntryOp* reshapeOperation = new JournalEntry::JournalEntryOp();
			reshapeOperation->id = new ushort[8];
			reshapeOperation->id[0] = operation->id[0];
			reshapeOperation->id[1] = operation->id[1];
			reshapeOperation->id[2] = operation->id[5];
			reshapeOperation->id[3] = operation->id[6];
			reshapeOperation->id[4] = operation->id[7];
			reshapeOperation->id[5] = operation->id[2];
			reshapeOperation->id[6] = operation->id[3];
			reshapeOperation->id[7] = operation->id[4];
			operation_reshapeTri( reshapeOperation );
		} else {
			raise(SIGTRAP); // um..what?
		}
	}

}
// ============================================== //


// ============================================== //
JournalEntry::JournalEntryOp* JournalEntry::AddTri(ushort id, ushort chunk, ushort p0, ushort p1, ushort p2) {
	JournalEntry::JournalEntryOp* entry = new JournalEntry::JournalEntryOp();
	entry->entry_type = JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDTRI;
	entry->id = new ushort[5];
	entry->id[0] = id;
	entry->id[1] = chunk;
	entry->id[2] = p0;
	entry->id[3] = p1;
	entry->id[4] = p2;
	return entry;
}
// ============================================== //


// ============================================== //
JournalEntry::JournalEntryOp* JournalEntry::ReshapeTri(ushort id, ushort chunk, ushort old_p0, ushort old_p1, ushort old_p2, ushort p0, ushort p1, ushort p2) {
	JournalEntry::JournalEntryOp* entry = new JournalEntry::JournalEntryOp();
	entry->entry_type = JournalEntry::JournalEntryOp::JOURNAL_ENTRY_RESHAPETRI;
	entry->id = new ushort[8];
	entry->id[0] = id;
	entry->id[1] = chunk;
	entry->id[2] = old_p0;
	entry->id[3] = old_p1;
	entry->id[4] = old_p2;
	entry->id[5] = p0;
	entry->id[6] = p1;
	entry->id[7] = p2;
	Log(str(format("			Added reshape tri journalentry: <%1%,%2%,%3%> TO <%4%,%5%,%6%>")%old_p0%old_p1%old_p2%p0%p1%p2));
	return entry;
}
// ============================================== //


// ============================================== //
JournalEntry::JournalEntryOp* JournalEntry::RemoveTri(ushort id, ushort chunk, ushort p0, ushort p1, ushort p2) {
	JournalEntry::JournalEntryOp* entry = new JournalEntry::JournalEntryOp();
	entry->entry_type = JournalEntry::JournalEntryOp::JOURNAL_ENTRY_REMOVETRI;
	entry->id = new ushort[2];
	entry->id[0] = id;
	entry->id[1] = chunk;
	entry->id[2] = p0;
	entry->id[3] = p1;
	entry->id[4] = p2;
	return entry;
}
// ============================================== //


// ============================================== //
JournalEntry::JournalEntryOp* JournalEntry::AddVert(ushort id, float x, float y, float z) {
	JournalEntry::JournalEntryOp* entry = new JournalEntry::JournalEntryOp();
	entry->entry_type = JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDVERT;
	entry->id = new ushort[1];
	entry->id[0] = id;
	entry->args = new float[3];
	entry->args[0] = x;
	entry->args[1] = y;
	entry->args[2] = z;
	return entry;
}
// ============================================== //


// ============================================== //
JournalEntry::JournalEntryOp* JournalEntry::AddEdge(ushort p0, ushort p1) {
	JournalEntry::JournalEntryOp* entry = new JournalEntry::JournalEntryOp();
	entry->entry_type = JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDEDGE;
	entry->id = new ushort[2];
	entry->id[0] = p0;
	entry->id[1] = p1;
	return entry;
}
// ============================================== //


// ============================================== //
JournalEntry::JournalEntryOp* JournalEntry::RemoveEdge(ushort p0, ushort p1) {
	JournalEntry::JournalEntryOp* entry = new JournalEntry::JournalEntryOp();
	entry->entry_type = JournalEntry::JournalEntryOp::JOURNAL_ENTRY_REMOVEEDGE;
	entry->id = new ushort[2];
	entry->id[0] = p0;
	entry->id[1] = p1;
	return entry;
}
// ============================================== //

