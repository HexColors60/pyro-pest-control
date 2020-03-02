#ifndef MATH_H
#define MATH_H

#ifndef M_PI
  #define M_PI (3.14159265358979323846264338327950288)
#endif

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define DIJ_MAX 99999

#include <math.h>
#include <stdlib.h>

typedef struct {
  int x, y;
} ivec2_t;

typedef struct {
  float x, y;
} fvec2_t;

static int roll(int n) {
  return (rand() % n) + 1;
}

static int adjacent[4][2] = {
  {0, -1},
  {0,  1},
  {-1, 0},
  {1,  0}
};

static int around[8][2] = {
  {-1,-1},
  {0, -1},
  {1, -1},
  {1,  0},
  {1,  1},
  {0,  1},
  {-1, 1},
  {-1, 0},
};

static int around_adjacent[8][2] = {
  {-1, 0},
  {1,  0},
  {0, -1},
  {0,  1},
  {-1,-1},
  {1, -1},
  {1,  1},
  {-1, 1}
};

static float lerp(float a, float b, float f) {
  return a + f * (b - a);
}

/**
 * [picktile picks a tile from an atlas via linear index]
 * @param t     [output position]
 * @param index [tile index]
 * @param w     [width of atlas]
 * @param tw    [tile width]
 * @param th    [tile height]
 */
static void picktile(ivec2_t *t, int index, int w, int tw, int th) {
  t->x = (index - ((index / w) * w)) * tw;
  t->y = (index / w) * th;
}

static int is_wall(int walls[32], int tile) {
  for (int i=0; i<32; i++)
    if (tile == walls[i])
      return 1;

  return 0;
}

static void dijkstra(int *arr, char *tiles, int walls[32], int x0, int y0, int x1, int y1, int w, int h) {
  x0 = MAX(0, MIN(x0, w));
  x1 = MAX(0, MIN(x1, w));
  y0 = MAX(0, MIN(y0, h));
  y1 = MAX(0, MIN(y1, h));

  if (x0 == x1 || y0 == y1)
    return;

  int changed = 1;
  while (changed) {
    changed = 0;
    for (int y=y0; y<y1; y++) {
      for (int x=x0; x<x1; x++) {
        int index = (y*w)+x;
        int tile = tiles[index];

        // make sure its a walkable tile
        if (is_wall(walls, tile))
          continue;

        // find the lowest value surrounding tile
        int lowest=DIJ_MAX;
        for (int i=0; i<8; i++) {
          int tx = MAX(0, MIN(x + around[i][0], w));
          int ty = MAX(0, MIN(y + around[i][1], h));
          if (tx == x && ty == y)
            continue;

          // make sure its walkable
          if (is_wall(walls, tiles[(ty*w)+tx]))
            continue;

          // is it the lowest value tile?
          int value = arr[(ty*w)+tx];
          if (value < lowest)
            lowest = value;
        }

        if (lowest < DIJ_MAX && lowest < arr[index]-1) {
          arr[index] = lowest+1;
          changed++;
        }
      }
    }
  }
}

static int line(int x0, int y0, int x1, int y1, int w, int h, char *arr, int walls[32], ivec2_t positions[512]) {
  int dx = abs(x1-x0);
  int dy = abs(y1-y0);

  int sx = x0 < x1 ? 1 : -1;
  int sy = y0 < y1 ? 1 : -1;

  int err2, err = (dx > dy ? dx : -dy) / 2;

  int pi = 0;

  for (;;) {
    if (x0 < 0 || y0 < 0 || x0 >= w || y0 >= h)
      break;
    int index = (y0*w)+x0;
    positions[pi].x = x0, positions[pi++].y = y0;
    if ((x0 == x1 && y0 == y1) || is_wall(walls, arr[index]))
      break;
    err2 = err;
    if (err2 > -dx) {
      err -= dy;
      x0 += sx;
    }
    if (err2 < dy) {
      err += dx;
      y0 += sy;
    }
  }

  return pi;
}

#endif // MATH_H