#include <cpctelera.h>
#include "game_sfx.h"

void psgWrite(u8 reg, u8 value);

/* Curt tic UI (menú / redefinir); síncron perquè soundTick() al menú és buit. */
void sfxMenuSelect(void) {
    u8 i;
    soundStopAll();
    psgWrite(7, 0x3E);
    for(i = 0u; i < 6u; ++i) {
        psgWrite(0, (u8)(230u - i * 28u));
        psgWrite(1, 0);
        psgWrite(8, (u8)(13u - i));
        cpct_waitVSYNC();
    }
    soundStopAll();
}

void soundStopAll(void) {
   psgWrite(7, 0x3F);
   psgWrite(8, 0);
   psgWrite(9, 0);
   psgWrite(10, 0);
   psgWrite(0, 0);
   psgWrite(1, 0);
   psgWrite(2, 0);
   psgWrite(3, 0);
   psgWrite(4, 0);
   psgWrite(5, 0);
   psgWrite(6, 0x1F);
}

void soundInit(void) { soundStopAll(); }
void soundTick(void) {}
void sfxEnemyShot(void) {}
