#include <cpctelera.h>
#include "enemy_sprite.h"
#include "enemy_heavy_sprite.h"

static const char enemy_heavy_rows[ENEMY_H_PIX][13] = {
    "............",
    "..CCCCCCCC..",
    ".CCWWWWWWCC.",
    "CCWWWWWWWWCC",
    "COOYYYYYYOOC",
    "OOYYYYYYYYOO",
    "OOYYRRRRYYOO",
    "OOYYRRRRYYOO",
    "OOYYYYYYYYOO",
    ".OOYYYYYYOO.",
    "..OORRRROO..",
    "............"
};

void enemy_heavy_buildMaskedSprite(void) {
    enemy_buildGenericMasked((const char*)enemy_heavy_rows, 13, enemy_heavy_masked);
}
