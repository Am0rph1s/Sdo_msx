// Intro only - test pur
#include "msxgl.h"
#include "compress/pletter.h"
#include "intro_plet.h"

void main()
{
    VDP_SetMode(VDP_MODE_GRAPHIC2);
    VDP_SetColor(COLOR_BLACK);
    VDP_ClearVRAM();
    VDP_SetSpriteFlag(VDP_SPRITE_SIZE_16);

    Pletter_UnpackToVRAM(g_IntroPlet, 0x0000);

    VDP_SetSpritePositionY(0, VDP_SPRITE_DISABLE_SM1);

    while (1)
    {
        Halt();
        if (Keyboard_Read(8) || Keyboard_Read(5))
            break;
    }
}
