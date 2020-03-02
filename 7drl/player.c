#include "player.h"
#include "main.h"
#include "generator.h"
#include "spell.h"

entity_t *player = NULL;
int *dmap_to_player = NULL, *dmap_from_player = NULL;
int *dmap_to_mouse = NULL;
uint8_t *light_map;

char inventory[4] = {0};

int aiming = 0;

void player_update();

void player_init()
{
  // discard if already initialized
  if (!player)
    player->alive = 0;

  // initialize an entity for the player
  player = entity_new();
  player->update = player_update;
  player->alive      = 1;
  player->pos.x      = 0;
  player->pos.y      = 0;
  player->to.x       = 0;
  player->to.y       = 0;
  player->speed      = 1;

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

void player_update(entity_t *e)
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

  for (int i=0; i<360; i++) {
    float fromx = player->to.x;
    float fromy = player->to.y;

    float angle = (float)i * 180.0f / M_PI;
    float tox = fromx + 999.0f * cos(angle);
    float toy = fromy + 999.0f * sin(angle);

    int count = line(fromx, fromy, tox, toy, level_width, level_height, tiles, walls, positions);

    for (int j=0; j<count; j++) {
      int x = MAX(0, MIN(positions[j].x, level_width));
      int y = MAX(0, MIN(positions[j].y, level_height));
      int index = ((y*level_width)+x)*4;
      light_map[index+0] = 0;
      light_map[index+1] = 0;
      light_map[index+2] = 0;
      light_map[index+3] = 255 - MAX(0, 255 - MIN((j * 24), 255));
    }
  }
  /*----------------------------------------*/
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
    rb.x = MAX(0, MIN(positions[j].x, level_width)) * tile_width;
    rb.y = MAX(0, MIN(positions[j].y, level_height)) * tile_height;
    rb.x -= cx;
    rb.y -= cy;
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
  // spell_new(SPELL_FIREBOLT, player->to.x, player->to.y, player->to.x+spell_range[SPELL_FIREBOLT], player->to.y);
}

void player_mousepress(int button, int mx, int my)
{
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