#include <cpctelera.h>
#include "boss_sprite.h"
#include "enemy_sprite.h"

u8 boss_masked[2 * BOSS_W_BYTES * BOSS_H_PIX];

/* Mothership boss - 16x16 pixels (8 bytes wide, Mode 0) */
static const char boss_rows[BOSS_H_PIX][17] = {
    "................",
    "....CCCCCCCC....",
    "..CCWWWWWWWWCC..",
    ".CWWWWWWWWWWWWC.",
    "COWWRRRRRRRRWWOC",
    "OOORRRRRRRRRROOO",
    "OOYYRRRRRRRRYYOO",
    "OYYYRRRRRRRRYYYO",
    "OYYYRRRRRRRRYYYO",
    "OOYYRRRRRRRRYYOO",
    "OOORRRRRRRRRROOO",
    ".OOORRRRRRRROOO.",
    "..OORRRRRRRROO..",
    "...OORRRRRROO...",
    ".....RRRRRR.....",
    "................"
};

void boss_buildMaskedSprite(void) {
    enemy_buildGenericMaskedEx((const char*)boss_rows, 17, BOSS_W_BYTES, BOSS_H_PIX, boss_masked);
}
