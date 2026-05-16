; Stable hot-path helpers: tick + stars + shots
.module v9_fast
.globl _v9_tickStarsShots
.globl _fire_cool
.globl _wallPhase
.globl _shots
.globl _s1
.globl _s2
.globl _s3
_v9_tickStarsShots::
    ld hl,#_fire_cool
    ld a,(hl)
    or a
    jr z,tick_phase
    dec (hl)
tick_phase:
    ld hl,#_wallPhase
    ld a,(hl)
    inc a
    and #0x07
    ld (hl),a
    ld hl,#_s1+1
    ld b,#4
s1_loop:
    ld a,(hl)
    add a,#4
    cp #200
    jr c,s1_nowrap
    sub #200
s1_nowrap:
    ld (hl),a
    inc hl
    inc hl
    djnz s1_loop
    ld hl,#_s2+1
    ld b,#7
s2_loop:
    ld a,(hl)
    add a,#9
    cp #200
    jr c,s2_nowrap
    sub #200
s2_nowrap:
    ld (hl),a
    inc hl
    inc hl
    djnz s2_loop
    ld hl,#_s3+1
    ld b,#11
s3_loop:
    ld a,(hl)
    add a,#16
    cp #200
    jr c,s3_nowrap
    sub #200
s3_nowrap:
    ld (hl),a
    inc hl
    inc hl
    djnz s3_loop
    ld hl,#_shots
    ld b,#4
shot_loop:
    push bc
    inc hl
    inc hl
    ld a,(hl)
    or a
    jr z,shot_done
    dec hl
    ld a,(hl)
    ; GAME_Y0=16, pas vertical del tret=12 -> no deixar y<16 actiu (coincideix amb drawShots).
    cp #28
    jr c,shot_deactivate_from_y
    sub #12
    ld (hl),a
    dec hl
    ld a,(hl)
    cp #4
    jr c,shot_deactivate_from_x
    cp #76
    jr nc,shot_deactivate_from_x
    inc hl
    inc hl
    jr shot_done
shot_deactivate_from_x:
    inc hl
shot_deactivate_from_y:
    inc hl
    ld (hl),#0
shot_done:
    pop bc
    inc hl
    djnz shot_loop
    ret
