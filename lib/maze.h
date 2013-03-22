typedef enum {
    FLOOR_CELL,
    WALL_CELL,
    HOME_CELL_1,
    HOME_CELL_2,
    JAIL_CELL_1,
    JAIL_CELL_2,
    FLAG_CELL_1,
    FLAG_CELL_2
} Cell_Type;

typedef enum{
    OCCUPIED,
    UNOCCUPIED
}Occupacy_Type;

typedef struct {
    int c;
    int r;
    
} position_t;

typedef struct {
    position_t pos;
    Cell_Type type;
    Occupacy_Type occ;
} cell_t;

#define NUM_COLUMN      200
#define NUM_ROW         200

typedef struct {
    int dim_c;
    int dim_r;
    cell_t * cells[NUM_ROW][NUM_COLUMN];
} maze_t;


extern int load(char* path, maze_t* maze);
extern int dump(maze_t* maze);

