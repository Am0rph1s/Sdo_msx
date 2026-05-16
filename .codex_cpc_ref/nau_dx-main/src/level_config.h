#pragma once
#include <cpctelera.h>

/* Màscara alineada amb ENEMY_TYPE_* de main.c (0..4). BOSS no entra al pool aleatori. */
#define LMASK_BASIC  ((u8)(1u << 0))
#define LMASK_FAST   ((u8)(1u << 1))
#define LMASK_HEAVY  ((u8)(1u << 2))
#define LMASK_DIVER  ((u8)(1u << 3))
#define LMASK_BOMBER ((u8)(1u << 4))

#define LCFG_F_BOSS1 ((u8)1u) /* 1 onada, 1 boss */
#define LCFG_F_BOSS2 ((u8)2u) /* 1 onada, 2 bosses (nivell final) */

typedef struct {
    u8 waves;       /* Nombre d'onades per completar el nivell */
    u8 per_wave;    /* Enemics per onada (fix; boss: 1 o 2) */
    u8 mask;        /* Pool per onades normals: tipus ja desbloquejats (sense el “nou” d’aquest acte) */
    u8 flags;       /* 0 normal; LCFG_F_BOSS1 / LCFG_F_BOSS2 */
    u8 intro_mask;  /* 0 cap; sinó una sola LMASK_* = primera onada només aquest tipus (pre-boss) */
} TLevelConfig;

const TLevelConfig* level_config_get(u8 level);
