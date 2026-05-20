// Intro independent - ASM pur, sense MSXgl
#include "msxgl.h"
#include "compress/pletter.h"

extern const u8 g_IntroPlet[];

// Trampoline code that will be copied to RAM
// Must be position-independent
// Sets up banks: bank0->segment2 (game_p1), bank1->segment3 (game_p2)
// Then jumps to game entry point at 0x4014
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

    // Inicialitza VDP
    VDP_SetMode(VDP_MODE_GRAPHIC2);
    VDP_SetColor(COLOR_BLACK);
    VDP_ClearVRAM();
    VDP_SetSpriteFlag(VDP_SPRITE_SIZE_16);

    // Carrega intro des del segment 1 (intro_sc2.plet)
    SET_BANK_SEGMENT(2, 1);
    Pletter_UnpackToVRAM(g_IntroPlet, 0x0000);

    // Desactiva sprites
    for (i = 0; i < 32; i++)
        VDP_SetSpritePositionY(i, VDP_SPRITE_DISABLE_SM1);

    // Espera
    t = 0;
    while (t < 300)
    {
        Halt();
        if (IS_KEY_PRESSED(Keyboard_Read(8), KEY_SPACE) ||
            IS_KEY_PRESSED(Keyboard_Read(5), KEY_Z))
            break;
        t++;
    }

    // Copia trampoline a RAM
    dst = (u8*)0xC000;
    for (i = 0; i < sizeof(g_Trampoline); i++)
        dst[i] = g_Trampoline[i];

    // Salta al trampoline (en RAM)
    ((void (*)())0xC000)();
}
