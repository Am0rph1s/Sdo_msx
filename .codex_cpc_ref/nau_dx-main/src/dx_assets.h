#pragma once

#include <cpctelera.h>

/* Runtime metadata copied from banked source at safepoints. */
typedef struct {
    u8 biome_id;
    u8 star_theme;
    u8 enemy_anim_set;
    u8 boss_set;
} TDXBiomeMeta;

extern TDXBiomeMeta g_dx_active_biome_meta;
extern u8 dx_assets_loaded_biome;

void dx_assets_init(void);
void dx_assets_prepare_for_level_at_safepoint(u8 safepoint_id, u8 level);
