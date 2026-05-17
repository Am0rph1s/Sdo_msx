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
// INTRO MUSIC - converted from CPC loader.s (3-channel PSG melody)
// CPC clock ~1.77MHz, MSX ~3.58MHz -> periods scaled x2
// Volumes: A=11, B=10, C=7 (from mt_apply_chs)
//=============================================================================

// Music data - extracted from CPC loader.s, periods scaled x2 for MSX
static const u16 g_MusicDur[46] = {
    16,15,15,16,15,15,16,20,16,15,15,16,20,16,16,16,
    15,15,16,20,16,16,16,15,15,16,20,18,16,15,16,18,
    25,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static const u16 g_MusicA[46] = {
    248,248,248,478,56,90,248,248,248,248,248,478,56,90,248,248,
    478,478,478,426,56,90,248,478,478,478,426,56,90,248,426,478,
    248,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static const u16 g_MusicB[46] = {
    496,496,496,252,252,252,496,496,496,496,496,252,252,252,496,496,
    252,252,252,496,252,252,496,252,252,252,496,496,252,496,252,252,
    496,0,0,0,0,0,0,0,0,0,0,0,0,0
};
static const u16 g_MusicC[46] = {
    164,164,164,292,248,126,164,164,164,164,164,292,248,126,164,164,
    292,292,292,340,248,126,164,292,292,292,340,248,126,164,340,292,
    164,0,0,0,0,0,0,0,0,0,0,0,0,0
};
#define MUSIC_NSTEPS 46
#define MUSIC_VOL_A 11
#define MUSIC_VOL_B 10
#define MUSIC_VOL_C 7

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
    u16 t;
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

    // Music + wait loop
    MusicTick(0);
    t = 0;
    while (t < 300)
    {
        Halt();
        mFrame++;
        if (mFrame >= g_MusicDur[mStep])
        {
            mFrame = 0;
            mStep++;
            if (mStep >= MUSIC_NSTEPS) { mStep = 0; }  // Loop
            MusicTick(mStep);
        }
        if (IS_KEY_PRESSED(Keyboard_Read(8), KEY_SPACE) ||
            IS_KEY_PRESSED(Keyboard_Read(5), KEY_Z))
            break;
        t++;
    }

    MusicSilence();

    // Copy trampoline to RAM
    dst = (u8*)0xC000;
    for (i = 0; i < sizeof(g_Trampoline); i++)
        dst[i] = g_Trampoline[i];

    // Jump to trampoline (in RAM)
    ((void (*)())0xC000)();
}
