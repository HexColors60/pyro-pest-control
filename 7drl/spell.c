#include "spell.h"
#include "main.h"
#include "entity.h"
#include "math.h"
#include "generator.h"
#include "player.h"

#define SPELL_MAX 512

uint8_t *fire_map;

spell_t *spell_list;
ivec2_t fire_tile;

void spell_init()
{
  // initialize entity list
  spell_list = malloc(sizeof(spell_t) * SPELL_MAX);

  spell_t *spell;
  for (int i=0; i<SPELL_MAX; i++) {
    spell = &spell_list[i];
    spell->alive = 0;
  }

  fire_map = malloc(level_width * level_height * 4);
  memset(fire_map, 0, level_width * level_height * 4);

  ivec2_t t;
  int tw = tiles_tex_width  / rtile_width;
  picktile(&t, TILE_SPELL_FIRE, tw, rtile_width, rtile_height);
  memcpy(&fire_tile, &t, sizeof(ivec2_t));
}

int spell_get_range(int s)
{
  switch (s) {
    case SPELL_FIREBOLT: {
      return 10;
      break;
    }
  }

  return 0;
}

void spell_new(int s, int x0, int y0, int x1, int y1)
{
  // get a new spell instance
  spell_t *spell = NULL;
  for (int i=0; i<SPELL_MAX; i++) {
    if (!spell_list[i].alive) {
      memset(&spell_list[i], 0, sizeof(spell_t));
      spell = &spell_list[i];
      break;
    }
  }

  // spell list overflow
  if (!spell)
    return;

  // set spell specific stats
  spell->spell = s;
  spell->alive = 1;
  int tile = 0;
  switch (s) {
    case SPELL_FIREBOLT: {
      spell->range      = spell_get_range(s);
      spell->firechance = 10;
      spell->speed      = 20;
      spell->damage     = 3;
      tile = TILE_SPELL_FIREBOLT;
      break;
    }
  }

  ivec2_t t;
  int tw = tiles_tex_width  / rtile_width;
  picktile(&t, tile, tw, rtile_width, rtile_height);
  memcpy(&spell->tile, &t, sizeof(ivec2_t));

  // set to position to wall hit
  spell->pos.x  = x0;
  spell->pos.y  = y0;
  spell->from.x = x0;
  spell->from.y = y0;
  spell->t      = 0.0f;
  spell->to.x   = x1;
  spell->to.y   = y1;
  spell->update = 0;
}

void spell_update()
{
  spell_t *spell;
  for (int i=0; i<SPELL_MAX; i++) {
    spell = &spell_list[i];

    if (!spell->alive)
      continue;

    if (!spell->update)
      spell->update = 1;
  }

  for (int i=0; i<level_width*level_height; i++) {
    if (!fire_map[i])
      continue;

    fire_map[i]--;
  }
}

int spell_ready()
{
  spell_t *spell;
  for (int i=0; i<SPELL_MAX; i++) {
    spell = &spell_list[i];

    if (!spell->alive)
      continue;

    if (spell->update)
      return 0;
  }

  return 1;
}

void spell_hit(spell_t *spell)
{
  // handle enemy hits etc
  // set fires etc

  // set fires
  int tx = floor(spell->to.x);
  int ty = floor(spell->to.y);
  int tindex = (ty*level_width)+tx;
  if (roll(spell->firechance) == 1)
    fire_map[tindex] = 5;

  switch (spell->spell) {
    case SPELL_FIREBOLT: {

      break;
    }
  }

  entity_hit(tx, ty, spell->damage);

  spell->alive = 0;
}

void spell_update_render()
{
  player_light();

  ivec2_t positions[512] = {0};

  // collidable tiles
  int walls[32] = {-1};
    for (int i=0; i<SOLID_COUNT; i++)
      walls[i] = solid[i];
  walls[SOLID_COUNT] = TILE_DOOR_CLOSED;

  char *tiles = level.layers[level.layer].tiles;

  spell_t *spell;
  for (int i=0; i<SPELL_MAX; i++) {
    spell = &spell_list[i];

    if (!spell->alive || !spell->update)
      continue;

    float distance = 1.0f / hypot(spell->from.x - spell->to.x, spell->from.y - spell->to.y);
    spell->t += delta_time;
    spell->step = (spell->t * distance) * spell->speed;

    // lerp positions
    spell->pos.x = lerp(spell->step, spell->from.x, spell->to.x);
    spell->pos.y = lerp(spell->step, spell->from.y, spell->to.y);

    if (spell->step >= 1.0f) {
      spell_hit(spell);
      continue;
    }

    // handle lighting
    float r = 5.5f - 4.0f * (spell->step);
    for (int j=0; j<360; j++) {
      float fromx = spell->pos.x + 0.5f;
      float fromy = spell->pos.y + 0.5f;
      float tox = fromx + (r * sintable[j]);
      float toy = fromy + (r * costable[j]);

      int count = line(fromx, fromy, tox, toy, level_width, level_height, tiles, walls, positions);

      for (int k=0; k<count; k++) {
        if (check_tile(positions[k].x, positions[k].y) == TILE_NONE)
          continue;
        float val = MIN((k / r), 1.0f);
        if (roll(2) == 1)
          val = MIN(val + 0.15f, 1.0f);
        int vala  = 100 * val;
        int valb  = 200 * val;
        int index = ((positions[k].y*level_width)+positions[k].x)*4;
        light_map[index+0] = 220 - vala;
        light_map[index+1] = 220 - valb;
        light_map[index+2] = 0;
        light_map[index+3] = 130 - vala;
      }
    }
  }

  // update lighting based on fire map
  /*int fx = MAX(0, floor(cx / tile_width) - 5);
  int fy = MAX(0, floor(cy / tile_height) - 5);
  int tx = MAX(0, MIN(fx + (window_width / tile_width) + 5, level_width));
  int ty = MAX(0, MIN(fy + (game_height / tile_height) + 5, level_height));
  for (int y=fy; y<ty; y++) {
    for (int x=fx; x<tx; x++) {
      int index = (y*level_width)+x;
      if (!fire_map[index])
        continue;

      // handle lighting
      float r = 2.5f;
      for (int j=0; j<360; j++) {
        float fromx = x + 0.5f;
        float fromy = y + 0.5f;
        float tox = fromx + (r * sintable[j]);
        float toy = fromy + (r * costable[j]);

        int count = line(fromx, fromy, tox, toy, level_width, level_height, tiles, walls, positions);

        for (int k=0; k<count; k++) {
          if (check_tile(positions[k].x, positions[k].y) == TILE_NONE)
            continue;
          float val = MIN((k / r), 1.0f);
          if (roll(2) == 1)
            val = MIN(val + 0.15f, 1.0f);
          int vala  = 100 * val;
          int valb  = 50 * val;
          int tindex = ((positions[k].y*level_width)+positions[k].x)*4;
          light_map[tindex+0] = 150 - vala;
          light_map[tindex+1] = 100 - vala;
          light_map[tindex+2] = 0;
          light_map[tindex+3] = MAX(0, MIN(light_map[tindex+3] | 50 - valb, 255));
        }
      }
    }
  }*/
}

void spell_render()
{
  spell_t *spell;
  SDL_Rect ra, rb;
  ra.w = rtile_width;
  ra.h = rtile_height;
  rb.w = tile_width;
  rb.h = tile_height;
  for (int i=0; i<SPELL_MAX; i++) {
    spell = &spell_list[i];

    if (!spell->alive)
      continue;

    SDL_SetTextureAlphaMod(tex_tiles, 120);

    ra.x = spell->tile.x;
    ra.y = spell->tile.y;
    rb.x = (spell->pos.x*tile_width)-cx;
    rb.y = (spell->pos.y*tile_height)-cy;
    SDL_RenderCopy(renderer, tex_tiles, &ra, &rb);

    // render a reflection
    if (check_tile(spell->to.x, spell->to.y) == TILE_STONE_FLOOR &&
        check_tile(spell->to.x, spell->to.y+1) == TILE_STONE_FLOOR) {
      SDL_SetTextureAlphaMod(tex_tiles, 80);
      rb.y += (tile_height);
      SDL_RenderCopyEx(renderer, tex_tiles, &ra, &rb, 0, NULL, SDL_FLIP_VERTICAL);
    }
  }

  // render fire
  int fx = MAX(0, floor(cx / tile_width) - 5);
  int fy = MAX(0, floor(cy / tile_height) - 5);
  int tx = MAX(0, MIN(fx + (window_width / tile_width) + 5, level_width));
  int ty = MAX(0, MIN(fy + (game_height / tile_height) + 5, level_height));
  SDL_SetTextureAlphaMod(tex_tiles, 150);
  for (int y=fy; y<ty; y++) {
    for (int x=fx; x<tx; x++) {
      int index = (y*level_width)+x;
      if (!fire_map[index])
        continue;

      ra.x = fire_tile.x;
      ra.y = fire_tile.y;
      rb.x = (x*tile_width)-cx;
      rb.y = (y*tile_height)-cy;
      SDL_RenderCopy(renderer, tex_tiles, &ra, &rb);
    }
  }

  SDL_SetTextureAlphaMod(tex_tiles, 255);
}
