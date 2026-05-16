#ifndef TITLE_LOGO_DATA_H
#define TITLE_LOGO_DATA_H

#include <cpctelera.h>

/* Mode 0: 74×25 px → 37 bytes × 25 línies; masked = 2 bytes (maska+píxel) per byte de pantalla. */
#define TITLE_LOGO_W_BYTES 37
#define TITLE_LOGO_H       25
/* Posició al menú/títol (mode 0, 80 bytes d’amplada): X centrat; Y pujat per aprofitar alçada sota. */
#define TITLE_LOGO_X       ((u8)((80u - TITLE_LOGO_W_BYTES) >> 1))
#define TITLE_LOGO_Y       6
/* Ratlla cyan sota el bloc títol (menú principal); ~13 línies sota el baix del logo (Y+H−1). */
#define TITLE_MENU_RULE_Y  43

extern const u8 title_logo_masked[];

/* Dibuixa el logo amb perfil fosc (silueta 1 px cardinals + masked), millor lectura en CRT. */
void title_logo_draw_masked_with_rim(u8* video_mem);

#endif
