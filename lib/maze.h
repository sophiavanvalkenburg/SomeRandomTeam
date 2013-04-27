typedef enum {
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT
} Move_Type;

typedef enum {
    JAILED,
    FREE
} Player_Status;

typedef enum {
    FLOOR_CELL,
    WALL_CELL,
    HOME_CELL_1,
    HOME_CELL_2,
    JAIL_CELL_1,
    JAIL_CELL_2
} Cell_Type;

typedef enum {
    T1=1,
    T2=2
} Team_Type;

typedef enum {
    NOTHING,
    FLAG,
    JACK
} Item_Type;

typedef struct {
    int c;
    int r;
} position_t;

typedef struct {
    Team_Type team;
    Item_Type type;
    position_t pos;
    int holder_id;
} item_t;

typedef struct {
    int id;
    Team_Type team;
    position_t pos;
    Player_Status status;
} player_t;

typedef struct {
    position_t pos;
    Cell_Type type;
    int player_id;
    Team_Type team;
} cell_t;

#define NUM_COLUMN      200
#define NUM_ROW         200
#define MAX_PLAYERS     400


typedef struct {
    
    int dim_c;
    int dim_r;
    
    cell_t * cells[NUM_ROW][NUM_COLUMN];
    player_t* players[MAX_PLAYERS];
    
    int num_t1_players;
    int num_t2_players;

    item_t* t1_flag;
    item_t* t2_flag;
    item_t* t1_jack;
    item_t* t2_jack;

} maze_t;


extern int maze_load(char* path, maze_t* maze);
extern int maze_dump(maze_t* maze);

extern int maze_get_num_home_cells(maze_t* maze, Team_Type team);
extern int maze_get_num_jail_cells(maze_t* maze, Team_Type team);
extern int maze_get_num_wall_cells(maze_t* maze);
extern int maze_get_num_floor_cells(maze_t* maze);
extern int maze_get_num_rows(maze_t* maze);
extern int maze_get_num_cols(maze_t* maze);

extern cell_t* maze_get_cell(maze_t* maze, int row, int col);
extern Cell_Type maze_get_cell_type(cell_t* c);
extern Team_Type maze_get_cell_team(cell_t* c);
extern int maze_get_cell_player_id(cell_t* c);
extern int maze_get_cell_occupied(cell_t* c);
extern int maze_cell_has_flag(maze_t* maze, cell_t* c);
extern int maze_cell_has_jackhammer(maze_t* maze, cell_t* c);
extern cell_t* maze_get_empty_cell(maze_t* maze, Cell_Type ct);
extern cell_t* maze_get_empty_home_cell(maze_t* maze, Team_Type team);
extern cell_t* maze_get_empty_jail_cell(maze_t* maze, Team_Type team);
extern cell_t* maze_get_random_cell(maze_t* maze, Cell_Type ct);
extern item_t* maze_get_cell_flag(maze_t* maze, cell_t* cell);
extern item_t* maze_get_cell_jackhammer(maze_t* maze, cell_t* cell);

extern item_t* maze_get_flag(maze_t* maze, Team_Type team);
extern item_t* maze_get_jackhammer(maze_t* maze, Team_Type team);

extern player_t* maze_get_player(maze_t* maze, int player_id);
//extern int maze_set_player(maze_t* maze, player_t* player, cell_t* cell);
extern int maze_get_next_free_player_index(maze_t* maze);
extern item_t* maze_get_player_flag(maze_t* maze, int player_id);
extern item_t* maze_get_player_jackhammer(maze_t* maze, int player_id);

extern void maze_print_item(item_t* item);
extern void maze_print_cell(maze_t* maze, cell_t* cell);
extern void maze_print_player(maze_t* maze, player_t* player);
extern char maze_cell_to_char(Cell_Type ct);

extern player_t* maze_add_new_player(maze_t* maze);
extern int maze_remove_player(maze_t* maze, int player_id);
extern int maze_move_player(maze_t* maze, int player_id, Move_Type move);
extern int maze_drop_jackhammer(maze_t* maze, int player_id);
extern int maze_drop_flag(maze_t* maze, int player_id);

extern int maze_destroy_wall(maze_t* maze, cell_t* wall, player_t* player);
extern int maze_pick_up_flag(maze_t* maze, cell_t* cell, player_t* player);
extern int maze_pick_up_jackhammer(maze_t* maze, cell_t* cell, player_t* player);
extern int maze_free_jail(maze_t* maze, Team_Type team);
extern int maze_send_to_jail(maze_t* maze, player_t* player);
extern int maze_capture(maze_t* maze, player_t* captor, player_t* hostage);

extern int maze_test_win(maze_t* maze);

extern void maze_free_player_mem(maze_t* maze, int player_id);
