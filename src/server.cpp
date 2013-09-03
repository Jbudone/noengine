
#include <stdio.h>

#include "config.h"

#include "extern/GL/glew.h"
#include "extern/GL/freeglut.h"
#include "extern/GL/glm/gtc/random.hpp"         // value_ptr

#include <boost/format.hpp>
#include <boost/tokenizer.hpp>

#include "libutils/lib_logger.h"
#include "libutils/lib_resmgr.h"
#include "kernel/k_net.server.h"


using boost::format;
using boost::str;
using boost::tokenizer;

using namespace Logger;


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
	
	LogSystem::startup( CFG_LOGFILE );

	Log( str( format( "Starting %1%" ) % argv[0] ) );
	Log( "This is the SERVER" );


	Log ( "starting net.." );
	net::startup();
	Log( "started net.." );

	Log( "starting world.." );
	ResourceManager::LoadWorld(false);

	thread t1(clickToQuit);
	Log( "Ready." );
	WorldActionResponses responses;
	while( !requestClose ) {
		usleep( 100 * 1000 );

		net::step( 100 );

		// load received actions from buffer queue
		net::bufferQueue.swap();
		for ( Action& action : (**net::bufferQueue.inactive) ) {
			(*ResourceManager::world->actions.active)->push_back( WorldAction( action.action, action.args, action.page, action.initiatorId, action.id ) );
		}
		(*net::bufferQueue.inactive)->clear();

		// apply 
		ResourceManager::world->step( 100, &responses );

		// send resulting actions to net actions (to send)
		for ( WorldActionResponse* response : responses ) {
			(*net::sendQueue.active)->push_back( Action( response->worldAction->page, response->worldAction->initiator, response->worldAction->action, response->worldAction->args, response->worldAction->id ) );
		}
		responses.clear();

		// TODO: step (TODO implement w/ worlds)
		net::send(); // TODO: thread
	}
	t1.detach();

	net::shutdown();

	Log( "shutting down ResourceManager" );
	ResourceManager::shutdown();

	Log("shutting down logger");
	LogSystem::shutdown();

	return -1;
}

