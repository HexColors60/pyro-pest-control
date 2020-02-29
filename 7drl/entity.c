#include "entity.h"
#include "main.h"
#include <string.h>

#define ENTITY_MAX 1024

entity_t *entity_list;

void entity_init()
{
  // initialize entity list
  entity_list = malloc(sizeof(entity_t) * ENTITY_MAX);

  entity_t *e;
  for (int i=0; i<ENTITY_MAX; i++) {
    e = &entity_list[i];
    e->update = NULL;
    e->onhit  = NULL;
    e->alive  = 0;
  }
}

entity_t *entity_new()
{
  for (int i=0; i<ENTITY_MAX; i++) {
    if (!entity_list[i].alive) {
      memset(&entity_list[i], 0, sizeof(entity_t));
      return &entity_list[i];
    }
  }

  return NULL;
}

void entity_set_tile(entity_t *e, int i, size_t index)
{
  ivec2_t t;
  int tw = tiles_tex_width  / tile_width;
  picktile(&t, index, tw, tile_width, tile_height);
  memcpy(&e->tiles[i], &t, sizeof(ivec2_t));
}

int entity_update()
{
  int updated = 0;
  entity_t *e;
  for (int i=0; i<ENTITY_MAX; i++) {
    e = &entity_list[i];

    if (!e->alive)
      continue;

    if (e->update)
      e->update();

    if (e->onhit)
      e->onhit();

    // walk
    if (e->walking && e->dmap) {
      int findx = 0, findy = 0;
      int ex = e->to.x;
      int ey = e->to.y;
      int lowest=e->dmap[(ey*level_width)+ex];

      if (!lowest) {
        e->walking = 0;
        continue;
      }

      for (int i=0; i<8; i++) {
        int tx = MAX(0, MIN(ex + around_adjacent[i][0], level_width));
        int ty = MAX(0, MIN(ey + around_adjacent[i][1], level_height));
        int tile = level.layers[level.layer].tiles[(ty*level_width)+tx];

        if (tile != 2 && tile != 3 && tile != 4)
          continue;

        // is it the lowest value tile?
        int value = e->dmap[(ty*level_width)+tx];
        if (value < lowest) {
          lowest = value;
          findx  = tx;
          findy  = ty;
        }
      }

      if (lowest != DIJ_MAX) {
        e->to.x = (float)findx;
        e->to.y = (float)findy;
      }
    }
  }

  // return updated;
  return 1;
}

void entity_update_render()
{
  int updated = 0;
  entity_t *e;
  for (int i=0; i<ENTITY_MAX; i++) {
    e = &entity_list[i];

    if (!e->alive)
      continue;

    // lerp positions
    float s = e->projectile ? 32.0f : 16.0f;
    e->pos.x += ((e->to.x - e->pos.x) * s) * delta_time;
    e->pos.y += ((e->to.y - e->pos.y) * s) * delta_time;

    if (fabs(e->pos.x - e->to.x) <= 0.01f)
      e->pos.x = e->to.x;
    if (fabs(e->pos.y - e->to.y) <= 0.01f)
      e->pos.y = e->to.y;
  }
}

void entity_render()
{
  entity_t *e;
  SDL_Rect ra, rb;
  ra.w = tile_width;
  ra.h = tile_height;
  rb.w = tile_width;
  rb.h = tile_height;
  for (int i=0; i<ENTITY_MAX; i++) {
    e = &entity_list[i];

    if (!e->alive)
      continue;

    ra.x = e->tiles[0].x;
    ra.y = e->tiles[0].y;
    rb.x = (e->pos.x*16.0f)-cx;
    rb.y = (e->pos.y*16.0f)-cy;
    SDL_RenderCopy(renderer, tex_tiles, &ra, &rb);
  }
}