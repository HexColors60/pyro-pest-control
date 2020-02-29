#ifndef ENTITY_H
#define ENTITY_H

#include "math.h"

// typedef void (*onhit)

typedef struct {
  fvec2_t pos, to;
  ivec2_t tiles[2];
  int alive, animated, frame;

  void (*onhit)();
  void (*update)();
} entity_t;

void entity_init();

void entity_update();

entity_t *entity_new();

void entity_set_tile(entity_t *e, int i, size_t index);

void entity_render();

#endif // ENTITY_H