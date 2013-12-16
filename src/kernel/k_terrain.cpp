#include "kernel/k_terrain.h"

const Point<int> Chunk::chunkSize = Point<int>(113,113,113);
const Point<int> EdgeTriTree::EdgeChunk::chunkSize = Point<int>(113,113,113);
// const Point<int> Chunk::chunkSize = Point<int>(501,201,113);
// const Point<int> EdgeTriTree::EdgeChunk::chunkSize = Point<int>(501,201,113);
// const Point<int> Chunk::chunkSize = Point<int>(100,100,100);
// const Point<int> EdgeTriTree::EdgeChunk::chunkSize = Point<int>(100,100,100);

const uint Terrain::width = 800;
const uint Terrain::depth = 800;
const uint Terrain::height = 5000;

Terrain* EdgeTriTree::terrain = 0;

// ============================================== //
Terrain::Terrain(GLuint gl) : gl(gl) {

	edgeTree = new EdgeTriTree(this);
	generateTerrain();
	construct();
	selection = new TerrainSelection();
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
			glBufferData( GL_ELEMENT_ARRAY_BUFFER, selectionClass->triangles.size() * sizeof(Triangle), selectionClass->triangles.data(), GL_STATIC_DRAW );


			GLint glMVP = glGetUniformLocation( gl, "MVP" );

			glm::mat4 mvp = camera.perspectiveView;
			mvp = glm::transpose(mvp);
			glUniformMatrix4fv( glMVP, 1, GL_FALSE, glm::value_ptr(mvp) );
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			if ( selectionClass->class_id == TerrainSelection::SelectionClass::CLASS_HIGHLIGHT ) {
				glUniform3f( glGetUniformLocation( gl, "in_color" ), 1.0f, 0.8f, 0.0f );
			} else if ( selectionClass->class_id == TerrainSelection::SelectionClass::CLASS_HIGHLIGHT_NEIGHBOUR ) {
				glUniform3f( glGetUniformLocation( gl, "in_color" ), 9.0f, 0.4f, 0.0f );
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
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, debugTriangles.size() * sizeof(Triangle), debugTriangles.data(), GL_STATIC_DRAW );


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
	Chunk::AddTriangleResults addedResults = chunk->addTriangle(p0, p1, p2); // Tessellated Triangle
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
			chunk->triangleBuffer.push_back(Triangle(p0->vertexIndex, p1->vertexIndex, p2->vertexIndex));
			ushort bad_tri = chunk->triangleBuffer.size() - 1;
			edgeTree->addTriangle( chunk, bad_tri );
		}
		*/
		return addedTriangles;
	}
	if ( addedResults.addResults == Chunk::AddTriangleResults::TRIANGLE_ADD_SUCCEEDED ) {

		// succeeded in adding triangle
		// debugTriangles.push_back( chunk->triangleBuffer[addedResults.addedTriangle] ); // DEBUG
		TriangleNode* addedTriangle = new TriangleNode();
		addedTriangle->chunk = chunk;
		addedTriangle->triangleID = addedResults.addedTriangle;
		addedTriangles.push_back( addedTriangle );

		Log(str(format("	Add-Success <%1%,%2%,%3%> <%4%, %5%, %6%>, <%7%, %8%, %9%>") % 
					p0->getX() % p0->getY() % p0->getZ() %
					p1->getX() % p1->getY() % p1->getZ() %
					p2->getX() % p2->getY() % p2->getZ() ));
		assert( !(p0==p1 || p0==p2 || p1==p2) );
		Triangle tr = addedTriangle->chunk->triangleBuffer[addedTriangle->triangleID];
		// TODO: triangles being formed with same vertices
		if ( tr.p0==tr.p1 || tr.p0==tr.p2 || tr.p1==tr.p2 ) {
			assert(false);
			addedTriangle->chunk->triangleBuffer.erase( addedTriangle->chunk->triangleBuffer.begin() + addedTriangle->triangleID );
		} else {
			edgeTree->addTriangle( chunk, addedResults.addedTriangle );
		}

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
		if ( d_up > 0 ) { // above
			// check if above chunk is hit
			t = midChunk->above->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, EdgeChunk::FACE_TOP );
			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t > 0 && midChunk->above->pointInFace( hitpoint, EdgeChunk::FACE_TOP ) ) {
				midChunk = midChunk->above;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				--d_up;
				continue;
			}
		}
		if ( d_up < 0 ) { // below
			t = midChunk->below->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, EdgeChunk::FACE_BOTTOM );
			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t > 0 && midChunk->below->pointInFace( hitpoint, EdgeChunk::FACE_BOTTOM ) ) {
				midChunk = midChunk->below;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				++d_up;
				continue;
			}
		}

		if ( d_ahead > 0 ) { // infront
			t = midChunk->infront->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, EdgeChunk::FACE_FRONT );
			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t > 0 && midChunk->infront->pointInFace( hitpoint, EdgeChunk::FACE_FRONT ) ) {
				midChunk = midChunk->infront;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				--d_ahead;
				continue;
			}
		}
		if ( d_ahead < 0 ) { // behind
			t = midChunk->behind->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, EdgeChunk::FACE_BACK );
			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t > 0 && midChunk->behind->pointInFace( hitpoint, EdgeChunk::FACE_BACK ) ) {
				midChunk = midChunk->behind;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				++d_ahead;
				continue;
			}
		}

		if ( d_right > 0 ) { // right
			t = midChunk->right->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, EdgeChunk::FACE_RIGHT );
			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t > 0 && midChunk->right->pointInFace( hitpoint, EdgeChunk::FACE_RIGHT ) ) {
				midChunk = midChunk->right;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				--d_right;
				continue;
			}
		}
		if ( d_right < 0 ) { // left
			t = midChunk->left->edgeHitPoint( vp0, direction.v_x, direction.v_y, direction.v_z, EdgeChunk::FACE_LEFT );
			hitpoint = Vertex( vp0.v_x + t*direction.v_x, vp0.v_y + t*direction.v_y, vp0.v_z + t*direction.v_z );
			if ( t <= 1 && t > 0 && midChunk->left->pointInFace( hitpoint, EdgeChunk::FACE_LEFT ) ) {
				midChunk = midChunk->left;
				if ( !EdgeChunk::vectorContainsEdgeChunk( &containers, midChunk ) ) containers.push_back( midChunk );
				++d_right;
				continue;
			}
		}

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
	} else {
		return this->p0;
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
// void EdgeTriTree::addTriangle(Chunk* chunk, ushort triIndex) {
// 
// 	Tri* tri = new Tri(chunk, triIndex);
// 	ushort p0 = chunk->triangleBuffer[triIndex].p0;
// 	ushort p1 = chunk->triangleBuffer[triIndex].p1;
// 	ushort p2 = chunk->triangleBuffer[triIndex].p2;
// 	addTriangle( chunk, triIndex, p0, p1 );
// 	addTriangle( chunk, triIndex, p1, p2 );
// 	addTriangle( chunk, triIndex, p2, p0 );
// 
// }
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
void EdgeTriTree::addTriangle(Chunk* chunk, ushort triIndex) {

	// TODO:
	// 	> find all edges along tri, and their associated information
	
	/* EdgeOfTri
	 *
	 * The triangle has 3 sides to it; each side needs to be added into the edge tree. The cases during this add phase
	 * can be very specific for each edge (eg. one edge needs to be added to the tree, another edge has been found as a
	 * smaller scale of an existing edge, the last edge is found as a larger scale of an existing edge). This struct
	 * defines all the necessary information for each edge, which can later be handled accordingly (subdividing the
	 * triangle, re-neighbouring, etc.)
	 ***/
	struct EdgeOfTri {

		void assertEdgeN(ushort pt, EdgeTriNode* neighbour) {
			assert(neighbour->p0 == pt || neighbour->p1 == pt);
		}

		void assertBadNeighbours(EdgeTriNode* edge) {
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
			if (edge->p0 == 21 && edge->p1 == 65) {
				Log("Touching edge again..");
			}
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
			for ( auto chunk : containers ) {
				for ( auto e_edge : chunk->edges ) {
					assertBadNeighbours(e_edge);
					Vertex e_p0  = terrain->vertexBuffer[e_edge->p0];
					Vertex e_p1  = terrain->vertexBuffer[e_edge->p1];

					// Edges are the same direction?
					glm::vec3 e_dir_vec = glm::normalize(glm::vec3( e_p1.v_x - e_p0.v_x, e_p1.v_y - e_p0.v_y, e_p1.v_z - e_p0.v_z ));
					if ( dir_vec == e_dir_vec || dir_vec == (e_dir_vec*=-1) ) {

						// Edges are parallel, or apart of the same continuous line?
						bool possibleMatch = false;
						if ( e_edge->p1 != p0 ) {
							e_dir_vec = glm::normalize(glm::vec3( e_p1.v_x - vp0.v_x, e_p1.v_y - vp0.v_y, e_p1.v_z - vp0.v_z ));
							if ( dir_vec == e_dir_vec || dir_vec == (e_dir_vec*=-1) ) possibleMatch = true;
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

				Log(str(format("	--> {%1%,%2%} <%3%, %4%, %5%> <%6%, %7%, %8%>") % e_edge->p0 % e_edge->p1 %
						e_p0.v_x % e_p0.v_y % e_p0.v_z %
						e_p1.v_x % e_p1.v_y % e_p1.v_z ));
				/*
				   Log(str(format("About to call: Tri::oneOrTheOther( %1%, %2%, %3%, %4% )")% p0% p1% e_edge->p0% e_edge->p1));
				   bool oneOrTheOther =  Tri::oneOrTheOther( p0, p1, e_edge->p0, e_edge->p1 ) ;
				   Log(str(format("Called oneOrTheOther success (%1%)")% (oneOrTheOther?"True":"False")));
				   Log(str(format("About to call (2)")));
				   bool between = vp0.between( e_p0, e_p1 ) && vp1.between( e_p0, e_p1 ) ;
				   Log(str(format("Called between (2): %1%")% between));
				   Log(str(format("About to call (3)")));
				   between = e_p0.between( vp0, vp1 ) && e_p1.between( vp0, vp1 ) ;
				   Log(str(format("Called between (3): %1%")%between));
				 */
				int whichCase = -1; // TODO: removing whichCase assignment in if statements causes crash..
				try {
					if ( ((whichCase = 1) && Tri::oneOrTheOther( p0, p1, e_edge->p0, e_edge->p1 )) ) {

						Log(str(format("	MATCH: Same edge!")));
						assert( edges.size() == 0 );
						// This edge is exactly the one we're looking for
						InnerEdgeOfTri* matchedEdge = new InnerEdgeOfTri();
						matchedEdge->edge = e_edge;
						matchedEdge->flippedEdge = ( p0 != e_edge->p0 );
						edges.push_back( matchedEdge );
						break;

					} else if ( ((whichCase = 2) && vp0.between( e_p0, e_p1 ) && vp1.between( e_p0, e_p1 )) ) {
						// The edge is a subdivision of this existing edge

						Log(str(format("	MATCH: adding edge which is subdivision of existing edge; (%1%)<%2%, %3%, %4%> <= (%5%)<%6%, %7%, %8%> <= (%9%)<%10%, %11%, %12%> <= (%13%)<%14%, %15%, %16%>")%
									e_edge->p0 % e_p0.v_x % e_p0.v_y % e_p0.v_z %
									p0 % vp0.v_x % vp0.v_y % vp0.v_z %
									p1 % vp1.v_x % vp1.v_y % vp1.v_z %
									e_edge->p1 % e_p1.v_x % e_p1.v_y % e_p1.v_z ));
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
						Log(str(format("		Disconnecting {%1%,%2%} edge")% e_edge->p0 % e_edge->p1));
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
									Log(str(format("			Disconnected from {%1%,%2%}")% neighbour_edge->p0 % neighbour_edge->p1));
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
									Log(str(format("			Disconnected from {%1%,%2%}")% neighbour_edge->p0 % neighbour_edge->p1));
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
							Log(str(format("			p2 {%1%}: <%2%, %3%, %4%>") % p2 % vp2.v_x % vp2.v_y % vp2.v_z));

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
							Log(str(format("			Neighboured: Edge(p1) {%1%,%2%} w/ EdgeEnd {%3%,%4%}")% 
										edge->p0 % edge->p1 % edge_end->p0 % edge_end->p0 ));
							Log(str(format("			Neighboured: Edge(p1) {%1%,%2%} w/ EdgeSplit {%3%,%4%}")% 
										edge->p0 % edge->p1 % edge_split->p0 % edge_split->p0 ));

							edge_end->p0_edges.push_back(edge);
							assertEdgeN( edge_end->p0, edge );
							edge_end->p0_edges.push_back(edge_split);
							assertEdgeN( edge_end->p0, edge_split );
							Log(str(format("			Neighboured: EdgeEnd(p0) {%1%,%2%} w/ Edge {%3%,%4%}")% 
										edge_end->p0 % edge_end->p1 % edge->p0 % edge->p1 ));
							Log(str(format("			Neighboured: EdgeEnd(p0) {%1%,%2%} w/ EdgeSplit {%3%,%4%}")% 
										edge_end->p0 % edge_end->p1 % edge_split->p0 % edge_split->p1 ));

							edge_split->p0_edges.push_back(edge);
							assertEdgeN( edge_split->p0, edge );
							Log(str(format("			Neighboured: EdgeSplit(p0) {%1%,%2%} w/ Edge {%3%,%4%}")% 
										edge_split->p0 % edge_split->p1 % edge->p0 % edge->p1 ));
							edge_split->p0_edges.push_back(edge_end);
							assertEdgeN( edge_split->p0, edge_end );
							Log(str(format("			Neighboured: EdgeSplit(p0) {%1%,%2%} w/ EdgeEnd {%3%,%4%}")% 
										edge_split->p0 % edge_split->p1 % edge_end->p0 % edge_end->p1 ));

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
							}


							// Subdivide Existing Triangle
							// ----------------------------

							// Subdivide (upper tri): {p1->ep1,p1->ep0}
							Log(str(format("	Subdivided Tri: <%1%, %2%, %3%>, <%4%, %5%, %6%>, <%7%, %8%, %9%>") %
										vp1.v_x % vp1.v_y % vp1.v_z %
										terrain->vertexBuffer[p2].v_x % terrain->vertexBuffer[p2].v_y % terrain->vertexBuffer[p2].v_z %
										e_p1.v_x % e_p1.v_y % e_p1.v_z ));
							e_tri->chunk->triangleBuffer.push_back( Triangle( p1, p2, (newEdge->flippedEdge? e_edge->p0 : e_edge->p1) ) );
							ushort subTri_outi = e_tri->chunk->triangleBuffer.size() - 1;
							Tri* subTri_out = new Tri( e_tri->chunk, subTri_outi );
							Tri::assertBadTri(subTri_out);

							// Subdivide (lower tri): {p0->p1} 
							// NOTE: using the same existing Tri since we refer to Triangles by their index, we do NOT want
							// to remove a triangle ever from the list
							e_tri->reshapeTriOnEdge( p0, (newEdge->flippedEdge? e_edge->p0 : e_edge->p1), p0, p1 );


							// Attach Tri's to Edges
							// -----------------------

							// Attach (upper tri)
							edge_end->triangle_p1p0 = subTri_out; // TODO: used to be p0p1
							edge_split->triangle_p0p1 = subTri_out;
							Tri** edge_p1p2_subTriSide = (edge_p1p2_flipped? &edge_p1p2->triangle_p0p1 : &edge_p1p2->triangle_p1p0 );
							(*edge_p1p2_subTriSide) = subTri_out;

							// Attach (lower tri)
							edge->triangle_p1p0 = e_tri;
							edge_split->triangle_p1p0 = e_tri;
							Tri** edge_p0p2_triSide = (edge_p0p2_flipped? &edge_p0p2->triangle_p1p0 : &edge_p0p2->triangle_p0p1 );
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
							}


							// Subdivide Existing Triangle
							// ----------------------------

							// Subdivide (lower tri): {ep0->p0,ep1->p0}
							Log(str(format("	Subdivided Tri: <%1%, %2%, %3%>, <%4%, %5%, %6%>, <%7%, %8%, %9%>") %
										terrain->vertexBuffer[p2].v_x % terrain->vertexBuffer[p2].v_y % terrain->vertexBuffer[p2].v_z %
										vp0.v_x % vp0.v_y % vp0.v_z %
										e_p0.v_x % e_p0.v_y % e_p0.v_z ));
							e_tri->chunk->triangleBuffer.push_back( Triangle( (newEdge->flippedEdge? e_edge->p1 : e_edge->p0), p2, p0 ) );
							ushort subTri_outi = e_tri->chunk->triangleBuffer.size() - 1;
							Tri* subTri_out = new Tri( e_tri->chunk, subTri_outi );
							Tri::assertBadTri(subTri_out);

							// Subdivide (lower tri): {p0->p1} 
							// NOTE: using the same existing Tri since we refer to Triangles by their index, we do NOT want
							// to remove a triangle ever from the list
							e_tri->reshapeTriOnEdge( (newEdge->flippedEdge? e_edge->p1 : e_edge->p0), p1, p0, p1 );



							// Attach Tri's to Edges
							// -----------------------

							// Attach (lower tri)
							edge_end->triangle_p1p0 = subTri_out;
							edge_split->triangle_p1p0 = subTri_out;
							Tri** edge_p0p2_subTriSide = (edge_p0p2_flipped? &edge_p0p2->triangle_p1p0 : &edge_p0p2->triangle_p0p1 );
							(*edge_p0p2_subTriSide) = subTri_out;

							// Attach (upper tri)
							edge->triangle_p1p0 = e_tri;
							edge_split->triangle_p0p1 = e_tri;
							Tri** edge_p1p2_triSide = (edge_p1p2_flipped? &edge_p1p2->triangle_p1p0 : &edge_p1p2->triangle_p0p1 );
							(*edge_p1p2_triSide) = e_tri;

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
							Tri** neighbourTri_ep0p2 = e_tri->getNeighbourOnEdge( ref_ep0, p2 );
							if ( (*neighbourTri_ep0p2) ) {
								(*subTri_out->getNeighbourOnEdge(ref_ep0, p2)) = (*neighbourTri_ep0p2);
								(*(*neighbourTri_ep0p2)->getNeighbourOnEdge( ref_ep0, p2 )) = subTri_out;
							}

							// Neighbour subTri_out w/ tri
							(*neighbourTri_ep0p2) = subTri_out;
							(*subTri_out->getNeighbourOnEdge(p0, p2)) = e_tri;
							Tri::assertBadTri(subTri_out);
							Tri::assertBadTri(e_tri);

							// TODO: using reference Tri**, however we rewrite the top Tri neighbour in e_tri after setting
							// subTri to e_tri's neighbour. Will this affect subTri's neighbouring?

							Log(str(format("	Deleting e_edge {%1%,%2%}")%e_edge->p0%e_edge->p1));
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

							edge_topsplit->p1_edges.push_back(edge_botsplit);
							assertEdgeN( edge_topsplit->p1, edge_botsplit );
							edge_botsplit->p1_edges.push_back(edge_topsplit);
							assertEdgeN( edge_botsplit->p1, edge_topsplit );
							Log(str(format("			Neighboured: EdgeTopSplit(p1) {%1%,%2%} w/ EdgeBotSplit {%3%,%4%}")% 
										edge_topsplit->p0 % edge_topsplit->p1 % edge_botsplit->p0 % edge_botsplit->p1 ));
							Log(str(format("			Neighboured: EdgeBotSplit(p1) {%1%,%2%} w/ EdgeTopSplit {%3%,%4%}")% 
										edge_botsplit->p0 % edge_botsplit->p1 % edge_topsplit->p0 % edge_topsplit->p1 ));

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
									edge_p1p2_flipped = (neighbourEdge->p1 == edge_p1_expected);
									(*(edge_p1p2_flipped? &edge_p1p2->triangle_p0p1 : &edge_p1p2->triangle_p1p0)) = 0; // unattach triangle (to-be changed)
									edge_p1p2_neighbouringElement = ( edge_p1p2_flipped ? &edge_p1p2->p0_edges : &edge_p1p2->p1_edges );

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

							assertBadNeighbours(e_edge);
							assertBadNeighbours(edge_p1p2);
							assertBadNeighbours(edge_p0p2);
							assertBadNeighbours(edge_topsplit);
							assertBadNeighbours(edge_botsplit);


							// Copy neighbours from existing edge
							vector<tuple<vector<EdgeTriNode*>*, EdgeTriNode*, ushort>> edge_neighbours; // neighbours: {source, destination_node, point}
							edge_neighbours.push_back(make_tuple( (newEdge->flippedEdge? &e_edge->p1_edges : &e_edge->p0_edges), edge_bot, edge_bot->p0 ));
							edge_neighbours.push_back(make_tuple( (newEdge->flippedEdge? &e_edge->p0_edges : &e_edge->p1_edges), edge_top, edge_top->p1 ));
							edge_neighbours.push_back(make_tuple( edge_p0p2_neighbouringElement, edge_botsplit, edge_botsplit->p1 ));
							edge_neighbours.push_back(make_tuple( edge_p1p2_neighbouringElement, edge_topsplit, edge_topsplit->p1 )); // TODO: should this be p0p2?

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
							}


							// Subdivide Existing Triangle
							// ----------------------------

							// Subdivide (upper tri): {p1->ep1,p1->ep0}
							Log(str(format("	Subdivided Tri: <%1%, %2%, %3%>, <%4%, %5%, %6%>, <%7%, %8%, %9%>") %
										vp1.v_x % vp1.v_y % vp1.v_z %
										terrain->vertexBuffer[p2].v_x % terrain->vertexBuffer[p2].v_y % terrain->vertexBuffer[p2].v_z %
										(newEdge->flippedEdge?e_p0.v_x:e_p1.v_x) % (newEdge->flippedEdge?e_p0.v_y:e_p1.v_y) % (newEdge->flippedEdge?e_p0.v_z:e_p1.v_z) ));
							e_tri->chunk->triangleBuffer.push_back( Triangle( p1, p2, (newEdge->flippedEdge? e_edge->p0 : e_edge->p1) ) );
							ushort subTri_topi = e_tri->chunk->triangleBuffer.size() - 1;
							Tri* subTri_top = new Tri( e_tri->chunk, subTri_topi );

							// Subdivide (lower tri): {ep0->p0,ep1->p0}
							Log(str(format("	Subdivided Tri: <%1%, %2%, %3%>, <%4%, %5%, %6%>, <%7%, %8%, %9%>") %
										(newEdge->flippedEdge?e_p1.v_x:e_p0.v_x) % (newEdge->flippedEdge?e_p1.v_y:e_p0.v_y) % (newEdge->flippedEdge?e_p1.v_z:e_p0.v_z) %
										terrain->vertexBuffer[p2].v_x % terrain->vertexBuffer[p2].v_y % terrain->vertexBuffer[p2].v_z %
										vp0.v_x % vp0.v_y % vp0.v_z));
							e_tri->chunk->triangleBuffer.push_back( Triangle( (newEdge->flippedEdge? e_edge->p1 : e_edge->p0), p2, p0 ) );
							ushort subTri_boti = e_tri->chunk->triangleBuffer.size() - 1;
							Tri* subTri_bot = new Tri( e_tri->chunk, subTri_boti );

							// Subdivide (mid tri): {p0->p1} 
							// NOTE: using the same existing Tri since we refer to Triangles by their index, we do NOT want
							// to remove a triangle ever from the list
							Log(str(format("	Reshaping Tri: <%1%, %2%, %3%>, <%4%, %5%, %6%>") %
										vp0.v_x % vp0.v_y % vp0.v_z % vp1.v_x % vp1.v_y % vp1.v_z ));
							e_tri->reshapeTriOnEdge( (newEdge->flippedEdge? e_edge->p1 : e_edge->p0), (newEdge->flippedEdge? e_edge->p0 : e_edge->p1), p0, p1 );


							// Attach Tri's to Edges
							// -----------------------

							// Attach (upper tri)
							edge_top->triangle_p1p0 = subTri_top;
							edge_topsplit->triangle_p0p1 = subTri_top;
							Tri** edge_p1p2_subTriSide = (edge_p1p2_flipped? &edge_p1p2->triangle_p1p0 : &edge_p1p2->triangle_p0p1 );
							(*edge_p1p2_subTriSide) = subTri_top;

							// Attach (lower tri)
							edge_bot->triangle_p1p0 = subTri_bot;
							edge_botsplit->triangle_p1p0 = subTri_bot;
							Tri** edge_p0p2_subTriSide = (edge_p0p2_flipped? &edge_p0p2->triangle_p1p0 : &edge_p0p2->triangle_p0p1 );
							(*edge_p0p2_subTriSide) = subTri_bot;

							// Attach (middle tri)
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
							Tri** neighbourTri_ep1p2 = e_tri->getNeighbourOnEdge( ref_ep1, p2 );
							if ( (*neighbourTri_ep1p2) ) {
								(*subTri_top->getNeighbourOnEdge(ref_ep1, p2)) = (*neighbourTri_ep1p2);
								(*(*neighbourTri_ep1p2)->getNeighbourOnEdge( ref_ep1, p2 )) = subTri_top;
							}

							// Neighbour subTri_top w/ tri
							(*neighbourTri_ep1p2) = subTri_top;
							(*subTri_top->getNeighbourOnEdge(p1, p2)) = e_tri;


							// Neighbour subTri_bot w/ bot
							ushort ref_ep0 = (newEdge->flippedEdge?e_edge->p1:e_edge->p0);
							Tri** neighbourTri_ep0p2 = e_tri->getNeighbourOnEdge( ref_ep0, p2 );
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

						/*
						// There are up to 3 cuts on our edge to match this tri; p0->ep0, ep0->ep1, ep1->p1
						// For each cut, if the cut is possible (length > 0) then create and add a new Tri for that position

						bool flippedEdge = (e_p0.between(vp0, e_p1));
						ushort expected_p0 = (flippedEdge? e_edge->p1 : e_edge->p0);
						ushort expected_p1 = (flippedEdge? e_edge->p0 : e_edge->p1);

						Tri* e_tri = (flippedEdge? e_edge->triangle_p0p1 : e_edge->triangle_p1p0);
						Vertex evp0 = terrain->vertexBuffer[e_tri->p0];
						Vertex evp1 = terrain->vertexBuffer[e_tri->p1];
						Vertex evp2 = terrain->vertexBuffer[e_tri->p2];
						ushort p2 = e_tri->getOddPoint(e_edge->p0, e_edge->p1);

						// upper tri (create new one)
						if ( expected_p1 != p1 ) {
						e_tri->chunk->triangleBuffer.push_back( Triangle( expected_p1, p2, p1 ) );
						ushort subTri_topi = e_tri->chunk->triangleBuffer.size() - 1;
						addTriangle( e_tri, subTri_topi );

						// TODO: retrieve edge for this side (add to edges)
						}

						// lower tri (create new one)
						if ( expected_p0 != p0 ) {
						e_tri->chunk->triangleBuffer.push_back( Triangle( expected_p0, p2, p0 ) );
						ushort subTri_boti = e_tri->chunk->triangleBuffer.size() - 1;
						addTriangle( e_tri, subTri_boti );

						// TODO: retrieve edge for this side (add to edges)
						}

						// Use existing (subdivided) edge
						InnerEdgeOfTri* e_newEdge = new InnerEdgeOfTri();
						e_newEdge->edge = e_edge;
						e_newEdge->flippedEdge = flippedEdge;
						edges.push_back( e_newEdge );

						break;
						 */
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
				}

			}
		}

		struct InnerEdgeOfTri {
			EdgeTriNode* edge;
			bool flippedEdge; // flipped specified p0p1?
		};

		vector<InnerEdgeOfTri*> edges; // the edge(s) along this side of the original triangle
		vector<EdgeChunk*> containers; // containers for all edges
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
	Triangle triangle = chunk->triangleBuffer[triIndex];
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
		/*
		 * TODO: edges are already neighboured together; when they were created they checked for existing edges to
		 * attach to
		if ( edge_p0p1->p0 == edge_p2p0->p0 || edge_p0p1->p0 == edge_p2p0->p1 ) {
			edge_p0p1->p0_edges.push_back( edge_p2p0 );
			edge_p0p1->p1_edges.push_back( edge_p1p2 );

			if ( edge_p2p0->p0 == edge_p0p1->p0 ) {
				edge_p2p0->p0_edges.push_back( edge_p0p1 );
				edge_p2p0->p1_edges.push_back( edge_p1p2 );

				if ( edge_p1p2->p0 == edge_p0p1->p1 ) {
					edge_p1p2->p0_edges.push_back( edge_p0p1 );
					edge_p1p2->p1_edges.push_back( edge_p2p0 );
				} else {
					edge_p1p2->p0_edges.push_back( edge_p2p0 );
					edge_p1p2->p1_edges.push_back( edge_p1p2 );
				}
			} else {
				edge_p2p0->p1_edges.push_back( edge_p0p1 );
				edge_p2p0->p0_edges.push_back( edge_p1p2 );

				if ( edge_p1p2->p0 == edge_p0p1->p1 ) {
					edge_p1p2->p0_edges.push_back( edge_p0p1 );
					edge_p1p2->p1_edges.push_back( edge_p2p0 );
				} else {
					edge_p1p2->p0_edges.push_back( edge_p2p0 );
					edge_p1p2->p1_edges.push_back( edge_p1p2 );
				}
			}

			triEdge_p0p1->assertBadNeighbours( edge_p0p1 );
			triEdge_p0p1->assertBadNeighbours( edge_p1p2 );
			triEdge_p0p1->assertBadNeighbours( edge_p2p0 );

		} else if ( edge_p0p1->p1 == edge_p2p0->p0 || edge_p0p1->p1 == edge_p2p0->p1 ) {

			edge_p0p1->p1_edges.push_back( edge_p2p0 );
			edge_p0p1->p0_edges.push_back( edge_p1p2 );

			if ( edge_p2p0->p0 == edge_p0p1->p1 ) {
				edge_p2p0->p0_edges.push_back( edge_p0p1 );
				edge_p2p0->p1_edges.push_back( edge_p1p2 );

				if ( edge_p1p2->p0 == edge_p0p1->p0 ) {
					edge_p1p2->p0_edges.push_back( edge_p0p1 );
					edge_p1p2->p1_edges.push_back( edge_p2p0 );
				} else {
					edge_p1p2->p0_edges.push_back( edge_p2p0 );
					edge_p1p2->p1_edges.push_back( edge_p1p2 );
				}
			} else {
				edge_p2p0->p1_edges.push_back( edge_p0p1 );
				edge_p2p0->p0_edges.push_back( edge_p1p2 );

				if ( edge_p1p2->p0 == edge_p0p1->p0 ) {
					edge_p1p2->p0_edges.push_back( edge_p0p1 );
					edge_p1p2->p1_edges.push_back( edge_p2p0 );
				} else {
					edge_p1p2->p0_edges.push_back( edge_p2p0 );
					edge_p1p2->p1_edges.push_back( edge_p1p2 );
				}
			}

			triEdge_p0p1->assertBadNeighbours( edge_p0p1 );
			triEdge_p0p1->assertBadNeighbours( edge_p1p2 );
			triEdge_p0p1->assertBadNeighbours( edge_p2p0 );

		} else {
			assert(false); // edge_p0p1 does NOT match edge_p2p0 on either point
		}
		*/

		// if (triEdge_p0p1->edges.front()->flippedEdge) {
		// 	triEdge_p0p1->edges.front()->edge->p1_edges.push_back( triEdge_p2p0->edges.front()->edge );
		// 	triEdge_p0p1->edges.front()->edge->p0_edges.push_back( triEdge_p1p2->edges.front()->edge );
		// } else {
			// triEdge_p0p1->edges.front()->edge->p1_edges.push_back( triEdge_p1p2->edges.front()->edge );
			// triEdge_p0p1->edges.front()->edge->p0_edges.push_back( triEdge_p2p0->edges.front()->edge );
		// }

		// if (triEdge_p1p2->edges.front()->flippedEdge) {
		// 	triEdge_p1p2->edges.front()->edge->p1_edges.push_back( triEdge_p0p1->edges.front()->edge );
		// 	triEdge_p1p2->edges.front()->edge->p0_edges.push_back( triEdge_p2p0->edges.front()->edge );
		// } else {
			// triEdge_p1p2->edges.front()->edge->p1_edges.push_back( triEdge_p2p0->edges.front()->edge );
			// triEdge_p1p2->edges.front()->edge->p0_edges.push_back( triEdge_p0p1->edges.front()->edge );
		// }

		// if (triEdge_p2p0->edges.front()->flippedEdge) {
		// 	triEdge_p2p0->edges.front()->edge->p1_edges.push_back( triEdge_p1p2->edges.front()->edge );
		// 	triEdge_p2p0->edges.front()->edge->p0_edges.push_back( triEdge_p0p1->edges.front()->edge );
		// } else {
			// triEdge_p2p0->edges.front()->edge->p1_edges.push_back( triEdge_p0p1->edges.front()->edge );
			// triEdge_p2p0->edges.front()->edge->p0_edges.push_back( triEdge_p1p2->edges.front()->edge );
		// }
		triEdge_p0p1->assertBadNeighbours(triEdge_p0p1->edges.front()->edge);
		triEdge_p1p2->assertBadNeighbours(triEdge_p1p2->edges.front()->edge);
		triEdge_p2p0->assertBadNeighbours(triEdge_p2p0->edges.front()->edge);

		vector<pair<EdgeOfTri::InnerEdgeOfTri*,Tri**>> edges;
		edges.push_back({ triEdge_p0p1->edges.front(), &newTri->neighbour_p0p1 });
		edges.push_back({ triEdge_p1p2->edges.front(), &newTri->neighbour_p1p2 });
		edges.push_back({ triEdge_p2p0->edges.front(), &newTri->neighbour_p2p0 });
		for ( auto edge_pair : edges ) {
			EdgeTriNode* edge = edge_pair.first->edge;
			if ( edge_pair.first->flippedEdge ) {
				edge->triangle_p1p0 = newTri;
				(*edge_pair.second) = edge->triangle_p0p1;
				if (edge->triangle_p0p1) (*edge->triangle_p0p1->getNeighbourOnEdge(edge->p0, edge->p1)) = newTri;
			} else {
				edge->triangle_p0p1 = newTri;
				(*edge_pair.second) = edge->triangle_p1p0;
				if (edge->triangle_p1p0) (*edge->triangle_p1p0->getNeighbourOnEdge(edge->p0, edge->p1)) = newTri;
			}
		}
		triEdge_p0p1->assertBadNeighbours(triEdge_p0p1->edges.front()->edge);
		triEdge_p1p2->assertBadNeighbours(triEdge_p1p2->edges.front()->edge);
		triEdge_p2p0->assertBadNeighbours(triEdge_p2p0->edges.front()->edge);
		Tri::assertBadTri(newTri);

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
		// Setup triangles from p1p2 side to p0p1 edge
		EdgeOfTri::InnerEdgeOfTri* nextEdge = triEdge_p0p1->edges.front();
		for ( auto in_edge : triEdge_p1p2->edges ) {
			if ( in_edge == triEdge_p1p2->edges.back() ) break;
			ushort expected_p0 = (in_edge->flippedEdge? in_edge->edge->p1 : in_edge->edge->p0);
			ushort expected_p1 = (in_edge->flippedEdge? in_edge->edge->p0 : in_edge->edge->p1);
			ushort p2 = (nextEdge->flippedEdge? nextEdge->edge->p1 : nextEdge->edge->p0);
			if (expected_p0 == expected_p1 || expected_p0 == p2 || expected_p1 == p2) continue;
			chunk->triangleBuffer.push_back( Triangle( expected_p0, expected_p1, p2 ) );
			addTriangle( chunk, chunk->triangleBuffer.size() - 1 );
		}

		// Setup triangles from p2p0 to last edge on p1p2
		nextEdge = triEdge_p1p2->edges.back();
		for ( auto in_edge : triEdge_p2p0->edges ) {
			ushort expected_p0 = (in_edge->flippedEdge? in_edge->edge->p1 : in_edge->edge->p0);
			ushort expected_p1 = (in_edge->flippedEdge? in_edge->edge->p0 : in_edge->edge->p1);
			ushort p2 = (nextEdge->flippedEdge? nextEdge->edge->p1 : nextEdge->edge->p0);
			if (expected_p0 == expected_p1 || expected_p0 == p2 || expected_p1 == p2) continue;
			chunk->triangleBuffer.push_back( Triangle( expected_p0, expected_p1, p2 ) );
			addTriangle( chunk, chunk->triangleBuffer.size() - 1 );
		}

	} else {

		// Setup triangles from each side's edges
		// ---------------------------
		// p0p1{i<n} -> p2p0: p0p1(p0,p1) to p2p0.last(p0)
		// p1p2{i<=n} -> p0p1: p1p2(p0,p1) to p0p1.last(p0)
		// p2p0{i<n} -> p0p1: p2p0(p0,p1) to p0p1.last(p0)

		Log(str(format("	Triangle: Tessellation Case (2)")));
		// Setup triangles from p0p1 side to last edge on p2p0
		EdgeOfTri::InnerEdgeOfTri* nextEdge = triEdge_p2p0->edges.back();
		for ( auto in_edge : triEdge_p0p1->edges ) {
			if ( in_edge == triEdge_p0p1->edges.back() ) break;
			ushort expected_p0 = (in_edge->flippedEdge? in_edge->edge->p1 : in_edge->edge->p0);
			ushort expected_p1 = (in_edge->flippedEdge? in_edge->edge->p0 : in_edge->edge->p1);
			ushort p2 = (nextEdge->flippedEdge? nextEdge->edge->p1 : nextEdge->edge->p0);
			if (expected_p0 == expected_p1 || expected_p0 == p2 || expected_p1 == p2) continue;
			chunk->triangleBuffer.push_back( Triangle( expected_p0, expected_p1, p2 ) );
			addTriangle( chunk, chunk->triangleBuffer.size() - 1 );
		}

		// Setup triangles from p1p2 side to last edge on p0p1
		nextEdge = triEdge_p0p1->edges.back();
		for ( auto in_edge : triEdge_p1p2->edges ) {
			ushort expected_p0 = (in_edge->flippedEdge? in_edge->edge->p1 : in_edge->edge->p0);
			ushort expected_p1 = (in_edge->flippedEdge? in_edge->edge->p0 : in_edge->edge->p1);
			ushort p2 = (nextEdge->flippedEdge? nextEdge->edge->p1 : nextEdge->edge->p0);
			if (expected_p0 == expected_p1 || expected_p0 == p2 || expected_p1 == p2) continue;
			chunk->triangleBuffer.push_back( Triangle( expected_p0, expected_p1, p2 ) );
			addTriangle( chunk, chunk->triangleBuffer.size() - 1 );
		}

		// Setup triangles from p2p0 side to last edge on p0p1
		for ( auto in_edge : triEdge_p2p0->edges ) {
			if ( in_edge == triEdge_p0p1->edges.back() ) break;
			ushort expected_p0 = (in_edge->flippedEdge? in_edge->edge->p1 : in_edge->edge->p0);
			ushort expected_p1 = (in_edge->flippedEdge? in_edge->edge->p0 : in_edge->edge->p1);
			ushort p2 = (nextEdge->flippedEdge? nextEdge->edge->p1 : nextEdge->edge->p0);
			if (expected_p0 == expected_p1 || expected_p0 == p2 || expected_p1 == p2) continue;
			chunk->triangleBuffer.push_back( Triangle( expected_p0, expected_p1, p2 ) );
			addTriangle( chunk, chunk->triangleBuffer.size() - 1 );
		}

	}

	/*
	// TODO: Create new edges between results (each side's extra points create a new edge to the p1 point of the
	// following side)
	// TODO: for each *extra* edge on a side (n-1) create a new Tri for it & add to edges
	// TODO: create main Tri & add to edges
	// TODO: `
	// TODO: add our new Tri(s), subdivide the Triangle as necessary, add to the correct edges, add new necessary edges
	// (ones that lay on the inner-volume of the triangle), connect edges to graph & containers, neighbour Tri's
	// properly



	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO: remove below

	Tri* addedTri;
	if ( referenceTri ) {
		addedTri = (*referenceTri);
	}

	Vertex vp0 = terrain->vertexBuffer[p0];
	Vertex vp1 = terrain->vertexBuffer[p1];
	Vertex direction = Vertex( vp1.v_x - vp0.v_x, vp1.v_y - vp0.v_y, vp1.v_z - vp0.v_z );
	glm::vec3 dir_vec = glm::normalize(glm::vec3( vp1.v_x - vp0.v_x, vp1.v_y - vp0.v_y, vp1.v_z - vp0.v_z ));
	vector<EdgeChunk*> containers = EdgeTriNode::getContainers(p0, p1);

	// check all edges in containers to see if there are any potentially matching edges
	// 	ie. this edge is a subdivided portion of another edge, or vice versa
	vector<EdgeTriNode*> potentialMatches;
	for ( auto chunk : containers ) {
		for ( auto e_edge : chunk->edges ) {
			Vertex e_p0  = terrain->vertexBuffer[e_edge->p0];
			Vertex e_p1  = terrain->vertexBuffer[e_edge->p1];

			// same direction?
			glm::vec3 e_dir_vec = glm::normalize(glm::vec3( e_p1.v_x - e_p0.v_x, e_p1.v_y - e_p0.v_y, e_p1.v_z - e_p0.v_z ));
			if ( dir_vec == e_dir_vec || dir_vec == -1*e_dir_vec ) {

				// are they parallel or apart of the same continuous line?
				e_dir_vec = glm::normalize(glm::vec3( e_p1.v_x - vp0.v_x, e_p1.v_y - e_p0.v_y, e_p1.v_z - e_p0.v_z ));
				if ( dir_vec == e_dir_vec || dir_vec == -1*e_dir_vec ) {

					bool alreadyAdded = false;
					for ( auto ep_edge : potentialMatches ) {
						if ( ep_edge == e_edge ) {
							alreadyAdded = true;
							break;
						}
					}

					if ( !alreadyAdded ) potentialMatches.push_back( e_edge );
				}

			}

		}
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// WARNING WARNING WARNING WARNING WARNING WARNING!
	// TODO: the Tri is added 3x (one for each edge); to save on memory and safety concerns, check against connected
	// Tri's to see if the same Tri already exists, then simply use that one instead of adding a new one
	// WARNING WARNING WARNING WARNING WARNING WARNING!
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	// potential matches have same direction; does either edge have point p0 that lays within the edge?
	for ( auto e_edge : potentialMatches ) {
		// Edge (parametric): x = at + x0; ...
		Vertex e_p0  = terrain->vertexBuffer[e_edge->p0];
		Vertex e_p1  = terrain->vertexBuffer[e_edge->p1];
		
		// TODO: avoid VERTEX based == comparisons, use indices instead

	//  - merge edges: add on left/right side? merge edges (use the bigger edge); add to nodesNeedSubdividing
		if ( p0 == e_edge->p0 && p1 == e_edge->p1 ) {

			// add triangle
			Triangle tri = chunk->triangleBuffer[triIndex];
			bool leftSide = true;
			ushort triPoint_p2;
			if ( !addedTri ) {
				addedTri = new Tri( chunk, triIndex );
				(*referenceTri) = addedTri;
			}
			if ( (tri.p0 == p0 && tri.p1 == p1) ||
				 (tri.p1 == p0 && tri.p2 == p1) ||
				 (tri.p2 == p0 && tri.p0 == p1) ) { // left side
				e_edge->triangle_p0p1 = addedTri;
				addedTri->neighbour_p0p1 = e_edge->triangle_p1p0;
				if ( e_edge->triangle_p1p0 ) e_edge->triangle_p1p0->neighbour_p0p1 = addedTri;
				triPoint_p2 = addedTri->getOddPoint( p0, p1 );
			} else { // right side
				e_edge->triangle_p1p0 = addedTri;
				addedTri->neighbour_p1p0 = addedTri;
				if ( e_edge->triangle_p0p1 ) e_edge->triangle_p0p1->neighbour_p1p0 = addedTri;
				leftSide = false;
				triPoint_p2 = addedTri->getOddPoint( p0, p1 );
			}

			// TODO: find any neighbour triangles (neighbour edges whose unshared point is a point on this triangle)
			for ( auto neighbour_edge : e_edge->p0_edges ) {
				ushort matchedPoint, unmatchedPoint;
				bool neighbourFlipped = false;
				if ( neighbour_edge->p0 == p0 ) {
					matchedPoint   = p0;
					unmatchedPoint = p1;
					neighbourFlipped = true;
				} else {
					matchedPoint   = p1;
					unmatchedPoint = p0;
				}

				if ( unmatchedPoint == triPoint_p2 ) {
					Tri** neighbourTri = ((leftSide ^ neighbourFlipped) ? (&neighbour_edge->triangle_p0p1) : (&neighbour_edge->triangle_p1p0));
					if ((*neighbourTri)) {
						Tri** neighbourTri_neighbourEdge = (*neighbourTri)->getNeighbourOnEdge( matchedPoint, unmatchedPoint );
						(*addedTri->getNeighbourOnEdge( matchedPoint, unmatchedPoint )) = (*neighbourTri);
						(*neighbourTri_neighbourEdge) = addedTri;
					}
				}
			}
			for ( auto neighbour_edge : e_edge->p1_edges ) {
				// TODO
			}

			break;
		} else if ( vp1 == e_p0 && vp0 == e_p1 ) {
			// TODO: add triangle
			break;
		} else if ( vp0.between( e_p0, e_p1 ) && vp1.between( e_p0, e_p1 ) ) {
			// this is a subdivision of the existing edge

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
			for ( auto neighbour_edge : e_edge->p0_edges ) {
				int i = 0;
				vector<EdgeTriNode*>* neighbourConnection;
				if ( neighbour_edge->p0 == e_edge->p0 ) {
					neighbourConnection = &neighbour_edge->p0_edges;
				} else {
					neighbourConnection = &neighbour_edge->p1_edges;
				}

				for ( auto some_edge : *neighbourConnection ) {
					if ( some_edge == e_edge ) {
						break;
					}
					++i;
				}
				neighbourConnection->erase( neighbourConnection->begin() + i );
			}

			// Create new edge
			EdgeTriNode* edge = new EdgeTriNode();
			edge->p0 = p0;
			edge->p1 = p1;

			// left or right side?
			Triangle tri = chunk->triangleBuffer[triIndex];
			bool leftSide = true;
			Triangle* e_tri;
			if ( (tri.p0 == p0 && tri.p1 == p1) ||
					(tri.p1 == p0 && tri.p2 == p1) ||
					(tri.p2 == p0 && tri.p0 == p1) ) {
				// left side
				edge->triangle_p0p1 = new Tri( chunk, triIndex );

			} else {
				// right side
				edge->triangle_p1p0 = new Tri( chunk, triIndex );
				leftSide = false;

			}

			// TODO: subdivide triangle
			// NOTE: this edge could lay on the left end of the edge, on the right end of the edge, or inbetween the
			// edge while not touching either the left or right ends
			// Triangle gets cut from p0 to tp0 (its p0 on the line); tp0 to tp1; tp1 to p1 (3 cuts)
			// Triangle could also be cut from p0 to tp0, tp1 to p1  (2 cuts)
			Tri* e_tri = (leftSide?e_edge->triangle_p1p0:e_edge->triangle_p0p1);
			Vertex evp0 = terrain->vertexBuffer[e_tri->p0];
			Vertex evp1 = terrain->vertexBuffer[e_tri->p1];
			Vertex evp2 = terrain->vertexBuffer[e_tri->p2];
			ushort p2 = e_tri->getOddPoint(e_p0, e_p1);

			// vp0, vp1 [p0,p1] refer to the new edge p0/p1
			// e_p0, e_p1 [e_edge->p0,e_edge->p1] refer to the existing edges p0 and p1
			// evp0, evp1, evp2 refer to the existing triangle's p0/p1/p2
			if ( vp0 == e_p0 ) {
				if (leftSide) {
					// TODO: (2 cuts); e_p0->vp2, vp2->e_p1
				} else {
					// TODO: (2 cuts); e_p0->vp1, vp1->e_p1
				}
			} else if ( vp1 == e_p0 ) {
				if (leftSide) {
					// TODO: (2 cuts); e_p0->vp0, vp0->e_p1
				} else {
					// TODO: (2 cuts); e_p0->vp2, vp2->e_p1
				}
			} else {

				if ( vp0.between( e_p0, vp1 ) ) {
					// vp0 is closer to e_p0; hence e_p0->vp0 for first subdivision
					// TODO: (3 cuts); e_p0->vp0, vp0->vp1, vp1->e_p1
					if (leftSide) {

						// Subdivide (lower tri): e_p0->vp0
						e_tri->chunk->triangleBuffer.push_back( Triangle( e_edge->p0, p2, p0 ) );
						ushort subTri_ep0vp0i = e_tri->chunk->triangleBuffer.size() - 1;
						Tri* subTri_ep0vp0 = new Tri( e_tri->chunk, subTri_ep0vp0i );

						// Subdivide (higher tri): vp1->e_p1
						e_tri->chunk->triangleBuffer.push_back( Triangle( p1, p2, e_edge->p1 ) );
						ushort subTri_ep1vp1i = e_tri->chunk->triangleBuffer.size() - 1;
						Tri* subTri_ep1vp1 = new Tri( e_tri->chunk, subTri_ep1vp1i );

						// Subdivide (middle tri): vp0->vp1
						// NOTE: edit the existing triangle to avoid deleting it from list
						uchar e_tri_pointsOnEdge;
						if ( e_tri->p0 == e_edge->p1 ) {
							e_tri->chunk->triangleBuffer[e_tri->triIndex].p0 = edge->p0;
							e_tri->chunk->triangleBuffer[e_tri->triIndex].p1 = edge->p1;
							e_tri->p0 = edge->p0;
							e_tri->p1 = edge->p1;
							e_tri_pointsOnEdge = 0;
						} else if ( e_tri->p1 == e_edge->p1 ) {
							e_tri->chunk->triangleBuffer[e_tri->triIndex].p1 = edge->p0;
							e_tri->chunk->triangleBuffer[e_tri->triIndex].p2 = edge->p1;
							e_tri->p1 = edge->p0;
							e_tri->p2 = edge->p1;
							e_tri_pointsOnEdge = 1;
						} else {
							e_tri->chunk->triangleBuffer[e_tri->triIndex].p2 = edge->p0;
							e_tri->chunk->triangleBuffer[e_tri->triIndex].p0 = edge->p1;
							e_tri->p2 = edge->p0;
							e_tri->p0 = edge->p1;
							e_tri_pointsOnEdge = 2;
						}


						// neighbouring
						if ( e_tri_pointsOnEdge == 0 ) {
							subTri_ep0vp0->neighbour_p2p0 = e_tri->neighbour_p2p0;
							if ( subTri_ep0vp0->neighbour_p2p0 ) subTri_ep0vp0->neighbour_p2p0->neighbour_p1p2 = subTri_ep0vp0;
							subTri_ep1vp1->neighbour_p1p2 = e_tri->neighbour_p1p2;
							if ( subTri_ep1vp1->neighbour_p1p2 ) subTri_ep1vp1->neighbour_p1p2->neighbour_p2p0 = subTri_ep1vp1;

							e_tri->neighbour_p2p0 = subTri_ep0vp0;
							subTri_ep0vp0->neighbour_p1p2 = e_tri;

							e_tri->neighbour_p1p2 = subTri_ep1vp1;
							subTri_ep1vp1->neighbour_p2p0 = e_tri;

							e_tri->neighbour_p0p1 = edge->triangle_p0p1;
							if ( edge->triangle_p0p1->p0 == e_edge->p0 ) {
								edge->triangle_p0p1->neighbour_p0p1 = e_tri;
							} else if ( edge->triangle_p0p1->p1 == e_edge->p0 ) {
								edge->triangle_p0p1->neighbour_p1p2 = e_tri;
							} else {
								edge->triangle_p0p1->neighbour_p2p0 = e_tri;
							}
						}

						// add new tri's
						addTriangle( e_tri->chunk, subTri_ep0vp0i, e_edge->p0, p0 );
						addTriangle( e_tri->chunk, subTri_ep1vp1i, p1, e_edge->p1 );

						// add new edge
						edge->triangle_p1p0 = e_tri;
						for ( auto container : containers ) {
							container->edges.push_back( edge );
						}
						delete e_edge;

					} else {
						if (leftSide) {

						} else {

						}
					}
				} else {
					// vp1 is closer to e_p0; hence e_p0->vp1 for first subdivision
					// TODO: (3 cuts); e_p0->vp1, vp1->vp0, vp0->e_p1
				}
			}

			break;
		} else if ( e_p0.between( vp0, vp1 ) && e_p1.between( vp0, vp1 ) ) {
			// existing edge is a subdivision of this edge
			// TODO

			break;
		}
	}

	// Find edges from end-points which match our end points; then neighbour edges
	EdgeChunk* p0Container = EdgeTriNode::getContainer( p0 ); // TODO: p0Container is first in containers?
	EdgeChunk* p1Container = EdgeTriNode::getContainer( p1 ); // TODO: p1Container is last in containers?

	// Connected edge to graph
	for ( auto some_edge : p0Container->edges ) {
		if ( some_edge->p0 == p0 ) {
			some_edge->p0_edges.push_back( edge );
		} else if ( some_edge->p1 == p0 ) {
			some_edge->p1_edges.push_back( edge );
		}
	}
	for ( auto some_edge : p1Container->edges ) {
		if ( some_edge->p0 == p1 ) {
			some_edge->p0_edges.push_back( edge );
		} else if ( some_edge->p1 == p1 ) {
			some_edge->p1_edges.push_back( edge );
		}
	}


	// Add edge to each chunk
	for ( auto container : containers ) {
		container->edges.push_back( edge );
	}
	*/
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
/*
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
*/
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

	Log(str(format("Chunk::addTriangle {%1%,%2%,%3%}: <%4%,%5%,%6%> <%7%,%8%,%9%> <%10%,%11%,%12%>") %
			p0->vertexIndex % p1->vertexIndex % p2->vertexIndex %
			p0->getX() % p0->getY() % p0->getZ() % 
			p1->getX() % p1->getY() % p1->getZ() % 
			p2->getX() % p2->getY() % p2->getZ() ));
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
			if ( projectedVertex == 0 ) {
				results.addResults = AddTriangleResults::TRIANGLE_ADD_FAILED;
				return results;
			}

			Vertex new_vert =  Vertex(projectedVertex->v_x, projectedVertex->v_y, projectedVertex->v_z);
			int i=0;
			for( auto vertex : terrain->vertexBuffer ) {
				if ( vertex == new_vert ) {
					Log("Borrowed vertex elsewhere..");
					// assert(false);
					results.projected_p1->vertexIndex = i;
					break;
				}
				++i;
			}
			if ( i >= terrain->vertexBuffer.size() ) {
				terrain->vertexBuffer.push_back( new_vert );
				results.projected_p1->vertexIndex = terrain->vertexBuffer.size() - 1;
			}

			results.projected_p1->terrain = terrain;

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
					Log("Borrowed vertex elsewhere..");
					// assert(false);
					results.projected_p2->vertexIndex = i;
					break;
				}
				++i;
			}
			if ( i >= terrain->vertexBuffer.size() ) {
				terrain->vertexBuffer.push_back( new_vert );
				results.projected_p2->vertexIndex = terrain->vertexBuffer.size() - 1;
			}

			results.projected_p2->terrain = terrain;

			// both points project onto the same face
			if ( facep1 == facep2 ) {

				// find midpoint between outside points
				Vertex* midpoint = new Vertex();
				midpoint->v_x = (p1end.v_x - p2end.v_x)/2 + p2end.v_x;
				midpoint->v_y = (p1end.v_y - p2end.v_y)/2 + p2end.v_y;
				midpoint->v_z = (p1end.v_z - p2end.v_z)/2 + p2end.v_z;
				results.projected_midpoint = new Voxel();

				Vertex new_vert =  Vertex(midpoint->v_x, midpoint->v_y, midpoint->v_z);
				i=0;
				for( auto vertex : terrain->vertexBuffer ) {
					if ( vertex == new_vert ) {
						Log("Borrowed vertex elsewhere..");
						// assert(false);
						results.projected_midpoint->vertexIndex = i;
						break;
					}
					++i;
				}
				if ( i >= terrain->vertexBuffer.size() ) {
					terrain->vertexBuffer.push_back( new_vert );
					results.projected_midpoint->vertexIndex = terrain->vertexBuffer.size() - 1;
				}

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

				Vertex new_vert =  Vertex(projectedVertex_p1p2->v_x, projectedVertex_p1p2->v_y, projectedVertex_p1p2->v_z);
				i=0;
				for( auto vertex : terrain->vertexBuffer ) {
					if ( vertex == new_vert ) {
						Log("Borrowed vertex elsewhere..");
						// assert(false);
						results.projected_p1Mid->vertexIndex = i;
						break;
					}
					++i;
				}
				if ( i >= terrain->vertexBuffer.size() ) {
					terrain->vertexBuffer.push_back( new_vert );
					results.projected_p1Mid->vertexIndex = terrain->vertexBuffer.size() - 1;
				}

				results.projected_p1Mid->terrain = terrain;

				// find point along edge of chunk of projected-p2 face
				results.projected_p2Mid = new Voxel();

				new_vert =  Vertex(projectedVertex_p2p1->v_x, projectedVertex_p2p1->v_y, projectedVertex_p2p1->v_z);
				i=0;
				for( auto vertex : terrain->vertexBuffer ) {
					if ( vertex == new_vert ) {
						Log("Borrowed vertex elsewhere..");
						// assert(false);
						results.projected_p2Mid->vertexIndex = i;
						break;
					}
					++i;
				}
				if ( i >= terrain->vertexBuffer.size() ) {
					terrain->vertexBuffer.push_back( new_vert );
					results.projected_p2Mid->vertexIndex = terrain->vertexBuffer.size() - 1;
				}

				results.projected_p2Mid->terrain = terrain;

				// TODO: necessary to include _THREESIDE result?
				Log("NEED TO SUBDIVIDE TRIANGLE: four points / two sides...three sides?");
				results.addResults = AddTriangleResults::TRIANGLE_ADD_FOURPOINT_TWOSIDE;
				return results;

			}

			// TODO: we're forcing TWOPOINT_TWOSIDE even if its not the case; which is
			// better, p1mid or p2mid?
			Vertex* p1mid = getSeamIntersectionPoint( &p0end, &p1end, &p2end, facep1 ); // TODO: returns 0 in some cases!?
			if (p1mid->v_x == results.projected_p1->getX() &&
				p1mid->v_y == results.projected_p1->getY() &&
				p1mid->v_z == results.projected_p1->getZ() ) {
				Log("	projected p1p2 is the SAME as p1'");
				results.addResults = AddTriangleResults::TRIANGLE_ADD_TWOPOINT_ONESIDE_NOMID;
				return results;
			} else if (p1mid->v_x == results.projected_p2->getX() &&
				p1mid->v_y == results.projected_p2->getY() &&
				p1mid->v_z == results.projected_p2->getZ() ) {
				Log("	projected p1p2 is the SAME as p2'");
				results.addResults = AddTriangleResults::TRIANGLE_ADD_TWOPOINT_ONESIDE_NOMID;
				return results;
			}
			// Vertex* p2mid = getSeamIntersectionPoint( &p0end, &p1end, &p2end, facep2 );

			// find shared midpoint between projected-p1 and projected-p2 along edge of
			// chunk (along faces)
			results.projected_p1p2 = new Voxel();

			new_vert =  Vertex(p1mid->v_x, p1mid->v_y, p1mid->v_z);
			i=0;
			for( auto vertex : terrain->vertexBuffer ) {
				if ( vertex == new_vert ) {
					Log("Borrowed vertex elsewhere..");
					// assert(false);
					results.projected_p1p2->vertexIndex = i;
					break;
				}
				++i;
			}
			if ( i >= terrain->vertexBuffer.size() ) {
				terrain->vertexBuffer.push_back( new_vert );
				results.projected_p1p2->vertexIndex = terrain->vertexBuffer.size() - 1;
			}

			results.projected_p1p2->terrain = terrain;

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
						Log("Borrowed vertex elsewhere..");
						// assert(false);
						projection->vertexIndex = i;
						break;
					}
					++i;
				}
				if ( i >= terrain->vertexBuffer.size() ) {
					terrain->vertexBuffer.push_back( new_vert );
					projection->vertexIndex = terrain->vertexBuffer.size() - 1;
				}

				projection->terrain = terrain;
				if ( p1_isOutside ) results.projected_p1 = projection;
				else results.projected_p2 = projection;
			}

			if ( projectedVertexN ) {

				// project outside voxel along p1-p2 edge, onto chunk
				results.projected_p1p2 = new Voxel();
				Vertex new_vert = Vertex(projectedVertexN->v_x, projectedVertexN->v_y, projectedVertexN->v_z);
				i=0;
				for( auto vertex : terrain->vertexBuffer ) {
					if ( vertex == new_vert ) {
						Log("Borrowed vertex elsewhere..");
						// assert(false);
						results.projected_p1p2->vertexIndex = i;
						break;
					}
					++i;
				}
				if ( i >= terrain->vertexBuffer.size() ) {
					terrain->vertexBuffer.push_back( new_vert );
					results.projected_p1p2->vertexIndex = terrain->vertexBuffer.size() - 1;
				}

				results.projected_p1p2->terrain = terrain;
			}

			// in case one of the inside voxels lays on the seam of the chunk, the outside voxel will likely project
			// directly onto the same voxel as that one...hence we only have 1 unique projection, and no midpoint
			// necessary
			if ( projectedVertex0 == 0 || projectedVertexN == 0 ) {
				results.addResults = AddTriangleResults::TRIANGLE_ADD_ONEPOINT_ONESIDE_ONEPROJ;
				Log("NEED TO SUBDIVIDE TRIANGLE: one point / one side - one projection");
				return results;
			}


			// both projections lay on the same face
			if ( facep0 == facep2 ) {
				results.addResults = AddTriangleResults::TRIANGLE_ADD_ONEPOINT_ONESIDE;
				Log("NEED TO SUBDIVIDE TRIANGLE: one point / one side");
				return results;
			}

			// find midpoint along seam of chunk between both projections
			Vertex* midpoint = getSeamIntersectionPoint( &p0end, &p1end, &p2end, facep0, facep2 ); // TODO: sometimes returns 0 !?
			results.projected_midpoint = new Voxel();

			Vertex new_vert = Vertex(midpoint->v_x, midpoint->v_y, midpoint->v_z);
			i=0;
			for( auto vertex : terrain->vertexBuffer ) {
				if ( vertex == new_vert ) {
					Log("Borrowed vertex elsewhere..");
					// assert(false);
					results.projected_midpoint->vertexIndex = i;
					break;
				}
				++i;
			}
			if ( i >= terrain->vertexBuffer.size() ) {
				terrain->vertexBuffer.push_back( new_vert );
				results.projected_midpoint->vertexIndex = terrain->vertexBuffer.size() - 1;
			}

			results.projected_midpoint->terrain = terrain;
			
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
	if (p0->vertexIndex == p1->vertexIndex || p0->vertexIndex == p2->vertexIndex || p1->vertexIndex == p2->vertexIndex) {
		results.addResults = AddTriangleResults::TRIANGLE_ADD_FAILED;
		return results;
	}
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
	if ( t < 0 || t > chunkSize.x || t > chunkSize.y || t > chunkSize.z ) {
		// TODO: this setup appears buggy in some situations....seriously why is this such a problem ?? Can we assume
		// that if we've gotten this far then the line hits inside the triangle, except that t is wrong??

		glm::vec3 plane_norm = glm::cross( e1, e2 );
		float right_side = glm::dot( plane_norm, v0 );
		t = (right_side - (glm::dot(plane_norm, lineO))) / (glm::dot(plane_norm, lineD));
		if (  t < 0 || t > chunkSize.x || t > chunkSize.y || t > chunkSize.z ) {
			// TODO: perhaps the lines intersect?
			// NOTE: the lines may not intersect precisely, but are close enough to be considered intersecting

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
	glBufferData( GL_ARRAY_BUFFER, ( terrain->vertexBuffer.size() ) * sizeof(Vertex), terrain->vertexBuffer.data(), GL_STATIC_DRAW ); // TODO: this is NOT okay!

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
	for ( auto selectionClass : selection->selections ) {
		delete selectionClass;
	}
	selection->selections.clear();

	Triangle triangle = tri->chunk->triangleBuffer[tri->triIndex];

	// Select main triangles
	TerrainSelection::SelectionClass* selectionClass = new TerrainSelection::SelectionClass();
	selectionClass->class_id = TerrainSelection::SelectionClass::CLASS_HIGHLIGHT;
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

