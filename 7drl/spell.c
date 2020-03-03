#include "spell.h"
#include "main.h"
#include "entity.h"
#include "math.h"
#include "generator.h"
#include "player.h"

#define SPELL_MAX 512

spell_t *spell_list;

void spell_init()
{
  // initialize entity list
  spell_list = malloc(sizeof(spell_t) * SPELL_MAX);

  spell_t *spell;
  for (int i=0; i<SPELL_MAX; i++) {
    spell = &spell_list[i];
    spell->alive = 0;
  }
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
      tile = TILE_SPELL_FIREBOLT;
      break;
    }
  }

  ivec2_t t;
  int tw = tiles_tex_width  / rtile_width;
  picktile(&t, tile, tw, rtile_width, rtile_height);
  memcpy(&spell->tile, &t, sizeof(ivec2_t));

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
  spell->pos.x  = x0;
  spell->pos.y  = y0;
  spell->from.x = x0;
  spell->from.y = y0;
  spell->t      = 0.0f;
  spell->to.x   = positions[count-1].x;
  spell->to.y   = positions[count-1].y;
}

void spell_update()
{

}

int spell_ready()
{
  int update = 0;
  spell_t *spell;
  for (int i=0; i<SPELL_MAX; i++) {
    spell = &spell_list[i];

    if (!spell->alive)
      continue;

    if (spell->pos.x != spell->to.x && spell->pos.y != spell->to.y)
      update = 1;
  }

  return update;
}

void spell_hit(spell_t *spell)
{
  // handle enemy hits etc
  // set fires etc

  switch (spell->spell) {
    case SPELL_FIREBOLT: {

      break;
    }
  }

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

    if (!spell->alive)
      continue;

    float distance = 1.0f / hypot(spell->from.x - spell->to.x, spell->from.y - spell->to.y);
    spell->t += delta_time;
    float smoothstep = (spell->t * distance) * spell->speed;

    // lerp positions
    spell->pos.x = lerp(smoothstep, spell->from.x, spell->to.x);
    spell->pos.y = lerp(smoothstep, spell->from.y, spell->to.y);

    if (fabs(spell->pos.x - spell->to.x) <= 0.1f)
      spell->pos.x = spell->to.x;
    if (fabs(spell->pos.y - spell->to.y) <= 0.1f)
      spell->pos.y = spell->to.y;

    if (spell->pos.x == spell->to.x && spell->pos.y == spell->to.y) {
      spell_hit(spell);
      continue;
    }

    // handle lighting
    float r = 5.5f - 4.0f * (smoothstep);
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

    SDL_SetTextureAlphaMod(tex_tiles, 80);

    ra.x = spell->tile.x;
    ra.y = spell->tile.y;
    rb.x = (spell->pos.x*tile_width)-cx;
    rb.y = (spell->pos.y*tile_height)-cy;
    SDL_RenderCopy(renderer, tex_tiles, &ra, &rb);

    // render a reflection
    if (check_tile(spell->to.x, spell->to.y) == TILE_STONE_FLOOR &&
        check_tile(spell->to.x, spell->to.y+1) == TILE_STONE_FLOOR) {
      SDL_SetTextureAlphaMod(tex_tiles, 50);
      rb.y += (tile_height);
      SDL_RenderCopyEx(renderer, tex_tiles, &ra, &rb, 0, NULL, SDL_FLIP_VERTICAL);
    }
  }

  SDL_SetTextureAlphaMod(tex_tiles, 255);
}
