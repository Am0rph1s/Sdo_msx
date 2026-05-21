// Intro independent - sense joc
#include "msxgl.h"
#include "psg.h"

// Trampoline code that will be copied to RAM
// Must be position-independent
static const u8 g_Trampoline[] = {
    0xF3,              // DI
    0x3E, 0x02,        // LD A, 2
    0x32, 0x00, 0x60,  // LD (0x6000), A   ; ASCII16 Page 1 (4000-5FFF) -> seg 2
    0x32, 0x01, 0x60,  // LD (0x6001), A   ; ASCII16 Page 2 (6000-7FFF) -> seg 2
    0x3E, 0x03,        // LD A, 3
    0x32, 0x02, 0x60,  // LD (0x6002), A   ; ASCII16 Page 3 (8000-9FFF) -> seg 3
    0x32, 0x03, 0x60,  // LD (0x6003), A   ; ASCII16 Page 4 (A000-BFFF) -> seg 3
    0xFB,              // EI
    0xC3, 0x14, 0x40   // JP 0x4014         ; Jump to game entry point
};

//=============================================================================
// INTRO MUSIC - exact port of CPC loader.s loading_music[] (lines 583-661)
// CPC AY clock = 1MHz, MSX AY clock = 3.58MHz -> scale periods x3.58
// Each CPC step: .dw duration, .db A_lo,A_hi, B_lo,B_hi, C_lo,C_hi
// Volume from mt_apply_chs: A=11, B=10, C=7
//=============================================================================

// 39 steps parsed from loader.s, periods scaled x3.58 for MSX
static const u16 g_MusicDur[39] = {
    16,15,15,16,15,15,16,20,16,15,15,16,15,15,16,20,
    16,15,15,16,15,15,16,20,16,15,15,16,15,15,16,20,
    18,16,15,16,18,25,0
};
static const u16 g_MusicA[39] = {
    1360,1360,1360,856,1017,1078,1360,1360,1360,1360,1360,856,1017,1078,1360,1360,
    856,856,856,763,1017,1078,1360,1360,856,856,856,763,1017,1078,1360,1360,
    763,856,1017,1078,1360,1360,0
};
static const u16 g_MusicB[39] = {
    2721,2721,2721,2284,2284,2284,2721,2721,2721,2721,2721,2284,2284,2284,2721,2721,
    2284,2284,2284,2721,2284,2284,2721,2721,2284,2284,2284,2721,2284,2284,2721,2721,
    2721,2721,2721,2284,2721,2721,0
};
static const u16 g_MusicC[39] = {
    1210,1210,1210,1439,1360,1142,1210,1210,1210,1210,1210,1439,1360,1142,1210,1210,
    1439,1439,1439,1525,1360,1142,1210,1210,1439,1439,1439,1525,1360,1142,1210,1210,
    1525,1439,1360,1142,1210,1210,0
};
#define MUSIC_NSTEPS 39
#define MUSIC_VOL_A 11
#define MUSIC_VOL_B 10
#define MUSIC_VOL_C 7

static void MusicTick(u8 step)
{
    u16 per;

    // Channel A
    per = g_MusicA[step];
    PSG_SetRegister(0, (u8)(per & 0xFF));
    PSG_SetRegister(1, (u8)((per >> 8) & 0x0F));
    PSG_SetRegister(8, MUSIC_VOL_A);

    // Channel B
    per = g_MusicB[step];
    PSG_SetRegister(2, (u8)(per & 0xFF));
    PSG_SetRegister(3, (u8)((per >> 8) & 0x0F));
    PSG_SetRegister(9, MUSIC_VOL_B);

    // Channel C
    per = g_MusicC[step];
    PSG_SetRegister(4, (u8)(per & 0xFF));
    PSG_SetRegister(5, (u8)((per >> 8) & 0x0F));
    PSG_SetRegister(10, MUSIC_VOL_C);

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
    u16 mFrame;

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

    // Music + wait loop (exact CPC loader.s timing: 1 step per VSYNC, looped)
    mStep = 0;
    while (1)
    {
        MusicTick(mStep);
        mFrame = 0;
        while (mFrame < g_MusicDur[mStep])
        {
            Halt();
            mFrame++;

            // Check for key press
            u8 kbd = Keyboard_Read(8);
            if (IS_KEY_PRESSED(kbd, KEY_SPACE)) goto music_done;
            kbd = Keyboard_Read(5);
            if (IS_KEY_PRESSED(kbd, KEY_Z)) goto music_done;
            if ((Joystick_Read(JOY_PORT_1) & JOY_INPUT_TRIGGER_A) == 0) goto music_done;
        }
        mStep++;
        if (g_MusicDur[mStep] == 0) { mStep = 0; }  // Loop on terminator
    }

music_done:

    MusicSilence();

    // Copy trampoline to RAM
    dst = (u8*)0xC000;
    for (i = 0; i < sizeof(g_Trampoline); i++)
        dst[i] = g_Trampoline[i];

    // Jump to trampoline (in RAM)
    ((void (*)())0xC000)();
}
