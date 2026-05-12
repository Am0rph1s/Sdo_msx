// Nau DX - MSX1 Port using MSXgl

#include "msxgl.h"

//=============================================================================
// CONSTANTS
//=============================================================================

#define SCREEN_W        256
#define SCREEN_H        192

// Layout: [col_L 16px][game 144px][col_R 16px][HUD 80px]
#define GAME_X0         16
#define GAME_X1         160
#define GAME_W          144
#define GAME_Y0         0
#define GAME_H          SCREEN_H

#define SHIP_W          16
#define SHIP_H          16
#define SHIP_MIN_X      GAME_X0
#define SHIP_MAX_X      (GAME_X1 - SHIP_W)
#define SHIP_MIN_Y      8
#define SHIP_MAX_Y      (SCREEN_H - SHIP_H - 8)

#define MAX_SHOTS       4
#define SHOT_SPEED      4
#define FIRE_COOLDOWN   10

// Starfield - 3 layers
#define N1              4    // slow stars
#define N2              7    // medium stars
#define N3              11   // fast stars
#define STAR_X0         (GAME_X0 + 4)
#define STAR_X1         (GAME_X1 - 4)
#define STAR_SPEED1     1    // 1px every 4 frames
#define STAR_SPEED2     1    // 1px every 2 frames
#define STAR_SPEED3     2    // 2px every 2 frames

// Enemy types - 1:1 CPC
#define ENEMY_TYPE_BASIC   0
#define ENEMY_TYPE_FAST    1
#define ENEMY_TYPE_HEAVY   2
#define ENEMY_TYPE_DIVER   3
#define ENEMY_TYPE_BOMBER  4

// Enemy speeds - 1:1 CPC (adaptats a MSX: CPC té 50Hz i pixels més grans)
#define ENEMY_SPEED_BASIC   1
#define ENEMY_SPEED_FAST    2
#define ENEMY_SPEED_HEAVY   1
#define ENEMY_SPEED_DIVER   2
#define ENEMY_SPEED_BOMBER  1

// Enemy scores - 1:1 CPC
#define ENEMY_SCORE_BASIC   10
#define ENEMY_SCORE_FAST    20
#define ENEMY_SCORE_HEAVY   50
#define ENEMY_SCORE_DIVER   40
#define ENEMY_SCORE_BOMBER  80

// Movement patterns - 1:1 CPC
#define PATT_STRAIGHT  0
#define PATT_ZIGZAG    1
#define PATT_DIAGONAL  2

// Wave system - 1:1 CPC
#define WAVE_PLAN_MAX       8
#define WAVE_MODE_RANK      0
#define WAVE_MODE_INDIAN    1
#define SPAWN_BASE          2
#define SPAWN_VARIANCE      5
#define SPAWN_FIRST_DELAY   4
#define SERIAL_DELAY        8
#define ENDGAME_FINAL_LEVEL 25
#define EXTRA_LIFE_EVERY    5000

// Level masks - 1:1 CPC
#define LMASK_BASIC   ((u8)(1u<<0))
#define LMASK_FAST    ((u8)(1u<<1))
#define LMASK_HEAVY   ((u8)(1u<<2))
#define LMASK_DIVER   ((u8)(1u<<3))
#define LMASK_BOMBER  ((u8)(1u<<4))
#define LCFG_F_BOSS1  ((u8)1u)
#define LCFG_F_BOSS2  ((u8)2u)

// Enemies
#define MAX_ENEMIES        8   // CPC: WAVE_PLAN_MAX=8
#define ENEMY_W            12
#define ENEMY_H            12
#define ENEMYSHOT_W        2
#define ENEMYSHOT_H        6
#define ENEMYSHOT_SPEED_Y  3
#define ENEMYSHOT_COOLDOWN 60

// Explosions
#define MAX_EXPLOSIONS     4
#define EXP_KIND_ENEMY     0
#define EXP_KIND_SHIP      1

// Enemy shots
#define MAX_ENEMY_SHOTS    6
#define SHIP_HIT_W         12
#define SHIP_HIT_H         12
#define SHIP_HIT_OX        2
#define SHIP_HIT_OY        2

// Respawn - valors 1:1 del CPC
#define SHIP_EXPL_TIMER    12   // CPC: ship_expl_timer = 12
#define RESPAWN_INVUL_TICKS 40  // CPC: RESPAWN_INVUL_TICKS = 40

// Game states - 1:1 CPC
#define GS_TITLE    0
#define GS_PLAYING  1

// Title screen modes - 1:1 CPC
#define TS_MENU          0
#define TS_HISCORE_VIEW  1
#define TS_HISCORE_INPUT 2
#define TS_ATTRACT_SCORE 3
#define TS_HELP          4
#define TS_WIN           5
#define TS_REDEFINE      6
#define ATTRACT_HOLD_FRAMES 300

// HUD font colors (MSX1 TMS9918)
// IMPORTANT: Mode 2 - cada tile te 1 color global -> necessitem tiles separats per color
#define HUD_FONT_TILE_BASE  130  // tiles 130-166: font en VERD (normal)
#define HUD_FONT_TILE_HI    167  // tiles 167-203: font en BLANC (hi)
#define HUD_FONT_COLOR_NRM  0x31  // Light Green (3) on Black (1)
#define HUD_FONT_COLOR_HI   0xF1  // White (15) on Black (1)
#define HUD_FONT_COLOR_CYN  0x71  // Cyan (7) on Black

// Hi-scores
#define HISCORE_COUNT    3

// Wall scroll
#define WALL_TILE_BASE     200
#define WALL_SPEED         4

//=============================================================================
// DATA STRUCTURES
//=============================================================================

typedef struct { u8 x; u8 y; } TStar;
typedef struct { u8 x; u8 y; u8 active; } TShot;
typedef struct { u8 x; u8 y; u8 active; u8 type; u8 fire_cd; i8 vx; u8 vy; u8 pattern; u8 zig_timer; u8 health; } TEnemy;
typedef struct { u8 x; u8 y; u8 active; i8 vx; } TEnemyShot;
typedef struct { u8 x; u8 y; u8 active; u8 frame; u8 kind; } TExplosion;
typedef struct { u16 score; u8 level; u8 name[3]; } THiScore;
typedef struct { u8 waves; u8 per_wave; u8 mask; u8 flags; u8 intro_mask; } TLevelConfig;

//=============================================================================
// GAME STATE
//=============================================================================

static u8  g_ShipX, g_ShipY;
static u8  g_ShipSpeedX = 3;
static u8  g_ShipSpeedY = 3;
static u8  g_ShipExploding  = 0;
static u8  g_ShipExplTimer  = 0;
static u8  g_ShipInvul      = 0;
static u8  g_ShipInvulTimer = 0;
static u8  g_ShipLastLife   = 0;  // 1:1 CPC: ship_last_life
static u8  g_GameOverDelay  = 0;
static u8  g_FrameCount     = 0;

// Game state
static u8  g_GameState      = GS_TITLE;

// Menu state - 1:1 CPC
static u8  g_TitleMode      = TS_MENU;
static u8  g_TitleDirty     = 1;
static u8  g_TitlePhase     = 0;
static u16 g_AttractFrm     = 0;
static u8  g_AttractCycle   = 0;
static u8  g_BlinkCtr       = 0;

// Hi-scores - 1:1 CPC (Top 3)
static THiScore g_HiScores[HISCORE_COUNT];
static u8  g_HsPos          = 0;
static u8  g_HsInputPos     = 0;
static u8  g_HsInputChar[3];
static u16 g_LastScore      = 0;
static u8  g_LastLevel      = 1;

// SET KEYS - 1:1 CPC redefine
static u8  g_RedefineStep   = 0;
// Tecles actuals (fila,bit) - valors per defecte = fletxes + Z + P + Return
// KEY_LEFT=row8b4, KEY_RIGHT=row8b7, KEY_UP=row8b5, KEY_DOWN=row8b6
// KEY_Z=row5b7, KEY_P=row9b?, KEY_RETURN=row7b?
static u8  g_KeyLeft   = KEY_LEFT;
static u8  g_KeyRight  = KEY_RIGHT;
static u8  g_KeyUp     = KEY_UP;
static u8  g_KeyDown   = KEY_DOWN;
static u8  g_KeyFire   = KEY_Z;
static u8  g_LastKeyPressed = 0;
static TShot      g_Shots[MAX_SHOTS];
static u8         g_FireCooldown = 0;
static TEnemy     g_Enemies[MAX_ENEMIES];
static TEnemyShot g_EnemyShots[MAX_ENEMY_SHOTS];
static TExplosion g_Explosions[MAX_EXPLOSIONS];
static u16        g_Score = 0;
static u8         g_Lives = 3;
static u8         g_Level = 1;
static u8         g_HudDirty = 1;

// Wave system state - 1:1 CPC
static u8  g_WaveActive    = 0;
static u8  g_WaveTotal     = 0;
static u8  g_WaveSpawned   = 0;
static u8  g_WaveKilled    = 0;
static u8  g_WavesCleared  = 0;
static u8  g_WaveMode      = WAVE_MODE_RANK;
static u8  g_SpawnTimer    = SPAWN_FIRST_DELAY;
static u8  g_WaveIndianDelay = 0;
static u16 g_WaveBonusBase = 10;
static u8  g_WaveSlotX[WAVE_PLAN_MAX];
static u8  g_WaveSlotY[WAVE_PLAN_MAX];
static u8  g_WaveSlotType[WAVE_PLAN_MAX];
static u8  g_WaveUnifiedPatt = PATT_STRAIGHT;
static i8  g_WaveUnifiedVX   = 0;

#define SHIP_SPAWN_X  (GAME_X0 + GAME_W / 2 - SHIP_W / 2)
#define SHIP_SPAWN_Y  (SCREEN_H - SHIP_H - 8)
// Valors 1:1 del CPC (la segona definicio, eliminem la duplicada)

// Starfield
static TStar g_S1[N1];
static TStar g_S2[N2];
static TStar g_S3[N3];
static u8 g_StarTimer1 = 0;

// Wall scroll
static u8 g_WallPhase = 0;
static u8 g_WallPhaseTimer = 0;

//=============================================================================
// SPRITE DATA
//=============================================================================

// Ship sprite (white + red overlay)
static const u8 g_SpriteWhite[32] = {
    0x01, 0x01, 0x03, 0x02, 0x02, 0x07, 0x07, 0x0F,
    0x18, 0x38, 0x64, 0x80, 0xFF, 0x1F, 0x00, 0x00,
    0x80, 0x80, 0xC0, 0x40, 0x40, 0xE0, 0xE0, 0xF0,
    0x18, 0x1C, 0x22, 0x01, 0xFF, 0xF8, 0x00, 0x00
};
static const u8 g_SpriteRed[32] = {
    0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00,
    0x07, 0x07, 0x3B, 0x7F, 0x00, 0x00, 0x0C, 0x0C,
    0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00,
    0xE0, 0xE0, 0xDC, 0xFE, 0x00, 0x00, 0x30, 0x30
};

// Shot sprite (8x8) - centrat en slot 16x16
// Els primers 8 bytes son el patró, els altres 24 zeros
static const u8 g_ShotPattern[8] = {
    0x18, 0x3C, 0x3C, 0x3C, 0x18, 0x18, 0x18, 0x00
};

// Enemy shot sprite (16x16) - 2px ample x 6px alt, des de fila 0
// Centrat a px 7,8 (bit0 left byte + bit7 right byte)
static const u8 g_EnemyShotPattern[32] = {
    0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x80,0x80,0x80,0x80,0x80,0x80,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

// Explosion sprites (16x16) - creu radial que creix/decreix com al CPC
// Radi enemic: frames 0,1,2,1 (radis 1,2,3,2)
// Radi nau:    frames 0,1,2,3,2,1 (radis 1,2,3,4,3,2)
static const u8 g_ExpSprite0[32] = { // radi 1
    0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x03,
    0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x80,
    0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
static const u8 g_ExpSprite1[32] = { // radi 2
    0x00,0x00,0x00,0x00,0x00,0x03,0x07,0x07,
    0x07,0x03,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x80,0xC0,0xC0,
    0xC0,0x80,0x00,0x00,0x00,0x00,0x00,0x00
};
static const u8 g_ExpSprite2[32] = { // radi 3
    0x00,0x00,0x00,0x00,0x03,0x03,0x0F,0x0F,
    0x0F,0x03,0x03,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x80,0x80,0xE0,0xE0,
    0xE0,0x80,0x80,0x00,0x00,0x00,0x00,0x00
};
static const u8 g_ExpSprite3[32] = { // radi 4 (nau only)
    0x00,0x00,0x00,0x03,0x03,0x03,0x1F,0x1F,
    0x1F,0x03,0x03,0x03,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x80,0x80,0x80,0xF0,0xF0,
    0xF0,0x80,0x80,0x80,0x00,0x00,0x00,0x00
};

// Enemy sprite - converted from CPC 12x12, centered in 16x16 (2px padding each side)
// MSX format: first 16 bytes = left half (px 0-7) per row, next 16 = right half (px 8-15)
// White layer: C (blue->white), W (white), Y (yellow->white)
static const u8 g_EnemyWhite[32] = {
    0x00,0x00,0x00,0x07,0x0F,0x1F,0x33,0x27,
    0x0C,0x0F,0x07,0x03,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0xE0,0xF0,0xF8,0xCC,0xE4,
    0x30,0xF0,0xE0,0xC0,0x00,0x00,0x00,0x00
};
static const u8 g_EnemyRed[32] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x0C,0x18,
    0x33,0x30,0x18,0x0C,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x18,
    0xCC,0x0C,0x18,0x30,0xC0,0x00,0x00,0x00
};

static const u8 g_EnemyFastWhite[32] = {
    0x00,0x00,0x00,0x03,0x07,0x0F,0x07,0x0F,
    0x1E,0x0F,0x07,0x01,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0xC0,0xE0,0xF0,0xE0,0xF0,
    0x78,0xF0,0xE0,0x80,0x00,0x00,0x00,0x00
};
static const u8 g_EnemyFastRed[32] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x01,0x00,0x00,0x06,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x80,0x00,0x00,0x60,0xC0,0x00,0x00,0x00
};

static const u8 g_EnemyHeavyWhite[32] = {
    0x00,0x00,0x00,0x0F,0x1F,0x3F,0x27,0x0F,
    0x0C,0x0C,0x0F,0x07,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0xF0,0xF8,0xFC,0xE4,0xF0,
    0x30,0x30,0xF0,0xE0,0x00,0x00,0x00,0x00
};
static const u8 g_EnemyHeavyRed[32] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x30,
    0x33,0x33,0x30,0x18,0x0F,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x0C,
    0xCC,0xCC,0x0C,0x18,0xF0,0x00,0x00,0x00
};

static const u8 g_EnemyDiverWhite[32] = {
    0x00,0x00,0x00,0x03,0x07,0x0F,0x1F,0x3F,
    0x33,0x3C,0x3F,0x1F,0x0F,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0xE0,0xF0,0xF8,0xFC,
    0xCC,0x3C,0xFC,0xF8,0xF0,0x00,0x00,0x00
};
static const u8 g_EnemyDiverRed[32] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x0C,0x03,0x00,0x00,0x00,0x03,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x30,0xC0,0x00,0x00,0x00,0xC0,0x00,0x00
};

static const u8 g_EnemyBomberWhite[32] = {
    0x00,0x00,0x00,0x0F,0x1F,0x3F,0x27,0x0C,
    0x0C,0x0F,0x07,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0xF0,0xF8,0xFC,0xE4,0x30,
    0x30,0xF0,0xE0,0x00,0x00,0x00,0x00,0x00
};
static const u8 g_EnemyBomberRed[32] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x33,
    0x33,0x30,0x18,0x0F,0x0F,0x03,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x18,0xCC,
    0xCC,0x0C,0x18,0xF0,0xF0,0xC0,0x00,0x00
};

//=============================================================================
// WALL SCROLL TILES
//=============================================================================

static const u8 g_RockBase[8] = {
    0xFF, 0xAA, 0xFF, 0x55,
    0xFF, 0xAA, 0xFF, 0x55
};
static const u8 g_RockColor[8] = {
    0x54, 0x54, 0x54, 0x54,
    0x54, 0x54, 0x54, 0x54
};

void InitWallTiles()
{
    u8 phase, row;
    u8 tile[8];
    for (phase = 0; phase < 8; phase++)
    {
        for (row = 0; row < 8; row++)
            tile[row] = g_RockBase[(row + phase) & 7];
        VDP_LoadBankPattern_GM2(tile, 1, 0, WALL_TILE_BASE + phase);
        VDP_LoadBankPattern_GM2(tile, 1, 1, WALL_TILE_BASE + phase);
        VDP_LoadBankPattern_GM2(tile, 1, 2, WALL_TILE_BASE + phase);
        VDP_LoadBankColor_GM2(g_RockColor, 1, 0, WALL_TILE_BASE + phase);
        VDP_LoadBankColor_GM2(g_RockColor, 1, 1, WALL_TILE_BASE + phase);
        VDP_LoadBankColor_GM2(g_RockColor, 1, 2, WALL_TILE_BASE + phase);
    }
}

void UpdateWallScroll()
{
    // Avança fase
    g_WallPhaseTimer++;
    if (g_WallPhaseTimer < WALL_SPEED) return; // res a fer aquest frame
    g_WallPhaseTimer = 0;
    g_WallPhase = (g_WallPhase + 1) & 7;

    // Quan canvia la fase, actualitza TOTES les files d'una vegada
    // pero repartim: escrivim les 24 files en 2 passades de 12
    // per no sobrecarregar el frame
    u8 row, tile;
    for (row = 0; row < 24; row++)
    {
        tile = WALL_TILE_BASE + ((g_WallPhase + row) & 7);
        VDP_Poke_GM2(0,  row, tile);
        VDP_Poke_GM2(1,  row, tile);
        VDP_Poke_GM2(20, row, tile);
        VDP_Poke_GM2(21, row, tile);
    }
}

//=============================================================================
// STARFIELD - pixels individuals escrits directament a VRAM
// En Mode 2 cada tile 8x8 te el seu propi patró.
// Usem tiles especials: STAR_TILE_BASE+row (0-7) = pixel encès a la fila 'row'
// STAR_TILE_BLANK = tile buit per esborrar
//=============================================================================

#define STAR_TILE_BASE_1  210   // tiles 210-217: capa lenta  - blau fosc  (0x41)
#define STAR_TILE_BASE_2  218   // tiles 218-225: capa mitja  - blau clar  (0x51)
#define STAR_TILE_BASE_3  226   // tiles 226-233: capa rapida - blanc      (0xF1)

void InitStarTilesForBase(u8 base, u8 inkColor)
{
    u8 row, r, bank;
    u8 tile[8];
    u8 color[8];

    for (row = 0; row < 8; row++)
    {
        for (r = 0; r < 8; r++) { tile[r] = 0x00; color[r] = 0x01; }
        tile[row]  = 0x80;      // pixel a bit7 de la fila 'row'
        color[row] = inkColor;  // ink on black bg

        for (bank = 0; bank < 3; bank++)
        {
            VDP_LoadBankPattern_GM2(tile,  1, bank, base + row);
            VDP_LoadBankColor_GM2(color,   1, bank, base + row);
        }
    }
}

void InitStarTiles()
{
    InitStarTilesForBase(STAR_TILE_BASE_1, 0x41); // dark blue ink (color 4) on black (1)
    InitStarTilesForBase(STAR_TILE_BASE_2, 0x51); // light blue ink (color 5) on black (1)
    InitStarTilesForBase(STAR_TILE_BASE_3, 0xF1); // white ink (color 15) on black (1)
}

// Dibuixa un pixel a la posició (x,y) en mode 2
// x: pixel coord (0-255), y: pixel coord (0-191)
void DrawStarPixel(u8 x, u8 y, u8 base)
{
    u8 tx = x >> 3;
    u8 ty = y >> 3;
    u8 row = y & 7;
    VDP_Poke_GM2(tx, ty, base + row);
}

void EraseStarPixel(u8 x, u8 y)
{
    u8 tx = x >> 3;
    u8 ty = y >> 3;
    VDP_Poke_GM2(tx, ty, 0);
}

void InitStars(TStar* s, u8 n, u8 base)
{
    u8 i;
    // Posicions pseudo-aleatòries pero ben distribuïdes
    // Usem una sequencia de Halton simplificada per evitar ordenacio
    static const u8 xoff[11] = {11, 37, 63, 21, 51, 7, 41, 27, 57, 17, 47};
    static const u8 yoff[11] = {17, 83, 43, 131, 67, 107, 23, 157, 53, 97, 173};
    u8 span = STAR_X1 - STAR_X0;
    for (i = 0; i < n; i++)
    {
        s[i].x = STAR_X0 + (u8)(((u16)xoff[i] * span) >> 6);
        s[i].y = yoff[i] % SCREEN_H;
        DrawStarPixel(s[i].x, s[i].y, base);
    }
}

void TickStars(TStar* s, u8 n, u8 speed, u8 base)
{
    u8 i;
    for (i = 0; i < n; i++)
    {
        EraseStarPixel(s[i].x, s[i].y);
        s[i].y += speed;
        if (s[i].y >= SCREEN_H) s[i].y = 0;
        DrawStarPixel(s[i].x, s[i].y, base);
    }
}

//=============================================================================
// HUD
//=============================================================================

void UpdateHUD()
{
    if (!g_HudDirty) return;

    // Score - esborra i redibouxa
    Print_SetPosition(22, 1);
    Print_DrawText("     "); // esborra
    Print_SetPosition(22, 1);
    Print_DrawInt(g_Score);

    // Level
    Print_SetPosition(22, 3);
    Print_DrawText("  ");
    Print_SetPosition(22, 3);
    Print_DrawInt(g_Level);

    // Lives - esborra i redibuixa
    Print_SetPosition(22, 5);
    Print_DrawText("  ");
    Print_SetPosition(22, 5);
    Print_DrawInt(g_Lives);

    g_HudDirty = 0;
}

//=============================================================================
// ENEMIES
//=============================================================================

//=============================================================================
// LEVEL CONFIG - 1:1 CPC
//=============================================================================

// Taules de velocitats i scores per tipus - 1:1 CPC (han d'estar abans de wave functions)
static const u8  g_EnemySpeedY[5] = {
    ENEMY_SPEED_BASIC, ENEMY_SPEED_FAST, ENEMY_SPEED_HEAVY,
    ENEMY_SPEED_DIVER, ENEMY_SPEED_BOMBER
};
static const u16 g_EnemyScore[5] = {
    ENEMY_SCORE_BASIC, ENEMY_SCORE_FAST, ENEMY_SCORE_HEAVY,
    ENEMY_SCORE_DIVER, ENEMY_SCORE_BOMBER
};

static const TLevelConfig g_LevelTable[25] = {
    /* 1-5 Rocky */
    {3,2,LMASK_BASIC,0,0},
    {3,3,LMASK_BASIC,0,0},
    {4,3,LMASK_BASIC,0,0},
    {4,3,LMASK_BASIC,0,LMASK_FAST},
    {1,1,0,LCFG_F_BOSS1,0},
    /* 6-10 Ice */
    {4,4,LMASK_BASIC|LMASK_FAST,0,0},
    {4,4,LMASK_BASIC|LMASK_FAST,0,0},
    {5,4,LMASK_BASIC|LMASK_FAST,0,0},
    {4,4,LMASK_BASIC|LMASK_FAST,0,LMASK_HEAVY},
    {1,1,0,LCFG_F_BOSS1,0},
    /* 11-15 Forest */
    {5,5,LMASK_BASIC|LMASK_FAST|LMASK_HEAVY,0,0},
    {5,5,LMASK_BASIC|LMASK_FAST|LMASK_HEAVY,0,0},
    {5,6,LMASK_BASIC|LMASK_FAST|LMASK_HEAVY,0,0},
    {4,5,LMASK_BASIC|LMASK_FAST|LMASK_HEAVY,0,LMASK_DIVER},
    {1,1,0,LCFG_F_BOSS1,0},
    /* 16-20 Fire */
    {6,6,LMASK_BASIC|LMASK_FAST|LMASK_HEAVY|LMASK_DIVER,0,0},
    {6,6,LMASK_BASIC|LMASK_FAST|LMASK_HEAVY|LMASK_DIVER,0,0},
    {6,6,LMASK_BASIC|LMASK_FAST|LMASK_HEAVY|LMASK_DIVER,0,0},
    {4,6,LMASK_BASIC|LMASK_FAST|LMASK_HEAVY|LMASK_DIVER,0,LMASK_BOMBER},
    {1,1,0,LCFG_F_BOSS1,0},
    /* 21-25 Tech */
    {6,7,LMASK_BASIC|LMASK_FAST|LMASK_HEAVY|LMASK_DIVER|LMASK_BOMBER,0,0},
    {6,7,LMASK_BASIC|LMASK_FAST|LMASK_HEAVY|LMASK_DIVER|LMASK_BOMBER,0,0},
    {7,7,LMASK_BASIC|LMASK_FAST|LMASK_HEAVY|LMASK_DIVER|LMASK_BOMBER,0,0},
    {1,8,LMASK_BASIC|LMASK_FAST|LMASK_HEAVY|LMASK_DIVER|LMASK_BOMBER,0,0},
    {1,2,0,LCFG_F_BOSS2,0}
};

static const TLevelConfig* LevelConfigGet(u8 lv)
{
    if (lv < 1)  lv = 1;
    if (lv > 25) lv = 25;
    return &g_LevelTable[lv - 1];
}

//=============================================================================
// WAVE SYSTEM - 1:1 CPC
//=============================================================================

static u8 CountActiveEnemies()
{
    u8 i, c = 0;
    for (i = 0; i < MAX_ENEMIES; i++)
        if (g_Enemies[i].active) c++;
    return c;
}

// 1:1 CPC: patternInitialVX
static i8 PatternInitialVX(u8 patt)
{
    if (patt == PATT_ZIGZAG)   return 1;
    if (patt == PATT_DIAGONAL) return (Math_GetRandom8() & 1) ? 1 : -1;
    return 0;
}

// 1:1 CPC: getPatternForType
static void GetPatternForType(u8 type, u8* patt, i8* vx)
{
    static const u8 basic_patt_r3[8] = {0,1,2,0,1,2,0,1};
    if (type == ENEMY_TYPE_BASIC)
    {
        *patt = basic_patt_r3[Math_GetRandom8() & 7];
        *vx   = PatternInitialVX(*patt);
    }
    else if (type == ENEMY_TYPE_FAST)
    {
        *patt = (Math_GetRandom8() & 1) ? PATT_DIAGONAL : PATT_ZIGZAG;
        *vx   = PatternInitialVX(*patt);
    }
    else if (type == ENEMY_TYPE_HEAVY || type == ENEMY_TYPE_BOMBER)
    {
        *patt = PATT_STRAIGHT;
        *vx   = (Math_GetRandom8() & 1) ? 1 : -1;
    }
    else if (type == ENEMY_TYPE_DIVER)
    {
        *patt = PATT_DIAGONAL;
        *vx   = (Math_GetRandom8() & 1) ? 2 : -2;
    }
    else { *patt = PATT_STRAIGHT; *vx = 0; }
}

// 1:1 CPC: waveTypeFromSingleLmask
static u8 WaveTypeFromMask(u8 mask)
{
    switch (mask)
    {
        case LMASK_BASIC:  return ENEMY_TYPE_BASIC;
        case LMASK_FAST:   return ENEMY_TYPE_FAST;
        case LMASK_HEAVY:  return ENEMY_TYPE_HEAVY;
        case LMASK_DIVER:  return ENEMY_TYPE_DIVER;
        case LMASK_BOMBER: return ENEMY_TYPE_BOMBER;
        default:           return 255;
    }
}

// 1:1 CPC: pickHomogeneousTypeForWave (simplificat per MSX)
static u8 PickTypeForWave(u8 mask)
{
    u8 buf[5], nc = 0, b, idx;
    for (b = 0; b < 5; b++)
    {
        if (mask & (1u << b))
            buf[nc++] = b;
    }
    if (!nc) return ENEMY_TYPE_BASIC;
    idx = (u8)((Math_GetRandom8() ^ (g_WavesCleared << 1) ^ g_Level) % nc);
    return buf[idx];
}

// 1:1 CPC: buildWaveTypePlan
static void BuildWaveTypePlan(u8 total, u8 mask)
{
    u8 slot, t;
    u8 st = WaveTypeFromMask(mask);
    if (st != 255)
        t = st;
    else
        t = PickTypeForWave(mask);
    for (slot = 0; slot < total; slot++)
        g_WaveSlotType[slot] = t;
    g_WaveBonusBase = g_EnemyScore[t];
}

// Layout: mai mes de 2 enemics per fila -> max 4 sprites per linia (2 enemics x 2 sprites)
// n=1: [1]
// n=2: [2]
// n=3: [2][1]  (V shape)
// n=4: [2][2]
// n=5: [2][2][1]
// n=6: [2][2][2]
// n=7: [2][2][2][1]
// n=8: [2][2][2][2]
static void BuildWaveLayoutRank(u8 n)
{
    u8 row, col, idx, x, step, per_row, remaining;
    step = (u8)(ENEMY_W + 4);
    idx = 0;
    remaining = n;
    row = 0;

    while (remaining > 0)
    {
        per_row = (remaining >= 2) ? 2 : 1;
        // Centra la fila a l'area de joc
        x = (u8)(GAME_X0 + (GAME_W - (u8)(per_row * step)) / 2);
        for (col = 0; col < per_row && idx < WAVE_PLAN_MAX; col++)
        {
            g_WaveSlotX[idx] = (u8)(x + col * step);
            g_WaveSlotY[idx] = (u8)(row * 20); // 20px entre files
            idx++;
        }
        remaining -= per_row;
        row++;
    }
}

// 1:1 CPC: buildWaveSlotLayoutIndian
static void BuildWaveLayoutIndian(u8 n)
{
    u8 i, x;
    x = (u8)(GAME_X0 + MOD_POW2(Math_GetRandom8(), 128));
    if (x > GAME_X1 - ENEMY_W) x = GAME_X1 - ENEMY_W;
    for (i = 0; i < n; i++)
    {
        g_WaveSlotX[i] = x;
        g_WaveSlotY[i] = 0;
    }
}

// 1:1 CPC: spawnSingleEnemy
static void SpawnSingleEnemy(u8 x, u8 y, u8 patt, i8 vx, u8 type)
{
    u8 i;
    for (i = 0; i < MAX_ENEMIES; i++)
    {
        if (g_Enemies[i].active) continue;
        g_Enemies[i].active   = 1;
        g_Enemies[i].x        = x;
        g_Enemies[i].y        = y;
        g_Enemies[i].type     = type;
        g_Enemies[i].pattern  = patt;
        g_Enemies[i].vx       = vx;
        g_Enemies[i].vy       = g_EnemySpeedY[type];
        g_Enemies[i].zig_timer = 6;
        g_Enemies[i].health   = (type == ENEMY_TYPE_HEAVY || type == ENEMY_TYPE_BOMBER) ? 3 : 1;
        g_Enemies[i].fire_cd  = (u8)(ENEMYSHOT_COOLDOWN + (g_WaveSpawned << 1));
        return;
    }
}

// 1:1 CPC: spawnOneFromQueue
static void SpawnOneFromQueue()
{
    u8 slot;
    if (g_WaveSpawned >= g_WaveTotal) return;
    slot = g_WaveSpawned;
    SpawnSingleEnemy(g_WaveSlotX[slot], g_WaveSlotY[slot],
                     g_WaveUnifiedPatt, g_WaveUnifiedVX,
                     g_WaveSlotType[slot]);
    g_WaveSpawned++;
}

// 1:1 CPC: tryWaveSpawnTopup
static void TryWaveSpawnTopup()
{
    if (g_WaveMode == WAVE_MODE_INDIAN)
    {
        if (g_WaveIndianDelay) { g_WaveIndianDelay--; return; }
        if (g_WaveSpawned < g_WaveTotal && CountActiveEnemies() < g_WaveTotal)
        {
            SpawnOneFromQueue();
            if (g_WaveSpawned < g_WaveTotal)
                g_WaveIndianDelay = SERIAL_DELAY;
        }
        return;
    }
    // RANK: spawna tots de cop
    while (g_WaveSpawned < g_WaveTotal && CountActiveEnemies() < g_WaveTotal)
        SpawnOneFromQueue();
}

// 1:1 CPC: startNewWave
static void StartNewWave()
{
    const TLevelConfig* cfg = LevelConfigGet(g_Level);
    u8 mask, total;

    g_WaveKilled    = 0;
    g_WaveActive    = 1;
    g_WaveSpawned   = 0;
    g_WaveIndianDelay = 0;

    total = cfg->per_wave;
    if (total > WAVE_PLAN_MAX) total = WAVE_PLAN_MAX;
    g_WaveTotal = total;

    mask = cfg->mask;
    if (cfg->intro_mask && g_WavesCleared == 0)
        mask = cfg->intro_mask;

    // Mode indian o rank aleatori
    g_WaveMode = (Math_GetRandom8() & 1) ? WAVE_MODE_INDIAN : WAVE_MODE_RANK;

    BuildWaveTypePlan(total, mask);

    // Layout segons tipus
    if (g_WaveMode == WAVE_MODE_INDIAN || g_WaveSlotType[0] == ENEMY_TYPE_DIVER)
    {
        g_WaveMode = WAVE_MODE_INDIAN;
        BuildWaveLayoutIndian(total);
    }
    else
        BuildWaveLayoutRank(total);

    // Patró i vx unificats per tota l'onada
    GetPatternForType(g_WaveSlotType[0], &g_WaveUnifiedPatt, &g_WaveUnifiedVX);

    TryWaveSpawnTopup();
    g_SpawnTimer = (u8)(SPAWN_BASE + MOD_POW2(Math_GetRandom8(), SPAWN_VARIANCE + 1));
}

// 1:1 CPC: spawnWave
static void SpawnWave()
{
    if (g_WaveActive)
    {
        if (g_WaveSpawned < g_WaveTotal) TryWaveSpawnTopup();
        return;
    }
    if (g_SpawnTimer) { g_SpawnTimer--; return; }
    if (CountActiveEnemies()) return;
    StartNewWave();
}

// 1:1 CPC: updateWaveBonus
static void UpdateWaveBonus()
{
    if (!g_WaveActive) return;
    if (CountActiveEnemies()) return;
    if (g_WaveSpawned < g_WaveTotal) return;

    // Tota l'onada eliminada: suma bonus
    if (g_WaveKilled == g_WaveTotal)
    {
        u16 bonus = (u16)(g_WaveBonusBase * g_WaveTotal);
        g_Score += bonus;
        if (g_Score > 65535u) g_Score = 65535u;
        g_HudDirty = 1;
    }

    g_WavesCleared++;
    // Comprova si cal pujar de nivell
    {
        const TLevelConfig* cfg = LevelConfigGet(g_Level);
        if (g_WavesCleared >= cfg->waves)
        {
            g_WavesCleared = 0;
            if (g_Level < ENDGAME_FINAL_LEVEL)
            {
                g_Level++;
                g_HudDirty = 1;
            }
        }
    }
    g_WaveActive = 0;
}

void InitEnemies()
{
    u8 i;
    for (i = 0; i < MAX_ENEMIES;     i++) g_Enemies[i].active    = 0;
    for (i = 0; i < MAX_ENEMY_SHOTS; i++) g_EnemyShots[i].active = 0;
    for (i = 0; i < MAX_EXPLOSIONS;  i++) g_Explosions[i].active = 0;
    // 1:1 CPC: initEnemies wave state
    g_SpawnTimer    = SPAWN_FIRST_DELAY;
    g_WaveTotal     = 0;
    g_WaveSpawned   = 0;
    g_WaveKilled    = 0;
    g_WaveActive    = 0;
    g_WaveBonusBase = 10;
    g_WaveMode      = WAVE_MODE_RANK;
    g_WaveIndianDelay = 0;
}

void SpawnEnemyShot(u8 ex, u8 ey)
{
    u8 i;
    i8 vx;
    for (i = 0; i < MAX_ENEMY_SHOTS; i++)
    {
        if (!g_EnemyShots[i].active)
        {
            // Apunta cap a la nau
            i16 dx = (i16)g_ShipX - (i16)ex;
            if      (dx < -10) vx = -1;
            else if (dx >  10) vx =  1;
            else               vx =  0;
            g_EnemyShots[i].x      = ex + ENEMY_W / 2;
            g_EnemyShots[i].y      = ey + ENEMY_H;
            g_EnemyShots[i].vx     = vx;
            g_EnemyShots[i].active = 1;
            return;
        }
    }
}

void SpawnEnemyShot(u8 ex, u8 ey);

void UpdateEnemies()
{
    u8 i;

    // 1:1 CPC: spawnWave (no spawna mentre explota o invulnerable)
    if (!g_ShipExploding && !g_ShipInvul)
        SpawnWave();

    // 1:1 CPC: moveEnemies
    for (i = 0; i < MAX_ENEMIES; i++)
    {
        if (!g_Enemies[i].active) continue;

        g_Enemies[i].y += g_Enemies[i].vy;

        if (g_Enemies[i].pattern == PATT_ZIGZAG)
        {
            if (g_Enemies[i].zig_timer) g_Enemies[i].zig_timer--;
            else { g_Enemies[i].zig_timer = 6; g_Enemies[i].vx = -g_Enemies[i].vx; }
            g_Enemies[i].x = (u8)((i8)g_Enemies[i].x + g_Enemies[i].vx);
            if (g_Enemies[i].x < GAME_X0)        g_Enemies[i].x = GAME_X0;
            if (g_Enemies[i].x > GAME_X1-ENEMY_W) g_Enemies[i].x = GAME_X1-ENEMY_W;
        }
        else if (g_Enemies[i].pattern == PATT_DIAGONAL)
        {
            if      (g_Enemies[i].vx < 0 && g_Enemies[i].x <= GAME_X0)        g_Enemies[i].vx = 1;
            else if (g_Enemies[i].vx > 0 && g_Enemies[i].x >= GAME_X1-ENEMY_W) g_Enemies[i].vx = -1;
            else g_Enemies[i].x = (u8)((i8)g_Enemies[i].x + g_Enemies[i].vx);
        }

        if (g_Enemies[i].y >= SCREEN_H) { g_Enemies[i].active = 0; continue; }

        if (g_Enemies[i].fire_cd > 0) g_Enemies[i].fire_cd--;
        else { SpawnEnemyShot(g_Enemies[i].x, g_Enemies[i].y); g_Enemies[i].fire_cd = ENEMYSHOT_COOLDOWN; }
    }

    // Actualitza dispars enemics
    for (i = 0; i < MAX_ENEMY_SHOTS; i++)
    {
        if (!g_EnemyShots[i].active) continue;
        g_EnemyShots[i].y += ENEMYSHOT_SPEED_Y;
        g_EnemyShots[i].x  = (u8)((i8)g_EnemyShots[i].x + g_EnemyShots[i].vx);
        if (g_EnemyShots[i].y >= SCREEN_H ||
            g_EnemyShots[i].x < GAME_X0   ||
            g_EnemyShots[i].x > GAME_X1)
            g_EnemyShots[i].active = 0;
    }
}

//=============================================================================
// COLLISIONS
//=============================================================================
// COLLISIONS + SHIP HIT
//=============================================================================

void SpawnExplosion(u8 x, u8 y, u8 kind)
{
    u8 i;
    for (i = 0; i < MAX_EXPLOSIONS; i++)
    {
        if (!g_Explosions[i].active)
        {
            g_Explosions[i].x      = x;
            g_Explosions[i].y      = y;
            g_Explosions[i].active = 1;
            g_Explosions[i].frame  = 0;
            g_Explosions[i].kind   = kind;
            return;
        }
    }
}

void TickExplosions()
{
    u8 i;
    u8 maxf;
    for (i = 0; i < MAX_EXPLOSIONS; i++)
    {
        if (!g_Explosions[i].active) continue;
        maxf = (g_Explosions[i].kind == EXP_KIND_SHIP) ? 6 : 4;
        g_Explosions[i].frame++;
        if (g_Explosions[i].frame >= maxf)
            g_Explosions[i].active = 0;
    }
}

void RespawnShip()
{
    // 1:1 CPC: resetShipRuntimeState() + placeShipAtSpawn()
    g_ShipExploding  = 0;
    g_ShipExplTimer  = 0;
    g_ShipInvul      = 0;
    g_ShipInvulTimer = 0;
    g_FireCooldown   = 0;
    g_ShipX          = SHIP_SPAWN_X;
    g_ShipY          = SHIP_SPAWN_Y;
}

void UpdateShipExplosionState()
{
    // 1:1 CPC: updateShipExplosionState
    if (!g_ShipExploding) return;
    if (g_ShipExplTimer) { g_ShipExplTimer--; return; }
    // Timer acabat
    if (g_ShipLastLife) { g_ShipExploding = 0; return; } // game over, no respawn
    RespawnShip();
    g_ShipInvul      = 1;
    g_ShipInvulTimer = RESPAWN_INVUL_TICKS;
}

void UpdateShipInvulnerability()
{
    // 1:1 CPC: updateShipInvulnerability
    if (!g_ShipInvul) return;
    if (g_ShipInvulTimer) g_ShipInvulTimer--;
    if (!g_ShipInvulTimer) g_ShipInvul = 0;
}

void TriggerShipHit()
{
    u8 i;
    // 1:1 CPC: triggerShipHit
    SpawnExplosion(g_ShipX, g_ShipY, EXP_KIND_SHIP);
    if (g_Lives == 0)
        g_ShipLastLife = 1;
    else
    {
        g_Lives--;
        g_HudDirty = 1;
    }
    g_ShipExploding = 1;
    g_ShipExplTimer = SHIP_EXPL_TIMER;
    g_FireCooldown  = 0;

    // CPC: clearActiveCombatState
    for (i = 0; i < MAX_ENEMIES;     i++) g_Enemies[i].active    = 0;
    for (i = 0; i < MAX_SHOTS;       i++) g_Shots[i].active      = 0;
    for (i = 0; i < MAX_ENEMY_SHOTS; i++) g_EnemyShots[i].active = 0;
}

u8 RectOverlap(u8 x1, u8 y1, u8 w1, u8 h1, u8 x2, u8 y2, u8 w2, u8 h2){
    if ((u16)x1 + w1 <= x2) return 0;
    if ((u16)x2 + w2 <= x1) return 0;
    if ((u16)y1 + h1 <= y2) return 0;
    if ((u16)y2 + h2 <= y1) return 0;
    return 1;
}

void CheckCollisions()
{
    u8 i, j;

    // Shots vs enemies
    for (i = 0; i < MAX_SHOTS; i++)
    {
        if (!g_Shots[i].active) continue;
        for (j = 0; j < MAX_ENEMIES; j++)
        {
            if (!g_Enemies[j].active) continue;
            if (RectOverlap(g_Shots[i].x, g_Shots[i].y, 4, 8,
                            g_Enemies[j].x, g_Enemies[j].y, ENEMY_W, ENEMY_H))
            {
                g_Shots[i].active   = 0;
                g_Enemies[j].health--;
                if (g_Enemies[j].health == 0)
                {
                    g_Enemies[j].active = 0;
                    g_WaveKilled++;  // 1:1 CPC: ++wave_killed
                    SpawnExplosion(g_Enemies[j].x + ENEMY_W/2 - 8,
                                   g_Enemies[j].y + ENEMY_H/2 - 8,
                                   EXP_KIND_ENEMY);
                    g_Score += g_EnemyScore[g_Enemies[j].type];
                    if (g_Score > 65535u) g_Score = 65535u;
                    g_HudDirty = 1;
                }
                else
                {
                    // Enemic amb vida però tocat (heavy/bomber)
                    SpawnExplosion(g_Enemies[j].x + ENEMY_W/2 - 8,
                                   g_Enemies[j].y + ENEMY_H/2 - 8,
                                   EXP_KIND_ENEMY);
                }
            }
        }
    }

    if (g_ShipInvul || g_ShipExploding || g_ShipLastLife) return;

    // Nau vs enemics
    for (j = 0; j < MAX_ENEMIES; j++)
    {
        if (!g_Enemies[j].active) continue;
        if (RectOverlap(g_ShipX, g_ShipY, SHIP_W, SHIP_H,
                        g_Enemies[j].x, g_Enemies[j].y, ENEMY_W, ENEMY_H))
        {
            g_Enemies[j].active = 0;
            TriggerShipHit();
            return;
        }
    }

    // Nau vs dispars enemics
    for (j = 0; j < MAX_ENEMY_SHOTS; j++)
    {
        if (!g_EnemyShots[j].active) continue;
        if (RectOverlap(g_ShipX, g_ShipY, SHIP_W, SHIP_H,
                        g_EnemyShots[j].x, g_EnemyShots[j].y, 4, 8))
        {
            g_EnemyShots[j].active = 0;
            TriggerShipHit();
            return;
        }
    }
}

//=============================================================================
// MAIN
//=============================================================================
// MENU SYSTEM - 1:1 CPC
//=============================================================================

void InitHiScores()
{
    g_HiScores[0].score=300; g_HiScores[0].level=1;
    g_HiScores[0].name[0]='X'; g_HiScores[0].name[1]='M'; g_HiScores[0].name[2]='A';
    g_HiScores[1].score=200; g_HiScores[1].level=1;
    g_HiScores[1].name[0]='P'; g_HiScores[1].name[1]='M'; g_HiScores[1].name[2]='H';
    g_HiScores[2].score=100; g_HiScores[2].level=1;
    g_HiScores[2].name[0]='C'; g_HiScores[2].name[1]='M'; g_HiScores[2].name[2]='T';
}

// 1:1 CPC: hs_isTop - retorna posicio (0-2) o 255 si no entra
u8 HsIsTop(u16 score, u8 level)
{
    u8 i;
    for (i = 0; i < HISCORE_COUNT; i++)
    {
        if (score > g_HiScores[i].score) return i;
        if (score == g_HiScores[i].score && level > g_HiScores[i].level) return i;
    }
    return 255;
}

// 1:1 CPC: hs_insert
void HsInsert(u8 pos, u16 score, u8 level, u8* name)
{
    u8 i;
    for (i = HISCORE_COUNT - 1; i > pos; i--)
    {
        g_HiScores[i].score    = g_HiScores[i-1].score;
        g_HiScores[i].level    = g_HiScores[i-1].level;
        g_HiScores[i].name[0]  = g_HiScores[i-1].name[0];
        g_HiScores[i].name[1]  = g_HiScores[i-1].name[1];
        g_HiScores[i].name[2]  = g_HiScores[i-1].name[2];
    }
    g_HiScores[pos].score   = score;
    g_HiScores[pos].level   = level;
    g_HiScores[pos].name[0] = name[0];
    g_HiScores[pos].name[1] = name[1];
    g_HiScores[pos].name[2] = name[2];
}

// Forward declarations per poder usar HudDrawText abans de la seva definicio
void HudDrawText(u8 col, u8 row, const char* s, u8 colorByte);
void HudDrawHLine(u8 col, u8 row, u8 len, u8 colorByte);

void DrawHiScoreTable()
{
    u8 i;
    char buf[17];
    for (i = 0; i < HISCORE_COUNT; i++)
    {
        // Formatem: "1 XXXXX LVxx AAA" - 1:1 CPC hs_formatRow
        buf[0] = (char)('1' + i);
        buf[1] = ' ';
        {
            u16 s = g_HiScores[i].score;
            buf[6] = (char)('0' + s % 10); s /= 10;
            buf[5] = (char)('0' + s % 10); s /= 10;
            buf[4] = (char)('0' + s % 10); s /= 10;
            buf[3] = (char)('0' + s % 10); s /= 10;
            buf[2] = (char)('0' + s % 10);
        }
        buf[7]  = ' ';
        buf[8]  = 'L'; buf[9]  = 'V';
        buf[10] = (char)('0' + g_HiScores[i].level / 10);
        buf[11] = (char)('0' + g_HiScores[i].level % 10);
        buf[12] = ' ';
        buf[13] = (char)g_HiScores[i].name[0];
        buf[14] = (char)g_HiScores[i].name[1];
        buf[15] = (char)g_HiScores[i].name[2];
        buf[16] = 0;
        // 1:1 CPC: rank 1 en "hi" (blanc), resta en normal (blau)
        HudDrawText(4, (u8)(10 + i * 2), buf,
                    i == 0 ? HUD_FONT_COLOR_HI : HUD_FONT_COLOR_NRM);
    }
}

// Menu starfield - 1:1 CPC: 40 estrelles laterals + 12 centrals
#define MENU_STAR_N         40
#define MENU_STAR_CN        12   // estrelles centrals per TOP3/HELP
#define MENU_STAR_TILE_BASE 168
#define MENU_TWINKLE_FRAMES 4

static const u8 g_MenuStarX[MENU_STAR_N] = {
    0x02,0x06,0x04,0x07,0x02,0x05,0x01,0x06,0x04,0x02,
    0x08,0x05,0x03,0x06,0x04,0x08,0x03,0x07,0x01,0x08,
    0x1A,0x1E,0x18,0x1C,0x1F,0x1A,0x1D,0x1B,0x1E,0x19,
    0x1C,0x1A,0x1D,0x1C,0x1E,0x19,0x1F,0x1B,0x18,0x1F
};
static const u8 g_MenuStarY[MENU_STAR_N] = {
    0x03,0x05,0x03,0x09,0x07,0x0D,0x0B,0x11,0x0F,0x14,
    0x13,0x16,0x06,0x08,0x0A,0x0C,0x0E,0x10,0x12,0x14,
    0x02,0x04,0x06,0x06,0x0A,0x0C,0x0E,0x10,0x12,0x11,
    0x15,0x16,0x06,0x08,0x0A,0x0C,0x0E,0x10,0x12,0x15
};
// Estrelles centrals per TOP3/HELP (zona superior, no solapen text)
static const u8 g_MenuStarCX[MENU_STAR_CN] = {
    0x0C,0x12,0x0A,0x11,0x0E,0x14,0x0A,0x0F,0x12,0x13,0x0D,0x15
};
static const u8 g_MenuStarCY[MENU_STAR_CN] = {
    0x02,0x03,0x03,0x03,0x04,0x04,0x04,0x05,0x05,0x07,0x07,0x03
};
static const u8 g_MenuStarSeed[MENU_STAR_N] = {
    0,2,1,3,1,0,3,2,0,1,3,2,2,0,3,1,1,2,0,3,
    2,0,3,1,3,2,1,0,2,3,0,1,3,2,0,1,2,3,1,0
};
static const u8 g_MenuStarCS[MENU_STAR_CN] = {
    0,3,1,2,2,0,3,1,1,2,3,0
};

static u8 g_MenuStarState[MENU_STAR_N];
static u8 g_MenuStarCState[MENU_STAR_CN];
static u8 g_MenuTwinkleTick = 0;
static u8 g_MenuTwinkleRng  = 0x5A;
static u8 g_MenuUseCenter   = 0; // 1=estrelles centrals (TOP3/HELP)

void InitMenuStarTiles()
{
    static const u8 star_tile[8] = {0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00};
    static const u8 star_colors[3] = {0x41, 0x51, 0x71};
    u8 s, bank, r;
    u8 color[8];
    for (s = 0; s < 3; s++)
    {
        for (r = 0; r < 8; r++) color[r] = star_colors[s];
        for (bank = 0; bank < 3; bank++)
        {
            VDP_LoadBankPattern_GM2(star_tile, 1, bank, MENU_STAR_TILE_BASE + s);
            VDP_LoadBankColor_GM2(color,       1, bank, MENU_STAR_TILE_BASE + s);
        }
    }
}

void InitMenuStars()
{
    u8 i;
    for (i = 0; i < MENU_STAR_N;  i++) g_MenuStarState[i]  = g_MenuStarSeed[i];
    for (i = 0; i < MENU_STAR_CN; i++) g_MenuStarCState[i] = g_MenuStarCS[i];
    g_MenuTwinkleTick = 0;
    g_MenuTwinkleRng  = 0x5A;
}

void DrawMenuStar(u8 idx)
{
    u8 state, tile;
    if (g_MenuUseCenter && idx < MENU_STAR_CN)
    {
        state = g_MenuStarCState[idx];
        tile  = (state == 0) ? 0 : (u8)(MENU_STAR_TILE_BASE + state - 1);
        VDP_Poke_GM2(g_MenuStarCX[idx], g_MenuStarCY[idx], tile);
    }
    else if (!g_MenuUseCenter && idx < MENU_STAR_N)
    {
        state = g_MenuStarState[idx];
        tile  = (state == 0) ? 0 : (u8)(MENU_STAR_TILE_BASE + state - 1);
        VDP_Poke_GM2(g_MenuStarX[idx], g_MenuStarY[idx], tile);
    }
}

void TickMenuStars()
{
    u8 i, j;
    u8 total = g_MenuUseCenter ? MENU_STAR_CN : MENU_STAR_N;

    g_MenuTwinkleTick++;
    if (g_MenuTwinkleTick < MENU_TWINKLE_FRAMES) return;
    g_MenuTwinkleTick = 0;

    g_MenuTwinkleRng = (u8)(g_MenuTwinkleRng * 17u + 29u);
    i = g_MenuTwinkleRng;
    while (i >= total) i = (u8)(i - total);

    if (g_MenuUseCenter)
        g_MenuStarCState[i] = (u8)((g_MenuStarCState[i] + 1u) & 3u);
    else
        g_MenuStarState[i]  = (u8)((g_MenuStarState[i]  + 1u) & 3u);
    DrawMenuStar(i);

    g_MenuTwinkleRng = (u8)(g_MenuTwinkleRng * 17u + 29u);
    j = g_MenuTwinkleRng;
    while (j >= total) j = (u8)(j - total);
    if (j == i) { j = (u8)(i + 7u); while (j >= total) j = (u8)(j - total); }

    if (g_MenuUseCenter)
        g_MenuStarCState[j] = (u8)((g_MenuStarCState[j] + 1u) & 3u);
    else
        g_MenuStarState[j]  = (u8)((g_MenuStarState[j]  + 1u) & 3u);
    DrawMenuStar(j);
}

// Logo tiles - 24 tiles 8x8 (8 tiles ample x 3 files alt = 64x24px)
// Convertit del sprite CPC Mode 0 74x25px escalat a 64x24px
#define LOGO_TILE_BASE  100  // patrons 100-123
#define LOGO_TILES_X    8
#define LOGO_TILES_Y    3
#define LOGO_COL_START  12   // centrat: (32-8)/2=12

static const u8 g_LogoTiles[24*8] = {
    0x0F,0x1F,0x1F,0x3F,0x3F,0x3F,0x3F,0x3F,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x01,0x01,0x01,0xC1,0xC1,0xC1,0xC1,0xC1,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xF4,0xFC,0xFA,0xFE,0xFE,0xFE,0xFE,0xFE,
    0x01,0x03,0x07,0x0F,0x0F,0x0F,0x0F,0x0F,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xC0,0xE0,0xE0,0xF0,0xF0,0xF0,0xF0,0xF0,
    0x3F,0x3F,0x3F,0x3F,0x1F,0x0F,0x1F,0x3F,
    0x9F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x9F,
    0xC1,0xC1,0x01,0x01,0xC1,0xC1,0xC1,0xC1,
    0xF8,0xF8,0xF8,0xF8,0xF8,0xF8,0xF8,0xF8,
    0x7E,0x7E,0x7E,0x7E,0x7E,0x7E,0x7E,0x7E,
    0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,0x0F,
    0xE3,0xE3,0xE3,0xE3,0xE3,0xE3,0xE3,0xE3,
    0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,
    0x3F,0x3F,0x3F,0x3F,0x3F,0x3F,0x0F,0x0F,
    0x1F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xBF,0x3F,0x3F,
    0xF8,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x7F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF9,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xF7,0xF3,0xF1,
    0xE3,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xEF,0xCF,
};

// Colors per zona del logo - 1:1 CPC paleta
// Fila 0 (tiles 0-7):  Dark Blue (4) on Black (1) = 0x41
// Fila 1 (tiles 8-15): Light Blue (5) on Black (1) = 0x51
// Fila 2 (tiles 16-23): Medium Green (2) on Black (1) = 0x21
static const u8 g_LogoColors[24*8] = {
    0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,
    0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,
    0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,
    0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,
    0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,
    0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,
    0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,
    0x41,0x41,0x41,0x41,0x41,0x41,0x41,0x41,
    0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,
    0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,
    0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,
    0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,
    0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,
    0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,
    0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,
    0x51,0x51,0x51,0x51,0x51,0x51,0x51,0x51,
    0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21,
    0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21,
    0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21,
    0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21,
    0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21,
    0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21,
    0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21,
    0x21,0x21,0x21,0x21,0x21,0x21,0x21,0x21,
};

void InitLogoTiles()
{
    u8 t, bank;
    for (t = 0; t < 24; t++)
    {
        const u8* tile  = g_LogoTiles + t * 8;
        const u8* color = g_LogoColors + t * 8;
        for (bank = 0; bank < 3; bank++)
        {
            VDP_LoadBankPattern_GM2(tile,  1, bank, LOGO_TILE_BASE + t);
            VDP_LoadBankColor_GM2(color,   1, bank, LOGO_TILE_BASE + t);
        }
    }
}

void DrawLogo()
{
    u8 tx, ty, tileIdx;
    for (ty = 0; ty < LOGO_TILES_Y; ty++)
        for (tx = 0; tx < LOGO_TILES_X; tx++)
        {
            tileIdx = LOGO_TILE_BASE + ty * LOGO_TILES_X + tx;
            VDP_Poke_GM2((u8)(LOGO_COL_START + tx), ty, tileIdx);
        }
}

//=============================================================================
// HUD FONT CUSTOM - 1:1 CPC font_bits escalat de 4px a 8px
// Colors: Light Blue (5) on Black (1) = 0x51  (text normal, pen4 CPC)
//         White (15) on Black (1) = 0xF1       (text hi, pen15 CPC)
// Ordre: espai(0), A-Z(1-26), 0-9(27-36)
//=============================================================================

static const u8 g_HudFontTiles[37*8] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //  
    0x3C,0xC3,0xC3,0xFF,0xC3,0xC3,0xC3,0x00, // A
    0xFC,0xC3,0xC3,0xFC,0xC3,0xC3,0xFC,0x00, // B
    0x3C,0xC3,0xC0,0xC0,0xC0,0xC3,0x3C,0x00, // C
    0xF0,0xC3,0xC3,0xC3,0xC3,0xC3,0xF0,0x00, // D
    0xFF,0xC0,0xC0,0xFC,0xC0,0xC0,0xFF,0x00, // E
    0xFF,0xC0,0xC0,0xFC,0xC0,0xC0,0xC0,0x00, // F
    0x3C,0xC3,0xC0,0xCF,0xC3,0xC3,0x3C,0x00, // G
    0xC3,0xC3,0xC3,0xFF,0xC3,0xC3,0xC3,0x00, // H
    0xFC,0x30,0x30,0x30,0x30,0x30,0xFC,0x00, // I
    0x3F,0x0C,0x0C,0x0C,0x0C,0xCC,0x30,0x00, // J
    0xC3,0xCC,0xF0,0xC0,0xF0,0xCC,0xC3,0x00, // K
    0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xFF,0x00, // L
    0xC3,0xFF,0xFF,0xC3,0xC3,0xC3,0xC3,0x00, // M
    0xC3,0xC3,0xF3,0xCF,0xC3,0xC3,0xC3,0x00, // N
    0x3C,0xC3,0xC3,0xC3,0xC3,0xC3,0x3C,0x00, // O
    0xFC,0xC3,0xC3,0xFC,0xC0,0xC0,0xC0,0x00, // P
    0x3C,0xC3,0xC3,0xC3,0xCF,0x33,0x0F,0x00, // Q
    0xFC,0xC3,0xC3,0xFC,0xCC,0xC3,0xC3,0x00, // R
    0x3C,0xC3,0xC0,0x3C,0x03,0xC3,0x3C,0x00, // S
    0xFF,0x30,0x30,0x30,0x30,0x30,0x30,0x00, // T
    0xC3,0xC3,0xC3,0xC3,0xC3,0xC3,0x3C,0x00, // U
    0xC3,0xC3,0xC3,0xC3,0x3C,0x3C,0x3C,0x00, // V
    0xC3,0xC3,0xC3,0xFF,0xFF,0xC3,0xC3,0x00, // W
    0xC3,0xC3,0x3C,0x3C,0x3C,0xC3,0xC3,0x00, // X
    0xC3,0xC3,0x3C,0x30,0x30,0x30,0x30,0x00, // Y
    0xFF,0x03,0x0C,0x30,0xC0,0xC0,0xFF,0x00, // Z
    0x3C,0xC3,0xC3,0xC3,0xC3,0xC3,0x3C,0x00, // 0
    0x30,0xF0,0x30,0x30,0x30,0x30,0xFC,0x00, // 1
    0x3C,0xC3,0x03,0x0C,0x30,0xC0,0xFF,0x00, // 2
    0xFC,0x03,0x0C,0x3C,0x03,0xC3,0x3C,0x00, // 3
    0x0C,0x3C,0xCC,0xFF,0x0C,0x0C,0x0C,0x00, // 4
    0xFF,0xC0,0xFC,0x03,0x03,0xC3,0x3C,0x00, // 5
    0x3C,0xC3,0xC0,0xFC,0xC3,0xC3,0x3C,0x00, // 6
    0xFF,0x03,0x0C,0x0C,0x30,0x30,0x30,0x00, // 7
    0x3C,0xC3,0xC3,0x3C,0xC3,0xC3,0x3C,0x00, // 8
    0x3C,0xC3,0xC3,0x3F,0x03,0xC3,0x3C,0x00, // 9
};

void InitHudFontTiles()
{
    u8 t, bank, r;
    u8 color_nrm[8], color_hi[8];
    for (r = 0; r < 8; r++) { color_nrm[r] = HUD_FONT_COLOR_NRM; color_hi[r] = HUD_FONT_COLOR_HI; }
    for (t = 0; t < 37; t++)
    {
        const u8* tile = g_HudFontTiles + t * 8;
        for (bank = 0; bank < 3; bank++)
        {
            // Set verd (normal)
            VDP_LoadBankPattern_GM2(tile, 1, bank, HUD_FONT_TILE_BASE + t);
            VDP_LoadBankColor_GM2(color_nrm, 1, bank, HUD_FONT_TILE_BASE + t);
            // Set blanc (hi)
            VDP_LoadBankPattern_GM2(tile, 1, bank, HUD_FONT_TILE_HI + t);
            VDP_LoadBankColor_GM2(color_hi,  1, bank, HUD_FONT_TILE_HI + t);
        }
    }
}

// Converteix char a index de la font HUD (igual que CPC: charToIdx)
u8 HudCharToIdx(char c)
{
    if (c >= 'A' && c <= 'Z') return (u8)(1 + (c - 'A'));
    if (c >= '0' && c <= '9') return (u8)(27 + (c - '0'));
    return 0; // espai
}

// Dibuixa text amb la font HUD a posicio (col, row) del name table
void HudDrawText(u8 col, u8 row, const char* s, u8 colorByte)
{
    // Tria el banc de tiles segons el color demanat
    u8 base = (colorByte == HUD_FONT_COLOR_HI) ? HUD_FONT_TILE_HI : HUD_FONT_TILE_BASE;
    while (*s)
    {
        VDP_Poke_GM2(col, row, (u8)(base + HudCharToIdx(*s)));
        col++;
        s++;
        if (col >= 32) { col = 0; row++; }
    }
}

// Linia horitzontal amb color (simula cpct_drawSolidBox d'1 pixel alt del CPC)
void HudDrawHLine(u8 col, u8 row, u8 len, u8 colorByte)
{
    // Usem un tile especial: patró ple a la fila 4 (pixel central)
    // Guardem el tile 167 per a la linia
    static const u8 hline_tile[8] = {0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00};
    u8 hline_color[8];
    u8 bank, i, r;
    for (r = 0; r < 8; r++) hline_color[r] = colorByte;
    for (bank = 0; bank < 3; bank++)
    {
        VDP_LoadBankPattern_GM2(hline_tile,  1, bank, 167);
        VDP_LoadBankColor_GM2(hline_color,   1, bank, 167);
    }
    for (i = 0; i < len; i++)
        VDP_Poke_GM2((u8)(col+i), row, 167);
}

void UpdateMenuInput()
{
    u8 row8 = Keyboard_Read(8);
    u8 row5 = Keyboard_Read(5);
    u8 row0 = Keyboard_Read(0); // tecles 1, 2, 3
    u8 fire  = IS_KEY_PRESSED(row8, KEY_SPACE) || IS_KEY_PRESSED(row5, KEY_Z);

    g_BlinkCtr++;

    if (g_TitleMode == TS_MENU)
    {
        // Attract mode
        g_AttractFrm++;
        if (g_AttractFrm >= ATTRACT_HOLD_FRAMES)
        {
            g_AttractFrm = 0;
            if (g_AttractCycle == 0) { g_TitleMode = TS_ATTRACT_SCORE; g_AttractCycle = 1; }
            else if (g_AttractCycle == 1) { g_TitleMode = TS_HELP; g_AttractCycle = 2; }
            else { g_TitleMode = TS_MENU; g_AttractCycle = 0; }
            g_TitleDirty = 1; g_TitlePhase = 0;
            return;
        }

        if (g_TitlePhase == 0)
        {
            if (!fire) g_TitlePhase = 1;
            return;
        }

        if (IS_KEY_PRESSED(row0, KEY_3))
        {
            g_TitleMode = TS_REDEFINE; g_RedefineStep = 0;
            g_TitleDirty = 1; g_TitlePhase = 0;
        }
        else if (fire)
        {
            g_GameState  = GS_PLAYING;
        }

        // Blink "FIRE TO START" - 1:1 CPC
        if ((g_BlinkCtr & 0x10) != ((u8)(g_BlinkCtr - 1) & 0x10))
        {
            if (g_BlinkCtr & 0x10)
                HudDrawText(9, 15, "FIRE TO START", HUD_FONT_COLOR_HI);
            else
                HudDrawText(9, 15, "             ", HUD_FONT_COLOR_HI);
        }
    }
    else if (g_TitleMode == TS_ATTRACT_SCORE || g_TitleMode == TS_HELP)
    {
        g_AttractFrm++;
        if (g_AttractFrm >= ATTRACT_HOLD_FRAMES || fire)
        {
            g_AttractFrm = 0;
            // 1:1 CPC: cicle MENU->TOP3->HELP->MENU
            if (g_TitleMode == TS_ATTRACT_SCORE)
            { g_TitleMode = TS_HELP; g_AttractCycle = 2; }
            else
            { g_TitleMode = TS_MENU; g_AttractCycle = 0; }
            g_TitleDirty = 1; g_TitlePhase = 0;
        }
    }
    else if (g_TitleMode == TS_HISCORE_VIEW)
    {
        if (g_TitlePhase == 0)
        {
            if (!fire) g_TitlePhase = 1;
        }
        else if (fire)
        {
            g_TitleMode = TS_MENU; g_TitleDirty = 1; g_TitlePhase = 0;
            g_AttractFrm = 0; g_AttractCycle = 0;
        }
    }
    else if (g_TitleMode == TS_WIN)
    {
        if (fire)
        {
            g_TitleMode = TS_HISCORE_VIEW;
            g_TitleDirty = 1; g_TitlePhase = 0;
        }
    }
    else if (g_TitleMode == TS_HISCORE_INPUT)
    {
        static u8 key_cd = 0;
        if (key_cd > 0) { key_cd--; return; }

        if (IS_KEY_PRESSED(row8, KEY_UP))
        {
            g_HsInputChar[g_HsInputPos] = (g_HsInputChar[g_HsInputPos] <= 'A') ? 'Z' :
                                           (u8)(g_HsInputChar[g_HsInputPos] - 1);
            g_TitleDirty = 1; key_cd = 8;
        }
        else if (IS_KEY_PRESSED(row8, KEY_DOWN))
        {
            g_HsInputChar[g_HsInputPos] = (g_HsInputChar[g_HsInputPos] >= 'Z') ? 'A' :
                                           (u8)(g_HsInputChar[g_HsInputPos] + 1);
            g_TitleDirty = 1; key_cd = 8;
        }
        else if (IS_KEY_PRESSED(row8, KEY_RIGHT) && g_HsInputPos < 2)
        {
            g_HsInputPos++; g_TitleDirty = 1; key_cd = 8;
        }
        else if (IS_KEY_PRESSED(row8, KEY_LEFT) && g_HsInputPos > 0)
        {
            g_HsInputPos--; g_TitleDirty = 1; key_cd = 8;
        }
        else if (fire)
        {
            if (g_HsInputPos < 2)
            { g_HsInputPos++; g_TitleDirty = 1; key_cd = 8; }
            else
            {
                HsInsert(g_HsPos, g_LastScore, g_LastLevel, g_HsInputChar);
                g_TitleMode = TS_HISCORE_VIEW;
                g_TitleDirty = 1; g_TitlePhase = 0; key_cd = 16;
            }
        }
    }
    else if (g_TitleMode == TS_REDEFINE)
    {
        // 1:1 CPC: detecta qualsevol tecla premuda i l'assigna
        // Llegim totes les files del teclat per detectar la primera tecla
        static u8 redef_phase = 0;
        u8 detected = 0;
        u8 fr;
        for (fr = 0; fr < 9 && !detected; fr++)
        {
            u8 row = Keyboard_Read(fr);
            u8 bit;
            for (bit = 0; bit < 8 && !detected; bit++)
            {
                if (IS_KEY_PRESSED(row, bit << 4))
                {
                    g_LastKeyPressed = MAKE_KEY(fr, bit);
                    detected = 1;
                }
            }
        }

        if (redef_phase == 0)
        {
            if (!detected) redef_phase = 1; // espera que no hi hagi cap tecla
        }
        else if (redef_phase == 1 && detected)
        {
            // Assigna la tecla al pas actual
            switch (g_RedefineStep)
            {
                case 0: g_KeyLeft  = g_LastKeyPressed; break;
                case 1: g_KeyRight = g_LastKeyPressed; break;
                case 2: g_KeyUp    = g_LastKeyPressed; break;
                case 3: g_KeyDown  = g_LastKeyPressed; break;
                case 4: g_KeyFire  = g_LastKeyPressed; break;
                default: break;
            }
            g_RedefineStep++;
            redef_phase = 0;
            if (g_RedefineStep >= 5)
            {
                g_TitleMode = TS_MENU;
                g_RedefineStep = 0;
            }
            g_TitleDirty = 1;
        }
    }
}

void DrawMenuScreen()
{
    u8 k, i;
    VDP_FillScreen_GM2(0);
    InitLogoTiles();
    InitHudFontTiles();
    InitMenuStarTiles();
    InitMenuStars();

    // 1:1 CPC: estrelles centrals per pantalles sense logo
    g_MenuUseCenter = (g_TitleMode == TS_ATTRACT_SCORE ||
                       g_TitleMode == TS_HISCORE_VIEW  ||
                       g_TitleMode == TS_HISCORE_INPUT ||
                       g_TitleMode == TS_HELP) ? 1 : 0;

    // Dibuixa estrelles inicials
    {
        u8 total = g_MenuUseCenter ? MENU_STAR_CN : MENU_STAR_N;
        for (i = 0; i < total; i++) DrawMenuStar(i);
        // Estrelles laterals sempre
        if (g_MenuUseCenter)
        {
            for (i = 0; i < MENU_STAR_N; i++)
            {
                u8 state = g_MenuStarState[i];
                u8 tile  = (state == 0) ? 0 : (u8)(MENU_STAR_TILE_BASE + state - 1);
                VDP_Poke_GM2(g_MenuStarX[i], g_MenuStarY[i], tile);
            }
        }
    }

    if (g_TitleMode == TS_MENU)
    {
        DrawLogo();
        HudDrawHLine(0, 4, 32, HUD_FONT_COLOR_CYN);
        HudDrawText(10, 6, "1 JOYSTICK", HUD_FONT_COLOR_NRM);
        HudDrawText(10, 8, "2 KEYBOARD", HUD_FONT_COLOR_NRM);
        HudDrawText(10,10, "3 SET KEYS", HUD_FONT_COLOR_NRM);
        HudDrawHLine(0, 12, 32, HUD_FONT_COLOR_CYN);
        HudDrawText(9, 15, "FIRE TO START", HUD_FONT_COLOR_HI);
        HudDrawText(0, 23, "GAME BY XEVIMET4L", HUD_FONT_COLOR_NRM);
        HudDrawText(28, 23, "2026", HUD_FONT_COLOR_NRM);
    }
    else if (g_TitleMode == TS_ATTRACT_SCORE)
    {
        // 1:1 CPC: top 3 SENSE logo, amb estrelles centrals
        HudDrawText(13, 8, "TOP  3", HUD_FONT_COLOR_HI);
        HudDrawHLine(0,  9, 32, HUD_FONT_COLOR_CYN);
        DrawHiScoreTable();
        HudDrawHLine(0, 16, 32, HUD_FONT_COLOR_CYN);
        HudDrawText(0, 23, "GAME BY XEVIMET4L", HUD_FONT_COLOR_NRM);
        HudDrawText(28, 23, "2026", HUD_FONT_COLOR_NRM);
    }
    else if (g_TitleMode == TS_HISCORE_VIEW)
    {
        HudDrawText(11, 6, "GAME OVER", HUD_FONT_COLOR_HI);
        HudDrawHLine(0,  7, 32, HUD_FONT_COLOR_NRM);
        HudDrawText(13, 8, "TOP 3", HUD_FONT_COLOR_HI);
        HudDrawHLine(0,  9, 32, HUD_FONT_COLOR_CYN);
        DrawHiScoreTable();
        HudDrawHLine(0, 16, 32, HUD_FONT_COLOR_CYN);
        HudDrawText(9, 18, "PRESS ANY KEY", HUD_FONT_COLOR_HI);
        HudDrawText(0, 23, "GAME BY XEVIMET4L", HUD_FONT_COLOR_NRM);
        HudDrawText(28, 23, "2026", HUD_FONT_COLOR_NRM);
    }
    else if (g_TitleMode == TS_HELP)
    {
        HudDrawText(10, 3, "HOW TO PLAY", HUD_FONT_COLOR_HI);
        HudDrawHLine(0,  4, 32, HUD_FONT_COLOR_CYN);
        HudDrawText(0, 5,  "JOY OR ARROWS TO MOVE",    HUD_FONT_COLOR_NRM);
        HudDrawText(1, 7,  "Z OR FIRE 1 TO SHOOT",     HUD_FONT_COLOR_NRM);
        HudDrawText(6, 9,  "P TO PAUSE",                HUD_FONT_COLOR_NRM);
        HudDrawText(1,11,  "Q OR RETURN TO QUIT",       HUD_FONT_COLOR_NRM);
        HudDrawText(0,13,  "EXTRA LIFE EVERY 5000 PTS", HUD_FONT_COLOR_NRM);
        HudDrawText(1,15,  "COMPLETE WAVE FOR BONUS",   HUD_FONT_COLOR_NRM);
        HudDrawHLine(0, 17, 32, HUD_FONT_COLOR_CYN);
        HudDrawText(9, 19, "FIRE TO MENU", HUD_FONT_COLOR_HI);
        HudDrawText(0, 23, "GAME BY XEVIMET4L", HUD_FONT_COLOR_NRM);
        HudDrawText(28, 23, "2026", HUD_FONT_COLOR_NRM);
    }
    else if (g_TitleMode == TS_WIN)
    {
        DrawLogo();
        HudDrawHLine(0,  4, 32, HUD_FONT_COLOR_NRM);
        HudDrawText(8,  6, "CONGRATULATIONS",       HUD_FONT_COLOR_HI);
        HudDrawText(6,  8, "THE INVASION IS OVER",  HUD_FONT_COLOR_NRM);
        HudDrawText(5, 10, "THANK YOU FOR PLAYING", HUD_FONT_COLOR_NRM);
        HudDrawHLine(0, 12, 32, HUD_FONT_COLOR_NRM);
        HudDrawText(6, 20, "PRESS FIRE TO MENU", HUD_FONT_COLOR_HI);
        HudDrawText(0, 23, "GAME BY XEVIMET4L", HUD_FONT_COLOR_NRM);
        HudDrawText(28, 23, "2026", HUD_FONT_COLOR_NRM);
    }
    else if (g_TitleMode == TS_HISCORE_INPUT)
    {
        HudDrawHLine(0,  3, 32, HUD_FONT_COLOR_NRM);
        HudDrawText(11, 4, "GAME OVER", HUD_FONT_COLOR_HI);
        HudDrawHLine(0,  5, 32, HUD_FONT_COLOR_NRM);
        HudDrawText(13, 6, "TOP 3", HUD_FONT_COLOR_HI);
        HudDrawHLine(0,  7, 32, HUD_FONT_COLOR_CYN);
        DrawHiScoreTable();
        HudDrawHLine(0, 16, 32, HUD_FONT_COLOR_CYN);
        HudDrawText(5, 18, "ENTER YOUR NAME", HUD_FONT_COLOR_NRM);
        for (k = 0; k < 3; k++)
        {
            char c[2];
            c[0] = (char)g_HsInputChar[k]; c[1] = 0;
            HudDrawText((u8)(14 + k * 2), 20, c,
                        k == g_HsInputPos ? HUD_FONT_COLOR_HI : HUD_FONT_COLOR_NRM);
        }
        HudDrawText(0, 23, "GAME BY XEVIMET4L", HUD_FONT_COLOR_NRM);
        HudDrawText(28, 23, "2026", HUD_FONT_COLOR_NRM);
    }
    else if (g_TitleMode == TS_REDEFINE)
    {
        static const char* const key_names[5] = {"LEFT","RIGHT","UP","DOWN","FIRE"};
        HudDrawText(12, 6,  "SET KEYS",   HUD_FONT_COLOR_HI);
        HudDrawHLine(0,  8, 32, HUD_FONT_COLOR_CYN);
        if (g_RedefineStep < 5)
            HudDrawText(12, 12, key_names[g_RedefineStep], HUD_FONT_COLOR_HI);
        HudDrawText(10, 16, "PRESS A KEY", HUD_FONT_COLOR_NRM);
        HudDrawText(0, 23, "GAME BY XEVIMET4L", HUD_FONT_COLOR_NRM);
        HudDrawText(28, 23, "2026", HUD_FONT_COLOR_NRM);
    }
}

// Crida quan acaba la partida per anar al menu correcte
void EnterPostGame(u8 won)
{
    g_LastScore = g_Score;
    g_LastLevel = g_Level;

    if (won)
    {
        g_TitleMode = TS_WIN;
    }
    else
    {
        g_HsPos = HsIsTop(g_Score, g_Level);
        if (g_Score > 0 && g_HsPos < HISCORE_COUNT)
        {
            g_HsInputPos    = 0;
            g_HsInputChar[0] = 'A';
            g_HsInputChar[1] = 'A';
            g_HsInputChar[2] = 'A';
            g_TitleMode = TS_HISCORE_INPUT;
        }
        else
        {
            g_TitleMode = TS_HISCORE_VIEW;
        }
    }
    g_GameState  = GS_TITLE;
    g_TitleDirty = 1;
    g_TitlePhase = 0;
    g_AttractFrm = 0;
}

void ResetGameSession()
{
    // 1:1 CPC: resetGameSession
    g_Score       = 0;
    g_Lives       = 3;
    g_Level       = 1;
    g_WavesCleared = 0;
    g_HudDirty    = 1;
    g_ShipLastLife = 0;
    g_GameOverDelay = 0;
    InitEnemies();
    RespawnShip();
}

void InitGamePlay()
{
    // Inicialitza pantalla de joc
    VDP_FillScreen_GM2(0);
    Print_SetPosition(22, 0);  Print_DrawText("SC:");
    Print_SetPosition(22, 2);  Print_DrawText("LV:");
    Print_SetPosition(22, 4);  Print_DrawText("LI:");
    InitWallTiles();
    InitStarTiles();
    InitStars(g_S1, N1, STAR_TILE_BASE_1);
    InitStars(g_S2, N2, STAR_TILE_BASE_2);
    InitStars(g_S3, N3, STAR_TILE_BASE_3);
    ResetGameSession();
    Print_SetPosition(22, 1); Print_DrawInt(g_Score);
    Print_SetPosition(22, 3); Print_DrawInt(g_Level);
    Print_SetPosition(22, 5); Print_DrawInt(g_Lives);
}

//=============================================================================

void main()
{
    u8 i, j, spr;
    u8 f, pat, col;
    u8 row8, row5;

    // Init screen
    VDP_SetMode(VDP_MODE_GRAPHIC2);
    VDP_SetColor(COLOR_BLACK);
    VDP_ClearVRAM();
    VDP_SetSpriteFlag(VDP_SPRITE_SIZE_16);

    // Load sprites (una sola vegada)
    VDP_LoadSpritePattern(g_SpriteWhite,      0,  4);
    VDP_LoadSpritePattern(g_SpriteRed,        4,  4);
    VDP_LoadSpritePattern(g_ShotPattern,      8,  1);
    VDP_LoadSpritePattern(g_EnemyShotPattern, 12, 4);
    VDP_LoadSpritePattern(g_EnemyWhite,       16, 4);
    VDP_LoadSpritePattern(g_EnemyRed,         20, 4);
    VDP_LoadSpritePattern(g_EnemyFastWhite,   24, 4);
    VDP_LoadSpritePattern(g_EnemyFastRed,     28, 4);
    VDP_LoadSpritePattern(g_EnemyHeavyWhite,  32, 4);
    VDP_LoadSpritePattern(g_EnemyHeavyRed,    36, 4);
    VDP_LoadSpritePattern(g_EnemyDiverWhite,  40, 4);
    VDP_LoadSpritePattern(g_EnemyDiverRed,    44, 4);
    VDP_LoadSpritePattern(g_EnemyBomberWhite, 48, 4);
    VDP_LoadSpritePattern(g_EnemyBomberRed,   52, 4);
    VDP_LoadSpritePattern(g_ExpSprite0,       56, 4);
    VDP_LoadSpritePattern(g_ExpSprite1,       60, 4);
    VDP_LoadSpritePattern(g_ExpSprite2,       64, 4);
    VDP_LoadSpritePattern(g_ExpSprite3,       68, 4);

    // Init font (necessari per tot)
    Print_SetTextFont(PRINT_DEFAULT_FONT, 1);

    // Init hi-scores
    InitHiScores();

    // Mostra menu inicial
    g_GameState  = GS_TITLE;
    g_TitleMode  = TS_MENU;
    g_TitleDirty = 1;
    g_TitlePhase = 0;
    g_AttractFrm = 0;
    DrawMenuScreen();

    // Disable all sprites durant el menu
    for (i = 0; i < 32; i++)
        VDP_SetSpritePositionY(i, VDP_SPRITE_DISABLE_SM1);

    while (1)
    {
        Halt();
        row8 = Keyboard_Read(8);
        row5 = Keyboard_Read(5);

        if (g_GameState == GS_TITLE)
        {
            //=== MENU ===
            UpdateMenuInput();
            TickMenuStars(); // 1:1 CPC: tickTitleTwinkle

            if (g_TitleDirty)
            {
                DrawMenuScreen();
                g_TitleDirty = 0;
            }

            // Transicio a joc
            if (g_GameState == GS_PLAYING)
            {
                InitGamePlay();
            }
        }
        else
        {
            //=== PLAYING ===

            // Wall scroll + starfield
            UpdateWallScroll();
            switch (g_StarTimer1 & 3)
            {
                case 0: TickStars(g_S1, N1, 1, STAR_TILE_BASE_1); break;
                case 1: TickStars(g_S2, N2, 1, STAR_TILE_BASE_2); break;
                case 2: TickStars(g_S3, N3, 2, STAR_TILE_BASE_3); break;
                default: break;
            }
            g_StarTimer1++;

            // Input
            if (!g_ShipExploding)
            {
                if (IS_KEY_PRESSED(row8, KEY_LEFT)  && g_ShipX > SHIP_MIN_X) g_ShipX -= g_ShipSpeedX;
                if (IS_KEY_PRESSED(row8, KEY_RIGHT) && g_ShipX < SHIP_MAX_X) g_ShipX += g_ShipSpeedX;
                if (IS_KEY_PRESSED(row8, KEY_UP)    && g_ShipY > SHIP_MIN_Y) g_ShipY -= g_ShipSpeedY;
                if (IS_KEY_PRESSED(row8, KEY_DOWN)  && g_ShipY < SHIP_MAX_Y) g_ShipY += g_ShipSpeedY;

                if ((IS_KEY_PRESSED(row8, KEY_SPACE) || IS_KEY_PRESSED(row5, KEY_Z)) && g_FireCooldown == 0)
                {
                    for (i = 0; i < MAX_SHOTS; i++)
                    {
                        if (!g_Shots[i].active)
                        {
                            g_Shots[i].x = g_ShipX + SHIP_W / 2 - 2;
                            g_Shots[i].y = g_ShipY - 8;
                            g_Shots[i].active = 1;
                            g_FireCooldown = FIRE_COOLDOWN;
                            break;
                        }
                    }
                }
            }

            // Update
            if (g_FireCooldown > 0) g_FireCooldown--;
            for (i = 0; i < MAX_SHOTS; i++)
            {
                if (!g_Shots[i].active) continue;
                if (g_Shots[i].y < SHOT_SPEED) { g_Shots[i].active = 0; continue; }
                g_Shots[i].y -= SHOT_SPEED;
            }

            UpdateEnemies();
            CheckCollisions();
            TickExplosions();
            UpdateWaveBonus();
            UpdateShipExplosionState();
            UpdateShipInvulnerability();

            // Game over - 1:1 CPC: 30 frames de scroll dramatic, sense text
            // El text "GAME OVER" apareix a la pantalla d'hiscore (menu)
            if (g_ShipLastLife && !g_ShipExploding)
            {
                if (g_GameOverDelay == 0)
                {
                    // 1:1 CPC: sfxGameOver() - aqui podriem posar so
                    g_GameOverDelay = 1;
                }
                else if (g_GameOverDelay < 30)
                {
                    // Scroll segueix, pantalla buida de combat (dramatic)
                    g_GameOverDelay++;
                }
                else
                {
                    EnterPostGame(0);
                }
            }

            // Victoria (nivell final completat)
            if (g_Level > ENDGAME_FINAL_LEVEL)
                EnterPostGame(1);

            UpdateHUD();

            // Draw sprites
            spr = 0;
            g_FrameCount++;

            // Ship
            if (!g_ShipExploding && (!g_ShipInvul || (g_ShipInvulTimer & 1)))
            {
                VDP_SetSpriteSM1(spr++, g_ShipX, g_ShipY, 0, COLOR_WHITE);
                VDP_SetSpriteSM1(spr++, g_ShipX, g_ShipY, 4, COLOR_MEDIUM_RED);
            }
            else { spr += 2; }

            // Player shots
            for (i = 0; i < MAX_SHOTS; i++)
                if (g_Shots[i].active)
                    VDP_SetSpriteSM1(spr++, g_Shots[i].x, g_Shots[i].y, 8, COLOR_LIGHT_YELLOW);

            // Enemy shots
            for (i = 0; i < MAX_ENEMY_SHOTS && spr < 18; i++)
                if (g_EnemyShots[i].active)
                    VDP_SetSpriteSM1(spr++, g_EnemyShots[i].x, g_EnemyShots[i].y, 12, COLOR_MEDIUM_RED);

            // Enemies
            {
                static const u8 etype_pat_w[5] = {16, 24, 32, 40, 48};
                static const u8 etype_pat_r[5] = {20, 28, 36, 44, 52};
                for (i = 0; i < MAX_ENEMIES && spr < 28; i++)
                {
                    if (!g_Enemies[i].active) continue;
                    j = g_Enemies[i].type;
                    VDP_SetSpriteSM1(spr++, g_Enemies[i].x, g_Enemies[i].y, etype_pat_w[j], COLOR_WHITE);
                    VDP_SetSpriteSM1(spr++, g_Enemies[i].x, g_Enemies[i].y, etype_pat_r[j], COLOR_MEDIUM_RED);
                }
            }

            // Explosions
            for (j = 0; j < MAX_EXPLOSIONS && spr < 30; j++)
            {
                static const u8 exp_pat_e[4] = {56, 60, 64, 60};
                static const u8 exp_pat_s[6] = {56, 60, 64, 68, 64, 60};
                if (!g_Explosions[j].active) continue;
                f = g_Explosions[j].frame;
                if (g_Explosions[j].kind == EXP_KIND_SHIP)
                { if (f > 5) f = 5; pat = exp_pat_s[f]; }
                else
                { if (f > 3) f = 3; pat = exp_pat_e[f]; }
                col = (f == 0) ? COLOR_WHITE :
                      (f == 1) ? COLOR_LIGHT_YELLOW : COLOR_MEDIUM_RED;
                VDP_SetSpriteSM1(spr++, g_Explosions[j].x, g_Explosions[j].y, pat, col);
            }

            while (spr < 32)
                VDP_SetSpritePositionY(spr++, VDP_SPRITE_DISABLE_SM1);
        }
    }
}
