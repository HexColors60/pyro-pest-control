#ifndef ENTITY_H
#define ENTITY_H

#include "math.h"

typedef struct {
  fvec2_t pos, to;
  ivec2_t tiles[2];
  int alive, animated, frame;

  int walking, speed;
  int *dmap;

  void (*onhit)(void *e);
  void (*update)(void *e);
  void (*ondeath)(void *e);
} entity_t;

void entity_init();

int entity_update();

void entity_update_render();

entity_t *entity_new();

void entity_set_tile(entity_t *e, int i, size_t index);

void entity_render();

#endif // ENTITY_H