
/* TODO
 *
 *  > BUG: texture saving, loop through local actions
 *  > MESH referencing (server to describe meshes when loading)
 *  > ENTITY guid set
 *  > setting lights
 *  > NET actions
 *  	> local buffer object (message id, clearing/ignoring)
 *  > connection initialization
 *  > server tell users of new connection
 ***/
#define CLIENT
#include <stdio.h>

#include "config.h"
#include "util.inc.h"

#include "extern/GL/glew.h"
#include "extern/GL/freeglut.h"
#include "extern/GL/glm/gtc/random.hpp"         // value_ptr

#include "libutils/lib_logger.h"
#include "libutils/lib_resmgr.h"
#include "kernel/k_camera.h"
#include "kernel/k_net.client.h"


using namespace Logger;

void check_gl_error();
void display();
void reshape(int, int);
void keyboard(unsigned char, int, int);
void keyboardRelease(unsigned char, int, int);
void mouseClick(int,int,int,int);
void mouseMove(int,int);

int input_clock;
int input_state;
int mouse_xoffset, mouse_yoffset;
int INPUT_LEFT  = 1<<0;
int INPUT_RIGHT = 1<<1;
int INPUT_UP    = 1<<2;
int INPUT_DOWN  = 1<<3;
int INPUT_FORWARD = 1<<4;
int INPUT_BACKWARD = 1<<5;
bool mouse_down = false;
bool mouse_selecting = false;
int num_lights = 1;

bool newMsg       = false;
bool requestClose = false;
bool threadPause  = false;
bool requestDebug = false;
char msgBuf[BUFLEN];
void getMessage() {
	try {
	while (true) {
		while (threadPause) {
			usleep(100 * 1000);
		}
		Log( " Enter data to send (type 'exit' to exit): " );

		// newMsg = false;
		scanf( "%[^\n]", msgBuf );
		getchar();
		if ( strcmp( msgBuf, "exit" ) == 0 ) {
			Log("Requesting close!");
			requestClose = true;
		}
		/*
		(*net::actionBuffer.active)->push_back( LocalAction( (++net::reqid), 0, 0, msgBuf, -1 ) );
		net::send( (*net::actionBuffer.active)->back() );
		Log("sending message!");
		*/
		// newMsg = true;
		// threadPause = true;
	}
	} catch(exception& e) {
		Log("Error: getMessage()");
	}
}

namespace Input {
	char msgBuf[BUFLEN];
	thread listener;

	void getMessage();
	void startup() {
		listener = thread(Input::getMessage);
	}

	void shutdown() {
		listener.detach();
	}

	void getMessage() {
		Log( " Enter data to send (type 'exit' to exit): " );

		scanf( "%[^\n]", msgBuf );
		getchar();
		if ( strcmp( msgBuf, "exit" ) == 0 ) {
			Log("Requesting close!");
			requestClose = true;
		} else if ( strcmp( msgBuf, "d" ) == 0 ) {
			Log("Requesting debug mode!");
			requestDebug = true;
		}
		/*
		(*net::actionBuffer.active)->push_back( LocalAction( (++net::reqid), 0, 0, msgBuf, -1 ) );
		net::send( (*net::actionBuffer.active)->back() );
		Log("sending message!");
		*/
	}
};

	struct LocalWorldAction {
		LocalWorldAction( WorldAction action, uint id, uint time ) : action(action), id(id), time(time) { }
		WorldAction action; uint id; uint time;
	};
	// typedef union { WorldAction action; uint id; uint time; } LocalWorldAction;
	typedef SwapBuffer<vector< LocalWorldAction >> LocalWorldActions;
	LocalWorldActions localActions = LocalWorldActions(); // actions made locally are placed here and confirmed from the server replies
int main(int argc, char **argv) {

	// ==========================================
	// start global systems
	
	LogSystem::startup( CFG_LOGFILE );

	Log( str( format( "Starting %1%" ) % argv[0] ) );

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#ifdef CLIENT
	Log( "This is the CLIENT" );





	// ==========================================
	// startup window


	glutInit( &argc, argv );

#ifdef WIN_SETTINGS
	int width  = WIN_WIDTH,
		height = WIN_HEIGHT,
		winX   = WIN_X,
		winY   = WIN_Y;
	
	unsigned int displayMode = WIN_DISPLAY;
	int majorVersion = VERSION_MAJOR;
	int minorVersion = VERSION_MINOR;
#endif

	glutInitDisplayMode( displayMode );
	glutInitContextVersion( majorVersion, minorVersion );
	glutInitContextProfile( GLUT_CORE_PROFILE );

#ifdef DEBUG
	glutInitContextFlags( GLUT_DEBUG );
#endif

	glutInitWindowSize( width, height );
	glutInitWindowPosition( winX, winY );
	int nWindow = glutCreateWindow( argv[0] );
	// glutSetOption( GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION );


	// ==========================================
	// startup gl program

	try {
		/*
		// load individual shaders properly
		program = new ShaderProgram();
		ResourceManager::LoadProgram( &program );
		glUseProgram( program->programid );
		ShaderSet *shader = new ShaderSet();
		if ( ResourceManager::LoadShader( program, "data/shaders/shade.vert", GL_VERTEX_SHADER, SHD_PHONG | SHD_BUMP, &shader ) & ERROR ) { throw exception(); }
		Log("Loaded vert shader");
		if ( ResourceManager::LoadShader( program, "data/shaders/shade.frag", GL_FRAGMENT_SHADER, 0, &shader ) & ERROR ) { throw exception(); }
		Log("Loaded frag shader");
		ResourceManager::AddShaderParameter( shader, "in_Position", SHDIN_POSITION );
		ResourceManager::AddShaderParameter( shader, "in_Color", SHDIN_COLOR );
		ResourceManager::AddShaderParameter( shader, "in_Normal", SHDIN_NORMAL );
		ResourceManager::AddShaderParameter( shader, "in_Texcoord", SHDIN_TEXCOORD );

		program->shaders.push_back( shader );
		ResourceManager::LinkProgram( program );
		renderer = new RenderGroup();
		renderer->program = program;
		ResourceManager::ResourceManagerSystem::renderers.push_back( renderer );
		glUseProgram(0);



		// load highlight shader
		ShaderProgram *program2 = new ShaderProgram();
		ResourceManager::LoadProgram( &program2 );
		glUseProgram( program2->programid );
		ShaderSet *shader2 = new ShaderSet();
		if ( ResourceManager::LoadShader( program2, "data/shaders/highlight.vert", GL_VERTEX_SHADER, SHD_HGHLT, &shader2 ) & ERROR ) { throw exception(); }
		Log("Loaded highlight vert shader");
		if ( ResourceManager::LoadShader( program2, "data/shaders/highlight.frag", GL_FRAGMENT_SHADER, 0, &shader2 ) & ERROR ) { throw exception(); }
		Log("Loaded highlight frag shader");
		ResourceManager::AddShaderParameter( shader2, "in_Position", SHDIN_POSITION );
		ResourceManager::AddShaderParameter( shader2, "in_Color", SHDIN_COLOR );
		ResourceManager::AddShaderParameter( shader2, "in_Normal", SHDIN_NORMAL );
		ResourceManager::AddShaderParameter( shader2, "in_Texcoord", SHDIN_TEXCOORD );

		program2->shaders.push_back( shader2 );
		ResourceManager::LinkProgram( program2 );
		RenderGroup *renderer2 = new RenderGroup();
		renderer2->program = program2;
		ResourceManager::ResourceManagerSystem::renderers.push_back( renderer2 );
		glUseProgram(0);
		
		*/

		/*
		// TODO: 
		// if (glext_ARB_debug_output) {
		// 	glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB );
		// }

		// // initialize glew program
		if ( ResourceManager::LoadProgram() & ERROR ) { throw exception(); }

		// // load shaders
		if ( ResourceManager::LoadShader( "data/shaders/shade.vert", GL_VERTEX_SHADER ) & ERROR ) { throw exception(); }
		Log("Loaded vert shader");
		if ( ResourceManager::LoadShader( "data/shaders/shade.frag", GL_FRAGMENT_SHADER ) & ERROR ) { throw exception(); }

		glBindAttribLocation( ResourceManager::gl, 0, "in_Position" );
		glBindAttribLocation( ResourceManager::gl, 1, "in_Color" );
		glBindAttribLocation( ResourceManager::gl, 2, "in_Normal" );
		glBindAttribLocation( ResourceManager::gl, 3, "in_Texcoord" );
		Log("Loaded frag shader");
		Log( "Successfully loaded shaders", LOG_INFO );

		// link program
		if ( ResourceManager::LinkProgram() & ERROR ) { throw exception(); }
		*/
	
	} catch ( exception& e ) {

		// error, shutdown system
		Log( "Could not start GL program", LOG_ERROR );
		LogSystem::shutdown();

		return -1;
	}

// #define NO_INPUT
// #define PLAY_LOCALLY
#ifndef PLAY_LOCALLY
	net::startup();
	Log( "Started net" );
	while ( net::uid == -1 ) {
		net::send( (*net::actionBuffer.active)->back() );
		usleep( 1000 * 1000 );
	}
	(*net::actionBuffer.active)->clear();
	Log(str(format( "Connected as user %1%" )%net::uid));
#endif





	// ==========================================
	//

	try {
		ResourceManager::LoadWorld(true);
	} catch ( exception& e ) {
		Log( "Could not load world", LOG_ERROR );
		LogSystem::shutdown();

		return -1;
	}
	glutDisplayFunc( display );
	glutReshapeFunc( reshape );
	glutKeyboardFunc( keyboard );
	glutKeyboardUpFunc( keyboardRelease );
	glutMouseFunc( mouseClick );
	glutMotionFunc( mouseMove );


	// TODO: using first program by default
	// glUseProgram( ResourceManager::ResourceManagerSystem::programs.front()->programid );
/*	glEnable( GL_TEXTURE_2D );
	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );
	glFrontFace( GL_CCW );
	glProvokingVertex( GL_FIRST_VERTEX_CONVENTION );
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
	glDepthFunc( GL_LEQUAL );
	glDepthRange( NGL_NEAR, NGL_FAR ); */
	input_clock = glutGet( GLUT_ELAPSED_TIME );
	camera.update();


	// thread t1(getMessage);
#ifndef NO_INPUT
	Input::startup();
#endif
	while ( !requestClose ) {
		/*
		if ( newMsg ) {
			threadPause = true;

			(*net::actionBuffer.active)->push_back( LocalAction( (++net::reqid), 0, 0, msgBuf, -1 ) );
			net::send( (*net::actionBuffer.active)->back() );
			Log("sending message!");
			threadPause = false;
			newMsg = false;
		}
		*/

#ifndef PLAY_LOCALLY
		// copy local action buffer to world action buffer, net send buffer
		// when the local action has been added to the world actions, it becomes active and is added to the 
		// active part of the swapbuffer; that way we only have to check the active actions for receives
		// from the server
		for ( LocalWorldAction& localAction : (**localActions.inactive) ) {
			Log(str(format("Adding local action to World Actions (%1%): %2%")%localAction.action.action%localAction.action.args));
			(*ResourceManager::world->actions.active)->push_back( localAction.action );
			Log(str(format("Adding local action to active local actions (%1%): %2%")%localAction.action.action%localAction.action.args));
			(*localActions.active)->push_back( localAction );
			// TODO: set fast low-collision hashed id from (uid,reqid,time)
			Log(str(format("Adding local action to net action buffer (%1%): %2%")%localAction.action.action%localAction.action.args));
			(*net::actionBuffer.active)->push_back( LocalAction( (++net::reqid), 0, 0, localAction.action.action, localAction.action.args, (net::uid*256 + net::reqid) ) );
		}
		(*localActions.inactive)->clear();


		if ( !net::step(10) ) {
			break; // problem with network
		}


		// go through the received actions from server
		net::actionQueue.swap();
		for ( Action netAction : (**net::actionQueue.inactive) ) {
			// confirm net action buffer (received) with local action buffer
			// otherwise add net action to world action buffer

			// is this an action sent by us?
			int actionCount = 0, removeAction = -1;
			Log(str(format("checking received netAction (%1%): %2%")%netAction.action%netAction.args));
			for ( LocalWorldAction localAction : (**localActions.active) ) {
				Log(str(format("does netAction (%1%) == (%2%) ?")%netAction.id%localAction.id));
				if ( localAction.id == netAction.id ) {
					Log(str(format("netAction (%1%) == (%2%)")%netAction.id%localAction.id));
					// TODO: check for modifications in the action; resync modifications if necessary
					removeAction = actionCount;
					break;
				}
				++actionCount;
			}
			// remove local action
			if ( removeAction != -1 ) {
				Log("Removing local action (received from server)");
				(*localActions.active)->erase((*localActions.active)->begin()+removeAction);
				continue;
			}



			// move net action buffer (received) to action buffer
			Log(str(format("Pushing received net action to world actions (%1%): %2%")%netAction.action%netAction.args));
			(*ResourceManager::world->actions.active)->push_back( WorldAction( netAction.action, netAction.args, netAction.page, netAction.initiator, netAction.id ) );
		}
		(*net::actionQueue.inactive)->clear();

		ResourceManager::world->step( 10 );
#endif

		check_gl_error();
		glutMainLoopEvent();
		usleep( 10 * 1000 );
	}
	// glutDestroyWindow( nWindow );
	// glutLeaveMainLoop();
	// t1.detach();
#ifndef NO_INPUT
	Input::shutdown();
	Log("SHUTTING DOWN INPUT!!!");
#endif

#ifndef PLAY_LOCALLY
	net::shutdown();
	Log("SHUTDOWN NET");
#endif
#endif
	ResourceManager::shutdown();
	Log("SHUTDOWN RESMGR");
	LogSystem::shutdown();
	return -1;
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


	// check_gl_error();
	// glutMainLoop();

	// ==========================================
	// shutdown global systems
	
	// ResourceManager::shutdown();
	// LogSystem::shutdown();

	return -1;
}

void check_gl_error() {
	for ( GLenum curError = glGetError(); curError != GL_NO_ERROR; curError = glGetError() ) {
		if ( curError == GL_NO_ERROR ) continue;
		else if ( curError == GL_INVALID_VALUE ) {
			printf( "GL Error: invalid value\n" );
		} else if ( curError == GL_INVALID_ENUM ) {
			printf( "GL Error: invalid enum\n" );
		} else if ( curError == GL_INVALID_OPERATION ) {
			printf( "GL Error: invalid operation\n" );
		} else if ( curError == GL_INVALID_FRAMEBUFFER_OPERATION ) {
			printf( "GL Error: invalid framebuffer operation\n" );
		} else if ( curError == GL_OUT_OF_MEMORY ) {
			printf( "GL Error: out of memory\n" );
		} else if ( curError == GL_STACK_UNDERFLOW ) {
			printf( "GL Error: stack underflow\n" );
		} else if ( curError == GL_STACK_OVERFLOW ) {
			printf( "GL Error: stack overflow\n" );
		}
	}
}

void display() {

	glClearColor( 0.0f, 0.0f, 0.0f, 0.0f );
	glClearDepth( 1.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	ResourceManager::world->render();
	
	glutSwapBuffers();
	glutPostRedisplay();


	// =============================================
	// check for input stuff
	if ( glutGet( GLUT_ELAPSED_TIME ) - input_clock > 40 ) {
		input_clock = glutGet( GLUT_ELAPSED_TIME );
		// TODO: check individual keys pressed, respond to action
		int right = 0, up = 0, forward = 0;
		if ( input_state & INPUT_LEFT     ) right      += 1;
		if ( input_state & INPUT_RIGHT    ) right      -= 1;
		if ( input_state & INPUT_UP       ) up         -= 1;
		if ( input_state & INPUT_DOWN     ) up         += 1;
		if ( input_state & INPUT_FORWARD  ) forward    += 1;
		if ( input_state & INPUT_BACKWARD ) forward    -= 1;

		if ( input_state ) {
			camera.move( right, up, forward );
		}
	}

	check_gl_error();
}

void reshape(int width, int height) {
	camera.aspect = (float)width / (float)height;
	camera.width = (float)width;
	camera.height = (float)height;
	glViewport( 0, 0, (GLsizei)width, (GLsizei)height );
}

// ============================================== //
// keypress from regular ascii characters
void keyboard(unsigned char key, int xx, int yy) {
	Log(str(format("key: %1%")%key));
	switch(key) {
		case 27: // ESC
			requestClose = true; break;
			// glutLeaveMainLoop(); break;
		case 97: // LEFT
			input_state |= INPUT_LEFT; break;
		case 100: // RIGHT
			input_state |= INPUT_RIGHT; break;
		case 119: // BACKWARD (w)
			input_state |= INPUT_BACKWARD; break;
		case 115: // DOWN
			input_state |= INPUT_DOWN; break;
	}
}

void keyboardRelease(unsigned char key, int xx, int yy) {
	switch(key) {
		case 97: // LEFT
			input_state &= ~INPUT_LEFT; break;
		case 100: // RIGHT
			input_state &= ~INPUT_RIGHT; break;
		case 119: // BACKWARD (w)
			input_state &= ~INPUT_BACKWARD; break;
		case 115: // DOWN
			input_state &= ~INPUT_DOWN; break;
		case 111: // DROP
			{
				uint dropguid = ResourceManager::world->entities.size();
				// TODO: set proper id and time
				(*localActions.inactive)->push_back( LocalWorldAction( WorldAction::serialize_create_mesh( 1, dropguid, camera.position ), 8923, 1 ) ); break;
				// (*ResourceManager::world->actions.active)->push_back( WorldAction::serialize_create_mesh( 1, dropguid, camera.position ) ); break;
			}
		case 108: // LIGHT

			{
				// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				// TODO: Add lights

				glUseProgram( ResourceManager::world->shadermgr->renderers.front()->program->programid );
				GLint light;
				light = glGetUniformLocation( ResourceManager::world->shadermgr->renderers.front()->program->programid, str(format("lights[%1%].position")%num_lights).c_str() );
				// glUniform3fv( light, 1, glm::value_ptr(camera.position) );
				glUniform3f( light, -1*camera.position.x, -1*camera.position.y, -1*camera.position.z );
				light = glGetUniformLocation( ResourceManager::world->shadermgr->renderers.front()->program->programid, str(format("lights[%1%].diffuse")%num_lights).c_str() );
				glUniform3fv( light, 1, glm::value_ptr(glm::sphericalRand(1.0f)) );
				Log(str(format("Dropped light %1%")%num_lights));
				num_lights++;
				glUseProgram(0);
				// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			}
			break;
		case 112: // PUSH
			{
				// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				// TODO: push object
				Entity* selection = ResourceManager::world->worldPick( 0, 0 );
				if (selection) {
					Log(str(format("Pushing object! %1%")%selection));
					uchar scale = 10;
					glm::vec4 push = -1 * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f) * camera.view;
					// push = glm::normalize(push);
					Log(str(format("Requesting to push (%1%)")%selection->guid));
					(*localActions.inactive)->push_back( LocalWorldAction( WorldAction::serialize_push_entity( selection->guid, glm::vec3(push.x,push.y,push.z), scale ), 8924, 1 ) ); break;
					// (*ResourceManager::world->actions.active)->push_back( WorldAction::serialize_push_entity( selection->guid, glm::vec3(push.x,push.y,push.z), scale ) );
					// glm::vec4 push = -1 * scale * glm::vec4(0.0f, 0.0f, 1.0f, 1.0f) * camera.view;
					// Log(str(format("Pushing with: <%1%,%2%,%3%>")%push.x%push.y%push.z));
					// selection->velocity += glm::vec3( push.x, push.y, push.z );
				}


				// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			}
			break;
	}
}

void mouseClick(int button, int state, int x, int y) {
	switch(button) {
		case GLUT_RIGHT_BUTTON:
			mouse_down=( state==GLUT_DOWN );
			mouse_xoffset = x;
			mouse_yoffset = y;
			break;
		case GLUT_LEFT_BUTTON:
			if ( mouse_down ) {
				if ( state==GLUT_DOWN ) input_state |= INPUT_FORWARD;
				else input_state &= ~INPUT_FORWARD;
			} else if ( state==GLUT_DOWN && !mouse_selecting ) {
				mouse_selecting = true;

				// TODO: picking

				// project coordinates into screen-space [-1,1]
				// use the mid-section of pixel
				float xw=((float)(x+0.5)/camera.width)*2-1;
				float yw=((float)(y+0.5)/camera.height)*2-1;
				yw*=-1; // y flipped

				// project coordinates into world-space (fov)
				float fov=tan(camera.fov*3.14159265358979/((float)180*2)); // convert to fov angle (in rad)
				xw*=-1*fov*camera.aspect;
				yw*=-1*fov;

				Entity* selection = ResourceManager::world->worldPick( xw, yw );

				mouse_selecting = false;
			}
			break;
	}
}

void mouseMove(int x, int y) {
	if ( mouse_down ) {
		camera.rotY( -1 * (mouse_xoffset - x) );
		camera.rotX( mouse_yoffset - y );
	} 

	mouse_xoffset = x;
	mouse_yoffset = y;
}

