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
        /*
                printf("%x\n", ptr);
         */
        if (tmpchar == '\n') {
            //change line
            r++;
            c = 0;
            /*
             *ptr = maze->cells[r][c];
             */
        } else {
            maze->cells[r][c] = malloc(sizeof (cell_t));
            maze->cells[r][c]->pos.c = c;
            maze->cells[r][c]->pos.r = r;
            maze->cells[r][c]->player_id = -1;
            maze->cells[r][c]->flag = NO_FLAG;
            maze->cells[r][c]->jack = NO_JACK;
            if (c < NUM_COLUMN / 2) {
                maze->cells[r][c]->team = T1;
            } else {
                maze->cells[r][c]->team = T2;
            }
            /*
                        printf("ptr: %x, r: %d, c:%d\n", maze->cells[r][c], maze->cells[r][c]->pos.r, maze->cells[r][c]->pos.c);
             */

            /*
                        ptr->type = tmpchar;
             */
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
            /*
             *ptr = maze->cells[r][c];
             */
        }
    }
    fclose(fp);
    /*
        printf("r: %d, c:%d\n", maze->cells[20][20]->pos.r, maze->cells[20][20]->pos.c);
     */
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

/*
int main() {
    char* path = malloc(100);
    memcpy(path, "dagame.map", 10);
    maze_t map;
    maze_load(path, &map);
    maze_dump(&map);
    return 0;
}
 */

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
    return maze->cells[row][col];
}

extern Cell_Type
maze_get_cell_type(cell_t* c) {
    return c->type;
}

extern Team_Type
maze_get_cell_team(cell_t* c){
    return c->team;
}

extern int
maze_get_cell_player_id(cell_t* c) {
    return c->player_id;
}

extern int
maze_get_cell_occupied(cell_t* c){
    return maze_get_cell_player_id(c) < 0 ? 0 : 1;
}

extern Flag_Type
maze_get_cell_flag(cell_t* c){
    return c->flag;
}

extern Jackhammer_Type
maze_get_cell_jackhammer(cell_t* c){
    return c->jack;
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
maze_get_empty_cell(maze_t* maze, Cell_Type ct){
    int dim_r = maze_get_num_rows(maze);
    int dim_c = maze_get_num_cols(maze);
    int i, j;
    for (i = 0; i < dim_r; i++) {
        for (j = 0; j < dim_c; j++) {
            cell_t* cell = maze_get_cell(maze, i, j);
            Cell_Type cell_type = maze_get_cell_type(cell);
            int occupied = maze_get_cell_occupied(cell);
            Flag_Type flag = maze_get_cell_flag(cell);
            Jackhammer_Type jack = maze_get_cell_jackhammer(cell);
            if (cell_type == ct && !occupied && flag == NO_FLAG && jack == NO_JACK) {
                return cell;
            }
        }
    }
    return NULL;
   
}

extern cell_t*
maze_get_empty_home_cell(maze_t* maze, Team_Type team){
    switch(team){
        case T1:
            return maze_get_empty_cell(maze, HOME_CELL_1);
        case T2:
            return maze_get_empty_cell(maze, HOME_CELL_2);
    }
}


extern int maze_get_next_free_player_index(maze_t* maze){
    int i;
    for (i=0; i<MAX_PLAYERS; i++){
        if (maze->players[i] == NULL) return i;
    }
    return -1;
}

extern player_t*
maze_get_player(maze_t* maze, int player_id){
    if (player_id >= MAX_PLAYERS){
        return NULL;
    }else{
        return maze->players[player_id];
    }
}



/*** methods for printing **/

extern void
maze_print_player(player_t* player){
    
    if (player == NULL){
        fprintf(stdout, "player NULL\n");
        return;
    }

    char status;
    switch(player->status){
        case JAILED:
            status = 'J';
        case FREE:
            status = 'F';
    }
    fprintf(stdout, "player { id:%d, team:%d, pos:(%d,%d), flag:%d, jack:%d, status:%c}\n", 
            player->id, player->team, player->pos.r, player->pos.c, player->flag, player->jack, status);
}

extern void
maze_print_cell(cell_t* cell){
    
    if (cell == NULL){
        fprintf(stdout, "cell NULL\n");
        return;
    }

    char type;
    switch(cell->type){
        case HOME_CELL_1:
            type = 'H';
        case HOME_CELL_2:
            type = 'h';
        case JAIL_CELL_1:
            type = 'J';
        case JAIL_CELL_2:
            type = 'j';
        case WALL_CELL:
            type = '#';
        case FLOOR_CELL:
            type = 'f';
    }
    fprintf(stdout, "cell { team:%d, pos:(%d,%d), type:%c, player_id:%d, flag:%d, jack:%d, }\n", 
            cell->team, cell->pos.r, cell->pos.c,  type, cell->player_id, cell->flag, cell->jack);
}


/*** methods for game state ***/

/** interface to server side **/

extern player_t* 
maze_add_new_player(maze_t* maze)
{
    int total_players = maze->num_t1_players + maze->num_t2_players;
    //if too many players, don't allow any more
    if (total_players >= MAX_PLAYERS) {
        return NULL;
    }else{
        //alternate team
        Team_Type team = maze->num_t1_players >= maze->num_t2_players ? T2 : T1;
        
        int id = maze_get_next_free_player_index(maze);
        if (id < 0) return NULL; //shouldn't happen but just in case

        switch(team){
            case T1:
                maze->num_t1_players++;
                break;
            case T2:
                maze->num_t2_players++;
                break;
        } 
        cell_t* empty_cell  = maze_get_empty_home_cell(maze, team);
        if (empty_cell == NULL) return NULL; //shouldn't happen but just in case
  
        maze->players[id] = malloc(sizeof(player_t));
  
        maze->players[id]->id = id;
        maze->players[id]->team = team;
        maze->players[id]->flag = NO_FLAG;
        maze->players[id]->jack = NO_JACK;
        maze->players[id]->status = FREE;
        maze->players[id]->pos = empty_cell->pos;
        empty_cell->player_id = id;

        return maze->players[id];
    }
 
}

extern int
maze_remove_player(maze_t* maze, int player_id){
    
    player_t* player = maze_get_player(maze, player_id);
    if (player == NULL) return -1;

    Team_Type team = player->team;
    //FIXME: should drop flag and jackhammer
    position_t pos = player->pos;
    cell_t* cell = maze_get_cell(maze, pos.r, pos.c);
    if (cell != NULL) cell->player_id = -1;
    switch(team){
        case T1: maze->num_t1_players--; break;
        case T2: maze->num_t2_players--; break;
    }

    maze_free_player_mem(maze, player_id);
    
    return 1;
}
extern int move_player(maze_t* maze, int player_id, Move_Type move);


extern void
maze_free_player_mem(maze_t* maze, int player_id){
    
    if (maze->players[player_id] != NULL){
        free(maze->players[player_id]);
        maze->players[player_id] = NULL;
    }
}
