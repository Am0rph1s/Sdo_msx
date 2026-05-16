#pragma once
#include <cpctelera.h>

#define SHIP_W_BYTES 6
#define SHIP_H_PIX   19

extern u8 ship_masked[2 * SHIP_W_BYTES * SHIP_H_PIX];
void ship_buildMaskedSprite(void);
