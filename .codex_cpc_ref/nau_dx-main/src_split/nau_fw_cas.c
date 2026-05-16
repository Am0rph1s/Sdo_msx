/* nau_fw_peek_firmware_jp_from_vector: cridar al començament de main(), abans de cpct_disableFirmware().
 *
 * nau_fw_reenable_for_cas (abans de nau_load_run):
 * - soundStopAll
 * - nau_fw_patch_irq_vector(g_shared->firmware_jp): &38 apunta al firmware sense EI (MC_START/CAS
 *   poden tornar al BASIC si el vector segueix com FB:C9 de cpct_disableFirmware).
 * - cpct_enableLowerROM, nau_mc_bootstrap, cpct_disableUpperROM, di.
 * No cpct_reenableFirmware: acaba amb EI → IRQ abans de nau_load_run.
 */
#include "nau_shared.h"
#include "game_sfx.h"

u16 nau_fw_peek_firmware_jp_from_vector(void) {
    volatile u8* p = (volatile u8*)(void*)0x0038u;
    if(p[0] != 0xC3u)
        return 0u;
    /* JP nn: nn baix a &39, alt a &3A (evita u16* a adreça senar amb SDCC). */
    return (u16)p[1] | ((u16)p[2] << 8);
}

void nau_fw_reenable_for_cas(void) {
    u16 jp;

    soundStopAll();
    jp = g_shared->firmware_jp;
    if(jp == 0u)
        jp = nau_fw_peek_firmware_jp_from_vector();
    if(jp == 0u)
        jp = NAU_FIRMWARE_IRQ_ROM_DEFAULT;
    nau_fw_patch_irq_vector(jp);

    cpct_enableLowerROM();
    nau_mc_bootstrap();
    cpct_disableUpperROM();
    __asm__("di");
}
