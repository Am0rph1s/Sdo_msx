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
// INTRO MUSIC - simple looping arpeggio for MSX PSG
// AY-3-8910: Freq = 3579545 / (16 * Period)
// Periods calculated for pleasant notes
//=============================================================================

// Note periods for MSX (3.58MHz PSG)
// C3=956, D3=852, E3=758, F3=716, G3=638, A3=568, B3=506
// C4=478, D4=426, E4=379, F4=358, G4=319, A4=284, B4=253
// C5=239, D5=213, E5=190, G5=160, A5=142

static const u16 g_MusicDur[38] = {
    20,20,20,20, 20,20,20,20,
    20,20,20,20, 20,20,20,20,
    20,20,20,20, 20,20,20,20,
    20,20,20,20, 20,20,20,20,
    40,40,40,40, 0,0
};
static const u16 g_MusicA[38] = {
    478,478,478,478,
    319,319,319,319,
    284,284,284,284,
    358,358,358,358,
    478,478,478,478,
    319,319,319,319,
    478,426,379,358,
    478,478,478,478,
    956,956,956,956, 0,0
};
static const u16 g_MusicB[38] = {
    956,956,956,956,
    638,638,638,638,
    568,568,568,568,
    716,716,716,716,
    956,956,956,956,
    638,638,638,638,
    956,852,758,716,
    956,956,956,956,
    478,478,478,478, 0,0
};
static const u16 g_MusicC[38] = {
    716,716,716,716,
    478,478,478,478,
    426,426,426,426,
    538,538,538,538,
    716,716,716,716,
    478,478,478,478,
    716,638,568,538,
    716,716,716,716,
    0,0,0,0, 0,0
};
#define MUSIC_NSTEPS 38
#define MUSIC_VOL_A 10
#define MUSIC_VOL_B 8
#define MUSIC_VOL_C 5

static void MusicTick(u8 step)
{
    u16 per;
    if (step >= MUSIC_NSTEPS) return;

    // Channel A
    per = g_MusicA[step];
    PSG_SetRegister(0, (u8)(per & 0xFF));
    PSG_SetRegister(1, (u8)((per >> 8) & 0x0F));
    PSG_SetRegister(8, per ? MUSIC_VOL_A : 0);

    // Channel B
    per = g_MusicB[step];
    PSG_SetRegister(2, (u8)(per & 0xFF));
    PSG_SetRegister(3, (u8)((per >> 8) & 0x0F));
    PSG_SetRegister(9, per ? MUSIC_VOL_B : 0);

    // Channel C
    per = g_MusicC[step];
    PSG_SetRegister(4, (u8)(per & 0xFF));
    PSG_SetRegister(5, (u8)((per >> 8) & 0x0F));
    PSG_SetRegister(10, per ? MUSIC_VOL_C : 0);

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
    u8 mStep = 0;
    u16 mFrame = 0;

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

    // Music + wait loop (infinite until key pressed)
    MusicTick(0);
    while (1)
    {
        Halt();
        mFrame++;
        if (mFrame >= g_MusicDur[mStep])
        {
            mFrame = 0;
            mStep++;
            if (g_MusicDur[mStep] == 0) { mStep = 0; }  // Loop on terminator
            MusicTick(mStep);
        }
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
