; dx_bank.s — RAM paging real CPC 6128 / 128K (Gate Array reg. 3, com cpct_pageMemory).
; No usa IX/IY. Valors cfg: RAMCFG_* | BANK_* (veure CPCTelera memutils/banks.h).

    .module dx_bank

    .globl _dx_init
    .globl _dx_enter_safepoint
    .globl _dx_memory_default
    .globl _dx_memory_map
    .globl _dx_ram128_probe_ok
    .globl _dx_last_safepoint

    .area _CODE

; ---------- nucli: escriu (A | 0xC0) al port Gate Array 0x7F00 ----------
_dx_ram_ga_out::
    or #0xC0
    ld bc, #0x7F00
    out (c), a
    ret

; void dx_memory_default(void)
; Mapa estàndard només primer 64K (RAMCFG_0 | BANK_0), el que espera el core CPCT/joc.
_dx_memory_default::
    xor a
    jp _dx_ram_ga_out

; void dx_memory_map(u8 cfg_and_bank)
; Paràmetre C / SDCC: primer byte a (sp+2).
_dx_memory_map::
    ld hl, #2
    add hl, sp
    ld a, (hl)
    jp _dx_ram_ga_out

; void dx_enter_safepoint(u8 safepoint_id)
; Sempre torna al mapa per defecte abans de tornar al C (evita VRAM/punters incoherents).
_dx_enter_safepoint::
    ld hl, #2
    add hl, sp
    ld a, (hl)
    ld (_dx_last_safepoint), a
    jp _dx_memory_default

; void dx_init(void)
; 1) Mapa per defecte. 2) Prova escriptura a 0x4000 amb RAMCFG_4|BANK_0 (finestra al 2n xip).
; 3) Torna a mapa per defecte. Omple _dx_ram128_probe_ok (1=OK, 0=fallback / no 128K).
_dx_init::
    xor a
    ld (_dx_last_safepoint), a
    call _dx_memory_default
    ld a, #4                  ; RAMCFG_4 | BANK_0 (BANK_0 = 0)
    call _dx_ram_ga_out
    ld hl, #0x4000
    ld e, (hl)
    ld a, #0x5a
    ld (hl), a
    ld a, (hl)
    cp #0x5a
    jr nz, _dx_init_bad
    ld a, e
    ld (hl), a
    ld a, #1
    ld (_dx_ram128_probe_ok), a
    jp _dx_memory_default
_dx_init_bad:
    ld a, e
    ld (hl), a
    xor a
    ld (_dx_ram128_probe_ok), a
    jp _dx_memory_default

    .area _BSS
_dx_ram128_probe_ok:
    .ds 1
_dx_last_safepoint:
    .ds 1
