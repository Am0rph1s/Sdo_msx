// Intro test - minimal, no game code
#include "msxgl.h"

void main()
{
    u8 old_bank;
    VDP_SetMode(VDP_MODE_GRAPHIC2);
    VDP_SetColor(COLOR_BLACK);
    VDP_ClearVRAM();
    VDP_SetSpriteFlag(VDP_SPRITE_SIZE_16);

    old_bank = GET_BANK_SEGMENT(1);
    SET_BANK_SEGMENT(1, 2);
    VDP_WriteVRAM_16K((const u8*)0x8000, 0x0000, 6144);
    VDP_WriteVRAM_16K((const u8*)(0x8000 + 6144), 0x1800, 768);
    VDP_WriteVRAM_16K((const u8*)(0x8000 + 6912), 0x2000, 6144);
    SET_BANK_SEGMENT(1, old_bank);

    VDP_SetSpritePositionY(0, VDP_SPRITE_DISABLE_SM1);

    while (1) Halt();
}
