; cpc_entry.s — Punt d'entrada per RUN del BIN (SDCC + --no-std-crt0 sense crt0 de la lib).
;
; CPCTelera (GETRUNADDRESS) posa -e al símbol «cpc_run_address» del .map. Si en lloc d'això
; s'usa _main, NO es copia INITIALIZER→INITIALIZED: frontBuffer/backBuffer queden a 0 al RAM
; carregat (0x7E3E… buit al fitxer; valors reals només a s__INITIALIZER). Resultat: crash/reinici.
;
; No usar IX/IY com a frame (regla projecte). Preserva BC’ si cal — aquí no toquem alternats.

    .module cpc_entry

    .globl _main
    .globl cpc_run_address
    .globl _cpc_run_address
    ; Símbols de segment (resolen a l'enllaçador, no en aquest .rel)
    .globl l__DATA
    .globl s__DATA
    .globl l__INITIALIZER
    .globl s__INITIALIZED
    .globl s__INITIALIZER
    .globl l__BSS
    .globl s__BSS

    .area _CODE

; Adreça de g_shared.firmware_jp (nau_shared.h) — ha de coincidir amb nau_loader.s
NAU_FIRMWARE_JP_ADDR = 0x0A27
JP_OPCODE_ROM        = 0xC3

; Mateix punt d'entrada: mapa CPCTelera i enllaç C (_ prefix)
_cpc_run_address::
cpc_run_address::
    ;; Pila segura abans de qualsevol C / proleg SDCC (CALL des de BASIC deixa SP baix).
    ld sp, #0xBFFE

    ;; Abans de zeroar _DATA: copiar el destí del vector IRQ del firmware (0x0039..0x003A)
    ;; cap a g_shared.firmware_jp. BASIC deixa JP C3 nn nn a 0x0038; el bucle _DATA
    ;; ho esborraria i cpct_disableFirmware llegiria 0 → TRANSIT restauraria JP 0.
    ;; Si (0x0038)!=C3 (p.ex. EI després de cpct_disableFirmware del binari anterior),
    ;; no escriure: es conserva firmware_jp vàlid d'una arrencada anterior.
    ld  a, (#0x0038)
    cp  #JP_OPCODE_ROM
    jr  nz, _cpc_skip_fw_snap
    ld  hl, (#0x0039)
    ld  (#NAU_FIRMWARE_JP_ADDR), hl
_cpc_skip_fw_snap:

    ; Netejar _DATA (globals C no inicialitzats = BSS en C).
    ; Amb --data-loc 0, s__DATA = 0x0000. l__DATA = mida total de globals uninit.
    ; Cal fer-ho ABANS de copiar INITIALIZER.
    ; g_shared ara viu a 0x0A00, fora de _DATA dels binaris split.
    ld  bc, #l__DATA
    ld  a, b
    or  a, c
    jr  Z, _cpc_skip_data
    ld  hl, #s__DATA
    xor a
_cpc_data_loop:
    xor a
    ld  (hl), a
    inc hl
    dec bc
    ld  a, b
    or  a, c
    jr  NZ, _cpc_data_loop
_cpc_skip_data:
    ld  bc, #l__INITIALIZER
    ld  a, b
    or  a, c
    jr  Z, _cpc_skip_copy
    ld  de, #s__INITIALIZED
    ld  hl, #s__INITIALIZER
    ldir
_cpc_skip_copy:
    ld  bc, #l__BSS
    ld  a, b
    or  a, c
    jr  Z, _cpc_skip_bss
    ld  hl, #s__BSS
    xor a
_cpc_bss_loop:
    ld  (hl), a
    inc hl
    dec bc
    ld  a, b
    or  a, c
    jr  NZ, _cpc_bss_loop
_cpc_skip_bss:
    jp  _main

;; Si no hi ha cap _BSS (p. ex. DX_HW_128K=0 sense dx_bank.s), l'enllaçador no defineix s__BSS/l__BSS.
.area _BSS
_cpc_bss_anchor::
    .ds 1
