#include "entity.h"
#include "main.h"
#include "generator.h"
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
  int tw = tiles_tex_width  / rtile_width;
  picktile(&t, index, tw, rtile_width, rtile_height);
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
      e->update(e);

    if (e->onhit)
      e->onhit(e);

    // walk
    if (e->walking && e->dmap) {
      for (int s=0; s<e->speed; s++) {
        int findx = 0, findy = 0;
        int found_tile = 0;
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

          if (check_solid(tile))
            continue;

          // is it the lowest value tile?
          int value = e->dmap[(ty*level_width)+tx];
          if (value < lowest) {
            lowest = value;
            findx  = tx;
            findy  = ty;
            found_tile = tile;
          }
        }

        if (lowest != DIJ_MAX) {
          e->to.x = (float)findx;
          e->to.y = (float)findy;

          if (found_tile == TILE_DOOR_CLOSED) {
            update_chunk(findx * tile_width, findy * tile_height, TILE_DOOR_OPEN);
          }
        }
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
    e->pos.x += ((e->to.x - e->pos.x) * tile_width) * delta_time;
    e->pos.y += ((e->to.y - e->pos.y) * tile_height) * delta_time;

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
  ra.w = rtile_width;
  ra.h = rtile_height;
  rb.w = tile_width;
  rb.h = tile_height;
  for (int i=0; i<ENTITY_MAX; i++) {
    e = &entity_list[i];

    if (!e->alive)
      continue;

    SDL_SetTextureAlphaMod(tex_tiles, 255);

    if (level.layers[level.layer].tiles[((int)e->to.y*level_width)+(int)e->to.x] == TILE_DOOR_OPEN)
      SDL_SetTextureAlphaMod(tex_tiles, 50);

    ra.x = e->tiles[0].x;
    ra.y = e->tiles[0].y;
    rb.x = (e->pos.x*tile_width)-cx;
    rb.y = (e->pos.y*tile_height)-cy;
    SDL_RenderCopy(renderer, tex_tiles, &ra, &rb);

    // render a reflection
    if (check_tile(e->to.x, e->to.y) == TILE_STONE_FLOOR && check_tile(e->to.x, e->to.y+1) == TILE_STONE_FLOOR) {
      SDL_SetTextureAlphaMod(tex_tiles, 50);
      rb.y += (tile_height);
      SDL_RenderCopyEx(renderer, tex_tiles, &ra, &rb, 0, NULL, SDL_FLIP_VERTICAL);
    }
  }

  SDL_SetTextureAlphaMod(tex_tiles, 255);
}