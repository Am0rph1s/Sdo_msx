#include <cpctelera.h>
#include "game_sfx.h"

void psgWrite(u8 reg, u8 value);

#define SFX_NONE        0
#define SFX_EXP_ENEMY   1
#define SFX_EXP_SHIP    2
#define SFX_SHOT        3
#define SFX_BEEP        4
#define SFX_GAMEOVER    5
#define SFX_BOSSWARN    6
#define SFX_LEVELUP     7

static u8 fxB_kind = SFX_NONE;
static u8 fxB_frame = 0;
static u8 fxC_kind = SFX_NONE;
static u8 fxC_frame = 0;
static u8 fxA_kind = SFX_NONE;
static u8 fxA_frame = 0;

static void setToneA(u8 lo, u8 hi, u8 vol) {
   psgWrite(0, lo); psgWrite(1, hi); psgWrite(8, vol);
}
static void setToneB(u8 lo, u8 hi, u8 vol) {
   psgWrite(2, lo); psgWrite(3, hi); psgWrite(9, vol);
}
static void setToneC(u8 lo, u8 hi, u8 vol) {
   psgWrite(4, lo); psgWrite(5, hi); psgWrite(10, vol);
}

static void updateMixer(void) {
   u8 mixer = 0x3F;

   // Game Over uses all 3 channels
   if (fxA_kind == SFX_GAMEOVER || fxB_kind == SFX_GAMEOVER || fxC_kind == SFX_GAMEOVER) {
      mixer &= (u8)~0x01;  // Enable Channel A
      mixer &= (u8)~0x02;  // Enable Channel B
      mixer &= (u8)~0x04;  // Enable Channel C
   } else if (fxA_kind == SFX_LEVELUP) {
      mixer &= (u8)~0x01;
   } else if (fxA_kind == SFX_BEEP || fxB_kind == SFX_BEEP) {
      mixer &= (u8)~0x01;  // Enable Channel A
      mixer &= (u8)~0x02;  // Enable Channel B
   } else {
      // Normal operation - individual sounds
      if (fxA_kind != SFX_NONE) {
         mixer &= (u8)~0x01;
      }

      if (fxB_kind == SFX_EXP_ENEMY) {
         mixer &= (u8)~0x10;
      } else if (fxB_kind == SFX_EXP_SHIP) {
         mixer &= (u8)~0x02;
         mixer &= (u8)~0x10;
      } else if (fxB_kind != SFX_NONE) {
         mixer &= (u8)~0x02;
      }

      if (fxC_kind == SFX_SHOT) {
         mixer &= (u8)~0x04;
      }
   }

   psgWrite(7, mixer);
}

void soundStopAll(void) {
   fxB_kind = SFX_NONE;
   fxB_frame = 0;
   fxC_kind = SFX_NONE;
   fxC_frame = 0;
   fxA_kind = SFX_NONE;
   fxA_frame = 0;

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
void sfxShot(void) { fxC_kind = SFX_SHOT; fxC_frame = 0; }
void sfxEnemyExplosion(void) { fxB_kind = SFX_EXP_ENEMY; fxB_frame = 0; }
void sfxShipExplosion(void) { fxB_kind = SFX_EXP_SHIP; fxB_frame = 0; }
void sfxBeep(void) {
   fxA_kind = SFX_BEEP; fxA_frame = 0;
   fxB_kind = SFX_BEEP; fxB_frame = 0;
}
void sfxGameOver(void) {
   // A minor chord: A4-C5 (trist, 2 channels)
   fxA_kind = SFX_GAMEOVER; fxA_frame = 0;
   fxB_kind = SFX_GAMEOVER; fxB_frame = 0;
}
void sfxLevelUp(void) {
   fxA_kind = SFX_LEVELUP;
   fxA_frame = 0;
}
void sfxBossWarn(void) {
   fxA_kind = SFX_BOSSWARN;
   fxA_frame = 0;
}

static void tickChannelA(void) {
   if (fxA_kind == SFX_BEEP) {
      if (fxA_frame < 2)           setToneA(239, 0, 15);
      else if (fxA_frame == 2)     psgWrite(8, 0);
      else if (fxA_frame < 5)      setToneA(190, 0, 15);
      else { fxA_kind = SFX_NONE; fxA_frame = 0; psgWrite(8, 0); return; }
      ++fxA_frame; return;
   }
   if (fxA_kind == SFX_LEVELUP) {
      /* Arpegi major curt (estil level-up clàssic): C5→E5→G5→C6, ~10 frames */
      if      (fxA_frame < 2)  setToneA(239, 0, 14);
      else if (fxA_frame < 4)  setToneA(190, 0, 14);
      else if (fxA_frame < 6)  setToneA(160, 0, 13);
      else if (fxA_frame < 8)  setToneA(119, 0, 15);
      else if (fxA_frame < 11) setToneA(119, 0, 7);
      else { fxA_kind = SFX_NONE; fxA_frame = 0; psgWrite(8, 0); return; }
      ++fxA_frame; return;
   }
   if (fxA_kind == SFX_GAMEOVER) {
      if (fxA_frame < 24) { setToneA(28, 1, 12); }
      else { fxA_kind = SFX_NONE; fxA_frame = 0; psgWrite(8, 0); return; }
      ++fxA_frame; return;
   }
   if (fxA_kind == SFX_BOSSWARN) {
      if      (fxA_frame < 2) setToneA(95, 1, 13);
      else if (fxA_frame == 2) setToneA(60, 1, 11);
      else if (fxA_frame == 3) psgWrite(8, 0);
      else if (fxA_frame < 6) setToneA(95, 1, 9);
      else { fxA_kind = SFX_NONE; fxA_frame = 0; psgWrite(8, 0); return; }
      ++fxA_frame; return;
   }
   psgWrite(8, 0);
}

static void tickChannelB(void) {
   if (fxB_kind == SFX_EXP_ENEMY) {
      switch (fxB_frame) {
         case 0: psgWrite(6, 0x04); psgWrite(9, 13); break;
         case 1: psgWrite(6, 0x07); psgWrite(9, 11); break;
         case 2: psgWrite(6, 0x0B); psgWrite(9,  9); break;
         case 3: psgWrite(6, 0x10); psgWrite(9,  6); break;
         case 4: psgWrite(6, 0x16); psgWrite(9,  3); break;
         default: fxB_kind = SFX_NONE; fxB_frame = 0; psgWrite(9, 0); return;
      }
      ++fxB_frame; return;
   }
   if (fxB_kind == SFX_EXP_SHIP) {
      switch (fxB_frame) {
         case 0: setToneB(220, 0, 15); psgWrite(6, 0x04); break;
         case 1: setToneB(180, 0, 14); psgWrite(6, 0x06); break;
         case 2: setToneB(150, 0, 12); psgWrite(6, 0x08); break;
         case 3: setToneB(120, 0, 10); psgWrite(6, 0x0C); break;
         case 4: setToneB( 90, 0,  8); psgWrite(6, 0x12); break;
         case 5: setToneB( 70, 0,  6); psgWrite(6, 0x16); break;
         case 6: setToneB( 50, 0,  4); psgWrite(6, 0x1B); break;
         default: fxB_kind = SFX_NONE; fxB_frame = 0; psgWrite(9, 0); return;
      }
      ++fxB_frame; return;
   }
   if (fxB_kind == SFX_BEEP) {
      if (fxB_frame < 2)           setToneB(190, 0, 12);
      else if (fxB_frame == 2)     psgWrite(9, 0);
      else if (fxB_frame < 5)      setToneB(159, 0, 12);
      else { fxB_kind = SFX_NONE; fxB_frame = 0; psgWrite(9, 0); return; }
      ++fxB_frame; return;
   }
   if (fxB_kind == SFX_GAMEOVER) {
      if (fxB_frame < 24) { setToneB(239, 0, 12); }
      else { fxB_kind = SFX_NONE; fxB_frame = 0; psgWrite(9, 0); return; }
      ++fxB_frame; return;
   }
   psgWrite(9, 0);
}

static void tickChannelC(void) {
   if (fxC_kind == SFX_SHOT) {
      if (fxC_frame == 0)          setToneC(160, 0, 9);
      else if (fxC_frame == 1)     setToneC(112, 0, 4);
      else { fxC_kind = SFX_NONE; fxC_frame = 0; psgWrite(10, 0); return; }
      ++fxC_frame; return;
   }
   if (fxC_kind == SFX_GAMEOVER) {
      if (fxC_frame < 24) { setToneC(190, 0, 12); }
      else { fxC_kind = SFX_NONE; fxC_frame = 0; psgWrite(10, 0); return; }
      ++fxC_frame; return;
   }
   psgWrite(10, 0);
}

void soundTick(void) {
   tickChannelA();
   tickChannelB();
   tickChannelC();
   updateMixer();
}
