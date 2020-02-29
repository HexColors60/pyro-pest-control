#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"

extern entity_t *player;

void player_init();

void player_keypress(int button);

void player_mousepress(int button, int mx, int my);

#endif // PLAYER_H