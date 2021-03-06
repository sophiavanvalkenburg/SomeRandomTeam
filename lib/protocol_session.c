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

 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

#include "net.h"
#include "protocol.h"
#include "protocol_utils.h"
#include "protocol_session.h"
#include "maze.h"

extern long getTimestamp() {

}

extern void
proto_session_dump(Proto_Session *s) {
    fprintf(stderr, "Session s=%p:\n", s);
    fprintf(stderr, " fd=%d, extra=%p slen=%d, rlen=%d\n shdr:\n  ",
            s->fd, s->extra,
            s->slen, s->rlen);
    proto_dump_msghdr(&(s->shdr));
    fprintf(stderr, " rhdr:\n  ");
    proto_dump_msghdr(&(s->rhdr));
}

extern void
proto_session_init(Proto_Session *s) {
    if (s) bzero(s, sizeof (Proto_Session));
}

extern void
proto_session_reset_send(Proto_Session *s) {
    bzero(&s->shdr, sizeof (s->shdr));
    s->slen = 0;
}

extern void
proto_session_reset_receive(Proto_Session *s) {
    bzero(&s->rhdr, sizeof (s->rhdr));
    s->rlen = 0;
}

static void
proto_session_hdr_marshall_sver(Proto_Session *s, Proto_StateVersion v) {
    s->shdr.sver.raw = htonll(v.raw);
}

static void
proto_session_hdr_unmarshall_sver(Proto_Session *s, Proto_StateVersion *v) {
    v->raw = ntohll(s->rhdr.sver.raw);
}

static void
proto_session_hdr_marshall_pstate(Proto_Session *s, Proto_Player_State *ps) {
    s->shdr.pstate.v0.raw = htonl(ps->v0.raw);
    s->shdr.pstate.v1.raw = htonl(ps->v1.raw);
    s->shdr.pstate.v2.raw = htonl(ps->v2.raw);
    s->shdr.pstate.v3.raw = htonl(ps->v3.raw);
    // ADD CODE 

}

static void
proto_session_hdr_unmarshall_pstate(Proto_Session *s, Proto_Player_State *ps) {
    //  ADD CODE 
    ps->v0.raw = ntohl(s->rhdr.pstate.v0.raw);
    ps->v1.raw = ntohl(s->rhdr.pstate.v1.raw);
    ps->v2.raw = ntohl(s->rhdr.pstate.v2.raw);
    ps->v3.raw = ntohl(s->rhdr.pstate.v3.raw);


}

static void
proto_session_hdr_marshall_gstate(Proto_Session *s, Proto_Game_State *gs) {
    //  ADD CODE
    s->shdr.gstate.v0.raw = htonl(gs->v0.raw);
    s->shdr.gstate.v1.raw = htonl(gs->v1.raw);
    s->shdr.gstate.v2.raw = htonl(gs->v2.raw);
}

static void
proto_session_hdr_unmarshall_gstate(Proto_Session *s, Proto_Game_State *gs) {
    // ADD CODE 
    gs->v0.raw = ntohl(s->rhdr.gstate.v0.raw);
    gs->v1.raw = ntohl(s->rhdr.gstate.v1.raw);
    gs->v2.raw = ntohl(s->rhdr.gstate.v2.raw);
}

static int
proto_session_hdr_unmarshall_blen(Proto_Session *s) {
    //  ADD CODE 
    return ntohl(s->rhdr.blen);
}

static void
proto_session_hdr_marshall_type(Proto_Session *s, Proto_Msg_Types t) {
    // ADD CODE 
    //printf("hdr marhsall type: %d\n",htonl(t));
    s->shdr.type = htonl(t);
    // have to make sure if htonl is appropriate here for the enum type Proto_Msg_Types.   
}

static int
proto_session_hdr_unmarshall_version(Proto_Session *s) {
    // ADD CODE
    return ntohl(s->rhdr.version);
}

extern Proto_Msg_Types
proto_session_hdr_unmarshall_type(Proto_Session *s) {
    //  ADD CODE 
    return ntohl(s->rhdr.type);
    // have to confirm whether ntohl is appropriate here for the enum type Proto_Msg_Types. 

}

extern void
proto_session_hdr_unmarshall(Proto_Session *s, Proto_Msg_Hdr *h) {

    h->version = proto_session_hdr_unmarshall_version(s);
    h->type = proto_session_hdr_unmarshall_type(s);
    proto_session_hdr_unmarshall_sver(s, &h->sver);
    proto_session_hdr_unmarshall_pstate(s, &h->pstate);
    proto_session_hdr_unmarshall_gstate(s, &h->gstate);
    h->blen = proto_session_hdr_unmarshall_blen(s);
}

extern void
proto_session_hdr_marshall(Proto_Session *s, Proto_Msg_Hdr *h) {
    // ignore the version number and hard code to the version we support
    s->shdr.version = PROTOCOL_BASE_VERSION;
    proto_session_hdr_marshall_type(s, h->type);
    proto_session_hdr_marshall_sver(s, h->sver);
    proto_session_hdr_marshall_pstate(s, &h->pstate);
    proto_session_hdr_marshall_gstate(s, &h->gstate);
    // we ignore the body length as we will explicity set it
    // on the send path to the amount of body data that was
    // marshalled.
}

extern int
proto_session_body_marshall_ll(Proto_Session *s, long long v) {
    if (s && ((s->slen + sizeof (long long)) < PROTO_SESSION_BUF_SIZE)) {
        *((int *) (s->sbuf + s->slen)) = htonll(v);
        s->slen += sizeof (long long);
        return 1;
    }
    return -1;
}

extern int
proto_session_body_unmarshall_ll(Proto_Session *s, int offset, long long *v) {
    if (s && ((s->rlen - (offset + sizeof (long long))) >= 1)) {
        *v = *((long long *) (s->rbuf + offset));
        *v = htonl(*v);
        return offset + sizeof (long long);
    }
    return -1;
}

extern int
proto_session_body_marshall_int(Proto_Session *s, int v) {
    //	printf("body marshall int: add int %d to slen %d\n",v,s->slen);
    if (s && ((s->slen + sizeof (int)) < PROTO_SESSION_BUF_SIZE)) {
        *((int *) (s->sbuf + s->slen)) = htonl(v);
        s->slen += sizeof (int);
        return 1;
    }
    return -1;
}

extern int
proto_session_body_unmarshall_int(Proto_Session *s, int offset, int *v) {
    if (s && ((s->rlen - (offset + sizeof (int))) >= 0)) {
        *v = *((int *) (s->rbuf + offset));
        *v = htonl(*v);
        return offset + sizeof (int);
    }
    return -1;
}

extern int
proto_session_body_marshall_char(Proto_Session *s, char v) {
    if (s && ((s->slen + sizeof (char)) < PROTO_SESSION_BUF_SIZE)) {
        s->sbuf[s->slen] = v;
        s->slen += sizeof (char);
        return 1;
    }
    return -1;
}

extern int
proto_session_body_unmarshall_char(Proto_Session *s, int offset, char *v) {
    if (s && ((s->rlen - (offset + sizeof (char))) >= 0)) {
        *v = s->rbuf[offset];
        return offset + sizeof (char);
    }
    return -1;
}

extern int
proto_session_body_reserve_space(Proto_Session *s, int num, char **space) {
    if (s && ((s->slen + num) < PROTO_SESSION_BUF_SIZE)) {
        *space = &(s->sbuf[s->slen]);
        s->slen += num;
        return 1;
    }
    *space = NULL;
    return -1;
}

extern int
proto_session_body_ptr(Proto_Session *s, int offset, char **ptr) {
    if (s && ((s->rlen - offset) > 0)) {
        *ptr = &(s->rbuf[offset]);
        return 1;
    }
    return -1;
}

extern int
proto_session_body_marshall_bytes(Proto_Session *s, int len, char *data) {
    if (s && ((s->slen + len) < PROTO_SESSION_BUF_SIZE)) {
        memcpy(s->sbuf + s->slen, data, len);
        s->slen += len;
        return 1;
    }
    return -1;
}

extern int
proto_session_body_unmarshall_bytes(Proto_Session *s, int offset, int len,
        char *data) {
    if (s && ((s->rlen - (offset + len)) >= 0)) {
        memcpy(data, s->rbuf + offset, len);
        return offset + len;
    }
    return -1;
}

// rc < 0 on comm failures
// rc == 1 indicates comm success

extern int
proto_session_send_msg(Proto_Session *s, int reset) {
    //int n;
    s->shdr.blen = sizeof (s->sbuf);
    s->slen = sizeof (s->shdr);

    net_writen(s->fd, &(s->shdr), s->slen);
    net_writen(s->fd, &(s->sbuf), s->shdr.blen);


    // write request
    // ADD CODE

    if (proto_debug()) {
        fprintf(stderr, "%p: proto_session_send_msg: SENT:\n", pthread_self());
        proto_session_dump(s);
    }

    // communication was successfull 
    if (reset) proto_session_reset_send(s);

    return 1;
}

extern int
proto_session_rcv_msg(Proto_Session *s) {

    proto_session_reset_receive(s);

    // read reply
    // ADD CODE
    s->rhdr.blen = sizeof (s->sbuf);
    s->rlen = sizeof (s->rhdr);
    net_readn(s->fd, &(s->rhdr), s->rlen);
    net_readn(s->fd, &(s->rbuf), s->rhdr.blen);

    if (proto_debug()) {
        fprintf(stderr, "%p: proto_session_rcv_msg: RCVED:\n", pthread_self());
        proto_session_dump(s);
    }
    return 1;
}

extern int
proto_session_rpc(Proto_Session *s) {
    int rc;

    // ADD CODE
    int n1;
    int n2;
    n1 = proto_session_send_msg(s, 1);
    if (n1 != 1) {
        printf("prote_session_rpc error: sending messge");
        return -1;
    }
    n2 = proto_session_rcv_msg(s);

    if (n2 == 1)
        rc = 1;
    else
        rc = -1;

    return rc;
}

int
wrap_player(player_t * player, Proto_Session *s) {
    if (s && ((s->slen + sizeof (player_t)) < PROTO_SESSION_BUF_SIZE)) {
        memcpy(s->sbuf + s->slen, player, sizeof (player_t));
        s->slen += sizeof (player_t);
        return 1;
    }
    return -1;
}

extern int
unwrap_player(Proto_Session *s, int offset, player_t *v) {
    if (s && ((s->rlen - (offset + sizeof (player_t))) >= 0)) {
        *v = *((player_t *) (s->rbuf + offset));
        return offset + sizeof (player_t);
    }
    return -1;
}

extern int
wrap_cell(cell_t * cell, Proto_Session *s) {
    //printf("%d,%d %d\n", s->slen, sizeof (cell_t), PROTO_SESSION_BUF_SIZE);
    if (s && ((s->slen + sizeof (cell_t)) < PROTO_SESSION_BUF_SIZE)) {
        memcpy(s->sbuf + s->slen, cell, sizeof (cell_t));
        s->slen += sizeof (cell_t);
        return 1;
    }
    return -1;
}

extern int
unwrap_cell(Proto_Session *s, int offset, cell_t *v) {
    if (s && ((s->rlen - (offset + sizeof (cell_t))) >= 0)) {
        *v = *((cell_t *) (s->rbuf + offset));
        return offset + sizeof (cell_t);
    }
    return -1;
}

int
wrap_item(item_t * item, Proto_Session *s) {
    if (s && ((s->slen + sizeof (item_t)) < PROTO_SESSION_BUF_SIZE)) {
        memcpy(s->sbuf + s->slen, item, sizeof (item_t));
        s->slen += sizeof (item_t);
        return 1;
    }
    return -1;
}

extern int
unwrap_item(Proto_Session *s, int offset, item_t *v) {
    if (s && ((s->rlen - (offset + sizeof (item_t))) >= 0)) {
        *v = *((item_t *) (s->rbuf + offset));
        return offset + sizeof (item_t);
    }
    return -1;
}

/*
 * dim_c
 * dim_r
 * num_t1_players
 * num_t2_players
 * t1_flag
 * t2_flag
 * t1_jack
 * t2_jack
 * player_t 400
 */
extern int
wrap_update(maze_t * maze, Proto_Session * s) {
    if (proto_session_body_marshall_int(s, maze->dim_c) == -1) {
        fprintf(stderr, "ERROR: marhsall dim_c\n");
        return -1;
    }
    if (proto_session_body_marshall_int(s, maze->dim_r) == -1) {
        fprintf(stderr, "ERROR: marhsall dim_r\n");
        return -1;
    }
    if (proto_session_body_marshall_int(s, maze->num_t1_players) == -1) {
        fprintf(stderr, "ERROR: marhsall num_t1_players\n");
        return -1;
    }
    if (proto_session_body_marshall_int(s, maze->num_t2_players) == -1) {
        fprintf(stderr, "ERROR: marhsall num_t2_players\n");
        return -1;
    }

    if (wrap_item(maze->t1_flag, s) == -1) {
        fprintf(stderr, "ERROR: marhsall t1_flag\n");
        return -1;
    }
    if (wrap_item(maze->t2_flag, s) == -1) {
        fprintf(stderr, "ERROR: marhsall t2_flag\n");
        return -1;
    }
    if (wrap_item(maze->t1_jack, s) == -1) {
        fprintf(stderr, "ERROR: marhsall t1_jack\n");
        return -1;
    }
    if (wrap_item(maze->t2_jack, s) == -1) {
        fprintf(stderr, "ERROR: marhsall t2_jack\n");
        return -1;
    }

    int i;
    for (i = 0; i < (maze->num_t1_players + maze->num_t2_players); i++) {
        if (wrap_player((maze->players[i]), s) == -1) {
            fprintf(stderr, "ERROR: marhsall player_t\n");
            return -1;
        }
    }

    return 1;
}

extern int
wrap_maze(maze_t * maze, Proto_Session * s, int r) {
    int j;
    for (j = 0; j < NUM_COLUMN; j++) {
        if (wrap_cell((maze->cells[r][j]), s) == -1) {
            fprintf(stderr, "ERROR: marhsall cell_t\n");
            return -1;
        }
    }

}