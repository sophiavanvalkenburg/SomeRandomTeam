#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include "maze.h"

extern int load(char* path, maze_t* maze) {
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
            maze->cells[r][c]->occ = UNOCCUPIED;
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

extern int dump(maze_t* maze) {
    /*
        printf("%d\n",maze->dim_r);
     */
    int i;
    int j;
    Cell_Type type;
    for (i = 0; i < maze->dim_r; i++) {
        for (j = 0; j < maze->dim_c; j++) {
            type = maze->cells[i][j]->type;
            switch (type) {
                case FLOOR_CELL:
                    printf(" ");
                    break;
                case WALL_CELL:
                    printf("#");
                    break;
                case HOME_CELL_1:
                    printf("h");
                    break;
                case HOME_CELL_2:
                    printf("H");
                    break;
                case JAIL_CELL_1:
                    printf("j");
                    break;
                case JAIL_CELL_2:
                    printf("J");
                    break;
                default:
                    printf("X");
                    break;
            }
        }
        printf("\n");
    }
    return 0;
}

int main() {
    char* path = malloc(100);
    memcpy(path, "daGame.map", 10);
    maze_t map;
    load(path, &map);
    dump(&map);
    return 0;
}
