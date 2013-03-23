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

typedef enum {
    OCCUPIED=1,
    UNOCCUPIED=0
} Occupancy_Type;

typedef enum {
    T1=1,
    T2=2
} Team_Type;

typedef struct {
    int c;
    int r;

} position_t;

typedef struct {
    position_t pos;
    Cell_Type type;
    Occupancy_Type occ;
    Team_Type team;
} cell_t;

#define NUM_COLUMN      200
#define NUM_ROW         200

typedef struct {
    int dim_c;
    int dim_r;
    cell_t * cells[NUM_ROW][NUM_COLUMN];
} maze_t;


extern int maze_load(char* path, maze_t* maze);
extern int maze_dump(maze_t* maze);
extern int
maze_get_num_home_cells(maze_t* maze, Team_Type team);
extern int
maze_get_num_jail_cells(maze_t* maze, Team_Type team);
extern int
maze_get_num_wall_cells(maze_t* maze);
extern int
maze_get_num_floor_cells(maze_t* maze);

extern int
maze_get_num_rows(maze_t* maze);

extern int
maze_get_num_cols(maze_t* maze);

extern cell_t*
maze_get_cell(maze_t* maze, int row, int col);

extern Cell_Type
maze_get_cell_type(cell_t* c);

extern Team_Type
maze_get_cell_team(cell_t* c);

extern Occupancy_Type
maze_get_cell_occupied(cell_t* c);

extern char
maze_cell_to_char(Cell_Type ct);



