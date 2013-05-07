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
#include "../uistandalone/types.h"
#include "../uistandalone/tty.h"
#include "../uistandalone/uistandalone.h"

#define STRLEN 81


UI *ui;

void initGlobals(char *host, char *port);
static int event_getmap_handler(Proto_Session *s);
static int event_update_handler(Proto_Session *s);

struct Globals {
    char host[STRLEN];
    PortType port;
} globals;

typedef struct ClientState {
    int data;
    char type;
    int id;
    Proto_Client_Handle ph;
    maze_t maze;
} Client;

Client client;

void*
client_maze_init(maze_t* maze) {
    memset(maze, 0, sizeof (maze_t));
    maze->dim_c = NUM_COLUMN;
    maze->dim_r = NUM_ROW;
    int r;
    int c;
    for (r = 0; r < NUM_ROW; r++) {
        for (c = 0; c < NUM_COLUMN; c++) {
            maze->cells[r][c] = malloc(sizeof (cell_t));
        }
    }
    maze->t1_flag = (item_t *) malloc(sizeof (item_t));
    maze->t2_flag = (item_t *) malloc(sizeof (item_t));
    maze->t1_jack = (item_t *) malloc(sizeof (item_t));
    maze->t2_jack = (item_t *) malloc(sizeof (item_t));
    int n;
    for (n = 0; n < MAX_PLAYERS; n++) {
        maze->players[n] = malloc(sizeof (player_t));
    }
    return;
}

static int
clientInit(Client *C) {
    bzero(C, sizeof (Client));
    C->type = T1;
    // initialize the client protocol subsystem
    if (proto_client_init(&(C->ph)) < 0) {
        fprintf(stderr, "client: main: ERROR initializing proto system\n");
        return -1;
    }
    client_maze_init(&(client.maze));
    proto_client_set_event_handler(C->ph, PROTO_MT_EVENT_BASE_GETMAP, event_getmap_handler);
    proto_client_set_event_handler(C->ph, PROTO_MT_EVENT_BASE_UPDATE, event_update_handler);
    return 1;
}

static int
update_event_handler(Proto_Session *s) {
    //Client *C = proto_session_get_data(s);

    fprintf(stderr, "%s: called", __func__);
    return 1;
}

static int
event_getmap_handler(Proto_Session *s) {
    fprintf(stderr, "receive map\n");
    int n;
    proto_session_body_unmarshall_int(s, 0, &n);
    printf("num: %d\n", n);
    int i;
    int offset = sizeof (int);
    for (i = 0; i < n; i++) {
        cell_t* cell = malloc(sizeof (cell_t));
        offset = unwrap_cell(s, offset, cell);
        //printf("cell: %d,%d\n",cell->pos.r,cell->pos.c);
        //client.maze.cells[cell->pos.r][cell->pos.c] = malloc(sizeof (cell_t));
        memcpy(client.maze.cells[cell->pos.r][cell->pos.c], cell, sizeof (cell_t));
        free(cell);
    }
    return 1;
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
static int
event_update_handler(Proto_Session *s) {
    fprintf(stderr, "receive update\n");
    int offset = 0;
    offset = proto_session_body_unmarshall_int(s, offset, &(client.maze.dim_c));
    offset = proto_session_body_unmarshall_int(s, offset, &(client.maze.dim_r));
    offset = proto_session_body_unmarshall_int(s, offset, &(client.maze.num_t1_players));
    offset = proto_session_body_unmarshall_int(s, offset, &(client.maze.num_t2_players));
    printf("start unwrap item\n");
    fflush(stdout);
    offset = unwrap_item(s, offset, (client.maze.t1_flag));
    offset = unwrap_item(s, offset, (client.maze.t2_flag));
    offset = unwrap_item(s, offset, (client.maze.t1_jack));
    offset = unwrap_item(s, offset, (client.maze.t2_jack));
    printf("start unwrap player\n");
    fflush(stdout);
    int i;
    for (i = 0; i < (client.maze.num_t1_players + client.maze.num_t2_players); i++) {
        player_t* player = malloc(sizeof (player_t));
        if ((offset = unwrap_player(s, offset, player)) == -1) {
            fprintf(stderr, "ERROR: unmarhsall player_t\n");
            return -1;
        }
        memcpy(client.maze.players[player->id], player, sizeof (cell_t));
        free(player);
    }

    printf("dim_c: %d\n", client.maze.dim_c);
    printf("dim_r: %d\n", client.maze.dim_r);
    printf("num_t1_players: %d\n", client.maze.num_t1_players);
    printf("num_t2_players: %d\n", client.maze.num_t2_players);

    /*
        int i;
        int offset = sizeof (int);
        for (i = 0; i < n; i++) {
            cell_t* cell = malloc(sizeof (cell_t));
            unwrap_cell(s, offset, cell);
            offset += sizeof (cell_t);
            //printf("cell: %d,%d\n",cell->pos.r,cell->pos.c);
            client.maze.cells[cell->pos.r][cell->pos.c] = malloc(sizeof (cell_t));
            memcpy(client.maze.cells[cell->pos.r][cell->pos.c], cell, sizeof (cell_t));
        }
     */
    /*
        client.maze;
        player_t player;
        unwrap_player(s, sizeof (int) *4 + sizeof (item_t)*4, &player);
        printf("%d,%d\n", player.pos.c, player.pos.r);
     */
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

    if (menu) printf("\n%c>", C->id);
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
        exit(-1);
    } else {
        int rc = proto_client_hello(C->ph);
        if (rc >= 0) {
            C->id = rc;
        } else if (rc == -1) {
            fprintf(stderr, "ERROR: Not able to initialize player, probably the game is full\n");
            exit(-1);
        }

        fprintf(stdout, "Connected to %s:%d, player ID: %d\n", globals.host, globals.port, C->id);
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
doMove(Client *C, Move_Type m) {
    if (globals.host == NULL || globals.port == 0) {
        fprintf(stdout, "Not connected\n");
        return 1;
    }

    //first input type
    int* buf = malloc(sizeof (int) *3);
    int rc = proto_client_query(C->ph, MOVE, m, C->id, buf);

    if (rc < 0) {
        //error
        fprintf(stderr, "There was a problem sending a move\n");

    } else {
        printf("ACK number: %d\n", *buf);
        if (*buf == 0) {
            fprintf(stderr, "The server rejects the move\n");
        } else if (*buf == -2) {
            fprintf(stderr, "The server cannot identify the move\n");
        }
    }
    return 1;
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

    if (streql(arg0, "numhome")) {
        while (*(char*) (cmd + j) != ' ' && *(char*) (cmd + j) != ':' && *(char*) (cmd + j) != '\n') {
            j++;
        }
        memcpy(arg1, (char*) (cmd + i), 1);
        j++;
        //        printf("arg1 : %s\n", arg1);
        rc = doNumHomeCmd(C, arg1);
    } else if (streql(arg0, "numjail")) {
        while (*(char*) (cmd + j) != ' ' && *(char*) (cmd + j) != ':' && *(char*) (cmd + j) != '\n') {
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
    } else if (streql(arg0, "a")) {
        printf("a ->do rpc: up\n");
        doMove(C, MOVE_UP);
        //ui_dummy_up(ui);
        rc = 2;
    } else if (streql(arg0, "z")) {
        printf("z ->do rpc: down\n");
        doMove(C, MOVE_DOWN);
        //ui_dummy_down(ui);
        rc = 2;
    } else if (streql(arg0, ",")) {
        printf(", ->do rpc: left\n");
        doMove(C, MOVE_LEFT);
        //ui_dummy_left(ui);
        rc = 2;
    } else if (streql(arg0, ".")) {
        printf(". ->do rpc: right\n");
        doMove(C, MOVE_RIGHT);
        //ui_dummy_right(ui);
        rc = 2;
    } else {
        rc = doDefaultCmd(C);

    }
    if (rc == 2) ui_update(ui);
    return rc;
}

void *
shell(void *arg) {
    Client *C = arg;
    char *c;
    int rc;
    int menu = 1;

    //pthread_detach(pthread_self());


    while (1) {
        if ((c = prompt(C, menu)) != 0) rc = docmd(C, c);
        if (rc < 0) break;
        if (rc == 1) menu = 1;
        else menu = 0;
    }

    fprintf(stderr, "terminating\n");
    fflush(stdout);
    ui_quit(ui);
    return NULL;
}

void
initGlobals(char *host, char *port) {
    bzero(&globals, sizeof (globals));

    strncpy(globals.host, host, STRLEN);
    globals.port = atoi(port);
    printf("%s %d", globals.host, globals.port);
}

extern sval
ui_keypress(UI *ui, SDL_KeyboardEvent *e) {
    SDLKey sym = e->keysym.sym;
    SDLMod mod = e->keysym.mod;

    if (e->type == SDL_KEYDOWN) {
        if (sym == SDLK_LEFT && mod == KMOD_NONE) {
            int rc = proto_client_move(client.ph, client.type, MOVE_LEFT);
            if (rc) {
                fprintf(stderr, "%s: move left\n", __func__);
                //return ui_dummy_left(ui);
            } else {
                return 1;
            }
        }
        if (sym == SDLK_RIGHT && mod == KMOD_NONE) {
            int rc = proto_client_move(client.ph, client.type, MOVE_RIGHT);
            if (rc) {
                fprintf(stderr, "%s: move right\n", __func__);
                //return ui_dummy_right(ui);
            } else {
                return 1;
            }
        }
        if (sym == SDLK_UP && mod == KMOD_NONE) {
            int rc = proto_client_move(client.ph, client.type, MOVE_UP);
            if (rc) {
                fprintf(stderr, "%s: move up\n", __func__);
                //return ui_dummy_up(ui);
            } else {
                return 1;
            }
        }
        if (sym == SDLK_DOWN && mod == KMOD_NONE) {
            int rc = proto_client_move(client.ph, client.type, MOVE_DOWN);
            if (rc) {
                fprintf(stderr, "%s: move down\n", __func__);
                //return ui_dummy_down(ui);
            } else {
                return 1;
            }
        }
        if (sym == SDLK_r && mod == KMOD_NONE) {
            fprintf(stderr, "%s: dummy pickup red flag\n", __func__);
            //return ui_dummy_pickup_red(ui);
        }
        if (sym == SDLK_g && mod == KMOD_NONE) {
            fprintf(stderr, "%s: dummy pickup green flag\n", __func__);
            //return ui_dummy_pickup_green(ui);
        }
        if (sym == SDLK_j && mod == KMOD_NONE) {
            fprintf(stderr, "%s: dummy jail\n", __func__);
            //return ui_dummy_jail(ui);
        }
        if (sym == SDLK_n && mod == KMOD_NONE) {
            fprintf(stderr, "%s: dummy normal state\n", __func__);
            //return ui_dummy_normal(ui);
        }
        if (sym == SDLK_t && mod == KMOD_NONE) {
            fprintf(stderr, "%s: dummy toggle team\n", __func__);
            //return ui_dummy_toggle_team(ui);
        }
        if (sym == SDLK_i && mod == KMOD_NONE) {
            fprintf(stderr, "%s: dummy inc player id \n", __func__);
            //return ui_dummy_inc_id(ui);
        }
        if (sym == SDLK_q) return -1;
        if (sym == SDLK_z && mod == KMOD_NONE) return ui_zoom(ui, 1);
        if (sym == SDLK_z && mod & KMOD_SHIFT) return ui_zoom(ui, -1);
        if (sym == SDLK_LEFT && mod & KMOD_SHIFT) return ui_pan(ui, -1, 0);
        if (sym == SDLK_RIGHT && mod & KMOD_SHIFT) return ui_pan(ui, 1, 0);
        if (sym == SDLK_UP && mod & KMOD_SHIFT) return ui_pan(ui, 0, -1);
        if (sym == SDLK_DOWN && mod & KMOD_SHIFT) return ui_pan(ui, 0, 1);
        else {
            fprintf(stderr, "%s: key pressed: %d\n", __func__, sym);
        }
    } else {
        fprintf(stderr, "%s: key released: %d\n", __func__, sym);
    }
    return 1;
}

void
cell_state_to_ui(UI *ui, cell_t* c, int i, int j) {

    if (c == NULL) {
        ui->ui_state.ui_cells[i][j] = -1;
    } else {
        switch (c->type) {
            case WALL_CELL:
                ui->ui_state.ui_cells[i][j] = (c->team == T1) ? REDWALL_S : GREENWALL_S;
                ;
                break;
            case FLOOR_CELL:
            case HOME_CELL_1:
            case HOME_CELL_2:
            case JAIL_CELL_1:
            case JAIL_CELL_2:
                ui->ui_state.ui_cells[i][j] = FLOOR_S;
                break;
        }
    }
}

void
player_state_to_ui(UI *ui, maze_t *maze, player_t* player, int i, int j) {
    if (player != NULL) {
        ui->ui_state.ui_players[i][j] = (UI_Player *) malloc(sizeof (UI_Player));
        UI_Player *p = ui->ui_state.ui_players[i][j];

        p->id = player->id;
        p->x = j;
        p->y = i;
        p->team = player->team == T1 ? 0 : 1;
        p->state = 0;
        item_t *flag = maze_get_player_flag(maze, player->id);
        if (flag) {
            p->state = flag->team == T1 ? 1 : 2;
        }
        if (player->status == JAILED) {
            p->state = 3;
        }
    }

}

void
item_state_to_ui(UI *ui, item_t* item, int i, int j, int ind) {
    if (item != NULL /*&& item->holder_id < 0*/) {
        ui->ui_state.ui_items[ind] = (UI_Item *) malloc(sizeof (UI_Item));
        UI_Item *ui_item = ui->ui_state.ui_items[ind];
        ui_item->x = j;
        ui_item->y = i;
        switch (item->type) {
            case JACK:
                ui_item->type = 0;
                break;
            case FLAG:
                ui_item->type = 1;
                break;
        }
        switch (item->team) {
            case T1:
                ui_item->team = 0;
                break;
            case T2:
                ui_item->team = 1;
                break;
        }
    }

}

void
clear_ui_state(UI *ui) {
    int i, j;
    for (i = 0; i < ui->ui_state.ui_dim_r; i++) {
        for (j = 0; j < ui->ui_state.ui_dim_c; j++) {

            ui->ui_state.ui_cells[i][j] = -1;

            if (ui->ui_state.ui_players[i][j] != NULL) {
                free(ui->ui_state.ui_players[i][j]);
                ui->ui_state.ui_players[i][j] = NULL;
            }

        }
    }

    for (i = 0; i < 4; i++) {
        if (ui->ui_state.ui_items[i] != NULL) {
            free(ui->ui_state.ui_items[i]);
            ui->ui_state.ui_items[i] = NULL;
        }

    }

}

void
client_state_to_ui(UI* ui) {
    maze_t maze;
    bzero(&maze, sizeof (maze_t));
    proto_client_sample_board(&maze);

    clear_ui_state(ui);

    int id = client.id;
    player_t* me = maze_get_player(&maze, id);
    if (me == NULL) {
        fprintf(stderr, "Player %d does not exist\n", id);
        return;
    }

    int start_row = me->pos.r - ui->ui_state.ui_dim_r / 2;
    int start_col = me->pos.c - ui->ui_state.ui_dim_c / 2;
    int i, j;
    for (i = 0; i < ui->ui_state.ui_dim_r; i++) {
        for (j = 0; j < ui->ui_state.ui_dim_c; j++) {
            // fprintf(stdout,"%d %d ", i, j);
            // maze_print_cell(&maze, maze.cells[3][4]);

            cell_t* c = maze_get_cell(&maze, start_row + i, start_col + j);
            cell_state_to_ui(ui, c, i, j);

            if (c == NULL) continue;

            player_t* player = maze_get_player(&maze, c->player_id);
            player_state_to_ui(ui, &maze, player, i, j);

            item_t* flag = maze_get_cell_flag(&maze, c);
            if (flag) {
                switch (flag->team) {
                    case T1:
                        item_state_to_ui(ui, flag, i, j, 0);
                        break;
                    case T2:
                        item_state_to_ui(ui, flag, i, j, 1);
                        break;
                }
            }

            item_t* jack = maze_get_cell_jackhammer(&maze, c);
            if (jack) {
                switch (jack->team) {
                    case T1:
                        item_state_to_ui(ui, jack, i, j, 2);
                        break;
                    case T2:
                        item_state_to_ui(ui, jack, i, j, 3);
                        break;
                }
            }
        }
    }

}

int
main(int argc, char **argv) {
    //  Client* c = (Client*)malloc(sizeof(Client));
    Client* c = &client;


    if (clientInit(c) < 0) {
        fprintf(stderr, "ERROR: clientInit failed\n");
        return -1;
    }

    pthread_t tid;

    tty_init(/*STDIN_FILENO*/0);

    ui_init(&(ui));

    pthread_create(&tid, NULL, shell, c);

    doConnectCmd(c, argv[1], argv[2]);
    // WITH OSX ITS IS EASIEST TO KEEP UI ON MAIN THREAD
    // SO JUMP THROW HOOPS :-(
    client_state_to_ui(ui);
    ui_main_loop(ui);
    //    free(c);
    return 0;
}



