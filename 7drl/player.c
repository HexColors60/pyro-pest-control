#include "player.h"
#include "main.h"
#include "generator.h"
#include "spell.h"
#include "text.h"

entity_t *player = NULL;
int *dmap_to_player = NULL, *dmap_from_player = NULL;
int *dmap_to_mouse = NULL;
uint8_t *light_map;

char inventory[4] = {0};

int aiming = 0, flame = 1;
int active_spell = SPELL_FIREBOLT;
ivec2_t fire_to;

void player_init(int x, int y)
{
  // discard if already initialized
  if (!player)
    player->alive = 0;

  // initialize an entity for the player
  player = entity_new();
  player->onhit  = player_hit;
  player->update = NULL;
  player->alive  = 1;
  player->hp     = 10;
  player->hp_max = player->hp;
  player->speed  = 2;
  player->pos.x  = x;
  player->pos.y  = y;
  player->to.x   = x;
  player->to.y   = y;
  fire_to.x      = x;
  fire_to.y      = y;

  // tile 16
  entity_set_tile(player, 0, TILEI_PLAYER);

  // initialize dmap arrays
  if (dmap_to_mouse)
    free(dmap_to_mouse);
  dmap_to_mouse = malloc(sizeof(int) * level_width * level_height);
  if (dmap_to_player)
    free(dmap_to_player);
  dmap_to_player = malloc(sizeof(int) * level_width * level_height);
  if (dmap_from_player)
    free(dmap_from_player);
  dmap_from_player = malloc(sizeof(int) * level_width * level_height);

  if (light_map)
    free(light_map);
  light_map = malloc(level_width * level_height * 4);

  player_update(player);
  player_light();
}

void player_hit(entity_t *e, int damage, int type)
{
  if (damage > 0) {
    char buff[512];
    sprintf(buff, "You take %i damage", damage);
    text_log_add(buff);
  }

  if (type == SPELL_WEB) {
    text_log_add("You get caught in a spider web");
    text_log_add("You cannot move");
    player->stuck = 1 + roll(3);
  }
}

void player_light()
{
  /*-----------------------------------------/
  /---------------- FOV ---------------------/
  /-----------------------------------------*/
  for (int i=0; i<level_width*level_height; i++) {
    light_map[(i*4)+0] = 0;
    light_map[(i*4)+1] = 0;
    light_map[(i*4)+2] = 0;
    light_map[(i*4)+3] = 255;
  }

  char *tiles = level.layers[level.layer].tiles;

  ivec2_t positions[512] = {0};

  int walls[32] = {-1};
    for (int i=0; i<SOLID_COUNT; i++)
      walls[i] = solid[i];
  walls[SOLID_COUNT-1] = TILE_DOOR_CLOSED;

  int distance = flame ? 18 : 48;

  for (int i=0; i<360; i++) {
    float fromx = player->to.x;
    float fromy = player->to.y;

    float tox = fromx + 0.5f + ((float)distance * sintable[i]);
    float toy = fromy + 0.5f + ((float)distance * costable[i]);

    int count = line(fromx, fromy, tox, toy, level_width, level_height, tiles, walls, positions);

    for (int j=0; j<count; j++) {
      // get position and light value
      int x = MAX(0, MIN(positions[j].x, level_width));
      int y = MAX(0, MIN(positions[j].y, level_height));
      int val = 255 - MAX(0, 255 - MIN((j * distance), 255));

      // set pixeldata
      int index = ((y*level_width)+x)*4;
      if (light_map[index+3] > val) {
        light_map[index+0] = 0;
        light_map[index+1] = 0;
        light_map[index+2] = 0;
        light_map[index+3] = val;
      }

      int tile = check_tile(x, y);
      if (tile == TILE_STONE_HWALL || tile == TILE_STONE_VWALL || tile == TILE_DOOR_CLOSED || tile == TILE_DOOR_OPEN)
        continue;

      // light dark walls that are next to lit floor
      for (int k=0; k<8; k++) {
        int cx = MAX(0, MIN(x + around[k][0], level_width));
        int cy = MAX(0, MIN(y + around[k][1], level_height));
        if (cx == x && cy == y)
          continue;

        int tile = check_tile(cx, cy);
        int index = ((cy*level_width)+cx)*4;
        if (light_map[index+3] > val) {
          light_map[index+0] = 0;
          light_map[index+1] = 0;
          light_map[index+2] = 0;
          light_map[index+3] = val;
        }
      }
    }
  }
  /*----------------------------------------*/
}

void player_fire()
{
  aiming = 0;

  spell_new(active_spell, player->to.x, player->to.y, fire_to.x, fire_to.y);
  text_log_add("You cast a firebolt");
  update = 1;
}

int player_update()
{
  int update = 0;

  if (!player->alive) {
    player->hp = 0;
    return 1;
  }

  if (player->walking)
    update = 1;

  // regenerate dmap to player
  char *tiles = level.layers[level.layer].tiles;
  for (int i=0; i<level_width*level_height; i++)
    dmap_to_player[i] = DIJ_MAX;

  dmap_to_player[((int)player->to.y*level_width)+(int)player->to.x] = 0;

  int walls[32] = {-1};
  for (int i=0; i<SOLID_COUNT; i++)
    walls[i] = solid[i];

  int w = (window_width/tile_width)/2;
  int h = (window_height/tile_height)/2;
  int fromx = player->to.x - w;
  int fromy = player->to.y - h;
  int tox   = player->to.x + w;
  int toy   = player->to.y + h;
  dijkstra(dmap_to_player, tiles, walls, fromx, fromy, tox, toy, level_width, level_height);

  // calculate fleeing dmap map
  for (int i=0; i<level_width*level_height; i++) {
    if (dmap_to_player[i] == DIJ_MAX)
      dmap_from_player[i] = -DIJ_MAX;
    else
      dmap_from_player[i] = -(dmap_to_player[i] * 1.2f);
  }

  // recalculate it
  dijkstra(dmap_from_player, tiles, walls, fromx, fromy, tox, toy, level_width, level_height);

  return update;
}

void player_render()
{
  SDL_Rect ra, rb;
  if (player->stuck) {
    ivec2_t t;
    int tw = tiles_tex_width  / rtile_width;
    picktile(&t, TILE_SPELL_WEB, tw, rtile_width, rtile_height);

    ra.x = t.x;
    ra.y = t.y;
    ra.w = tile_width;
    ra.h = tile_height;
    rb.w = tile_width;
    rb.h = tile_height;
    rb.x = (player->pos.x*tile_width)-cx;
    rb.y = (player->pos.y*tile_height)-cy;
    SDL_RenderCopy(renderer, tex_tiles, &ra, &rb);
  }

  /*-----------------------------------------/
  /---------------- AIMING ------------------/
  /-----------------------------------------*/
  if (!aiming)
    return;

  // collidable tiles
  int walls[32] = {-1};
    for (int i=0; i<SOLID_COUNT; i++)
      walls[i] = solid[i];
  walls[SOLID_COUNT-1] = TILE_DOOR_CLOSED;

  // raycast
  char *tiles = level.layers[level.layer].tiles;
  ivec2_t positions[512] = {0};

  int tmx = ((mx + cx)) / tile_width;
  int tmy = ((my + cy)) / tile_height;
  tmx = MAX(0, MIN(tmx, level_width));
  tmy = MAX(0, MIN(tmy, level_height));

  int count = line(player->to.x, player->to.y, tmx, tmy, level_width, level_height, tiles, walls, positions);

  ra.x = 0;
  ra.y = 0;
  ra.w = rtile_width;
  ra.h = rtile_height;
  rb.w = tile_width;
  rb.h = tile_height;
  fire_to.x = player->to.x;
  fire_to.y = player->to.y;
  for (int j=1; j<count; j++) {
    int tile = check_tile(positions[j].x, positions[j].y);
    if (j > spell_get_range(active_spell) || (tile == TILE_STONE_HWALL || tile == TILE_STONE_VWALL || tile == TILE_DOOR_CLOSED))
      break;
    rb.x = MAX(0, MIN(positions[j].x, level_width)) * tile_width;
    rb.y = MAX(0, MIN(positions[j].y, level_height)) * tile_height;
    rb.x -= cx;
    rb.y -= cy;
    fire_to.x = positions[j].x;
    fire_to.y = positions[j].y;
    SDL_RenderCopy(renderer, tex_tiles, &ra, &rb);
  }
  /*----------------------------------------*/
}

void player_keypress(int key)
{
  if (player->walking) {
    player->walking = 0;
    return;
  }

  aiming = !aiming;
}

void player_mousepress(int button, int mx, int my)
{
  if (aiming) {
    player_fire();
    return;
  }

  /*-----------------------------------------/
  /---------------- mouse dmap --------------/
  /-----------------------------------------*/
  int tmx = ((mx + cx)) / tile_width;
  int tmy = ((my + cy)) / tile_height;
  tmx = MAX(0, MIN(tmx, level_width));
  tmy = MAX(0, MIN(tmy, level_height));

  if (player->walking) {
    player->walking = 0;
    return;
  }

  int tile = level.layers[level.layer].tiles[(tmy*level_width)+tmx];
  if (check_solid(tile))
    return;

  // regenerate dmap to mouse
  char *tiles = level.layers[level.layer].tiles;
  for (int i=0; i<level_width*level_height; i++)
    dmap_to_mouse[i] = DIJ_MAX;

  dmap_to_mouse[(tmy*level_width)+tmx] = 0;

  int walls[32] = {-1};
  for (int i=0; i<SOLID_COUNT; i++)
    walls[i] = solid[i];

  int w = (window_width/tile_width)/2;
  int h = (window_height/tile_height)/2;
  int fromx = player->to.x - w;
  int fromy = player->to.y - h;
  int tox   = player->to.x + w;
  int toy   = player->to.y + h;
  dijkstra(dmap_to_mouse, tiles, walls, fromx, fromy, tox, toy, level_width, level_height);

  player->walking = 1;
  player->dmap = dmap_to_mouse;

  // run an update pass
  update = 1;
  tick = -(1.0 / 16.0);
  /*----------------------------------------*/
}