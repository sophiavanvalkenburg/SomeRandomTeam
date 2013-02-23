/******************************************************************************
 * Copyright (C) 2011 by Jonathan Appavoo, Boston University
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <strings.h>
#include <errno.h>
#include <pthread.h>
#include <memory.h>

#include "net.h"
#include "protocol.h"
#include "protocol_utils.h"
#include "protocol_server.h"
#include "protocol_session.h"

#define PROTO_SERVER_MAX_EVENT_SUBSCRIBERS 1024
#define X 1
#define O 0

struct {
    FDType RPCListenFD;
    PortType RPCPort;


    FDType EventListenFD;
    PortType EventPort;
    pthread_t EventListenTid;
    pthread_mutex_t EventSubscribersLock;
    int EventLastSubscriber;
    int EventNumSubscribers;
    FDType EventSubscribers[PROTO_SERVER_MAX_EVENT_SUBSCRIBERS];
    Proto_Session EventSession;
    pthread_t RPCListenTid;
    Proto_MT_Handler session_lost_handler;
    Proto_MT_Handler base_req_handlers[PROTO_MT_REQ_BASE_RESERVED_LAST -
            PROTO_MT_REQ_BASE_RESERVED_FIRST - 1];
    int turn; //0 represents O, 1 represents X
    int cb[9]; //chess board
} Proto_Server;

extern PortType proto_server_rpcport(void) {
    return Proto_Server.RPCPort;
}

extern PortType proto_server_eventport(void) {
    return Proto_Server.EventPort;
}

extern Proto_Session *
proto_server_event_session(void) {
    return &Proto_Server.EventSession;
}


static void* printCB(){
	long k;
	for(k=0;k<9;k++){
		printf("cb: %d\n",Proto_Server.cb[k]);
	}
}

static int getWinner(){
	return -1;
}

extern int
proto_server_set_session_lost_handler(Proto_MT_Handler h) {
    Proto_Server.session_lost_handler = h;
}

extern int
proto_server_set_req_handler(Proto_Msg_Types mt, Proto_MT_Handler h) {
    int i;

    if (mt > PROTO_MT_REQ_BASE_RESERVED_FIRST &&
            mt < PROTO_MT_REQ_BASE_RESERVED_LAST) {
        i = mt - PROTO_MT_REQ_BASE_RESERVED_FIRST - 1;
        // (complete) ADD CODE
        Proto_Server.base_req_handlers[i] = h;
        return 1;
    } else {
        return -1;
    }
}

static int
proto_server_record_event_subscriber(int fd, int *num) {
    int rc = -1;

    pthread_mutex_lock(&Proto_Server.EventSubscribersLock);

    if (Proto_Server.EventLastSubscriber < PROTO_SERVER_MAX_EVENT_SUBSCRIBERS
            && Proto_Server.EventSubscribers[Proto_Server.EventLastSubscriber]
            == -1) {
        Proto_Server.EventSubscribers[Proto_Server.EventLastSubscriber] = fd;
        *num = Proto_Server.EventLastSubscriber;
        Proto_Server.EventNumSubscribers++;
        Proto_Server.EventLastSubscriber++;
        // (complete)  ADD CODE
        rc = 1;
    } else {
        int i;
        for (i = 0; i < PROTO_SERVER_MAX_EVENT_SUBSCRIBERS; i++) {
            if (Proto_Server.EventSubscribers[i] == -1) {
                Proto_Server.EventSubscribers[i] = fd;
                Proto_Server.EventNumSubscribers++;
                Proto_Server.EventLastSubscriber = i + 1;
                // (complete)  ADD CODE
                *num = i;
                rc = 1;
            }
        }
    }

    pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);

    return rc;
}

static
void *
proto_server_event_listen(void *arg) {
    int fd = Proto_Server.EventListenFD;
    int connfd;

    if (net_listen(fd) < 0) {
        exit(-1);
    }

    for (;;) {
        connfd = net_accept(fd); // (complete) ADD CODE
        if (connfd < 0) {
            fprintf(stderr, "Error: EventListen accept failed (%d)\n", errno);
        } else {
            int i;
            fprintf(stderr, "EventListen: connfd=%d -> ", connfd);

            if (/* (complete ADD CODE (*/ proto_server_record_event_subscriber(connfd, &i) < 0) {
                fprintf(stderr, "oops no space for any more event subscribers\n");
                close(connfd);
            } else {
                fprintf(stderr, "subscriber num %d\n", i);
            }
        }
    }
}

void
proto_server_post_event(void) {
    int i;
    int num;

    pthread_mutex_lock(&Proto_Server.EventSubscribersLock);
	//organize data into eventsession
	long j;
	for(j=0;j<9;j++){
		proto_session_body_marshall_int(&Proto_Server.EventSession,Proto_Server.cb[j]);
	}
	proto_session_body_marshall_int(&Proto_Server.EventSession,getWinner());

    i = 0;
    num = Proto_Server.EventNumSubscribers;
    while (num) {
        Proto_Server.EventSession.fd = Proto_Server.EventSubscribers[i];
        if (Proto_Server.EventSession.fd != -1) {
            num--;
            if (/*(complete) ADD CODE*/proto_session_send_msg(&Proto_Server.EventSession, 0) < 0) {
                // must have lost an event connection
                close(Proto_Server.EventSession.fd);
                Proto_Server.EventSubscribers[i] = -1;
                Proto_Server.EventNumSubscribers--;
                Proto_Server.session_lost_handler(&Proto_Server.EventSession);
            }
            // FIXME: add ack message here to ensure that game is updated 
            // correctly everywhere... at the risk of making server dependent
            // on client behaviour  (use time out to limit impact... drop
            // clients that misbehave but be carefull of introducing deadlocks
        }
        i++;
    }
    proto_session_reset_send(&Proto_Server.EventSession);
    pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);
}

static void *
proto_server_req_dispatcher(void * arg) {
    Proto_Session s;
    Proto_Msg_Types mt;
    Proto_MT_Handler hdlr;
    int i;
    unsigned long arg_value = (unsigned long) arg;

    pthread_detach(pthread_self());

    //lock turn here

    proto_session_init(&s);

    s.fd = (FDType) arg_value;

    fprintf(stderr, "proto_rpc_dispatcher: %p: Started: fd=%d\n",
            pthread_self(), s.fd);

    for (;;) {
	//flush the buffer
	bzero(&(s.rbuf),sizeof(int)*2);
	s.slen=0;
        //read msg to s
        if (proto_session_rcv_msg(&s) == 1) {
            // (complete) ADD CODE
            mt = proto_session_hdr_unmarshall_type(&s);
            if (mt > PROTO_MT_REQ_BASE_RESERVED_FIRST &&
                    mt < PROTO_MT_REQ_BASE_RESERVED_LAST) {
                int i = mt - PROTO_MT_REQ_BASE_RESERVED_FIRST - 1;
                hdlr = Proto_Server.base_req_handlers[i];

                if (hdlr(&s) < 0) goto leave;
            }
        } else {
            goto leave;
        }
    }
leave:
    Proto_Server.session_lost_handler(&s);
    close(s.fd);

//unlock turn here
return NULL;
}

static
void *
proto_server_rpc_listen(void *arg) {
    int fd = Proto_Server.RPCListenFD;
    unsigned long connfd;
    pthread_t tid;

    if (net_listen(fd) < 0) {
        fprintf(stderr, "Error: proto_server_rpc_listen listen failed (%d)\n", errno);
        exit(-1);
    }

    for (;;) {
        connfd = net_accept(fd); // (complete) ADD CODE
        if (connfd < 0) {
            fprintf(stderr, "Error: proto_server_rpc_listen accept failed (%d)\n", errno);
        } else {
            pthread_create(&tid, NULL, &proto_server_req_dispatcher,
                    (void *) connfd);
        }
    }
}

extern int
proto_server_start_rpc_loop(void) {
    if (pthread_create(&(Proto_Server.RPCListenTid), NULL,
            &proto_server_rpc_listen, NULL) != 0) {
        fprintf(stderr,
                "proto_server_rpc_listen: pthread_create: create RPCListen thread failed\n");
        perror("pthread_create:");
        return -3;
    }
    return 1;
}

static int
proto_session_lost_default_handler(Proto_Session *s) {
    fprintf(stderr, "Session lost [server side]...:\n");
    proto_session_dump(s);
    return -1;
}

static int
proto_server_mt_null_handler(Proto_Session *s) {
    int rc = 1;
    Proto_Msg_Hdr h;

    fprintf(stderr, "proto_server_mt_null_handler: invoked for session:\n");
    proto_session_dump(s);

    // setup dummy reply header : set correct reply message type and 
    // everything else empty
    bzero(&h, sizeof (s));
    h.type = proto_session_hdr_unmarshall_type(s);
    h.type += PROTO_MT_REP_BASE_RESERVED_FIRST;
    proto_session_hdr_marshall(s, &h);

    // setup a dummy body that just has a return code 
    proto_session_body_marshall_int(s, 0xdeadbeef);

    rc = proto_session_send_msg(s, 1);

    return rc;
}

static int
proto_server_mt_hello_handler(Proto_Session *s) {
    int rc = 1;
    Proto_Msg_Hdr h;

    fprintf(stderr, "proto_server_mt_hello_handler: invoked for session:\n");
    proto_session_dump(s);
    //assign either X or O to this
    pthread_mutex_lock(&Proto_Server.EventSubscribersLock);
    // setup dummy reply header : set correct reply message type and 
    // everything else empty
    bzero(&h, sizeof (s));
    h.type = PROTO_MT_REP_BASE_HELLO;
    proto_session_hdr_marshall(s, &h);
	printf("proto_server_hello_handler: eventNumSubscribers %d\n",Proto_Server.EventNumSubscribers);
    if (Proto_Server.EventNumSubscribers == 1) {
        //assign X
        proto_session_body_marshall_int(s, X);
    } else if (Proto_Server.EventNumSubscribers == 2) {
        //assign O
        proto_session_body_marshall_int(s, O);
    } else {
        //full
        proto_session_body_marshall_int(s, -1);
    }
    pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);
    rc = proto_session_send_msg(s, 1);
    return rc;
}

static int
proto_server_mt_move_handler(Proto_Session *s) {
    int rc = 1;
    Proto_Msg_Hdr h;

    fprintf(stderr, "proto_server_mt_move_handler: invoked for session:\n");
    proto_session_dump(s);
    //read the move informtaion from s
    //1. who is this
    //2. which move it performs
    int who;
    int move;
	printf("move handler: body unmarshall int\n");
    proto_session_body_unmarshall_int(s, 0, &who);
	printf("move handler: body unmarshall move\n");
    proto_session_body_unmarshall_int(s, sizeof (int), &move);
	move=move-1;

    // setup dummy reply header : set correct reply message type and 
    // everything else empty
    printf("move handler: bzero header\n");
    bzero(&h, sizeof (s));
	printf("move handler: bzero sbuf\n");
    bzero(&(s->sbuf), sizeof(int)*2);
	s->slen=0;
    h.type = PROTO_MT_REP_BASE_MOVE; 
	printf("move handler: who is %d, turn is %d\n",who,Proto_Server.turn);
    if (who == Proto_Server.turn) {
        int valid;
        //verify
        if(move<0 || move >8){
		valid=0;
	}else if(Proto_Server.cb[move]==-1){
		valid=1;
	}else{
		valid=0;
	}
        if (valid) {
		printf("move handler: valid\n");
            //make the move
            Proto_Server.cb[move]=who;
		printCB();
            //switch turn
            	printf("move handler: switching turn\n");
            if(Proto_Server.turn==0){
		Proto_Server.turn=1;
		}else{
			Proto_Server.turn=0;
		}
		printf("move handler: start sending message back\n");
            //send success message back
            proto_session_body_marshall_int(s,1);
        } else {
            //send error message: not a valid move
            proto_session_body_marshall_int(s,0);
        }
    } else {
        //send error message: not your turn
        proto_session_body_marshall_int(s,-1);
    }
	printf("move handler: marshalling header\n");
    proto_session_hdr_marshall(s, &h);
	printf("move handler: sending message back\n");
    rc = proto_session_send_msg(s, 1);


    Proto_Session event_session = Proto_Server.EventSession;
    Proto_Msg_Hdr hdr = event_session.shdr;
    hdr.type = PROTO_MT_EVENT_BASE_UPDATE;
    proto_session_hdr_marshall(&event_session, &hdr);
    proto_server_post_event();  
 
    return rc;
}



extern int
proto_server_init(void) {
    int i;
    int rc;

    //initialize turn to X (1)
    Proto_Server.turn = X;
    //initialize chess board to empty (-1)
    memset (Proto_Server.cb, -1, sizeof(Proto_Server.cb));

    proto_session_init(&Proto_Server.EventSession);

    proto_server_set_session_lost_handler(
            proto_session_lost_default_handler);

    //set handler to message types
    for (i = PROTO_MT_REQ_BASE_RESERVED_FIRST + 1;
            i < PROTO_MT_REQ_BASE_RESERVED_LAST; i++) {
        int ind = i - PROTO_MT_REQ_BASE_RESERVED_FIRST - 1;
        Proto_Server.base_req_handlers[ind] = proto_server_mt_null_handler;
        // (complete) ADD CODE
    }
	//set hello handler
    int ind = PROTO_MT_REQ_BASE_HELLO - PROTO_MT_REQ_BASE_RESERVED_FIRST - 1;
    Proto_Server.base_req_handlers[ind] = proto_server_mt_hello_handler;

	//set move handler
	ind = PROTO_MT_REQ_BASE_MOVE - PROTO_MT_REQ_BASE_RESERVED_FIRST - 1;
	Proto_Server.base_req_handlers[ind] = proto_server_mt_move_handler;

    for (i = 0; i < PROTO_SERVER_MAX_EVENT_SUBSCRIBERS; i++) {
        Proto_Server.EventSubscribers[i] = -1;
    }
    Proto_Server.EventNumSubscribers = 0;
    Proto_Server.EventLastSubscriber = 0;
    pthread_mutex_init(&Proto_Server.EventSubscribersLock, 0);


    rc = net_setup_listen_socket(&(Proto_Server.RPCListenFD),
            &(Proto_Server.RPCPort));

    if (rc == 0) {
        fprintf(stderr, "prot_server_init: net_setup_listen_socket: FAILED for RPCPort\n");
        return -1;
    }

    Proto_Server.EventPort = Proto_Server.RPCPort + 1;

    rc = net_setup_listen_socket(&(Proto_Server.EventListenFD),
            &(Proto_Server.EventPort));

    if (rc == 0) {
        fprintf(stderr, "proto_server_init: net_setup_listen_socket: FAILED for EventPort=%d\n",
                Proto_Server.EventPort);
        return -2;
    }

    if (pthread_create(&(Proto_Server.EventListenTid), NULL,
            &proto_server_event_listen, NULL) != 0) {
        fprintf(stderr,
                "proto_server_init: pthread_create: create EventListen thread failed\n");
        perror("pthread_createt:");
        return -3;
    }

    return 0;
}

