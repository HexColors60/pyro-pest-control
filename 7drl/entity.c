#include "entity.h"
#include "main.h"
#include "generator.h"
#include "player.h"
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

void entity_hit(int x, int y, int damage)
{
  entity_t *e;
  for (int i=0; i<ENTITY_MAX; i++) {
    e = &entity_list[i];

    if (!e->alive || e->to.x != x || e->to.y != y)
      continue;

    damage = MAX(1, damage - e->resist) + (roll(2) - 1);

    if (e->onhit)
      e->onhit(e, damage);

    e->hp -= damage;
    if (e->hp <= 0) {
      e->alive = 0;
      if (e->ondeath)
        e->ondeath(e);
    }
  }
}

void entity_update()
{
  entity_t *e;
  for (int i=0; i<ENTITY_MAX; i++) {
    e = &entity_list[i];

    if (!e->alive)
      continue;

    if (e->update)
      e->update(e);

    // walk
    if (e->walking && e->dmap) {
      for (int s=0; s<e->speed; s++) {
        int found_tile = 0;
        int ex = e->to.x;
        int ey = e->to.y;
        int findx = ex, findy = ey;
        int lowest=e->dmap[(ey*level_width)+ex];

        if (!lowest) {
          e->walking = 0;
          continue;
        }

        for (int j=0; j<8; j++) {
          int tx = MAX(0, MIN(ex + around_adjacent[j][0], level_width));
          int ty = MAX(0, MIN(ey + around_adjacent[j][1], level_height));
          int tile = level.layers[level.layer].tiles[(ty*level_width)+tx];

          if (check_solid(tile))
            continue;

          int value = e->dmap[(ty*level_width)+tx];

          // make sure there's no other entity in the way
          entity_t *e2;
          for (int k=0; k<ENTITY_MAX; k++) {
            if (k == i)
              continue;

            e2 = &entity_list[k];
            if (!e2->alive)
              continue;

            if ((int)e2->to.x == tx && (int)e2->to.y == ty) {
              value = DIJ_MAX;
              break;
            }
          }

          // is it the lowest value tile?
          if (value < lowest) {
            lowest = value;
            findx  = tx;
            findy  = ty;
            found_tile = tile;
          }
        }

        if (lowest != DIJ_MAX) {
          if (findx == e->to.x && findy == e->to.y) {
            e->walking = 0;
            break;
          }

          e->to.x = (float)findx;
          e->to.y = (float)findy;

          if (!lowest)
            e->walking = 0;

          if (found_tile == TILE_DOOR_CLOSED) {
            update_chunk(findx * tile_width, findy * tile_height, TILE_DOOR_OPEN);
          }
        }
      }
    }
  }
}

int entity_ready()
{
  entity_t *e;
  for (int i=0; i<ENTITY_MAX; i++) {
    e = &entity_list[i];

    if (!e->alive)
      continue;

    if (e->updating)
      return 0;
  }

  return 1;
}

void entity_update_render()
{
  entity_t *e;
  for (int i=0; i<ENTITY_MAX; i++) {
    e = &entity_list[i];

    if (!e->alive)
      continue;

    if (e->pos.x == e->to.x && e->pos.y == e->to.y)
      e->updating = 0;
    else
      e->updating = 1;

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

    float ex = (e->to.x*tile_width)-cx;
    float ey = (e->to.y*tile_height)-cy;
    if (ex < 0 || ey < 0 || ex > window_width || ey > window_height)
      continue;

    int index = ((e->to.y*level_width)+e->to.x) * 4;
    if (light_map[index+3] == 255)
      continue;

    SDL_SetTextureAlphaMod(tex_tiles, 255);

    if (level.layers[level.layer].tiles[((int)e->to.y*level_width)+(int)e->to.x] == TILE_DOOR_OPEN)
      SDL_SetTextureAlphaMod(tex_tiles, 80);

    ra.x = e->tiles[0].x;
    ra.y = e->tiles[0].y;
    rb.x = (e->pos.x*tile_width)-cx;
    rb.y = (e->pos.y*tile_height)-cy;
    SDL_RenderCopy(renderer, tex_tiles, &ra, &rb);

    // render a reflection
    if (check_tile(e->to.x, e->to.y) == TILE_STONE_FLOOR && check_tile(e->to.x, e->to.y+1) == TILE_STONE_FLOOR) {
      SDL_SetTextureAlphaMod(tex_tiles, 80);
      rb.y += (tile_height);
      SDL_RenderCopyEx(renderer, tex_tiles, &ra, &rb, 0, NULL, SDL_FLIP_VERTICAL);
    }
  }

  SDL_SetTextureAlphaMod(tex_tiles, 255);
}