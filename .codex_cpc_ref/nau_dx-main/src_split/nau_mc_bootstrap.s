;; nau_mc_bootstrap.s — MC_START_PROGRAM + KL_ROM_WALK + drive (&BE7D), patró Kevin Thacker.
;; S’executa des MENU/GAME (codi a &1000+), abans de nau_load_run; el nucli CAS rau a &7000 (nau_loader).

.module nau_mc_bootstrap
.globl _nau_mc_bootstrap

.include "nau_cas_equ.inc"

_nau_mc_bootstrap::
    di
    ld   hl, (#AMSDOS_DRV_PTR)
    ld   a, (hl)
    ld   (nau_mc_drv+1), a
    ld   c, #0xFF
    ld   hl, #nau_mc_resume
    call #MC_START_PROGRAM
nau_mc_resume::
    call #KL_ROM_WALK
nau_mc_drv::
    ld   a, #0
    ld   hl, (#AMSDOS_DRV_PTR)
    ld   (hl), a
    ret

;; JP firmware a &38/&39 sense EI (no usar cpct_reenableFirmware: fa EI i pot saltar IRQ abans del CAS).
;; SDCC: primer paràmetre u16 a HL.
.globl _nau_fw_patch_irq_vector
_nau_fw_patch_irq_vector::
    di
    ld   a, #JP_OPCODE
    ld   (#0x0038), a
    ld   (#0x0039), hl
    ret
