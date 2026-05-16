; PSG writer for CPC / CPCtelera
; C prototype: void psgWrite(u8 reg, u8 value);
; Uses the 8255 PPI and leaves PSG inactive between operations.

.module sound_hw
.globl _psgWrite

_psgWrite::
    ld hl, #2
    add hl, sp
    ld a, (hl)
    inc hl
    ld e, (hl)

    ; 8255 mode 0, Port A output, Port B input, Port C output
    ld bc, #0xF782
    out (c), c

    ; select register
    ld bc, #0xF400
    out (c), a
    ld bc, #0xF6C0
    out (c), c
    ld bc, #0xF600
    out (c), c

    ; write value
    ld a, e
    ld bc, #0xF400
    out (c), a
    ld bc, #0xF680
    out (c), c
    ld bc, #0xF600
    out (c), c
    ret
