#ifndef GAME_SFX_H
#define GAME_SFX_H

#include <cpctelera.h>

void soundInit(void);
void soundTick(void);
void soundStopAll(void);
void sfxShot(void);
void sfxEnemyShot(void);
void sfxEnemyExplosion(void);
void sfxShipExplosion(void);
void sfxBeep(void);
void sfxGameOver(void);
void sfxLevelUp(void);
void sfxBossWarn(void);
void sfxMenuSelect(void);

#endif
