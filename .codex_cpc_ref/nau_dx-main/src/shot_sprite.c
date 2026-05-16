#include <cpctelera.h>
#include "shot_sprite.h"

u8 shot_masked[2 * SHOT_W_BYTES * SHOT_H_PIX];

/*
 * Sprite del laser del jugador: 2x6 pixels (Mode 0 -> 1 byte ample)
 * Format: 1 píxel visible, 1 transparent per a poder centrar
 * "W" = blanc (color del laser)
 * "." = transparent
 */
static const char shot_rows[SHOT_H_PIX][4] = {
    "WW\0",  /* 2 píxels blancs */
    "WW\0",
    "WW\0",
    "WW\0",
    "WW\0",
    "WW\0"
};

static u8 shotColorFromChar(char c) {
    switch (c) {
        case 'W': return 6;  /* groc/vermell brillant per laser */
        default:  return 0;  /* transparent */
    }
}

void shot_buildMaskedSprite(void) {
    u8 y;
    u8 i = 0;

    for (y = 0; y < SHOT_H_PIX; ++y) {
        u8 p0 = shotColorFromChar(shot_rows[y][0]);  /* W */
        u8 p1 = shotColorFromChar(shot_rows[y][1]);  /* W */

        /* Byte: màscara + color (píxels 0-1) */
        shot_masked[i++] = cpct_px2byteM0(p0 ? 0 : 15, p1 ? 0 : 15);
        shot_masked[i++] = cpct_px2byteM0(p0, p1);
    }
}

u8* shot_getSprite(u8 type) {
    (void)type;  /* per ara només tenim un tipus */
    return shot_masked;
}
