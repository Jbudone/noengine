#include "lib_resmgr.h"

namespace ResourceManager {
	// GLuint gl;
	vector<Texture> textures;
	World* world;
}


// ============================================== //
// Load a mesh from a file
Entity* ResourceManager::LoadMesh (RenderGroup* renderer, const char* filename) {


	// load the mesh file
	ifstream fileHandle( filename );
	if ( !fileHandle.is_open() ) {
		Log( str( format( "Error opening mesh file (%1%)" ) % filename ), LOG_ERROR );
		throw BadFileException(filename);
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO: store mesh elsewhere
	Mesh* mesh;
	if (renderer) {
		mesh = new Mesh(true);
		mesh->renderData->gl = renderer->program->programid;
		glGenTextures(2, mesh->renderData->textures);
	} else {
		mesh = new Mesh(false);
	}
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


	// parse obj file
	string buffer;
	while ( fileHandle.good() ) {
		getline( fileHandle, buffer );
		if ( buffer.empty() ) continue;
		
		// setup tokenizer
		CharSeparator tokenSeparator( " \t" );
		CharTokenizer tokens( buffer, tokenSeparator );
		CharTokenizer::iterator token = tokens.begin();
		CharTokenizer::iterator lastToken = tokens.end();

		// Log ( str( format( "buffer: %1%" ) % (buffer) ) );
		// Log ( str( format( "token: %1%" ) % (*token) ) );
		if ( (*token)[0] == '#' ) { continue; } // comment
		else if ( (*token) == "v" ) { // vertex
			float x = tokenToFloat(++token);
			float y = tokenToFloat(++token);
			float z = tokenToFloat(++token);
			mesh->addVertex( x, y, z );
		} else if ( (*token) == "vn" ) { // vertex normal
			float x = tokenToFloat(++token);
			float y = tokenToFloat(++token);
			float z = tokenToFloat(++token);
			mesh->addVertexNormal( x, y, z );
		} else if ( *token == "vt" ) { // vertex texcoord
			float s = tokenToFloat(++token);
			float t = tokenToFloat(++token);
			mesh->addVertexTex( s, t );
		} else if ( *token == "f" ) { // face
			/* each face contains 3 or more vertices
			 *
			 * Vertex only format is:  v1 v2 v3 ... 
			 * Vertex/Texture format:  v1/vt1 v2/vt2 v3/vt3 ...
			 * Vertex/Tex/Norm format: v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3 ...
			 * Vertex/Normal format:   v1//vn1 v2//vn2 v3//vn3 ...
			 *
			 **/

			(++token);
			ushort point_index = 0; // this helps with auto-face texcoord deciding (in case no UV mapping is specified)
			do {

				// setup sub-tokenizer (vert, norm, texcoord)
				short v_index = -1, n_index = -1, t_index = -1; // NOTE: -1 means nonexistent; do NOT use ushort since 0 is a legal index
				CharTokenizer v_indices( (*token), CharSeparator("/ ","",boost::keep_empty_tokens) );
				CharTokenizer::iterator index = v_indices.begin();

				for (uchar tokenIndex=0; (index) != v_indices.end() && tokenIndex <= 2 ; ++tokenIndex, ++index ) {
					if ( (*index).empty() ) continue;
					switch (tokenIndex) {
						case 0: // Vertex Index
							// Log( str( format( "Face Index: %1%" ) % tokenToUShort( index ) ) );
							v_index = tokenToUShort( index ) - 1; break;
						case 1: // Texcoord Index
							// Log( str( format( "Tex Index: %1%" ) % tokenToUShort( index ) ) );
							t_index = tokenToUShort( index ) -1; break;
						case 2: // Norm Index
							// Log( str( format( "Norm Index: %1%" ) % tokenToUShort( index ) ) );
							n_index = tokenToUShort( index ) -1; break;
					}
				}

				// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				// if no texcoord specified then create one
				// TODO: this assumes we're running quads, provide a NON-HARDCODED solution
				if ( t_index == -1 ) {
					if ( mesh->texcoords.empty() ) {

						float top = 0.0f, bottom = 1.0f; // TODO: test st coords for correct top/bottom
						mesh->texcoords.push_back( 1.0f ); // bottom right
						mesh->texcoords.push_back( bottom );

						mesh->texcoords.push_back( 1.0f ); // top right
						mesh->texcoords.push_back( top );

						mesh->texcoords.push_back( 0.0f ); // top left
						mesh->texcoords.push_back( top );

						mesh->texcoords.push_back( 0.0f ); // bottom left
						mesh->texcoords.push_back( bottom );

					}

					t_index = point_index % 4;
				}
				// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


				mesh->pushVertex( v_index, t_index, n_index );
				point_index++;

			} while( (++token) != tokens.end() );


		} else if ( *token == "mtllib" ) { // material file

			// TODO: test that this will work on Windows (\\ path end could return only \ which may break the mtl filename)
			ushort path_ending = string( filename ).find_last_of( "/\\" ) + 1; // works for /home/user/.. or c:\\programfiles\\..
			string mtlFilename = string( filename ).substr( 0, path_ending ).append( *(++token) );
			
			// load material file
			if ( LoadMeshMaterial( mtlFilename.c_str(), mesh ) & ERROR ) {
				Log( str( format( "Error opening material file (%1%)" ) % mtlFilename ), LOG_ERROR );
				throw BadFileException(mtlFilename);
			}
			Log( str( format( "Loaded material file (%1%)" ) % mtlFilename ), LOG_INFO );

		} else {
			Log ( str( format( "Unknown line: %1%" ) % buffer ) );
		}
	
	}

	fileHandle.close();


	// finish the entity construction
	mesh->setupRenderData();
	Entity* entity = new Entity();
	entity->mesh = mesh;
	entity->updateAABB();
	world->entities.push_back(entity);
	if (renderer) renderer->entities.push_back(entity);

	return entity;
}
// ============================================== //



// ============================================== //
// Load a mesh material from a file
t_error ResourceManager::LoadMeshMaterial (const char* filename, Mesh* mesh) {

	// load the mesh material file
	ifstream fileHandle( filename );
	if ( !fileHandle.is_open() ) {
		Log( str( format( "Error opening material file (%1%)" ) % filename ), LOG_ERROR );
		throw BadFileException(filename);
	}

	// parse mtl file
	string buffer;
	while ( fileHandle.good() ) {
		getline( fileHandle, buffer );
		if ( buffer.empty() ) continue;
		
		// setup tokenizer
		CharSeparator tokenSeparator( " \t" );
		CharTokenizer tokens( buffer, tokenSeparator );
		CharTokenizer::iterator token = tokens.begin();
		CharTokenizer::iterator lastToken = tokens.end();

		// Log ( str( format( "buffer: %1%" ) % (buffer) ) );
		// Log ( str( format( "token: %1%" ) % (*token) ) );
		if ( (*token)[0] == '#' ) { continue; } // comment
		else if ( (*token) == "Ka" ) { // ambient colour
			float r = tokenToFloat(++token);
			float g = tokenToFloat(++token);
			float b = tokenToFloat(++token);
			Log( str( format( "Ka: %1%,%2%,%3%" ) % r % g % b ), LOG_DEBUG );
			mesh->color_ambient = fTriple( r, g, b );
		} else if ( (*token) == "Kd" ) { // diffuse colour
			float r = tokenToFloat(++token);
			float g = tokenToFloat(++token);
			float b = tokenToFloat(++token);
			Log( str( format( "Kd: %1%,%2%,%3%" ) % r % g % b ), LOG_DEBUG );
			mesh->color_diffuse = fTriple( r, g, b );
		} else if ( (*token) == "Ks" ) { // specular colour
			float r = tokenToFloat(++token);
			float g = tokenToFloat(++token);
			float b = tokenToFloat(++token);
			Log( str( format( "Ks: %1%,%2%,%3%" ) % r % g % b ), LOG_DEBUG );
			mesh->color_specular = fTriple( r, g, b );
		} else if ( (*token) == "map_Kd" ) { // texture

			// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			// TODO: load texture in resource mgr; use a lookup table to find the gl id and point to that
			string texFilename = *(++token);
			mesh->loadTexture( texFilename.c_str(), MeshRenderData::MESH_RENDER_TEXTURE );
			// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

		} else if ( (*token) == "bump" ) { // bumpmap
			string texFilename = *(++token);
			mesh->loadTexture( texFilename.c_str(), MeshRenderData::MESH_RENDER_TEXBUMP );
		} else {
			Log ( str( format( "Unknown line: %1%" ) % buffer ) );
		}
	
	}

	fileHandle.close();
	return NO_ERROR;
}
// ============================================== //


// ============================================== //
// Load the world
void ResourceManager::LoadWorld(bool renderable) {
	if ( world ) delete world;

	world = new World( renderable );
	world->loadWorld();
}
// ============================================== //

void ResourceManager::shutdown() {
	delete ResourceManager::world;
	ResourceManager::textures.clear();
}


Texture::Texture(const char* filename, const uchar* imageData, int width, int height) : width(width), height(height) {
	this->filename = new char[strlen(filename)+1];
	strcpy( this->filename, filename );
	unsigned int size = getSize();
	this->imageData = new uchar[size+1];
	memcpy( this->imageData, imageData, size );
}
Texture::Texture(const Texture &rhs) {
	this->width = rhs.width;
	this->height = rhs.height;
	this->filename = new char[strlen(rhs.filename)+1];
	strcpy( this->filename, rhs.filename );
	unsigned int size = getSize();
	this->imageData = new uchar[size+1];
	memcpy( this->imageData, rhs.imageData, size );
}
Texture& Texture::operator=(const Texture &rhs) {
	this->width = rhs.width;
	this->height = rhs.height;
	this->filename = new char[strlen(rhs.filename)+1];
	strcpy( this->filename, rhs.filename );
	unsigned int size = getSize();
	this->imageData = new uchar[size+1];
	memcpy( this->imageData, rhs.imageData, size );

	return *this;
}

int Texture::getSize() {
	return width * height * 3 * 1;
}
Texture::~Texture() {
	delete filename;
	SOIL_free_image_data( this->imageData );
}

Texture* Texture::loadTexture(const char* filename) {

	// load texture image
	// check if texture has already been loaded
	Texture* texture = 0;
	for ( Texture& _texture : ResourceManager::textures ) {
		if ( strcmp(_texture.filename, filename) == 0 ) {
			texture = &_texture;
			break;
		}
	}
	if ( texture == 0 ) {
		// create texture
		int width, height;
		unsigned char* imageData = SOIL_load_image( filename, &width, &height, 0, SOIL_LOAD_RGB );
		texture = new Texture(filename, imageData, width, height);
		SOIL_free_image_data( imageData );

		ResourceManager::textures.push_back( *texture );
	}

	return texture;
}
