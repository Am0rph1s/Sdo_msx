// Nau DX - MSX1 Port using MSXgl

#include "msxgl.h"
#include "psg.h"








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
#define SHOT_SPEED      3
#define FIRE_COOLDOWN   4   // CPC: 1, nosaltres 4 (per compensar 50fps estables)

// Starfield - 3 layers (mes estels i mes rapid que l'original)
#define N1              8    // slow stars
#define N2              12   // medium stars
#define N3              16   // fast stars
#define STAR_X0         (GAME_X0 + 4)
#define STAR_X1         (GAME_X1 - 4)

// Enemy types - 1:1 CPC
#define ENEMY_TYPE_BASIC   0
#define ENEMY_TYPE_FAST    1
#define ENEMY_TYPE_HEAVY   2
#define ENEMY_TYPE_DIVER   3
#define ENEMY_TYPE_BOMBER  4
#define ENEMY_TYPE_BOSS    5

// Enemy speeds - 1:1 CPC (adaptats a MSX: CPC té 50Hz i pixels més grans)
#define ENEMY_SPEED_BASIC   2
#define ENEMY_SPEED_FAST    3
#define ENEMY_SPEED_HEAVY   2
#define ENEMY_SPEED_DIVER   3
#define ENEMY_SPEED_BOMBER  2
#define ENEMY_SPEED_BOSS    1   // Boss moves at 1 px/frame vertically

// Enemy scores - 1:1 CPC
#define ENEMY_SCORE_BASIC   10
#define ENEMY_SCORE_FAST    20
#define ENEMY_SCORE_HEAVY   50
#define ENEMY_SCORE_DIVER   40
#define ENEMY_SCORE_BOMBER  80
#define ENEMY_SCORE_BOSS    1500

// Movement patterns - 1:1 CPC
#define PATT_STRAIGHT  0
#define PATT_ZIGZAG    1
#define PATT_DIAGONAL  2

// Wave system - 1:1 CPC
#define WAVE_PLAN_MAX       8
#define WAVE_MAX_HEAVY_PER_WAVE 3
#define DIVER_WAVE_MIN      5
#define DIVER_WAVE_MAX      8
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

// Boss tiers - 1:1 CPC
#define BOSS_HP_BASE         15
#define BOSS_HP_PER_TIER     5
#define BOSS_LANE_W          ((u8)(GAME_W / 3u))  // CPC: screen/3 per lane
#define BOSS_HOLD_Y          58   // CPC: 74, nosaltres 58 (mes amunt)
#define BOSS_Y_OSC           22
#define BOSS_DUAL_LANE_HALF  24
#define BOSS_DUAL_MIN_GAP    18
#define BOSS_VOSC_DESC       0
#define BOSS_VOSC_UP         1
#define BOSS_VOSC_DOWN       2
#define BOSS_TIERS_MAX       5

// Enemies
#define MAX_ENEMIES        5   // CPC: 5 (no WAVE_PLAN_MAX)
#define ENEMY_W            13
#define ENEMY_H            13
#define ENEMY_BOSS_W       16   // Boss is 16x16
#define ENEMY_BOSS_H       16
#define ENEMYSHOT_W        1
#define ENEMYSHOT_H        1
#define ENEMYSHOT_SPEED_Y  3
#define ENEMYSHOT_COOLDOWN 18
#define ENEMYSHOT_STAGGER  5
#define ENEMYSHOT_VX_FAST  1
#define ENEMYSHOT_VX_SLOW  1
#define ENEMYSHOT_TYPE_BULLET 0
#define ENEMYSHOT_TYPE_BOMB   1

// Explosions
#define MAX_EXPLOSIONS     4
#define EXP_KIND_ENEMY     0
#define EXP_KIND_SHIP      1
#define EXP_KIND_BOSS      2   // Boss explosion (2 sprites compuestos)

// Enemy shots
#define MAX_ENEMY_SHOTS    11
#define SHIP_HIT_W         12
#define SHIP_HIT_H         12
#define SHIP_HIT_OX        2
#define SHIP_HIT_OY        2

// Respawn - valors 1:1 del CPC
#define SHIP_EXPL_TIMER    9    // CPC: 13, nosaltres 9 = prou per veure l'explosio
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
#define HUD_FONT_TILE_BASE  100  // tiles 100-136: font en VERD (normal)
#define HUD_FONT_TILE_HI    137  // tiles 137-173: font en BLANC (hi)
#define HUD_FONT_COLOR_NRM  0x31  // Light Green (3) on Black (1)
#define HUD_FONT_COLOR_HI   0xF1  // White (15) on Black (1)
#define HUD_FONT_COLOR_CYN  0x71  // Cyan (7) on Black
#define HLINE_TILE         98   // Tile for cyan separator lines (free slot, outside all ranges)
#define BAR_FILL_TILE      99   // Tile for boss HP bar fill (free slot)

// HUD right-side layout (columns 22-31)
#define HUD_COL 22

// Helper: check if a redefinable key is pressed
u8 IsKeyPressed(u8 key) { return IS_KEY_PRESSED(Keyboard_Read(key & 0x0F), key); }

// Joystick input helpers (port 1, bits are 0 when pressed)
#define JOY_DIR()   (~Joystick_Read(JOY_PORT_1) & 0x0F)
#define JOY_FIRE()  ((Joystick_Read(JOY_PORT_1) & JOY_INPUT_TRIGGER_A) == 0)

// Unified input macros - check joystick or keyboard based on g_ControlMode
#define INPUT_LEFT()   (g_ControlMode ? IsKeyPressed(g_KeyLeft)   : (JOY_DIR() & JOY_INPUT_DIR_LEFT))
#define INPUT_RIGHT()  (g_ControlMode ? IsKeyPressed(g_KeyRight)  : (JOY_DIR() & JOY_INPUT_DIR_RIGHT))
#define INPUT_UP()     (g_ControlMode ? IsKeyPressed(g_KeyUp)     : (JOY_DIR() & JOY_INPUT_DIR_UP))
#define INPUT_DOWN()   (g_ControlMode ? IsKeyPressed(g_KeyDown)   : (JOY_DIR() & JOY_INPUT_DIR_DOWN))
#define INPUT_FIRE()   (g_ControlMode ? IsKeyPressed(g_KeyFire)   : JOY_FIRE())
#define INPUT_PAUSE()  (IsKeyPressed(g_KeyPause) || ((Joystick_Read(JOY_PORT_1) & JOY_INPUT_TRIGGER_B) == 0))
#define INPUT_QUIT()   (IsKeyPressed(g_KeyQuit))

// Hi-scores
#define HISCORE_COUNT    3

// Wall scroll
#define WALL_TILE_BASE     90   // Abans 200: solapava amb font HI (nums 6-9)
#define WALL_SPEED         4

// Biome palettes - CPC: only indices 8,9 change per biome
// CPC palette: {20,6,30,19,0,25,13,21,28,14,21,20,20,20,20,20} (Rocky)
// MSX color byte: (pen << 4) | ink
// Wall color A (idx 8) and B (idx 9) per biome:
//   0 Rocky:  28=grey → 15(white), 14=med red → 8(med red)
//   1 Ice:    13=lt green → 3(lt green), 25=bright cyan → 7(cyan)
//   2 Forest: 24=bright yellow → 14(lt yellow), 18=med green → 2(med green)
//   3 Fire:   14=med red → 8(med red), 4=dark blue → 4(blue)
//   4 Tech:   5=lt blue → 5(lt blue), 11=dark yellow → 13(yellow)
#define BIOME_COUNT 5
static const u8 g_BiomeWallColorA[BIOME_COUNT] = {
    0xF1,  // Rocky:  white
    0x31,  // Ice:    light green
    0xE1,  // Forest: light yellow
    0x81,  // Fire:   medium red
    0x51,  // Tech:   light blue
};
static const u8 g_BiomeWallColorB[BIOME_COUNT] = {
    0x81,  // Rocky:  medium red
    0x71,  // Ice:    cyan
    0x21,  // Forest: medium green
    0x41,  // Fire:   blue
    0xC1,  // Tech:   yellow
};

// Star colors are FIXED (CPC indices 1,2,4 never change per biome)
// idx 1=6(dark red) → MSX 8(med red), idx 2=30(bright white) → MSX 15(white)
#define STAR_COLOR_1  0x81  // medium red on black
#define STAR_COLOR_2  0xF1  // white on black
#define STAR_COLOR_3  0xF1  // white on black (CPC uses black but bg is white there)

static u8 g_CurrentBiome = 0;

//=============================================================================
// DATA STRUCTURES
//=============================================================================

typedef struct { u8 x; u8 y; } TStar;
typedef struct { u8 x; u8 y; u8 active; } TShot;
typedef struct { u8 x; u8 y; u8 active; u8 type; u8 fire_cd; i8 vx; u8 vy; u8 pattern; u8 zig_timer; u8 health; u8 boss_hp_max; u8 boss_lane_x0; u8 boss_vosc; } TEnemy;
typedef struct { u8 x; u8 y; u8 active; i8 vx; u8 vy; u8 pattern; u8 cd; u8 type; } TEnemyShot;
typedef struct { u8 x; u8 y; u8 active; u8 frame; u8 kind; } TExplosion;
typedef struct { u16 score; u8 level; u8 name[3]; } THiScore;
typedef struct { u8 waves; u8 per_wave; u8 mask; u8 flags; u8 intro_mask; } TLevelConfig;

//=============================================================================
// GAME STATE
//=============================================================================

static u8  g_ShipX, g_ShipY;
static const u8  g_ShipSpeedX = 3;
static const u8  g_ShipSpeedY = 3;
static u8  g_ShipExploding  = 0;
static u8  g_ShipExplTimer  = 0;
static u8  g_ShipInvul      = 0;
static u8  g_ShipInvulTimer = 0;
static u8  g_ShipLastLife   = 0;  // 1:1 CPC: ship_last_life
static u8  g_ShipThrust     = 0;
static u8  g_ShipThrustFrame = 0;
static u8  g_ShipThrustLevel = 0;
static u8  g_GameOverDelay  = 0;

static u8  g_PausedFlag     = 0;  // 1 = paused, 0 = playing

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
// Control mode: 0=joystick, 1=keyboard
static u8  g_ControlMode    = 0;  // default: joystick
// Tecles actuals (fila,bit) - valors per defecte = fletxes + Z + P + Return
// KEY_LEFT=row8b4, KEY_RIGHT=row8b7, KEY_UP=row8b5, KEY_DOWN=row8b6
// KEY_Z=row5b7, KEY_P=row9b2, KEY_Q=row4b6
static u8  g_KeyLeft   = KEY_LEFT;
static u8  g_KeyRight  = KEY_RIGHT;
static u8  g_KeyUp     = KEY_UP;
static u8  g_KeyDown   = KEY_DOWN;
static u8  g_KeyFire   = KEY_Z;
static u8  g_KeyPause  = KEY_P;
static u8  g_KeyQuit   = KEY_Q;
static u8  g_LastKeyPressed = 0;
static TShot      g_Shots[MAX_SHOTS];
static u8         g_FireCooldown = 0;
static TEnemy     g_Enemies[MAX_ENEMIES];
static TEnemyShot g_EnemyShots[MAX_ENEMY_SHOTS];
static TExplosion g_Explosions[MAX_EXPLOSIONS];
static u16        g_Score = 0;
static u8         g_Lives = 2;
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
static u8  g_BonusDisplayCnt = 0;  // Comptador per mostrar "XN" al completar wave
static u16 g_WaveBonusBase = 10;
static u8  g_SprBuf[128];  // Sprite attribute buffer (32 × 4 bytes)
static u8  g_WaveSlotX[WAVE_PLAN_MAX];
static u8  g_WaveSlotY[WAVE_PLAN_MAX];
static u8  g_WaveSlotType[WAVE_PLAN_MAX];
static u8  g_WaveUnifiedPatt = PATT_STRAIGHT;
static i8  g_WaveUnifiedVX   = 0;
static u8  g_BossLaneIdx     = 1;
static u8  g_DualBossAimLock = 0;

#define SHIP_SPAWN_X  (GAME_X0 + GAME_W / 2 - SHIP_W / 2)
#define SHIP_SPAWN_Y  (SCREEN_H - SHIP_H - 8)

// Starfield
static TStar g_S1[N1];
static TStar g_S2[N2];
static TStar g_S3[N3];
static u8 g_StarTimer1 = 0;

// Wall scroll
static u8  g_WallPhase = 0;
static u8  g_WallPhaseTimer = 0;

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

// Shot sprite (8x8 in 16x16 slot) - original size
static const u8 g_ShotPattern[8] = {
    0x18, 0x3C, 0x3C, 0x3C, 0x18, 0x18, 0x18, 0x00
};

// Thruster flame (8x8) - small flame at top
static const u8 g_ThrusterPattern[8] = {
    0x18, 0x3C, 0x3C, 0x18, 0x00, 0x00, 0x00, 0x00
};

// Enemy shot sprite (8x8) - 1 pixel vermell centrat
// Format MSX: 1 byte per row, 8 rows
static const u8 g_EnemyShotPattern[8] = {
    0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00  // 2px wide (matches CPC Mode 0 visual size)
};
// Bomber shot (16x16) - allargat, 2px ample x 8px alt (punta avall)
static const u8 g_BomberShotPattern[32] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x01,  // TL: rows 6-7, col 7
    0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,  // BL: rows 8-13, col 7
    0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x80,  // TR: rows 6-7, col 8
    0x80,0x80,0x80,0x80,0x80,0x80,0x00,0x00   // BR: rows 8-13, col 8
};

// Explosion sprites (16x16) - cercles centrats a pixel (8,8)
// ORDRE: TL (0-7), BL (8-15), TR (16-23), BR (24-31) ← com el boss!
// Radi enemic: frames 0,1,2,1; Radi nau: frames 0,1,2,3,2,1

static const u8 g_ExpSprite0[32] = { // radi 2 - quadrat 4x4 al centre
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,  // TL: row7 cols6-7
    0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  // BL: row8 cols6-7
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,  // TR: row7 cols8-9
    0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00   // BR: row8 cols8-9
};
static const u8 g_ExpSprite1[32] = { // radi 4 - cercle petit
    0x00,0x00,0x00,0x00,0x00,0x07,0x0F,0x1F,  // TL: rows5-7 cols4-7
    0x1F,0x0F,0x07,0x00,0x00,0x00,0x00,0x00,  // BL: rows8-10 cols4-7
    0x00,0x00,0x00,0x00,0x00,0xE0,0xF0,0xF8,  // TR: rows5-7 cols8-11
    0xF8,0xF0,0xE0,0x00,0x00,0x00,0x00,0x00   // BR: rows8-10 cols8-11
};
static const u8 g_ExpSprite2[32] = { // radi 6 - cercle mitja
    0x00,0x03,0x0F,0x1F,0x3F,0x7F,0x7F,0xFF,  // TL
    0xFF,0x7F,0x7F,0x3F,0x1F,0x0F,0x03,0x00,  // BL
    0x00,0xC0,0xF0,0xF8,0xFC,0xFE,0xFE,0xFF,  // TR
    0xFF,0xFE,0xFE,0xFC,0xF8,0xF0,0xC0,0x00   // BR
};
// Enemy sprite - converted from CPC 13x13, centered in 16x16 (2px padding each side)
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

// Boss sprite - CPC mothership ovni converted to MSX 16x16
// Like player ship: 2 layers (white + red) that composite to show details
// 
// Structure: 32 bytes = rows 0-15
// Each row: 2 bytes (left half px 0-7, right half px 8-15)
// Bytes 0-7: rows 0-7 left half
// Bytes 8-15: rows 0-7 right half
// Bytes 16-23: rows 8-15 left half
// Bytes 24-31: rows 8-15 right half

// Boss sprite - CPC mothership ovni 16x16 faithful conversion  
// Original CPC: C=Cyan W=White O=Orange Y=Yellow R=Red
// Cyan layer (72): C+W+O → COLOR_CYAN (7)  
// Red layer (76): Y+R → COLOR_MEDIUM_RED (8)
// MSX 16x16 format: TL(8), BL(8), TR(8), BR(8)

static const u8 g_BossWhiteNew[32] = {
    // TL (rows 0-7, cols 0-7)
    0x00, 0x0F, 0x3F, 0x7F, 0xF0, 0xE0, 0xC0, 0x80,
    // BL (rows 8-15, cols 0-7)
    0x80, 0xC0, 0xE0, 0x70, 0x30, 0x18, 0x00, 0x00,
    // TR (rows 0-7, cols 8-15)
    0x00, 0xF0, 0xFC, 0xFE, 0x0F, 0x07, 0x03, 0x01,
    // BR (rows 8-15, cols 8-15)
    0x01, 0x03, 0x07, 0x0E, 0x0C, 0x18, 0x00, 0x00
};

// Red layer: core nucleus + yellow accents
static const u8 g_BossRedNew[32] = {
    // TL (rows 0-7, cols 0-7)
    0x00, 0x00, 0x00, 0x00, 0x0F, 0x1F, 0x3F, 0x7F,
    // BL (rows 8-15, cols 0-7)
    0x7F, 0x3F, 0x1F, 0x0F, 0x0F, 0x06, 0x06, 0x00,
    // TR (rows 0-7, cols 8-15)
    0x00, 0x00, 0x00, 0x00, 0xF0, 0xF8, 0xFC, 0xFE,
    // BR (rows 8-15, cols 8-15)
    0xFE, 0xFC, 0xF8, 0xF0, 0xF0, 0x60, 0x60, 0x00
};

//=============================================================================
// WALL SCROLL TILES
//=============================================================================

static const u8 g_RockBase[8] = {
    0xF7,  // 11110111 - small gap
    0xDB,  // 11011011 - two offset gaps
    0xFE,  // 11111110 - edge gap
    0x6D,  // 01101101 - irregular middle
    0xFB,  // 11111011 - small gap
    0xB7,  // 10110111 - two gaps
    0xEF,  // 11101111 - small gap
    0xDD,  // 11011101 - two gaps
};



void InitWallTiles()
{
    u8 phase, row;
    u8 tile[8];
    u8 rcol[8];
    u8 ca = g_BiomeWallColorA[g_CurrentBiome];
    u8 cb = g_BiomeWallColorB[g_CurrentBiome];
    for (row = 0; row < 8; row++) rcol[row] = (row & 1) ? cb : ca;
    for (phase = 0; phase < 8; phase++)
    {
        for (row = 0; row < 8; row++)
            tile[row] = g_RockBase[(row + phase) & 7];
        VDP_LoadBankPattern_GM2(tile, 1, 0, WALL_TILE_BASE + phase);
        VDP_LoadBankPattern_GM2(tile, 1, 1, WALL_TILE_BASE + phase);
        VDP_LoadBankPattern_GM2(tile, 1, 2, WALL_TILE_BASE + phase);
        VDP_LoadBankColor_GM2(rcol, 1, 0, WALL_TILE_BASE + phase);
        VDP_LoadBankColor_GM2(rcol, 1, 1, WALL_TILE_BASE + phase);
        VDP_LoadBankColor_GM2(rcol, 1, 2, WALL_TILE_BASE + phase);
    }
}

void UpdateWallScroll()
{
    g_WallPhaseTimer++;
    if (g_WallPhaseTimer < WALL_SPEED) return;
    g_WallPhaseTimer = 0;
    g_WallPhase = (g_WallPhase + 1) & 7;

    u8 row;
    u16 base = g_ScreenLayoutLow;
    for (row = 0; row < 24; row++)
    {
        u8 tile[2];
        u16 addr = base + (u16)(row * 32);
        tile[0] = WALL_TILE_BASE + ((g_WallPhase + row) & 7);
        tile[1] = tile[0];
        // Write cols 0-1 i 20-21 (2 bytes cada grup, un sol setup VRAM per grup)
        VDP_WriteVRAM(tile, addr,      g_ScreenLayoutHigh, 2);
        VDP_WriteVRAM(tile, addr + 20, g_ScreenLayoutHigh, 2);
    }
}

//=============================================================================
// STARFIELD - pixels individuals escrits directament a VRAM
// En Mode 2 cada tile 8x8 te el seu propi patró.
// Usem tiles especials: STAR_TILE_BASE+row (0-7) = pixel encès a la fila 'row'
// STAR_TILE_BLANK = tile buit per esborrar
//=============================================================================

#define MENU_STAR_TILE_BASE 174   // 8 tiles for menu star states (174-181)

#define STAR_TILE_BASE_1  182   // tiles 182-189: capa lenta  - blau fosc  (0x41)
#define STAR_TILE_BASE_2  190   // tiles 190-197: capa mitja  - blau clar  (0x51)
#define STAR_TILE_BASE_3  198   // tiles 198-205: capa rapida - blanc      (0xF1)

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
    InitStarTilesForBase(STAR_TILE_BASE_1, STAR_COLOR_1);
    InitStarTilesForBase(STAR_TILE_BASE_2, STAR_COLOR_2);
    InitStarTilesForBase(STAR_TILE_BASE_3, STAR_COLOR_3);
}

void UpdateBiomeColors()
{
    u8 newBiome = (u8)((g_Level - 1) / 5);
    if (newBiome > 4) newBiome = 4;
    if (newBiome == g_CurrentBiome) return;
    g_CurrentBiome = newBiome;
    InitWallTiles();
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
    static const u8 xoff[16] = {11, 37, 63, 21, 51, 7, 41, 27, 57, 17, 47, 31, 3, 53, 13, 45};
    static const u8 yoff[16] = {17, 83, 43, 131, 67, 107, 23, 157, 53, 97, 173, 61, 113, 79, 137, 149};
    u8 span = (u8)(STAR_X1 - STAR_X0);
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

// Forward declarations
void HudDrawText(u8 col, u8 row, const char* s, u8 colorByte);
void HudDrawHLine(u8 col, u8 row, u8 len, u8 colorByte);
void sfxLevelUp(void);
void sfxEnemyShot(void);
void sfxBeep(void);
void EnterPostGame(u8 won);
void InitGamePlay(void);
static u8 BossTierFromLevel(u8 lv);




// Boss max HP per tier (defined in table below)
static u8 GetBossMaxHP(u8 tier)
{
    return (u8)(BOSS_HP_BASE + tier * BOSS_HP_PER_TIER);
}

static u8 ScoreFloorExtraLives(u16 s)
{
    u8 n = 0;
    while (s >= EXTRA_LIFE_EVERY)
    {
        s = (u16)(s - EXTRA_LIFE_EVERY);
        n++;
    }
    return n;
}

static void AddScore(u16 points)
{
    u16 old_score = g_Score;
    u16 new_score = (u16)(g_Score + points);
    u8 hi, lo;

    g_Score = (new_score < g_Score) ? 65535u : new_score;
    hi = ScoreFloorExtraLives(g_Score);
    lo = ScoreFloorExtraLives(old_score);
    while (hi > lo)
    {
        lo++;
        g_Lives++;
        g_BonusDisplayCnt = 20;
        sfxBeep();
    }
    g_HudDirty = 1;
}

void UpdateHUD()
{
    static u16 last_score = 0xFFFF;
    static u8 last_level = 0xFF;
    static u8 last_lives = 0xFF;
    static u8 last_bonus_visible = 0xFF;
    static u8 last_bonus_wave = 0xFF;
    static u8 last_has_boss = 0xFF;
    static u8 last_boss_filled = 0xFF;
    u8 i, t;
    u8 has_boss = 0;
    u8 cur_hp = 0;
    u8 max_hp = 0;
    u8 boss_filled = 0;
    u8 full;
    u8 bonus_visible;
    u16 s;
    char buf[6];

    if (!g_HudDirty) return;

    full = (last_score == 0xFFFF);

    if (full)
    {
        HudDrawText(HUD_COL, 0, "SCORE", HUD_FONT_COLOR_NRM);
        HudDrawText(HUD_COL, 6, "LEVEL", HUD_FONT_COLOR_NRM);
        HudDrawText(HUD_COL, 9, "LIVES", HUD_FONT_COLOR_NRM);
    }

    if (full || last_score != g_Score)
    {
        s = g_Score;
        buf[4] = (char)('0' + s % 10); s /= 10;
        buf[3] = (char)('0' + s % 10); s /= 10;
        buf[2] = (char)('0' + s % 10); s /= 10;
        buf[1] = (char)('0' + s % 10); s /= 10;
        buf[0] = (char)('0' + s % 10);
        buf[5] = 0;
        HudDrawText(HUD_COL, 1, buf, HUD_FONT_COLOR_HI);
        last_score = g_Score;
    }

    bonus_visible = (g_BonusDisplayCnt > 0) ? 1 : 0;
    if (full || last_bonus_visible != bonus_visible || (bonus_visible && last_bonus_wave != g_WaveTotal))
    {
        if (bonus_visible)
        {
            HudDrawText(HUD_COL, 3, "BONUS", HUD_FONT_COLOR_NRM);
            buf[0] = 'X';
            buf[1] = (char)('0' + g_WaveTotal);
            buf[2] = 0;
            HudDrawText(HUD_COL, 4, buf, HUD_FONT_COLOR_HI);
        }
        else
        {
            HudDrawText(HUD_COL, 3, "     ", HUD_FONT_COLOR_NRM);
            HudDrawText(HUD_COL, 4, "     ", HUD_FONT_COLOR_NRM);
        }
        last_bonus_visible = bonus_visible;
        last_bonus_wave = g_WaveTotal;
    }

    if (full || last_level != g_Level)
    {
        buf[0] = (char)('0' + g_Level / 10);
        buf[1] = (char)('0' + g_Level % 10);
        buf[2] = 0;
        HudDrawText(HUD_COL, 7, buf, HUD_FONT_COLOR_HI);
        last_level = g_Level;
    }

    if (full || last_lives != g_Lives)
    {
        buf[0] = (char)('0' + g_Lives);
        buf[1] = 0;
        HudDrawText(HUD_COL, 10, buf, HUD_FONT_COLOR_HI);
        last_lives = g_Lives;
    }

    for (i = 0; i < MAX_ENEMIES; i++)
    {
        if (g_Enemies[i].active && g_Enemies[i].type == ENEMY_TYPE_BOSS)
        {
            cur_hp = g_Enemies[i].health;
            max_hp = g_Enemies[i].boss_hp_max;
            has_boss = 1;
            break;
        }
    }
    if (has_boss)
    {
        if (!max_hp) max_hp = GetBossMaxHP(BossTierFromLevel(g_Level));
        boss_filled = (u8)((u16)cur_hp * 5u / max_hp);
        if (full || last_has_boss != has_boss)
            HudDrawText(HUD_COL, 13, "HP", HUD_FONT_COLOR_HI);
        if (full || last_has_boss != has_boss || last_boss_filled != boss_filled)
        {
            for (t = 0; t < 5; t++)
                VDP_Poke_GM2((u8)(HUD_COL + 3 + t), 13, (t < boss_filled) ? BAR_FILL_TILE : 0);
        }
    }
    else if (full || last_has_boss != has_boss)
    {
        HudDrawText(HUD_COL, 13, "          ", HUD_FONT_COLOR_NRM);
    }
    last_has_boss = has_boss;
    last_boss_filled = boss_filled;

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

// Boss tier data: HP, vertical speed, fire rate cooldown, bullet spread count
typedef struct { u8 hp; u8 vy; u8 fire_cd; u8 num_bullets; } TBossTier;
static const TBossTier g_BossTierTable[BOSS_TIERS_MAX] = {
    {15, 1, 8, 2},
    {20, 1, 8, 2},
    {25, 2, 7, 3},
    {30, 2, 6, 3},
    {35, 3, 6, 3}
};
static const u8 g_BossFireCdBase[4] = {8, 8, 7, 6};
static const u8 g_BossFireCdRandMask[4] = {4, 3, 3, 2};

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

// 1:1 CPC: pickHomogeneousTypeForWave - sense filtres, caps s'apliquen despres
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

static u8 HeavyWaveCapForLevel()
{
    if (g_Level >= 21) return 5;
    if (g_Level >= 16) return 4;
    if (g_Level >= 11) return 3;
    return 3;
}

static u8 BomberWaveCapForLevel()
{
    if (g_Level >= 23) return 5;
    if (g_Level >= 22) return 4;
    return 3;
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

static void BuildWaveLayoutFastRank(u8 n)
{
    BuildWaveLayoutRank(n);
    if (n == 3)
    {
        g_WaveSlotY[0] = 0;
        g_WaveSlotY[1] = 20;
        g_WaveSlotY[2] = 20;
    }
    else if (n == 5)
    {
        g_WaveSlotY[0] = 20;
        g_WaveSlotY[1] = 20;
        g_WaveSlotY[2] = 20;
        g_WaveSlotY[3] = 0;
        g_WaveSlotY[4] = 0;
    }
}

// 1:1 CPC: buildWaveSlotLayoutIndian
static void BuildWaveLayoutIndian(u8 n)
{
    u8 i, x;
    x = (u8)(GAME_X0 + MOD_POW2(Math_GetRandom8(), 138));
    if (x > GAME_X1 - ENEMY_W) x = (u8)(GAME_X1 - ENEMY_W);
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
        g_Enemies[i].boss_hp_max = 0;
        g_Enemies[i].boss_vosc = 0;
        if (type == ENEMY_TYPE_BOMBER)
        {
            g_Enemies[i].fire_cd = (u8)(8u + (Math_GetRandom8() & 7u));
            g_Enemies[i].pattern = (u8)(Math_GetRandom8() & 1u);
        }
        else
            g_Enemies[i].fire_cd = 0;
        if (type == ENEMY_TYPE_DIVER) g_EnemyShots[i].cd = (u8)(Math_GetRandom8() & 1u);
        else g_EnemyShots[i].cd = (u8)((g_WaveSpawned << 1) + (Math_GetRandom8() & 7u));
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
    u8 active;
    u8 budget;
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
    // RANK: spawna en petits blocs per evitar pics de CPU quan apareix la formacio
    active = CountActiveEnemies();
    budget = 2;
    while (budget && g_WaveSpawned < g_WaveTotal && active < g_WaveTotal)
    {
        SpawnOneFromQueue();
        active++;
        budget--;
    }
}

// 1:1 CPC: startNewWave
static void StartNewWave()
{
    const TLevelConfig* cfg = LevelConfigGet(g_Level);
    u8 mask, total, tier, i;

    g_WaveKilled    = 0;
    g_WaveActive    = 1;
    g_WaveSpawned   = 0;
    g_WaveIndianDelay = 0;

    // Handle boss waves
    if (cfg->flags & LCFG_F_BOSS1)
    {
        // Single boss
        g_WaveTotal = 1;
        tier = (g_Level >= 25) ? 4 : ((g_Level - 1) / 5);   // Tier 0-4 based on level
        g_WaveSpawned = 1;
        
        // Find empty enemy slot and spawn boss
        for (i = 0; i < MAX_ENEMIES; i++)
        {
            if (!g_Enemies[i].active)
            {
                g_BossLaneIdx = 1;
                g_Enemies[i].boss_lane_x0 = (u8)(GAME_X0 + (GAME_W - ENEMY_BOSS_W) / 2);
                g_Enemies[i].x       = g_Enemies[i].boss_lane_x0;
                g_Enemies[i].y       = 18;
                g_Enemies[i].active  = 1;
                g_Enemies[i].type    = ENEMY_TYPE_BOSS;
                g_Enemies[i].health  = g_BossTierTable[tier].hp;
                g_Enemies[i].boss_hp_max = g_Enemies[i].health;
                g_Enemies[i].vy      = g_BossTierTable[tier].vy;
                g_Enemies[i].vx      = 1;
                g_Enemies[i].pattern = (u8)(Math_GetRandom8() & 7u);
                g_Enemies[i].zig_timer = (u8)(8u + (Math_GetRandom8() & 7u));
                g_Enemies[i].fire_cd = g_BossTierTable[tier].fire_cd;
                g_Enemies[i].boss_vosc = BOSS_VOSC_DESC;
                break;
            }
        }
        g_WaveBonusBase = 0;
        g_SpawnTimer = (u8)(SPAWN_BASE + MOD_POW2(Math_GetRandom8(), SPAWN_VARIANCE + 1));
        sfxLevelUp();  // So d'avís de boss
        g_HudDirty = 1;
        return;
    }
    if (cfg->flags & LCFG_F_BOSS2)
    {
        // Dual boss (Level 25)
        g_WaveTotal = 2;
        tier = 3; // CPC final dual: HP per boss like single level 20
        g_WaveSpawned = 2;
        g_BossLaneIdx = (Math_GetRandom8() & 1u) ? 0 : 2;
        
        // Spawn two bosses
        for (i = 0; i < MAX_ENEMIES && g_WaveSpawned > 0; i++)
        {
            if (!g_Enemies[i].active)
            {
                if (g_BossLaneIdx == 0) g_Enemies[i].boss_lane_x0 = (u8)(GAME_X0 + 24);
                else if (g_BossLaneIdx == 2) g_Enemies[i].boss_lane_x0 = (u8)(GAME_X1 - ENEMY_BOSS_W - 24);
                else g_Enemies[i].boss_lane_x0 = (u8)(GAME_X0 + (GAME_W - ENEMY_BOSS_W) / 2);
                g_Enemies[i].x       = g_Enemies[i].boss_lane_x0;
                g_Enemies[i].y       = 18;
                g_Enemies[i].active  = 1;
                g_Enemies[i].type    = ENEMY_TYPE_BOSS;
                g_Enemies[i].health  = g_BossTierTable[tier].hp;
                g_Enemies[i].boss_hp_max = g_Enemies[i].health;
                g_Enemies[i].vy      = g_BossTierTable[tier].vy;
                g_Enemies[i].vx      = 1;
                g_Enemies[i].pattern = (u8)(Math_GetRandom8() & 7u);
                g_Enemies[i].zig_timer = (u8)(8u + (Math_GetRandom8() & 7u));
                g_Enemies[i].fire_cd = g_BossTierTable[tier].fire_cd;
                g_Enemies[i].boss_vosc = BOSS_VOSC_DESC;
                g_BossLaneIdx = (u8)(2u - g_BossLaneIdx);
                g_WaveSpawned--;
            }
        }
        g_WaveSpawned = 2;
        g_WaveBonusBase = 0;
        g_SpawnTimer = (u8)(SPAWN_BASE + MOD_POW2(Math_GetRandom8(), SPAWN_VARIANCE + 1));
        sfxLevelUp();
        return;
    }

    // Regular enemy wave
    total = cfg->per_wave;
    if (total > WAVE_PLAN_MAX) total = WAVE_PLAN_MAX;
    g_WaveTotal = total;

    mask = cfg->mask;
    if (cfg->intro_mask && g_WavesCleared == 0)
        mask = cfg->intro_mask;

    // Mode indian o rank aleatori (1:1 CPC: 25% indian)
    g_WaveMode = ((Math_GetRandom8() & 3) == 0) ? WAVE_MODE_INDIAN : WAVE_MODE_RANK;

    BuildWaveTypePlan(total, mask);

    if (g_WaveSlotType[0] == ENEMY_TYPE_DIVER)
    {
        g_WaveMode = WAVE_MODE_INDIAN;
        if (g_WaveTotal < DIVER_WAVE_MIN) g_WaveTotal = DIVER_WAVE_MIN;
        if (g_WaveTotal > DIVER_WAVE_MAX) g_WaveTotal = DIVER_WAVE_MAX;
        BuildWaveTypePlan(g_WaveTotal, mask);
    }
    if (g_WaveSlotType[0] == ENEMY_TYPE_HEAVY && g_WaveTotal > HeavyWaveCapForLevel())
        g_WaveTotal = HeavyWaveCapForLevel();
    if (g_WaveSlotType[0] == ENEMY_TYPE_BOMBER && g_WaveTotal > BomberWaveCapForLevel())
        g_WaveTotal = BomberWaveCapForLevel();

    // Layout segons tipus; MSX: mai mes de 2 enemics per fila.
    if (g_WaveMode == WAVE_MODE_INDIAN)
    {
        BuildWaveLayoutIndian(g_WaveTotal);
    }
    else if (g_WaveSlotType[0] == ENEMY_TYPE_FAST)
        BuildWaveLayoutFastRank(g_WaveTotal);
    else
        BuildWaveLayoutRank(g_WaveTotal);

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
    if (g_ShipExploding) return;  // No avancis wave si el jugador esta morint
    if (CountActiveEnemies()) return;
    if (g_WaveSpawned < g_WaveTotal) return;

    // Tota l'onada eliminada: suma bonus
    if (g_WaveKilled == g_WaveTotal)
    {
        u16 bonus = 0;
        u8 wk;
        for (wk = 0; wk < g_WaveTotal; wk++)
            bonus = (u16)(bonus + g_WaveBonusBase);
        AddScore(bonus);
        g_BonusDisplayCnt = 30;  // Mostra "XN" cada cop que es compleix una wave
    }

    g_WavesCleared++;
    // Comprova si cal pujar de nivell
    {
        const TLevelConfig* cfg = LevelConfigGet(g_Level);
        if (g_WavesCleared >= cfg->waves)
        {
            g_WavesCleared = 0;
            if (g_Level >= ENDGAME_FINAL_LEVEL)
            {
                EnterPostGame(1);
            }
            else
            {
                g_Level++;
                sfxLevelUp();
                UpdateBiomeColors();
            }
            g_HudDirty = 1;
        }
    }
    g_WaveActive = 0;
}

void InitEnemies()
{
    u8 i;
    for (i = 0; i < MAX_ENEMIES;     i++)
    {
        g_Enemies[i].active = 0;
        g_Enemies[i].boss_hp_max = 0;
        g_Enemies[i].boss_vosc = 0;
    }
    for (i = 0; i < MAX_ENEMY_SHOTS; i++)
    {
        g_EnemyShots[i].active = 0;
        g_EnemyShots[i].pattern = 12;
        g_EnemyShots[i].vy = ENEMYSHOT_SPEED_Y;
        g_EnemyShots[i].cd = (u8)(i * ENEMYSHOT_STAGGER);
        g_EnemyShots[i].type = ENEMYSHOT_TYPE_BULLET;
    }
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
    g_DualBossAimLock = 0;
}

// CPC: enemyShotAimVX - apunta cap a la nau
static i8 enemyShotAimVX(u8 sx)
{
    i16 dx = (i16)(g_ShipX + (SHIP_W >> 1)) - (i16)sx;
    if      (dx <= -10) return -ENEMYSHOT_VX_FAST;
    else if (dx <=  -3) return -ENEMYSHOT_VX_SLOW;
    else if (dx >=  10) return  ENEMYSHOT_VX_FAST;
    else if (dx >=   3) return  ENEMYSHOT_VX_SLOW;
    else if (dx < 0) return -ENEMYSHOT_VX_SLOW;
    else if (dx > 0) return ENEMYSHOT_VX_SLOW;
    return 0;
}

static i8 enemyShotAimVXDiver(u8 sx)
{
    i16 dx = (i16)(g_ShipX + (SHIP_W >> 1)) - (i16)sx;
    if      (dx <= -16) return -ENEMYSHOT_VX_SLOW;
    else if (dx <=  -6) return -1;
    else if (dx >=  16) return  ENEMYSHOT_VX_SLOW;
    else if (dx >=   6) return  1;
    else if (dx < 0) return -1;
    else if (dx > 0) return 1;
    return 0;
}

static i8 enemyShotAimVXBoss(u8 sx)
{
    i16 dx = (i16)g_ShipX - (i16)sx;
    if      (dx <= -16) return -ENEMYSHOT_VX_FAST;
    else if (dx <=  -6) return -ENEMYSHOT_VX_SLOW;
    else if (dx >=  16) return  ENEMYSHOT_VX_FAST;
    else if (dx >=   6) return  ENEMYSHOT_VX_SLOW;
    return 0;
}

static u8 AllocEnemyShotSlot()
{
    u8 k;
    for (k = 0; k < MAX_ENEMY_SHOTS; k++)
        if (!g_EnemyShots[k].active) return k;
    return 255;
}

static u8 HeavyShotCooldown()
{
    if (g_Level >= 21) return (u8)(5u + (Math_GetRandom8() & 3u));
    if (g_Level >= 16) return (u8)(6u + (Math_GetRandom8() & 3u));
    if (g_Level >= 11) return (u8)(7u + (Math_GetRandom8() & 4u));
    return (u8)(8u + (Math_GetRandom8() & 5u));
}

static u8 DiverShotCooldown()
{
    if (g_Level >= 21) return (u8)(5u + (Math_GetRandom8() & 2u));
    if (g_Level >= 16) return (u8)(6u + (Math_GetRandom8() & 2u));
    if (g_Level >= 11) return (u8)(7u + (Math_GetRandom8() & 2u));
    return (u8)(8u + (Math_GetRandom8() & 3u));
}

static u8 BossTierFromLevel(u8 lv)
{
    if (lv >= 25) return 4;
    if (lv < 10) return 0;
    if (lv < 15) return 1;
    if (lv < 20) return 2;
    return 3;
}

static u8 BossBulletVyFromTier(u8 tier)
{
    u8 v = (u8)(ENEMYSHOT_SPEED_Y + tier);
    if (tier >= 3) v++;
    return v;
}

static void SpawnEnemyBullet(u8 xspawn, u8 yspawn, i8 vx, u8 vy, u8 type)
{
    u8 k = AllocEnemyShotSlot();
    i16 xx = xspawn;
    if (k == 255) return;
    if (xx < GAME_X0) xx = GAME_X0;
    if (xx > (i16)(GAME_X1 - ENEMYSHOT_W)) xx = (i16)(GAME_X1 - ENEMYSHOT_W);
    g_EnemyShots[k].active = 1;
    g_EnemyShots[k].x = (u8)xx;
    g_EnemyShots[k].y = yspawn;
    g_EnemyShots[k].vx = vx;
    g_EnemyShots[k].vy = vy;
    g_EnemyShots[k].cd = 0;
    g_EnemyShots[k].type = type;
    g_EnemyShots[k].pattern = (type == ENEMYSHOT_TYPE_BOMB) ? 88 : 12;
    sfxEnemyShot();
}

static void BomberFireVolley(u8 i)
{
    u8 bx = (u8)(g_Enemies[i].x + ENEMY_W / 2 - 1);
    u8 by = (u8)(g_Enemies[i].y + ENEMY_H);
    i16 dxShip = (i16)g_ShipX - (i16)bx;
    if (g_Enemies[i].pattern & 1u)
    {
        SpawnEnemyBullet((u8)(bx - 1u), by, -1, (u8)(ENEMYSHOT_SPEED_Y - 1u), ENEMYSHOT_TYPE_BOMB);
        SpawnEnemyBullet((u8)(bx + 1u), by,  1, (u8)(ENEMYSHOT_SPEED_Y - 1u), ENEMYSHOT_TYPE_BOMB);
    }
    else
    {
        SpawnEnemyBullet(bx, by, enemyShotAimVXBoss(bx), ENEMYSHOT_SPEED_Y, ENEMYSHOT_TYPE_BOMB);
        if (dxShip >= -8 && dxShip <= 8)
            SpawnEnemyBullet((u8)(bx + 1u), by, 0, (u8)(ENEMYSHOT_SPEED_Y + 1u), ENEMYSHOT_TYPE_BOMB);
    }
    g_Enemies[i].pattern ^= 1u;
    g_Enemies[i].fire_cd = (u8)(13u + (Math_GetRandom8() & 7u));
}

static u8 BossTryAimedShot(u8 dual_mode, u8 x, u8 y, i8 vx, u8 vy)
{
    if (dual_mode)
    {
        if (g_DualBossAimLock) return 0;
        g_DualBossAimLock = 10;
        if (vy > 4) vy--;
    }
    SpawnEnemyBullet(x, y, vx, vy, ENEMYSHOT_TYPE_BULLET);
    return 1;
}

// Boss shots: alterna 2 aimed (CPC) o 3 en ventall real
static u8 CountActiveBosses()
{
    u8 i, n = 0;
    for (i = 0; i < MAX_ENEMIES; i++)
        if (g_Enemies[i].active && g_Enemies[i].type == ENEMY_TYPE_BOSS) n++;
    return n;
}

static void BossFireVolley(u8 i)
{
    u8 pat = (u8)(g_Enemies[i].pattern & 3u);
    u8 tier = BossTierFromLevel(g_Level);
    u8 bx = (u8)(g_Enemies[i].x + ENEMY_BOSS_W / 2);
    u8 by = (u8)(g_Enemies[i].y + ENEMY_BOSS_H);
    u8 bvy = BossBulletVyFromTier(tier);
    u8 dual_mode = (CountActiveBosses() >= 2u) ? 1u : 0u;
    u8 bvyFan = (u8)((bvy > 6u) ? (bvy - 2u) : (bvy - 1u));
    u8 cd, ti;
    i16 dxShip = (i16)g_ShipX - (i16)bx;
    if (bvyFan < 4) bvyFan = 4;
    g_Enemies[i].pattern = (u8)((pat + 1u) & 3u);

    switch (pat)
    {
        case 0:
            if (BossTryAimedShot(dual_mode, bx, by, enemyShotAimVXBoss(bx), bvy))
            {
                if (dxShip >= -5 && dxShip <= 5)
                    SpawnEnemyBullet((u8)(bx + 1u), by, 0, (u8)(bvy + 1u), ENEMYSHOT_TYPE_BULLET);
                else if (tier >= 2)
                    BossTryAimedShot(dual_mode, (u8)(bx - 1u), by, enemyShotAimVXBoss((u8)(bx - 1u)), bvy);
            }
            break;
        case 1:
            SpawnEnemyBullet((u8)(bx - 6u), by, -2, bvyFan, ENEMYSHOT_TYPE_BULLET);
            SpawnEnemyBullet((u8)(bx + 6u), by,  2, bvyFan, ENEMYSHOT_TYPE_BULLET);
            if (tier >= 2) SpawnEnemyBullet(bx, by, 0, (u8)(bvyFan + 1u), ENEMYSHOT_TYPE_BULLET);
            break;
        case 2:
            SpawnEnemyBullet((u8)(bx - 7u), by, -2, bvyFan, ENEMYSHOT_TYPE_BULLET);
            SpawnEnemyBullet(bx, by, 0, bvyFan, ENEMYSHOT_TYPE_BULLET);
            SpawnEnemyBullet((u8)(bx + 7u), by, 2, bvyFan, ENEMYSHOT_TYPE_BULLET);
            if (tier >= 2) BossTryAimedShot(dual_mode, bx, by, enemyShotAimVXBoss(bx), (u8)(bvyFan + 1u));
            if (tier >= 3) SpawnEnemyBullet((u8)(bx + 3u), by, -2, (u8)(bvyFan + 1u), ENEMYSHOT_TYPE_BULLET);
            break;
        default:
            BossTryAimedShot(dual_mode, bx, by, enemyShotAimVXBoss(bx), bvy);
            SpawnEnemyBullet((u8)(bx + 8u), by, -1, bvyFan, ENEMYSHOT_TYPE_BULLET);
            if (dxShip >= -7 && dxShip <= 7) SpawnEnemyBullet((u8)(bx - 7u), by, 1, bvyFan, ENEMYSHOT_TYPE_BULLET);
            if (tier >= 3) BossTryAimedShot(dual_mode, (u8)(bx - 2u), by, enemyShotAimVXBoss((u8)(bx - 2u)), bvyFan);
            break;
    }

    ti = tier;
    if (ti > 3) ti = 3;
    cd = (u8)(g_BossFireCdBase[ti] + (Math_GetRandom8() & g_BossFireCdRandMask[ti]));
    if (cd < 7) cd = 7;
    if (tier >= 3 && pat == 2) cd = (u8)(cd + 3u);
    g_Enemies[i].fire_cd = cd;
}

void UpdateEnemies()
{
    u8 i, tier;

    // 1:1 CPC: spawnWave (no spawna mentre explota o invulnerable)
    if (!g_ShipExploding && !g_ShipInvul)
        SpawnWave();

    // 1:1 CPC: moveEnemies
    for (i = 0; i < MAX_ENEMIES; i++)
    {
        if (!g_Enemies[i].active) continue;

        // Boss-specific behavior
        if (g_Enemies[i].type == ENEMY_TYPE_BOSS)
        {
            u8 xmn = GAME_X0;
            u8 xmx = (u8)(GAME_X1 - ENEMY_BOSS_W);
            u8 pivot = g_Enemies[i].boss_lane_x0;
            u8 hold = BOSS_HOLD_Y;
            u8 ymin = (u8)(hold - BOSS_Y_OSC);
            u8 dual_mode = (CountActiveBosses() >= 2u) ? 1u : 0u;
            i16 nxx, dxp;
            tier = BossTierFromLevel(g_Level);
            if (dual_mode)
            {
                i16 lx = (i16)pivot - BOSS_DUAL_LANE_HALF;
                i16 rx = (i16)pivot + BOSS_DUAL_LANE_HALF;
                if (lx < GAME_X0) lx = GAME_X0;
                if (rx > (i16)(GAME_X1 - ENEMY_BOSS_W)) rx = (i16)(GAME_X1 - ENEMY_BOSS_W);
                xmn = (u8)lx;
                xmx = (u8)rx;
            }
            
            if (ymin < GAME_Y0 + 4u) ymin = GAME_Y0 + 4u;
            if (g_Enemies[i].boss_vosc == BOSS_VOSC_DESC)
            {
                if (g_Enemies[i].y < hold)
                {
                    g_Enemies[i].y = (u8)(g_Enemies[i].y + g_Enemies[i].vy);
                    if (g_Enemies[i].y > hold) g_Enemies[i].y = hold;
                }
                if (g_Enemies[i].y >= hold) g_Enemies[i].boss_vosc = BOSS_VOSC_UP;
            }
            else if (g_Enemies[i].boss_vosc == BOSS_VOSC_UP)
            {
                if (g_Enemies[i].y > ymin) g_Enemies[i].y--;
                else g_Enemies[i].boss_vosc = BOSS_VOSC_DOWN;
            }
            else
            {
                if (g_Enemies[i].y < hold) g_Enemies[i].y++;
                else g_Enemies[i].boss_vosc = BOSS_VOSC_UP;
            }
            
            // CPC-style random lateral oscillation
            if (g_Enemies[i].zig_timer)
                g_Enemies[i].zig_timer--;
            else
            {
                u8 r;
                if (tier >= 3) g_Enemies[i].zig_timer = (u8)(6u + (Math_GetRandom8() & 7u));
                else if (tier == 2) g_Enemies[i].zig_timer = (u8)(7u + (Math_GetRandom8() & 7u));
                else if (tier == 1) g_Enemies[i].zig_timer = (u8)(8u + (Math_GetRandom8() & 7u));
                else g_Enemies[i].zig_timer = (u8)(8u + (Math_GetRandom8() & 11u));
                r = Math_GetRandom8() & 7u;
                if (tier >= 2)
                {
                    if (r < 4u) g_Enemies[i].vx = (g_Enemies[i].vx < 0) ? -2 : 2;
                    else g_Enemies[i].vx = (Math_GetRandom8() & 1u) ? 2 : -2;
                }
                else
                {
                    if (r < 3u) g_Enemies[i].vx = (g_Enemies[i].vx < 0) ? -2 : 2;
                    else if (r < 6u) g_Enemies[i].vx = (Math_GetRandom8() & 1u) ? 1 : -1;
                    else g_Enemies[i].vx = (Math_GetRandom8() & 1u) ? 2 : -2;
                }
                if (!g_Enemies[i].vx) g_Enemies[i].vx = 1;
            }
            
            dxp = (i16)pivot - (i16)g_Enemies[i].x;
            if (dxp > 10 && g_Enemies[i].vx < 2) g_Enemies[i].vx++;
            else if (dxp < -10 && g_Enemies[i].vx > -2) g_Enemies[i].vx--;
            nxx = (i16)g_Enemies[i].x + (i16)g_Enemies[i].vx;
            if (nxx < (i16)xmn) { g_Enemies[i].x = xmn; g_Enemies[i].vx = (g_Enemies[i].vx < 0) ? 2 : 1; }
            else if (nxx > (i16)xmx) { g_Enemies[i].x = xmx; g_Enemies[i].vx = (g_Enemies[i].vx > 0) ? -2 : -1; }
            else g_Enemies[i].x = (u8)nxx;
            
            // Boss firing: determine tier to get bullet count
            tier = (g_Level >= 25) ? 4 : ((g_Level - 1) / 5);
            if (g_Enemies[i].fire_cd > 0) 
                g_Enemies[i].fire_cd--;
            else 
            { 
                BossFireVolley(i);
            }
        }
        else
        {
            // Regular enemy movement
            g_Enemies[i].y += g_Enemies[i].vy;

            if (g_Enemies[i].pattern == PATT_ZIGZAG)
            {
                if (g_Enemies[i].zig_timer) g_Enemies[i].zig_timer--;
                else { g_Enemies[i].zig_timer = 6; g_Enemies[i].vx = -g_Enemies[i].vx; }
                g_Enemies[i].x = (u8)((i8)g_Enemies[i].x + g_Enemies[i].vx);
                if (g_Enemies[i].x < GAME_X0)        g_Enemies[i].x = GAME_X0;
                if (g_Enemies[i].x > GAME_X1-ENEMY_W) g_Enemies[i].x = (u8)(GAME_X1-ENEMY_W);
            }
            else if (g_Enemies[i].pattern == PATT_DIAGONAL)
            {
                if      (g_Enemies[i].vx < 0 && g_Enemies[i].x <= GAME_X0)        g_Enemies[i].vx = 1;
                else if (g_Enemies[i].vx > 0 && g_Enemies[i].x >= GAME_X1-ENEMY_W) g_Enemies[i].vx = -1;
                else g_Enemies[i].x = (u8)((i8)g_Enemies[i].x + g_Enemies[i].vx);
            }

            if (g_Enemies[i].type == ENEMY_TYPE_BOMBER)
            {
                if (g_Enemies[i].fire_cd) g_Enemies[i].fire_cd--;
                else BomberFireVolley(i);
            }
            else if (g_Enemies[i].type == ENEMY_TYPE_HEAVY)
            {
                if (!g_EnemyShots[i].cd)
                {
                    u8 k = AllocEnemyShotSlot();
                    if (k != 255)
                    {
                        u8 bx = (u8)(g_Enemies[i].x + ENEMY_W / 2 - 1);
                        g_EnemyShots[k].active = 1;
                        g_EnemyShots[k].x = bx;
                        g_EnemyShots[k].y = (u8)(g_Enemies[i].y + ENEMY_H);
                        g_EnemyShots[k].vx = enemyShotAimVX(bx);
                        g_EnemyShots[k].vy = ENEMYSHOT_SPEED_Y;
                        g_EnemyShots[k].pattern = 12;
                        g_EnemyShots[k].type = ENEMYSHOT_TYPE_BULLET;
                        g_EnemyShots[k].cd = 0;
                        g_EnemyShots[i].cd = HeavyShotCooldown();
                        sfxEnemyShot();
                    }
                }
            }
            else
            {
                if (!g_EnemyShots[i].active && !g_EnemyShots[i].cd)
                {
                    u8 bx = (u8)(g_Enemies[i].x + ENEMY_W / 2 - 1);
                    g_EnemyShots[i].active = 1;
                    g_EnemyShots[i].x = bx;
                    g_EnemyShots[i].y = (u8)(g_Enemies[i].y + ENEMY_H);
                    g_EnemyShots[i].pattern = 12;
                    g_EnemyShots[i].type = ENEMYSHOT_TYPE_BULLET;
                    if (g_Enemies[i].type == ENEMY_TYPE_DIVER)
                    {
                        g_EnemyShots[i].vx = enemyShotAimVXDiver(bx);
                        g_EnemyShots[i].vy = (u8)(ENEMYSHOT_SPEED_Y + 1u);
                        g_EnemyShots[i].cd = DiverShotCooldown();
                    }
                    else
                    {
                        g_EnemyShots[i].vx = enemyShotAimVX(bx);
                        g_EnemyShots[i].vy = ENEMYSHOT_SPEED_Y;
                        g_EnemyShots[i].cd = (u8)(ENEMYSHOT_COOLDOWN + (i << 1));
                    }
                    sfxEnemyShot();
                }
            }
        }

        if (g_Enemies[i].y >= SCREEN_H) { g_Enemies[i].active = 0; continue; }
    }

    // Actualitza dispars enemics
    for (i = 0; i < MAX_ENEMY_SHOTS; i++)
    {
        if (g_EnemyShots[i].cd) g_EnemyShots[i].cd--;
        if (!g_EnemyShots[i].active) continue;
        g_EnemyShots[i].y = (u8)(g_EnemyShots[i].y + g_EnemyShots[i].vy);
        g_EnemyShots[i].x  = (u8)((i8)g_EnemyShots[i].x + g_EnemyShots[i].vx);
        if (g_EnemyShots[i].y >= SCREEN_H ||
            g_EnemyShots[i].x < GAME_X0   ||
            g_EnemyShots[i].x > GAME_X1)
            g_EnemyShots[i].active = 0;
    }
}

//=============================================================================
// SOUND FX - 1:1 CPC (AY-3-8910 / YM2149)
//=============================================================================

#define SFX_NONE        0
#define SFX_EXP_ENEMY   1
#define SFX_EXP_SHIP    2
#define SFX_SHOT        3
#define SFX_BEEP        4
#define SFX_GAMEOVER    5
#define SFX_BOSSWARN    6
#define SFX_LEVELUP     7

static u8 sfxA_kind = SFX_NONE, sfxA_frame = 0;
static u8 sfxB_kind = SFX_NONE, sfxB_frame = 0;
static u8 sfxC_kind = SFX_NONE, sfxC_frame = 0;

static void setToneA(u8 lo, u8 hi, u8 vol) {
    PSG_SetTone(PSG_CHANNEL_A, (u16)(hi * 256u + lo));
    PSG_SetVolume(PSG_CHANNEL_A, vol);
}
static void setToneB(u8 lo, u8 hi, u8 vol) {
    PSG_SetTone(PSG_CHANNEL_B, (u16)(hi * 256u + lo));
    PSG_SetVolume(PSG_CHANNEL_B, vol);
}
static void setToneC(u8 lo, u8 hi, u8 vol) {
    PSG_SetTone(PSG_CHANNEL_C, (u16)(hi * 256u + lo));
    PSG_SetVolume(PSG_CHANNEL_C, vol);
}

static void updateMixer(void) {
    // Enable tone channels based on active effects
    PSG_EnableTone(PSG_CHANNEL_A, sfxA_kind != SFX_NONE);
    PSG_EnableNoise(PSG_CHANNEL_A, FALSE);
    
    if (sfxB_kind == SFX_EXP_ENEMY) {
        PSG_EnableTone(PSG_CHANNEL_B, FALSE);
        PSG_EnableNoise(PSG_CHANNEL_B, TRUE);
    } else if (sfxB_kind == SFX_EXP_SHIP) {
        PSG_EnableTone(PSG_CHANNEL_B, TRUE);
        PSG_EnableNoise(PSG_CHANNEL_B, TRUE);
    } else if (sfxB_kind != SFX_NONE) {
        PSG_EnableTone(PSG_CHANNEL_B, TRUE);
        PSG_EnableNoise(PSG_CHANNEL_B, FALSE);
    } else {
        PSG_EnableTone(PSG_CHANNEL_B, FALSE);
        PSG_EnableNoise(PSG_CHANNEL_B, FALSE);
    }
    
    PSG_EnableTone(PSG_CHANNEL_C, sfxC_kind == SFX_SHOT || sfxC_kind == SFX_GAMEOVER);
    PSG_EnableNoise(PSG_CHANNEL_C, FALSE);
}

void soundStopAll(void) {
    sfxA_kind = SFX_NONE; sfxA_frame = 0;
    sfxB_kind = SFX_NONE; sfxB_frame = 0;
    sfxC_kind = SFX_NONE; sfxC_frame = 0;
    PSG_Mute();
}

void soundInit(void) { soundStopAll(); }
void sfxShot(void) { sfxC_kind = SFX_SHOT; sfxC_frame = 0; setToneC(30, 1, 9); PSG_EnableTone(PSG_CHANNEL_C, TRUE); PSG_EnableNoise(PSG_CHANNEL_C, FALSE); }
void sfxEnemyExplosion(void) { sfxB_kind = SFX_EXP_ENEMY; sfxB_frame = 0; PSG_SetRegister(6, 0x04); PSG_SetVolume(PSG_CHANNEL_B, 13); PSG_EnableTone(PSG_CHANNEL_B, FALSE); PSG_EnableNoise(PSG_CHANNEL_B, TRUE); }
void sfxShipExplosion(void) { sfxB_kind = SFX_EXP_SHIP; sfxB_frame = 0; setToneB(138, 1, 15); PSG_SetRegister(6, 0x04); PSG_EnableTone(PSG_CHANNEL_B, TRUE); PSG_EnableNoise(PSG_CHANNEL_B, TRUE); }
void sfxBeep(void) { sfxA_kind = SFX_BEEP; sfxA_frame = 0; sfxB_kind = SFX_BEEP; sfxB_frame = 0; setToneA(172, 1, 15); setToneB(84, 1, 13); PSG_EnableTone(PSG_CHANNEL_A, TRUE); PSG_EnableNoise(PSG_CHANNEL_A, FALSE); PSG_EnableTone(PSG_CHANNEL_B, TRUE); PSG_EnableNoise(PSG_CHANNEL_B, FALSE); }
void sfxGameOver(void) { sfxA_kind = SFX_GAMEOVER; sfxA_frame = 0; sfxB_kind = SFX_GAMEOVER; sfxB_frame = 0; sfxC_kind = SFX_GAMEOVER; sfxC_frame = 0; setToneA(252, 1, 13); setToneB(172, 1, 13); setToneC(84, 1, 13); PSG_EnableTone(PSG_CHANNEL_A, TRUE); PSG_EnableNoise(PSG_CHANNEL_A, FALSE); PSG_EnableTone(PSG_CHANNEL_B, TRUE); PSG_EnableNoise(PSG_CHANNEL_B, FALSE); PSG_EnableTone(PSG_CHANNEL_C, TRUE); PSG_EnableNoise(PSG_CHANNEL_C, FALSE); }
void sfxLevelUp(void) { sfxA_kind = SFX_LEVELUP; sfxA_frame = 0; setToneA(172, 1, 14); PSG_EnableTone(PSG_CHANNEL_A, TRUE); PSG_EnableNoise(PSG_CHANNEL_A, FALSE); }
void sfxEnemyShot(void) { sfxB_kind = SFX_EXP_ENEMY; sfxB_frame = 3; PSG_SetRegister(6, 0x10); PSG_SetVolume(PSG_CHANNEL_B, 8); PSG_EnableTone(PSG_CHANNEL_B, FALSE); PSG_EnableNoise(PSG_CHANNEL_B, TRUE); }
void sfxMenuSelect(void) { sfxA_kind = SFX_LEVELUP; sfxA_frame = 0; setToneA(172, 1, 15); PSG_EnableTone(PSG_CHANNEL_A, TRUE); PSG_EnableNoise(PSG_CHANNEL_A, FALSE); }

static void tickChannelA(void) {
    if (sfxA_kind == SFX_BEEP) {
        if (sfxA_frame < 2)           setToneA(172, 1, 15);
        else if (sfxA_frame == 2)     PSG_SetVolume(PSG_CHANNEL_A, 0);
        else if (sfxA_frame < 5)      setToneA( 84, 1, 15);
        else { sfxA_kind = SFX_NONE; sfxA_frame = 0; PSG_SetVolume(PSG_CHANNEL_A, 0); return; }
        ++sfxA_frame; return;
    }
    if (sfxA_kind == SFX_LEVELUP) {
        if      (sfxA_frame < 2)  setToneA(172, 1, 14);
        else if (sfxA_frame < 4)  setToneA( 84, 1, 14);
        else if (sfxA_frame < 6)  setToneA( 30, 1, 13);
        else if (sfxA_frame < 8)  setToneA(213, 0, 15);
        else if (sfxA_frame < 11) setToneA(213, 0,  7);
        else { sfxA_kind = SFX_NONE; sfxA_frame = 0; PSG_SetVolume(PSG_CHANNEL_A, 0); return; }
        ++sfxA_frame; return;
    }
    if (sfxA_kind == SFX_GAMEOVER) {
        if (sfxA_frame < 60) { setToneA(252, 1, 13); }
        else { sfxA_kind = SFX_NONE; sfxA_frame = 0; PSG_SetVolume(PSG_CHANNEL_A, 0); return; }
        ++sfxA_frame; return;
    }
    PSG_SetVolume(PSG_CHANNEL_A, 0);
}
static void tickChannelB(void) {
    if (sfxB_kind == SFX_EXP_ENEMY) {
        switch (sfxB_frame) {
            case 0: PSG_SetRegister(6, 0x04); PSG_SetVolume(PSG_CHANNEL_B, 13); break;
            case 1: PSG_SetRegister(6, 0x07); PSG_SetVolume(PSG_CHANNEL_B, 11); break;
            case 2: PSG_SetRegister(6, 0x0B); PSG_SetVolume(PSG_CHANNEL_B,  9); break;
            case 3: PSG_SetRegister(6, 0x10); PSG_SetVolume(PSG_CHANNEL_B,  6); break;
            case 4: PSG_SetRegister(6, 0x16); PSG_SetVolume(PSG_CHANNEL_B,  3); break;
            default: sfxB_kind = SFX_NONE; sfxB_frame = 0; PSG_SetVolume(PSG_CHANNEL_B, 0); return;
        }
        ++sfxB_frame; return;
    }
    if (sfxB_kind == SFX_EXP_SHIP) {
        switch (sfxB_frame) {
            case 0: setToneB(138, 1, 15); PSG_SetRegister(6, 0x04); break;
            case 1: setToneB( 66, 1, 14); PSG_SetRegister(6, 0x06); break;
            case 2: setToneB( 13, 1, 13); PSG_SetRegister(6, 0x08); break;
            case 3: setToneB(215, 0, 10); PSG_SetRegister(6, 0x0C); break;
            case 4: setToneB(161, 0,  8); PSG_SetRegister(6, 0x13); break;
            case 5: setToneB(135, 0,  6); PSG_SetRegister(6, 0x16); break;
            case 6: setToneB( 90, 0,  4); PSG_SetRegister(6, 0x1B); break;
            default: sfxB_kind = SFX_NONE; sfxB_frame = 0; PSG_SetVolume(PSG_CHANNEL_B, 0); return;
        }
        ++sfxB_frame; return;
    }
    if (sfxB_kind == SFX_BEEP) {
        if (sfxB_frame < 2)           setToneB( 84, 1, 13);
        else if (sfxB_frame == 2)     PSG_SetVolume(PSG_CHANNEL_B, 0);
        else if (sfxB_frame < 5)      setToneB( 29, 1, 13);
        else { sfxB_kind = SFX_NONE; sfxB_frame = 0; PSG_SetVolume(PSG_CHANNEL_B, 0); return; }
        ++sfxB_frame; return;
    }
    if (sfxB_kind == SFX_GAMEOVER) {
        if (sfxB_frame < 60) { setToneB(172, 1, 13); }
        else { sfxB_kind = SFX_NONE; sfxB_frame = 0; PSG_SetVolume(PSG_CHANNEL_B, 0); return; }
        ++sfxB_frame; return;
    }
    PSG_SetVolume(PSG_CHANNEL_B, 0);
}
static void tickChannelC(void) {
    if (sfxC_kind == SFX_SHOT) {
        if (sfxC_frame == 0)          setToneC( 30, 1, 9);
        else if (sfxC_frame == 1)     setToneC(200, 0, 4);
        else { sfxC_kind = SFX_NONE; sfxC_frame = 0; PSG_SetVolume(PSG_CHANNEL_C, 0); return; }
        ++sfxC_frame; return;
    }
    if (sfxC_kind == SFX_GAMEOVER) {
        if (sfxC_frame < 60) { setToneC(84, 1, 13); }
        else { sfxC_kind = SFX_NONE; sfxC_frame = 0; PSG_SetVolume(PSG_CHANNEL_C, 0); return; }
        ++sfxC_frame; return;
    }
    PSG_SetVolume(PSG_CHANNEL_C, 0);
}
void soundTick(void) {
    tickChannelA();
    tickChannelB();
    tickChannelC();
    updateMixer();
}

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
        maxf = (g_Explosions[i].kind == EXP_KIND_SHIP || g_Explosions[i].kind == EXP_KIND_BOSS) ? 6 : 4;
        g_Explosions[i].frame++;
        if (g_Explosions[i].frame >= maxf)
            g_Explosions[i].active = 0;
    }
}

void RespawnShip()
{
    u8 i;
    // 1:1 CPC: resetShipRuntimeState() + placeShipAtSpawn() + clearActiveCombatState
    g_ShipExploding  = 0;
    g_ShipExplTimer  = 0;
    g_ShipInvul      = 0;
    g_ShipInvulTimer = 0;
    g_FireCooldown   = 0;
    g_ShipThrust     = 0;
    g_ShipThrustLevel = 0;
    g_ShipThrustFrame = 0;
    g_ShipX          = SHIP_SPAWN_X;
    g_ShipY          = (u8)(SHIP_SPAWN_Y);
    // CPC: clearActiveCombatState - ara aqui per no netejar pantalla durant l'explosio
    for (i = 0; i < MAX_ENEMIES;     i++) g_Enemies[i].active    = 0;
    for (i = 0; i < MAX_SHOTS;       i++) g_Shots[i].active      = 0;
    for (i = 0; i < MAX_ENEMY_SHOTS; i++) g_EnemyShots[i].active = 0;
    g_WaveActive = 0;
}

void UpdateShipExplosionState()
{
    // 1:1 CPC: updateShipExplosionState
    if (!g_ShipExploding) return;
    if (g_ShipExplTimer > 0) { g_ShipExplTimer--; return; }
    // CPC: game over check
    if (g_ShipLastLife) { g_ShipExploding = 0; return; } // game over, no respawn
    // CPC: resetShipRuntimeState() + placeShipAtSpawn() + setShipRespawnInvul()
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

void UpdateShipThrustAnim()
{
    if (g_ShipThrust)
    {
        if (g_ShipThrustLevel < 5) g_ShipThrustLevel++;
        g_ShipThrustFrame++;
    }
    else
    {
        if (g_ShipThrustLevel) g_ShipThrustLevel--;
        if (g_ShipThrustLevel) g_ShipThrustFrame++;
        else g_ShipThrustFrame = 0;
    }
}

void TriggerShipHit()
{
    // 1:1 CPC: triggerShipHit
    SpawnExplosion(g_ShipX, g_ShipY, EXP_KIND_SHIP);
    sfxShipExplosion();
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
    g_ShipThrust    = 0;
    g_ShipThrustLevel = 0;
    g_ShipThrustFrame = 0;
    // NOTA: No netegem enemics aqui - ho fa RespawnShip per veure l'explosio
}

#define OVERLAP(x1,y1,w1,h1,x2,y2,w2,h2) \
    ((x1)+(w1)>(x2) && (x2)+(w2)>(x1) && (y1)+(h1)>(y2) && (y2)+(h2)>(y1))

void CheckCollisions()
{
    u8 i, j;
    u8 enemy_w, enemy_h;

    // Shots vs enemies
    for (i = 0; i < MAX_SHOTS; i++)
    {
        if (!g_Shots[i].active) continue;
        for (j = 0; j < MAX_ENEMIES; j++)
        {
            if (!g_Enemies[j].active) continue;
            
            // Determine enemy hitbox size
            enemy_w = (g_Enemies[j].type == ENEMY_TYPE_BOSS) ? ENEMY_BOSS_W : ENEMY_W;
            enemy_h = (g_Enemies[j].type == ENEMY_TYPE_BOSS) ? ENEMY_BOSS_H : ENEMY_H;
            
            if (OVERLAP(g_Shots[i].x, g_Shots[i].y, 4, 8,
                        g_Enemies[j].x, g_Enemies[j].y, enemy_w, enemy_h))
            {
                g_Shots[i].active   = 0;
                g_Enemies[j].health--;
                if (g_Enemies[j].health == 0)
                {
                    g_Enemies[j].active = 0;
                    g_WaveKilled++;  // 1:1 CPC: ++wave_killed
                    SpawnExplosion(g_Enemies[j].x + enemy_w/2 - 8,
                                   g_Enemies[j].y + enemy_h/2 - 8,
                                   (g_Enemies[j].type == ENEMY_TYPE_BOSS) ? EXP_KIND_BOSS : EXP_KIND_ENEMY);
                    sfxEnemyExplosion();
                    // Boss scores differently
                    if (g_Enemies[j].type == ENEMY_TYPE_BOSS)
                        AddScore(ENEMY_SCORE_BOSS);
                    else
                        AddScore(g_EnemyScore[g_Enemies[j].type]);
                }
                else
                {
                    // Enemy with remaining health hit (heavy/bomber/boss)
                    SpawnExplosion(g_Enemies[j].x + enemy_w/2 - 8,
                                   g_Enemies[j].y + enemy_h/2 - 8,
                                   EXP_KIND_ENEMY);
                    sfxEnemyExplosion();  // So d'impacte al boss/heavy/bomber
                    g_HudDirty = 1;  // Update HUD (boss HP bar etc.)
                }
            }
        }
    }

    if (g_ShipInvul || g_ShipExploding || g_ShipLastLife) return;

    // Nau vs enemics
    for (j = 0; j < MAX_ENEMIES; j++)
    {
        if (!g_Enemies[j].active) continue;
        
        // Determine enemy hitbox size
        enemy_w = (g_Enemies[j].type == ENEMY_TYPE_BOSS) ? ENEMY_BOSS_W : ENEMY_W;
        enemy_h = (g_Enemies[j].type == ENEMY_TYPE_BOSS) ? ENEMY_BOSS_H : ENEMY_H;
        
        if (OVERLAP(g_ShipX, g_ShipY, SHIP_W, SHIP_H,
                    g_Enemies[j].x, g_Enemies[j].y, enemy_w, enemy_h))
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
        if (OVERLAP((u8)(g_ShipX + SHIP_HIT_OX), (u8)(g_ShipY + SHIP_HIT_OY), SHIP_HIT_W, SHIP_HIT_H,
                    g_EnemyShots[j].x, g_EnemyShots[j].y, ENEMYSHOT_W, ENEMYSHOT_H))
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

void DrawHiScoreTable(u8 inputPos)
{
    u8 i;
    char buf[17];
    for (i = 0; i < HISCORE_COUNT; i++)
    {
        u16 s = g_HiScores[i].score;
        u8 lvl = g_HiScores[i].level;
        // Formatem: "1 XXXXX LVxx AAA" - 1:1 CPC hs_formatRow
        buf[0] = (char)('1' + i);
        buf[1] = ' ';
        {
            u16 ss = (i == inputPos) ? g_LastScore : s;
            buf[6] = (char)('0' + ss % 10); ss /= 10;
            buf[5] = (char)('0' + ss % 10); ss /= 10;
            buf[4] = (char)('0' + ss % 10); ss /= 10;
            buf[3] = (char)('0' + ss % 10); ss /= 10;
            buf[2] = (char)('0' + ss % 10);
        }
        buf[7]  = ' ';
        buf[8]  = 'L'; buf[9]  = 'V';
        buf[10] = (char)('0' + ((i == inputPos) ? g_LastLevel : lvl) / 10);
        buf[11] = (char)('0' + ((i == inputPos) ? g_LastLevel : lvl) % 10);
        buf[13] = ' ';
        if (i == inputPos)
        {
            buf[13] = (char)g_HsInputChar[0];
            buf[14] = (char)g_HsInputChar[1];
            buf[15] = (char)g_HsInputChar[2];
        }
        else
        {
            buf[13] = (char)g_HiScores[i].name[0];
            buf[14] = (char)g_HiScores[i].name[1];
            buf[15] = (char)g_HiScores[i].name[2];
        }
        buf[16] = 0;
        // 1:1 CPC: rank 1 en "hi" (blanc), resta en normal (blau)
        u8 color = (i == 0) ? HUD_FONT_COLOR_HI : HUD_FONT_COLOR_NRM;
        HudDrawText(8, (u8)(10 + i * 2), buf, color);
        // Si es la fila d'input, pinta el cursor en blanc sobre el color base
        if (i == inputPos)
        {
            char c[2];
            c[0] = (char)g_HsInputChar[g_HsInputPos]; c[1] = 0;
            HudDrawText((u8)(8 + 13 + g_HsInputPos), (u8)(10 + i * 2), c, HUD_FONT_COLOR_HI);
        }
    }
}

// Redraw only the hi-score input row (fast update without full redraw)
void RedrawHsInput()
{
    // Redibuixa la fila sencera on s'estan editant les inicials
    char buf[17];
    u8 i;
    for (i = 0; i < HISCORE_COUNT; i++)
    {
        if (i == g_HsPos)
        {
            u16 ss = g_LastScore;
            buf[0] = (char)('1' + i);
            buf[1] = ' ';
            buf[6] = (char)('0' + ss % 10); ss /= 10;
            buf[5] = (char)('0' + ss % 10); ss /= 10;
            buf[4] = (char)('0' + ss % 10); ss /= 10;
            buf[3] = (char)('0' + ss % 10); ss /= 10;
            buf[2] = (char)('0' + ss % 10);
            buf[7]  = ' ';
            buf[8]  = 'L'; buf[9]  = 'V';
            buf[10] = (char)('0' + g_LastLevel / 10);
            buf[11] = (char)('0' + g_LastLevel % 10);
            buf[13] = ' ';
            buf[13] = (char)g_HsInputChar[0];
            buf[14] = (char)g_HsInputChar[1];
            buf[15] = (char)g_HsInputChar[2];
            buf[16] = 0;
            u8 color = (i == 0) ? HUD_FONT_COLOR_HI : HUD_FONT_COLOR_NRM;
            HudDrawText(8, (u8)(10 + i * 2), buf, color);
            // Cursor
            char c[2];
            c[0] = (char)g_HsInputChar[g_HsInputPos]; c[1] = 0;
            HudDrawText((u8)(8 + 13 + g_HsInputPos), (u8)(10 + i * 2), c, HUD_FONT_COLOR_HI);
        }
    }
}

// Menu starfield - safe zones per pantalla
// MENU: logo (8-23,0-4) + text (11-21,6-15) → lliure: 3-7,24-28 + vores
// TOP3: taula (8-23,10-16) → lliure: 3-7,24-28 + 3-28,0-7 + vores
// HELP: text (3-28,5-16) → lliure: només vores 1-2,29-30
#define MENU_TWINKLE_FRAMES 4

// Estrelles comunes (vores estretes, totes les pantalles)
#define MENU_STAR_COMMON_N  30
static const u8 g_MenuStarCX[MENU_STAR_COMMON_N] = {
    0x01,0x02,0x01,0x02,0x01,0x02,0x01,0x02,0x01,0x02,
    0x01,0x02,0x01,0x02,0x01,0x02,0x01,0x02,0x01,0x02,
    0x1D,0x1E,0x1D,0x1E,0x1D,0x1E,0x1D,0x1E,0x1D,0x1E,
};
static const u8 g_MenuStarCY[MENU_STAR_COMMON_N] = {
    0x00,0x01,0x03,0x04,0x06,0x07,0x09,0x0A,0x0C,0x0D,
    0x0F,0x10,0x12,0x13,0x15,0x16,0x18,0x19,0x21,0x22,
    0x00,0x01,0x03,0x04,0x06,0x07,0x09,0x0A,0x0C,0x0D,
};
static const u8 g_MenuStarCS[MENU_STAR_COMMON_N] = {
    0,2,1,3,1,0,3,2,0,1,3,2,2,0,3,1,1,2,0,3,
    2,0,3,1,3,2,1,0,2,3,
};

// Estrelles extra MENU (zones laterals logo + zona central inferior)
#define MENU_STAR_MENU_N    32
static const u8 g_MenuStarMX[MENU_STAR_MENU_N] = {
    // Zones laterals logo (files 0-3) - 20 estrelles
    0x03,0x04,0x05,0x06,0x07,0x03,0x05,0x07,0x04,0x06,
    0x18,0x19,0x1A,0x1B,0x1C,0x18,0x1A,0x1C,0x19,0x1B,
    // Zona central inferior (files 17-19, cols 10-25) - 12 estrelles
    0x0A,0x0D,0x10,0x13,0x16,0x19,0x0B,0x0E,0x11,0x14,
    0x17,0x1A,
};
static const u8 g_MenuStarMY[MENU_STAR_MENU_N] = {
    // Zones laterals logo
    0x00,0x01,0x00,0x01,0x00,0x02,0x02,0x02,0x03,0x03,
    0x00,0x01,0x00,0x01,0x00,0x02,0x02,0x02,0x03,0x03,
    // Zona central inferior (rows 17 & 19 decimal = 0x11 & 0x13)
    0x11,0x11,0x11,0x11,0x11,0x11,0x13,0x13,0x13,0x13,
    0x13,0x13,
};
static const u8 g_MenuStarMS[MENU_STAR_MENU_N] = {
    1,3,0,2,1,3,0,2,1,3,0,2,1,3,0,2,1,3,0,2,
    0,1,2,3,0,1,2,3,0,1,2,3,
};

// Estrelles extra TOP3 (zona superior + laterals)
#define MENU_STAR_TOP3_N    24
static const u8 g_MenuStarTX[MENU_STAR_TOP3_N] = {
    0x04,0x06,0x08,0x0A,0x0C,0x0E,0x10,0x12,0x14,0x16,0x18,0x1A,
    0x04,0x06,0x08,0x0A,0x0C,0x0E,0x10,0x12,0x14,0x16,0x18,0x1A,
};
static const u8 g_MenuStarTY[MENU_STAR_TOP3_N] = {
    0x00,0x00,0x01,0x01,0x02,0x02,0x03,0x03,0x04,0x04,0x05,0x05,
    0x06,0x06,0x07,0x07,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
};
static const u8 g_MenuStarTS[MENU_STAR_TOP3_N] = {
    0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3,
};

static u8 g_MenuStarState[MENU_STAR_COMMON_N];
static u8 g_MenuStarMState[MENU_STAR_MENU_N];
static u8 g_MenuStarTState[MENU_STAR_TOP3_N];
static u8 g_MenuTwinkleTick = 0;
static u8 g_MenuTwinkleRng  = 0x5A;

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
    for (i = 0; i < MENU_STAR_COMMON_N; i++) g_MenuStarState[i] = g_MenuStarCS[i];
    for (i = 0; i < MENU_STAR_MENU_N; i++) g_MenuStarMState[i] = g_MenuStarMS[i];
    for (i = 0; i < MENU_STAR_TOP3_N; i++) g_MenuStarTState[i] = g_MenuStarTS[i];
    g_MenuTwinkleTick = 0;
    g_MenuTwinkleRng  = 0x5A;
}

static void DrawMenuStarCommon(u8 idx)
{
    u8 tile = (g_MenuStarState[idx] == 0) ? 0 : (u8)(MENU_STAR_TILE_BASE + g_MenuStarState[idx] - 1);
    VDP_Poke_GM2(g_MenuStarCX[idx], g_MenuStarCY[idx], tile);
}
static void DrawMenuStarMenu(u8 idx)
{
    u8 tile = (g_MenuStarMState[idx] == 0) ? 0 : (u8)(MENU_STAR_TILE_BASE + g_MenuStarMState[idx] - 1);
    VDP_Poke_GM2(g_MenuStarMX[idx], g_MenuStarMY[idx], tile);
}
static void DrawMenuStarTop3(u8 idx)
{
    u8 tile = (g_MenuStarTState[idx] == 0) ? 0 : (u8)(MENU_STAR_TILE_BASE + g_MenuStarTState[idx] - 1);
    VDP_Poke_GM2(g_MenuStarTX[idx], g_MenuStarTY[idx], tile);
}

static void DrawAllStarsForMode(u8 mode)
{
    u8 i;
    // Clear all extra star positions first
    for (i = 0; i < MENU_STAR_MENU_N; i++)
        VDP_Poke_GM2(g_MenuStarMX[i], g_MenuStarMY[i], 0);
    for (i = 0; i < MENU_STAR_TOP3_N; i++)
        VDP_Poke_GM2(g_MenuStarTX[i], g_MenuStarTY[i], 0);
    // Draw common stars
    for (i = 0; i < MENU_STAR_COMMON_N; i++) DrawMenuStarCommon(i);
    // Draw mode-specific extra stars
    if (mode == TS_MENU)
        for (i = 0; i < MENU_STAR_MENU_N; i++) DrawMenuStarMenu(i);
    else if (mode == TS_ATTRACT_SCORE)
        for (i = 0; i < MENU_STAR_TOP3_N; i++) DrawMenuStarTop3(i);
}

void TickMenuStars()
{
    u8 i, j, src;

    g_MenuTwinkleTick++;
    if (g_MenuTwinkleTick < MENU_TWINKLE_FRAMES) return;
    g_MenuTwinkleTick = 0;

    // Twinkle common stars
    g_MenuTwinkleRng = (u8)(g_MenuTwinkleRng * 17u + 29u);
    i = g_MenuTwinkleRng;
    while (i >= MENU_STAR_COMMON_N) i = (u8)(i - MENU_STAR_COMMON_N);
    g_MenuStarState[i] = (u8)((g_MenuStarState[i] + 1u) & 3u);
    DrawMenuStarCommon(i);

    g_MenuTwinkleRng = (u8)(g_MenuTwinkleRng * 17u + 29u);
    j = g_MenuTwinkleRng;
    while (j >= MENU_STAR_COMMON_N) j = (u8)(j - MENU_STAR_COMMON_N);
    if (j == i) { j = (u8)(i + 7u); while (j >= MENU_STAR_COMMON_N) j = (u8)(j - MENU_STAR_COMMON_N); }
    g_MenuStarState[j] = (u8)((g_MenuStarState[j] + 1u) & 3u);
    DrawMenuStarCommon(j);

    // Twinkle extra stars based on mode
    if (g_TitleMode == TS_MENU)
    {
        src = g_MenuTwinkleRng;
        i = src; while (i >= MENU_STAR_MENU_N) i = (u8)(i - MENU_STAR_MENU_N);
        g_MenuStarMState[i] = (u8)((g_MenuStarMState[i] + 1u) & 3u);
        DrawMenuStarMenu(i);
    }
    else if (g_TitleMode == TS_ATTRACT_SCORE)
    {
        src = (u8)(g_MenuTwinkleRng * 13u + 7u);
        i = src; while (i >= MENU_STAR_TOP3_N) i = (u8)(i - MENU_STAR_TOP3_N);
        g_MenuStarTState[i] = (u8)((g_MenuStarTState[i] + 1u) & 3u);
        DrawMenuStarTop3(i);
    }
}

// Logo tiles - 80 tiles 16x5 (128x40px)
#define LOGO_TILE_BASE  1
#define LOGO_TILES_X    16
#define LOGO_TILES_Y    5
#define LOGO_COL_START  8

static const u8 g_LogoTiles[80*8] = {
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x00,0x00,0x00,0x00,0x10,0x30,0x10,0x00,
    0x00,0x00,0x00,0x00,0x00,0x1F,0x10,0x28,
    0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0xF8,0x1C,0x14,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x00,0x00,0x00,0x00,0x00,0x3F,0x30,0x30,
    0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0xFE,0x03,0x05,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x01,
    0x00,0x00,0x00,0x00,0x00,0xFF,0x80,0x80,
    0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x01,
    0x00,0x00,0x00,0x00,0x00,0x80,0xC0,0x40,
    0x00,0x00,0x00,0x00,0x10,0x30,0x00,0x10,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x02,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x08,0x40,0x00,0x00,0x00,0x00,0x00,0x00,
    0xFF,0xFF,0x00,0x80,0x00,0xFF,0x81,0x81,
    0x00,0x00,0x01,0x01,0x01,0x81,0x81,0xFE,
    0x00,0x00,0x80,0x80,0x80,0x80,0x80,0x80,
    0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xD0,0xC0,
    0x00,0x00,0x00,0x3F,0x3F,0x20,0x2F,0x2F,
    0x00,0x00,0x80,0x80,0xC0,0x20,0xA0,0xA0,
    0xC0,0x40,0xE0,0xA0,0xA0,0xA0,0xA0,0xA0,
    0x06,0x0D,0x0E,0x0F,0x0F,0x0F,0x0F,0x0F,
    0x00,0x80,0x07,0x07,0x0F,0x08,0x0B,0x0B,
    0x00,0x00,0xE0,0xE0,0xF0,0x08,0xE8,0xE8,
    0xF0,0xD0,0xE8,0xE8,0xE8,0xE8,0xE8,0xE8,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x40,
    0x02,0x0D,0x0D,0x27,0x01,0x02,0x02,0x02,
    0x00,0x00,0x00,0xE0,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x60,0x30,0xD7,0xF0,0xEF,
    0x00,0x00,0x00,0x00,0x00,0xFE,0x00,0xFF,
    0x00,0x03,0x0B,0x03,0x00,0xF8,0x74,0x3E,
    0x00,0x00,0x00,0x00,0x00,0x80,0x80,0x80,
    0x38,0x38,0x38,0x38,0x38,0x3A,0x38,0x18,
    0xD0,0xD0,0xD0,0xD0,0xD0,0x50,0x00,0x10,
    0x50,0x50,0x50,0x50,0x71,0x70,0x70,0x30,
    0xA0,0xA0,0xA0,0xA0,0xA0,0x20,0x00,0x00,
    0x0E,0x06,0x06,0x06,0x0E,0x0E,0x0E,0x0E,
    0x14,0x14,0x14,0x1C,0x34,0x14,0x14,0x10,
    0x14,0x14,0x14,0x1C,0x1C,0x1D,0x1C,0x1C,
    0x38,0x38,0x38,0x38,0x38,0xA8,0x00,0x00,
    0x00,0x00,0x00,0x03,0x00,0x00,0x00,0x00,
    0x40,0xB0,0x90,0x60,0x00,0x50,0x40,0x40,
    0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x0F,0x7F,0x01,0x82,0x80,0xA1,0x00,0x60,
    0x00,0x81,0x81,0x81,0x7F,0x80,0x81,0xFF,
    0xFF,0x83,0x83,0x81,0x01,0x01,0x03,0x06,
    0x7F,0xFF,0x40,0x00,0x20,0x20,0x20,0x20,
    0xEF,0xE0,0x40,0x40,0x40,0x40,0xE0,0x40,
    0x40,0xC0,0xC0,0xD0,0xDF,0xC0,0xC0,0xFF,
    0xCF,0x80,0x80,0x20,0x00,0x00,0x80,0x01,
    0x00,0x00,0x00,0x8F,0x90,0x97,0x97,0x17,
    0xF9,0xF8,0x10,0x10,0x30,0x30,0x34,0x37,
    0xEF,0x0F,0x0F,0x08,0x00,0x00,0x07,0x00,
    0xE3,0xE0,0xE0,0x00,0x00,0x08,0xE0,0x00,
    0xFF,0x3F,0x14,0x18,0x18,0x18,0xB2,0x6A,
    0x00,0x00,0x00,0xF8,0x10,0x30,0xF0,0xF0,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0x8F,0x2F,0x00,0x10,0x0F,0x00,0x00,0x00,
    0xFF,0x0F,0x00,0x00,0xFF,0x00,0x00,0x00,
    0xE1,0xF0,0x04,0x08,0xF0,0x00,0x00,0x00,
    0x5F,0x5F,0x40,0x60,0x1F,0x00,0x00,0x00,
    0xB7,0xB6,0xA0,0xA0,0x1F,0x00,0x00,0x00,
    0xFF,0xDF,0x00,0x00,0xFF,0x00,0x00,0x00,
    0xFC,0x9C,0x00,0x02,0xFC,0x00,0x00,0x00,
    0x17,0x17,0x10,0x18,0x0F,0x00,0x00,0x00,
    0xE4,0xE0,0x21,0x21,0xC0,0x00,0x00,0x00,
    0xFF,0x79,0x00,0x80,0xFF,0x00,0x00,0x00,
    0xFF,0xFF,0x00,0x00,0xFF,0x00,0x00,0x00,
    0x15,0x05,0x44,0x86,0x03,0x00,0x00,0x00,
    0xF0,0xF0,0x08,0x08,0xF0,0x00,0x00,0x00,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
};

static const u8 g_LogoColors[80] = {
    0x00,0x50,0x50,0x50,0x50,0x00,0x50,0x50,0x50,0x00,0x50,0x50,0x50,0x50,0x50,0x00,
    0x50,0x00,0x45,0x50,0x05,0x50,0x05,0x05,0x05,0x50,0x50,0x05,0x05,0x50,0x00,0x50,
    0x50,0x40,0x05,0x05,0x05,0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x40,0x50,
    0x50,0x00,0x05,0x50,0x05,0x05,0x05,0x50,0x05,0x50,0x05,0x05,0x05,0x05,0x50,0x00,
    0x00,0x00,0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x50,0x00,
};
void InitLogoTiles()
{
    u8 t, bank, r;
    for (t = 0; t < (LOGO_TILES_X * LOGO_TILES_Y); t++)
    {
        const u8* tile = g_LogoTiles + t * 8;
        u8 color[8];
        for (r = 0; r < 8; r++) color[r] = g_LogoColors[t];
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
    u8 base = (colorByte == HUD_FONT_COLOR_HI) ? HUD_FONT_TILE_HI : HUD_FONT_TILE_BASE;
    u8 tiles[32];
    u8 len = 0;
    const char* p = s;
    while (*p && len < 32 && (u8)(col + len) < 32)
    {
        tiles[len] = (u8)(base + HudCharToIdx(*p));
        len++;
        p++;
    }
    if (len)
        VDP_WriteVRAM(tiles, (u16)(g_ScreenLayoutLow + (u16)row * 32u + col), g_ScreenLayoutHigh, len);
    while (*p)
    {
        col = 0;
        row++;
        len = 0;
        while (*p && len < 32)
        {
            tiles[len] = (u8)(base + HudCharToIdx(*p));
            len++;
            p++;
        }
        if (len)
            VDP_WriteVRAM(tiles, (u16)(g_ScreenLayoutLow + (u16)row * 32u), g_ScreenLayoutHigh, len);
    }
}

// Linia horitzontal amb color (simula cpct_drawSolidBox d'1 pixel alt del CPC)
void HudDrawHLine(u8 col, u8 row, u8 len, u8 colorByte)
{
    static const u8 hline_tile[8] = {0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00};
    u8 hline_color[8];
    u8 bank, i, r;
    for (r = 0; r < 8; r++) hline_color[r] = colorByte;
    for (bank = 0; bank < 3; bank++)
    {
        VDP_LoadBankPattern_GM2(hline_tile,  1, bank, HLINE_TILE);
        VDP_LoadBankColor_GM2(hline_color,   1, bank, HLINE_TILE);
    }
    for (i = 0; i < len; i++)
        VDP_Poke_GM2((u8)(col+i), row, HLINE_TILE);
}

void UpdateMenuInput()
{
    u8 row8 = Keyboard_Read(8);
    u8 row5 = Keyboard_Read(5);
    u8 row0 = Keyboard_Read(0); // tecles 1, 2, 3
    u8 joy = ~Joystick_Read(JOY_PORT_1) & 0x0F;
    u8 joy_fire = (Joystick_Read(JOY_PORT_1) & JOY_INPUT_TRIGGER_A) == 0;
    u8 kbd_fire = IS_KEY_PRESSED(row8, KEY_SPACE) || IS_KEY_PRESSED(row5, KEY_Z);
    u8 fire  = g_ControlMode ? kbd_fire : joy_fire;  // Only check input matching selected mode

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

        // Control mode selection: 1=joystick, 2=keyboard, 3=set keys
        if (IS_KEY_PRESSED(row0, KEY_1) || (joy & JOY_INPUT_DIR_LEFT))
        {
            g_ControlMode = 0;
            g_TitleDirty = 1; g_TitlePhase = 0;
        }
        else if (IS_KEY_PRESSED(row0, KEY_2) || (joy & JOY_INPUT_DIR_RIGHT))
        {
            g_ControlMode = 1;
            g_TitleDirty = 1; g_TitlePhase = 0;
        }
        else if (IS_KEY_PRESSED(row0, KEY_3) || (joy & JOY_INPUT_DIR_DOWN))
        {
            g_TitleMode = TS_REDEFINE; g_RedefineStep = 0;
            g_TitleDirty = 1; g_TitlePhase = 0;
        }
        else if (fire)
        {
            InitGamePlay();
            g_GameState  = GS_PLAYING;
            sfxMenuSelect();
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
             // Limpia sprites al volver al menú
             u8 s;
             for (s = 0; s < 32; s++)
                 VDP_SetSpritePositionY(s, VDP_SPRITE_DISABLE_SM1);
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

        // Phase 0 = entering (full draw queued by EnterPostGame)
        // Phase 1 = full draw done, wait for fire release
        // Phase 2 = fire released, allow input
        if (g_TitlePhase == 1)
        {
            if (!fire) g_TitlePhase = 2;
            return;
        }
        if (g_TitlePhase < 2) return;

        if (key_cd > 0) { key_cd--; return; }

        if (IS_KEY_PRESSED(row8, KEY_UP))
        {
            g_HsInputChar[g_HsInputPos] = (g_HsInputChar[g_HsInputPos] >= 'Z') ? 'A' :
                                           (u8)(g_HsInputChar[g_HsInputPos] + 1);
            g_TitleDirty = 1; key_cd = 8;
        }
        else if (IS_KEY_PRESSED(row8, KEY_DOWN))
        {
            g_HsInputChar[g_HsInputPos] = (g_HsInputChar[g_HsInputPos] <= 'A') ? 'Z' :
                                           (u8)(g_HsInputChar[g_HsInputPos] - 1);
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
        u8 detected = 0;
        u8 fr;
        // Use g_TitlePhase to track redefinition state (reset to 0 when entering)
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

        if (g_TitlePhase == 0)
        {
            if (!detected) g_TitlePhase = 1; // espera que no hi hagi cap tecla
        }
        else if (g_TitlePhase == 1 && detected)
        {
            // Assigna la tecla al pas actual
            switch (g_RedefineStep)
            {
                case 0: g_KeyLeft  = g_LastKeyPressed; break;
                case 1: g_KeyRight = g_LastKeyPressed; break;
                case 2: g_KeyUp    = g_LastKeyPressed; break;
                case 3: g_KeyDown  = g_LastKeyPressed; break;
                case 4: g_KeyFire  = g_LastKeyPressed; break;
                case 5: g_KeyPause = g_LastKeyPressed; break;
                case 6: g_KeyQuit  = g_LastKeyPressed; break;
                default: break;
            }
            g_RedefineStep++;
            g_TitlePhase = 0;
            if (g_RedefineStep >= 7)
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
    u8 i;
    VDP_FillScreen_GM2(0);
    InitLogoTiles();
    InitHudFontTiles();

    // 1:1 CPC: estrelles centrals NOMÉS per TOP3 (zona superior buida sense logo)
    // HISCORE_INPUT i HISCORE_VIEW: no estrelles (corrompen text i sobreescriuen font A,B,C)
    if (g_TitleMode != TS_HISCORE_INPUT && g_TitleMode != TS_HISCORE_VIEW)
    {
        InitMenuStarTiles();
        InitMenuStars();
        DrawAllStarsForMode(g_TitleMode);
    }
    else
    {
        // No stars during hi-score input (they corrupt text)
    }

    if (g_TitleMode == TS_MENU)
    {
        DrawAllStarsForMode(TS_MENU);
        DrawLogo();
        HudDrawHLine(3, 4, 26, HUD_FONT_COLOR_CYN);
        HudDrawText(11, 6, "1 JOYSTICK", g_ControlMode == 0 ? HUD_FONT_COLOR_HI : HUD_FONT_COLOR_NRM);
        HudDrawText(11, 8, "2 KEYBOARD", g_ControlMode == 1 ? HUD_FONT_COLOR_HI : HUD_FONT_COLOR_NRM);
        HudDrawText(11,10, "3 SET KEYS", HUD_FONT_COLOR_NRM);
        HudDrawHLine(3, 13, 26, HUD_FONT_COLOR_CYN);
        HudDrawText(9, 15, "FIRE TO START", HUD_FONT_COLOR_HI);
        HudDrawText(0, 23, "GAME BY XEVIMET4L", HUD_FONT_COLOR_NRM);
        HudDrawText(28, 23, "2026", HUD_FONT_COLOR_NRM);
    }
    else if (g_TitleMode == TS_ATTRACT_SCORE)
    {
        DrawAllStarsForMode(TS_ATTRACT_SCORE);
        HudDrawText(13, 8, "TOP  3", HUD_FONT_COLOR_HI);
        HudDrawHLine(3,  9, 26, HUD_FONT_COLOR_CYN);
        DrawHiScoreTable(0xFF);
        HudDrawHLine(3, 16, 26, HUD_FONT_COLOR_CYN);
        HudDrawText(0, 23, "GAME BY XEVIMET4L", HUD_FONT_COLOR_NRM);
        HudDrawText(28, 23, "2026", HUD_FONT_COLOR_NRM);
    }
    else if (g_TitleMode == TS_HISCORE_VIEW)
    {
        HudDrawText(11, 6, "GAME OVER", HUD_FONT_COLOR_HI);
        HudDrawText(13, 8, "TOP 3",     HUD_FONT_COLOR_HI);
        HudDrawHLine(3,  9, 26, HUD_FONT_COLOR_CYN);
        DrawHiScoreTable(0xFF);
        HudDrawHLine(3, 16, 26, HUD_FONT_COLOR_CYN);
        HudDrawText(9, 18, "PRESS ANY KEY", HUD_FONT_COLOR_HI);
        HudDrawText(0, 23, "GAME BY XEVIMET4L", HUD_FONT_COLOR_NRM);
        HudDrawText(28, 23, "2026", HUD_FONT_COLOR_NRM);
    }
    else if (g_TitleMode == TS_HELP)
    {
        HudDrawText(10, 3, "HOW TO PLAY", HUD_FONT_COLOR_HI);
        HudDrawHLine(3,  4, 26, HUD_FONT_COLOR_CYN);
        HudDrawText(5, 5,  "JOY OR ARROWS TO MOVE",    HUD_FONT_COLOR_NRM);
        HudDrawText(6, 7,  "Z OR FIRE 1 TO SHOOT",     HUD_FONT_COLOR_NRM);
        HudDrawText(11, 9,  "P TO PAUSE",               HUD_FONT_COLOR_NRM);
        HudDrawText(6,11,  "Q OR RETURN TO QUIT",       HUD_FONT_COLOR_NRM);
        HudDrawText(3,13,  "EXTRA LIFE EVERY 5000 PTS", HUD_FONT_COLOR_NRM);
        HudDrawText(4,15,  "COMPLETE WAVE FOR BONUS",   HUD_FONT_COLOR_NRM);
        HudDrawHLine(3, 17, 26, HUD_FONT_COLOR_CYN);
        HudDrawText(10, 19, "FIRE TO MENU", HUD_FONT_COLOR_HI);
        HudDrawText(0, 23, "GAME BY XEVIMET4L", HUD_FONT_COLOR_NRM);
        HudDrawText(28, 23, "2026", HUD_FONT_COLOR_NRM);
    }
    else if (g_TitleMode == TS_WIN)
    {
        DrawLogo();
        HudDrawHLine(3,  4, 26, HUD_FONT_COLOR_NRM);
        HudDrawText(8,  6, "CONGRATULATIONS",       HUD_FONT_COLOR_HI);
        HudDrawText(6,  8, "THE INVASION IS OVER",  HUD_FONT_COLOR_NRM);
        HudDrawText(5, 10, "THANK YOU FOR PLAYING", HUD_FONT_COLOR_NRM);
        HudDrawHLine(3, 13, 26, HUD_FONT_COLOR_NRM);
        HudDrawText(6, 20, "PRESS FIRE TO MENU", HUD_FONT_COLOR_HI);
        HudDrawText(0, 23, "GAME BY XEVIMET4L", HUD_FONT_COLOR_NRM);
        HudDrawText(28, 23, "2026", HUD_FONT_COLOR_NRM);
    }
    else if (g_TitleMode == TS_HISCORE_INPUT)
    {
        HudDrawText(11, 4, "GAME OVER", HUD_FONT_COLOR_HI);
        HudDrawText(13, 6, "TOP 3",     HUD_FONT_COLOR_HI);
        HudDrawHLine(3,  7, 26, HUD_FONT_COLOR_CYN);
        DrawHiScoreTable(g_HsPos);
        HudDrawHLine(3, 16, 26, HUD_FONT_COLOR_CYN);
        HudDrawText(0, 23, "GAME BY XEVIMET4L", HUD_FONT_COLOR_NRM);
        HudDrawText(28, 23, "2026", HUD_FONT_COLOR_NRM);
    }
    else if (g_TitleMode == TS_REDEFINE)
    {
        static const char* const key_names[7] = {"LEFT","RIGHT","UP","DOWN","FIRE","PAUSE","QUIT"};
        HudDrawText(13, 6,  "SET KEYS",   HUD_FONT_COLOR_HI);
        HudDrawHLine(3,  8, 26, HUD_FONT_COLOR_CYN);
        if (g_RedefineStep < 7)
            HudDrawText(13, 13, key_names[g_RedefineStep], HUD_FONT_COLOR_HI);
        HudDrawText(10, 16, "PRESS A KEY", HUD_FONT_COLOR_NRM);
        HudDrawText(0, 23, "GAME BY XEVIMET4L", HUD_FONT_COLOR_NRM);
        HudDrawText(28, 23, "2026", HUD_FONT_COLOR_NRM);
    }
}

// Crida quan acaba la partida per anar al menu correcte
void EnterPostGame(u8 won)
{
    u8 s;
    // Atura TOTS els sons abans d'entrar al menu (evita notes perpetuades)
    soundStopAll();
    // Desactiva TOTS els sprites abans d'entrar al menu
    for (s = 0; s < 32; s++)
        VDP_SetSpritePositionY(s, VDP_SPRITE_DISABLE_SM1);

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
             g_HsInputChar[0] = ' ';
             g_HsInputChar[1] = ' ';
             g_HsInputChar[2] = ' ';
             g_TitleMode = TS_HISCORE_INPUT;
             g_TitleDirty = 1;
         }
         else
         {
             g_TitleMode = TS_HISCORE_VIEW;
             g_TitleDirty = 1;
        }
    }
    g_GameState  = GS_TITLE;
    g_TitleDirty = 1;
    g_TitlePhase = 0;
    g_AttractFrm = 0;
}

void ResetGameSession()
{
    u8 i;
    // 1:1 CPC: resetGameSession
    g_Score       = 0;
    g_Lives       = 2;
    g_Level       = 1;
    g_CurrentBiome = 0;
    g_WavesCleared = 0;
    g_HudDirty    = 1;
    g_ShipLastLife = 0;
    g_GameOverDelay = 0;
    g_WallPhase = 0;
    g_WallPhaseTimer = 0;
    InitEnemies();
    // 1:1 CPC: initShots - neteja trets vells que poden surar de la partida anterior
    for (i = 0; i < MAX_SHOTS; i++) g_Shots[i].active = 0;
    RespawnShip();
    g_FireCooldown = 15;  // No disparar al començar (el foc ve premut del menu)
}

void InitGamePlay()
{
    u8 bank, t;
    u8 s;
    // Inicialitza pantalla de joc
    VDP_FillScreen_GM2(0);
     InitHudFontTiles();   // font per HUD

    // Disable all sprites before starting game
    for (s = 0; s < 32; s++)
        VDP_SetSpritePositionY(s, VDP_SPRITE_DISABLE_SM1);

    // Draw HUD labels immediately (VDP_FillScreen_GM2 cleared name table,
    // and UpdateHUD's static vars still hold old values so won't redraw labels)
    HudDrawText(HUD_COL, 0, "SCORE", HUD_FONT_COLOR_NRM);
    HudDrawText(HUD_COL, 6, "LEVEL", HUD_FONT_COLOR_NRM);
    HudDrawText(HUD_COL, 9, "LIVES", HUD_FONT_COLOR_NRM);
    // Precarrega patro de barra HP (tile 135, càrrega única)
    {
        u8 bar_col[8];
        u8 bar_pat[8];
        for (t = 0; t < 8; t++) { bar_col[t] = 0x81; bar_pat[t] = 0xFF; }
        for (bank = 0; bank < 3; bank++)
        {
            VDP_LoadBankPattern_GM2(bar_pat, 1, bank, BAR_FILL_TILE);
            VDP_LoadBankColor_GM2(bar_col,  1, bank, BAR_FILL_TILE);
        }
    }
    InitWallTiles();
    InitStarTiles();
    InitStars(g_S1, N1, STAR_TILE_BASE_1);
    InitStars(g_S2, N2, STAR_TILE_BASE_2);
    InitStars(g_S3, N3, STAR_TILE_BASE_3);
    ResetGameSession();
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
    VDP_LoadSpritePattern(g_EnemyShotPattern, 12, 1);
    VDP_LoadSpritePattern(g_BomberShotPattern, 88, 4);
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
    VDP_LoadSpritePattern(g_ThrusterPattern,  68, 1);
    // Boss 16x16 (composite 2-sprite like player ship)
    VDP_LoadSpritePattern(g_BossWhiteNew,     72, 4);  // Boss white layer
    VDP_LoadSpritePattern(g_BossRedNew,       76, 4);  // Boss red layer

    // Init font (necessari per tot)
    Print_SetTextFont(PRINT_DEFAULT_FONT, 1);

    // Init hi-scores
    InitHiScores();
    soundInit();

    VDP_SetSpritePositionY(0, VDP_SPRITE_DISABLE_SM1);

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
             if (g_TitleMode != TS_HISCORE_INPUT && g_TitleMode != TS_HISCORE_VIEW)
                 TickMenuStars(); // Stars corrupt text in gameover screens

               if (g_TitleDirty)
               {
                   if (g_TitleMode == TS_HISCORE_INPUT && g_TitlePhase > 1)
                   {
                       // Optimized: only redraw the input line, not the whole screen
                       RedrawHsInput();
                   }
                    else
                    {
                        // Full redraw for other screens or first frame of input
                        DrawMenuScreen();
                        if (g_TitleMode == TS_HISCORE_INPUT && g_TitlePhase == 0)
                            g_TitlePhase = 1;  // Full draw done, mark ready for fire wait
                    }
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

               // Pause/unpause with redefinable key or joystick button B
               static u8 p_key_was_pressed = 0;
               u8 p_key_pressed = INPUT_PAUSE();
               if (p_key_pressed && !p_key_was_pressed)
               {
                   g_PausedFlag = !g_PausedFlag;
                   if (!g_PausedFlag)
                   {
                       // Clear "PAUSE" text when unpausing (was drawn at col 10, row 10)
                       u8 ci;
                       for (ci = 0; ci < 5; ci++)
                           VDP_Poke_GM2((u8)(10 + ci), 10, 0);
                   }
               }
               p_key_was_pressed = p_key_pressed;

               // Quit to menu with redefinable key (joystick: no quit button)
               if (INPUT_QUIT())
              {
                  u8 s;
                  soundStopAll();
                  // Disable all sprites (ship, enemies, explosions)
                  for (s = 0; s < 32; s++)
                      VDP_SetSpritePositionY(s, VDP_SPRITE_DISABLE_SM1);
                  g_GameState = GS_TITLE;
                  g_TitleMode = TS_MENU;
                  g_TitleDirty = 1;
                  g_TitlePhase = 0;
                  // Skip rest of game frame, go directly to menu rendering
                  goto quit_to_menu;
              }

              // Skip game updates if paused (wall scroll, stars, enemies, shots, etc.)
              if (!g_PausedFlag)
              {
                  // Wall scroll + starfield
                  UpdateWallScroll();
                  switch (g_StarTimer1 & 3)
                  {
                   case 0: TickStars(g_S1, N1, 3, STAR_TILE_BASE_1); break;
                   case 1: TickStars(g_S2, N2, 4, STAR_TILE_BASE_2); break;
                   case 2: TickStars(g_S3, N3, 6, STAR_TILE_BASE_3); break;
                   case 3: TickStars(g_S1, N1, 1, STAR_TILE_BASE_1); break;
                  }
                  g_StarTimer1++;

                   // Input
                    if (!g_ShipExploding)
                    {
                        u8 thrust_now = 0;
                        if (INPUT_LEFT()  && g_ShipX > SHIP_MIN_X) { g_ShipX -= g_ShipSpeedX; thrust_now = 1; }
                        if (INPUT_RIGHT() && g_ShipX < SHIP_MAX_X) { g_ShipX += g_ShipSpeedX; thrust_now = 1; }
                        if (INPUT_UP()    && g_ShipY > SHIP_MIN_Y) { g_ShipY -= g_ShipSpeedY; thrust_now = 1; }
                        if (INPUT_DOWN()  && g_ShipY < SHIP_MAX_Y) { g_ShipY += g_ShipSpeedY; thrust_now = 1; }
                        g_ShipThrust = thrust_now;

                       if (INPUT_FIRE() && g_FireCooldown == 0)
                     {
                         for (i = 0; i < MAX_SHOTS; i++)
                         {
                             if (!g_Shots[i].active)
                             {
                                 g_Shots[i].x = g_ShipX + SHIP_W / 2 - 2;
                                 g_Shots[i].y = g_ShipY - 8;
                                 g_Shots[i].active = 1;
                                 g_FireCooldown = FIRE_COOLDOWN;
                                 sfxShot();
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
                   UpdateShipThrustAnim();
              }
              else
              {
                  // Paused: draw "PAUSE" centered in game area
                  // Game area cols 2-19 (X 16-160), "PAUSE" = 5 chars, center = col 10
                  HudDrawText(10, 10, "PAUSE", HUD_FONT_COLOR_HI);
              }

              // Victoria (nivell final completat)
             if (g_Level > ENDGAME_FINAL_LEVEL)
                 EnterPostGame(1);

             // Decrementa timer de display de bonus; el HUD nomes canvia quan desapareix
             if (g_BonusDisplayCnt > 0)
             {
                 g_BonusDisplayCnt--;
                 if (g_BonusDisplayCnt == 0) g_HudDirty = 1;
             }

             if (g_GameState == GS_PLAYING)
                 UpdateHUD();

               // Draw sprites (but not during game over delay)
               if (g_GameOverDelay == 0)
               {
  				spr = 0;

                  // Ship (spr 0-1)
                  if (!g_ShipExploding && (!g_ShipInvul || (g_ShipInvulTimer & 1)))
                  {
                      g_SprBuf[spr*4+0] = g_ShipY;  g_SprBuf[spr*4+1] = g_ShipX;  g_SprBuf[spr*4+2] = 0;   g_SprBuf[spr*4+3] = COLOR_WHITE;       spr++;
                      g_SprBuf[spr*4+0] = g_ShipY;  g_SprBuf[spr*4+1] = g_ShipX;  g_SprBuf[spr*4+2] = 4;   g_SprBuf[spr*4+3] = COLOR_MEDIUM_RED;  spr++;

                      // Thrusters (spr 2-3) - flame sprites below ship
                      if (g_ShipThrustLevel > 0)
                      {
                           u8 ty = (u8)(g_ShipY + 14);
                           u8 txl = (u8)(g_ShipX + 1);
                           u8 txr = (u8)(g_ShipX + 7);
                           u8 tcol = (g_ShipThrustFrame & 1) ? COLOR_LIGHT_YELLOW : COLOR_WHITE;
                           g_SprBuf[spr*4+0] = ty;  g_SprBuf[spr*4+1] = txl;  g_SprBuf[spr*4+2] = 68;  g_SprBuf[spr*4+3] = tcol;  spr++;
                           g_SprBuf[spr*4+0] = ty;  g_SprBuf[spr*4+1] = txr;  g_SprBuf[spr*4+2] = 68;  g_SprBuf[spr*4+3] = tcol;  spr++;
                      }
                      // Si la nau es visible NO hi ha explosio
                  }
                 else if (g_ShipExploding)
                 {
                     // Explosio 2 capes (blanc exterior + vermell interior)
                     u8 exp_pat_w, exp_pat_r;
                      if (g_ShipExplTimer > 6) { exp_pat_w = 56; exp_pat_r = 60; }
                      else { exp_pat_w = 60; exp_pat_r = 64; }
                     g_SprBuf[spr*4+0] = g_ShipY;  g_SprBuf[spr*4+1] = g_ShipX;  g_SprBuf[spr*4+2] = exp_pat_w;  g_SprBuf[spr*4+3] = COLOR_WHITE;      spr++;
                     g_SprBuf[spr*4+0] = g_ShipY;  g_SprBuf[spr*4+1] = g_ShipX;  g_SprBuf[spr*4+2] = exp_pat_r;  g_SprBuf[spr*4+3] = COLOR_MEDIUM_RED; spr++;
                 }
                 else
                 {
                     // Nau invisible o invulnerable
                     g_SprBuf[spr*4+0] = VDP_SPRITE_DISABLE_SM1; spr++;
                 }

                 // Player shots
                 for (i = 0; i < MAX_SHOTS; i++)
                     if (g_Shots[i].active)
                     {
                         g_SprBuf[spr*4+0] = g_Shots[i].y;  g_SprBuf[spr*4+1] = g_Shots[i].x;  g_SprBuf[spr*4+2] = 8;  g_SprBuf[spr*4+3] = COLOR_LIGHT_YELLOW;
                         spr++;
                     }

                  // Enemy shots
                 for (i = 0; i < MAX_ENEMY_SHOTS && spr < 18; i++)
                     if (g_EnemyShots[i].active)
                     {
                         g_SprBuf[spr*4+0] = g_EnemyShots[i].y;  g_SprBuf[spr*4+1] = g_EnemyShots[i].x;  g_SprBuf[spr*4+2] = g_EnemyShots[i].pattern;  g_SprBuf[spr*4+3] = COLOR_LIGHT_RED;
                         spr++;
                     }

                  // Explosions (ABANS dels enemics per tenir mes prioritat)
                 for (j = 0; j < MAX_EXPLOSIONS && spr < 28; j++)
                 {
                     static const u8 exp_pat_s[5] = {56, 60, 64, 64, 60};
                     if (!g_Explosions[j].active) continue;
                     f = g_Explosions[j].frame;
                     if (f > 5) f = 5;
                     pat = exp_pat_s[f];
                     col = (f == 0) ? COLOR_WHITE :
                           (f == 1) ? COLOR_LIGHT_YELLOW : COLOR_MEDIUM_RED;
                     if (g_Explosions[j].kind == EXP_KIND_BOSS)
                     {
                         g_SprBuf[spr*4+0] = g_Explosions[j].y;  g_SprBuf[spr*4+1] = g_Explosions[j].x;  g_SprBuf[spr*4+2] = pat;  g_SprBuf[spr*4+3] = COLOR_WHITE; spr++;
                         pat = exp_pat_s[(f < 5) ? f+1 : 5];
                         g_SprBuf[spr*4+0] = g_Explosions[j].y;  g_SprBuf[spr*4+1] = g_Explosions[j].x;  g_SprBuf[spr*4+2] = pat;  g_SprBuf[spr*4+3] = COLOR_MEDIUM_RED; spr++;
                     }
                     else
                     {
                         g_SprBuf[spr*4+0] = g_Explosions[j].y;  g_SprBuf[spr*4+1] = g_Explosions[j].x;  g_SprBuf[spr*4+2] = pat;  g_SprBuf[spr*4+3] = col;
                         spr++;
                     }
                 }

                    // Enemies - WHITE layer first (shapes, high priority)
                  {
                      static const u8 etype_pat_w[5] = {16, 24, 32, 40, 48};
                      for (i = 0; i < MAX_ENEMIES && spr < 28; i++)
                      {
                          if (!g_Enemies[i].active) continue;
                          j = g_Enemies[i].type;
                          if (j == ENEMY_TYPE_BOSS)
                          {
                              g_SprBuf[spr*4+0] = g_Enemies[i].y;  g_SprBuf[spr*4+1] = g_Enemies[i].x;  g_SprBuf[spr*4+2] = 72;  g_SprBuf[spr*4+3] = COLOR_CYAN;      spr++;
                          }
                          else
                          {
                              g_SprBuf[spr*4+0] = g_Enemies[i].y;  g_SprBuf[spr*4+1] = g_Enemies[i].x;  g_SprBuf[spr*4+2] = etype_pat_w[j];  g_SprBuf[spr*4+3] = COLOR_WHITE;      spr++;
                          }
                      }
                  }
                  // Enemies - RED layer last (details, lowest priority - discarded first on sprite overflow)
                  {
                      static const u8 etype_pat_r[5] = {20, 28, 36, 44, 52};
                      for (i = 0; i < MAX_ENEMIES && spr < 28; i++)
                      {
                          if (!g_Enemies[i].active) continue;
                          j = g_Enemies[i].type;
                          if (j == ENEMY_TYPE_BOSS)
                          {
                              g_SprBuf[spr*4+0] = g_Enemies[i].y;  g_SprBuf[spr*4+1] = g_Enemies[i].x;  g_SprBuf[spr*4+2] = 76;  g_SprBuf[spr*4+3] = COLOR_MEDIUM_RED; spr++;
                          }
                          else
                          {
                              g_SprBuf[spr*4+0] = g_Enemies[i].y;  g_SprBuf[spr*4+1] = g_Enemies[i].x;  g_SprBuf[spr*4+2] = etype_pat_r[j];  g_SprBuf[spr*4+3] = COLOR_MEDIUM_RED; spr++;
                          }
                      }
                  }

                 // Desactiva sprites que sobren (Y=208 = off-screen)
                 while (spr < 32)
                 {
                     g_SprBuf[spr*4+0] = VDP_SPRITE_DISABLE_SM1;
                     g_SprBuf[spr*4+1] = 0;
                     g_SprBuf[spr*4+2] = 0;
                     g_SprBuf[spr*4+3] = 0;
                     spr++;
                 }

                  // Batch write all 32 sprites to VRAM en un sol cop
                  VDP_WriteVRAM(g_SprBuf, g_SpriteAttributeLow, g_SpriteAttributeHigh, 128);
              }
              else
              {
                  // During game over delay, disable all sprites
                  u8 s;
                  for (s = 0; s < 32; s++)
                      VDP_SetSpritePositionY(s, VDP_SPRITE_DISABLE_SM1);
               }
               // Game over check - DESPRES del renderitzat de sprites (per veure l'explosio)
               if (g_ShipLastLife && !g_ShipExploding)
               {
                   if (g_GameOverDelay == 0)
                   {
                       sfxGameOver();
                       g_GameOverDelay = 1;
                   }
                   else if (g_GameOverDelay < 30)
                       g_GameOverDelay++;
                   else
                       EnterPostGame(0);
               }
                 soundTick();  // Cada frame, sempre (fins i tot al menu/pausa)
           }
quit_to_menu:
          ;
     }
}
