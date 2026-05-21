;--------------------------------------------------------
; File Created by SDCC : free open source ANSI-C Compiler
; Version 4.2.0 #13081 (MINGW64)
;--------------------------------------------------------
	.module loader
	.optsdcc -mz80
	
;--------------------------------------------------------
; Public variables in this module
;--------------------------------------------------------
	.globl _main
	.globl _rle_decompress
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _DATA
;--------------------------------------------------------
; ram data
;--------------------------------------------------------
	.area _INITIALIZED
;--------------------------------------------------------
; absolute external ram data
;--------------------------------------------------------
	.area _DABS (ABS)
;--------------------------------------------------------
; global & static initialisations
;--------------------------------------------------------
	.area _HOME
	.area _GSINIT
	.area _GSFINAL
	.area _GSINIT
;--------------------------------------------------------
; Home
;--------------------------------------------------------
	.area _HOME
	.area _HOME
;--------------------------------------------------------
; code
;--------------------------------------------------------
	.area _CODE
;loader.c:14: void rle_decompress(uint8_t *src, uint8_t *dest, uint16_t size) {
;	---------------------------------
; Function rle_decompress
; ---------------------------------
_rle_decompress::
	push	ix
	ld	ix,#0
	add	ix,sp
	push	af
	push	af
	dec	sp
	ld	-3 (ix), l
	ld	-2 (ix), h
;loader.c:15: uint16_t i = 0;
	ld	bc, #0x0000
;loader.c:16: while (i < size) {
00107$:
	ld	a, c
	sub	a, 4 (ix)
	ld	a, b
	sbc	a, 5 (ix)
	jr	NC, 00110$
;loader.c:17: uint8_t byte = src[i++];
	ld	a, c
	ld	h, b
;	spillPairReg hl
;	spillPairReg hl
	inc	bc
	add	a, -3 (ix)
	ld	l, a
;	spillPairReg hl
;	spillPairReg hl
	ld	a, h
	adc	a, -2 (ix)
	ld	h, a
	ld	a, (hl)
;loader.c:18: if (byte == 0xC0) {
	ld	-1 (ix), a
	sub	a, #0xc0
	jr	NZ, 00105$
;loader.c:19: uint8_t val = src[i++];
	ld	a, c
	ld	h, b
;	spillPairReg hl
;	spillPairReg hl
	inc	bc
	add	a, -3 (ix)
	ld	l, a
;	spillPairReg hl
;	spillPairReg hl
	ld	a, h
	adc	a, -2 (ix)
	ld	h, a
	ld	a, (hl)
	ld	-5 (ix), a
;loader.c:20: uint8_t count = src[i++];
	ld	a, c
	ld	h, b
;	spillPairReg hl
;	spillPairReg hl
	inc	bc
	add	a, -3 (ix)
	ld	l, a
;	spillPairReg hl
;	spillPairReg hl
	ld	a, h
	adc	a, -2 (ix)
	ld	h, a
	ld	a, (hl)
	ld	-1 (ix), a
;loader.c:21: while (count--) {
	ld	l, e
;	spillPairReg hl
;	spillPairReg hl
	ld	h, d
;	spillPairReg hl
;	spillPairReg hl
00101$:
	ld	a, -1 (ix)
	ld	-4 (ix), a
	dec	-1 (ix)
	ld	a, -4 (ix)
	or	a, a
	jr	Z, 00107$
;loader.c:22: *dest++ = val;
	ld	a, -5 (ix)
	ld	(hl), a
	inc	hl
	ld	e, l
	ld	d, h
	jp	00101$
00105$:
;loader.c:25: *dest++ = byte;
	ld	a, -1 (ix)
	ld	(de), a
	inc	de
	jp	00107$
00110$:
;loader.c:28: }
	ld	sp, ix
	pop	ix
	pop	hl
	pop	af
	jp	(hl)
;loader.c:30: void main(void) {
;	---------------------------------
; Function main
; ---------------------------------
_main::
	push	ix
	ld	ix,#0
	add	ix,sp
	push	af
;loader.c:42: __endasm;
	di
	im	1
	ld	sp, #0xD000
;	Change border to RED (color 4) to indicate loader is running
	ld	a, #7
	out	(#0x99), a
	ld	a, #4
	out	(#0x99), a
;loader.c:46: uint8_t *ptr = (uint8_t *)0x8000;
	ld	bc, #0x8000
;loader.c:47: uint8_t *data_start = 0;
	ld	hl, #0x0000
	ex	(sp), hl
;loader.c:50: while (ptr < (uint8_t *)0xF000) {
	ld	de, #0x8000
00106$:
	ld	a, d
	sub	a, #0xf0
	jr	NC, 00108$
;loader.c:51: if (ptr[0] == MAGIC_1 && ptr[1] == MAGIC_2 && 
	ld	a, (de)
	push	de
	pop	iy
	inc	iy
	sub	a, #0xde
	jr	NZ, 00102$
	ld	a, 0 (iy)
	sub	a, #0xad
	jr	NZ, 00102$
;loader.c:52: ptr[2] == MAGIC_3 && ptr[3] == MAGIC_4) {
	ld	l, e
;	spillPairReg hl
;	spillPairReg hl
	ld	h, d
;	spillPairReg hl
;	spillPairReg hl
	inc	hl
	inc	hl
	ld	a, (hl)
	sub	a, #0xbe
	jr	NZ, 00102$
	ld	hl, #3
	add	hl, de
	ld	a, (hl)
	sub	a, #0xef
	jr	NZ, 00102$
;loader.c:53: data_start = ptr + 4;
	ld	hl, #0x0004
	add	hl, bc
	ex	(sp), hl
;loader.c:54: break;
	jp	00108$
00102$:
;loader.c:56: ptr++;
	push	iy
	pop	de
	push	iy
	pop	bc
	jp	00106$
00108$:
;loader.c:59: if (data_start) {
	ld	a, -1 (ix)
	or	a, -2 (ix)
	jp	Z,0x4000
;loader.c:61: rle_decompress(data_start, (uint8_t *)0x4000, 0x7D3A);
	ld	hl, #0x7d3a
	push	hl
	ld	de, #0x4000
	ld	l, -2 (ix)
;	spillPairReg hl
;	spillPairReg hl
	ld	h, -1 (ix)
;	spillPairReg hl
;	spillPairReg hl
	call	_rle_decompress
;loader.c:67: __endasm;
	jp	0x4000
;loader.c:68: }
	ld	sp, ix
	pop	ix
	ret
	.area _CODE
	.area _INITIALIZER
	.area _CABS (ABS)
