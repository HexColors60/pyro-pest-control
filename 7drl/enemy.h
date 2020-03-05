#ifndef ENEMY_H
#define ENEMY_H

#include "entity.h"

enum {
  ENEMY_TYPE_SKELETON,
  ENEMY_TYPE_GOBLIN,
  ENEMY_TYPE_RAT,
  ENEMY_TYPE_SLIME,
  ENEMY_TYPE_SPIDER,
  ENEMY_TYPE_SHAMEN
};

enum {
  ENEMY_TILE_SKELETON = 17,
  ENEMY_TILE_GOBLIN   = 18,
  ENEMY_TILE_RAT      = 19,
  ENEMY_TILE_SLIME    = 20,
  ENEMY_TILE_SPIDER   = 21,
  ENEMY_TILE_SHAMEN   = 22
};

void enemy_new(int enemy, int x, int y);

void enemy_hit(entity_t *e, int damage, int type);

void enemy_update(entity_t *e);

#endif // ENEMY_H