// Intro independent - sense joc
#include "msxgl.h"
#include "compress/pletter.h"

// Trampoline code that will be copied to RAM
// Must be position-independent
static const u8 g_Trampoline[] = {
    0x3E, 0x02,          // LD A, 2
    0x32, 0x00, 0x60,    // LD (0x6000), A   ; Bank 0 -> segment 2
    0x3E, 0x03,          // LD A, 3
    0x32, 0xFF, 0x77,    // LD (0x77FF), A   ; Bank 1 -> segment 3
    0xC3, 0x14, 0x40     // JP 0x4014         ; Jump to game entry point
};

void main()
{
    u16 t;
    u8 i;
    u8* dst;

    // Init VDP
    VDP_SetMode(VDP_MODE_GRAPHIC2);
    VDP_SetColor(COLOR_BLACK);
    VDP_ClearVRAM();
    VDP_SetSpriteFlag(VDP_SPRITE_SIZE_16);

    // Bank 1 ja mostra segment 1 (SC2 data per defecte)
    Pletter_UnpackToVRAM((const u8*)0x8000, 0x0000);

    // Disable sprites
    for (i = 0; i < 32; i++)
        VDP_SetSpritePositionY(i, VDP_SPRITE_DISABLE_SM1);

    // Wait for key
    t = 0;
    while (t < 300)
    {
        Halt();
        if (IS_KEY_PRESSED(Keyboard_Read(8), KEY_SPACE) ||
            IS_KEY_PRESSED(Keyboard_Read(5), KEY_Z))
            break;
        t++;
    }

    // Copy trampoline to RAM
    dst = (u8*)0xC000;
    for (i = 0; i < sizeof(g_Trampoline); i++)
        dst[i] = g_Trampoline[i];

    // Jump to trampoline (in RAM)
    ((void (*)())0xC000)();
}
