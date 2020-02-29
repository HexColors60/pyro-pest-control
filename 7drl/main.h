#ifndef MAIN_H
#define MAIN_H

#include <SDL2/SDL.h>

#define CHUNK_STRIDE 8     // width of chunk array
#define CHUNK_COUNT  (8*8) // max amount of chunks we will need
#define CHUNK_SIZE   512   // size in pixels

extern SDL_Window *window;
extern SDL_Renderer *renderer;
static const int window_width  = 800;
static const int window_height = 640;
static const int game_height   = 512;
static const int tile_width    = 16;
static const int tile_height   = 16;

extern double delta_time, tick;
extern float camx, camy, cx, cy, zoom;

extern int update;

extern int tiles_tex_width, tiles_tex_height;

extern SDL_Texture *tex_tiles;

extern uint8_t keys_down[SDL_NUM_SCANCODES];

typedef struct {
  size_t width, height, sx, sy, ex, ey;
  char *tiles;
} map_t;

typedef struct {
  map_t layers[50];
  int layer, max;
} level_t;

// each chunk is 512x512px
// so, 32x32 tiles of 16x16px each
typedef struct {
  SDL_Texture *tex;
  int update, tile_count;
} level_chunk_t;

typedef struct {
  level_chunk_t chunks[CHUNK_COUNT];
} level_texture_t;

extern level_t level;
extern level_texture_t level_textures;
extern int level_width, level_height;

#endif // MAIN_H