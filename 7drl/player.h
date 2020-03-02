#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"
#include <inttypes.h>

extern entity_t *player;
extern int light_map_width, light_map_height;
extern uint8_t *light_map;

void player_init();

void player_update(entity_t *e);

void player_render();

void player_keypress(int button);

void player_mousepress(int button, int mx, int my);

#endif // PLAYER_H