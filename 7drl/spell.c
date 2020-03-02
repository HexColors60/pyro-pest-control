#include "spell.h"
#include "main.h"
#include "entity.h"
#include "math.h"
#include "generator.h"

#define SPELL_MAX 512

spell_t *spell_list;

void spell_init()
{
  // initialize entity list
  spell_list = malloc(sizeof(entity_t) * SPELL_MAX);

  spell_t *s;
  for (int i=0; i<SPELL_MAX; i++) {
    s = &spell_list[i];
    s->alive = 0;
  }
}

void spell_new(int s, int x0, int y0, int x1, int y1)
{
  // initialize an entity for the player
  /*entity_t *spell = entity_new();
  spell->onhit      = spell_hit;
  spell->alive      = 1;
  spell->pos.x      = x0;
  spell->pos.y      = y0;
  spell->projectile = 1;
  spell->speed      = 2;

  // tile 16
  entity_set_tile(spell, 0, TILEI_SPELL);

  // set spell specific values
  switch(s) {
    case SPELL_FIREBOLT: {
      spell->firechance = 10;
      break;
    }
  }

  // collidable tiles
  int walls[32] = {-1};
    for (int i=0; i<SOLID_COUNT; i++)
      walls[i] = solid[i];
  walls[SOLID_COUNT] = TILE_DOOR_CLOSED;

  // raycast
  char *tiles = level.layers[level.layer].tiles;
  ivec2_t positions[512] = {0};
  int count = line(x0, y0, x1, y1, level_width, level_height, tiles, walls, positions);

  // set to position to wall hit
  spell->to.x = positions[count-1].x;
  spell->to.y = positions[count-1].y;*/
}

void spell_update()
{

}

void spell_update_render()
{

}

void spell_render()
{
  spell_t *s;
  SDL_Rect ra, rb;
  ra.w = rtile_width;
  ra.h = rtile_height;
  rb.w = tile_width;
  rb.h = tile_height;
  for (int i=0; i<SPELL_MAX; i++) {
    s = &spell_list[i];

    if (!s->alive)
      continue;

    SDL_SetTextureAlphaMod(tex_tiles, 255);

    if (check_tile(s->to.y, s->to.x) == TILE_DOOR_OPEN)
      SDL_SetTextureAlphaMod(tex_tiles, 50);

    ra.x = s->tile.x;
    ra.y = s->tile.y;
    rb.x = (s->pos.x*tile_width)-cx;
    rb.y = (s->pos.y*tile_height)-cy;
    SDL_RenderCopy(renderer, tex_tiles, &ra, &rb);

    // render a reflection
    if (check_tile(s->to.x, s->to.y) == TILE_STONE_FLOOR &&
        check_tile(s->to.x, s->to.y+1) == TILE_STONE_FLOOR) {
      SDL_SetTextureAlphaMod(tex_tiles, 50);
      rb.y += (tile_height);
      SDL_RenderCopyEx(renderer, tex_tiles, &ra, &rb, 0, NULL, SDL_FLIP_VERTICAL);
    }
  }

  SDL_SetTextureAlphaMod(tex_tiles, 255);
}
