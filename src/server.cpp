
/***
 * Server
 *
 *	    Main file to run for the server
 *
 * TODO
 *  - click ESC to exit
 *  - high precision clock for loop
 *
 **/

#include "config.h"
#include "util.inc.h"

#include "extern/GL/glew.h"
#include "extern/GL/freeglut.h"
#include "extern/GL/glm/gtc/random.hpp"         // value_ptr

#include "libutils/lib_resmgr.h"
#include "kernel/k_net.server.h"



bool requestClose = false;
void clickToQuit() {
	int s;
	char kb[100];
	while (true) {
		scanf( "%[^\n]", kb );
		getchar();
		if ( strcmp( kb, "exit" ) == 0 ) {
			Log("Requesting close!");
			requestClose = true;
		}

		/*
		s = getchar();
		if ( s == -1 ) {
			usleep( 10 * 1000 );
			continue;
		}
		if ( s == 27 ) {
			Log("Requesting close!");
			requestClose = true;
		}
		Log(str(format("Clicked: %1%")%s));
		s = -1;
		*/
	}
}



int main(int argc, char **argv) {


	// ==========================================
	// start global systems
	
	LogSystem::startup( CFG_LOGFILE_SERVER );
	Log( str( format( "Starting %1%" ) % argv[0] ) );
	Log( "This is the SERVER" );


	Log ( "starting net.." );
	net::startup();

	Log( "starting world.." );
	ResourceManager::LoadWorld(false);

	thread t1(clickToQuit);


	/*
	 * Main Loop
	 *
	 *	Clients will create in-world events (eg. walking around, pushing
	 *	objects, starting particle effects, adding lights, breaking objects,
	 *	etc. anything that can be done to the world is considered an event) and
	 *	those events are packed and sent to the server as an Action
	 *
	 *	Actions received through the network are pushed to the world's action
	 *	queue, and later applied. When an action is applied it may fail (eg. a
	 *	user trying to move through a wall, push an object through a colliding
	 *	area, etc.) which results in a modified action that may best fit the
	 *	requested action (user slides along the wall instead of through it). For
	 *	actions which can't be modified for the original action, it simply 
	 *	denies the action. These results are stored in an action response and
	 *	are relayed to clients
	 *
	 ***/
	Log( "Ready." );
	WorldActionResponses responses; 
	while( !requestClose ) {

		net::step( 100 );


		// load received actions from buffer queue
		net::bufferQueue.swap();
		for ( Action& action : (**net::bufferQueue.inactive) ) {
			(*ResourceManager::world->actions.active)->push_back( WorldAction( action.action, action.args, action.page, action.initiatorId, action.id ) );
		}
		(*net::bufferQueue.inactive)->clear();


		ResourceManager::world->step( 100, &responses );


		// send resulting actions to net actions (to send)
		for ( WorldActionResponse* response : responses ) {
			(*net::sendQueue.active)->push_back( Action( response->worldAction->page, response->worldAction->initiator, response->worldAction->action, response->worldAction->args, response->worldAction->id ) );
		}
		responses.clear();

		// TODO: step (TODO implement w/ worlds)
		net::send(); // TODO: thread

		usleep( 100 * 1000 );
	}


	t1.detach();

	Log( "shutting down Network" );
	net::shutdown();

	Log( "shutting down ResourceManager" );
	ResourceManager::shutdown();

	Log("shutting down logger");
	LogSystem::shutdown();

	return -1;
}

