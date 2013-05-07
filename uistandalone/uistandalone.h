#ifndef __DA_GAME_UI_H__
#define __DA_GAME_UI_H__
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

#include <SDL/SDL.h>   /* All SDL apps need this */

#define SPRITE_H 32
#define SPRITE_W 32
#define SCREEN_H 320
#define SCREEN_W 320


typedef enum { 
  TEAMA_S=0, TEAMB_S, FLOOR_S, REDWALL_S, GREENWALL_S, LOGO_S, JACKHAMMER_S, REDFLAG_S, GREENFLAG_S, NUM_S 
} SPRITE_INDEX;

typedef struct {
  SDL_Surface *img;
  uval base_clip_x;
  SDL_Rect clip;
} UI_Image;

typedef struct {
  UI_Image *uimg;
  int id;
  int x, y;
  int team;
  int state;
} UI_Player;

typedef struct {
    UI_Image *uimg;
    int x, y;
    int type;
    int team;
} UI_Item;

typedef struct {
    UI_Item* ui_items[4];
    UI_Player* ui_players[SCREEN_H/SPRITE_H][SCREEN_W/SPRITE_W];
    int ui_dim_r;
    int ui_dim_c;
    SPRITE_INDEX ui_cells[SCREEN_H/SPRITE_H][SCREEN_W/SPRITE_W];
} UI_State;

struct UI_Struct {
  SDL_Surface *screen;
  int32_t depth;
  int32_t tile_h;
  int32_t tile_w;

  struct Sprite {
    SDL_Surface *img;
  } sprites[NUM_S];

  uint32_t red_c;
  uint32_t green_c;
  uint32_t blue_c;
  uint32_t white_c;
  uint32_t black_c;
  uint32_t yellow_c;
  uint32_t purple_c;
  uint32_t isle_c;
  uint32_t wall_teama_c;
  uint32_t wall_teamb_c;
  uint32_t player_teama_c;
  uint32_t player_teamb_c;
  uint32_t flag_teama_c;
  uint32_t flag_teamb_c;
  uint32_t jackhammer_c;

  UI_State ui_state; 
};

typedef struct UI_Struct UI;


static void ui_player_init(UI *ui, UI_Player *p);
static void ui_player_paint(UI *ui, UI_Player *p, SDL_Rect *t);
static void ui_item_paint(UI *ui, UI_Item *item, SDL_Rect *t);
static sval ui_uip_init(UI *ui, UI_Image **uip, int id, int team);
static sval ui_uitem_init(UI *ui, UI_Image **uimg, int type, int team);

sval ui_zoom(UI *ui, sval fac);
sval ui_pan(UI *ui, sval xdir, sval ydir);
sval ui_move(UI *ui, sval xdir, sval ydir);
sval ui_keypress(UI *ui, SDL_KeyboardEvent *e);
void ui_update(UI *ui);
void ui_quit(UI *ui);
void ui_main_loop(UI *ui);
void ui_init(UI **ui);


// DUMMY TEST CALLS
int ui_dummy_left(UI *ui);
int ui_dummy_right(UI *ui); 
int ui_dummy_down(UI *ui);
int ui_dummy_up(UI *ui);
int ui_dummy_normal(UI *ui);
int ui_dummy_pickup_red(UI *ui);
int ui_dummy_pickup_green(UI *ui);
int ui_dummy_jail(UI *ui);
int ui_dummy_toggle_team(UI *ui);
int ui_dummy_inc_id(UI *ui);

#endif
