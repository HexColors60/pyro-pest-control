#ifndef SPELL_H
#define SPELL_H

#include "entity.h"

enum {
  SPELL_FIREBOLT,
  SPELL_NUM
};

enum {
  TILE_SPELL_FIREBOLT = 32,
  TILE_SPELL_FIRE = 33
};

typedef struct {
  fvec2_t pos, to, from;
  ivec2_t tile;
  float t, step;

  int range, firechance, speed;
  int damage, update;
  int alive, spell;
  int *dmap;
} spell_t;

int spell_get_range(int s);

void spell_init();

void spell_new(int s, int x0, int y0, int x1, int y1);

void spell_update();

int spell_ready();

void spell_hit(spell_t *spell);

void spell_update_render();

void spell_render();

#endif // SPELL_H