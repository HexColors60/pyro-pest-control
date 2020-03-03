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
#include "spell.h"

// sdl vars
SDL_Window *window;
SDL_Renderer *renderer;

// camera vars
float camx = 512.0f, camy = 512.0f;
float cx = 0.0f, cy = 0.0f;
int mx, my;

// runs an update pass on all entites when set
int update = 0;

// delta time vars
const double phys_delta_time   = 1.0 / 60.0;
const double slowest_frame     = 1.0 / 15.0;
double delta_time, tick        = 0.0;
double last_frame_time         = 0.0;
int init = 0;

// level vars
SDL_Texture *tex_tiles, *tex_map, *tex_fov;
int tiles_tex_width = 0, tiles_tex_height = 0;
level_t level;
level_texture_t level_textures;
int level_width = 0, level_height = 0;

// input
uint8_t keys_down[SDL_NUM_SCANCODES];
void keypressed(int key);
void input(SDL_Event *event);

void loop()
{
  /*-----------------------------------------/
  /---------------- INIT STUFF --------------/
  /-----------------------------------------*/
  if (!init) {
    init++;

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
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    // dt stuff
    last_frame_time = SDL_GetPerformanceCounter();

    // load textures
    tex_tiles = texture_load("tiles.png", &tiles_tex_width, &tiles_tex_height);
    SDL_SetTextureBlendMode(tex_tiles, SDL_BLENDMODE_BLEND);

    // screen render target
    tex_map = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, window_width, game_height);

    // field of view light map
    tex_fov = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, (window_width / tile_width) + 2, (game_height / tile_height) + 2);
    SDL_SetTextureBlendMode(tex_fov, SDL_BLENDMODE_BLEND);

    for (int i=0; i<CHUNK_COUNT; i++) {
      level_textures.chunks[i].tex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, CHUNK_WIDTH, CHUNK_HEIGHT);
      level_textures.chunks[i].update     = 1;
      level_textures.chunks[i].tile_count = 0;
      level_textures.chunks[i].count      = 0;
    }

    // generate the first 3 layers
    level.layer = 0;
    level.max   = 10 + roll(10);
    for (int i=0; i<2; i++) {
      while (!gen(&level.layers[i])) {};
    }
    level_width  = level.layers[level.layer].width;
    level_height = level.layers[level.layer].height;

    // init entity stuff
    entity_init();

    // init player
    player_init(level.layers[level.layer].sx, level.layers[level.layer].sy);

    // init spell system
    spell_init();

    // setup vga atlas etc
    text_init();
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
        break;
      }
    }
  }

  // update mouse coords
  if (SDL_GetRelativeMouseMode())
    SDL_GetRelativeMouseState(&mx, &my);
  else
    SDL_GetMouseState(&mx, &my);

  if (player != NULL && player->alive) {
    camx += ((player->to.x*tile_width) - camx) * 8.0f * delta_time;
    camy += ((player->to.y*tile_height) - camy) * 8.0f * delta_time;
  }

  // repeat until all are done updating
  if (update && tick > 1.0f / 16.0f) {
    if (!spell_ready()) {
      update = player_update();
      entity_update();
      spell_update();
      tick = 0.0f;
    }
  }
  entity_update_render();
  spell_update_render();
  /*----------------------------------------*/



  /*-----------------------------------------/
  /---------------- TILE MAP CHUNK RENDER ---/
  /-----------------------------------------*/
  for (int iy=0; iy<CHUNK_STRIDE; iy++) {
    for (int ix=0; ix<CHUNK_STRIDE; ix++) {
      int index = (iy*CHUNK_STRIDE)+ix;

      if (!level_textures.chunks[index].update)
        continue;

      // num of tiles that need updating
      int *updated = level_textures.chunks[index].updated;
      int count    = level_textures.chunks[index].count;

      level_textures.chunks[index].update = 0;

      if (!count)
        level_textures.chunks[index].tile_count = 0;

      SDL_SetRenderTarget(renderer, level_textures.chunks[index].tex);
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

      if (!init)
        SDL_RenderClear(renderer);

      SDL_Rect ra, rb;
      int w  = level_width;
      int h  = level_height;
      int tw = tiles_tex_width  / rtile_width;
      int fromx = (ix*CHUNK_WIDTH) / tile_width;
      int fromy = (iy*CHUNK_HEIGHT) / tile_height;
      int tox = fromx + (CHUNK_WIDTH / tile_width);
      int toy = fromy + (CHUNK_HEIGHT / tile_height);
      for (int y=fromy; y<toy; y++) {
        for (int x=fromx; x<tox; x++) {
          // current tile
          int tindex = (y*w)+x;
          int tile = 0;
          if (x >= 0 && x < w && y >= 0 && y < h)
            tile = level.layers[level.layer].tiles[tindex];

          if (!tile)
            continue;

          // check if anything needs updating
          if (count) {
            int found = 0;
            for (int i=0; i<count; i++) {
              if (tindex == updated[i]) {
                found = 1;
                break;
              }
            }

            // no need to update it
            if (!found)
              continue;
          }

          // position of tile in texture
          ra.x = (tile - ((tile / tw) * tw)) * rtile_width;
          ra.y = (tile / tw) * rtile_height;
          ra.w = rtile_width;
          ra.h = rtile_height;

          // position of tile in map
          rb.x = (x - fromx) * tile_width;
          rb.y = (y - fromy) * tile_height;
          rb.w = tile_width;
          rb.h = tile_height;

          SDL_RenderCopy(renderer, tex_tiles, &ra, &rb);

          if (!count)
            level_textures.chunks[index].tile_count++;
        }
      }

      level_textures.chunks[index].count = 0;
    }
  }
  /*----------------------------------------*/



  /*-----------------------------------------/
  /---------------- CHUNKS TO TEXTURE -------/
  /-----------------------------------------*/
  SDL_SetRenderTarget(renderer, tex_map);
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  SDL_Rect r;
  cx = (camx + tile_width / 2) - (window_width / 2);
  cy = (camy + tile_height / 2) - (game_height / 2);
  int fromx = floor(cx / (float)CHUNK_WIDTH);
  int fromy = floor(cy / (float)CHUNK_HEIGHT);
  int tox = fromx + ceil((float)(window_width + CHUNK_WIDTH) / (float)CHUNK_WIDTH);
  int toy = fromy + ceil((float)(game_height + CHUNK_HEIGHT) / (float)CHUNK_HEIGHT);
  for (int y=fromy; y<toy; y++) {
    for (int x=fromx; x<tox; x++) {
      if (x < 0 || y < 0 || x >= CHUNK_STRIDE || y >= CHUNK_STRIDE)
        continue;

      int index = (y*CHUNK_STRIDE)+x;

      if (!level_textures.chunks[index].tile_count)
        continue;

      r.x = (int)floor(((x * CHUNK_WIDTH) - (cx)));
      r.y = (int)floor(((y * CHUNK_HEIGHT) - (cy)));
      r.w = (int)CHUNK_WIDTH;
      r.h = (int)CHUNK_HEIGHT;

      SDL_RenderCopy(renderer, level_textures.chunks[index].tex, NULL, &r);
    }
  }
  /*----------------------------------------*/



  /*-----------------------------------------/
  /---------------- FOV LIGHT MAP -----------/
  /-----------------------------------------*/
  SDL_SetRenderTarget(renderer, tex_fov);

  uint8_t *pixels = NULL;
  int pitch = 0;
  SDL_LockTexture(tex_fov, NULL, (void**)&pixels, &pitch);

  fromx = floor(cx / tile_width);
  fromy = floor(cy / tile_height);
  tox = MAX(0, MIN(fromx + (window_width / tile_width), level_width));
  toy = MAX(0, MIN(fromy + (game_height / tile_height) + 2, level_height));
  int count = (MIN((window_width / tile_width) + 2, level_width-fromx)) * 4;
  for (int y=fromy; y<toy; y++) {
    if (y < 0 || y > level_height)
      continue;
    int index = ((y*level_width)+fromx)*4;
    memcpy(&pixels[(y-fromy)*pitch], &light_map[index], count);
  }

  SDL_UnlockTexture(tex_fov);
  /*----------------------------------------*/



  /*-----------------------------------------/
  /---------------- RENDER ------------------/
  /-----------------------------------------*/
  // render to default target
  SDL_SetRenderTarget(renderer, NULL);
  SDL_RenderClear(renderer);

  // render the level
  r.x = 0, r.y = 0, r.w = window_width, r.h = game_height;
  SDL_RenderCopy(renderer, tex_map, NULL, &r);

  // render entities
  entity_render();

  player_render();

  spell_render();

  float rcx = (cx / tile_width);
  float rcy = (cy / tile_height);
  r.x = ((floor(rcx) - rcx) * tile_width) - 1;
  r.y = ((floor(rcy) - rcy) * tile_height) - 1;
  r.w = ((window_width / tile_width) + 2) * tile_width;
  r.h = ((game_height / tile_height) + 2) * tile_height;
  SDL_RenderCopy(renderer, tex_fov, NULL, &r);

  // render text box
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

  player_keypress(key);
}

void mousepress(int key)
{
  player_mousepress(key, mx, my);
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
    case SDL_MOUSEBUTTONDOWN: {
      mousepress(event->button.button);
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