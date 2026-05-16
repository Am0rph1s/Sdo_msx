// Intro independent - ASM pur, sense MSXgl
#include "msxgl.h"
#include "compress/pletter.h"

extern const u8 g_IntroPlet[];
void GameMain();

void main()
{
    // Inicialitza VDP
    VDP_SetMode(VDP_MODE_GRAPHIC2);
    VDP_SetColor(COLOR_BLACK);
    VDP_ClearVRAM();
    VDP_SetSpriteFlag(VDP_SPRITE_SIZE_16);

    // Carrega intro
    SET_BANK_SEGMENT(2, 4);
    Pletter_UnpackToVRAM(g_IntroPlet, 0x0000);
    SET_BANK_SEGMENT(2, 2);

    // Espera
    {
        u16 t = 200;
        while (t)
        {
            Halt();
            if (Keyboard_Read(8) || Keyboard_Read(5)) break;
            t--;
        }
    }

    // Llença el joc
    GameMain();
}

void GameMain();
