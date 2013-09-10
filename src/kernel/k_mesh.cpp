#include "kernel/k_mesh.h"

// ============================================== //
Mesh::Mesh(bool renderable) : renderable(renderable) {
	renderData = 0;
	if ( renderable ) renderData = new MeshRenderData();
}
Mesh::~Mesh() { }
// ============================================== //


// ============================================== //
void Mesh::addVertex(const float x, const float y, const float z) {
	vertices.push_back(x);
	vertices.push_back(y);
	vertices.push_back(z);
	colors.push_back(1.0);
	colors.push_back(0.3);
	colors.push_back(1.0);
}
// ============================================== //


// ============================================== //
void Mesh::addVertexNormal(const float x, const float y, const float z) {
	normals.push_back(x);
	normals.push_back(y);
	normals.push_back(z);
}
// ============================================== //


// ============================================== //
void Mesh::addVertexTex(const float s, const float t) {
	texcoords.push_back(s);
	texcoords.push_back(t);
}
// ============================================== //


// ============================================== //
void Mesh::pushVertex(ushort v_index, ushort t_index, ushort n_index) {
	v_index *= 3;
	t_index *= 2;
	n_index *= 3;
	vertexBuffers.push_back( VertexBuffer( vertices[v_index], vertices[v_index+1], vertices[v_index+2],
											texcoords[t_index], texcoords[t_index+1],
											normals[n_index], normals[n_index+1], normals[n_index+2] ) );
}
// ============================================== //


// ============================================== //
// load image and send to shader texture
void Mesh::loadTexture(const char* filename, const uchar textureType) {
	textures.push_back( TextureFile( filename, textureType ) );
}
// ============================================== //


// ============================================== //
/* Where (if anywhere) does the ray intersect with the given triangle coordinates?
 *
 * This uses a standardized method for ray/triangle intersection testing; this code is not
 * my own, but has been accepted as the industry standard for ray/triangle testing
 **/
float Mesh::rayIntersectTriangle(glm::vec3 position, glm::vec3 direction,
		glm::vec3 v0, glm::vec3 v1, glm::vec3 v2) {

	
	glm::vec3 u = v1 - v0;
	glm::vec3 v = v2 - v0;
	glm::vec3 norm = glm::cross( u, v );
	
	if ( norm == glm::vec3(0.0f) ) {
		return 0; // triangle is degenerate
	}

	
	glm::vec3 triRayDir = position - v0;
	float triRayNormDot = -1 * glm::dot( norm, triRayDir );
	float rayNormDot    = glm::dot( norm, direction );
	// if (rayNormDot < 0) rayNormDot *= -1; // JB: double sided triangle?
	float r             = triRayNormDot / rayNormDot;
	if ( fabs(rayNormDot) < 0.0000001 ) {
		Log("fabs == 0?");
		if ( triRayNormDot == 0 ) return 1; // TODO: lays ON triangle..where exactly?
		return 0; // ray disjoint from plane
	}

	
	if ( r < 0.0f ) return 0; // ray points away from triangle
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
		return 0; // intersection outside triangle
	}
	t = ( uv * wu - uu * wv ) / D;
	if ( t < 0.0 || ( s + t ) > 1.0 ) {
		// Log("  outside t");
		return 0; // intersection outside triangle
	}
	
	glm::vec3 vHit = intersection - position;
	return glm::dot( vHit, vHit );
	return 1; // TODO: intersection inside triangle..where exactly?
}
// ============================================== //


// ============================================== //
float Mesh::lineIntersects(glm::vec3 position, glm::vec3 direction) {

	// loop through quads
	glm::vec4 triangles[2][3];


	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// modify the vertex buffers for our world-space mesh
	// TODO: include scale/rotation
	glm::mat4 model = -1 * glm::translate( glm::mat4(1.0f), this->position );
	// model = glm::scale( model, glm::vec3(0.5f) );
	glm::mat4 mvp = model;
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	float nearestHit = 0.0f, thisHit = 0.0f;
	for ( uint i=0; i<vertexBuffers.size(); i+=4 ) {
		// triangulate this quad, and project into world space
		triangles[0][0] = triangles[1][0] = mvp * glm::vec4( vertexBuffers[i+0].v_x, vertexBuffers[i+0].v_y, vertexBuffers[i+0].v_z, 1.0f );
		triangles[0][1]                   = mvp * glm::vec4( vertexBuffers[i+1].v_x, vertexBuffers[i+1].v_y, vertexBuffers[i+1].v_z, 1.0f );
		triangles[0][2] = triangles[1][1] = mvp * glm::vec4( vertexBuffers[i+2].v_x, vertexBuffers[i+2].v_y, vertexBuffers[i+2].v_z, 1.0f );
		triangles[1][2]                   = mvp * glm::vec4( vertexBuffers[i+3].v_x, vertexBuffers[i+3].v_y, vertexBuffers[i+3].v_z, 1.0f );
		
		// intersection w/ triangle?
		thisHit = rayIntersectTriangle( position, direction, triangles[0][0].xyz, triangles[0][1].xyz, triangles[0][2].xyz );
		if ( thisHit > 0.0f ) nearestHit = thisHit;

		thisHit = rayIntersectTriangle( position, direction, triangles[1][0].xyz, triangles[1][1].xyz, triangles[1][2].xyz );
		if ( thisHit > 0.0f && ( nearestHit == 0.0f || thisHit < nearestHit ) ) nearestHit = thisHit;
	}
	
	// return nearest intersection
	return nearestHit;
}
// ============================================== //


// ============================================== //
void Mesh::setupRenderData() {
	if ( renderable ) {
		renderData->construct(this);
	}
}
// ============================================== //


// ============================================== //
void Mesh::render() {
	if (renderable) {
		
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// TODO: implement translation / rotation / scale
		glm::mat4 model = glm::translate( glm::mat4(1.0f), position );
		// model = glm::scale( model, glm::vec3(0.5f) ); // TODO: note scale does not work for collide routine
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		renderData->render(model);
	}
}
// ============================================== //

MeshRenderData::MeshRenderData() { }
MeshRenderData::~MeshRenderData() { }


// ============================================== //
t_error MeshRenderData::loadTexture(const char* filename, uchar texType) {
	glUseProgram(gl);
	Log( str( format("Loading texture: %1%") % filename ) );

	// load texture image
	Texture* texture = Texture::loadTexture( filename );

	// copy file to opengl
	glActiveTexture( GL_TEXTURE0 + texType );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, textures[texType] );
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture->width, texture->height, 0,
			GL_RGB, GL_UNSIGNED_BYTE, texture->imageData );
	if ( texType == MESH_RENDER_TEXTURE ) {
		glUniform1i( glGetUniformLocation( gl, "Tex" ), texType );
		hasTexture = true;
	} else if ( texType == MESH_RENDER_TEXBUMP ) {
		glUniform1i( glGetUniformLocation( gl, "Bump" ), texType );
		hasBumpmap = true;
	}

	// mipmapping
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glGenerateMipmap( GL_TEXTURE_2D );

	// cleanup
	glActiveTexture(0);
	Log( str( format("Loaded texture %1%") % filename ) );

	return NO_ERROR;


}
// ============================================== //


// ============================================== //
// setup display list for mesh
t_error MeshRenderData::construct(Mesh* mesh, bool reconstruction) {

	this->mesh = mesh;
	if ( !reconstruction ) {
		for ( auto tex : mesh->textures ) {
			loadTexture( tex.filename.c_str(), tex.texType );
		}
	}

	glUseProgram(gl);
	// TODO: delete vao (in case its already setup for another program)

	// enable display list
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );

	// setup buffer object
	glGenBuffers( 1, &vbo );
	glBindBuffer( GL_ARRAY_BUFFER, vbo );
	glBufferData( GL_ARRAY_BUFFER, ( mesh->vertexBuffers.size() ) * sizeof(VertexBuffer), mesh->vertexBuffers.data(), GL_STATIC_DRAW );

	GLint glVertex   = glGetAttribLocation  ( gl, "in_Position" ) ;
	// GLint glColor = glGetAttribLocation  ( gl, "in_Color"    ) ;
	GLint glTexcoord = glGetAttribLocation  ( gl, "in_Texcoord" ) ;
	GLint glNormal   = glGetAttribLocation  ( gl, "in_Normal"   ) ;
	GLint glAmbient  = glGetUniformLocation ( gl, "ambient"     ) ;
	GLint glDiffuse  = glGetUniformLocation ( gl, "diffuse"     ) ;
	GLint glSpecular = glGetUniformLocation ( gl, "specular"    ) ;
	glEnableVertexAttribArray( glVertex );
	// glEnableVertexAttribArray( glColor );
	glEnableVertexAttribArray( glTexcoord );
	glEnableVertexAttribArray( glNormal );

	// load data into shader
	glVertexAttribPointer( glVertex, 3, GL_FLOAT, GL_FALSE, sizeof(VertexBuffer), (void*)0 );
	// glVertexAttribPointer( glColor, 3, GL_FLOAT, GL_FALSE, 0, (void*)(vertices.size() * sizeof(float)) );
	glVertexAttribPointer( glTexcoord, 2, GL_FLOAT, GL_FALSE, sizeof(VertexBuffer), (void*)( sizeof(float) * 3 ) );
	glVertexAttribPointer( glNormal, 3, GL_FLOAT, GL_TRUE, sizeof(VertexBuffer), (void*)( sizeof(float) * 5 ) );
	glUniform3f( glAmbient,  mesh->color_ambient.t1,  mesh->color_ambient.t2,  mesh->color_ambient.t3 );
	glUniform3f( glDiffuse,  mesh->color_diffuse.t1,  mesh->color_diffuse.t2,  mesh->color_diffuse.t3 );
	glUniform3f( glSpecular, mesh->color_specular.t1, mesh->color_specular.t2, mesh->color_specular.t3 );

	// setup textures
	glActiveTexture( GL_TEXTURE1 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, textures[1] );

	glActiveTexture( GL_TEXTURE0 );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, textures[0] );


	// cleanup
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindVertexArray( 0 );

	return NO_ERROR;
}
// ============================================== //


// ============================================== //
t_error MeshRenderData::clear() {
	// clear
	glDeleteVertexArrays( 1, &vao );
	glDeleteBuffers( 1, &vbo );
	return NO_ERROR;
}
// ============================================== //


// ============================================== //
// draw mesh
t_error MeshRenderData::render(glm::mat4 model) {
	glUseProgram(gl);
	// enable display list
	glBindVertexArray( vao );


	GLint glMVP = glGetUniformLocation( gl, "MVP" );


	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO: implement translation / rotation / scale
	// glm::mat4 model = glm::translate( glm::mat4(1.0f), position );
	// model = glm::scale( model, glm::vec3(0.5f) );
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


	glm::mat4 mvp = camera.perspectiveView * model;
	mvp = glm::transpose( mvp );
	glUniformMatrix4fv( glMVP, 1, GL_FALSE, glm::value_ptr(mvp) );

	// glDrawArrays( GL_TRIANGLE_STRIP, 0, vertexBuffers.size() );
	glDrawArrays( GL_QUADS, 0, mesh->vertexBuffers.size() );
	glBindVertexArray( 0 );

	return NO_ERROR;
}
