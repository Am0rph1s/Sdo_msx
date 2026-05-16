.module draw_map_fast

.globl _drawMapWalls_asm
.globl _cpct_getScreenPtr
.globl _cpct_drawTileAligned2x8_f

;; Draws wall columns for map rows ty=2..24 (y=16..192), matching C drawMap().
;; Input:
;;   stack+2: u8* v
;;   stack+4: u8  phase (wallPhase)
;;   stack+5: u8* tiles_base (&wallTiles[0][0], 8 tiles * 16 bytes)
_drawMapWalls_asm::
    ld   hl, #2
    add  hl, sp
    ld   a, (hl)
    ld   (_dm_v_l), a
    inc  hl
    ld   a, (hl)
    ld   (_dm_v_h), a
    inc  hl
    ld   a, (hl)
    add  a, #2
    and  #7
    ld   (_dm_phase), a
    inc  hl
    ld   a, (hl)
    ld   (_dm_tiles_l), a
    inc  hl
    ld   a, (hl)
    ld   (_dm_tiles_h), a

    ld   a, #16
    ld   (_dm_y), a
    ld   a, #23
    ld   (_dm_rows), a

001$:
    ;; tile_ptr = tiles_base + (phase << 4)
    ld   a, (_dm_phase)
    add  a, a
    add  a, a
    add  a, a
    add  a, a
    ld   e, a
    ld   d, #0
    ld   hl, (_dm_tiles_l)
    add  hl, de
    ld   (_dm_tile_l), hl

    ;; x = 0
    call _dm_draw_one_x0
    ;; x = 2
    call _dm_draw_one_x2
    ;; x = 76
    call _dm_draw_one_x76
    ;; x = 78
    call _dm_draw_one_x78

    ;; next row
    ld   a, (_dm_y)
    add  a, #8
    ld   (_dm_y), a

    ld   a, (_dm_phase)
    inc  a
    and  #7
    ld   (_dm_phase), a

    ld   a, (_dm_rows)
    dec  a
    ld   (_dm_rows), a
    jr   nz, 001$
    ret

_dm_draw_one_x0:
    ld   hl, #0
    jr   _dm_draw_at_x
_dm_draw_one_x2:
    ld   hl, #2
    jr   _dm_draw_at_x
_dm_draw_one_x76:
    ld   hl, #76
    jr   _dm_draw_at_x
_dm_draw_one_x78:
    ld   hl, #78
_dm_draw_at_x:
    ;; cpct_getScreenPtr(v, x, y)
    ld   c, l                  ; C = x
    ld   a, (_dm_y)
    ld   b, a                  ; B = y, packed with x for __z88dk_callee
    push bc                  ; y:x
    ld   hl, (_dm_v_l)
    push hl                  ; v
    call _cpct_getScreenPtr  ; HL = screen ptr

    ;; cpct_drawTileAligned2x8_f(tile_ptr, screen_ptr)
    push hl                  ; screen ptr
    ld   hl, (_dm_tile_l)
    push hl                  ; tile ptr
    call _cpct_drawTileAligned2x8_f
    ret

_dm_v_l:      .ds 1
_dm_v_h:      .ds 1
_dm_tiles_l:  .ds 1
_dm_tiles_h:  .ds 1
_dm_tile_l:   .ds 1
_dm_tile_h:   .ds 1
_dm_phase:    .ds 1
_dm_y:        .ds 1
_dm_rows:     .ds 1
