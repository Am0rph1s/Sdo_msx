#include "level_config.h"

/* 5 biomes × 5 nivells. Cada nivell pre-boss (4,9,14,19): `intro_mask` = tast d’un tipus nou
 * només a la 1a onada; `mask` = pool de la resta (el tipus nou entra a `mask` des del primer
 * nivell després del boss). Nivell 25: 2 bosses.
 */
static const TLevelConfig LEVEL_TABLE[25] = {
    /* 1-5 Rocky */
    {3, 2, LMASK_BASIC, 0, 0},
    {3, 3, LMASK_BASIC, 0, 0},
    {4, 3, LMASK_BASIC, 0, 0},
    {4, 3, LMASK_BASIC, 0, LMASK_FAST},
    {1, 1, 0, LCFG_F_BOSS1, 0},
    /* 6-10 Ice — després del boss: basic+fast barrejat; nivell 9 intro heavy */
    {4, 4, LMASK_BASIC | LMASK_FAST, 0, 0},
    {4, 4, LMASK_BASIC | LMASK_FAST, 0, 0},
    {5, 4, LMASK_BASIC | LMASK_FAST, 0, 0},
    {4, 4, LMASK_BASIC | LMASK_FAST, 0, LMASK_HEAVY},
    {1, 1, 0, LCFG_F_BOSS1, 0},
    /* 11-15 Forest */
    {5, 5, LMASK_BASIC | LMASK_FAST | LMASK_HEAVY, 0, 0},
    {5, 5, LMASK_BASIC | LMASK_FAST | LMASK_HEAVY, 0, 0},
    {5, 6, LMASK_BASIC | LMASK_FAST | LMASK_HEAVY, 0, 0},
    {4, 5, LMASK_BASIC | LMASK_FAST | LMASK_HEAVY, 0, LMASK_DIVER},
    {1, 1, 0, LCFG_F_BOSS1, 0},
    /* 16-20 Fire */
    {6, 6, LMASK_BASIC | LMASK_FAST | LMASK_HEAVY | LMASK_DIVER, 0, 0},
    {6, 6, LMASK_BASIC | LMASK_FAST | LMASK_HEAVY | LMASK_DIVER, 0, 0},
    {6, 6, LMASK_BASIC | LMASK_FAST | LMASK_HEAVY | LMASK_DIVER, 0, 0},
    {4, 6, LMASK_BASIC | LMASK_FAST | LMASK_HEAVY | LMASK_DIVER, 0, LMASK_BOMBER},
    {1, 1, 0, LCFG_F_BOSS1, 0},
    /* 21-25 Tech */
    {6, 7, LMASK_BASIC | LMASK_FAST | LMASK_HEAVY | LMASK_DIVER | LMASK_BOMBER, 0, 0},
    {6, 7, LMASK_BASIC | LMASK_FAST | LMASK_HEAVY | LMASK_DIVER | LMASK_BOMBER, 0, 0},
    {7, 7, LMASK_BASIC | LMASK_FAST | LMASK_HEAVY | LMASK_DIVER | LMASK_BOMBER, 0, 0},
    {1, 8, LMASK_BASIC | LMASK_FAST | LMASK_HEAVY | LMASK_DIVER | LMASK_BOMBER, 0, 0},
    {1, 2, 0, LCFG_F_BOSS2, 0},
};

const TLevelConfig* level_config_get(u8 level) {
    if (level < 1u)
        level = 1u;
    else if (level > 25u)
        level = 25u;
    return &LEVEL_TABLE[level - 1u];
}
