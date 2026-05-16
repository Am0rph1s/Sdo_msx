// Nau DX - Intro screen standalone test
#include "msxgl.h"

void LoadIntroScreen() __banked;

void main()
{
    VDP_SetMode(VDP_MODE_GRAPHIC2);
    VDP_SetColor(COLOR_BLACK);
    VDP_ClearVRAM();
    VDP_SetSpriteFlag(VDP_SPRITE_SIZE_16);

    LoadIntroScreen();

    VDP_SetSpritePositionY(0, VDP_SPRITE_DISABLE_SM1);

    while (1)
    {
        Halt();
        if (Keyboard_Read(8) || Keyboard_Read(5))
            break;
    }
}
