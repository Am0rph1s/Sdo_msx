#include <cpctelera.h>
#include "enemy_sprite.h"
#include "enemy_diver_sprite.h"

static const char enemy_diver_rows[ENEMY_H_PIX][14] = {
    "............",
    "....CC......",
    "...CCWWCC....",
    "..CCWWWWCC...",
    ".CCWWWWWWCC..",
    "CCWWWWWWWWCC.",
    "CCOOYYYYOOCC.",
    "CCYYRRRRYYCC.",
    "CCYYYYYYYYCC.",
    ".CCYYYYYYCC..",
    "..CCYYYYCC...",
    "....RRRR...."
};

void enemy_diver_buildMaskedSprite(void) {
    enemy_buildGenericMasked((const char*)enemy_diver_rows, 14, enemy_diver_masked);
}
