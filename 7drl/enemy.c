#include "enemy.h"
#include "player.h"
#include "text.h"
#include "main.h"
#include "math.h"
#include "spell.h"
#include "generator.h"

void skeleton_attack(entity_t *e, int first, int dist);
void goblin_attack(entity_t *e, int first, int dist);
void rat_attack(entity_t *e, int first, int dist);
void slime_attack(entity_t *e, int first, int dist);
void spider_attack(entity_t *e, int first, int dist);
void shamen_attack(entity_t *e, int first, int dist);
void bslime_attack(entity_t *e, int first, int dist);

void slime_die(entity_t *e);

void enemy_new(int enemy, int x, int y)
{
  // initialize an entity for the player
  entity_t *e = entity_new();
  if (!e)
    return;
  e->update = NULL;
  e->onhit  = enemy_hit;
  e->alive  = 1;
  e->speed  = 1;
  e->pos.x  = x;
  e->pos.y  = y;
  e->to.x   = x;
  e->to.y   = y;
  e->type   = enemy;
  e->dmap   = dmap_to_player;
  e->distance = -DIJ_MAX;

  // tile 16
  int tile = 0;

  switch(e->type) {
    case ENEMY_TYPE_SKELETON: {
      tile      = ENEMY_TILE_SKELETON;
      e->speed  = 1;
      e->update = enemy_update;
      e->attack = skeleton_attack;
      e->hp     = 10;
      e->lowhp  = 3;
      e->resist = 1;
      break;
    }
    case ENEMY_TYPE_GOBLIN: {
      tile      = ENEMY_TILE_GOBLIN;
      e->speed  = 2;
      e->update = enemy_update;
      e->attack = goblin_attack;
      e->hp     = 15;
      e->lowhp  = 5;
      e->resist = 0;
      break;
    }
    case ENEMY_TYPE_RAT: {
      tile      = ENEMY_TILE_RAT;
      e->speed  = 5;
      e->update = enemy_update;
      e->attack = rat_attack;
      e->hp     = 5;
      e->lowhp  = 0;
      e->resist = 0;
      break;
    }
    case ENEMY_TYPE_SLIME: {
      tile      = ENEMY_TILE_SLIME;
      e->speed  = 1;
      e->update = enemy_update;
      e->attack = slime_attack;
      e->ondeath= slime_die;
      e->hp     = 8;
      e->lowhp  = 0;
      e->resist = -1;
      break;
    }
    case ENEMY_TYPE_SPIDER: {
      tile      = ENEMY_TILE_SPIDER;
      e->speed  = 1;
      e->update = enemy_update;
      e->attack = spider_attack;
      e->hp     = 8;
      e->resist = -1;
      e->lowhp  = 3;
      e->distance = 3;
      break;
    }
    case ENEMY_TYPE_SHAMEN: {
      tile      = ENEMY_TILE_SHAMEN;
      e->speed  = 2;
      e->update = enemy_update;
      e->attack = shamen_attack;
      e->hp     = 20;
      e->resist = 1;
      e->lowhp  = 3;
      e->distance = 5;
      break;
    }
    case ENEMY_TYPE_BSLIME: {
      tile      = ENEMY_TILE_BSLIME;
      e->speed  = 2;
      e->update = enemy_update;
      e->attack = bslime_attack;
      e->hp     = 3;
      e->resist = 0;
      e->lowhp  = 0;
      break;
    }
  }

  e->hp_max = e->hp;
  e->idistance = e->distance;
  entity_set_tile(e, 0, tile);
}

void enemy_hit(entity_t *e, int damage, int type)
{
  char buff[512];
  switch(e->type) {
    case ENEMY_TYPE_SKELETON: {
      text_log_add("Skeleton resists some damage due to lack of flesh");
      sprintf(buff, "Skeleton suffers %i damage", damage);
      break;
    }
    case ENEMY_TYPE_GOBLIN: {
      sprintf(buff, "Goblin suffers %i damage", damage);
      break;
    }
    case ENEMY_TYPE_RAT: {
      sprintf(buff, "Rat suffers %i damage", damage);
      break;
    }
    case ENEMY_TYPE_SLIME: {
      text_log_add("Slime ignites and takes increased damage");
      sprintf(buff, "Slime suffers %i damage", damage);
      break;
    }
    case ENEMY_TYPE_SPIDER: {
      sprintf(buff, "Spider suffers %i damage", damage);
      break;
    }
    case ENEMY_TYPE_SHAMEN: {
      sprintf(buff, "Shamen suffers %i damage", damage);
      break;
    }
    case ENEMY_TYPE_BSLIME: {
      sprintf(buff, "Baby slime suffers %i damage", damage);
      break;
    }
  }

  e->aggro = 1;

  text_log_add(buff);
}

void enemy_update(entity_t *e)
{
  e->last_attack--;

  if (e->aggro)
    e->walking = 1;

  // set fleeing map if hurt
  if (e->hp <= e->lowhp) {
    e->dmap = dmap_from_player;
    e->distance = -DIJ_MAX;
    e->walking = 1;
  } else {
    e->dmap = dmap_to_player;
    e->distance = e->idistance;
  }

  if (!player->alive)
    return;

  // check LoS to player
  // collidable tiles
  int walls[32] = {-1};
    for (int i=0; i<SOLID_COUNT; i++)
      walls[i] = solid[i];
  walls[SOLID_COUNT-1] = TILE_DOOR_CLOSED;

  // raycast
  char *tiles = level.layers[level.layer].tiles;
  ivec2_t pos[512] = {0};

  int count = line(e->to.x, e->to.y, player->to.x, player->to.y, level_width, level_height, tiles, walls, pos);

  int d = hypot(e->to.x - player->to.x, e->to.y - player->to.y);

  // heal
  if (d > 15 && e->hp < e->hp_max) {
    e->hp++;
  }

  if (pos[count-1].x == player->to.x && pos[count-1].y == player->to.y) {
    if (e->attack && !e->aggro)
      e->attack(e, 1, d);
    else if (e->attack)
      e->attack(e, 0, d);
    e->aggro = 1;
  }
}

void skeleton_attack(entity_t *e, int first, int dist)
{
  if (first)
    text_log_add("The skeleton notices you");

  if (e->aggro && e->last_attack <= 0 && dist <= 1) {
    player->hp -= 2;
    text_log_add("The skeleton hits you for 2 damage");
    e->last_attack = 2;
  }
}

void goblin_attack(entity_t *e, int first, int dist)
{
  if (first)
    text_log_add("The goblin notices you");

  if (e->aggro && e->last_attack <= 0 && dist <= 1) {
    player->hp -= 2;
    text_log_add("The goblin hits you for 2 damage");
    e->last_attack = 2;
  }
}

void rat_attack(entity_t *e, int first, int dist)
{
  if (first)
    text_log_add("The rat notices you");

  if (e->aggro && e->last_attack <= 0 && dist <= 1) {
    player->hp -= 2;
    text_log_add("The rat hits you for 1 damage");
    e->last_attack = 1;
  }
}

void slime_attack(entity_t *e, int first, int dist)
{
  if (first)
    text_log_add("The slime notices you");

  if (e->aggro && e->last_attack <= 0 && dist <= 1) {
    player->hp -= 3;
    text_log_add("The slime hits you for 3 damage");
    e->last_attack = 3;
  }
}

void spider_attack(entity_t *e, int first, int dist)
{

  if (first)
    text_log_add("The spider notices you");

  if (e->last_attack <= 0 && dist <= e->distance + 5) {
    int errx = 0;
    int erry = 0;
    if (roll(5) >= 3) {
      errx = -2 + roll(3);
      erry = -2 + roll(3);
    }
    spell_new(SPELL_WEB, e->to.x, e->to.y, player->to.x + errx, player->to.y + erry);
    e->last_attack = roll(2);
  }
}

void shamen_attack(entity_t *e, int first, int dist)
{

  if (first)
    text_log_add("The shamen notices you");

  if (e->aggro && e->last_attack <= 0 && dist <= e->distance + 5) {
    int errx = 0;
    int erry = 0;
    if (roll(2) == 1) {
      errx = -2 + roll(3);
      erry = -2 + roll(3);
    }
    spell_new(SPELL_SPIRIT, e->to.x, e->to.y, player->to.x + errx, player->to.y+ erry);
    e->last_attack = 4 + roll(3);
  }
}

void bslime_attack(entity_t *e, int first, int dist)
{
  if (first)
    text_log_add("The baby slime notices you");

  if (e->aggro && e->last_attack <= 0 && dist <= 1) {
    player->hp -= 1;
    text_log_add("The baby slime hits you for 1 damage");
    e->last_attack = 0;
  }
}

void slime_die(entity_t *e)
{
  text_log_add("The slime splits into multiple smaller slimes");

  for (int j=0; j<8; j++) {
    int tx = MAX(0, MIN(e->to.x + around[j][0], level_width));
    int ty = MAX(0, MIN(e->to.y + around[j][1], level_height));
    int tile = level.layers[level.layer].tiles[(ty*level_width)+tx];

    if (tx == e->to.x && ty == e->to.y)
      continue;

    if (tile != TILE_WOOD_FLOOR && tile != TILE_STONE_FLOOR)
      continue;

    enemy_new(ENEMY_TYPE_BSLIME, tx, ty);
  }
}