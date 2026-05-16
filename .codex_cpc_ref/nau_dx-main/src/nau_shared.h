#ifndef NAU_SHARED_H
#define NAU_SHARED_H

/* nau_shared.h — Estat compartit entre MENU.BIN i GAME.BIN.
 *
 * Resideix a RAM fixa a NAU_SHARED_ADDR (per sota del codi a 0x1000).
 * NO es toca per BSS/INITIALIZED init (queda fora del rang d'inici de cpc_entry.s).
 * Persisteix entre càrregues de binaris; la primera arrencada inicialitza per magic.
 *
 * firmware_jp: destí del JP a &39 (handler firmware a ROM). Si &38=C3, usar el retorn de
 *   cpct_disableFirmware() (llegeix &39 abans de posar EI;RET). Si &38=0xFB (ja desactivat per CPCT),
 *   el word a &39 no és vàlid — usar valor persistit o NAU_FIRMWARE_IRQ_ROM_DEFAULT (&C06B UK 6128).
 *   ROM espanyola / altres: provar NAU_FIRMWARE_IRQ_ROM_ALT.
 */

#include <cpctelera.h>

/* Adreça fixa. Per sota de Z80CODELOC (0x1000) i fora de _DATA del split actual. */
#define NAU_SHARED_ADDR   0x0A00
#define NAU_SHARED_MAGIC  0xDA   /* valor sentinella: ja inicialitzat */

/* Handler IRQ firmware (6128 OS anglès ~3.x). Si el teu CPC torna al BASIC, prova NAU_FIRMWARE_IRQ_ROM_ALT. */
#define NAU_FIRMWARE_IRQ_ROM_DEFAULT  0xC06B
#define NAU_FIRMWARE_IRQ_ROM_ALT      0xC972

/* game_result: escrit per GAME.BIN en acabar la partida */
#define NAU_RESULT_GAMEOVER  0
#define NAU_RESULT_WIN       1

/* from_game: MENU.BIN el comprova en arrencar per saber si ve d'una partida (GAME escriu NAU_FROM_GAME). */
#define NAU_FROM_FRESH  0
#define NAU_FROM_GAME   1

typedef struct {
    u8  magic;          /* NAU_SHARED_MAGIC quan ja inicialitzat               */
    u8  from_game;      /* NAU_FROM_FRESH | NAU_FROM_GAME                      */
    /* Resultat de la partida (escrit per GAME.BIN, llegit per MENU.BIN)       */
    u16 score;
    u8  level;
    u8  game_result;    /* NAU_RESULT_GAMEOVER / NAU_RESULT_WIN                */
    /* Controls (escrits per MENU.BIN, llegits per GAME.BIN)                  */
    u8  control_mode;
    cpct_keyID key_left;
    cpct_keyID key_right;
    cpct_keyID key_up;
    cpct_keyID key_down;
    cpct_keyID key_fire;
    cpct_keyID key_pause;
    cpct_keyID key_quit;
    /* Hi-scores Top 3 (exclusivament MENU.BIN; persistit entre partides)     */
    u16 hs_score[3];
    u8  hs_level[3];
    u8  hs_name[3][3];
    u16 firmware_jp;   /* veure cpc_entry.s + TRANSIT (nau_cas_core) */
} TNauShared;

/* Punter constant a adreça fixa: accés directe sense declarar cap variable. */
#define g_shared  ((TNauShared*)(NAU_SHARED_ADDR))
/* Evita fals positiu: només 0xDA a magic amb la resta de RAM aleatòria semblava “inicialitzat”. */
static inline u8 nau_shared_payload_plausible(void) {
    const TNauShared* s = g_shared;
    if(s->from_game != NAU_FROM_FRESH && s->from_game != NAU_FROM_GAME) return 0u;
    if(s->game_result != NAU_RESULT_GAMEOVER && s->game_result != NAU_RESULT_WIN) return 0u;
    if(s->level < 1u || s->level > 25u) return 0u;
    if(s->control_mode > 1u) return 0u;
    return 1u;
}

/* Adreça RAM del camp firmware_jp (cpc_entry.s, TRANSIT / nau_cas_core). */
#define NAU_FIRMWARE_JP_ADDR  (NAU_SHARED_ADDR + 39)

/* Noms en format catàleg AMSDOS (8 + 3, sense punt): CAS IN OPEN espera això, no "GAME.BIN". */
#define NAU_FNAME_GAME      "GAME    BIN"
#define NAU_FNAME_GAME_LEN  11
#define NAU_FNAME_MENU      "MENU    BIN"
#define NAU_FNAME_MENU_LEN  11

/* TRANSIT.BIN — loader resident FDC (NAU_TRANSIT_ORG): únic camí càrrega MENU<->GAME (exec &1000).
 * Crides directes; nau_transit_call.c encapsula l’adreça (Makefile + map). */
#define NAU_TRANSIT_ORG  0x7880

void nau_crtc_apply_mode0(void);

void nau_transit_load_menu(void);
void nau_transit_load_game(void);

#endif /* NAU_SHARED_H */
