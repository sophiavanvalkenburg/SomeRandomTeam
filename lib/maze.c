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




