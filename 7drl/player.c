#include "player.h"

entity_t *player = NULL;

void player_init()
{
  // discard if already initialized
  if (!player)
    player->alive = 0;

  // initialize an entity for the player
  player = entity_new();
  player->alive = 1;

  player->pos.x = 0;
  player->pos.y = 0;
  player->to.x = 0;
  player->to.y = 0;

  // tile 16
  entity_set_tile(player, 0, 16);
}