#include <cpctelera.h>
#include "ship_sprite.h"

u8 ship_masked[2 * SHIP_W_BYTES * SHIP_H_PIX];

/*
 * Sprite de 12x20 píxels (Mode 0 -> 2 píxels per byte).
 * Reduced from 16px wide to eliminate side transparent columns.
 */
static const char ship_rows[SHIP_H_PIX][15] = {
    ".....WW.....",
    ".....WW.....",
    "....WWWW....",
    "....WWWW....",
    "....W..W....",
    "....BBBB....",
    "....BBBB....",
    "...R.BB.R...",
    "...RRBBRR...",
    "..WRWWWWRW..",
    "..RRWWWWRR..",
    ".WRRWWWWRRW.",
    ".RWWWWWWWWR.",
    "WRRWWWWWWRRW",
    "WRRWW..WWRRW",
    "RRRW....WRRR",
    "WRRW....WRRW",
    "W.RR....RR.W",
    "..RR....RR.."
};

static u8 shipColorFromChar(char c) {
    switch (c) {
        case 'W': return 4;  /* blanc */
        case 'B': return 1;  /* blau  */
        case 'R': return 6;  /* vermell */
        case 'G': return 5;  /* compatibilitat amb l'antic format */
        default:  return 0;  /* transparent */
    }
}

void ship_buildMaskedSprite(void) {
    u8 y, xb;
    u8 i = 0;

    for (y = 0; y < SHIP_H_PIX; ++y) {
        for (xb = 0; xb < SHIP_W_BYTES; ++xb) {
            u8 x0 = xb << 1;
            u8 x1 = x0 + 1;

            u8 p0 = shipColorFromChar(ship_rows[y][x0]);
            u8 p1 = shipColorFromChar(ship_rows[y][x1]);

            /* Byte de màscara: 0 si el píxel és visible, 15 si és transparent */
            ship_masked[i++] = cpct_px2byteM0(p0 ? 0 : 15, p1 ? 0 : 15);

            /* Byte de color */
            ship_masked[i++] = cpct_px2byteM0(p0, p1);
        }
    }
}
