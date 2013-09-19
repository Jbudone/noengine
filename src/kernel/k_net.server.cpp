#include "kernel/k_net.server.h"


namespace net {
	BufferList bufferQueue;
	BufferList sendQueue;
	AcknowledgeList acknowledgeQueue;
	vector<Client> clients;

	struct sockaddr_in my_addr;
	struct sockaddr_in cli_addr;
	int sockfd;
	int i;
	socklen_t slen;
	char buf[BUFLEN];
	bool threadPause;
	thread listener;
	int skipAck = 0;
	int clientCount = 0;
	int sendDelay = 0;
};


void net::startup() {
	bufferQueue = BufferList();
	sendQueue = BufferList();
	acknowledgeQueue = AcknowledgeList();

	if (( sockfd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP )) == -1 ) {
		// error?
		Log( "Error: socket" );
		return;
	} else {
		// socket received?
		Log( "Socket successful" );
	}


	// bzero( &my_addr, sizeof( my_addr ) );
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons( PORT );
	my_addr.sin_addr.s_addr = htonl( INADDR_ANY );

	slen = sizeof( cli_addr );

	if ( bind( sockfd, (struct sockaddr*) &my_addr, sizeof( my_addr ) ) == -1 ) {
		Log ( "Error: bind" );
	} else {
		Log( "Bind successful" );
	}

	Log( "starting listener.." );
	listener = thread(net::receive);
}

void net::shutdown() {

	listener.detach();

	Log("closing sockfd");
	close( sockfd );
}

void net::receive() {
	Log( "receiving.." );
	while ( true ) {
		if ( recvfrom( sockfd, buf, BUFLEN, 0, (struct sockaddr*) &cli_addr, &slen ) == -1 ) {
			Log( "Error: recvfrom" );
		} else {
			Log( "Received message.." );

			// fetch message
			uchar msgType;
			int reqId;
			uint id;
			uint msg;
			Bfield128_t args;
			Bitfield<BUFLEN> message = buf;
			Log(str(format("Received: %1%")%message.getChar()));
			unsigned char offset = 0;
			msgType = message.fetch<uchar>(offset); offset += sizeof(uchar);
			reqId   = message.fetch<int>(offset);   offset += sizeof(int);
			id      = message.fetch<uint>(offset);  offset += sizeof(uint);
			msg     = message.fetch<uint>(offset);  offset += sizeof(uint);
			args    = Bfield128_t((const char*)message.fields+offset);
			Log(str(format(" Args (%1%): %2%")%id%args.getChar()));
			/*
			msgType = buf[0];
			reqId = parseInt(buf+1,10,false);
			id = parseInt(buf+11,10,false);
			msg = parseInt(buf+21,10,false);
			args = Bfield128_t(buf+31);
			*/
			// Log(str(format("   msg(%1%) [ %2% ]")%msgType%buf));
			// Log(str(format("   message (%1%): %2%")%reqId%msg));

			if ( msgType == 3 && msg == 0 ) {
				// CONNECT user
				Log("User requesting connection..");

				// does this client already exist ??
				Client* thisClient = 0;
				for( Client& client : clients ) {
					if ( client.client.sin_addr.s_addr == cli_addr.sin_addr.s_addr && client.client.sin_port == cli_addr.sin_port ) {
						thisClient = &client;
						break;
					}
				}

				if ( thisClient==0 ) {
					Log( "Adding client" );
					clients.push_back( Client(++clientCount, cli_addr, -1, 0, -1) );
					thisClient = &clients.at(clients.size()-1);
				} else {
					Log( "User already exists!!" );
				}


				// give user uid
				(*acknowledgeQueue.active)->push_back( Acknowledge( thisClient->client, reqId, thisClient->playerId ) );
				thisClient->reqId = reqId;
				continue;
			}

		
			// who is this client?
			Client* thisClient = 0;
			for( Client& client : clients ) {
				if ( client.client.sin_addr.s_addr == cli_addr.sin_addr.s_addr && client.client.sin_port == cli_addr.sin_port ) {
					thisClient = &client;
					break;
				}
			}
			if ( thisClient==0 ) {
				Log( "WHO IS THIS CLIENT!? /ignored" );
				continue;
			}

			thisClient->delay = 0;
			if ( msgType == 1 ) {
				// user ping
				continue;
			}

			// check if this was received out of order (already received? missed previous messages?)
			if ( thisClient->reqId >= reqId ) {
				// already received this request.. resend ack
				Log( "   ..message already received, re-acknowledging it" );
				(*acknowledgeQueue.active)->push_back( Acknowledge( thisClient->client, reqId ) );
				continue;
			} else if ( thisClient->reqId+1 != reqId ) {
				// received this message out of order.. drop message
				Log( "   ..message received out of order, skipping it" );
				continue;
			}

			// add to buffer for adding to world, stepping through world, modifying, and adding results to send buffer
			// push to buffer
			// args = Bfield128_t(buf+31);
			(*bufferQueue.active)->push_back(
					Action(-1, thisClient->playerId, msg, args, id) );


			// randomly skip acknowledgement for testing
#ifdef SIMULATE_LAG
			thisClient->reqId = reqId;
			if ( ++skipAck % 3 == 0 ) {
				Log( "   SKIPPING Acknowledgement!!" );
			} else {
#endif
				(*acknowledgeQueue.active)->push_back( Acknowledge( thisClient->client, reqId, thisClient->playerId ) );
				Log( "   Acknowledged.." );
#ifdef SIMULATE_LAG
			}
#endif
		}
	}
}

void net::send() {
	sendQueue.swap();

	if ( !(*sendQueue.inactive)->empty() )
		Log( "Sending messages.." );
	else if ( sendDelay >= 3000 ) {
		// no messages in a while.. ping everybody to remind them
		// that we're still up and running
		sendDelay = 0;

		string message;
		message = str(format("%0160i")%(0));
		if ( !clients.empty() ) Log("Pinging all clients..");
		for ( auto client : clients ) {
			if ( sendto( sockfd, message.c_str(), BUFLEN, 0, (struct sockaddr*) &client.client, slen ) == -1 ) {
				Log( "Error: pinging client.." );
			}
		}
	}

	// relay actions
	for ( auto action : (**sendQueue.inactive) ) {
		Bitfield<BUFLEN> message;
		unsigned char offset = 0;
		offset = message.append<char>((char)3,offset);
		offset = message.append<int>(action.initiatorId,offset);
		offset = message.append<uint>(action.id,offset);
		offset = message.append<uint>(action.action,offset);
		offset = message.append<128>(&action.args,offset);
		/*
		string message;
		message = str(format("%c%010i%010i%010i%-128s")%((char)3)%(action.initiatorId)%(action.id)%(action.action)%(action.args));
		*/
		for ( auto client : clients ) {
			/*
			if ( client.client.sin_port == action.initiatorId ) {
				string localMessage;
				localMessage = str(format("%032i%032i%032i%32s%032i")%(7)%(action.initiatorId)%(action.entityId)%(action.action)%(action.args));
				Log(str(format( "Sending message to initiator (%1%)!!!")%(localMessage)));
				if ( sendto( sockfd, localMessage.c_str(), BUFLEN, 0, (struct sockaddr*) &client.client, slen ) == -1 ) {
					Log( "Error: sending message to initiator" );
				}
			} else {
			*/
				Log(str(format( "Relaying message (%1%)!!!")%action.id));
				if ( sendto( sockfd, message, BUFLEN, 0, (struct sockaddr*) &client.client, slen ) == -1 ) {
					Log( "Error: relaying message" );
				}
			/*
			}
			*/
		}
	}

	// send acknowledgements
	acknowledgeQueue.swap();
	if ( !(*acknowledgeQueue.inactive)->empty() ) Log( "Sending acknowledgements.." );
	for ( auto acknowledge : (**acknowledgeQueue.inactive) ) {
		Bitfield<BUFLEN> message;
		unsigned char offset = 0;
		offset = message.append<char>(0,offset);
		offset = message.append<int>(acknowledge.reqId,offset);
		offset = message.append<int>(acknowledge.args,offset);
		// string message;
		// message = str(format("%c%010i%010i%0139i")%((char)0)%(acknowledge.reqId)%(acknowledge.args)%(0));
		Log(str(format("resending acknowledgement: %1%")%message));
		if ( sendto( sockfd, message, BUFLEN, 0, (struct sockaddr*) &(acknowledge.client), slen ) == -1 ) {
				Log( "Error: sending acknowledgement" );
		}
	}
	
	(*sendQueue.inactive)->clear();
	(*acknowledgeQueue.inactive)->clear();
}

void net::step(int delay) {
	sendDelay += delay;

	int removeClient = -1; // NOTE: we cannot remove a client while 
							//	reading them in the iter. loop; so
							//	simply remove them in the next
							//	iteration
	int clientCount = 0;
	for ( Client& client : clients ) {

		// remove previous client
		if ( removeClient != -1 ) {
			Log(str(format("   Client (%1%) was removed..")%(clients.at(removeClient).playerId)));
			clients.erase(clients.begin()+removeClient);
			removeClient = -1;
			clientCount -= 1;
		}

		// haven't heard from user for this long
		// NOTE: this isn't COMPLETELY accurate since we could have heard
		// 		from the user just before this; however that means we could
		// 		be off by only a maximum of (delay) amount
		if ( (client.delay += delay) >= 30000 ) {
			// Disconnect this user
			Log(str(format("DISCONNECTING USER (%1%) haven't heard from him %2% ms")%client.playerId%client.delay));
			removeClient = clientCount;

		}
		clientCount++;
	}

	// remove previous client
	if ( removeClient != -1 ) {
		Log(str(format("   Client (%1%) was removed..")%(clients.at(removeClient).playerId)));
		clients.erase(clients.begin()+removeClient);
		removeClient = -1;
		clientCount -= 1;
	}

}
