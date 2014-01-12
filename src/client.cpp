
/***
 * Client
 *
 *	    Main file to run for the client
 *
 * TODO
 *  - high precision clock for loop
 *  - protocol for mapping input to game requests
 *
 **/


#include "config.h"
#include "util.inc.h"

#include "extern/GL/glew.h"
#include "extern/GL/freeglut.h"
#include "extern/GL/glm/gtc/random.hpp"         // value_ptr

#include "kernel/k_camera.h"
#include "libutils/lib_resmgr.h"
#include "kernel/k_net.client.h"

// #define NO_INPUT
#define PLAY_LOCALLY


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

	// LocalWorldAction
	//
	// This is an action which has been made locally, and is saved in the local
	// action buffer waiting for a response from the server to see if it needs
	// to be modified or cancelled
	struct LocalWorldAction {
		LocalWorldAction( WorldAction action, uint id, uint time ) : action(action), id(id), time(time) { }
		WorldAction action; uint id; uint time;
	};
	typedef SwapBuffer<vector< LocalWorldAction >> LocalWorldActions;
	LocalWorldActions localActions = LocalWorldActions(); // actions made locally are placed here and confirmed from the server replies
	UIManager* ui;
int main(int argc, char **argv) {
	
	LogSystem::startup( CFG_LOGFILE );
	Log( str( format( "Starting %1%" ) % argv[0] ) );
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
	glutSetOption( GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_CONTINUE_EXECUTION );

	glutDisplayFunc( display );
	glutReshapeFunc( reshape );
	glutKeyboardFunc( keyboard );
	glutKeyboardUpFunc( keyboardRelease );
	glutMouseFunc( mouseClick );
	glutMotionFunc( mouseMove );


	// ==========================================
	// start global systems

#ifndef PLAY_LOCALLY
	Log( "starting net.." );
	net::startup();
	// wait for reply from server for our UID (which means we've connected)
	while ( net::uid == -1 ) {
		net::send( (*net::actionBuffer.active)->back() );
		usleep( 1000 * 1000 );
	}
	(*net::actionBuffer.active)->clear();
	Log(str(format( "Connected as user %1%" )%net::uid));
#endif

	try {
		ResourceManager::LoadWorld(true);
	} catch ( exception& e ) {
		Log( "Could not load world", LOG_ERROR );
		LogSystem::shutdown();
		return -1;
	}

	ui = new UIManager( 
			ResourceManager::world->shadermgr->renderers.at(2)->program->programid,
			ResourceManager::world->shadermgr->renderers.back()->program->programid,
			width, height );

#ifndef NO_INPUT
	Input::startup();
#endif




	/**
	 * Main Loop
	 *
	 * We may create Local actions which are applied locally and stored on a
	 * buffer. These local actions are sent off to the server and await
	 * acknowledgement. Its possible that the action may need to be modified,
	 * like another user closes a door as we're trying to walk through the door
	 * way, and we haven't yet seen the door close; then afterwards we turn left
	 * and move forward. The server could reply and tell us that our first
	 * action failed, in which case we would cancel it out of the buffer and
	 * resync all of the following actions (move back to our original spot, then
	 * turn left, then move forward). 
	 *
	 * Other actions will be received from the server (from all users), and
	 * applied to in the world.
	 * NOTE: we do not acknowledge packets from the server (to cut down on noise
	 * 		and processing time for the server); keep this in mind in case any
	 * 		bugs show up later on which require us to change this around
	 ***/
	input_clock = glutGet( GLUT_ELAPSED_TIME );
	camera.update();
	Log( "Ready." );
	while ( !requestClose ) {

		// copy local action buffer to world action buffer, net send buffer
		// when the local action has been added to the world actions, it becomes active and is added to the 
		// active part of the swapbuffer; that way we only have to check the active actions for receives
		// from the server
		for ( LocalWorldAction& localAction : (**localActions.inactive) ) {
			Log(str(format("Adding local action to World Actions (%1%): %2%")%localAction.action.action%localAction.action.args));
			(*ResourceManager::world->actions.active)->push_back( localAction.action );
			Log(str(format("Adding local action to active local actions (%1%): %2%")%localAction.action.action%localAction.action.args));
			(*localActions.active)->push_back( localAction );
#ifndef PLAY_LOCALLY
			// TODO: set fast low-collision hashed id from (uid,reqid,time)
			Log(str(format("Adding local action to net action buffer (%1%): %2%")%localAction.action.action%localAction.action.args));
			(*net::actionBuffer.active)->push_back( LocalAction( (++net::reqid), 0, 0, localAction.action.action, localAction.action.args, localAction.id ) );
#endif
		}
		(*localActions.inactive)->clear();


#ifndef PLAY_LOCALLY
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
			for ( LocalWorldAction localAction : (**localActions.active) ) {
				Log(str(format("checking received netAction (%1%) == %2% ?")%localAction.id%netAction.id));
				if ( localAction.id == netAction.id ) {
					Log(str(format("netAction (%1%) == (%2%)   removing action from buffer")%netAction.id%localAction.id));
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
#endif



		ResourceManager::world->step( 10 );

		check_gl_error();
		glutMainLoopEvent();
		usleep( 10 * 1000 );
	}


	// ==========================================
	// shutdown global systems

	glutDestroyWindow( nWindow );

#ifndef NO_INPUT
	Log( "shutting down input.." );
	Input::shutdown();
#endif

#ifndef PLAY_LOCALLY
	Log( "shutting down net.." );
	net::shutdown();
#endif


	Log( "shutting down res.." );
	ResourceManager::shutdown();

	Log( "shutting down logger.." );
	LogSystem::shutdown();

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

	glClearColor( 1.0f, 0.0f, 1.0f, 0.0f );
	glClearDepth( 1.0f );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	ResourceManager::world->render();
	ui->render();
	
	glutSwapBuffers();
	glutPostRedisplay();


	// =============================================
	// check for input stuff
	if ( glutGet( GLUT_ELAPSED_TIME ) - input_clock > 40 ) {
		input_clock = glutGet( GLUT_ELAPSED_TIME );
		// check individual keys pressed, respond to action
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
		case 115: // UP
			input_state |= INPUT_UP; break;
		case 117: // Journal Undo
			if ( ResourceManager::world->terrain->activeJournalEntry >= 0 ) {
				ResourceManager::world->terrain->undo( ResourceManager::world->terrain->journal->entries.at( ResourceManager::world->terrain->activeJournalEntry ) );
				--ResourceManager::world->terrain->activeJournalEntry;
			}
			break;
		case 114: // Journal Redo
			if ( ResourceManager::world->terrain->activeJournalEntry < ResourceManager::world->terrain->journal->entries.size() - 1 ||
				 ResourceManager::world->terrain->activeJournalEntry < 0 ) {
				ResourceManager::world->terrain->redo( ResourceManager::world->terrain->journal->entries.at( ResourceManager::world->terrain->activeJournalEntry+1 ) );
				++ResourceManager::world->terrain->activeJournalEntry;
			}
			break;
		case 85: // Journal Undo at Tri
			if ( ResourceManager::world->terrain->selection ) {
				for ( auto selection : ResourceManager::world->terrain->selection->selections ) {
					if ( selection->class_id == TerrainSelection::SelectionClass::CLASS_HIGHLIGHT ) {
						ushort triId = selection->refTri_id;
						JournalEntry* triEntry = 0;
						int triEntryId = 0;
						for ( int i = ResourceManager::world->terrain->activeJournalEntry; i >= 0; --i ) {
							for ( auto operation : ResourceManager::world->terrain->journal->entries.at(i)->operations ) {
								if ( ( operation->entry_type == JournalEntry::JournalEntryOp::JOURNAL_ENTRY_ADDTRI ||
									   operation->entry_type == JournalEntry::JournalEntryOp::JOURNAL_ENTRY_RESHAPETRI ) &&
									operation->id[0] == triId && operation->id[1] == selection->refChunk->id ) {
									triEntry = ResourceManager::world->terrain->journal->entries.at(i);
									triEntryId = i;
									break;
								}
							}
							if ( triEntry ) break;
						}

						// Undo to this point
						if ( triEntry ) {
							for ( int i = ResourceManager::world->terrain->activeJournalEntry; i >= triEntryId; --i ) {
								ResourceManager::world->terrain->undo( ResourceManager::world->terrain->journal->entries.at(i) );
							}
							ResourceManager::world->terrain->activeJournalEntry = triEntryId-1;
							Log(str(format("activeJournalEntry: %1%")%ResourceManager::world->terrain->activeJournalEntry));
						}
					}
				}
			}
			break;
		case 82: // Journal Redo till end
			for ( int i = ResourceManager::world->terrain->activeJournalEntry; i < ResourceManager::world->terrain->journal->entries.size() - 1 || i < 0; ++i ) {
				ResourceManager::world->terrain->redo( ResourceManager::world->terrain->journal->entries.at(i+1) );
			}
			ResourceManager::world->terrain->activeJournalEntry = ResourceManager::world->terrain->journal->entries.size() - 1;
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
			input_state &= ~INPUT_UP; break;
		case 111: // DROP
			{
				uint dropguid = ResourceManager::world->entities.size();
				// TODO: set proper id and time
				(*localActions.inactive)->push_back( LocalWorldAction( WorldAction::serialize_create_mesh( 1, dropguid, camera.position ), 8923, 1 ) ); break;
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

				// ==========================================
				// world picking

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

				if ( !selection ) {

					glm::vec3 rayPos = glm::vec3(-1*camera.position.x, camera.position.y, -1*camera.position.z);

					// ray direction
					glm::vec4 rayDir      = glm::vec4( xw, yw, 1.0f, 1.0f );
					glm::mat4 perspective = camera.perspective;
					glm::quat quatY       = glm::angleAxis( glm::degrees( camera.rotation.y ), glm::vec3( -1.0f, 0.0f, 0.0f ) );
					glm::quat quatX       = glm::angleAxis( glm::degrees( camera.rotation.x ), glm::vec3( 0.0f, 1.0f, 0.0f ) );
					glm::quat quat        = quatY * quatX;
					glm::mat4 view        = glm::toMat4(quat);

					glm::vec4 modelRayDir = glm::inverse(view) * glm::inverse(perspective) * rayDir * glm::vec4( 1.0f, -1.0f, -1.0f, 1.0f );
					// glm::vec4 modelRayDir = glm::inverse(view) * rayDir;
					modelRayDir.x /= modelRayDir.w;
					modelRayDir.y /= modelRayDir.w;
					modelRayDir.z /= modelRayDir.w;
					modelRayDir.w /= modelRayDir.w;
					modelRayDir = glm::normalize(modelRayDir);

					glm::vec3 modelRayDir3 = glm::vec3(modelRayDir);
					Log(str(format("Trying to select.. (%1%,%2%,%3%) + t<%4%,%5%,%6%>")%rayPos.x%rayPos.y%rayPos.z%modelRayDir3.x%modelRayDir3.y%modelRayDir3.z));
					Tri* triHit = ResourceManager::world->terrain->terrainPick( rayPos, modelRayDir3 );
					if ( triHit ) {
						/* Selection:
						 * 	- Tri (tri, edges, verts)
						 * 	- Neighbours (tris, edges, verts)
						 * 	- Indirect Neighbours (sharing same verts)
						 *
						 *	* The above should only place those items into buckets
						 *	* Each bucket has a class; that class will be rendered a certain way
						 *	* Rewrite existing tri to point to another existing tri; then restore when unselected
						 */
						Log(str(format("Hit a Tri: {%1%,%2%,%3%}") % triHit->p0 % triHit->p1 % triHit->p2));
						ResourceManager::world->terrain->selectTri( triHit );
					}
				}

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

