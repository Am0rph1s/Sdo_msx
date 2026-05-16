#ifndef _ENEMY_SHOT_SPRITE_H_
#define _ENEMY_SHOT_SPRITE_H_

#include <cpctelera.h>

/* Mides dels projectils enemics (1 byte x 4 linies = 2x4 pixels Mode 0) */
#define ENEMYSHOT_W_BYTES 1
#define ENEMYSHOT_H_PIX   4

/* Tipus de projectils enemics */
#define ENEMYSHOT_TYPE_BULLET 0  /* Estàndard - rodó petit */
#define ENEMYSHOT_TYPE_BOMB   1  /* Bomba - més gros */

#define ENEMYSHOT_NUM_TYPES 2

extern u8 enemyshot_masked[ENEMYSHOT_NUM_TYPES][2 * ENEMYSHOT_W_BYTES * ENEMYSHOT_H_PIX];

#endif
