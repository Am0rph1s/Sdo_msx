#include <cpctelera.h>
#include "enemy_sprite.h"
#include "enemy_fast_sprite.h"

static const char enemy_fast_rows[ENEMY_H_PIX][13] = {
    "............",
    "....CCCC....",
    "...CCWWCC...",
    "..CCWWWWCC..",
    "...WWYYWW...",
    "..WWYYYYWW..",
    ".WWYYRRYYWW.",
    "..WWYYYYWW..",
    "...WYYYYW...",
    "...OOYYOO...",
    "....RRRR....",
    "............"
};

void enemy_fast_buildMaskedSprite(void) {
    enemy_buildGenericMasked((const char*)enemy_fast_rows, 13, enemy_fast_masked);
}
