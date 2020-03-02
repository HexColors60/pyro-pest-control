#ifndef SPELL_H
#define SPELL_H

#include "entity.h"

enum {
  SPELL_FIREBOLT,

  SPELL_NUM
};

typedef struct {
  fvec2_t pos, to;
  ivec2_t tile;

  int range, firechance;
  int alive;
  int *dmap;
} spell_t;

void spell_new(int s, int x0, int y0, int x1, int y1);

void spell_update();

void spell_update_render();

void spell_render();

#endif // SPELL_H