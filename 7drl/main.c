#include <stdio.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <emscripten/emscripten.h>

#include "main.h"
#include "math.h"
#include "text.h"
#include "texture.h"
#include "generator.h"
#include "entity.h"
#include "player.h"

// sdl vars
SDL_Window *window;
SDL_Renderer *renderer;

float camx = 512.0f, camy = 512.0f;
float cx = 0.0f, cy = 0.0f;
float zoom = 1.0f;

// delta time vars
const double phys_delta_time   = 1.0 / 60.0;
const double slowest_frame     = 1.0 / 15.0;
double delta_time, tick        = 0.0;
double last_frame_time         = 0.0;
int frame = 0;

SDL_Texture *tex_tiles, *tex_map;
int tiles_tex_width = 0, tiles_tex_height = 0;

level_t level;
level_texture_t level_textures;

int *dmap;

uint8_t keys_down[SDL_NUM_SCANCODES];

void keypressed(int key);
void input(SDL_Event *event);

void loop()
{
  /*-----------------------------------------/
  /---------------- INIT STUFF --------------/
  /-----------------------------------------*/
  if (!frame++) {
    // init SDL
    if (!SDL_Init(SDL_INIT_EVERYTHING)) {
      printf("Failed to init SDL2\n");
      return;
    }

    // init window
    window = SDL_CreateWindow("rl", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width, window_height, SDL_WINDOW_SHOWN);

    // init renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // nearest filtering
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    // dt stuff
    last_frame_time = SDL_GetPerformanceCounter();

    // load textures
    tex_tiles = texture_load("tiles.png", &tiles_tex_width, &tiles_tex_height);

    // screen render target
    tex_map = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, window_width, game_height);

    for (int i=0; i<CHUNK_COUNT; i++) {
      level_textures.chunks[i].tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, CHUNK_SIZE, CHUNK_SIZE);
      level_textures.chunks[i].update     = 1;
      level_textures.chunks[i].tile_count = 0;
    }

    // generate the first 3 layers
    level.layer = 0;
    level.max   = 10 + roll(10);
    for (int i=0; i<2; i++) {
      while (!gen(&level.layers[i])) {};
    }

    // init entity stuff
    entity_init();

    // init player
    player_init();

    // setup vga atlas etc
    text_init();

    // !! test junk !!
    int w = level.layers[level.layer].width;
    int h = level.layers[level.layer].height;
    dmap = malloc(sizeof(int) * w * h);
  }
  /*----------------------------------------*/



  /*-----------------------------------------/
  /---------------- UPDATE LOGIC ------------/
  /-----------------------------------------*/
  // calculate delta time
  double current_frame_time = (double)SDL_GetPerformanceCounter();
  delta_time = (double)(current_frame_time - last_frame_time) / (double)SDL_GetPerformanceFrequency();
  last_frame_time = current_frame_time;

  // prevent spiral of death
  if (delta_time > slowest_frame)
    delta_time = slowest_frame;

  tick += delta_time;

  // handle sdl events
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      // cya
      case SDL_QUIT:
        return;
        break;

      // input events
      case SDL_KEYDOWN:
      case SDL_KEYUP:
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
      case SDL_MOUSEWHEEL:
      case SDL_TEXTEDITING:
      case SDL_TEXTINPUT:
      case SDL_MOUSEMOTION:
      case SDL_KEYMAPCHANGED: {
        input(&event);
        break;
      }

      // window events
      case SDL_WINDOWEVENT: {
        // ex_window_event(&event);
        break;
      }
    }
  }

  float speed = 500.0f;
  if (keys_down[SDL_SCANCODE_LEFT])
    camx -= speed * delta_time;
  if (keys_down[SDL_SCANCODE_RIGHT])
    camx += speed * delta_time;
  if (keys_down[SDL_SCANCODE_UP])
    camy -= speed * delta_time;
  if (keys_down[SDL_SCANCODE_DOWN])
    camy += speed * delta_time;
  zoom = MAX(zoom, 0.5f);
  if (keys_down[SDL_SCANCODE_SPACE]) {
    player->to.x = floor(camx/16);
    player->to.y = floor(camy/16);
  }

  entity_update();
  /*----------------------------------------*/



  /*-----------------------------------------/
  /---------------- TILE MAP CHUNK RENDER ---/
  /-----------------------------------------*/
  for (int iy=0; iy<CHUNK_STRIDE; iy++) {
    for (int ix=0; ix<CHUNK_STRIDE; ix++) {
      int index = (iy*CHUNK_STRIDE)+ix;

      if (!level_textures.chunks[index].update)
        continue;

      level_textures.chunks[index].update     = 0;
      level_textures.chunks[index].tile_count = 0;

      SDL_SetRenderTarget(renderer, level_textures.chunks[index].tex);
      SDL_SetRenderDrawColor(renderer, 17, 17, 17, 255);
      SDL_RenderClear(renderer);

      SDL_Rect ra, rb;
      int w  = level.layers[level.layer].width;
      int h  = level.layers[level.layer].height;
      int tw = tiles_tex_width  / tile_width;
      int fromx = (ix*CHUNK_SIZE) / tile_width;
      int fromy = (iy*CHUNK_SIZE) / tile_height;
      int tox = fromx + (CHUNK_SIZE / tile_width);
      int toy = fromy + (CHUNK_SIZE / tile_height);
      for (int y=fromy; y<toy; y++) {
        for (int x=fromx; x<tox; x++) {
          // current tile
          int tile = 0;
          if (x >= 0 && x < w && y >= 0 && y < h)
            tile = level.layers[level.layer].tiles[(y*w)+x];

          if (!tile)
            continue;

          // position of tile in texture
          ra.x = (tile - ((tile / tw) * tw)) * tile_width;
          ra.y = (tile / tw) * tile_height;
          ra.w = tile_width;
          ra.h = tile_height;

          // position of tile in map
          rb.x = (x - fromx) * tile_width;
          rb.y = (y - fromy) * tile_height;
          rb.w = tile_width;
          rb.h = tile_height;

          SDL_RenderCopy(renderer, tex_tiles, &ra, &rb);
          level_textures.chunks[index].tile_count++;
        }
      }
    }
  }
  /*----------------------------------------*/



  /*-----------------------------------------/
  /---------------- CHUNKS TO TEXTURE -------/
  /-----------------------------------------*/
  SDL_SetRenderTarget(renderer, tex_map);
  SDL_SetRenderDrawColor(renderer, 17, 17, 17, 255);
  SDL_RenderClear(renderer);

  SDL_Rect r;
  cx = ((camx + tile_width / 2) * zoom) - (window_width / 2);
  cy = ((camy + tile_height / 2) * zoom) - (game_height / 2);
  int rzoom = zoom * (float)CHUNK_SIZE;
  int fromx = floor(cx / (float)rzoom);
  int fromy = floor(cy / (float)rzoom);
  int tox = fromx + ceil((float)(window_width + rzoom) / (float)rzoom);
  int toy = fromy + ceil((float)(game_height + rzoom) / (float)rzoom);
  for (int y=fromy; y<toy; y++) {
    for (int x=fromx; x<tox; x++) {
      if (x < 0 || y < 0 || x >= CHUNK_STRIDE || y >= CHUNK_STRIDE)
        continue;

      int index = (y*CHUNK_STRIDE)+x;

      if (!level_textures.chunks[index].tile_count)
        continue;

      r.x = (int)((x * rzoom) - (cx));
      r.y = (int)((y * rzoom) - (cy));
      r.w = (int)rzoom;
      r.h = (int)rzoom;

      SDL_RenderCopy(renderer, level_textures.chunks[index].tex, NULL, &r);
    }
  }
  /*----------------------------------------*/



  /*-----------------------------------------/
  /---------------- RENDER ------------------/
  /-----------------------------------------*/
  // render to default target
  SDL_SetRenderTarget(renderer, NULL);

  // render the level
  r.x = 0, r.y = 0, r.w = window_width, r.h = game_height;
  SDL_RenderCopy(renderer, tex_map, NULL, &r);

  // debug box
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 10);
  r.x = (window_width / 2) - (8 * zoom);
  r.y = (game_height / 2) - (8 * zoom);
  r.w = (16 * zoom);
  r.h = (16 * zoom);
  SDL_RenderDrawRect(renderer, &r);

  int w = level.layers[level.layer].width;
  int h = level.layers[level.layer].height;
  char *tiles = level.layers[level.layer].tiles;
  for (int y=0; y<h; y++) {
    for (int x=0; x<w; x++) {
      int index = (y*w)+x;
      if (tiles[index] != 2 && tiles[index] != 3)
        continue;
      int val = dmap[index];
      if (val > 50)
        continue;
      r.x = ((x*16)*zoom)-cx;
      r.y = ((y*16)*zoom)-cy;
      int c = 200*(1.0-((float)val/50));
      SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
      SDL_SetRenderDrawColor(renderer, c, c, 0, c);
      if (val == 0)
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
      SDL_RenderFillRect(renderer, &r);
      // char buf[32];
      // sprintf(buf, "%01i", val);
      // text_render(buf, (x*16)-cx, (y*16)-cy);
    }
  }

  entity_render();

  text_log_render();

  SDL_RenderPresent(renderer);
  /*----------------------------------------*/
}

void keypressed(int key)
{
  char buff[512];
  int l = 1+rand() % 61;
  for (int i=0; i<=l; i++)
    buff[i] = '0' + roll(42);
  buff[l] = '\0';
  text_log_add(buff);

  if (keys_down[SDL_SCANCODE_EQUALS])
    zoom += 0.5f;
  if (keys_down[SDL_SCANCODE_MINUS])
    zoom -= 0.5f;

  // test dmap
  if (key == SDL_SCANCODE_R) {
    int w = level.layers[level.layer].width;
    int h = level.layers[level.layer].height;
    char *tiles = level.layers[level.layer].tiles;
    memset(dmap, DIJ_MAX, sizeof(int) * w * h);
    for (int i=0; i<w*h; i++)
      dmap[i] = DIJ_MAX;

    int x = MAX(0, MIN(camx / 16, w));
    int y = MAX(0, MIN(camy / 16, h));

    dmap[(y*w)+x] = 0;

    int walkable[10] = {-1};
    walkable[0] = 2; // floor
    walkable[1] = 3; // floor
    walkable[2] = 4; // door
    dijkstra(dmap, tiles, walkable, 0, 0, w, h, w, h);
  }
}

void input(SDL_Event *event)
{
  switch (event->type) {
    case SDL_KEYDOWN: {
      int key = event->key.keysym.scancode;
      keys_down[key] = 1;
      if (event->key.repeat == 0)
        keypressed(key);
      break;
    }
    case SDL_KEYUP: {
      int key = event->key.keysym.scancode;
      keys_down[key] = 0;
      break;
    }
  }
}

int main(int argc, char **argv)
{
  // set random seed
  unsigned long seed = time(NULL);
  srand(seed);
  printf("SEED: %lu\n\n", seed);

  // start main loop, this doesnt ever return
  emscripten_set_main_loop(loop, 0, 1);

  // bye
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 1;
}