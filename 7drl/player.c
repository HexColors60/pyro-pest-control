#include "player.h"
#include "main.h"

entity_t *player = NULL;
int *dmap_to_player = NULL, *dmap_from_player = NULL;
int *dmap_to_mouse = NULL;

void player_init()
{
  // discard if already initialized
  if (!player)
    player->alive = 0;

  // initialize an entity for the player
  player = entity_new();
  player->alive = 1;

  player->pos.x      = 0;
  player->pos.y      = 0;
  player->to.x       = 0;
  player->to.y       = 0;
  player->projectile = 0;
  player->speed      = 1;

  // tile 16
  entity_set_tile(player, 0, 16);

  // initialize dmap arrays
  if (dmap_to_mouse)
    free(dmap_to_mouse);

  dmap_to_mouse = malloc(sizeof(int) * level_width * level_height);
}

void player_keypress(int key)
{
  if (player->walking) {
    player->walking = 0;
    return;
  }
}

void player_mousepress(int button, int mx, int my)
{
  mx = ((mx + cx)) / tile_width;
  my = ((my + cy)) / tile_height;
  mx = MAX(0, MIN(mx, level_width));
  my = MAX(0, MIN(my, level_height));

  if (player->walking) {
    player->walking = 0;
    return;
  }

  int tile = level.layers[level.layer].tiles[(my*level_width)+mx];
  if (tile != 2 && tile != 3 && tile != 4)
    return;

  // check if we have LOS to position
  // if so, just move straight there

  // regenerate dmap to mouse
  char *tiles = level.layers[level.layer].tiles;
  for (int i=0; i<level_width*level_height; i++)
    dmap_to_mouse[i] = DIJ_MAX;

  dmap_to_mouse[(my*level_width)+mx] = 0;

  int walkable[10] = {-1};
  walkable[0] = 2; // floor
  walkable[1] = 3; // floor
  walkable[2] = 4; // door
  dijkstra(dmap_to_mouse, tiles, walkable, 0, 0, level_width, level_height, level_width, level_height);

  player->walking = 1;
  player->dmap = dmap_to_mouse;

  // run an update pass
  update = 1;
  tick = - (1.0 / 16.0);
}