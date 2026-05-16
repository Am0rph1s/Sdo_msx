#ifndef _SHOT_SPRITE_H_
#define _SHOT_SPRITE_H_

#include <cpctelera.h>

/* Mides del laser del jugador (1 byte x 6 linies = 2x6 pixels Mode 0) */
#define SHOT_W_BYTES 1
#define SHOT_H_PIX   6

/* Tipus de shots del jugador */
#define SHOT_TYPE_SINGLE 0

/* Array del sprite masked (mida: 2 * SHOT_W_BYTES * SHOT_H_PIX) */
extern u8 shot_masked[2 * SHOT_W_BYTES * SHOT_H_PIX];

/* Construeix el sprite masked */
void shot_buildMaskedSprite(void);

/* Retorna punter al sprite segons tipus */
u8* shot_getSprite(u8 type);

#endif
