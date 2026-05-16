;; nau_loader.s — Transició MENU ↔ GAME: nom a &0300; nucli nau_cas_core.inc copiat a &7000-&7FFF i JP &7000.
;; Durant CAS DIRECT el nou binari va a &1000 (NDX_ENTRY); el stub a &7000 no es trepitja. LOADER.BIN no intervé.

.module nau_loader
.globl _nau_load_run

.include "nau_cas_equ.inc"

_nau_cas_core_bin::
.include "nau_cas_core.inc"
_nau_cas_core_bin_end::

_nau_load_run::
    di
    ld   hl, #2
    add  hl, sp
    ld   e, (hl)
    inc  hl
    ld   d, (hl)
    inc  hl
    ld   c, (hl)
    ld   b, #0            ;; BC = len

    push de               ;; fname
    ld   de, #0x0300
    pop  hl               ;; HL=fname, DE=dest
    ldir

    ld   hl, #_nau_cas_core_bin
    ld   de, #NAU_CAS_SHADOW
    ld   bc, #(_nau_cas_core_bin_end - _nau_cas_core_bin)
    ldir
    jp   NAU_CAS_SHADOW
