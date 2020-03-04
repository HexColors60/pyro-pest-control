#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"
#include <inttypes.h>

extern entity_t *player;
extern int light_map_width, light_map_height;
extern uint8_t *light_map;
extern int *dmap_to_player, *dmap_from_player, *dmap_to_mouse;

void player_init(int x, int y);

void player_light();

void player_fire();

int player_update();

void player_render();

void player_keypress(int button);

void player_mousepress(int button, int mx, int my);

#endif // PLAYER_H