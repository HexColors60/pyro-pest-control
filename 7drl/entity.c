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

void entity_update()
{
  entity_t *e;
  for (int i=0; i<ENTITY_MAX; i++) {
    e = &entity_list[i];

    if (!e->alive)
      continue;

    if (e->update)
      e->update();

    if (e->onhit)
      e->onhit();

    // lerp positions
    e->pos.x += ((e->to.x - e->pos.x) * 16.0f) * delta_time;
    e->pos.y += ((e->to.y - e->pos.y) * 16.0f) * delta_time;
  }
}

void entity_render()
{
  entity_t *e;
  SDL_Rect ra, rb;
  ra.w = tile_width;
  ra.h = tile_height;
  rb.w = (tile_width * zoom);
  rb.h = (tile_height * zoom);
  for (int i=0; i<ENTITY_MAX; i++) {
    e = &entity_list[i];

    if (!e->alive)
      continue;

    ra.x = e->tiles[0].x;
    ra.y = e->tiles[0].y;
    rb.x = ((e->pos.x*16.0f)*zoom)-cx;
    rb.y = ((e->pos.y*16.0f)*zoom)-cy;
    SDL_RenderCopy(renderer, tex_tiles, &ra, &rb);
  }
}