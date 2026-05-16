#pragma once
#include <cpctelera.h>

/* Font HUD mode 0: 4 píxels d'ample (2 bytes/fila) × 8 línies = 16 B/glif */
#define HUD_GLYPH_W_BYTES 2
#define HUD_GLYPH_H 8
/* 1 byte mode 0 = 2 px buits entre glifs (separació visual) */
#define HUD_GLYPH_GAP_BYTES 1
#define HUD_GLYPH_ADVANCE (HUD_GLYPH_W_BYTES + HUD_GLYPH_GAP_BYTES)

void hud_buildFont(void);
const u8* hud_getGlyph(char c);
const u8* hud_getGlyphHi(char c);
void hud_updateScoreText(u16 score, char* buffer);
void hud_updateLevelText(u8 level, char* buffer);
void hud_updateLivesText(u8 lives, char* buffer);
