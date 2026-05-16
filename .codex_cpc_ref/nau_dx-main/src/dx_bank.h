#pragma once

#include <cpctelera.h>

/*
 * Capa DX — dues màquines de build (cfg/build_config.mk, DX_HW_128K):
 *
 *   DX_HW_128K=0 (per defecte): perfil 64K, mateixa filosofia que el projecte «nau».
 *       Enllaça dx_bank_stub.c: cap OUT al port &7F, dx_ram128_probe_ok=0.
 *       Ús: binari estable a 464 i 6128 sense efectes col·laterals de paging.
 *
 *   DX_HW_128K=1: perfil 128K (dx_bank.s). dx_boot() → dx_init: prova del 2n xip i omple
 *       dx_ram128_probe_ok. Només activar-ho quan el codi faci servir de debò
 *       dx_memory_map / recursos bancats en safepoints; sinó només afegeix risc.
 *
 * Regla: no barrejar «proves de 128K» al camí crític del joc fins que hi hagi
 * una funcionalitat que en depengui (sinó s’espatlla el que ja funciona en 64K).
 */

/* Punts segurs on es permet canviar bancs/cargar recursos DX. */
#define DX_SP_BOOT      0
#define DX_SP_TITLE     1
#define DX_SP_STARTGAME 2
#define DX_SP_ENDGAME   3
#define DX_SP_BIOME     4

/* IDs de recursos DX reservats per fases futures. */
#define DX_RES_STARFIELD_BIOME0 0
#define DX_RES_STARFIELD_BIOME1 1
#define DX_RES_STARFIELD_BIOME2 2
#define DX_RES_STARFIELD_BIOME3 3
#define DX_RES_STARFIELD_BIOME4 4

typedef struct {
    u8 bank_id;
    u16 addr;
    u16 size;
} TDXResourceDesc;

/* Resultat de la prova 128K (perfil 1: asm); perfil 0 sempre 0. */
extern u8 dx_ram128_probe_ok;
/* Debug marker: últim safepoint registrat per la capa DX. */
extern u8 dx_last_safepoint;

#define dx_has_extra_ram() (dx_ram128_probe_ok != 0)

/* Nom «públic» a l’arrencada; implementació enllaçada: dx_init (asm o stub). */
#define dx_boot() (dx_init())

void dx_init(void);

/* Força RAMCFG_0|BANK_0. Cridar després de qualsevol dx_memory_map abans de tornar al bucle. */
void dx_memory_default(void);

/* cfg_and_bank = RAMCFG_* | BANK_* (veure CPCTelera banks.h, inclòs via cpctelera.h). Només en safepoints, sense IRQ que toquin 0x4000-0x7FFF. */
void dx_memory_map(u8 cfg_and_bank);

/* Restaura mapa per defecte (ara ignora safepoint_id; reservat per futures ramificacions). */
void dx_enter_safepoint(u8 safepoint_id);
