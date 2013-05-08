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
#include <unistd.h>

#include "net.h"
#include "protocol.h"
#include "protocol_utils.h"
#include "protocol_server.h"
#include "protocol_session.h"
#include "maze.h"

#define PROTO_SERVER_MAX_EVENT_SUBSCRIBERS 400

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
    maze_t maze;
    cell_t * updatelist[4000];
    int listsize;
    int eventid;
    int sendcounter;
} Proto_Server;

void proto_server_send_all_state(FDType fd);

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

extern int
proto_server_set_session_lost_handler(Proto_MT_Handler h) {
    Proto_Server.session_lost_handler = h;
    return 1;
}

extern int
proto_server_set_req_handler(Proto_Msg_Types mt, Proto_MT_Handler h) {
    int i;

    if (mt > PROTO_MT_REQ_BASE_RESERVED_FIRST &&
            mt < PROTO_MT_REQ_BASE_RESERVED_LAST) {
        i = mt - PROTO_MT_REQ_BASE_RESERVED_FIRST - 1;
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
        rc = 1;
    } else {
        int i;
        for (i = 0; i < PROTO_SERVER_MAX_EVENT_SUBSCRIBERS; i++) {
            if (Proto_Server.EventSubscribers[i] == -1) {
                Proto_Server.EventSubscribers[i] = fd;
                Proto_Server.EventNumSubscribers++;
                Proto_Server.EventLastSubscriber = i + 1;
                *num = i;
                rc = 1;
            }
        }
    }

    pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);

    return rc;
}

static int
proto_server_remove_event_subscriber(int fd) {
    int rc = -1;

    pthread_mutex_lock(&Proto_Server.EventSubscribersLock);

    int i;
    for (i = 0; i < PROTO_SERVER_MAX_EVENT_SUBSCRIBERS; i++) {
        if (Proto_Server.EventSubscribers[i] == fd) {
            Proto_Server.EventSubscribers[i] = -1;
            Proto_Server.EventNumSubscribers--;
            rc = 1;
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
        connfd = net_accept(fd);
        if (connfd < 0) {
            fprintf(stderr, "Error: EventListen accept failed (%d)\n", errno);
        } else {
            int i;
            fprintf(stderr, "EventListen: connfd=%d -> ", connfd);

            if (proto_server_record_event_subscriber(connfd, &i) < 0) {
                fprintf(stderr, "oops no space for any more event subscribers\n");
                close(connfd);
            } else {
                fprintf(stderr, "subscriber num %d\n", i);
            }
        }
        proto_server_send_all_state(connfd);
    }
}

void
proto_server_post_event(void) {
    int i;
    int num;
    printf("proto_server_post_event\n");
    //pthread_mutex_lock(&Proto_Server.EventSubscribersLock);
    //organize data into eventsession
    Proto_Msg_Hdr *h = malloc(sizeof (Proto_Msg_Hdr));
    h->type = PROTO_MT_EVENT_BASE_UPDATE;
    proto_session_hdr_marshall(&(Proto_Server.EventSession), h);
    proto_session_body_marshall_int(&(Proto_Server.EventSession), Proto_Server.sendcounter);
    if (wrap_update(&(Proto_Server.maze), &(Proto_Server.EventSession)) < 0) {
        fprintf(stderr, "wrap maze error\n");
    }
    i = 0;
    num = Proto_Server.EventNumSubscribers;
    while (num) {
        Proto_Server.EventSession.fd = Proto_Server.EventSubscribers[i];
        if (Proto_Server.EventSession.fd != -1) {
            num--;
            if (proto_session_send_msg(&Proto_Server.EventSession, 0) < 0) {
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
    //pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);
}

extern void
proto_server_post_map(void) {
    int i;
    int num;
    printf("proto_server_post_map\n");
    //pthread_mutex_lock(&Proto_Server.EventSubscribersLock);
    //organize data into eventsession
    Proto_Msg_Hdr *h = malloc(sizeof (Proto_Msg_Hdr));
    h->type = PROTO_MT_EVENT_BASE_GETMAP;
    proto_session_hdr_marshall(&(Proto_Server.EventSession), h);
    proto_session_body_marshall_int(&(Proto_Server.EventSession), Proto_Server.sendcounter);
    proto_session_body_marshall_int(&(Proto_Server.EventSession), Proto_Server.listsize);
    int j;
    for (j = 0; j < Proto_Server.listsize; j++) {
        if (wrap_cell(Proto_Server.updatelist[j], &(Proto_Server.EventSession)) < 0) {
            fprintf(stderr, "wrap maze error\n");
        }
    }
    i = 0;
    num = Proto_Server.EventNumSubscribers;
    while (num) {
        Proto_Server.EventSession.fd = Proto_Server.EventSubscribers[i];
        if (Proto_Server.EventSession.fd != -1) {
            num--;
            if (proto_session_send_msg(&Proto_Server.EventSession, 0) < 0) {
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
    Proto_Server.listsize = 0;
    //pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);
}

extern void
proto_server_post() {
    pthread_mutex_lock(&Proto_Server.EventSubscribersLock);
    proto_server_post_event();
    proto_server_post_map();
    Proto_Server.sendcounter++;
    pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);
}

static void *
proto_server_req_dispatcher(void * arg) {
    Proto_Session s;
    Proto_Msg_Types mt;
    Proto_MT_Handler hdlr;
    //int i;
    unsigned long arg_value = (unsigned long) arg;

    pthread_detach(pthread_self());

    //lock turn here

    proto_session_init(&s);

    s.fd = (FDType) arg_value;

    fprintf(stderr, "proto_rpc_dispatcher: %p: Started: fd=%d\n",
            pthread_self(), s.fd);

    for (;;) {
        //flush the buffer
        bzero(&(s.rbuf), sizeof (int) *2);
        s.slen = 0;
        //read msg to s
        if (proto_session_rcv_msg(&s) == 1) {
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
        connfd = net_accept(fd);
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
    //proto_session_dump(s);

    pthread_mutex_lock(&Proto_Server.EventSubscribersLock);
    // setup dummy reply header : set correct reply message type and 
    // everything else empty
    bzero(&h, sizeof (s));
    h.type = PROTO_MT_REP_BASE_HELLO;
    proto_session_hdr_marshall(s, &h);
    printf("proto_server_hello_handler: eventNumSubscribers %d\n", Proto_Server.EventNumSubscribers);
    /* 
     if (Proto_Server.EventNumSubscribers == 1) {
         //assign X
         proto_session_body_marshall_int(s, X);
     } else if (Proto_Server.EventNumSubscribers == 2) {
         //assign O
         proto_session_body_marshall_int(s, O);
     } else {
         //full
         proto_session_body_marshall_int(s, -1);
     }*/
    player_t* ppt = maze_add_new_player(&Proto_Server.maze);
    if (ppt == NULL) {
        proto_session_body_marshall_int(s, -1);
    } else {
        //proto_session_body_marshall_int(s, ppt->team);
        proto_session_body_marshall_int(s, ppt->id);
    }
    pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);
    rc = proto_session_send_msg(s, 1);
    //transfer whole game state

    return rc;
}

static int
proto_server_mt_query_handler(Proto_Session *s) {
    pthread_mutex_lock(&Proto_Server.EventSubscribersLock);

    int rc = 1;
    Proto_Msg_Hdr h;

    fprintf(stderr, "proto_server_mt_query_handler: invoked for session:\n");
    //proto_session_dump(s);
    //read the move informtaion from s
    //1. query type
    //2-3 arguments

    Query_Types qtype;
    int arg1;
    int arg2;

    printf("query handler: body unmarshall qtype\n");
    proto_session_body_unmarshall_int(s, 0, (int*) &qtype);
    printf("query handler: body unmarshall arg1\n");
    proto_session_body_unmarshall_int(s, sizeof (int), &arg1);
    printf("query handler: body unmarshall arg2\n");
    proto_session_body_unmarshall_int(s, 2 * sizeof (int), &arg2);


    int reply1 = 0;
    int reply2 = 0;
    int reply3 = 0;
    cell_t* cell;
    switch (qtype) {
        case NUM_HOME:
            reply1 = maze_get_num_home_cells(&Proto_Server.maze, arg1);
            printf("num_home %c : %d\n", arg1, reply1);
            break;
        case NUM_JAIL:
            reply1 = maze_get_num_jail_cells(&Proto_Server.maze, arg1);
            printf("num_jail %c : %d\n", arg1, reply1);
            break;
        case NUM_WALL:
            reply1 = maze_get_num_wall_cells(&Proto_Server.maze);
            printf("num_wall : %d\n", reply1);
            break;
        case NUM_FLOOR:
            reply1 = maze_get_num_floor_cells(&Proto_Server.maze);
            printf("num_floor : %d\n", reply1);
            break;
        case DIM:
            reply1 = maze_get_num_cols(&Proto_Server.maze);
            reply2 = maze_get_num_rows(&Proto_Server.maze);
            printf("dim_c : %d\tdim_r : %d\n", reply1, reply2);
            break;
        case CINFO:
            cell = maze_get_cell(&Proto_Server.maze, arg2 /*row*/, arg1 /*column*/);
            reply1 = maze_get_cell_type(cell);
            reply2 = maze_get_cell_team(cell);
            reply3 = maze_get_cell_occupied(cell);
            printf("cinfo %d %d :\n\ttype:%d\n\tteam:%d\n\toccupied:%d\n", arg1, arg2, reply1, reply2, reply3);
            break;
        case DUMP:
            reply1 = 1;
            maze_dump(&Proto_Server.maze);
            break;
        case MOVE:
            //types
            printf("MOVE!!! arg1: %d\n", arg1);
            switch (arg1) {
                case MOVE_UP:
                    reply1 = maze_move_player(&Proto_Server.maze, arg2, arg1, Proto_Server.updatelist, &(Proto_Server.listsize));
                    //printf("UP!!! reply: %d\n", reply1);
                    break;
                case MOVE_DOWN:
                    reply1 = maze_move_player(&Proto_Server.maze, arg2, arg1, Proto_Server.updatelist, &(Proto_Server.listsize));
                    break;
                case MOVE_LEFT:
                    reply1 = maze_move_player(&Proto_Server.maze, arg2, arg1, Proto_Server.updatelist, &(Proto_Server.listsize));
                    break;
                case MOVE_RIGHT:
                    reply1 = maze_move_player(&Proto_Server.maze, arg2, arg1, Proto_Server.updatelist, &(Proto_Server.listsize));
                    break;
                case DROP_FLAG:
                    reply1 = maze_drop_flag(&Proto_Server.maze, arg2);
                    break;
                case DROP_HAM:
                    reply1 = maze_drop_jackhammer(&Proto_Server.maze, arg2);
                    break;
                default:
                    reply1 = -2;
                    break;
            }
            break;
        default:
            reply1 = -1;
            printf("query unknown");
            break;
    }


    printf("query handler: bzero header\n");
    bzero(&h, sizeof (s));
    printf("query handler: bzero sbuf\n");
    bzero(&(s->sbuf), sizeof (int) *2);
    s->slen = 0;
    h.type = PROTO_MT_REP_BASE_QUERY;

    printf("query handler: marshalling header\n");
    proto_session_hdr_marshall(s, &h);

    printf("query handler: marshalling body\n");
    proto_session_body_marshall_int(s, reply1);
    proto_session_body_marshall_int(s, reply2);
    proto_session_body_marshall_int(s, reply3);

    printf("query handler: sending message back\n");
    rc = proto_session_send_msg(s, 1);
    pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);

    return rc;
}

static int
proto_server_mt_move_handler(Proto_Session * s) {
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
    move = move - 1;

    // setup dummy reply header : set correct reply message type and 
    // everything else empty
    printf("move handler: bzero header\n");
    bzero(&h, sizeof (s));
    printf("move handler: bzero sbuf\n");
    bzero(&(s->sbuf), sizeof (int) *2);
    s->slen = 0;
    h.type = PROTO_MT_REP_BASE_MOVE;

    printf("move handler: marshalling header\n");
    proto_session_hdr_marshall(s, &h);
    printf("move handler: sending message back\n");
    rc = proto_session_send_msg(s, 1);

    return rc;
}

void*
proto_server_post_disconnect_event(int clientType) {

    Proto_Session *event_session = &(Proto_Server.EventSession);
    Proto_Msg_Hdr hdr = event_session->shdr;
    hdr.type = PROTO_MT_EVENT_BASE_DISCONNECT;

    proto_session_hdr_marshall(event_session, &hdr);
    proto_session_body_marshall_int(event_session, clientType);

    proto_server_post_event();
    return 0;
}

static int
proto_server_mt_goodbye_handler(Proto_Session * s) {
    int rc = 1;
    Proto_Msg_Hdr h;

    int clientType;
    proto_session_body_unmarshall_int(s, 0, &clientType);
    printf("goodbye handler: client player id %d\n", clientType);
    bzero(&h, sizeof (Proto_Msg_Hdr));
    bzero(&(s->sbuf), sizeof (int) *2);
    s->slen = 0;

    h.type = PROTO_MT_REP_BASE_GOODBYE;
    proto_session_hdr_marshall(s, &h);

    proto_session_body_marshall_int(s, 1);

    rc = proto_session_send_msg(s, 1);

    if (rc) {
        maze_remove_player(&(Proto_Server.maze), clientType);
        proto_server_post_disconnect_event(clientType);
        close(s->fd);
    }
    return rc;
}

extern int
proto_server_load_maze() {
    char* path = malloc(100);
    memcpy(path, "daGame.map", 10);
    maze_load(path, &Proto_Server.maze);
    maze_dump(&Proto_Server.maze);
    return 1;
}

extern int
proto_server_init(void) {
    int i;
    int rc;

    //load the maze
    proto_server_load_maze();

    for (i = 0; i < 4000; i++) {
        Proto_Server.updatelist[i] = malloc(sizeof (cell_t));
    }
    Proto_Server.listsize = 0;
    Proto_Server.sendcounter = 0;

    /* printf("rows:%d\tcols:%d\n",maze_get_num_rows(&Proto_Server.maze),maze_get_num_cols(&Proto_Server.maze));
     printf("home1:%d\thome2:%d\tjail1:%d\tjail2:%d\twall:%d\tfloor:%d\n",   maze_get_num_home_cells(&Proto_Server.maze,T1),
                                                                             maze_get_num_home_cells(&Proto_Server.maze,T2),
                                                                             maze_get_num_jail_cells(&Proto_Server.maze,T1),
                                                                             maze_get_num_jail_cells(&Proto_Server.maze,T2),
                                                                             maze_get_num_wall_cells(&Proto_Server.maze),
                                                                             maze_get_num_floor_cells(&Proto_Server.maze));
     cell_t* test_cell = maze_get_cell(&Proto_Server.maze,100,100);
     printf("cell type:%c\t",maze_cell_to_char(maze_get_cell_type(test_cell)));
    
                                                             maze_get_cell_team(&Proto_Server.maze, test_cell), 
                                                             maze_get_cell_occupied(test_cell));
     */
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
    proto_server_set_req_handler(PROTO_MT_REQ_BASE_HELLO, proto_server_mt_hello_handler);
    //set query handler
    proto_server_set_req_handler(PROTO_MT_REQ_BASE_QUERY, proto_server_mt_query_handler);
    //set goodbye handler
    proto_server_set_req_handler(PROTO_MT_REQ_BASE_GOODBYE, proto_server_mt_goodbye_handler);

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

    Proto_Server.eventid = 0;

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

extern int
proto_server_testcases(void) {

    maze_t* m = &Proto_Server.maze;

    fprintf(stdout, "\n/***** GAME STATE TEST CASES *****/\n");

    fprintf(stdout, "\nadd 4 players\n\n");

    player_t* t1p1 = maze_add_new_player(m);
    player_t* t2p1 = maze_add_new_player(m);
    player_t* t1p2 = maze_add_new_player(m);
    player_t* t2p2 = maze_add_new_player(m);

    cell_t* t1p1_cell = maze_get_cell(m, t1p1->pos.r, t1p1->pos.c);
    cell_t* t2p1_cell = maze_get_cell(m, t2p1->pos.r, t2p1->pos.c);
    cell_t* t1p2_cell = maze_get_cell(m, t1p2->pos.r, t1p2->pos.c);
    cell_t* t2p2_cell = maze_get_cell(m, t2p2->pos.r, t2p2->pos.c);

    maze_print_player(m, t1p1);
    maze_print_player(m, t2p1);
    maze_print_player(m, t1p2);
    maze_print_player(m, t2p2);

    maze_print_cell(m, t1p1_cell);
    maze_print_cell(m, t2p1_cell);
    maze_print_cell(m, t1p2_cell);
    maze_print_cell(m, t2p2_cell);

    fprintf(stdout, "\nremove 2 players; attempt to remove nonexistant player\n\n");


    int t1p1_id = t1p1->id;
    int t2p2_id = t2p2->id;
    maze_remove_player(m, t1p1_id);
    maze_remove_player(m, t2p2_id);
    if (maze_remove_player(m, 5) <= 0) {
        fprintf(stdout, "player 5 does not exist\n");
    }

    maze_print_player(m, maze_get_player(m, t1p1_id));
    maze_print_player(m, t2p1);
    maze_print_player(m, t1p2);
    maze_print_player(m, maze_get_player(m, t2p2_id));

    maze_print_cell(m, t1p1_cell);
    maze_print_cell(m, t2p1_cell);
    maze_print_cell(m, t1p2_cell);
    maze_print_cell(m, t2p2_cell);

    fprintf(stdout, "\nadd flag and jackhammers to players\n\n");

    cell_t* flag_cell = maze_get_cell(m, m->t1_flag->pos.r, m->t1_flag->pos.c);
    cell_t* jack_cell = maze_get_cell(m, m->t1_jack->pos.r, m->t1_jack->pos.c);

    maze_pick_up_flag(m, flag_cell, t2p1);
    maze_pick_up_jackhammer(m, jack_cell, t1p2);

    maze_print_item(maze_get_player_flag(m, t2p1->id));
    maze_print_item(maze_get_player_jackhammer(m, t1p2->id));
    maze_print_player(m, t2p1);
    maze_print_player(m, t1p2);
    maze_print_cell(m, t2p1_cell);
    maze_print_cell(m, t1p2_cell);

    fprintf(stdout, "\nmove the players\n\n");

    maze_move_player(&Proto_Server.maze, t2p1->id, MOVE_DOWN, Proto_Server.updatelist, &(Proto_Server.listsize));
    maze_move_player(&Proto_Server.maze, t1p2->id, MOVE_RIGHT, Proto_Server.updatelist, &(Proto_Server.listsize));
    t2p1_cell = maze_get_cell(m, t2p1->pos.r, t2p1->pos.c);
    t1p2_cell = maze_get_cell(m, t1p2->pos.r, t1p2->pos.c);

    maze_print_item(maze_get_player_flag(m, t2p1->id));
    maze_print_item(maze_get_player_jackhammer(m, t1p2->id));
    maze_print_player(m, t2p1);
    maze_print_player(m, t1p2);
    maze_print_cell(m, t2p1_cell);
    maze_print_cell(m, t1p2_cell);

    fprintf(stdout, "\ntake away flags and jackhammers from players\n\n");

    maze_drop_flag(m, t2p1->id);
    maze_drop_jackhammer(m, t1p2->id);

    maze_print_item(m->t1_flag);
    maze_print_item(m->t1_jack);
    maze_print_player(m, t2p1);
    maze_print_player(m, t1p2);
    maze_print_cell(m, t2p1_cell);
    maze_print_cell(m, t1p2_cell);


    return 1;
}

void
proto_server_send_all_state(FDType fd) {
    printf("send all state to fd=%d PROTO_MT_EVENT_BASE_GETMAP %d\n", fd, PROTO_MT_EVENT_BASE_GETMAP);
    pthread_mutex_lock(&Proto_Server.EventSubscribersLock);
    int i;
    for (i = 0; i < 200; i += 10) {
        //organize data into eventsession
        Proto_Msg_Hdr *h = malloc(sizeof (Proto_Msg_Hdr));
        h->type = PROTO_MT_EVENT_BASE_GETMAP;
        proto_session_hdr_marshall(&(Proto_Server.EventSession), h);
        proto_session_body_marshall_int(&(Proto_Server.EventSession), Proto_Server.sendcounter);
        proto_session_body_marshall_int(&(Proto_Server.EventSession), 2000);
        int j;
        for (j = 0; j < 10; j++) {
            if (wrap_maze(&(Proto_Server.maze), &(Proto_Server.EventSession), i + j) < 0) {
                fprintf(stderr, "wrap maze error\n");
            }
        }
        Proto_Server.EventSession.fd = fd;
        if (Proto_Server.EventSession.fd != -1) {
            if (proto_session_send_msg(&Proto_Server.EventSession, 0) < 0) {
                // must have lost an event connection
                close(Proto_Server.EventSession.fd);
                fprintf(stderr, "session lost during sending all states\n");
                Proto_Server.session_lost_handler(&Proto_Server.EventSession);
            }
            // FIXME: add ack message here to ensure that game is updated 
            // correctly everywhere... at the risk of making server dependent
            // on client behaviour  (use time out to limit impact... drop
            // clients that misbehave but be carefull of introducing deadlocks
        }
        proto_session_reset_send(&Proto_Server.EventSession);
    }

    //organize data into eventsession
    Proto_Msg_Hdr *h = malloc(sizeof (Proto_Msg_Hdr));
    h->type = PROTO_MT_EVENT_BASE_UPDATE;
    proto_session_hdr_marshall(&(Proto_Server.EventSession), h);
    proto_session_body_marshall_int(&(Proto_Server.EventSession), Proto_Server.sendcounter);
    if (wrap_update(&(Proto_Server.maze), &(Proto_Server.EventSession)) < 0) {
        fprintf(stderr, "wrap maze error\n");
    }
    Proto_Server.EventSession.fd = fd;
    if (Proto_Server.EventSession.fd != -1) {
        if (proto_session_send_msg(&Proto_Server.EventSession, 0) < 0) {
            // must have lost an event connection
            close(Proto_Server.EventSession.fd);
            fprintf(stderr, "session lost during sending all states\n");
            Proto_Server.session_lost_handler(&Proto_Server.EventSession);
        }
        // FIXME: add ack message here to ensure that game is updated 
        // correctly everywhere... at the risk of making server dependent
        // on client behaviour  (use time out to limit impact... drop
        // clients that misbehave but be carefull of introducing deadlocks
    }
    proto_session_reset_send(&Proto_Server.EventSession);

    pthread_mutex_unlock(&Proto_Server.EventSubscribersLock);
}
