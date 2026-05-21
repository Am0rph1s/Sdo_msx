// Simple Tape Loader for MSX
// Loads a compressed file from tape, decompresses it, and runs it.
// Usage: BLOAD"CAS:",R

#include <stdint.h>

// Data is loaded at 0x8000 by the second CAS block
#define DATA_ADDR 0x8000

// RLE Decompressor
void rle_decompress(uint8_t *src, uint8_t *dest, uint16_t size) {
    uint16_t i = 0;
    while (i < size) {
        uint8_t byte = src[i++];
        if (byte == 0xC0) {
            uint8_t val = src[i++];
            uint8_t count = src[i++];
            while (count--) {
                *dest++ = val;
            }
        } else {
            *dest++ = byte;
        }
    }
}

void main(void) __naked {
    __asm
    di
    im 1
    ld sp, #0xD000
    __endasm;

    // Decompress data from 0x8000 to 0x4000
    // Size is approx 32KB, we use a large value
    rle_decompress((uint8_t *)DATA_ADDR, (uint8_t *)0x4000, 0x7D3A); // 32058 bytes placeholder

    // Jump to game
    __asm
    jp 0x4000
    __endasm;
}
