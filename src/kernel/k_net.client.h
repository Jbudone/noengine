#ifndef __K_CLIENTNET_H__
#define __K_CLIENTNET_H__


#include "util.inc.h"

#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <thread>

#define BUFLEN 160
#define PORT 9930

/*
 * Client Network
 *
 * TODO
 *
 *  > actions, local actions, acknowledgement, timeout resend
 *  > connect to server
 *  > thread with program, link with actions, local actions
 *  > ping, disconnection
 *  > connection initialization
 *  > swap world
 *  > local action buffer:
 *  	unsuccessful responses from server (revert back to previous stage)
 *		syncing modified responses from server
 *
 ***/


/*
=================================================

	Client Network

	Handles the client networking protocol

=================================================
*/



struct Action {
	Action(uint action, const Bfield128_t& args, int page, int initiator, uint id) :
		action(action),page(page), initiator(initiator), id(id) {
			this->args.copy(args);
		}
	uint action; Bfield128_t args; int page; int initiator; uint id;
};
struct LocalAction { 
	LocalAction(int reqId, int delay, int retries, uint action, const Bfield128_t& args, uint id) :
		reqId(reqId), delay(delay), retries(retries), action(action), id(id) {
			this->args.copy(args);
		}
	int reqId; int delay; int retries; uint action; Bfield128_t args; uint id; };
typedef SwapBuffer<vector<Action>> ActionQueue; // actions received from server
typedef SwapBuffer<vector<LocalAction>> ActionBuffer; // local actions sent to server

// extern const uint ACTION_NET_CONNECT = 0;
namespace net {

	extern bool running;
	extern int uid; // our unique ID (set from server)
	extern int reqid; // request id
	extern int delay; // delay from server
	extern int sendDelay; // delay from us (from POV of the server)
	extern ActionQueue actionQueue;
	extern ActionBuffer actionBuffer;

	extern struct sockaddr_in serv_addr;
	extern int sockfd, i;
	extern socklen_t slen;
	extern char buf[BUFLEN];
	extern char* server;
	extern thread listener;

	void startup();
	void shutdown();

	void receive();
	void send(const LocalAction&);
	bool step(int delay);
};

#endif
