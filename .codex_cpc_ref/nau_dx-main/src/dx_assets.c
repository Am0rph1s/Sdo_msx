#include "dx_assets.h"
#include "dx_bank.h"

/* Pilot pack (phase 4): tiny biome metadata, loaded through safepoint pipeline. */
static const TDXBiomeMeta s_dx_biome_seed[5] = {
    {0u, 1u, 0u, 0u},
    {1u, 2u, 1u, 0u},
    {2u, 3u, 2u, 1u},
    {3u, 4u, 2u, 1u},
    {4u, 5u, 3u, 2u}
};

TDXBiomeMeta g_dx_active_biome_meta = {0u, 0u, 0u, 0u};
u8 dx_assets_loaded_biome = 0xFFu;

static u8 dx_biome_from_level(u8 level) {
    u8 biome = (u8)((level - 1u) / 5u);
    if (biome > 4u) biome = 4u;
    return biome;
}

static void dx_copy_biome_meta_from_seed(u8 biome) {
    g_dx_active_biome_meta.biome_id = s_dx_biome_seed[biome].biome_id;
    g_dx_active_biome_meta.star_theme = s_dx_biome_seed[biome].star_theme;
    g_dx_active_biome_meta.enemy_anim_set = s_dx_biome_seed[biome].enemy_anim_set;
    g_dx_active_biome_meta.boss_set = s_dx_biome_seed[biome].boss_set;
}

void dx_assets_init(void) {
    dx_assets_loaded_biome = 0xFFu;
    dx_copy_biome_meta_from_seed(0u);
}

static void dx_assets_prepare_for_level(u8 level) {
    u8 biome = dx_biome_from_level(level);
    u8 cfg = (u8)(RAMCFG_4 | (biome + 1u));

    if (dx_assets_loaded_biome == biome) return;
    dx_memory_map(cfg);                              /* source bank select */
    dx_copy_biome_meta_from_seed(biome);            /* metadata copy */
    dx_memory_default();                            /* restore default map */

    dx_assets_loaded_biome = biome;
}

void dx_assets_prepare_for_level_at_safepoint(u8 safepoint_id, u8 level) {
    dx_enter_safepoint(safepoint_id);
    dx_assets_prepare_for_level(level);
}
