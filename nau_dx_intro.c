// Intro independent - sense joc
#include "msxgl.h"
#include "psg.h"

// Trampoline code that will be copied to RAM
// Must be position-independent
static const u8 g_Trampoline[] = {
    0x3E, 0x02,          // LD A, 2
    0x32, 0x00, 0x60,    // LD (0x6000), A   ; Bank 0 -> segment 2
    0x3E, 0x03,          // LD A, 3
    0x32, 0xFF, 0x77,    // LD (0x77FF), A   ; Bank 1 -> segment 3
    0xC3, 0x14, 0x40     // JP 0x4014         ; Jump to game entry point
};

//=============================================================================
// INTRO MUSIC - exact port of CPC playMenuCoin() from main_menu.c
// CPC AY clock = 1MHz, MSX AY clock = 3.58MHz -> scale factor = 3.58
// Original CPC periods: A={239,190,160,119}, B={190,160,119,95}, C={160,119,95,80}
// Scaled for MSX: x3.58
//=============================================================================

static const u16 g_MusicA[16] = {856,856,856, 680,680,680, 573,573,573, 426,426,426,426,426,426,426};
static const u16 g_MusicB[16] = {680,680,680, 573,573,573, 426,426,426, 340,340,340,340,340,340,340};
static const u16 g_MusicC[16] = {573,573,573, 426,426,426, 340,340,340, 286,286,286,286,286,286,286};
#define MUSIC_NSTEPS 16
#define MUSIC_VOL_A 15
#define MUSIC_VOL_B 12
#define MUSIC_VOL_C 9

static void MusicTick(u8 step)
{
    u16 per;

    // Channel A
    per = g_MusicA[step];
    PSG_SetRegister(0, (u8)(per & 0xFF));
    PSG_SetRegister(1, (u8)((per >> 8) & 0x0F));
    PSG_SetRegister(8, (step < 12) ? MUSIC_VOL_A : 8);

    // Channel B
    per = g_MusicB[step];
    PSG_SetRegister(2, (u8)(per & 0xFF));
    PSG_SetRegister(3, (u8)((per >> 8) & 0x0F));
    PSG_SetRegister(9, (step < 12) ? MUSIC_VOL_B : 5);

    // Channel C
    per = g_MusicC[step];
    PSG_SetRegister(4, (u8)(per & 0xFF));
    PSG_SetRegister(5, (u8)((per >> 8) & 0x0F));
    PSG_SetRegister(10, (step < 12) ? MUSIC_VOL_C : 2);

    // Mixer: enable all 3 tone channels, disable noise
    PSG_SetRegister(7, 0x38);
}

static void MusicSilence(void)
{
    PSG_SetRegister(8, 0);
    PSG_SetRegister(9, 0);
    PSG_SetRegister(10, 0);
    PSG_SetRegister(7, 0x3F);
}

void main()
{
    u8 i;
    u8* dst;
    const u8* src;
    u8 mStep;

    // Init VDP
    VDP_SetMode(VDP_MODE_GRAPHIC2);
    VDP_SetColor(COLOR_BLACK);
    VDP_ClearVRAM();
    VDP_SetSpriteFlag(VDP_SPRITE_SIZE_16);

    // Copy raw SC2 data directly to VRAM (no compression)
    // Raw data is at ROM address 0x8000 (segment 1), 16384 bytes
    // Pattern table: 8192 bytes at VRAM 0x0000
    src = (const u8*)0x8000;
    VDP_WriteVRAM(src, 0x0000, 0x0000, 8192);
    // Color table: 6144 bytes at VRAM 0x2000
    src = (const u8*)0xA000;
    VDP_WriteVRAM(src, 0x2000, 0x0000, 6144);

    // Disable sprites
    for (i = 0; i < 32; i++)
        VDP_SetSpritePositionY(i, VDP_SPRITE_DISABLE_SM1);

    // Music + wait loop (exact CPC playMenuCoin timing: 1 step per VSYNC, looped)
    mStep = 0;
    while (1)
    {
        MusicTick(mStep);
        mStep++;
        if (mStep >= MUSIC_NSTEPS) { mStep = 0; }

        Halt();  // Wait for VSYNC

        // Check for key press
        u8 kbd = Keyboard_Read(8);
        if (IS_KEY_PRESSED(kbd, KEY_SPACE)) break;
        kbd = Keyboard_Read(5);
        if (IS_KEY_PRESSED(kbd, KEY_Z)) break;
        // Also check joystick
        if ((Joystick_Read(JOY_PORT_1) & JOY_INPUT_TRIGGER_A) == 0) break;
    }

    MusicSilence();

    // Copy trampoline to RAM
    dst = (u8*)0xC000;
    for (i = 0; i < sizeof(g_Trampoline); i++)
        dst[i] = g_Trampoline[i];

    // Jump to trampoline (in RAM)
    ((void (*)())0xC000)();
}
