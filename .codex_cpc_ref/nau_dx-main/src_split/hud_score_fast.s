.module hud_score_fast

.globl _hud_drawScore5_asm
.globl _hud_drawBonus2_asm
.globl _hud_getGlyph
.globl _cpct_drawSprite
.globl _cpct_drawSolidBox
.globl _score_text
.globl _bonus_text
.globl _bonus_timer

;; Draw 5-digit HUD score at fixed position (x=3,y=4).
;; Input:
;;   stack+2 = u8* v (screen base page: &C000 or &8000)
_hud_drawScore5_asm::
    ld   hl, #2
    add  hl, sp
    ld   e, (hl)
    inc  hl
    ld   d, (hl)          ; DE = v

    ;; Clear full score band first (partial HUD refresh left stray pixels).
    ;; y=4 => +0x2000, x=3 => +3
    ld   hl, #0x2003
    add  hl, de
    ex   de, hl           ; DE = score rect base
    ld   hl, #0x080F      ; height=8, width=15
    push hl
    ld   hl, #0x0000      ; B_BG pattern
    push hl
    push de
    call _cpct_drawSolidBox

    ;; Restore v into DE for glyph loop
    ld   hl, #2
    add  hl, sp
    ld   e, (hl)
    inc  hl
    ld   d, (hl)

    ld   hl, #0x2003
    add  hl, de
    ld   (_hud_score_dst), hl

    ld   hl, #_score_text
    ld   a, #5
    ld   (_hud_score_cnt), a
001$:
    ld   a, (hl)
    inc  hl

    push hl
    ld   l, a
    ld   h, #0
    push hl
    call _hud_getGlyph
    pop  af
    pop  bc
    push bc

    ld   de, (_hud_score_dst)

    ld   c, l
    ld   b, h
    ld   hl, #0x0802
    push hl
    push de
    push bc
    call _cpct_drawSprite

    ld   de, (_hud_score_dst)
    inc  de
    inc  de
    ld   hl, #0x0801
    push hl
    ld   hl, #0x0000
    push hl
    push de
    call _cpct_drawSolidBox

    ld   de, (_hud_score_dst)
    inc  de
    inc  de
    inc  de
    ld   (_hud_score_dst), de

    pop  hl
    ld   a, (_hud_score_cnt)
    dec  a
    ld   (_hud_score_cnt), a
    jr   nz, 001$
    ret

;; Draw bonus HUD (x=20,y=4): clear 6 bytes wide so column 27 separator stays intact;
;; if bonus_timer != 0, draw bonus_text[0..1].
;; Input: stack+2 = u8* v
_hud_drawBonus2_asm::
    ld   hl, #2
    add  hl, sp
    ld   e, (hl)
    inc  hl
    ld   d, (hl)          ; DE = v

    ;; Base = v + 0x2014 (x=20, y=4)
    ld   hl, #0x2014
    add  hl, de
    ld   (_hud_bonus_dst), hl

    ;; cpct_drawSolidBox: clear 6x8 B_BG
    ex   de, hl
    ld   hl, #0x0806      ; height=8, width=6
    push hl
    ld   hl, #0x0000
    push hl
    push de
    call _cpct_drawSolidBox

    ld   a, (_bonus_timer)
    or   a
    ret  z

    ld   hl, #_bonus_text
    ld   a, #2
    ld   (_hud_bonus_cnt), a
002$:
    ld   a, (hl)
    inc  hl
    push hl
    ld   l, a
    ld   h, #0
    push hl
    call _hud_getGlyph
    pop  af
    pop  bc
    push bc

    ld   de, (_hud_bonus_dst)
    ld   c, l
    ld   b, h
    ld   hl, #0x0802
    push hl
    push de
    push bc
    call _cpct_drawSprite

    ld   de, (_hud_bonus_dst)
    inc  de
    inc  de
    ld   hl, #0x0801
    push hl
    ld   hl, #0x0000
    push hl
    push de
    call _cpct_drawSolidBox

    ld   de, (_hud_bonus_dst)
    inc  de
    inc  de
    inc  de
    ld   (_hud_bonus_dst), de

    pop  hl
    ld   a, (_hud_bonus_cnt)
    dec  a
    ld   (_hud_bonus_cnt), a
    jr   nz, 002$
    ret

_hud_score_dst:
    .ds 2
_hud_score_cnt:
    .ds 1
_hud_bonus_dst:
    .ds 2
_hud_bonus_cnt:
    .ds 1
