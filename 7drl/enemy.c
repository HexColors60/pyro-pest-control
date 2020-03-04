#include "enemy.h"
#include "player.h"
#include "text.h"

void enemy_new(int enemy, int x, int y)
{
  // initialize an entity for the player
  entity_t *e = entity_new();
  if (!e)
    return;
  e->update = NULL;
  e->onhit  = enemy_hit;
  e->alive  = 1;
  e->speed  = 1;
  e->pos.x  = x;
  e->pos.y  = y;
  e->to.x   = x;
  e->to.y   = y;
  e->type   = enemy;
  e->dmap   = dmap_to_player;

  // tile 16
  int tile = 0;

  switch(e->type) {
    case ENEMY_TYPE_SKELETON: {
      tile = ENEMY_TILE_SKELETON;
      e->speed = 1;
      e->update = skeleton_update;
      e->hp = 10;
      e->resist = 1;
      break;
    }
    case ENEMY_TYPE_GOBLIN: {
      tile = ENEMY_TILE_GOBLIN;
      e->speed = 2;
      e->update = skeleton_update;
      e->hp = 15;
      e->resist = 0;
      break;
    }
    case ENEMY_TYPE_RAT: {
      tile = ENEMY_TILE_RAT;
      e->speed = 5;
      e->update = skeleton_update;
      e->hp = 5;
      e->resist = 0;
      break;
    }
    case ENEMY_TYPE_SLIME: {
      tile = ENEMY_TILE_SLIME;
      e->speed = 1;
      e->update = skeleton_update;
      e->hp = 8;
      e->resist = -1;
      break;
    }
    case ENEMY_TYPE_SPIDER: {
      tile = ENEMY_TILE_SPIDER;
      e->speed = 1;
      e->update = skeleton_update;
      e->hp = 8;
      e->resist = -1;
      break;
    }
  }

  entity_set_tile(e, 0, tile);
}

void enemy_hit(entity_t *e, int damage)
{
  char buff[512];
  switch(e->type) {
    case ENEMY_TYPE_SKELETON: {
      text_log_add("Skeleton resists some damage due to lack of flesh");
      sprintf(buff, "Skeleton suffers %i damage", damage);
      break;
    }
    case ENEMY_TYPE_GOBLIN: {
      sprintf(buff, "Goblin suffers %i damage", damage);
      break;
    }
    case ENEMY_TYPE_RAT: {
      sprintf(buff, "Rat suffers %i damage", damage);
      break;
    }
    case ENEMY_TYPE_SLIME: {
      text_log_add("Slime ignites and takes increased damage");
      sprintf(buff, "Slime suffers %i damage", damage);
      break;
    }
    case ENEMY_TYPE_SPIDER: {
      sprintf(buff, "Spider suffers %i damage", damage);
      break;
    }
  }

  text_log_add(buff);
}

void skeleton_update(entity_t *e)
{
  e->walking = 1;
}