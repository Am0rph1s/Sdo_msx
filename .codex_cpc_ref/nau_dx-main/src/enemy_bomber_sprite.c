#include <cpctelera.h>
#include "enemy_sprite.h"
#include "enemy_bomber_sprite.h"

static const char enemy_bomber_rows[ENEMY_H_PIX][13] = {
    "............",
    "..CCCCCCCC..",
    ".CCWWWWWWCC.",
    "CCWWWWWWWWCC",
    "COOYYYYYYOOC",
    "OOYYRRRRYYOO",
    "OOYYRRRRYYOO",
    "OOYYYYYYYYOO",
    ".OOYYYYYYOO.",
    "..OOOOOOOO..",
    "..OOOOOOOO..",
    "....RRRR...."
};

void enemy_bomber_buildMaskedSprite(void) {
    enemy_buildGenericMasked((const char*)enemy_bomber_rows, 13, enemy_bomber_masked);
}
