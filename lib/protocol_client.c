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

#include "protocol.h"
#include "protocol_utils.h"
#include "protocol_client.h"

typedef struct {
    char board[9];
    int type;
    Proto_Session rpc_session;
    Proto_Session event_session;
    pthread_t EventHandlerTid;
    Proto_MT_Handler session_lost_handler;
    Proto_MT_Handler base_event_handlers[PROTO_MT_EVENT_BASE_RESERVED_LAST
            - PROTO_MT_EVENT_BASE_RESERVED_FIRST
            - 1];
} Proto_Client;

extern Proto_Session *
proto_client_rpc_session(Proto_Client_Handle ch) {
    Proto_Client *c = ch;
    return &(c->rpc_session);
}

extern Proto_Session *
proto_client_event_session(Proto_Client_Handle ch) {
    Proto_Client *c = ch;
    return &(c->event_session);
}

extern int
proto_client_set_session_lost_handler(Proto_Client_Handle ch, Proto_MT_Handler h) {
    Proto_Client *c = ch;
    c->session_lost_handler = h;
}

extern int
proto_client_set_event_handler(Proto_Client_Handle ch, Proto_Msg_Types mt,
        Proto_MT_Handler h) {
    int i;
    Proto_Client *c = ch;

    if (mt > PROTO_MT_EVENT_BASE_RESERVED_FIRST &&
            mt < PROTO_MT_EVENT_BASE_RESERVED_LAST) {
        i = mt - PROTO_MT_EVENT_BASE_RESERVED_FIRST - 1;
        //ADD CODE
        //set base event handler to client
        c->base_event_handlers[i] = h;
        return 1;
    } else {
        return -1;
    }
}

static int
proto_client_session_lost_default_hdlr(Proto_Session *s) {
    fprintf(stderr, "Session lost...:\n");
    proto_session_dump(s);
    return -1;
}

int
getBoardFromSession(Proto_Session *s){
    int winCode;
    int board[9];
    //proto_session_body_ptr(s, 0,&board);
    //int offs = proto_session_body_unmarshall_bytes(s,0,9,board);
    
    //proto_session_body_unmarshall_int(s,offs,&winCode);
    long i;
    for(i=0;i<9;i++){
      proto_session_body_unmarshall_int(s,i*sizeof(int),&(board[i]));
    }
    proto_session_body_unmarshall_int(s,9*sizeof(int),&winCode);
    
    char token;
    printf("\n", token); 
    for (i = 0; i < 8; i++) {
      if(board[i] == 0){
	token = 'O';
      }else if(board[i] == 1){
	token = 'X';
      }else{
	token = (char)(((int)'0')+i+1);
      }
      if ((i+1) % 3 == 0) 
	printf("%c\n-----\n", token); 
      else 
	printf("%c|", token);
  }
  if(board[8] == 0){
      token = 'O';
    }else if(board[i] == 1){
      token = 'X';
    }else{
      token = (char)(((int)'0')+i+1);
    }
  printf("%c\n-----\n", token);
    switch (winCode){
        case 2: //draw
            fprintf(stdout,"Game Over: Draw\n");
            return -1;
        case 1: //X wins
            fprintf(stdout,"Game Over: X Wins\n");
            return -1;

        case 0: //O wins
            fprintf(stdout,"Game Over: O Wins\n");
            return -1;
        default: //game not over yet
	  //	  fprintf(stdout,"%c");
            return 1;
    }
}

int
getBoardFromSessionHelper(Proto_Client_Handle ch){
  Proto_Client *c = ch;
  Proto_Session *s = &(c->event_session);
  return getBoardFromSession(s);
}

static int
proto_client_event_null_handler (Proto_Session *s) {
  fprintf(stderr,
         "proto_client_event null_handler: invoked for session:\n");

    return 1;
}


static int
proto_client_event_update_handler (Proto_Session *s) {
  //fprintf(stderr,
  //       "proto_client_event null_handler: invoked for session:\n");
    getBoardFromSession(s);

    return 1;
}

static int
proto_client_event_disconnect_handler (Proto_Session *s) {
  //fprintf(stderr
  //       "proto_client_event null_handler: invoked for session:\n");
    int disconnectCode;
    proto_session_body_unmarshall_int(s,0,&disconnectCode);
    close(s->fd);
    //printf("disconnect handler: code %d\n",disconnectCode);

    if (disconnectCode == 1){

        fprintf(stdout,"Game Over: X quit\n");
    }else if (disconnectCode == 0){
        fprintf(stdout, "Game Over: O quit\n");
    }

    return 1;
}

static void *
proto_client_event_dispatcher(void * arg) {
    Proto_Client *c;
    Proto_Session *s;
    Proto_Msg_Types mt;
    Proto_MT_Handler hdlr;
    int i;

    pthread_detach(pthread_self());

    //  c = ADD CODE
    //s = ADD CODE
    c = arg;
    s = &(c->event_session);

    for (;;) {
        if (proto_session_rcv_msg(s) == 1) {
            mt = proto_session_hdr_unmarshall_type(s);
            if (mt > PROTO_MT_EVENT_BASE_RESERVED_FIRST &&
                    mt < PROTO_MT_EVENT_BASE_RESERVED_LAST) {
                //ADD CODE
                int i = mt - PROTO_MT_EVENT_BASE_RESERVED_FIRST - 1;
                hdlr = c->base_event_handlers[i];
                if (hdlr(s) < 0) goto leave;
            }
        } else {
            //ADD CODE
            //incomplete
            //printf("proto_client_event_dispatcher: proto_session_rcv_msg(s)!=1");
            
            c->session_lost_handler(s);
                    goto leave;
        }
    }
leave:
    close(s->fd);
    return NULL;
}

extern int
proto_client_init(Proto_Client_Handle *ch) {
    Proto_Msg_Types mt;
    Proto_Client *c;

    c = (Proto_Client *) malloc(sizeof (Proto_Client));
    if (c == NULL) return -1;
    bzero(c, sizeof (Proto_Client));
    memset (c->event_session.rbuf, -1, sizeof(int)*10);
    proto_client_set_session_lost_handler(c,
            proto_client_session_lost_default_hdlr);

    for (mt = PROTO_MT_EVENT_BASE_RESERVED_FIRST + 1;
            mt < PROTO_MT_EVENT_BASE_RESERVED_LAST; mt++)
        //base event handler init
        proto_client_set_event_handler(c, mt, proto_client_event_null_handler);
    
    proto_client_set_event_handler(c,PROTO_MT_EVENT_BASE_UPDATE,proto_client_event_update_handler);
    proto_client_set_event_handler(c,PROTO_MT_EVENT_BASE_DISCONNECT,proto_client_event_disconnect_handler);
    
    *ch = c;
    return 1;
}

int
proto_client_connect(Proto_Client_Handle ch, char *host, PortType port) {
    Proto_Client *c = (Proto_Client *) ch;

    if (net_setup_connection(&(c->rpc_session.fd), host, port) < 0)
        return -1;

    if (net_setup_connection(&(c->event_session.fd), host, port + 1) < 0)
        return -2;

    if (pthread_create(&(c->EventHandlerTid), NULL,
            &proto_client_event_dispatcher, c) != 0) {
        fprintf(stderr,
                "proto_client_init: create EventHandler thread failed\n");
        perror("proto_client_init:");
        return -3;
    }
    

    return 0;
}

static void
marshall_mtonly(Proto_Session *s, Proto_Msg_Types mt) {
    Proto_Msg_Hdr h;

    bzero(&h, sizeof (h));
    h.type = mt;
    proto_session_hdr_marshall(s, &h);
};

// all rpc's are assume to only reply only with a return code in the body
// eg.  like the null_mes

static int
do_generic_dummy_rpc(Proto_Client_Handle ch, Proto_Msg_Types mt) {
    int rc;
    Proto_Session *s;
    Proto_Client *c = ch;

    s=&(c->rpc_session);
    // marshall

    marshall_mtonly(s, mt);
    rc = proto_session_rpc(s);

    if (rc == 1){
        proto_session_body_unmarshall_int(s, 0, &rc);
    }else{
        close(s->fd);
    }

    return rc;
}

extern int
proto_client_hello(Proto_Client_Handle ch) {
    return do_generic_dummy_rpc(ch, PROTO_MT_REQ_BASE_HELLO);
}

extern int
proto_client_move(Proto_Client_Handle ch, int tp, char move) {
    
    int rc;
    Proto_Session *s;
    Proto_Client *c = ch;

    s=&(c->rpc_session);
    // marshall
    
    marshall_mtonly(s, PROTO_MT_REQ_BASE_MOVE);
    
    proto_session_body_marshall_int(s,tp);
	proto_session_body_marshall_int(s,move);
    rc = proto_session_rpc(s);

    if (rc == 1){
        proto_session_body_unmarshall_int(s, 0, &rc);
    }else{
        close(s->fd);
    }

    return rc;
}

extern int
proto_client_goodbye(Proto_Client_Handle ch,int tp) {
    int rc;
    Proto_Session *s;
    Proto_Client *c = ch;
    //printf("client goodbye: tp %d\n",tp);
    s=&(c->rpc_session);
    // marshall

    marshall_mtonly(s, PROTO_MT_REQ_BASE_GOODBYE);
    //s->slen=0;
    proto_session_body_marshall_int(s,tp);

    rc = proto_session_rpc(s);

    proto_session_body_unmarshall_int(s, 0, &rc);
    if (rc){
        close(s->fd);
    }

    return rc;
}

