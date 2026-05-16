#include <cpctelera.h>
#include "game_sfx.h"

void psgWrite(u8 reg, u8 value);

/* Lightweight gameplay SFX engine:
 * - Channel A: melodic cues (beep, level up, game over, boss warn)
 * - Channel B: explosions (tone + optional noise)
 * - Channel C: player shot
 */

#define FX_NONE       0
#define FX_BEEP       1
#define FX_LEVELUP    2
#define FX_GAMEOVER   3
#define FX_BOSSWARN   4

#define FXB_NONE       0
#define FXB_EXP_ENEMY  1
#define FXB_EXP_SHIP   2
#define FXB_ENEMY_SHOT 3

#define FXC_NONE      0
#define FXC_SHOT      1

static u8 fxA_kind, fxA_frame;
static u8 fxB_kind, fxB_frame;
static u8 fxC_kind, fxC_frame;
/* Desfasament per tir: cada dispar sona lleugerament diferent (arcade). */
static u8 g_shot_detune;

static void setToneA(u8 lo, u8 hi, u8 vol) {
   psgWrite(0, lo); psgWrite(1, hi); psgWrite(8, vol);
}
static void setToneB(u8 lo, u8 hi, u8 vol) {
   psgWrite(2, lo); psgWrite(3, hi); psgWrite(9, vol);
}
static void setToneC(u8 lo, u8 hi, u8 vol) {
   psgWrite(4, lo); psgWrite(5, hi); psgWrite(10, vol);
}

void soundStopAll(void) {
   fxA_kind = FX_NONE; fxA_frame = 0;
   fxB_kind = FXB_NONE; fxB_frame = 0;
   fxC_kind = FXC_NONE; fxC_frame = 0;
   psgWrite(7, 0x3F);
   psgWrite(8, 0);
   psgWrite(9, 0);
   psgWrite(10, 0);
   psgWrite(6, 0x1F);
}

void soundInit(void) { soundStopAll(); }

void sfxShot(void) {
    fxC_kind = FXC_SHOT;
    fxC_frame = 0;
    g_shot_detune = (u8)(g_shot_detune + 7u) & 15u;
}
void sfxEnemyShot(void) {
    if(fxB_kind == FXB_EXP_ENEMY || fxB_kind == FXB_EXP_SHIP) return;
    fxB_kind = FXB_ENEMY_SHOT;
    fxB_frame = 0;
}
void sfxEnemyExplosion(void) { fxB_kind = FXB_EXP_ENEMY; fxB_frame = 0; }
void sfxShipExplosion(void) { fxB_kind = FXB_EXP_SHIP; fxB_frame = 0; }
void sfxBeep(void) { fxA_kind = FX_BEEP; fxA_frame = 0; }
void sfxGameOver(void) { fxA_kind = FX_GAMEOVER; fxA_frame = 0; }
void sfxLevelUp(void) { fxA_kind = FX_LEVELUP; fxA_frame = 0; }
void sfxBossWarn(void) { fxA_kind = FX_BOSSWARN; fxA_frame = 0; }

void sfxMenuSelect(void) { sfxBeep(); }

static void tickA(void) {
   switch(fxA_kind) {
      case FX_BEEP:
         if(fxA_frame < 2u) setToneA(239, 0, 14);
         else if(fxA_frame == 2u) psgWrite(8, 0);
         else if(fxA_frame < 5u) setToneA(190, 0, 14);
         else { fxA_kind = FX_NONE; fxA_frame = 0; psgWrite(8, 0); return; }
         ++fxA_frame;
         return;
      case FX_LEVELUP:
         if(fxA_frame < 2u) setToneA(239, 0, 14);
         else if(fxA_frame < 4u) setToneA(190, 0, 13);
         else if(fxA_frame < 6u) setToneA(160, 0, 13);
         else if(fxA_frame < 8u) setToneA(119, 0, 14);
         else { fxA_kind = FX_NONE; fxA_frame = 0; psgWrite(8, 0); return; }
         ++fxA_frame;
         return;
      case FX_GAMEOVER:
         if(fxA_frame < 24u) setToneA(28, 1, 12);
         else { fxA_kind = FX_NONE; fxA_frame = 0; psgWrite(8, 0); return; }
         ++fxA_frame;
         return;
      case FX_BOSSWARN:
         /* Amenaça més fosca: descens greu en dos passos + cua curta. */
         if(fxA_frame < 3u) setToneA(120, 1, 14);
         else if(fxA_frame < 6u) setToneA(95, 1, 13);
         else if(fxA_frame < 9u) setToneA(72, 1, 11);
         else if(fxA_frame < 11u) setToneA(60, 1, 8);
         else { fxA_kind = FX_NONE; fxA_frame = 0; psgWrite(8, 0); return; }
         ++fxA_frame;
         return;
      default:
         psgWrite(8, 0);
         return;
   }
}

static void tickB(void) {
   switch(fxB_kind) {
      case FXB_EXP_ENEMY:
         if(fxB_frame == 0u) { psgWrite(6, 0x04); psgWrite(9, 13); }
         else if(fxB_frame == 1u) { psgWrite(6, 0x07); psgWrite(9, 11); }
         else if(fxB_frame == 2u) { psgWrite(6, 0x0B); psgWrite(9,  9); }
         else if(fxB_frame == 3u) { psgWrite(6, 0x10); psgWrite(9,  6); }
         else if(fxB_frame == 4u) { psgWrite(6, 0x16); psgWrite(9,  3); }
         else { fxB_kind = FXB_NONE; fxB_frame = 0; psgWrite(9, 0); return; }
         ++fxB_frame;
         return;
      case FXB_EXP_SHIP:
         if(fxB_frame == 0u) { setToneB(220, 0, 15); psgWrite(6, 0x04); }
         else if(fxB_frame == 1u) { setToneB(180, 0, 14); psgWrite(6, 0x06); }
         else if(fxB_frame == 2u) { setToneB(150, 0, 12); psgWrite(6, 0x08); }
         else if(fxB_frame == 3u) { setToneB(120, 0, 10); psgWrite(6, 0x0C); }
         else if(fxB_frame == 4u) { setToneB( 90, 0,  8); psgWrite(6, 0x12); }
         else if(fxB_frame == 5u) { setToneB( 70, 0,  6); psgWrite(6, 0x16); }
         else if(fxB_frame == 6u) { setToneB( 50, 0,  4); psgWrite(6, 0x1B); }
         else { fxB_kind = FXB_NONE; fxB_frame = 0; psgWrite(9, 0); return; }
         ++fxB_frame;
         return;
      case FXB_ENEMY_SHOT:
         /* To B: més volum i caiguda una mica més ampla que abans (encara curt, 3 frames). */
         if(fxB_frame == 0u) setToneB(162, 0, 14);
         else if(fxB_frame == 1u) setToneB(198, 0, 10);
         else if(fxB_frame == 2u) setToneB(232, 0, 5);
         else { fxB_kind = FXB_NONE; fxB_frame = 0; psgWrite(9, 0); return; }
         ++fxB_frame;
         return;
      default:
         psgWrite(9, 0);
         return;
   }
}

static void tickC(void) {
   if(fxC_kind != FXC_SHOT) { psgWrite(10, 0); return; }
   /* Tir nau: baixada de to en una sola direcció (període creixent), sense “rebot” — evita l’efecte tiruriruru. */
   {
      u8 o = g_shot_detune;
      u8 f0 = (u8)(142u + o), f1 = (u8)(168u + o), f2 = (u8)(198u + o), f3 = (u8)(228u + o);
      if(fxC_frame == 0u)       setToneC(f0, 0, 14);
      else if(fxC_frame == 1u) setToneC(f1, 0, 11);
      else if(fxC_frame == 2u) setToneC(f2, 0, 7);
      else if(fxC_frame == 3u) setToneC(f3, 0, 3);
      else { fxC_kind = FXC_NONE; fxC_frame = 0; psgWrite(10, 0); return; }
   }
   ++fxC_frame;
}

void soundTick(void) {
   u8 mixer = 0x3F;

   tickA();
   tickB();
   tickC();

   if(fxA_kind != FX_NONE) mixer &= (u8)~0x01;
   if(fxB_kind == FXB_EXP_ENEMY) mixer &= (u8)~0x10;
   else if(fxB_kind == FXB_EXP_SHIP) {
      mixer &= (u8)~0x02;
      mixer &= (u8)~0x10;
   } else if(fxB_kind == FXB_ENEMY_SHOT) mixer &= (u8)~0x02;
   if(fxC_kind != FXC_NONE) mixer &= (u8)~0x04;
   psgWrite(7, mixer);
}
