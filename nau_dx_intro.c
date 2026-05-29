// Intro independent - sense joc
#include "msxgl.h"
#include "psg.h"

// BIOS GTTRIG (0x00D8) — direct CALL, compatible amb MSX1
static u8 BiosTrig(u8 n)
{
    __asm
        ld hl, #2
        add hl, sp
        ld a, (hl)
        call 0x00D8
        ld l, a
        ld h, #0
    __endasm;
}

// Trampoline code that will be copied to RAM
// Must be position-independent
static const u8 g_Trampoline[] = {
    0xF3,              // DI
    0x3E, 0x03,        // LD A, 3
    0x32, 0x00, 0x50,  // LD (0x5000), A   ; SCC Bank 0 (4000-5FFF) -> seg 3 (game_p1 first 8KB)
    0x3E, 0x04,        // LD A, 4
    0x32, 0x00, 0x70,  // LD (0x7000), A   ; SCC Bank 1 (6000-7FFF) -> seg 4 (game_p1 second 8KB)
    0x3E, 0x05,        // LD A, 5
    0x32, 0x00, 0x90,  // LD (0x9000), A   ; SCC Bank 2 (8000-9FFF) -> seg 5 (game_p2 first 8KB)
    0x3E, 0x06,        // LD A, 6
    0x32, 0x00, 0xB0,  // LD (0xB000), A   ; SCC Bank 3 (A000-BFFF) -> seg 6 (game_p2 second 8KB)
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

    // Mixer: enable all 3 tone channels, disable noise, Port A/B=input (safe for old HW)
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
    // Seg 1 at bank 1 (0x6000-0x7FFF): pattern table (8192 bytes)
    src = (const u8*)0x6000;
    VDP_WriteVRAM(src, 0x0000, 0x0000, 8192);
    // Seg 2 at bank 2 (0x8000-0x9FFF): color table (6144 bytes)
    src = (const u8*)0x8000;
    VDP_WriteVRAM(src, 0x2000, 0x0000, 6144);

    // Disable sprites
    for (i = 0; i < 32; i++)
        VDP_SetSpritePositionY(i, VDP_SPRITE_DISABLE_SM1);

    // Music + wait loop (exact CPC loader.s timing: 1 step per VSYNC, looped)
    mStep = 0;
    while (BiosTrig(1) != 0)
        Halt();
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
            if (BiosTrig(1) != 0) goto music_done;
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


