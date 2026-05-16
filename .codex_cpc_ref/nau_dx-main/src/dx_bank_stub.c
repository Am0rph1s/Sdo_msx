/* Perfil 64K (DX_HW_128K=0): mateixa API que dx_bank.s sense tocar el Gate Array.
 * Evita que «nau_dx» es comporti diferent de «nau» fins que hi hagi ús real de bancs. */
#include "dx_bank.h"

u8 dx_ram128_probe_ok = 0;
u8 dx_last_safepoint = DX_SP_BOOT;

void dx_init(void) {
    dx_last_safepoint = DX_SP_BOOT;
}

void dx_memory_default(void) {}

void dx_memory_map(u8 cfg_and_bank) {
    (void)cfg_and_bank;
}

void dx_enter_safepoint(u8 safepoint_id) {
    dx_last_safepoint = safepoint_id;
}
