#include "kernel/k_terrain.h"

const Point<int> Chunk::chunkSize = Point<int>(255,255,255);

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


	glUseProgram(gl);
	// TODO: delete vao (in case its already setup for another program)

	// enable display list
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );

	// setup buffer object
	glGenBuffers( 1, &vbo );
	glBindBuffer( GL_ARRAY_BUFFER, vbo );
	glBufferData( GL_ARRAY_BUFFER, ( vertexBuffer.size() ) * sizeof(TerrainVertexBuffer), vertexBuffer.data(), GL_STATIC_DRAW );

	GLint glVertex   = glGetAttribLocation  ( gl, "in_Position" ) ;
	glEnableVertexAttribArray( glVertex );

	// load data into shader
	glVertexAttribPointer( glVertex, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertexBuffer), (void*)0 );

	// cleanup
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );

}
// ============================================== //


// ============================================== //
void Terrain::render() {

	glUseProgram(gl);
	// enable display list
	glBindVertexArray( vao );


	GLint glMVP = glGetUniformLocation( gl, "MVP" );


	glm::mat4 mvp = camera.perspectiveView;
	mvp = glm::transpose( mvp );
	glUniformMatrix4fv( glMVP, 1, GL_FALSE, glm::value_ptr(mvp) );

	// glDrawArrays( GL_TRIANGLE_STRIP, 0, vertexBuffers.size() );
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glUniform3f( glGetUniformLocation( gl, "in_color" ), 0.0f, 1.0f, 0.0f );
	glDrawArrays( GL_QUADS, 0, vertexBuffer.size() );
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glUniform3f( glGetUniformLocation( gl, "in_color" ), 1.0f, 1.0f, 1.0f );
	glDrawArrays( GL_QUADS, 0, vertexBuffer.size() );
	glBindVertexArray( 0 );

	// TODO: draw terrain decal (selection indicator)
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glUniform3f( glGetUniformLocation( gl, "in_color" ), 0.6f, 0.0f, 0.8f );
	vector<TerrainVertexBuffer> selectionBuffer;
	QuadTree<TerrainVertexBuffer>* bottomLeft = verticesQuadTree;
	QuadTree<TerrainVertexBuffer>* leftEdge;
	float far_point = 100;
	while ( bottomLeft->forward != 0 && (bottomLeft->forward->self.v_z - 0) < far_point ) {
		leftEdge = bottomLeft;
		while ( bottomLeft->right != 0 && (bottomLeft->right->self.v_x - 0) > -far_point ) {
// Log(str(format("SELECTION POINT: (%1%,%2%)")%bottomLeft->forward->self.v_x%bottomLeft->right->self.v_z));
			selectionBuffer.push_back( bottomLeft->self );
			selectionBuffer.push_back( bottomLeft->right->self );
			selectionBuffer.push_back( bottomLeft->right->forward->self );
			selectionBuffer.push_back( bottomLeft->forward->self );

			bottomLeft = bottomLeft->right;
		}
		bottomLeft = leftEdge->forward;
	}
	GLuint decal_vbo;
	glGenBuffers( 1, &decal_vbo );
	glBindBuffer( GL_ARRAY_BUFFER, decal_vbo );
	glBufferData( GL_ARRAY_BUFFER, ( selectionBuffer.size() ) * sizeof(TerrainVertexBuffer), selectionBuffer.data(), GL_STATIC_DRAW );

	GLint glVertex   = glGetAttribLocation  ( gl, "in_Position" ) ;
	glEnableVertexAttribArray( glVertex );

	// load data into shader
	glVertexAttribPointer( glVertex, 3, GL_FLOAT, GL_FALSE, sizeof(TerrainVertexBuffer), (void*)0 );
	glDrawArrays( GL_QUADS, 0, selectionBuffer.size() );
	glDeleteBuffers( 1, &decal_vbo );

}
// ============================================== //


	/******************************************************************************/
	/*
							  Procedural Terrain Generation
																				   */
	/******************************************************************************/



// ============================================== //
void decompressTerrain() {
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// create compressed terrain
	// compressedQuadTree = new QuadTree<ControlPoint>( { { 0, 0 }, { 0, 0 }, 0, 0, 0, 0, 0, 1 } );
	// verticesQuadTree = 0;
	// vertexBuffer.clear();
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	
	// TODO: extend compressed tree; intersect all control points to make a fully linked
	// tree grid

	// TODO: start from (0,0,0) point, find next stopping point (control point or
	// 	boundary) along dx and dy; tessellate provided quad
	// Quad Tessellation: find all critical points along dx and dy within quad; intersect
	// 	create grid.. note the points along the seams for morphing with other quad pages
	// 	and other chunks and other portals/pages
	// Crit Points: note point at current spot, jump 2 steps forwards for endpoint of
	// 	triangle, and 1 step forwards for midsection of triangle; calculate z coord of
	// 	points; |P1-P2| == error; continue forwards until error exceeds threshold, then
	// 	use previous point (unless previous point is original point, then accept point
	// 	anyways)  OR  until boundary reached


}
// ============================================== //


// ============================================== //
void Terrain::generateTerrain() {

	// Chunk Generation
	// 	create a chunk hextree along the entire map grid; even if no voxels will exist in
	// 	some of these chunks, its important to set them up (sort of like an empty space
	// 	rather than a null space).
	int x_chunks = width / Chunk::chunkSize.x,
		y_chunks = height / Chunk::chunkSize.y,
		z_chunks = depth / Chunk::chunkSize.z;
	// TODO: routine to setup the bottom w/ a ptr to the top, and traverse along top while
	// setting up bottom, then link each up/down adjacent chunks
	
	// TODO: create chunks along X axis
	// TODO: for each Y; for each Z; create chunks that link w/ previous (Y,Z) axis
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
		chunkPosition.y += Chunk::chunkSize.y;
		chunkPosition.z = 0;
	}


	// initialize voxel quadtree
	verticesQuadTree = generateVoxel(0,0);


	QuadTree<TerrainVertexBuffer>* bottomLeft = verticesQuadTree;
	QuadTree<TerrainVertexBuffer>* leftEdge;
	while ( bottomLeft->forward != 0 ) {
		leftEdge = bottomLeft;
		while ( bottomLeft->right != 0 ) {
			vertexBuffer.push_back( bottomLeft->self );
			vertexBuffer.push_back( bottomLeft->right->self );
			vertexBuffer.push_back( bottomLeft->right->forward->self );
			vertexBuffer.push_back( bottomLeft->forward->self );

			bottomLeft = bottomLeft->right;
		}
		bottomLeft = leftEdge->forward;
	}



	/*
	for ( int z = 0; z < depth; z+=inc ) {
		for ( int x = 0; x < width; x+=inc ) {


			float divider = 20;
			float v_x = -1 * (float)x,
				  v_z = (float)z,
				  sep = inc/divider;
			float v_y_00 = perlinNoise( x,    z,    persistence, octaves ) / divider,
				  v_y_10 = perlinNoise( x+inc,  z,    persistence, octaves ) / divider,
				  v_y_01 = perlinNoise( x,    z+inc, persistence, octaves )  / divider,
				  v_y_11 = perlinNoise( x+inc,  z+inc, persistence, octaves )  / divider;
			Log(str(format( "Terrain Point: (%1%,%2%,%3%)" ) % v_x % v_y_00 % v_z ) );
			vertexBuffer.push_back( TerrainVertexBuffer( v_x, -1 * v_y_00, v_z ) ); // Lower Left
			vertexBuffer.push_back( TerrainVertexBuffer( v_x, -1 * v_y_01, v_z+inc ) ); // Upper Left
			vertexBuffer.push_back( TerrainVertexBuffer( v_x+inc, -1 * v_y_11, v_z+inc ) ); // Upper Right
			vertexBuffer.push_back( TerrainVertexBuffer( v_x+inc, -1 * v_y_10, v_z ) ); // Lower Right
			// vertexBuffer.push_back( TerrainVertexBuffer( v_x, -1 * perlinNoise( x, z, persistence, octaves )/divider, v_z ) ); // Lower Left
			// vertexBuffer.push_back( TerrainVertexBuffer( v_x, -1*perlinNoise( x, z+1, persistence, octaves )/divider, v_z+(1/divider) ) ); // Upper Left
			// vertexBuffer.push_back( TerrainVertexBuffer( v_x+(1/divider), -1*perlinNoise( x+1, z+1, persistence, octaves )/divider, v_z+(1/divider) ) ); // Upper Right
			// vertexBuffer.push_back( TerrainVertexBuffer( v_x+(1/divider), -1*perlinNoise( x+1, z, persistence, octaves )/divider, v_z ) ); // Lower Right
		}
	}
	*/
}
// ============================================== //


// ============================================== //
void Terrain::generateTerrain2() {
	// TODO: crawl through and create quadtree; random number, a chance for SOMETHING to
	// happen (create indicator point -- connect to nearest point/edge directly
	// below/left), pick slope values from 
	//
	// +v -a: beginning of rolling hill
	// -v -a: drop
	// +v +a: cliff
	// -v +a: down slope
	//
	// random num: [0,10], 0 drop, 10 cliff: if drop/cliff then range [0,20] where <2
	// drop, >18 cliff. 
	
	compressedQuadTree = new QuadTree<ControlPoint>( { { 0, 0 }, { 0, 0 }, 0, 0, 0, 0, 0, 1 } );
}
// ============================================== //


// ============================================== //
void Terrain::fetchCriticalPoints() {
	// TODO: go through voxel quadtree to find where dx & dy changes (critical points);
	// TODO: backstep points and add previous control points (left/behind)
	// TODO: create intersection point between backstepped points; link in quadtree
}
// ============================================== //


// ============================================== //
QuadTree<TerrainVertexBuffer>* Terrain::generateVoxel(int x, int z, QuadTree<TerrainVertexBuffer>* backVoxel, QuadTree<TerrainVertexBuffer>* rightVoxel) {
	float persistence = 0.8;
	int octaves     = 8,
		divider     = 1,
		inc         = 50;
	float v_x = (float)x * 0.5,
		  v_z = (float)z * 0.5,
		  v_y = perlinNoise( x*1, z*1, persistence, octaves ) * 0.5 + 10;
	QuadTree<TerrainVertexBuffer>* voxel = new QuadTree<TerrainVertexBuffer>( TerrainVertexBuffer( v_x, v_y, v_z ) );
	if ( backVoxel ) voxel->backward = backVoxel;
	Log(str(format( "Terrain Point: (%1%,%2%,%3%)" ) % v_x % v_y % v_z ) );
	if ( rightVoxel ) {
		voxel->right = rightVoxel;
		rightVoxel->left = voxel;
	} else if ( x < width ) {
		voxel->right = generateVoxel(x+50,z);
		voxel->right->left = voxel;
	}

	if ( z < depth ) {
		rightVoxel = 0;
		if ( voxel->right && voxel->right->forward ) rightVoxel = voxel->right->forward;
		voxel->forward = generateVoxel(x,z+50, voxel, rightVoxel);
	}
	
	Voxel* voxel_ = new Voxel();
	voxel_->vertex = voxel;
	addVoxelIntoChunk( headChunk, voxel_ );

	return voxel;
}
// ============================================== //


// ============================================== //
Chunk::Chunk(Point<int> position) : voxels(position.x, position.y, position.x, position.x+chunkSize.x, position.y+chunkSize.y, position.z+chunkSize.z) {
	worldOffset.x = position.x;
	worldOffset.y = position.y;
	worldOffset.z = position.z;

}
// ============================================== //


// ============================================== //
Chunk* Terrain::addVoxelIntoChunk(Chunk* chunk, Voxel* voxel) {

	float x = voxel->vertex->self.v_x,
		  y = voxel->vertex->self.v_y,
		  z = voxel->vertex->self.v_z;
	if ( x - chunk->worldOffset.x > chunk->chunkSize.x ) return addVoxelIntoChunk(chunk->right, voxel);
	if ( y - chunk->worldOffset.y > chunk->chunkSize.y ) return addVoxelIntoChunk(chunk->above, voxel);
	if ( z - chunk->worldOffset.z > chunk->chunkSize.z ) return addVoxelIntoChunk(chunk->infront, voxel);

	chunk->voxels.addNode( voxel, (x-chunk->worldOffset.x), (y-chunk->worldOffset.y), (z-chunk->worldOffset.z) );
	return chunk;

}
// ============================================== //


// ============================================== //
Chunk* Terrain::createChunks(Point<int> position, Chunk* below, Chunk* behind) {

	Chunk* chunk = new Chunk(position);

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
TerrainSelection* Terrain::terrainPick(float xw, float yw) {
	// TODO
	return 0;
}
// ============================================== //

