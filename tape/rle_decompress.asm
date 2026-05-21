; RLE Decompressor for RAM
; Input: HL = Source (compressed data)
;        DE = Destination (RAM buffer)
;        BC = Size of compressed data
; Output: Decompressed data at DE
; Preserves: AF, BC, DE, HL

RLE_DECOMPRESS:
    push bc
    push de
    push hl

.rle_next:
    ; Check remaining size
    pop bc
    pop de
    pop hl
    
    ld a, b
    or c
    jr z, .rle_done
    
    ; Read byte
    ld a, (hl)
    inc hl
    push hl
    push de
    push bc
    dec bc
    
    cp 0xC0          ; RLE marker?
    jr nz, .rle_write_byte
    
    ; RLE sequence
    pop bc           ; Restore BC (count-1)
    pop de
    pop hl
    
    ld a, (hl)       ; Byte to repeat
    inc hl
    push hl
    push de
    push bc
    dec bc
    
    ld b, (hl)       ; Repeat count
    inc hl
    push hl
    push de
    push bc
    dec bc
    
    ; Now: A=byte, B=count, HL=next_src, DE=dest
    ; We need to write A to (DE) B times
    ; Use a loop
.write_rle:
    push af
    ex de, hl
    pop af
    ld (hl), a
    inc hl
    ex de, hl
    djnz .write_rle
    
    ; Done with RLE block, continue
    pop bc
    pop de
    pop hl
    jr .rle_next

.rle_write_byte:
    ; Write A to (DE)
    pop bc
    pop de
    pop hl
    
    push af
    ex de, hl
    pop af
    ld (hl), a
    inc hl
    ex de, hl
    push hl
    push de
    push bc
    
    jr .rle_next

.rle_done:
    ret
