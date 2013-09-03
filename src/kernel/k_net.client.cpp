#include "kernel/k_net.client.h"


namespace net {
	bool running = false;
	int uid = -1;
	int reqid = -1;
	int delay = 0;
	int sendDelay = 0;
	ActionQueue actionQueue;
	ActionBuffer actionBuffer;

	struct sockaddr_in serv_addr;
	int sockfd;
	int i;
	socklen_t slen;
	char buf[BUFLEN];
	char* server = "192.168.0.100";
	thread listener;
};

void net::startup() {

	if ( ( sockfd = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) == -1 ) {
		Log( "Error: socket" );
	}

	// bzero( &serv_addr, sizeof( serv_addr ) );
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons( PORT );
	if ( inet_aton( server, &serv_addr.sin_addr ) == 0 ) {
		Log( "Error: inet_aton" );
	}

	slen = sizeof( serv_addr );

	running = true;
	listener = thread(net::receive);


	(*actionBuffer.active)->push_back( LocalAction( (++reqid), 0, 0, 0, 0, 0 ) );
}

void net::shutdown() {
	listener.detach();
	running = false;
	close(sockfd);
}

void net::receive() {
	try {
	while ( running ) {
		if ( recvfrom( sockfd, buf, BUFLEN, 0, (struct sockaddr*) &serv_addr, &slen ) == -1 ) {
			Log( "Error: recvfrom" );
		}
		if ( slen > 0 ) { 
			Log( str( format( "Received packet from %1%: %2% \n Data: %3% \n\n" ) % inet_ntoa( serv_addr.sin_addr ) % ntohs( serv_addr.sin_port ) % buf ) );
		}

		delay = 0;

		// format: (messageType)[args..]
		Bitfield<BUFLEN> message = buf;
		unsigned char offset = 0;
		uchar msgType;
		// char msgBuf[158];
		// msgType = buf[0];
		msgType = message.fetch<uchar>(offset); offset += sizeof(uchar);
		// strcpy(msgBuf,buf+1);
		if ( msgType == 3 ) {
			// RELAY message
			// format: (initiator)(id)(action)(args)
			int initiator;
			uint action;
			uint id;
			// char args[128];
			Bfield128_t args;
			initiator = message.fetch<int>(offset); offset += sizeof(int);
			id        = message.fetch<uint>(offset); offset += sizeof(uint);
			action    = message.fetch<uint>(offset); offset += sizeof(uint);
			args      = Bfield128_t((const char*)message.fields+offset);
			/*
			initiator = parseInt(msgBuf,10,false);
			id = parseInt(msgBuf+10,10,false);
			action = parseInt(msgBuf+20,10,false);
			strcpy(args, msgBuf+30);
			*/

			// Log(str(format( "   msgBuf[ %1% ]" )%msgBuf));
			Log(str(format( "   Message (%1%):%2%:%3%: %4%" )%initiator%id%action%args));

			(*actionQueue.active)->push_back( Action( action, args, -1, initiator, id ) );
		} else {
			// Acknowledgement
			// format: (reqId)
			int reqId, args;
			// reqId = parseInt(msgBuf,10,false);
			reqId = message.fetch<int>(offset); offset += sizeof(int);
			args  = message.fetch<int>(offset); offset += sizeof(int);

			// connection ack?
			if ( reqId == 0 && uid == -1 ) {
				// uid = parseInt(msgBuf+10,10,false);
				uid = args;
			}

			// acknowledge message
			int end = 0;
			for ( auto action : (**actionBuffer.active) ) {
				if ( action.reqId <= reqId ) {
					// Log(str(format( "   msgBuf[ %1% ]" )%msgBuf));
					Log(str(format("Acknowledged message (%1%): %2%")%action.reqId%action.action));
					end++;
				} else {
					break;
				}
			}
			(**actionBuffer.active).erase( (**actionBuffer.active).begin(), (**actionBuffer.active).begin()+end );
		}

	}
	} catch(exception& e) {
		Log("Caught exception in net listener");
	}
}

void net::send(const LocalAction& action) {
	Bitfield<BUFLEN> args;
	unsigned char offset = 0;
	offset = args.append<char>((char)3,offset);
	offset = args.append<int>(action.reqId,offset);
	offset = args.append<uint>(action.id,offset);
	offset = args.append<uint>(action.action,offset);
	offset = args.append<128>(&action.args,offset);
	/*
	string message;
	const char* args = action.args;
	message = str(format("%c%010i%010i%010i%-128s")%((char)3)%(action.reqId)%(action.id)%(action.action)%(args));
	*/
	Log(str(format("sending: %1%")%args.getChar()));
	Bfield128_t thoseArgs = action.args;
	Log(str(format(" Args: %1%")%thoseArgs.getChar()));
	if ( sendto( sockfd, args, BUFLEN, 0, (struct sockaddr*) &serv_addr, slen ) == -1 ) {
		Log( "Error: sendto" );
	}
	sendDelay = 0;
}

bool net::step(int delay) {

	// have we timed out from the server?
	if ( (net::delay+=delay) >= 10000 ) {
		Log("Disconnected from server! Timed out..");
		return false;
	}

	// resend unacknowledged messages
	if ( !(*actionBuffer.active)->empty() ) {
		for ( LocalAction& action : (**actionBuffer.active) ) {
			if ( (action.delay+=delay) > 1500 || (action.retries==0) ) {
				// resend this message
				Log(str(format( "Resending unacknowledged timed-out message.. (%1%) |%2%|: %3%" ) % action.reqId % action.delay % action.action ));
				action.delay = 0;
				action.retries++;
				send( action );
			}
		}
	} else {
		// are we too quiet? send ping to server to remind them
		// of our presence
		if ( (sendDelay+=delay) >= 2000 ) {
			Bitfield<BUFLEN> message;
			unsigned char offset = 0;
			offset = message.append<char>((char)1,offset);
			// string message;
			// message = str(format("%c%0160i")%((char)1)%(0));
			Log("Pinging server..");
			if ( sendto( sockfd, message, BUFLEN, 0, (struct sockaddr*) &serv_addr, slen ) == -1 ) {
				Log( "Error: sendto" );
			}
			sendDelay = 0;
		}
	}
	return true;
}

