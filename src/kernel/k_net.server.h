#ifndef __K_SERVNET_H__
#define __K_SERVNET_H__


#include "util.inc.h"


#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <thread>
#include <curses.h>
#include <cstring>

#define BUFLEN 160
#define PORT 9930


/*
 * Server Network
 *
 * TODO
 *
 *  > step, worldstep, swap buffer, relay
 *  > connect user
 *  > thread with program, handle actions, create actions (server cmd)
 *  > ping, disconnection
 *  > connection initialization
 *  > multiple worlds; client/server world; swap user world (server cmd)
 *  > user flooding
 *  > paged sends
 *
 ***/


/*
=================================================

	Server Network

	Handles the server networking protocol

=================================================
*/

#define SIMULATE_LAG

struct Client {
	Client(int playerId, struct sockaddr_in client, int floodCount, int delay, int reqId) : playerId(playerId), client(client), floodCount(floodCount), delay(delay), reqId(reqId) { }
	int playerId; struct sockaddr_in client; int floodCount; int delay; int reqId;
};
struct Action {
	Action(int page, int initiatorId, uint action, const Bfield128_t& args, uint id) : page(page), initiatorId(initiatorId), action(action), id(id) {
		this->args.copy(args);
	}
	int page; int initiatorId; uint action; Bfield128_t args; uint id;
};
struct Acknowledge {
	Acknowledge(struct sockaddr_in client, int reqId, int args=0) : client(client), reqId(reqId), args(args) { }
	struct sockaddr_in client; int reqId; int args;
};

typedef SwapBuffer<vector<Action>> BufferList;
typedef SwapBuffer<vector<Acknowledge>> AcknowledgeList;

// extern const uint ACTION_NET_CONNECT = 0;
namespace net {

	extern BufferList bufferQueue; // received from client; needs to be sent to world action buffer
	extern BufferList sendQueue; // actions confirmed by World, ready to send to clients
	extern AcknowledgeList acknowledgeQueue;
	extern vector<Client> clients;


	extern struct sockaddr_in my_addr, cli_addr;
	extern int sockfd, i;
	extern socklen_t slen;
	extern char buf[BUFLEN];
	extern bool threadPause;
	extern thread listener;
	extern int skipAck;
	extern int clientCount;
	extern int sendDelay; // delay in sending anything

	void startup();
	void shutdown();

	void receive();
	void send();
	void step(int delay);
};


#endif
