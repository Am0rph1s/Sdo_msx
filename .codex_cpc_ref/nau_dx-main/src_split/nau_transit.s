;; =============================================================================
;; nau_transit.s — Loader FDC pur per CPCtelera / sdasz 3.6.8
;; =============================================================================
.module nau_transit

.globl _nau_transit_load_menu
.globl _nau_transit_load_game

;; --- Ports NEC 765 en CPC ---
FDC_STATUS  = 0xFB7E
FDC_DATA    = 0xFB7F
FDC_MOTOR   = 0xFA7E

.include "nau_cas_equ.inc"
.include "nau_fdc_equ.inc"
.include "nau_fdc_extents.inc"

;; Exec MENU/GAME (cos sense capçalera AMSDOS a RAM)
EXEC_ADDR           = 0x1000

.area _CODE

;; Entrades fixes: MENU a ORG, GAME a ORG+6 (nau_transit_call.c / Makefile).
_nau_transit_load_menu::
    jp   transit_menu_body
    nop
    nop
    nop
_nau_transit_load_game::
    jp   transit_game_body

transit_menu_body::
    ;; Diagnòstic/robustesa: motor ON immediat en entrar al body.
    ld   bc, #FDC_MOTOR
    ld   a, #1
    out  (c), a
    ld   hl, #NDX_ENTRY
    ld   a, l
    ld   (transit_dst), a
    ld   a, h
    ld   (transit_dst_hi), a
    ld   hl, #MENU_ONDISK_BYTES
    ld   a, l
    ld   (transit_rem), a
    ld   a, h
    ld   (transit_rem_hi), a
    ld   a, #128
    ld   (transit_hdr_skip), a
    ld   hl, #MENU_SECTORS_CHS
    ld   a, l
    ld   (transit_tbl_lo), a
    ld   a, h
    ld   (transit_tbl_hi), a
    ld   a, #MENU_SECTOR_COUNT
    jr   transit_load_run

transit_game_body::
    ;; Diagnòstic/robustesa: motor ON immediat en entrar al body.
    ld   bc, #FDC_MOTOR
    ld   a, #1
    out  (c), a
    ld   hl, #NDX_ENTRY
    ld   a, l
    ld   (transit_dst), a
    ld   a, h
    ld   (transit_dst_hi), a
    ld   hl, #GAME_ONDISK_BYTES
    ld   a, l
    ld   (transit_rem), a
    ld   a, h
    ld   (transit_rem_hi), a
    ld   a, #128
    ld   (transit_hdr_skip), a
    ld   hl, #GAME_SECTORS_CHS
    ld   a, l
    ld   (transit_tbl_lo), a
    ld   a, h
    ld   (transit_tbl_hi), a
    ld   a, #GAME_SECTOR_COUNT
    ;; fall through

transit_load_run:
    ld   b, a
    ld   a, b
    ld   (transit_sectors_left), a

    ;; Motor ON immediat per validar entrada a TRANSIT (encara que el FDC falli ràpid).
    ld   bc, #FDC_MOTOR
    ld   a, #1
    out  (c), a

    di
    ;; Fixar mapa per defecte del projecte des del principi del flux TRANSIT.
    ;; Això evita carregar en un context i executar en un altre (ROM visible a &1000).
    ld   bc, #0x7F00
    ld   a, #0xC0
    out  (c), a
    ;; Guardar SP del caller (BASIC) per poder tornar net en cas d'error.
    ld   (transit_saved_sp), sp
    ;; Mateix pila segura que nau_fdc_t0 (nau_fdc_load.inc); evita pila BASIC massa baixa.
    ld   sp, #NDX_CAS_SP
    xor  a
    ld   (NAU_FDC_DRIVE), a
    call transit_motor_on_delay
    ;; Inicialització igual que nau_fdc_t0 (camí que tenim més provat):
    ;; motor delay + recalibrate (Sense Interrupt intern).
    call transit_fdc_init

transit_sector_loop:
    ld   a, (transit_sectors_left)
    or   a
    jr   z, load_done
    dec  a
    ld   (transit_sectors_left), a

    ld   a, (transit_tbl_lo)
    ld   l, a
    ld   a, (transit_tbl_hi)
    ld   h, a
    ld   b, (hl)
    inc  hl
    inc  hl
    ld   c, (hl)
    inc  hl
    ld   a, l
    ld   (transit_tbl_lo), a
    ld   a, h
    ld   (transit_tbl_hi), a

    ;; Camí FDC del loader (nau_fdc_low.inc), més robust en CPC/WinAPE que el camí local.
    ;; Entrada requerida: B=track, C=sector ID, DE=destí.
    ld   de, #NDX_CAS_BUF
    call nau_fdc_read_phys_sector
    jr   z, transit_stream_ok
    jr   transit_fail
transit_stream_ok:
    ld   hl, #NDX_CAS_BUF
    call transit_stream_sector
    jr   transit_sector_loop

load_done:
    ;; Fixar config RAM default i desactivar ROMs abans de llegir/saltar a &1000.
    ld   bc, #0x7F00
    ld   a, #0xC0
    out  (c), a
    ld   a, #0x8C
    out  (c), a

    xor  a
    ld   bc, #FDC_MOTOR
    out  (c), a
    im   1
    ld   sp, (transit_saved_sp)
    jp   EXEC_ADDR

transit_fail:
    xor  a
    ld   bc, #FDC_MOTOR
    out  (c), a
    im   1
    ld   sp, (transit_saved_sp)
    ei
    ret

;; --- Mateix FDC handshake que nau_fdc_low.inc (cpctech fdcload) ---

transit_motor_on_delay:
    ld   bc, #FDC_MOTOR
    ld   a, #1
    out  (c), a
    ld   b, #0x08
transit_md_o:
    ld   de, #0
transit_md_i:
    dec  de
    ld   a, d
    or   e
    jr   nz, transit_md_i
    djnz transit_md_o
    ret

;; Inicialització 765 (camí canònic de nau_fdc_low.inc, sense variacions locals).
transit_fdc_init:
    call nau_fdc_recalibrate
    ret

;; HL = NDX_CAS_BUF; mateix flux que nau_fdc_stream_sector (nau_fdc_load.inc)
transit_stream_sector:
    ld   bc, #512
transit_ss_lp:
    ld   a, (transit_rem)
    ld   e, a
    ld   a, (transit_rem_hi)
    or   e
    ret  z
    ld   a, (transit_hdr_skip)
    or   a
    jr   z, transit_ss_copy
    dec  a
    ld   (transit_hdr_skip), a
    inc  hl
    jr   transit_ss_dec
transit_ss_copy:
    ld   e, (hl)
    inc  hl
    push hl
    ld   a, (transit_dst)
    ld   l, a
    ld   a, (transit_dst_hi)
    ld   h, a
    ld   a, e
    ld   (hl), a
    inc  hl
    ld   a, l
    ld   (transit_dst), a
    ld   a, h
    ld   (transit_dst_hi), a
    pop  hl
transit_ss_dec:
    ld   a, (transit_rem)
    ld   e, a
    ld   a, (transit_rem_hi)
    ld   d, a
    dec  de
    ld   a, e
    ld   (transit_rem), a
    ld   a, d
    ld   (transit_rem_hi), a
    dec  bc
    ld   a, b
    or   c
    jr   nz, transit_ss_lp
    ret

.include "nau_fdc_low.inc"
.include "nau_fdc_extents_chs.inc"

.area _DATA
_transit_data_anchor::
    .db 0
transit_sectors_left:
    .db 0
transit_rem:
    .db 0
transit_rem_hi:
    .db 0
transit_dst:
    .db 0
transit_dst_hi:
    .db 0
transit_hdr_skip:
    .db 0
transit_tbl_lo:
    .db 0
transit_tbl_hi:
    .db 0
transit_saved_sp:
    .dw 0
