#pragma once
#include <cpctelera.h>

#define BOSS_W_BYTES 8
#define BOSS_H_PIX   16

extern u8 boss_masked[2 * BOSS_W_BYTES * BOSS_H_PIX];
void boss_buildMaskedSprite(void);
