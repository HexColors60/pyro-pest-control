#include "player.h"
#include "main.h"
#include "generator.h"
#include "spell.h"

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
  player->update = NULL;
  player->alive  = 1;
  player->pos.x  = 0;
  player->pos.y  = 0;
  player->to.x   = 0;
  player->to.y   = 0;
  player->speed  = 1;
  player->pos.x  = x;
  player->pos.y  = y;
  player->to.x   = x;
  player->to.y   = y;

  // tile 16
  entity_set_tile(player, 0, TILEI_PLAYER);

  // initialize dmap arrays
  if (dmap_to_mouse)
    free(dmap_to_mouse);
  dmap_to_mouse = malloc(sizeof(int) * level_width * level_height);

  if (light_map)
    free(light_map);
  light_map = malloc(level_width * level_height * 4);

  player_update(player);
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
  walls[SOLID_COUNT] = TILE_DOOR_CLOSED;

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
}

int player_update()
{
  int update = 0;

  if (player->walking)
    update = 1;

  player_light();

  return update;
}

void player_render()
{
  /*-----------------------------------------/
  /---------------- AIMING ------------------/
  /-----------------------------------------*/
  if (!aiming)
    return;

  // collidable tiles
  int walls[32] = {-1};
    for (int i=0; i<SOLID_COUNT; i++)
      walls[i] = solid[i];
  walls[SOLID_COUNT] = TILE_DOOR_CLOSED;

  // raycast
  char *tiles = level.layers[level.layer].tiles;
  ivec2_t positions[512] = {0};

  int tmx = ((mx + cx)) / tile_width;
  int tmy = ((my + cy)) / tile_height;
  tmx = MAX(0, MIN(tmx, level_width));
  tmy = MAX(0, MIN(tmy, level_height));

  int count = line(player->to.x, player->to.y, tmx, tmy, level_width, level_height, tiles, walls, positions);

  SDL_Rect ra, rb;
  ra.x = 0;
  ra.y = 0;
  ra.w = rtile_width;
  ra.h = rtile_height;
  rb.w = tile_width;
  rb.h = tile_height;
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

  // check if we have LOS to position
  // if so, just move straight there

  // regenerate dmap to mouse
  char *tiles = level.layers[level.layer].tiles;
  for (int i=0; i<level_width*level_height; i++)
    dmap_to_mouse[i] = DIJ_MAX;

  dmap_to_mouse[(tmy*level_width)+tmx] = 0;

  int walls[32] = {-1};
  for (int i=0; i<SOLID_COUNT; i++)
    walls[i] = solid[i];
  dijkstra(dmap_to_mouse, tiles, walls, 0, 0, level_width, level_height, level_width, level_height);

  player->walking = 1;
  player->dmap = dmap_to_mouse;

  // run an update pass
  update = 1;
  tick = -(1.0 / 16.0);
  /*----------------------------------------*/
}