// Simple Tape Loader for MSX
// Loads a compressed file from tape, decompresses it, and runs it.
// Usage: BLOAD"CAS:",R

#include <stdint.h>

// Magic number to locate compressed data
#define MAGIC_1 0xDE
#define MAGIC_2 0xAD
#define MAGIC_3 0xBE
#define MAGIC_4 0xEF

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

void main(void) {
    // Setup stack and interrupts
    __asm
    di
    im 1
    ld sp, #0xD000
    
    ; Change border to RED (color 4) to indicate loader is running
    ld a, #7
    out (#0x99), a
    ld a, #4
    out (#0x99), a
    __endasm;

    // Find magic number in memory
    // The entire file (loader + magic + data) is loaded at 0x8000.
    uint8_t *ptr = (uint8_t *)0x8000;
    uint8_t *data_start = 0;
    
    // Scan up to 0xF000 to avoid reading invalid memory
    while (ptr < (uint8_t *)0xF000) {
        if (ptr[0] == MAGIC_1 && ptr[1] == MAGIC_2 && 
            ptr[2] == MAGIC_3 && ptr[3] == MAGIC_4) {
            data_start = ptr + 4;
            break;
        }
        ptr++;
    }

    if (data_start) {
        // Decompress data to 0x4000
        rle_decompress(data_start, (uint8_t *)0x4000, 0x7D3A);
    }

    // Jump to game
    __asm
    jp 0x4000
    __endasm;
}
