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
#include <string.h>
#include "../lib/types.h"
#include "../lib/protocol_client.h"
#include "../lib/protocol_utils.h"
#include "../lib/maze.h"

#define STRLEN 81
#define T1 '1'
#define T2 '2'

void initGlobals(char *host, char *port);

struct Globals {
    char host[STRLEN];
    PortType port;
} globals;

typedef struct ClientState {
    int data;
    char type;
    Proto_Client_Handle ph;
} Client;

static int
clientInit(Client *C) {
    bzero(C, sizeof (Client));
    C->type = T1;
    // initialize the client protocol subsystem
    if (proto_client_init(&(C->ph)) < 0) {
        fprintf(stderr, "client: main: ERROR initializing proto system\n");
        return -1;
    }
    return 1;
}

static int
update_event_handler(Proto_Session *s) {
    //Client *C = proto_session_get_data(s);

    fprintf(stderr, "%s: called", __func__);
    return 1;
}

int
startConnection(Client *C, char *host, PortType port, Proto_MT_Handler h) {
    if (globals.host[0] != 0 && globals.port != 0) {
        if (proto_client_connect(C->ph, host, port) != 0) {
            fprintf(stderr, "failed to connect\n");
            return -1;
        }
        proto_session_set_data(proto_client_event_session(C->ph), C);
#if 0
        if (h != NULL) {
            proto_client_set_event_handler(C->ph, PROTO_MT_EVENT_BASE_UPDATE,
                    h);
        }
#endif
        return 1;
    }
    return 0;
}

char *
prompt(Client *C, int menu) {
    //int ret;
    char *c = malloc(sizeof (char) * STRLEN);
    ;

    if (menu) printf("\n%c>", C->type);
    fflush(stdout);
    fgets(c, STRLEN, stdin);
    return c;
}


// FIXME:  this is ugly maybe the speration of the proto_client code and
//         the game code is dumb

int
game_process_reply(Client *C) {
    Proto_Session *s;

    s = proto_client_rpc_session(C->ph);

    fprintf(stderr, "%s: do something %p\n", __func__, s);

    return 1;
}

int doNumHomeCmd(Client* C, char* team) {

    if (globals.host == NULL || globals.port == 0) {
        fprintf(stdout, "Not connected\n");
        return 1;
    }

    //first input type
    int tp;
    if (*team == T1) {
        tp = T1;
    } else if (*team == T2) {
        tp = T2;
    } else {
        fprintf(stderr, "invalid team selection\n");
        return -1;
    }
    int* buf = malloc(sizeof (int) *3);
    int rc = proto_client_query(C->ph, NUM_HOME, tp, 0, buf);
    // printf("do move command: rc = %d\n",rc);
    if (rc < 0) {
        //error
        fprintf(stderr, "There was a problem getting the number of home cells\n");

    } else {
        printf("Home Cells for Team %c : %d\n", tp, *buf);
    }
    return 1;
}

int doNumJailCmd(Client* C, char* team) {

    if (globals.host == NULL || globals.port == 0) {
        fprintf(stdout, "Not connected\n");
        return 1;
    }

    //first input type
    int tp;
    if (*team == T1) {
        tp = T1;
    } else if (*team == T2) {
        tp = T2;
    } else {
        fprintf(stderr, "invalid team selection\n");
        return -1;
    }
    int* buf = malloc(sizeof (int) *3);
    int rc = proto_client_query(C->ph, NUM_JAIL, tp, 0, buf);
    // printf("do move command: rc = %d\n",rc);
    if (rc < 0) {
        //error
        fprintf(stderr, "There was a problem getting the number of jail cells\n");

    } else {
        printf("Jail Cells for Team %c : %d\n", tp, *buf);
    }
    return 1;
}

int doNumWallCmd(Client* C) {

    if (globals.host == NULL || globals.port == 0) {
        fprintf(stdout, "Not connected\n");
        return 1;
    }

    //first input type
    int* buf = malloc(sizeof (int) *3);
    int rc = proto_client_query(C->ph, NUM_WALL, 0, 0, buf);
    // printf("do move command: rc = %d\n",rc);

    if (rc < 0) {
        //error
        fprintf(stderr, "There was a problem getting the number of wall cells\n");

    } else {
        printf("Wall Cells: %d\n", *buf);
    }

    return 1;

}

int doNumFloorCmd(Client* C) {

    if (globals.host == NULL || globals.port == 0) {
        fprintf(stdout, "Not connected\n");
        return 1;
    }

    //first input type
    int* buf = malloc(sizeof (int) *3);
    int rc = proto_client_query(C->ph, NUM_FLOOR, 0, 0, buf);
    // printf("do move command: rc = %d\n",rc);

    if (rc < 0) {
        //error
        fprintf(stderr, "There was a problem getting the number of floor cells\n");

    } else {
        printf("Floor Cells: %d\n", *buf);
    }
    return 1;
}

int doDimCmd(Client* C) {
    if (globals.host == NULL || globals.port == 0) {
        fprintf(stdout, "Not connected");
        return 1;
    }

    //first input type
    int* buf = malloc(sizeof (int) *3);
    int rc = proto_client_query(C->ph, DIM, 0, 0, buf);
    // printf("do move command: rc = %d\n",rc);
    if (rc < 0) {
        //error
        fprintf(stderr, "There was a problem getting the number of floor cells\n");

    } else {
        printf("number of columns: %d\nnumber of rows: %d\n", *buf, *(int*) ((int*) buf + 1));
    }
    return 1;
}

int doCInfoCmd(Client* C, char *x, char* y) {

    if (globals.host == NULL || globals.port == 0) {
        fprintf(stdout, "Not connected");
        return 1;
    }

    //first input type
    int xaxis = atoi(x);
    int yaxis = atoi(y);
    int* buf = malloc(sizeof (int) *3);
    int rc = proto_client_query(C->ph, CINFO, xaxis, yaxis, buf);
    // printf("do move command: rc = %d\n",rc);
    if (rc < 0) {
        //error
        fprintf(stderr, "There was a problem getting the number of floor cells\n");

    } else {
        char *tp = malloc(100);
        switch (*buf) {
            case FLOOR_CELL:
                memcpy(tp, "FLOOR_CELL", 10);
                break;
            case WALL_CELL:
                memcpy(tp, "WALL_CELL", 9);
                break;
            case HOME_CELL_1:
            case HOME_CELL_2:
                memcpy(tp, "HOME_CELL", 9);
                break;
            case JAIL_CELL_1:
            case JAIL_CELL_2:
                memcpy(tp, "JAIL_CELL", 9);
                break;
            case FLAG_CELL_1:
            case FLAG_CELL_2:
                memcpy(tp, "FLAG_CELL", 9);
                break;
        }

        printf("Type: %s\nTeam: %d\nOccupancy: ", tp, *(int*) ((int*) buf + 1));
        if (*(int*) ((int*) buf + 2)) {
            printf("Occupied\n");
        } else {
            printf("Unoccupied\n");
        }
    }
    return 1;
}

int doDumpCmd(Client* C) {

    if (globals.host == NULL || globals.port == 0) {
        fprintf(stdout, "Not connected");
        return 1;
    }

    //first input type
    int* buf = malloc(sizeof (int) *3);
    int rc = proto_client_query(C->ph, DUMP, 0, 0, buf);
    // printf("do move command: rc = %d\n",rc);
    if (rc < 0) {
        //error
        fprintf(stderr, "There was a problem sending dump msg to server\n");

    } else {
        printf("sent dump msg");
    }
    return 1;
}

int
streql(char *c1, char *c2) {
    return strcmp(c1, c2) == 0;
}

int
doConnectCmd(Client *C, char *host, char *port) {
    initGlobals(host, port);

    // ok startup our connection to the server
    if (startConnection(C, globals.host, globals.port, update_event_handler) < 0) {
        fprintf(stderr, "ERROR: Not able to connect to %s:%d\n", globals.host, globals.port);
        return -1;
    } else {
        int rc = proto_client_hello(C->ph);
        if (rc == 1) {
            C->type = T1;
        } else if (rc == 0) {
            C->type = T2;
        }

        fprintf(stdout, "Connected to %s:%d\n", globals.host, globals.port);
    }

    return 1;
}

int
doQuitCmd(Client *C) {
    if (globals.host == NULL || globals.port == 0) {
        printf("not connected\n");
        return -1;
    }
    int rc = proto_client_goodbye(C->ph, 1);
    if (rc < 0) {
        fprintf(stdout, "Error: problem disconnecting\n");
        return 1;
    } else {
        printf("You Quit\n");
        return -1;
    }
}

int
doDefaultCmd(Client * C) {
    fprintf(stdout, "not a valid command");
    return 1;
}

int
docmd(Client *C, char *cmd) {
    int rc = 1, i = 0, j = 0;
    char* arg0 = malloc(100);
    char* arg1 = malloc(100);
    char* arg2 = malloc(100);

    while (*(char*) (cmd + i) != ' ' && *(char*) (cmd + i) != '\n') {
        i++;
    }
    memcpy(arg0, (char*) (cmd), i);
    i++;
    j = i;

    if (streql(arg0, "connect")) {
        while (*(char*) (cmd + j) != ' ' && *(char*) (cmd + j) != ':' && j < 100) {
            j++;
        }
        memcpy(arg1, (char*) (cmd + i), j - i);
        j++;
        memcpy(arg2, (char*) (cmd + j), strlen(cmd) - j - 1);
        rc = doConnectCmd(C, arg1, arg2);
    } else if (streql(arg0, "numhome")) {
        while (*(char*) (cmd + j) != ' ' && *(char*) (cmd + j) != ':' && *(char*) (cmd + i) != '\n') {
            j++;
        }
        memcpy(arg1, (char*) (cmd + i), 1);
        j++;
        //        printf("arg1 : %s\n", arg1);
        rc = doNumHomeCmd(C, arg1);
    } else if (streql(arg0, "numjail")) {
        while (*(char*) (cmd + j) != ' ' && *(char*) (cmd + j) != ':' && *(char*) (cmd + i) != '\n') {
            j++;
        }
        memcpy(arg1, (char*) (cmd + i), 1);
        j++;
        rc = doNumJailCmd(C, arg1);
    } else if (streql(arg0, "numwall")) {
        rc = doNumWallCmd(C);
    } else if (streql(arg0, "numfloor")) {
        rc = doNumFloorCmd(C);
    } else if (streql(arg0, "dim")) {
        rc = doDimCmd(C);
    } else if (streql(arg0, "cinfo")) {
        while (*(char*) (cmd + j) != ' ' && *(char*) (cmd + j) != ':' && j < 100) {
            j++;
        }
        memcpy(arg1, (char*) (cmd + i), j - i);
        j++;
        memcpy(arg2, (char*) (cmd + j), strlen(cmd) - j - 1);
        /*
                printf("command: %s,%s\n", arg1, arg2);
         */
        rc = doCInfoCmd(C, arg1, arg2);
    } else if (streql(arg0, "dump")) {
        rc = doDumpCmd(C);
    } else if (streql(arg0, "quit")) {
        rc = doQuitCmd(C);
    } else {
        rc = doDefaultCmd(C);

    }

    /*
        printf("aaa%saaa%saaa%saaa\n", arg0, arg1, arg2);
     */


    return rc;
}

void *
shell(void *arg) {
    Client *C = arg;
    char *c;
    int rc;
    int menu = 1;

    while (1) {
        if ((c = prompt(C, menu)) != 0) rc = docmd(C, c);
        if (rc < 0) break;
        if (rc == 1) menu = 1;
        else menu = 0;
    }

    fprintf(stderr, "terminating\n");
    fflush(stdout);
    return NULL;
}

void
usage(char *pgm) {
    fprintf(stderr, "USAGE: %s <port|<<host port> [shell] [gui]>>\n"
            "  port     : rpc port of a game server if this is only argument\n"
            "             specified then host will default to localhost and\n"
            "             only the graphical user interface will be started\n"
            "  host port: if both host and port are specifed then the game\n"
            "examples:\n"
            " %s 12345 : starts client connecting to localhost:12345\n"
            " %s localhost 12345 : starts client connecting to locaalhost:12345\n",
            pgm, pgm, pgm);

}

void
initGlobals(char *host, char *port) {
    bzero(&globals, sizeof (globals));

    strncpy(globals.host, host, STRLEN);
    globals.port = atoi(port);

}

int
main(int argc, char **argv) {
    Client c;

    // initGlobals(argc, argv);

    if (clientInit(&c) < 0) {
        fprintf(stderr, "ERROR: clientInit failed\n");
        return -1;
    }

    shell(&c);

    return 0;
}

