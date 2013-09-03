#include "libutils/lib_shdmgr.h"


ShaderManager::ShaderManager() {

}





// ============================================== //
// Create gl program
ShaderProgram* ShaderManager::createProgram () {

	// initialize glew
	GLuint err = glewInit();
	if ( err != GLEW_OK ) {
		Log( str( format( "Could not load program: %1%" ) % glewGetErrorString(err) ), LOG_ERROR );
		return NULL;
	}
	Log( str( format( "Status: using GLEW %1%" ) % glewGetString(GLEW_VERSION) ), LOG_INFO );

	// create our gl program
	ShaderProgram* shaderProgram = new ShaderProgram();
	programs.push_back( shaderProgram );
	shaderProgram->programid = glCreateProgram();
	glUseProgram( shaderProgram->programid );

	return shaderProgram;
}

// ============================================== //
// Create a renderer
RenderGroup* ShaderManager::createRenderer (ShaderProgram* shaderProgram) {
	RenderGroup* renderer = new RenderGroup();
	renderer->program = shaderProgram;
	renderers.push_back( renderer );
	return renderer;
}

// ============================================== //
// Link the shaders in gl program
t_error ShaderManager::linkProgram (ShaderProgram* shaderProgram) {
	
	GLuint gl = shaderProgram->programid;

	// attach shaders, bind attributes
	uint paramCount = 0;
	for ( auto shaderSet : shaderProgram->shaders ) {
		for ( auto param : shaderSet->parameters ) {
			glBindAttribLocation( gl, paramCount, param->name.c_str() );
			paramCount++;
		}
		for ( auto shaderLink : shaderSet->shaderLinks ) {
			glAttachShader( gl, shaderLink );
		}
	}

	// link program
	glLinkProgram( gl );
	GLint status;
	glGetProgramiv( gl, GL_LINK_STATUS, &status );
	if ( status==GL_FALSE ) {
		GLint infoLogLength;
		glGetProgramiv( gl, GL_INFO_LOG_LENGTH, &infoLogLength );

		GLchar *strInfoLog = new GLchar[infoLogLength+1];
		glGetProgramInfoLog( gl, infoLogLength, NULL, strInfoLog );
		Log( str( format( "GL Program linker failure: %1%" ) % strInfoLog ), LOG_ERROR );
		delete[] strInfoLog;
		return ERROR;
	}

	return NO_ERROR;
}


// ============================================== //
// Load a shader from a file
t_error ShaderManager::loadShader (ShaderProgram* program, const char* filename, GLenum shaderType, uint description, ShaderSet** shaderSet) {

	// prep for loading the shader file
	stringstream shaderSrc;   // stream file into this
	string shaderSrcStr;      // string copy of shaderSrc
	const char *shaderSrcPtr; // pointer to shaderSrcStr

	// load the shader file
	ifstream fileHandle( filename );
	if ( !fileHandle.is_open() ) {
		Log( str( format( "Error opening shader file (%1%)" ) % filename ), LOG_ERROR );
		throw BadFileException(filename);
	}

	// load the shader source
	shaderSrc << fileHandle.rdbuf();
	fileHandle.close();
	
	// create the gl shader
	GLuint shaderLink = glCreateShader( shaderType );
	if ( shaderLink == 0 ) {
		Log( str( format( "Error creating shader (%1%)" ) % filename ), LOG_ERROR );
		return ERROR;
	}

	// compile shader
	shaderSrcStr = shaderSrc.str();      // NOTE: stringstream.str() creates a temporary str; stringstream.str().c_str() is unsafe!
	shaderSrcPtr = shaderSrcStr.c_str();
	int shaderSrcLength = shaderSrcStr.size();
	Log( str( format( "Shader: %1%" ) % shaderSrcStr ), LOG_DEBUG );
	glShaderSource( shaderLink, 1, &shaderSrcPtr, &shaderSrcLength );
	glCompileShader( shaderLink );

	// compilation issues?
	GLint status;
	glGetShaderiv( shaderLink, GL_COMPILE_STATUS, &status );
	if ( status==GL_FALSE ) {
		GLint infoLogLength;
		glGetShaderiv( shaderLink, GL_INFO_LOG_LENGTH, &infoLogLength );

		GLchar *strInfoLog = new GLchar[infoLogLength+1];
		glGetShaderInfoLog( shaderLink, infoLogLength, NULL, strInfoLog );
		Log( str( format( "Shader compilation failure: %1%" ) % strInfoLog ), LOG_ERROR );
		delete[] strInfoLog;
		return ERROR;
	}

	// attach shader to shaderset
	if ( (*shaderSet) == NULL ) {
		// create shaderset if its not created already
		ShaderSet *_shaderSet = new ShaderSet();
		shaderSet = &_shaderSet;
		shaders.push_back( _shaderSet );
	}
	(*shaderSet)->shaderLinks.push_back( shaderLink );
	(*shaderSet)->description |= description;

	return NO_ERROR;
}

t_error ShaderManager::addShaderParameter ( ShaderSet* shaderSet, const char *param, uint description ) {

	ShaderParameter* parameter = new ShaderParameter();
	parameter->name            = param;
	parameter->description     = description;
	parameter->output          = false;
	shaderSet->parameters.push_back( parameter );

	return NO_ERROR;
}

ShaderManager::~ShaderManager() {
	for ( auto renderer : renderers ) {
		for ( auto shader : renderer->program->shaders ) {
			for ( auto param : shader->parameters ) {
				delete param;
			}
			delete shader;
		}
		delete renderer->program;
		delete renderer;
	}
	shaders.clear();
	programs.clear();
	renderers.clear();
}
