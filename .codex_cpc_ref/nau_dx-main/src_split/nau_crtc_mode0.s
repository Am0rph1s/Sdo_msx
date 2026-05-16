;; nau_crtc_mode0.s — Restaura registres CRTC a mode 0 estàndard CPC (6128 PAL).
;; Es crida des de MENU.BIN al començament de main(), després de l’INTRO overscan:
;; així no es barreja amb IRQ durant el loader i el 6845 queda coherent abans de CPCT.

.module nau_crtc
.globl _nau_crtc_apply_mode0

.area _CODE

_nau_crtc_apply_mode0::
    di
    ld hl, #crtc_m0_tab
    ld c, #0
crtc_m0_lp:
    ld b, #0xBC
    out (c), c
    ld a, (hl)
    inc hl
    ld b, #0xBD
    out (c), a
    inc c
    ld a, c
    cp #16
    jr nz, crtc_m0_lp
    ei
    ret

crtc_m0_tab:
    .db 0x3F, 0x28, 0x2E, 0x8E, 0x26, 0x00, 0x19, 0x1E
    .db 0x00, 0x07, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00
