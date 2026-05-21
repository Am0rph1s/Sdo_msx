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

void main(void) __naked {
    __asm
    di
    im 1
    ld sp, #0xD000
    __endasm;

    // Find magic number in memory
    // We start searching from 0x8000 (where the loader is loaded)
    uint8_t *ptr = (uint8_t *)0x8000;
    uint8_t *data_start = 0;
    
    // Scan up to 32KB
    while (ptr < (uint8_t *)0xFFFF) {
        if (ptr[0] == MAGIC_1 && ptr[1] == MAGIC_2 && 
            ptr[2] == MAGIC_3 && ptr[3] == MAGIC_4) {
            data_start = ptr + 4;
            break;
        }
        ptr++;
    }

    if (data_start) {
        // Decompress data to 0x4000
        // Size is approx 32KB, we can use a large value or calculate it
        // Since the file ends after data, we can just decompress until error or use fixed size
        // The compressed data size is roughly 32KB.
        rle_decompress(data_start, (uint8_t *)0x4000, 0x7D3A); // 32058 bytes placeholder
    }

    // Jump to game
    __asm
    jp 0x4000
    __endasm;
}
