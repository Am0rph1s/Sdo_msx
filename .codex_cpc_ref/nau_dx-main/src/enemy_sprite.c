#include <cpctelera.h>
#include "enemy_sprite.h"

/* Buffers individuals per cada tipus d'enemic */
u8 enemy_masked[2 * ENEMY_W_BYTES * ENEMY_H_PIX];
u8 enemy_fast_masked[2 * ENEMY_W_BYTES * ENEMY_H_PIX];
u8 enemy_heavy_masked[2 * ENEMY_W_BYTES * ENEMY_H_PIX];
u8 enemy_diver_masked[2 * ENEMY_W_BYTES * ENEMY_H_PIX];
u8 enemy_bomber_masked[2 * ENEMY_W_BYTES * ENEMY_H_PIX];

static const char enemy_rows[ENEMY_H_PIX][13] = {
   "............",
   "...CCCCCC...",
   "..CCWWWWCC..",
   ".CCWWWWWWCC.",
   "CCOOYYYYOOCC",
   "COOYYYYYYOOC",
   "OOYYRRRRYYOO",
   "OOYYYYYYYYOO",
   ".OOYYYYYYOO.",
   "..OOYYYYOO..",
   "....RRRR....",
   "............"
};

static u8 enemyColorFromChar(char c) {
   switch (c) {
      case 'C': return 1;
      case 'W': return 7;
      case 'Y': return 2;
      case 'O': return 9;
      case 'R': return 6;
      default:  return 0;
   }
}

void enemy_buildGenericMaskedEx(const char* rows, u8 stride, u8 w_bytes, u8 h_pix, u8* out) {
   u8 y, xb, i = 0;
   for (y = 0; y < h_pix; ++y) {
      const char* row = rows + y * stride;
      for (xb = 0; xb < w_bytes; ++xb) {
         u8 x0 = xb << 1;
         char c0 = row[x0], c1 = row[x0 + 1];
         out[i++] = cpct_px2byteM0((c0 == '.') ? 15 : 0, (c1 == '.') ? 15 : 0);
         out[i++] = cpct_px2byteM0(enemyColorFromChar(c0), enemyColorFromChar(c1));
      }
   }
}

void enemy_buildGenericMasked(const char* rows, u8 stride, u8* out) {
   enemy_buildGenericMaskedEx(rows, stride, ENEMY_W_BYTES, ENEMY_H_PIX, out);
}

/* Builder específic per l'enemic bàsic (pre-construït a l'inici) */
void enemy_buildMaskedSprite(void) {
   enemy_buildGenericMasked((const char*)enemy_rows, 13, enemy_masked);
}
