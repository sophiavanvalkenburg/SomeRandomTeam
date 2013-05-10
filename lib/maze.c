#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include "maze.h"
#include <string.h>

extern char
maze_cell_to_char(Cell_Type ct) {
    char ct_char;
    switch (ct) {
        case FLOOR_CELL:
            ct_char = ' ';
            break;
        case WALL_CELL:
            ct_char = '#';
            break;
        case HOME_CELL_1:
            ct_char = 'h';
            break;
        case HOME_CELL_2:
            ct_char = 'H';
            break;
        case JAIL_CELL_1:
            ct_char = 'j';
            break;
        case JAIL_CELL_2:
            ct_char = 'J';
            break;
        default:
            ct_char = '?';
            break;
    }
    return ct_char;
}

extern int maze_load(char* path, maze_t* maze) {
    //initialize maze struct
    memset(maze, 0, sizeof (maze_t));
    maze->dim_c = NUM_COLUMN;
    maze->dim_r = NUM_ROW;
    maze->num_t1_players = 0;
    maze->num_t2_players = 0;

    if (access(path, R_OK) < 0) {
        fprintf(stderr, "No read permission for file %s\n", path);
        exit(-1);
    }
    //open
    FILE *fp;
    char tmpchar;
    if ((fp = fopen(path, "r")) == NULL) {
        fprintf(stderr, "file %s not open\n", path);
        exit(-1);
    }
    int r = 0;
    int c = 0;
    while ((tmpchar = (fgetc(fp))) != EOF) {
        if (tmpchar == '\n') {
            //change line
            r++;
            c = 0;
        } else {
            maze->cells[r][c] = malloc(sizeof (cell_t));
            maze->cells[r][c]->pos.c = c;
            maze->cells[r][c]->pos.r = r;
            maze->cells[r][c]->player_id = -1;
            if (c < NUM_COLUMN / 2) {
                maze->cells[r][c]->team = T1;
            } else {
                maze->cells[r][c]->team = T2;
            }
            switch (tmpchar) {
                case ' ':
                    maze->cells[r][c]->type = FLOOR_CELL;
                    break;
                case '#':
                    maze->cells[r][c]->type = WALL_CELL;
                    break;
                case 'h':
                    maze->cells[r][c]->type = HOME_CELL_1;
                    break;
                case 'H':
                    maze->cells[r][c]->type = HOME_CELL_2;
                    break;
                case 'j':
                    maze->cells[r][c]->type = JAIL_CELL_1;
                    break;
                case 'J':
                    maze->cells[r][c]->type = JAIL_CELL_2;
                    break;
                default:
                    break;
            }
            c++;
        }
    }

    //create flags, jackhammers
    maze->t1_flag = (item_t *) malloc(sizeof (item_t));
    maze->t1_flag->team = T1;
    maze->t1_flag->type = FLAG;
    maze->t1_flag->holder_id = -1;
    maze->t1_flag->pos = maze_get_random_cell(maze, FLOOR_CELL)->pos;

    maze->t2_flag = (item_t *) malloc(sizeof (item_t));
    maze->t2_flag->team = T2;
    maze->t2_flag->type = FLAG;
    maze->t2_flag->holder_id = -1;
    maze->t2_flag->pos = maze_get_random_cell(maze, FLOOR_CELL)->pos;

    maze->t1_jack = (item_t *) malloc(sizeof (item_t));
    maze->t1_jack->team = T1;
    maze->t1_jack->type = JACK;
    maze->t1_jack->holder_id = -1;
    maze->t1_jack->pos = maze_get_empty_home_cell(maze, T1)->pos;

    maze->t2_jack = (item_t *) malloc(sizeof (item_t));
    maze->t2_jack->team = T2;
    maze->t2_jack->type = JACK;
    maze->t2_jack->holder_id = -1;
    maze->t2_jack->pos = maze_get_empty_home_cell(maze, T2)->pos;

    fclose(fp);
    return 0;
}

extern int maze_dump(maze_t* maze) {
    /*
        printf("%d\n",maze->dim_r);
     */
    int i;
    int j;
    Cell_Type type;
    for (i = 0; i < maze->dim_r; i++) {
        for (j = 0; j < maze->dim_c; j++) {
            type = maze->cells[i][j]->type;
            printf("%c", maze_cell_to_char(type));
        }
        printf("\n");
    }
    return 0;
}


/*****
 * methods to get information about the map
 *
 */

/** dim commmand **/

extern int
maze_get_num_rows(maze_t* maze) {
    return maze->dim_r;
}

extern int
maze_get_num_cols(maze_t* maze) {
    return maze->dim_c;
}

/** cinfo command **/

extern cell_t*
maze_get_cell(maze_t* maze, int row, int col) {
    if (row >= maze->dim_r || col >= maze->dim_c
            || row < 0 || col < 0) {

        return NULL;
    } else {

        return maze->cells[row][col];
    }
}

extern Cell_Type
maze_get_cell_type(cell_t* c) {
    return c->type;
}

extern Team_Type
maze_get_cell_team(cell_t* c) {
    return c->team;
}

extern int
maze_get_cell_player_id(cell_t* c) {
    return c->player_id;
}

extern int
maze_get_cell_occupied(cell_t* c) {
    return maze_get_cell_player_id(c) < 0 ? 0 : 1;
}

extern int
maze_cell_has_flag(maze_t* maze, cell_t* c) {
    return maze_get_cell_flag(maze, c) == NULL ? 0 : 1;
}

extern int
maze_cell_has_jackhammer(maze_t* maze, cell_t* c) {
    return maze_get_cell_jackhammer(maze, c) == NULL ? 0 : 1;
}

extern item_t*
maze_get_cell_flag(maze_t* maze, cell_t* cell) {

    if (maze->t1_flag
            && maze->t1_flag->pos.c == cell->pos.c
            && maze->t1_flag->pos.r == cell->pos.r) {
        return maze->t1_flag;
    } else if (maze->t2_flag
            && maze->t2_flag->pos.c == cell->pos.c
            && maze->t2_flag->pos.r == cell->pos.r) {
        return maze->t2_flag;
    } else {
        return NULL;
    }
}

extern item_t*
maze_get_cell_jackhammer(maze_t* maze, cell_t* cell) {
    if (maze->t1_jack
            && maze->t1_jack->pos.c == cell->pos.c
            && maze->t1_jack->pos.r == cell->pos.r) {
        return maze->t1_jack;
    } else if (maze->t2_jack
            && maze->t2_jack->pos.c == cell->pos.c
            && maze->t2_jack->pos.r == cell->pos.r) {
        return maze->t2_jack;
    } else {
        return NULL;
    }
}

extern item_t*
maze_get_player_flag(maze_t* maze, int player_id) {

    player_t* p = maze_get_player(maze, player_id);
    if (p == NULL) return NULL;

    if (maze->t1_flag
            && maze->t1_flag->holder_id == player_id
            && maze->t1_flag->pos.c == p->pos.c
            && maze->t1_flag->pos.r == p->pos.r) {
        return maze->t1_flag;
    } else if (maze->t2_flag
            && maze->t2_flag->holder_id == player_id
            && maze->t2_flag->pos.c == p->pos.c
            && maze->t2_flag->pos.r == p->pos.r) {
        return maze->t2_flag;
    } else {
        return NULL;
    }
}

extern item_t*
maze_get_player_jackhammer(maze_t* maze, int player_id) {

    player_t* p = maze_get_player(maze, player_id);
    if (p == NULL) return NULL;

    if (maze->t1_jack
            && maze->t1_jack->holder_id == player_id
            && maze->t1_jack->pos.c == p->pos.c
            && maze->t1_jack->pos.r == p->pos.r) {
        return maze->t1_jack;
    } else if (maze->t2_jack
            && maze->t2_jack->holder_id == player_id
            && maze->t2_jack->pos.c == p->pos.c
            && maze->t2_jack->pos.r == p->pos.r) {
        return maze->t2_jack;
    } else {
        return NULL;
    }
}

extern int
maze_player_has_flag(maze_t* maze, int player_id) {
    return maze_get_player_flag(maze, player_id) == NULL ? 0 : 1;
}

extern int
maze_player_has_jackhammer(maze_t* maze, int player_id) {
    return maze_get_player_jackhammer(maze, player_id) == NULL ? 0 : 1;
}

extern cell_t*
maze_get_player_cell(maze_t* maze, int player_id) {

    player_t* p = maze_get_player(maze, player_id);
    if (p == NULL) return NULL;

    return maze_get_cell(maze, p->pos.r, p->pos.c);
}

extern item_t*
maze_get_flag(maze_t* maze, Team_Type t) {
    switch (t) {
        case T1: return maze->t1_flag;
        case T2: return maze->t2_flag;
    }
}

extern item_t*
maze_get_jackhammer(maze_t* maze, Team_Type t) {
    switch (t) {
        case T1: return maze->t1_jack;
        case T2: return maze->t2_jack;
    }
}

extern int
maze_get_num_cells(maze_t* maze, Cell_Type ct) {
    int num_ct = 0;
    int dim_r = maze_get_num_rows(maze);
    int dim_c = maze_get_num_cols(maze);
    int i, j;
    for (i = 0; i < dim_r; i++) {
        for (j = 0; j < dim_c; j++) {
            Cell_Type cell_type = maze_get_cell_type(maze_get_cell(maze, i, j));
            if (cell_type == ct) {
                num_ct++;
            }
        }
    }
    return num_ct;
}

extern int
maze_get_num_home_cells(maze_t* maze, Team_Type team) {
    if (team == T1)
        return maze_get_num_cells(maze, HOME_CELL_1);
    else /* team == T2 */
        return maze_get_num_cells(maze, HOME_CELL_2);
}

extern int
maze_get_num_jail_cells(maze_t* maze, Team_Type team) {
    if (team == T1)
        return maze_get_num_cells(maze, JAIL_CELL_1);
    else /* team == T2 */
        return maze_get_num_cells(maze, JAIL_CELL_2);
}

extern int
maze_get_num_wall_cells(maze_t* maze) {
    return maze_get_num_cells(maze, WALL_CELL);
}

extern int
maze_get_num_floor_cells(maze_t* maze) {
    return maze_get_num_cells(maze, FLOOR_CELL);
}

extern cell_t*
maze_get_empty_cell(maze_t* maze, Cell_Type ct) {
    int dim_r = maze_get_num_rows(maze);
    int dim_c = maze_get_num_cols(maze);
    int i, j;
    for (i = 0; i < dim_r; i++) {
        for (j = 0; j < dim_c; j++) {
            cell_t* cell = maze_get_cell(maze, i, j);
            Cell_Type cell_type = maze_get_cell_type(cell);
            int occupied = maze_get_cell_occupied(cell);
            int has_flag = maze_cell_has_flag(maze, cell);
            int has_jack = maze_cell_has_jackhammer(maze, cell);
            if (cell_type == ct && !occupied && !has_flag && !has_jack) {
                return cell;
            }
        }
    }
    return NULL;

}

extern cell_t*
maze_get_empty_home_cell(maze_t* maze, Team_Type team) {
    switch (team) {
        case T1:
            return maze_get_empty_cell(maze, HOME_CELL_1);
        case T2:
            return maze_get_empty_cell(maze, HOME_CELL_2);
        case NO_TEAM:
            return NULL;
    }
}

extern cell_t*
maze_get_random_cell(maze_t* maze, Cell_Type ct) {
    int dim_r = maze_get_num_rows(maze);
    int dim_c = maze_get_num_cols(maze);

    cell_t* c = maze_get_empty_cell(maze, ct);
    if (c == NULL) return NULL;

    int count = 10;
    //loop until you find a suitable cell, or counter runs out
    while (count > 0) {
        int rand_r = rand() % dim_r;
        int rand_c = rand() % dim_c;
        cell_t* cell = maze_get_cell(maze, rand_r, rand_c);
        Cell_Type cell_type = maze_get_cell_type(cell);
        int occupied = maze_get_cell_occupied(cell);
        int has_flag = maze_cell_has_flag(maze, cell);
        int has_jack = maze_cell_has_jackhammer(maze, cell);
        if (cell_type == ct && !occupied && !has_flag && !has_jack) {
            return cell;
        }
        count--;
    }

    return c;

}

extern int maze_get_next_free_player_index(maze_t* maze) {
    int i;
    for (i = 0; i < MAX_PLAYERS; i++) {
        if (maze->players[i] == NULL) return i;
    }
    return -1;
}

extern player_t*
maze_get_player(maze_t* maze, int player_id) {
    if (player_id >= MAX_PLAYERS || player_id == -1) {
        return NULL;
    } else {
        return maze->players[player_id];
    }
}

extern player_t* 
maze_get_player_at_pos(maze_t* maze, int i, int j){
    int k;
    for (k=0; k < MAX_PLAYERS; k++){
        player_t* p = maze_get_player(maze, k);
        if (p->pos.r == i && p->pos.c == j){
            return p;
        }
    }
    return NULL;
};
/*** methods for printing **/

extern void
maze_print_player(maze_t* maze, player_t* player) {

    if (player == NULL) {
        fprintf(stdout, "player NULL\n");
        return;
    }

    char status;
    switch (player->status) {
        case JAILED:
            status = 'J';
        case FREE:
            status = 'F';
    }

    item_t* flag = maze_get_player_flag(maze, player->id);
    int flag_type;
    if (flag == NULL) {
        flag_type = 0;
    } else {
        flag_type = flag->team;
    }
    item_t* jack = maze_get_player_jackhammer(maze, player->id);
    int jack_type;
    if (jack == NULL) {
        jack_type = 0;
    } else {
        jack_type = jack->team;
    }

    fprintf(stdout, "player { id:%d, team:%d, pos:(%d,%d), flag:%d, jack:%d, status:%c}\n",
            player->id, player->team, player->pos.r, player->pos.c, flag_type, jack_type, status);
}

extern void
maze_print_cell(maze_t* maze, cell_t* cell) {

    if (cell == NULL) {
        fprintf(stdout, "cell NULL\n");
        return;
    }

    char type = maze_cell_to_char(cell->type);

    item_t* flag = maze_get_cell_flag(maze, cell);
    int flag_type;
    if (flag == NULL) {
        flag_type = 0;
    } else {
        flag_type = flag->team;
    }
    item_t* jack = maze_get_cell_jackhammer(maze, cell);
    int jack_type;
    if (jack == NULL) {
        jack_type = 0;
    } else {
        jack_type = jack->team;
    }

    fprintf(stdout, "cell { team:%d, pos:(%d,%d), type:%c, player_id:%d, flag:%d, jack:%d }\n",
            cell->team, cell->pos.r, cell->pos.c, type, cell->player_id, flag_type, jack_type);
}

extern void
maze_print_item(item_t* item) {

    if (item == NULL) {
        fprintf(stdout, "item NULL\n");
        return;
    }


    char* item_type;
    switch (item->type) {
        case NOTHING:
            item_type = "NOTHING";
            break;
        case FLAG:
            item_type = "FLAG";
            break;
        case JACK:
            item_type = "JACK";
            break;
    }

    fprintf(stdout, "item { team:%d, type:%s, pos:(%d,%d), holder_id:%d }\n",
            item->team, item_type, item->pos.r, item->pos.c, item->holder_id);
}



/*** methods for game state ***/

/** interface to server side **/

extern player_t*
maze_add_new_player(maze_t* maze) {
    int total_players = maze->num_t1_players + maze->num_t2_players;
    //if too many players, don't allow any more
    if (total_players >= MAX_PLAYERS) {
        return NULL;
    } else {
        //alternate team
        Team_Type team = maze->num_t1_players > maze->num_t2_players ? T2 : T1;

        int id = maze_get_next_free_player_index(maze);
        if (id < 0) return NULL; //shouldn't happen but just in case

        switch (team) {
            case T1:
                maze->num_t1_players++;
                break;
            case T2:
                maze->num_t2_players++;
                break;
            case NO_TEAM:
                return NULL;
        }
        cell_t* empty_cell = maze_get_empty_home_cell(maze, team);
        if (empty_cell == NULL) return NULL; //shouldn't happen but just in case

        maze->players[id] = malloc(sizeof (player_t));

        maze->players[id]->id = id;
        maze->players[id]->team = team;
        maze->players[id]->status = FREE;
        maze->players[id]->pos = empty_cell->pos;
        empty_cell->player_id = id;

        return maze->players[id];
    }

}

extern int
maze_remove_player(maze_t* maze, int player_id) {

    player_t* player = maze_get_player(maze, player_id);
    if (player == NULL) return 0;

    Team_Type team = player->team;
    maze_drop_jackhammer(maze, player_id);
    maze_drop_flag(maze, player_id);
    cell_t* cell = maze_get_cell(maze, player->pos.r, player->pos.c);
    if (cell != NULL) cell->player_id = -1;
    switch (team) {
        case T1: maze->num_t1_players--;
            break;
        case T2: maze->num_t2_players--;
            break;
    }

    maze_free_player_mem(maze, player_id);

    return 1;
}

extern int
maze_pick_up_jackhammer(maze_t* maze, cell_t* cell, player_t* player) {
    item_t* jack = maze_get_cell_jackhammer(maze, cell);
    if (jack == NULL) return 0;

    jack->holder_id = player->id;
    jack->pos = player->pos;

    return 1;
}

extern int
maze_pick_up_flag(maze_t* maze, cell_t* cell, player_t* player) {
    item_t* flag = maze_get_cell_flag(maze, cell);
    if (flag == NULL) return 0;

    flag->holder_id = player->id;
    flag->pos = player->pos;

    return 1;
}

extern int
maze_drop_jackhammer(maze_t* maze, int player_id) {
    item_t* jack = maze_get_player_jackhammer(maze, player_id);
    if (jack == NULL) return 0;

    jack->holder_id = -1;
    //teleport to home base
    jack->pos = maze_get_empty_home_cell(maze, jack->team)->pos;

    return 1;
}

extern int
maze_drop_flag(maze_t* maze, int player_id) {
    player_t* player = maze_get_player(maze, player_id);
    if (player == NULL) return 0;

    item_t* flag = maze_get_player_flag(maze, player_id);
    if (flag == NULL) return 0;

    flag->holder_id = -1;
    flag->pos = player->pos;

    return 1;
}

extern int
maze_move_player(maze_t* maze, int player_id, Move_Type move, cell_t** list, int* sizept) {
    player_t* p = maze_get_player(maze, player_id);
    if (p == NULL) return 0;

    //is the player jailed?
    if (p->status == JAILED) return 0;

    cell_t* old_cell = maze_get_cell(maze, p->pos.r, p->pos.c);
    cell_t* new_cell;
    switch (move) {
        case MOVE_UP:
            new_cell = maze_get_cell(maze, old_cell->pos.r - 1, old_cell->pos.c);
            break;
        case MOVE_DOWN:
            new_cell = maze_get_cell(maze, old_cell->pos.r + 1, old_cell->pos.c);
            break;
        case MOVE_RIGHT:
            new_cell = maze_get_cell(maze, old_cell->pos.r, old_cell->pos.c + 1);
            break;
        case MOVE_LEFT:
            new_cell = maze_get_cell(maze, old_cell->pos.r, old_cell->pos.c - 1);
            break;
    }

    //does the cell exist? (is not out of bounds)
    if (new_cell == NULL) return 0;

    //is it a wall cell?
    if (new_cell->type == WALL_CELL) {
        //p has jackhammer?
        if (maze_player_has_jackhammer(maze, player_id)) {
            maze_destroy_wall(maze, new_cell);
            maze_drop_jackhammer(maze, p->id);
            maze_move_player_to_cell(maze, old_cell, new_cell, p);
            return 1;
        } else {
            return 0;
        }
    }

    //is there a player on the cell?
    player_t* new_cell_p = maze_get_player(maze, maze_get_cell_player_id(new_cell));
    if (new_cell_p != NULL) {

        //cannot do anything to players on your team
        if (new_cell_p->team == p->team) return 0;
        //cannot do anything to jailed players
        if (new_cell_p->status == JAILED) return 0;
        //if your side, capture player; otherwise, get captured
        if (new_cell->team == p->team) {
            maze_capture(maze, p, new_cell_p);
            //don't return yet so we can check if there is a flag (in case opponent has just dropped it)
        } else {
            maze_capture(maze, new_cell_p, p);
            return 1;
        }

    }
    //is it a jail cell?
    if (new_cell->type == JAIL_CELL_1 || new_cell->type == JAIL_CELL_2) {

        //if it's not on your side, then your teammates are trapped. free them.
        if (new_cell->team != p->team) {
            maze_free_players(maze, p->team);
        }
    }

    //cell has flag?
    if (maze_cell_has_flag(maze, new_cell)) {

        //if you are already holding a flag, can't take one.
        if (maze_player_has_flag(maze, player_id)) {
            return 0;
        } else {
            maze_pick_up_flag(maze, new_cell, p);
            maze_move_player_to_cell(maze, old_cell, new_cell, p);
            return 1;
        }
    }

    if (maze_cell_has_jackhammer(maze, new_cell)) {

        //can only take your own team's jackhammer
        if (new_cell->team != p->team) {
            return 0;
        } else {
            maze_pick_up_jackhammer(maze, new_cell, p);
            maze_move_player_to_cell(maze, old_cell, new_cell, p);
            return 1;
        }
    }

    maze_move_player_to_cell(maze, old_cell, new_cell, p);
    memcpy(list[*sizept], old_cell, sizeof (cell_t *));
    *sizept = *sizept + 1;
    memcpy(list[*sizept], new_cell, sizeof (cell_t *));
    *sizept = *sizept + 1;
    printf("move num %d\n", *sizept);
    return 1;
}

extern int
maze_move_player_to_cell(maze_t* maze, cell_t* old_cell, cell_t* new_cell, player_t* player) {

    item_t* jack = maze_get_player_jackhammer(maze, player->id);
    if (jack != NULL) {
        jack->pos = new_cell->pos;
    }
    item_t* flag = maze_get_player_flag(maze, player->id);
    if (flag != NULL) {
        flag->pos = new_cell->pos;
    }

    old_cell->player_id = -1;
    player->pos = new_cell->pos;
    new_cell->player_id = player->id;
    return 1;
}

extern int
maze_free_players(maze_t* maze, Team_Type team) {
    int i;
    for (i = 0; i < MAX_PLAYERS; i++) {
        player_t* player = maze_get_player(maze, i);
        if (player && player->team == team && player->status == JAILED) {
            player->status = FREE;
        }
    }
    return 1;
}

extern int
maze_send_to_jail(maze_t* maze, player_t* player) {

    player->status = JAILED;
    cell_t* old_cell = maze_get_cell(maze, player->pos.r, player->pos.c);
    Cell_Type ct;
    switch (player->team) {
        case T1: ct = JAIL_CELL_2;
            break;
        case T2: ct = JAIL_CELL_1;
            break;
    }
    cell_t* jail_cell = maze_get_random_cell(maze, ct);

    //should always return a cell but just in case
    if (jail_cell != NULL) {
        maze_move_player_to_cell(maze, old_cell, jail_cell, player);
    }
    return 1;
}

extern int
maze_capture(maze_t* maze, player_t* captor, player_t* hostage) {

    maze_drop_flag(maze, hostage->id);
    maze_drop_jackhammer(maze, hostage->id);

    maze_send_to_jail(maze, hostage);

    return 1;
}

extern int
maze_destroy_wall(maze_t* maze, cell_t* wall) {
    wall->type = FLOOR_CELL;
    return 1;
}

extern Team_Type
maze_test_win(maze_t* maze) {

    //win if all players are in base and contain both flags - return winning team

    if (maze_test_team_win(maze, T1)) {
        return T1;
    } else if (maze_test_team_win(maze, T2)) {
        return T2;
    } else {
        return NO_TEAM;
    }

}

extern int
maze_test_team_win(maze_t* maze, Team_Type team) {

    int num_team_players;
    Cell_Type home_cell;
    switch (team) {
        case T1:
            home_cell = HOME_CELL_1;
            num_team_players = maze->num_t1_players;
            break;
        case T2:
            home_cell = HOME_CELL_2;
            num_team_players = maze->num_t2_players;
            break;
        case NO_TEAM: return 0;
    }

    int players = 0;
    int flags = 0;
    int i, j;
    for (i = 0; i < maze->dim_r; i++) {
        for (j = 0; j < maze->dim_c; j++) {
            cell_t* cell = maze_get_cell(maze, i, j);
            if (cell->type != home_cell) continue;
            player_t* p = maze_get_player(maze, maze_get_cell_player_id(cell));
            if (p && p->team == team) {
                players++;
                if (maze_player_has_flag(maze, p->id)) {
                    flags++;
                }
            }
            if (maze_cell_has_flag(maze, cell)) {
                flags++;
            }
            if (flags == 2 && players == num_team_players) {
                return 1;
            }
        }
    }
    return 0;
}

extern void
maze_free_player_mem(maze_t* maze, int player_id) {

    if (maze->players[player_id] != NULL) {
        free(maze->players[player_id]);
        maze->players[player_id] = NULL;
    }
}
