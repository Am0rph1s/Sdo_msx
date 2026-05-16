#pragma once
#include <cpctelera.h>
#define ENEMY_W_BYTES 6
#define ENEMY_H_PIX   12

/* Buffers individuals per cada tipus d'enemic (pre-construits a l'inici) */
extern u8 enemy_masked[2 * ENEMY_W_BYTES * ENEMY_H_PIX];
extern u8 enemy_fast_masked[2 * ENEMY_W_BYTES * ENEMY_H_PIX];
extern u8 enemy_heavy_masked[2 * ENEMY_W_BYTES * ENEMY_H_PIX];
extern u8 enemy_diver_masked[2 * ENEMY_W_BYTES * ENEMY_H_PIX];
extern u8 enemy_bomber_masked[2 * ENEMY_W_BYTES * ENEMY_H_PIX];

/* Funcions de construcció de sprites */
void enemy_buildGenericMasked(const char* rows, u8 stride, u8* out);
void enemy_buildGenericMaskedEx(const char* rows, u8 stride, u8 w_bytes, u8 h_pix, u8* out);
void enemy_buildMaskedSprite(void);
