/* main_game.c — GAME.BIN: tota la lògica de joc (25 nivells, 5 biomes, boss).
 *
 * No inclou: títol, menú, hi-scores, redefinir tecles, title_logo.
 * Rep controls i hi-scores de MENU.BIN via shared state.
 * En acabar (game over / victòria): escriu resultats a shared state + carrega MENU.BIN.
 */

#include <cpctelera.h>
#include "ship_sprite.h"
#include "enemy_sprite.h"
#include "enemy_fast_sprite.h"
#include "enemy_heavy_sprite.h"
#include "enemy_diver_sprite.h"
#include "enemy_bomber_sprite.h"
#include "boss_sprite.h"
#include "level_config.h"
#include "hud_font.h"
#include "game_sfx.h"
#include "shot_sprite.h"
#include "enemy_shot_sprite.h"
#include "nau_shared.h"
#include "dx_bank.h"

/* ---- Dimensions ---- */
#define SCR_W_BYTES  80
#define SCR_H       200
#define TILE_W_BYTES  2
#define TILE_H        8
#define HUD_H        16
#define GAME_Y0      HUD_H
#define GAME_H      (SCR_H - HUD_H)
/* Fila on “surt” el tret (sota l’sprite): per disparar abans no cal y>=GAME_Y0 al cap superior. */
#define NAU_ENEMY_BOTTOM_PIX(i) ((u16)enemies[(i)].y+(u16)((enemies[(i)].type==(u8)ENEMY_TYPE_BOSS)?BOSS_H_PIX:ENEMY_H_PIX))
#define SCR_PAGE_C0 ((u8*)0xC000)
#define SCR_PAGE_80 ((u8*)0x8000)
/* GAME.BIN split: enllaçat a &1000 — _CODE acaba sota &8000; doble buffer C0 + &8000 (com el monolític). */
#define MAX_SHOTS     4
#define FIRE_COOLDOWN 1
#define MAX_ENEMIES   WAVE_PLAN_MAX
#define WAVE_PLAN_MAX 8
#define FORM_ROW_DY  10
#define WAVE_MAX_HEAVY_PER_WAVE 5
#define WAVE_MODE_RANK   0
#define WAVE_MODE_INDIAN 1
#define ENEMY_SPEED_Y    3
#define ENEMY_SPEED_Y_FAST  5
#define ENEMY_SPEED_Y_HEAVY 2
#define ENEMY_SPEED_Y_DIVER ENEMY_SPEED_Y_FAST
#define ENEMY_SPEED_Y_BOMBER 2
#define ENEMY_MIN_X   4
#define ENEMY_MAX_X  (76 - ENEMY_W_BYTES)
#define BOSS_MIN_X   ENEMY_MIN_X
#define BOSS_MAX_X   (76 - BOSS_W_BYTES) /* Evita solapament amb el wall dret (x>=76). */
#define BOSS_MID_X   ((u8)((BOSS_MIN_X+BOSS_MAX_X)/2))
#define ENEMY_SPACING 2
#define ENEMY_SPAWN_Y (GAME_Y0 + 2)
#define SPAWN_BASE    2
#define SPAWN_VARIANCE 5
#define SPAWN_FIRST_DELAY 4
#define SPAWN_START_GRACE 62
#define SERIAL_DELAY  8
#define MAP_W        40
#define MAP_H        25
#define PATT_STRAIGHT 0
#define PATT_ZIGZAG   1
#define PATT_DIAGONAL 2
#define RESPAWN_INVUL_TICKS 40
#define MAX_ENEMY_SHOTS 11
#define ENEMYSHOT_SPEED_Y 5
#define EXTRA_LIFE_EVERY 5000U
#define BOSS_HP_BASE 15
#define BOSS_HP_PER_TIER 5
#define BOSS_MAX_VY  3
#define BOSS_HOLD_Y  ((u8)(GAME_Y0+58))
/* Oscil·lació vertical del boss (píxels cap amunt des de la línia de hold). */
#define BOSS_Y_OSC   22
#define BOSS_DUAL_LANE_HALF  13
#define BOSS_DUAL_MIN_GAP    12
#define BOSS_VOSC_DESC 0u
#define BOSS_VOSC_UP   1u
#define BOSS_VOSC_DOWN 2u
#define ENEMYSHOT_VX_FAST 2
#define ENEMYSHOT_VX_SLOW 1
#define ENEMYSHOT_COOLDOWN 18
#define ENEMYSHOT_STAGGER  5
#define ENDGAME_FINAL_LEVEL 25
#define H_NUM 7
#define H_DEN 4
#define V_NUM 9
#define V_DEN 2
#define HUD_SCORE_X  3
#define HUD_BONUS_X 20
#define HUD_LIVES_X 29
#define HUD_LIVES_NUM_X 34
#define HUD_LEVEL_X 44
#define BOSS_BAR_X   59
#define BOSS_BAR_Y1  3
#define BOSS_BAR_Y2  9
#define BOSS_BAR_W   18
#define BOSS_BAR_H   2
#define MAX_EXPLOSIONS 6
#define EXP_KIND_ENEMY 0
#define EXP_KIND_SHIP  1
#define ENEMY_TYPE_BASIC  0
#define ENEMY_TYPE_FAST   1
#define ENEMY_TYPE_HEAVY  2
#define ENEMY_TYPE_DIVER  3
#define ENEMY_TYPE_BOMBER 4
#define ENEMY_TYPE_BOSS   5
#define NAU_TEST_NO_BOSS 0
#define DIVER_WAVE_MIN 5
#define DIVER_WAVE_MAX 8
#define N1 4
#define N2 7
#define N3 11
#define STAR_X_MIN  5
#define STAR_X_MAX 74
#define STAR_X_SPAN 70

/* ---- Colors ---- */
#define B_BG       cpct_px2byteM0(0,0)
#define B_WALL_A   cpct_px2byteM0(8,8)
#define B_WALL_B   cpct_px2byteM0(9,9)
#define B_STAR1    cpct_px2byteM0(1,1)
#define B_STAR2    cpct_px2byteM0(2,2)
#define B_STAR3    cpct_px2byteM0(4,4)
#define B_HUD_LINE cpct_px2byteM0(10,10)
#define B_EXP_W    cpct_px2byteM0(4,4)
#define B_EXP_Y    cpct_px2byteM0(2,2)
#define B_EXP_R    cpct_px2byteM0(6,6)
#define B_EXP_O    cpct_px2byteM0(9,9)

/* ---- Tipus ---- */
typedef struct { u8 x; u8 y; } TStar;
typedef struct { u8 x; u8 y; u8 active; } TShot;
typedef struct { u8 x; u8 y; i8 vx; u8 active; u8 cd; u8 vy; u8 type; } TEnemyShot;
typedef struct { u8 x; u8 y; u8 active; i8 vx; u8 vy; u8 pattern; u8 zig_timer; u8 type; u8 health; u8 boss_hp_max; u8 boss_fire_cd; u8 boss_lane_x0; u8 boss_vosc; } TEnemy;
typedef struct { u8 x; u8 y; u8 frame; u8 active; u8 kind; } TExplosion;

/* ---- Paletes ---- */
static const u8 biome_palettes[5][16] = {
    {20,6,30,19,0,25,12,21,28,14,21,20,20,20,20,20},
    {20,6,30,19,0,25,12,21,12,25,21,20,20,20,20,20},
    {20,6,30,19,0,25,12,21,24,18,21,20,20,20,20,20},
    {20,6,30,19,0,25,12,21,14, 4,21,20,20,20,20,20},
    {20,6,30,19,0,25,12,21, 5,11,21,20,20,20,20,20}
};
static const u16 enemy_score_tbl[] = {10,20,50,40,80,1500};
/* Noms de fitxers per a les transicions (scope global: SDCC C89 no suporta static local array init) */
static const u8  enemy_speed_tbl[] = {ENEMY_SPEED_Y,ENEMY_SPEED_Y_FAST,ENEMY_SPEED_Y_HEAVY,ENEMY_SPEED_Y_DIVER,ENEMY_SPEED_Y_BOMBER};

/* ---- Dirty-rect buffers (doble buffer) ---- */
static u8 dr_buf_ready[2];
static u8 dr_map_phase[2];
static u8 dr_ship_drawn[2],dr_ship_px[2],dr_ship_py[2];
static u8 dr_shot_drawn[2][MAX_SHOTS],dr_shot_px[2][MAX_SHOTS],dr_shot_py[2][MAX_SHOTS];
static u8 dr_emy_drawn[2][MAX_ENEMIES],dr_emy_px[2][MAX_ENEMIES],dr_emy_py[2][MAX_ENEMIES],dr_emy_type[2][MAX_ENEMIES];
static u8 dr_es_drawn[2][MAX_ENEMY_SHOTS],dr_es_px[2][MAX_ENEMY_SHOTS],dr_es_py[2][MAX_ENEMY_SHOTS];
static u8 dr_exp_drawn[2][MAX_EXPLOSIONS],dr_exp_px[2][MAX_EXPLOSIONS],dr_exp_py[2][MAX_EXPLOSIONS];

/* ---- Entitats ---- */
TStar s1[N1],s2[N2],s3[N3];
TShot shots[MAX_SHOTS];
static TEnemyShot enemyshots[MAX_ENEMY_SHOTS];
TEnemy enemies[MAX_ENEMIES];
static TExplosion explosions[MAX_EXPLOSIONS];

/* ---- Globals de joc ---- */
u8 fire_cool=0;
static u8 shipX,shipY;
u8 wallPhase=0;
static u8 wallTiles[8][16];
static u8* frontBuffer=SCR_PAGE_C0;
static u8* backBuffer=SCR_PAGE_80;
static u8  frontPage=cpct_pageC0;
static u8  backPage=cpct_page80;
static u8 spawn_timer=SPAWN_FIRST_DELAY,wave_total=0,wave_spawned=0;
static u16 current_wave_bonus_base=10;
u8 wave_killed=0;
static u8 wave_active=0;
static u8 boss_bar_visible=0;
static u8 wave_slot_x[WAVE_PLAN_MAX],wave_slot_y[WAVE_PLAN_MAX],wave_slot_type[WAVE_PLAN_MAX];
static u8 wave_unified_patt,wave_mode,wave_indian_delay;
static i8 wave_unified_vx;
static u8 mask_pick_buf[5];
static u8 g_boss_lane_idx;
static u16 score=0;
static u8  lives=2,level=1,waves_cleared=0,palette_dirty=0,hud_dirty=1;
static u8  hud_score_dirty=1,hud_bonus_dirty=1;
char score_text[6]="00000";
static char lives_text[2]="2";
static char level_text[6]="LV01";
char bonus_text[3]="X0";
u8 bonus_timer=0;
static u8 bonus_mult=0;
static u8 dual_boss_aim_lock=0;
static const u8 hud_heart_sprite[32]={
    0x14,0x28,0x14,0x28,
    0x3C,0x3C,0x3C,0x3C,
    0x3C,0x3C,0x3C,0x3C,
    0x3C,0x3C,0x3C,0x3C,
    0x14,0x3C,0x3C,0x28,
    0x14,0x3C,0x3C,0x28,
    0x00,0x3C,0x3C,0x00,
    0x00,0x14,0x28,0x00
};
static u8 h_acc=0,v_acc=0;
static u8 ship_exploding=0,ship_expl_timer=0,ship_thrust=0,ship_thrust_frame=0;
static u8 ship_thrust_level=0,ship_invul=0,ship_invul_timer=0,game_paused=0,ship_last_life=0;

/* ---- Controls (llegits de shared state a l'inici) ---- */
#define CTRL_JOYSTICK 0
#define CTRL_KEYBOARD 1
static u8 control_mode=CTRL_JOYSTICK;
static cpct_keyID key_left=Key_CursorLeft,key_right=Key_CursorRight;
static cpct_keyID key_up=Key_CursorUp,key_down=Key_CursorDown;
static cpct_keyID key_fire=Key_Z,key_pause=Key_P,key_quit=Key_Return;

/* ---- Funcions d'input ---- */
static u8 inputLeft(void)  { return (control_mode==CTRL_JOYSTICK)?cpct_isKeyPressed(Joy0_Left):cpct_isKeyPressed(key_left); }
static u8 inputRight(void) { return (control_mode==CTRL_JOYSTICK)?cpct_isKeyPressed(Joy0_Right):cpct_isKeyPressed(key_right); }
static u8 inputUp(void)    { return (control_mode==CTRL_JOYSTICK)?cpct_isKeyPressed(Joy0_Up):cpct_isKeyPressed(key_up); }
static u8 inputDown(void)  { return (control_mode==CTRL_JOYSTICK)?cpct_isKeyPressed(Joy0_Down):cpct_isKeyPressed(key_down); }
static u8 inputFire(void)  { return (control_mode==CTRL_JOYSTICK)?(cpct_isKeyPressed(Joy0_Fire1)||cpct_isKeyPressed(Joy0_Fire2)||cpct_isKeyPressed(Joy0_Fire3)):cpct_isKeyPressed(key_fire); }
static u8 inputPause(void) { return (control_mode==CTRL_JOYSTICK)?cpct_isKeyPressed(Key_P):cpct_isKeyPressed(key_pause); }
static u8 inputQuit(void)  { return (control_mode==CTRL_JOYSTICK)?cpct_isKeyPressed(Key_Q):cpct_isKeyPressed(key_quit); }
void hud_drawScore5_asm(u8* v);
void hud_drawBonus2_asm(u8* v);
void drawMapWalls_asm(u8* v,u8 phase,u8* tiles_base);

/* ---- Pàgines de vídeo ---- */
static void resetVideoPages(void) { frontBuffer=SCR_PAGE_C0;backBuffer=SCR_PAGE_80;frontPage=cpct_pageC0;backPage=cpct_page80;cpct_setVideoMemoryPage(frontPage); }
static void swapVideoPages(void)  { u8* tp=frontBuffer;u8 tg=frontPage;frontBuffer=backBuffer;frontPage=backPage;backBuffer=tp;backPage=tg; }

/* ---- Utilitats de dibuix ---- */
static u8 inScreen(u8 xb,u8 y){return(xb<SCR_W_BYTES&&y<SCR_H);}
static void drawPxSafe(u8* v,u8 x,u8 y,u8 c){if(y<GAME_Y0||y>=SCR_H||x>=SCR_W_BYTES)return;*cpct_getScreenPtr(v,x,y)=c;}
static void drawSolidBoxSafe(u8* v,u8 x,u8 y,u8 w,u8 h,u8 c){if(y>=SCR_H||x>=SCR_W_BYTES||y<GAME_Y0)return;if(x+w>SCR_W_BYTES)w=(u8)(SCR_W_BYTES-x);if(y+h>SCR_H)h=(u8)(SCR_H-y);if(!w||!h)return;cpct_drawSolidBox(cpct_getScreenPtr(v,x,y),c,w,h);}
static void clearGameArea(u8* v,u8 c){u8* p=cpct_getScreenPtr(v,0,GAME_Y0);cpct_drawSolidBox(p,c,64,(u8)GAME_H);cpct_drawSolidBox(cpct_getScreenPtr(p,64,0),c,16,(u8)GAME_H);}
static u8 rectIntersects(u8 x1,u8 y1,u8 w1,u8 h1,u8 x2,u8 y2,u8 w2,u8 h2){if(x1+w1<=x2)return 0;if(x2+w2<=x1)return 0;if(y1+h1<=y2)return 0;if(y2+h2<=y1)return 0;return 1;}
static u8* drawHudGlyphByte(u8* d,const u8* spr){cpct_drawSprite((u8*)spr,d,HUD_GLYPH_W_BYTES,HUD_GLYPH_H);if(HUD_GLYPH_GAP_BYTES)cpct_drawSolidBox(d+HUD_GLYPH_W_BYTES,B_BG,HUD_GLYPH_GAP_BYTES,HUD_GLYPH_H);return d+HUD_GLYPH_ADVANCE;}

/* ---- HUD ---- */
#define initHUD() do{hud_dirty=1;}while(0)
static void syncHUDScoreText(void){
    static const u16 pow10hi[4]={10000u,1000u,100u,10u};
    u16 s=score;
    u8 i,d;
    for(i=0;i<4u;++i){
        d=0;
        while(s>=pow10hi[i]){s=(u16)(s-pow10hi[i]);++d;}
        score_text[i]=(char)('0'+d);
    }
    score_text[4]=(char)('0'+(u8)s);
    score_text[5]=0;
}
static void syncHUDMetaText(void){
    hud_updateLevelText(level,level_text);
    if(lives>9u) lives=9u;
    lives_text[0]=(char)('0'+lives);
    lives_text[1]=0;
}
static void drawHUDScore(u8* v){hud_drawScore5_asm(v);}
static void drawHUDSeparators(u8* v){
    cpct_drawSolidBox(cpct_getScreenPtr(v,18,0),B_HUD_LINE,1,HUD_H);
    cpct_drawSolidBox(cpct_getScreenPtr(v,27,0),B_HUD_LINE,1,HUD_H);
    cpct_drawSolidBox(cpct_getScreenPtr(v,41,0),B_HUD_LINE,1,HUD_H);
    cpct_drawSolidBox(cpct_getScreenPtr(v,56,0),B_HUD_LINE,1,HUD_H);
}
static void drawHUDLives(u8* v){
    u8* d;
    cpct_drawSolidBox(cpct_getScreenPtr(v,HUD_LIVES_X,4),B_BG,18,8);
    cpct_drawSprite((u8*)hud_heart_sprite,cpct_getScreenPtr(v,HUD_LIVES_X,4),4,8);
    d=cpct_getScreenPtr(v,HUD_LIVES_NUM_X,4);
    d=drawHudGlyphByte(d,hud_getGlyph('X'));
    drawHudGlyphByte(d,hud_getGlyph(lives_text[0]));
}
static void drawHUDLevel(u8* v){u8 i;u8* d=cpct_getScreenPtr(v,HUD_LEVEL_X,4);for(i=0;i<4;++i)d=drawHudGlyphByte(d,hud_getGlyph(level_text[i]));}
static void drawHUDBonus(u8* v){
    /* ASM: neteja 6 bytes d’amplada per no esborrar el separador vertical a x=27. */
    hud_drawBonus2_asm(v);
    /* El caixetí bonus trepitjava la línia bonus|vides; la restaurem sempre. */
    cpct_drawSolidBox(cpct_getScreenPtr(v,27,0),B_HUD_LINE,1,HUD_H);
}
static void drawHUD(u8* v){cpct_drawSolidBox(v,B_BG,64,HUD_H);cpct_drawSolidBox(cpct_getScreenPtr(v,64,0),B_BG,16,HUD_H);cpct_drawSolidBox(cpct_getScreenPtr(v,0,0),B_HUD_LINE,64,1);cpct_drawSolidBox(cpct_getScreenPtr(v,64,0),B_HUD_LINE,16,1);cpct_drawSolidBox(cpct_getScreenPtr(v,0,HUD_H-1),B_HUD_LINE,64,1);cpct_drawSolidBox(cpct_getScreenPtr(v,64,HUD_H-1),B_HUD_LINE,16,1);cpct_drawSolidBox(cpct_getScreenPtr(v,0,0),B_HUD_LINE,1,HUD_H);cpct_drawSolidBox(cpct_getScreenPtr(v,79,0),B_HUD_LINE,1,HUD_H);drawHUDScore(v);drawHUDBonus(v);drawHUDLives(v);drawHUDLevel(v);drawHUDSeparators(v);}
static void flushHUDToBothBuffers(void){
    if(hud_dirty){
        syncHUDScoreText();
        syncHUDMetaText();
        drawHUD(SCR_PAGE_C0);
        drawHUD(SCR_PAGE_80);
        hud_dirty=0;
        hud_score_dirty=0;
        hud_bonus_dirty=0;
        return;
    }
    if(hud_score_dirty){
        syncHUDScoreText();
        drawHUDScore(SCR_PAGE_C0);
        drawHUDScore(SCR_PAGE_80);
        hud_score_dirty=0;
    }
    if(hud_bonus_dirty){
        drawHUDBonus(SCR_PAGE_C0);
        drawHUDBonus(SCR_PAGE_80);
        hud_bonus_dirty=0;
    }
}
static void clearPauseOverlayOn(u8* v){ cpct_drawSolidBox(cpct_getScreenPtr(v,32,96),B_BG,24,16); }
static void drawPauseOverlayOn(u8* v){
    u8* d;
    clearPauseOverlayOn(v);
    d=cpct_getScreenPtr(v,32,96);
    d=drawHudGlyphByte(d,hud_getGlyph('P'));
    d=drawHudGlyphByte(d,hud_getGlyph('A'));
    d=drawHudGlyphByte(d,hud_getGlyph('U'));
    d=drawHudGlyphByte(d,hud_getGlyph('S'));
    drawHudGlyphByte(d,hud_getGlyph('E'));
}

static void drawOneBossBar(u8* v,u8 x,u8 y,u8 hp,u8 hpmax){
    u8 fill=0;
    if(hpmax){
        u16 prod=(u16)hp*(u16)BOSS_BAR_W;
        while(prod>=(u16)hpmax&&fill<BOSS_BAR_W){prod=(u16)(prod-(u16)hpmax);++fill;}
        if(hp&&fill==0)fill=1;
        if(fill>BOSS_BAR_W)fill=BOSS_BAR_W;
    }
    cpct_drawSolidBox(cpct_getScreenPtr(v,(u8)(x-1u),(u8)(y-1u)),cpct_px2byteM0(1,1),(u8)(BOSS_BAR_W+2u),(u8)(BOSS_BAR_H+2u));
    cpct_drawSolidBox(cpct_getScreenPtr(v,x,y),B_BG,BOSS_BAR_W,BOSS_BAR_H);
    if(fill) cpct_drawSolidBox(cpct_getScreenPtr(v,x,y),B_EXP_R,fill,BOSS_BAR_H);
}
static void clearBossBarBothBuffers(void);
static u8 countActiveBosses(void);
#if !NAU_TEST_NO_BOSS
static void clearBossBarStrip(u8* v){
    /* Neteja acotada a la capsa de barra(s), sense tocar extrems del HUD. */
    cpct_drawSolidBox(cpct_getScreenPtr(v,(u8)(BOSS_BAR_X-1u),(u8)(BOSS_BAR_Y1-1u)),(u8)0,(u8)(BOSS_BAR_W+2u),(u8)(BOSS_BAR_Y2-BOSS_BAR_Y1+BOSS_BAR_H+2u));
}
static void drawBossHealthBars(u8* v){
    u8 idx0=255,idx1=255,i,n=0;
    for(i=0;i<MAX_ENEMIES;++i){
        if(enemies[i].active&&enemies[i].type==ENEMY_TYPE_BOSS&&enemies[i].health){
            if(idx0==255) idx0=i;
            else if(idx1==255) idx1=i;
            ++n;
        }
    }
    if(!n){
        clearBossBarBothBuffers();
        boss_bar_visible=0;
        return;
    }
    boss_bar_visible=1;
    clearBossBarStrip(v);
    if(idx0!=255) drawOneBossBar(v,BOSS_BAR_X,BOSS_BAR_Y1,enemies[idx0].health,enemies[idx0].boss_hp_max);
    if((level>=25u || n>=2u) && idx1!=255) drawOneBossBar(v,BOSS_BAR_X,BOSS_BAR_Y2,enemies[idx1].health,enemies[idx1].boss_hp_max);
}
static u8 countActiveBosses(void){
    u8 i,n=0;
    for(i=0;i<MAX_ENEMIES;++i)
        if(enemies[i].active && enemies[i].type==ENEMY_TYPE_BOSS)
            ++n;
    return n;
}
static void spawnEnemyBullet(u8 xspawn,u8 yspawn,i8 vx,u8 bvy,u8 type);
static u8 bossTryAimedShot(u8 dual_mode,u8 x,u8 y,i8 vx,u8 vy){
    if(dual_mode){
        if(dual_boss_aim_lock) return 0;
        dual_boss_aim_lock=10;
        if(vy>4u) --vy; /* In dual-boss mode, aimed bullets are slightly slower. */
    }
    spawnEnemyBullet(x,y,vx,vy,ENEMYSHOT_TYPE_BULLET);
    return 1;
}
static void clearBossBarBothBuffers(void){
    clearBossBarStrip(SCR_PAGE_C0);
    clearBossBarStrip(SCR_PAGE_80);
}
#else
static void clearBossBarStrip(u8* v){(void)v;}
static void drawBossHealthBars(u8* v){(void)v;boss_bar_visible=0;}
static u8 countActiveBosses(void){return 0;}
static void clearBossBarBothBuffers(void){boss_bar_visible=0;}
#endif

/* ---- Starfield ---- */
void v9_tickStarsShots(void);
static void goToMenu(u8 result);
static u8 tileIsWall(u8 tx,u8 ty);
/* Rand uniforme [0,n) sense __moduchar (z80.lib / modunsigned). */
static u8 nau_rand_u8_below(u8 n){u8 r;if(n<=1u)return 0;r=(u8)cpct_rand();while(r>=n)r=(u8)(r-n);return r;}
static u8 isCorridorPos(u8 xb,u8 y){u8 tx=xb/TILE_W_BYTES,ty=y/TILE_H;if(tx>=MAP_W||ty>=MAP_H)return 0;if(y<GAME_Y0)return 0;return tileIsWall(tx,ty)?0:1;}
static void randomCorridorPos(TStar* s){u8 t=0;while(t<64){u8 x=cpct_rand(),y=(u8)(GAME_Y0+nau_rand_u8_below((u8)GAME_H));if(x<SCR_W_BYTES&&y<SCR_H&&isCorridorPos(x,y)){s->x=x;s->y=y;return;}++t;}s->x=(SCR_W_BYTES>>1);s->y=(GAME_Y0+(GAME_H>>1));}
/* Offsets floor(i*STAR_X_SPAN/n) per n fix N1/N2/N3 — evita __divuint a initStars. */
static const u8 star_xoff_n4[4]={0,17,35,52};
static const u8 star_xoff_n7[7]={0,10,20,30,40,50,60};
static const u8 star_xoff_n11[11]={0,6,12,19,25,31,38,44,51,57,63};
static void initStars(TStar* s,u8 n){
    const u8* off;
    u8 i,slot,x;
    if(n==N1){off=star_xoff_n4;slot=(u8)(STAR_X_SPAN/N1);}
    else if(n==N2){off=star_xoff_n7;slot=(u8)(STAR_X_SPAN/N2);}
    else{off=star_xoff_n11;slot=(u8)(STAR_X_SPAN/N3);}
    if(!slot)slot=1;
    for(i=0;i<n;++i){
        x=(u8)(STAR_X_MIN+off[i]+nau_rand_u8_below(slot));
        if(x>STAR_X_MAX)x=STAR_X_MAX;
        s[i].x=x;
        s[i].y=(u8)(GAME_Y0+nau_rand_u8_below((u8)GAME_H));
        if(!isCorridorPos(s[i].x,s[i].y))randomCorridorPos(&s[i]);
    }
}
static void drawStars(u8* v,TStar* s,u8 n,u8 b){u8 i;for(i=0;i<n;++i){if(s[i].x<SCR_W_BYTES&&s[i].y<SCR_H&&s[i].y>=GAME_Y0)*cpct_getScreenPtr(v,s[i].x,s[i].y)=b;}}
static void eraseStars(u8* v,TStar* s,u8 n,u8 delta2){u8 i,oy;for(i=0;i<n;++i){if(s[i].x>=SCR_W_BYTES||s[i].y>=SCR_H)continue;oy=s[i].y;if(oy<delta2){oy=(u8)(oy+(u8)(200u-delta2));if(oy>=SCR_H)continue;}else{oy=(u8)(oy-delta2);}if(oy<GAME_Y0||oy>=SCR_H)continue;*cpct_getScreenPtr(v,s[i].x,oy)=B_BG;}}

/* ---- Mapa i parets ---- */
static const u8 rock_pairs[32]={8,8,8,9,9,9,9,8,0,8,9,9,8,0,8,9,9,9,0,8,8,9,8,8,9,8,8,0,9,9,8,9};
static u8 rockByte(u8 idx){u8 i=(u8)((idx&15)<<1);return cpct_px2byteM0(rock_pairs[i],rock_pairs[i+1]);}
static void buildWallTiles(void){u8 p,r,src;for(p=0;p<8;++p)for(r=0;r<8;++r){src=(u8)(((r+p)&7)<<1);wallTiles[p][r*2]=rockByte(src);wallTiles[p][r*2+1]=rockByte(src+1);}}
static u8 levelMapGet(u8 tx,u8 ty){if(tx>=MAP_W||ty>=MAP_H)return 0;return(tx<2||tx>=38)?1:0;}
static u8 tileIsWall(u8 tx,u8 ty){return levelMapGet(tx,ty);}
static u8 rectHitsWall(u8 xb,u8 y,u8 wb,u8 h){u8 tx0=xb/TILE_W_BYTES,ty0=y/TILE_H,tx1=(xb+wb-1)/TILE_W_BYTES,ty1=(y+h-1)/TILE_H,tx,ty;if(tx1>=MAP_W)tx1=MAP_W-1;if(ty1>=MAP_H)ty1=MAP_H-1;for(ty=ty0;ty<=ty1;++ty)for(tx=tx0;tx<=tx1;++tx)if(tileIsWall(tx,ty))return 1;return 0;}
static void drawMap(u8* v){drawMapWalls_asm(v,wallPhase,&wallTiles[0][0]);}

/* ---- Nau ---- */
static u8 shipStepX(void){u8 s=0;h_acc+=H_NUM;while(h_acc>=H_DEN){h_acc-=H_DEN;++s;}return s;}
static u8 shipStepY(void){u8 s=0;v_acc+=V_NUM;while(v_acc>=V_DEN){v_acc-=V_DEN;++s;}return s;}
static void placeShipAtSpawn(void){shipX=(SCR_W_BYTES-SHIP_W_BYTES)>>1;shipY=(u8)(SCR_H-SHIP_H_PIX-2);h_acc=0;v_acc=0;}
static void resetShipRuntimeState(void){ship_exploding=0;ship_expl_timer=0;ship_thrust=0;ship_thrust_frame=0;ship_thrust_level=0;ship_invul=0;ship_invul_timer=0;ship_last_life=0;}
static void respawnShip(void){resetShipRuntimeState();placeShipAtSpawn();}
static void drawShipAt(u8* v,u8 x,u8 y){if(!inScreen(x,y)||y<GAME_Y0)return;cpct_drawSpriteMasked(ship_masked,cpct_getScreenPtr(v,x,y),SHIP_W_BYTES,SHIP_H_PIX);}
static void drawShipThrusters(u8* v,u8 x,u8 y){u8 lx,rx,fy,c0,c1,c2;if(!ship_thrust_level)return;lx=(u8)(x+1);rx=(u8)(x+4);fy=(u8)(y+(SHIP_H_PIX-1));if(ship_thrust_frame&1){c0=B_EXP_W;c1=B_EXP_Y;c2=B_EXP_O;}else{c0=B_EXP_Y;c1=B_EXP_W;c2=B_EXP_O;}if(ship_thrust_level>=1){drawPxSafe(v,lx,fy,c0);drawPxSafe(v,rx,fy,c0);}if(ship_thrust_level>=2){drawPxSafe(v,lx,(u8)(fy+1),c1);drawPxSafe(v,rx,(u8)(fy+1),c1);}if(ship_thrust_level>=3){drawPxSafe(v,lx,(u8)(fy+2),c2);drawPxSafe(v,rx,(u8)(fy+2),c2);}if(ship_thrust_level>=4){drawPxSafe(v,lx,(u8)(fy+3),B_EXP_O);drawPxSafe(v,rx,(u8)(fy+3),B_EXP_O);}if(ship_thrust_level>=5){drawPxSafe(v,lx,(u8)(fy+4),B_EXP_R);drawPxSafe(v,rx,(u8)(fy+4),B_EXP_R);}}
static void updateShipThrustAnim(void){if(ship_thrust){if(ship_thrust_level<5)++ship_thrust_level;++ship_thrust_frame;}else{if(ship_thrust_level)--ship_thrust_level;if(ship_thrust_level)++ship_thrust_frame;else ship_thrust_frame=0;}}
static void updateShipInvulnerability(void){if(!ship_invul)return;if(ship_invul_timer)--ship_invul_timer;if(!ship_invul_timer)ship_invul=0;}
static void triggerShipHit(void);
static void updateShipExplosionState(void){if(!ship_exploding)return;if(ship_expl_timer)--ship_expl_timer;if(ship_expl_timer)return;if(ship_last_life){ship_exploding=0;return;}respawnShip();ship_invul=1;ship_invul_timer=RESPAWN_INVUL_TICKS;}

/* ---- Shots ---- */
static void initShots(void){u8 i;for(i=0;i<MAX_SHOTS;++i)shots[i].active=0;fire_cool=0;}
static void fireShot(u8 sx,u8 sy){u8 i;if(fire_cool||ship_exploding||ship_last_life)return;for(i=0;i<MAX_SHOTS;++i)if(!shots[i].active){shots[i].active=1;shots[i].x=(u8)(sx+(SHIP_W_BYTES/2)-(SHOT_W_BYTES/2));shots[i].y=sy;fire_cool=FIRE_COOLDOWN;sfxShot();return;}}
static void drawShots(u8* v){u8 i;for(i=0;i<MAX_SHOTS;++i)if(shots[i].active&&inScreen(shots[i].x,shots[i].y)&&shots[i].y>=GAME_Y0)cpct_drawSpriteMasked(shot_getSprite(SHOT_TYPE_SINGLE),cpct_getScreenPtr(v,shots[i].x,shots[i].y),SHOT_W_BYTES,SHOT_H_PIX);}

/* ---- Enemy shots ---- */
static void initEnemyShots(void){u8 i;for(i=0;i<MAX_ENEMY_SHOTS;++i){enemyshots[i].active=0;enemyshots[i].x=0;enemyshots[i].y=0;enemyshots[i].vx=0;enemyshots[i].cd=(u8)(i*ENEMYSHOT_STAGGER);enemyshots[i].type=ENEMYSHOT_TYPE_BULLET;}}
static void drawEnemyShots(u8* v){u8 i;for(i=0;i<MAX_ENEMY_SHOTS;++i)if(enemyshots[i].active&&inScreen(enemyshots[i].x,enemyshots[i].y)&&enemyshots[i].y>=GAME_Y0)cpct_drawSpriteMasked(enemyshot_masked[enemyshots[i].type],cpct_getScreenPtr(v,enemyshots[i].x,enemyshots[i].y),ENEMYSHOT_W_BYTES,ENEMYSHOT_H_PIX);}
/* Columna de punteria: centre de la nau (mode 0, bytes); millor que només shipX. */
#define NAU_SHIP_AIM_X ((u8)(shipX + (SHIP_W_BYTES >> 1)))
/* Faixes externes = ràpid/lent; llindars més baixos que abans per forçar moviment. */
static i8 enemyShotAimVX(u8 sx){
    i16 dx=(i16)NAU_SHIP_AIM_X-(i16)sx;
    if(dx<=-10)return -ENEMYSHOT_VX_FAST;
    if(dx<=-3)return -ENEMYSHOT_VX_SLOW;
    if(dx>=10)return ENEMYSHOT_VX_FAST;
    if(dx>=3)return ENEMYSHOT_VX_SLOW;
    if(dx<0)return -ENEMYSHOT_VX_SLOW;
    if(dx>0)return ENEMYSHOT_VX_SLOW;
    return 0;
}
/* Diver: mateix centre; faixes més estretes que abans. */
static i8 enemyShotAimVXDiver(u8 sx){
    i16 dx=(i16)NAU_SHIP_AIM_X-(i16)sx;
    if(dx<=-16)return -ENEMYSHOT_VX_SLOW;
    if(dx<=-6)return -1;
    if(dx>=16)return ENEMYSHOT_VX_SLOW;
    if(dx>=6)return 1;
    if(dx<0)return -1;
    if(dx>0)return 1;
    return 0;
}
static u8 heavyShotCooldown(void){
    /* Més cadència: onades de 3 heavies + slots lliures (alloc); no tocar boss. */
    if(level>=21u) return (u8)(5u + (cpct_rand()&3u));
    if(level>=16u) return (u8)(6u + (cpct_rand()&3u));
    if(level>=11u) return (u8)(7u + (cpct_rand()&4u));
    return (u8)(8u + (cpct_rand()&5u));
}
static u8 heavyWaveCapForLevel(void){
    if(level>=21u) return 5u; /* 21-24 */
    if(level>=16u) return 4u; /* 16-19 */
    if(level>=11u) return 3u; /* 11-14 */
    return WAVE_MAX_HEAVY_PER_WAVE; /* Test/altres trams */
}
static u8 bomberWaveCapForLevel(void){
    if(level>=23u) return 5u; /* 23-24 (i 25 si s'arriba per test) */
    if(level>=22u) return 4u; /* 22 */
    return 3u;                /* 19-21 i seguretat */
}
static u8 diverShotCooldown(void){
    if(level>=21u) return (u8)(5u + (cpct_rand()&2u));
    if(level>=16u) return (u8)(6u + (cpct_rand()&2u));
    if(level>=11u) return (u8)(7u + (cpct_rand()&2u));
    return (u8)(8u + (cpct_rand()&3u));
}
/* Punteria més “morta” per al boss: menys barrera de dues bales mirant al jugador. */
static i8 enemyShotAimVXBoss(u8 sx){i16 dx=(i16)shipX-(i16)sx;if(dx<=-16)return -ENEMYSHOT_VX_FAST;if(dx<=-6)return -ENEMYSHOT_VX_SLOW;if(dx>=16)return ENEMYSHOT_VX_FAST;if(dx>=6)return ENEMYSHOT_VX_SLOW;return 0;}
static u8 allocEnemyShotSlot(void){u8 k;for(k=0;k<MAX_ENEMY_SHOTS;++k)if(!enemyshots[k].active)return k;return 255;}
#if !NAU_TEST_NO_BOSS
static u8 bossTierFromLevel(u8 lv){
    if(lv>=25u) return 4u;
    if(lv<5u) return 0u;
    if(lv<10u) return 0u;
    if(lv<15u) return 1u;
    if(lv<20u) return 2u;
    return 3u;
}
static u8 bossBulletVyFromTier(u8 t){u8 v=(u8)(ENEMYSHOT_SPEED_Y+t);if(t>=3u)v++;return v;}
static const u8 boss_fire_cd_base[4]={8,8,7,6};
static const u8 boss_fire_cd_randmask[4]={4,3,3,2};
#endif
static void spawnEnemyBullet(u8 xspawn,u8 yspawn,i8 vx,u8 bvy,u8 type){u8 k;i16 xx;u8 yy=yspawn;if(yy<GAME_Y0)yy=GAME_Y0;k=allocEnemyShotSlot();if(k==255)return;xx=(i16)xspawn;if(xx<(i16)ENEMY_MIN_X)xx=(i16)ENEMY_MIN_X;if(xx>(i16)(SCR_W_BYTES-ENEMYSHOT_W_BYTES))xx=(i16)(SCR_W_BYTES-ENEMYSHOT_W_BYTES);enemyshots[k].active=1;enemyshots[k].x=(u8)xx;enemyshots[k].y=yy;enemyshots[k].vx=vx;enemyshots[k].vy=bvy;enemyshots[k].cd=0;enemyshots[k].type=type;sfxEnemyShot();}
static void bomberFireVolley(u8 i){
    u8 bx,by,vy;
    i16 dxShip;
    bx=(u8)(enemies[i].x+2u);
    by=(u8)(enemies[i].y+ENEMY_H_PIX);
    vy=(u8)(ENEMYSHOT_SPEED_Y-1u);
    dxShip=(i16)shipX-(i16)bx;
    if(enemies[i].pattern&1u){
        /* Fan curt: obre espai i castiga campejar davant del bomber. */
        spawnEnemyBullet((u8)(bx-1u),by,(i8)-1,vy,ENEMYSHOT_TYPE_BOMB);
        spawnEnemyBullet((u8)(bx+1u),by,(i8)1,vy,ENEMYSHOT_TYPE_BOMB);
    } else {
        /* Aimed + frontal anticamper si la nau està massa centrada. */
        spawnEnemyBullet(bx,by,enemyShotAimVXBoss(bx),ENEMYSHOT_SPEED_Y,ENEMYSHOT_TYPE_BOMB);
        if(dxShip>=-4 && dxShip<=4) spawnEnemyBullet((u8)(bx+1u),by,(i8)0,(u8)(ENEMYSHOT_SPEED_Y+1u),ENEMYSHOT_TYPE_BOMB);
    }
    enemies[i].pattern^=1u;
    enemies[i].boss_fire_cd=(u8)(13u+(cpct_rand()&7u));
}
/* `pattern` només per al boss: índex de salva (0..7), cicle ventall / alternat / plomall. */
#if !NAU_TEST_NO_BOSS
static void bossFireVolley(u8 i){
    u8 pat,tier,bx,by,bvy,bvyFan,cd,dual_mode;
    i16 dxShip;
    pat=(u8)(enemies[i].pattern&3u);
    enemies[i].pattern=(u8)((pat+1u)&3u);
    tier=bossTierFromLevel(level);
    bx=(u8)(enemies[i].x+4u);
    by=(u8)(enemies[i].y+BOSS_H_PIX);
    bvy=bossBulletVyFromTier(tier);
    dual_mode=(countActiveBosses()>=2u)?1u:0u;
    bvyFan=(u8)((bvy>6u)?(u8)(bvy-2u):(u8)(bvy-1u));
    if(bvyFan<4u)bvyFan=4u;
    dxShip=(i16)shipX-(i16)bx;
    switch(pat){
    case 0u:
        /* Anti-camp frontal: si la nau està molt alineada, doble tret central. */
        if(bossTryAimedShot(dual_mode,bx,by,enemyShotAimVXBoss(bx),bvy)){
            if(dxShip>=-5 && dxShip<=5)
                spawnEnemyBullet((u8)(bx+1u),by,(i8)0,(u8)(bvy+1u),ENEMYSHOT_TYPE_BULLET);
            else if(tier>=2u)
                bossTryAimedShot(dual_mode,(u8)(bx-1u),by,enemyShotAimVXBoss((u8)(bx-1u)),bvy);
        }
        break;
    case 1u:
        /* Laterals alternats: obren espai i obliguen reposicionament. */
        spawnEnemyBullet((u8)(bx-4u),by,(i8)-1,bvyFan,ENEMYSHOT_TYPE_BULLET);
        spawnEnemyBullet((u8)(bx+4u),by,(i8)1,bvyFan,ENEMYSHOT_TYPE_BULLET);
        if(tier>=2u) spawnEnemyBullet(bx,by,(i8)0,(u8)(bvyFan+1u),ENEMYSHOT_TYPE_BULLET);
        break;
    case 2u:
        /* Ventall principal. */
        spawnEnemyBullet((u8)(bx-5u),by,(i8)-1,bvyFan,ENEMYSHOT_TYPE_BULLET);
        spawnEnemyBullet(bx,by,(i8)0,bvyFan,ENEMYSHOT_TYPE_BULLET);
        spawnEnemyBullet((u8)(bx+5u),by,(i8)1,bvyFan,ENEMYSHOT_TYPE_BULLET);
        if(tier>=2u)
            bossTryAimedShot(dual_mode,bx,by,enemyShotAimVXBoss(bx),(u8)(bvyFan+1u));
        if(tier>=3u)
            spawnEnemyBullet((u8)(bx+2u),by,(i8)-1,(u8)(bvyFan+1u),ENEMYSHOT_TYPE_BULLET);
        break;
    default:
        /* Mixt: tir dirigit + lateral compensat. */
        bossTryAimedShot(dual_mode,bx,by,enemyShotAimVXBoss(bx),bvy);
        spawnEnemyBullet((u8)(bx+8u),by,(i8)-1,bvyFan,ENEMYSHOT_TYPE_BULLET);
        if(dxShip>=-7 && dxShip<=7)
            spawnEnemyBullet((u8)(bx-7u),by,(i8)1,bvyFan,ENEMYSHOT_TYPE_BULLET);
        if(tier>=3u) bossTryAimedShot(dual_mode,(u8)(bx-2u),by,enemyShotAimVXBoss((u8)(bx-2u)),bvyFan);
        break;
    }
    /* Escalat de "mala llet": bosses 10/15/20 disparen més sovint. */
    { u8 ti=tier;if(ti>3u)ti=3u;
      cd=(u8)(boss_fire_cd_base[ti]+(cpct_rand()&boss_fire_cd_randmask[ti]));}
    if(cd<7u)cd=7u;
    if(tier>=3u&&pat==2u)cd=(u8)(cd+3u);
    enemies[i].boss_fire_cd=cd;
}
#endif
static void trySpawnEnemyShots(void){
    u8 i,k,bx,by;
    if(dual_boss_aim_lock) --dual_boss_aim_lock;
    for(i=0;i<MAX_ENEMIES;++i){
        if(!enemies[i].active||NAU_ENEMY_BOTTOM_PIX(i)<(u16)GAME_Y0) continue;
#if !NAU_TEST_NO_BOSS
        if(enemies[i].type==ENEMY_TYPE_BOSS){
            if(enemies[i].boss_fire_cd){ --enemies[i].boss_fire_cd; continue; }
            bossFireVolley(i);
            continue;
        }
#endif
        if(enemies[i].type==ENEMY_TYPE_BOMBER){
            if(enemies[i].boss_fire_cd){ --enemies[i].boss_fire_cd; continue; }
            bomberFireVolley(i);
            continue;
        }
        /* Heavy: projectil en qualsevol slot lliure; slot i només recàrrega (no 1:1 amb bala). */
        if(enemies[i].type==ENEMY_TYPE_HEAVY){
            if(enemyshots[i].cd) continue;
            k=allocEnemyShotSlot();
            if(k==255u) continue;
            bx=(u8)(enemies[i].x+2u);
            by=(u8)(enemies[i].y+ENEMY_H_PIX);
            enemyshots[k].active=1;
            enemyshots[k].x=bx;
            enemyshots[k].y=by;
            enemyshots[k].type=ENEMYSHOT_TYPE_BULLET;
            enemyshots[k].vx=enemyShotAimVX(bx);
            enemyshots[k].vy=ENEMYSHOT_SPEED_Y;
            enemyshots[k].cd=0;
            enemyshots[i].cd=heavyShotCooldown();
            sfxEnemyShot();
            continue;
        }
        if(enemyshots[i].active||enemyshots[i].cd) continue;
        enemyshots[i].active=1;
        enemyshots[i].x=(u8)(enemies[i].x+2);
        enemyshots[i].y=(u8)(enemies[i].y+ENEMY_H_PIX);
        enemyshots[i].type=ENEMYSHOT_TYPE_BULLET;
        if(enemies[i].type==ENEMY_TYPE_DIVER){
            enemyshots[i].vx=enemyShotAimVXDiver((u8)(enemies[i].x+2u));
            enemyshots[i].vy=(u8)(ENEMYSHOT_SPEED_Y+1u);
            enemyshots[i].cd=diverShotCooldown();
        }else{
            enemyshots[i].vx=enemyShotAimVX((u8)(enemies[i].x+2u));
            enemyshots[i].vy=ENEMYSHOT_SPEED_Y;
            enemyshots[i].cd=(u8)(ENEMYSHOT_COOLDOWN+(i<<1));
        }
        sfxEnemyShot();
    }
}
static void tickEnemyShots(void){
    u8 i,sp;
    for(i=0;i<MAX_ENEMY_SHOTS;++i){
        i16 nx,ny;
        if(enemyshots[i].cd) --enemyshots[i].cd;
        if(!enemyshots[i].active) continue;
        sp=enemyshots[i].vy;
        if(!sp) sp=ENEMYSHOT_SPEED_Y;
        /* sp gran corromp (SCR_H-sp) en u8; y fora de rang no s’ha de dibuixar ni conservar. */
        if(sp>=SCR_H||enemyshots[i].y<GAME_Y0){ enemyshots[i].active=0; continue; }
        ny=(i16)enemyshots[i].y+(i16)sp;
        if(ny>=(i16)SCR_H){ enemyshots[i].active=0; continue; }
        enemyshots[i].y=(u8)ny;
        nx=(i16)enemyshots[i].x+(i16)enemyshots[i].vx;
        if(nx<0||nx>=(i16)SCR_W_BYTES){ enemyshots[i].active=0; continue; }
        enemyshots[i].x=(u8)nx;
    }
}
static void collideShipEnemyShots(void){u8 i;if(ship_exploding||ship_invul||ship_last_life)return;for(i=0;i<MAX_ENEMY_SHOTS;++i){if(!enemyshots[i].active)continue;if(rectIntersects(enemyshots[i].x,enemyshots[i].y,ENEMYSHOT_W_BYTES,ENEMYSHOT_H_PIX,shipX,shipY,SHIP_W_BYTES,SHIP_H_PIX)){enemyshots[i].active=0;triggerShipHit();return;}}}

/* ---- Enemics ---- */
static void clearAllEnemies(void){u8 i;for(i=0;i<MAX_ENEMIES;++i)enemies[i].active=0;wave_active=0;boss_bar_visible=0;clearBossBarBothBuffers();}
static void clearAllShots(void){u8 i;for(i=0;i<MAX_SHOTS;++i)shots[i].active=0;fire_cool=0;}
static void clearActiveCombatState(void){clearAllEnemies();clearAllShots();initEnemyShots();}
static void initEnemies(void){u8 i;for(i=0;i<MAX_ENEMIES;++i){enemies[i].active=0;enemies[i].vy=ENEMY_SPEED_Y;enemies[i].vx=0;enemies[i].pattern=PATT_STRAIGHT;enemies[i].zig_timer=6;enemies[i].boss_hp_max=0;enemies[i].boss_fire_cd=0;enemies[i].boss_lane_x0=0;enemies[i].boss_vosc=0;}spawn_timer=SPAWN_FIRST_DELAY;wave_total=0;wave_spawned=0;wave_killed=0;wave_active=0;current_wave_bonus_base=10;wave_mode=WAVE_MODE_RANK;wave_indian_delay=0;bonus_timer=0;bonus_mult=0;dual_boss_aim_lock=0;}
static void drawEnemies(u8* v){u8 i;for(i=0;i<MAX_ENEMIES;++i)if(enemies[i].active&&enemies[i].y>=GAME_Y0){
#if !NAU_TEST_NO_BOSS
if(enemies[i].type==ENEMY_TYPE_BOSS){cpct_drawSpriteMasked(boss_masked,cpct_getScreenPtr(v,enemies[i].x,enemies[i].y),BOSS_W_BYTES,BOSS_H_PIX);}else
#endif
{static u8* const emspr[]={enemy_masked,enemy_fast_masked,enemy_heavy_masked,enemy_diver_masked,enemy_bomber_masked};cpct_drawSpriteMasked(emspr[enemies[i].type],cpct_getScreenPtr(v,enemies[i].x,enemies[i].y),ENEMY_W_BYTES,ENEMY_H_PIX);}}}
static u8 countActiveEnemies(void){u8 i,c=0;for(i=0;i<MAX_ENEMIES;++i)if(enemies[i].active)++c;return c;}
static u8 formationRowTw(u8 cnt){return(u8)((u16)cnt*ENEMY_W_BYTES+(u16)(cnt-1u)*ENEMY_SPACING);}
static i8 patternInitialVX(u8 patt){if(patt==PATT_ZIGZAG)return 1;if(patt==PATT_DIAGONAL)return(cpct_rand()&1)?1:-1;return 0;}

/* ---- Explosions ---- */
static void clearAllExplosions(void){u8 i;for(i=0;i<MAX_EXPLOSIONS;++i)explosions[i].active=0;}
void spawnExplosion(u8 x,u8 y,u8 kind){u8 i=MAX_EXPLOSIONS;while(i--)if(!explosions[i].active){explosions[i].active=1;explosions[i].x=x;explosions[i].y=y;explosions[i].frame=0;explosions[i].kind=kind;return;}}
static void tickExplosions(void){u8 i=MAX_EXPLOSIONS;while(i--){u8 maxf;if(!explosions[i].active)continue;maxf=(explosions[i].kind==EXP_KIND_SHIP)?6:4;++explosions[i].frame;if(explosions[i].frame>=maxf)explosions[i].active=0;}}
static void drawRadialBurst(u8* v,u8 cx,u8 cy,u8 r,u8 core,u8 mid,u8 outer){u8 l=(cx>r)?(u8)(cx-r):0,t=(cy>r)?(u8)(cy-r):GAME_Y0;drawSolidBoxSafe(v,cx,cy,2,2,core);if(r>=1){drawSolidBoxSafe(v,(cx>0)?(u8)(cx-1):0,cy,4,2,mid);drawSolidBoxSafe(v,cx,(cy>0)?(u8)(cy-1):GAME_Y0,2,4,mid);}if(r>=2){drawSolidBoxSafe(v,(cx>1)?(u8)(cx-2):0,cy,6,2,outer);drawSolidBoxSafe(v,cx,(cy>1)?(u8)(cy-2):GAME_Y0,2,6,outer);drawSolidBoxSafe(v,(cx>1)?(u8)(cx-1):0,(cy>1)?(u8)(cy-1):GAME_Y0,4,4,mid);}if(r>=3){drawSolidBoxSafe(v,l,cy,8,2,outer);drawSolidBoxSafe(v,cx,t,2,8,outer);drawSolidBoxSafe(v,(cx>2)?(u8)(cx-2):0,(cy>2)?(u8)(cy-2):GAME_Y0,6,6,mid);}}
static const u8 exp_maxf[2]={4,6};
static const u8 exp_cxoff[2]={1,2};
static const u8 exp_cyoff[2]={2,3};
static const u8 exp_r[2][6]={{1,2,3,2,0,0},{1,2,3,4,3,2}};
static void drawExplosionAt(u8* v,TExplosion* e){
    u8 k=(e->kind==EXP_KIND_SHIP)?1u:0u;
    u8 f=e->frame;
    u8 core,mid,outer;
    if(f>=exp_maxf[k]) f=(u8)(exp_maxf[k]-1u);
    if(!k){
        switch(f){
            case 0u: core=B_EXP_W; mid=B_EXP_Y; outer=B_EXP_R; break;
            case 1u: core=B_EXP_W; mid=B_EXP_Y; outer=B_EXP_O; break;
            case 2u: core=B_EXP_Y; mid=B_EXP_O; outer=B_EXP_O; break;
            default: core=B_EXP_W; mid=B_EXP_O; outer=B_EXP_O; break;
        }
    }else{
        switch(f){
            case 0u: core=B_EXP_W; mid=B_EXP_Y; outer=B_EXP_R; break;
            case 1u: core=B_EXP_W; mid=B_EXP_Y; outer=B_EXP_O; break;
            case 2u: core=B_EXP_W; mid=B_EXP_R; outer=B_EXP_O; break;
            case 3u: core=B_EXP_Y; mid=B_EXP_O; outer=B_EXP_O; break;
            case 4u: core=B_EXP_W; mid=B_EXP_O; outer=B_EXP_O; break;
            default: core=B_EXP_W; mid=B_EXP_Y; outer=B_EXP_O; break;
        }
    }
    drawRadialBurst(v,(u8)(e->x+exp_cxoff[k]),(u8)(e->y+exp_cyoff[k]),exp_r[k][f],core,mid,outer);
}
static void drawExplosions(u8* v){u8 i=MAX_EXPLOSIONS;while(i--){if(!explosions[i].active)continue;drawExplosionAt(v,&explosions[i]);}}

/* Comptar quants «blocs» de EXTRA_LIFE_EVERY cap en s sense __divuint del z80.lib. */
static u8 score_floor_extra_lives(u16 s){
    u8 n=0;
    while(s>=EXTRA_LIFE_EVERY){s=(u16)(s-EXTRA_LIFE_EVERY);++n;}
    return n;
}
/* (level-1)/5 per a paletes (nivells 1..25). */
static u8 biome_idx_from_level(u8 lev){
    u8 t=(u8)(lev-1u);
    if(t>=20u) return 4u;
    if(t>=15u) return 3u;
    if(t>=10u) return 2u;
    if(t>=5u) return 1u;
    return 0u;
}

/* ---- Score i vides ---- */
void addScore(u16 p){u16 os=score;u16 ns=(u16)(score+p);u8 hi,lo;if(ns<score)score=65535;else score=ns;hi=score_floor_extra_lives(score);lo=score_floor_extra_lives(os);for(;hi>lo;++lo){++lives;bonus_text[0]='1';bonus_text[1]='U';bonus_text[2]=0;bonus_timer=20;hud_bonus_dirty=1;hud_dirty=1;sfxBeep();}hud_score_dirty=1;}
static void triggerShipHit(void){spawnExplosion(shipX,shipY,EXP_KIND_SHIP);if(lives==0)ship_last_life=1;else{--lives;hud_dirty=1;}ship_exploding=1;ship_expl_timer=12;ship_thrust=0;ship_thrust_level=0;clearActiveCombatState();sfxShipExplosion();}

/* ---- Reset de partida ---- */
#define resetGameplayState() do{respawnShip();initShots();initEnemies();clearAllExplosions();initEnemyShots();}while(0)
static void resetGameSession(void){score=0;lives=2;level=1;waves_cleared=0;hud_dirty=1;hud_score_dirty=1;hud_bonus_dirty=1;resetGameplayState();}

/* ---- Wave building (totes les funcions del sistema d'onades) ---- */
static void buildWaveSlotLayoutRank(u8 n){u8 r0,r1,twb,sxb,sxf,twf,avail,col,idx,yb,maxsx;if(n<=3u){r0=n;r1=0;}else if(n==4u){r0=2;r1=2;}else if(n==5u){r0=3;r1=2;}else if(n==6u){r0=3;r1=3;}else if(n==7u){r0=4;r1=3;}else{r0=5;r1=3;}twb=formationRowTw(r0);avail=(u8)((ENEMY_MAX_X-ENEMY_MIN_X+1u)-twb+1u);sxb=ENEMY_MIN_X;if(avail>1u)sxb=(u8)(ENEMY_MIN_X+nau_rand_u8_below(avail));idx=0;yb=ENEMY_SPAWN_Y;for(col=0;col<r0;++col){wave_slot_x[idx]=(u8)(sxb+col*(ENEMY_W_BYTES+ENEMY_SPACING));wave_slot_y[idx]=yb;++idx;}if(!r1)return;twf=formationRowTw(r1);sxf=(u8)(sxb+(u8)((twb-twf)>>1));avail=(u8)((ENEMY_MAX_X-ENEMY_MIN_X+1u)-twf+1u);maxsx=(u8)(ENEMY_MIN_X+((avail>0u)?(u8)(avail-1u):0u));if(sxf<ENEMY_MIN_X)sxf=ENEMY_MIN_X;if(sxf>maxsx)sxf=maxsx;yb=(u8)(yb+FORM_ROW_DY);for(col=0;col<r1;++col){wave_slot_x[idx]=(u8)(sxf+col*(ENEMY_W_BYTES+ENEMY_SPACING));wave_slot_y[idx]=yb;++idx;}}
static void buildWaveSlotLayoutIndian(u8 n){u8 i,tw,avail,sx;tw=ENEMY_W_BYTES;avail=(u8)((ENEMY_MAX_X-ENEMY_MIN_X+1u)-tw+1u);sx=ENEMY_MIN_X;if(avail>1u)sx=(u8)(ENEMY_MIN_X+nau_rand_u8_below(avail));for(i=0;i<n;++i){wave_slot_x[i]=sx;wave_slot_y[i]=ENEMY_SPAWN_Y;}}
static void buildWaveSlotLayoutFastV3(void){u8 sxf,twf,avail,step,yb;step=(u8)(ENEMY_W_BYTES+ENEMY_SPACING);twf=formationRowTw(2);avail=(u8)((ENEMY_MAX_X-ENEMY_MIN_X+1u)-twf+1u);sxf=ENEMY_MIN_X;if(avail>1u)sxf=(u8)(ENEMY_MIN_X+nau_rand_u8_below(avail));yb=(u8)(ENEMY_SPAWN_Y+FORM_ROW_DY);wave_slot_x[1]=sxf;wave_slot_y[1]=yb;wave_slot_x[2]=(u8)(sxf+step);wave_slot_y[2]=yb;wave_slot_x[0]=(u8)(sxf+(u8)((twf-ENEMY_W_BYTES)>>1));wave_slot_y[0]=ENEMY_SPAWN_Y;if(wave_slot_x[0]<ENEMY_MIN_X)wave_slot_x[0]=ENEMY_MIN_X;if(wave_slot_x[0]>ENEMY_MAX_X)wave_slot_x[0]=ENEMY_MAX_X;}
static void buildWaveSlotLayoutFastRank(u8 n){if(n==3u){buildWaveSlotLayoutFastV3();return;}buildWaveSlotLayoutRank(n);if(n==4u){u8 shift=(u8)((ENEMY_W_BYTES>>1)+(ENEMY_SPACING>>1));if(wave_slot_x[3]+shift<=ENEMY_MAX_X){wave_slot_x[2]+=shift;wave_slot_x[3]+=shift;}else if(wave_slot_x[2]>=ENEMY_MIN_X+shift){wave_slot_x[2]-=shift;wave_slot_x[3]-=shift;}return;}if(n==5u){wave_slot_y[0]=ENEMY_SPAWN_Y+FORM_ROW_DY;wave_slot_y[1]=ENEMY_SPAWN_Y+FORM_ROW_DY;wave_slot_y[2]=ENEMY_SPAWN_Y+FORM_ROW_DY;wave_slot_y[3]=ENEMY_SPAWN_Y;wave_slot_y[4]=ENEMY_SPAWN_Y;return;}}
static u8 waveMaxFastAllowed(void){
    if(level>=16u) return 0u;
    if(level>=13u) return 6u;
    if(level>=9u)  return 5u;
    if(level>=6u)  return 4u;
    if(level>=4u)  return 3u;
    return 2u;
}
static u8 pickHomogeneousTypeForWave(u8 mask,u8 total){u8 nc=0,b,tt,mf,idx;if(!mask)return ENEMY_TYPE_BASIC;mf=waveMaxFastAllowed();for(b=0;b<5;++b){if(!(mask&(1u<<b)))continue;tt=(u8)b;if(tt==ENEMY_TYPE_FAST&&total>mf)continue;if(tt==ENEMY_TYPE_HEAVY&&total>WAVE_MAX_HEAVY_PER_WAVE)continue;mask_pick_buf[nc++]=tt;}if(!nc)return ENEMY_TYPE_BASIC;idx=cpct_rand();idx^=(u8)(waves_cleared<<1);idx=(u8)(idx+level);while(idx>=nc)idx=(u8)(idx-nc);return mask_pick_buf[idx];}
static u8 waveTypeFromSingleLmask(u8 m){
    switch(m){
        case 0:            return ENEMY_TYPE_BASIC;
        case LMASK_BASIC:  return ENEMY_TYPE_BASIC;
        case LMASK_FAST:   return ENEMY_TYPE_FAST;
        case LMASK_HEAVY:  return ENEMY_TYPE_HEAVY;
        case LMASK_DIVER:  return ENEMY_TYPE_DIVER;
        case LMASK_BOMBER: return ENEMY_TYPE_BOMBER;
        default:           return (u8)255;
    }
}
static void buildWaveTypePlan(u8 total,u8 mask){u8 slot,t,st;st=waveTypeFromSingleLmask(mask);if(st!=(u8)255){t=st;for(slot=0;slot<total;++slot)wave_slot_type[slot]=t;current_wave_bonus_base=enemy_score_tbl[t];return;}t=pickHomogeneousTypeForWave(mask,total);for(slot=0;slot<total;++slot)wave_slot_type[slot]=t;current_wave_bonus_base=enemy_score_tbl[t];}
static void getPatternForType(u8 enemy_type,u8* patt,i8* vx){static const u8 basic_patt_r3[8]={0,1,2,0,1,2,0,1};if(enemy_type==ENEMY_TYPE_BASIC){*patt=basic_patt_r3[cpct_rand()&7u];*vx=patternInitialVX(*patt);}else if(enemy_type==ENEMY_TYPE_FAST){*patt=(cpct_rand()&1u)?PATT_DIAGONAL:PATT_ZIGZAG;*vx=patternInitialVX(*patt);}else if(enemy_type==ENEMY_TYPE_HEAVY||enemy_type==ENEMY_TYPE_BOMBER){*patt=PATT_STRAIGHT;*vx=(cpct_rand()&1u)?(i8)1:(i8)-1;}else if(enemy_type==ENEMY_TYPE_DIVER){*patt=PATT_DIAGONAL;*vx=(cpct_rand()&1u)?(i8)2:(i8)-2;}else{*patt=PATT_STRAIGHT;*vx=0;}}
static void buildWaveLayoutAfterType(u8 n){u8 t=wave_slot_type[0];if(wave_mode==WAVE_MODE_INDIAN)buildWaveSlotLayoutIndian(n);else if(t==ENEMY_TYPE_FAST)buildWaveSlotLayoutFastRank(n);else buildWaveSlotLayoutRank(n);}
static void pickWaveUnifiedMovement(void){getPatternForType(wave_slot_type[0],&wave_unified_patt,&wave_unified_vx);}
static void spawnSingleEnemy(u8 x,u8 y,u8 patt,i8 vx,u8 enemy_type){
    u8 i=MAX_ENEMIES;
    while(i--) if(!enemies[i].active){
#if !NAU_TEST_NO_BOSS
        u8 tier;
#endif
        enemies[i].active=1;
        enemies[i].x=x;
        enemies[i].y=y;
        enemies[i].vx=vx;
        enemies[i].pattern=patt;
        enemies[i].zig_timer=6;
        enemies[i].type=enemy_type;
        #if !NAU_TEST_NO_BOSS
        if(enemy_type==ENEMY_TYPE_BOSS){
            /* Pivot central: el boss tendeix a la meitat del corredor jugable. */
            tier=bossTierFromLevel(level);
            if(level>=25u && wave_total==2u) tier=3u; /* Dual final: HP per boss = boss single de LV20. */
            enemies[i].health=(u8)(BOSS_HP_BASE+tier*BOSS_HP_PER_TIER);
            enemies[i].boss_hp_max=enemies[i].health;
            enemies[i].vy=(u8)(1u+(tier>>1));
            if(enemies[i].vy>BOSS_MAX_VY)enemies[i].vy=BOSS_MAX_VY;
            enemies[i].boss_fire_cd=(u8)(8u+(cpct_rand()&15u));
            enemies[i].boss_lane_x0=BOSS_MID_X;
            if(g_boss_lane_idx==0u) enemies[i].boss_lane_x0=(u8)(BOSS_MIN_X+12u);
            else if(g_boss_lane_idx==2u) enemies[i].boss_lane_x0=(u8)(BOSS_MAX_X-12u);
            enemies[i].x=enemies[i].boss_lane_x0;
            if(enemies[i].x<BOSS_MIN_X) enemies[i].x=BOSS_MIN_X;
            if(enemies[i].x>BOSS_MAX_X) enemies[i].x=BOSS_MAX_X;
            enemies[i].zig_timer=(u8)(8u+(cpct_rand()&7u));
            enemies[i].boss_vosc=BOSS_VOSC_DESC;
            enemies[i].pattern=(u8)(cpct_rand()&7u);
        }else{
            enemies[i].vy=enemy_speed_tbl[enemy_type];
            enemies[i].health=(enemy_type==ENEMY_TYPE_HEAVY||enemy_type==ENEMY_TYPE_BOMBER)?3:1;
            enemies[i].boss_hp_max=0;
            if(enemy_type==ENEMY_TYPE_BOMBER){
                /* Desfase inicial per evitar salves idèntiques en files de 3. */
                enemies[i].boss_fire_cd=(u8)(8u+(cpct_rand()&7u));
                enemies[i].pattern=(u8)(cpct_rand()&1u);
            }else{
                enemies[i].boss_fire_cd=0;
            }
            enemies[i].boss_lane_x0=0;
            enemies[i].boss_vosc=0;
        }
        #else
        enemies[i].vy=enemy_speed_tbl[enemy_type];
        enemies[i].health=(enemy_type==ENEMY_TYPE_HEAVY||enemy_type==ENEMY_TYPE_BOMBER)?3:1;
        enemies[i].boss_hp_max=0;
        if(enemy_type==ENEMY_TYPE_BOMBER){
            enemies[i].boss_fire_cd=(u8)(8u+(cpct_rand()&7u));
            enemies[i].pattern=(u8)(cpct_rand()&1u);
        }else{
            enemies[i].boss_fire_cd=0;
        }
        enemies[i].boss_lane_x0=0;
        enemies[i].boss_vosc=0;
        #endif
        /* Desfasament pel primer tret: ordre d’aparició a l’onada (wave_spawned), no l’índex de slot
         * (sovint alt → i*3 gran i sembla “nivells lents”). */
        if(enemy_type==ENEMY_TYPE_DIVER) enemyshots[i].cd=(u8)(cpct_rand()&1u);
        else enemyshots[i].cd=(u8)((wave_spawned<<1)+(cpct_rand()&7));
        return;
    }
}
static void spawnOneFromQueue(void){u8 slot,t,x,y;if(wave_spawned>=wave_total)return;slot=wave_spawned;t=wave_slot_type[slot];x=wave_slot_x[slot];y=wave_slot_y[slot];spawnSingleEnemy(x,y,wave_unified_patt,wave_unified_vx,t);++wave_spawned;}
static void tryWaveSpawnTopup(void){if(wave_mode==WAVE_MODE_INDIAN){if(wave_indian_delay){--wave_indian_delay;return;}if(wave_spawned<wave_total&&countActiveEnemies()<wave_total){spawnOneFromQueue();if(wave_spawned<wave_total)wave_indian_delay=SERIAL_DELAY;}return;}while(wave_spawned<wave_total&&countActiveEnemies()<wave_total)spawnOneFromQueue();}
static void startNewWave(void){const TLevelConfig* cfg;u8 wave_mask;cfg=level_config_get(level);wave_killed=0;wave_active=1;wave_spawned=0;wave_indian_delay=0;
#if !NAU_TEST_NO_BOSS
if(cfg->flags&LCFG_F_BOSS2){wave_total=2;current_wave_bonus_base=0;g_boss_lane_idx=(cpct_rand()&1u)?0u:2u;spawnSingleEnemy(0,ENEMY_SPAWN_Y,PATT_STRAIGHT,(i8)1,ENEMY_TYPE_BOSS);g_boss_lane_idx=(u8)(2u-g_boss_lane_idx);spawnSingleEnemy(0,ENEMY_SPAWN_Y,PATT_STRAIGHT,(i8)1,ENEMY_TYPE_BOSS);wave_spawned=2;spawn_timer=(u8)(SPAWN_BASE+(cpct_rand()&SPAWN_VARIANCE));sfxBossWarn();return;}if(cfg->flags&LCFG_F_BOSS1){wave_total=1;current_wave_bonus_base=0;g_boss_lane_idx=1u;spawnSingleEnemy(0,ENEMY_SPAWN_Y,PATT_STRAIGHT,(i8)1,ENEMY_TYPE_BOSS);wave_spawned=1;spawn_timer=(u8)(SPAWN_BASE+(cpct_rand()&SPAWN_VARIANCE));sfxBossWarn();return;}
#endif
wave_total=cfg->per_wave;if(wave_total>WAVE_PLAN_MAX)wave_total=WAVE_PLAN_MAX;wave_mask=cfg->mask;if(cfg->intro_mask&&waves_cleared==0u)wave_mask=cfg->intro_mask;wave_mode=((cpct_rand()&1u)==0u)?WAVE_MODE_INDIAN:WAVE_MODE_RANK;buildWaveTypePlan(wave_total,wave_mask);if(wave_slot_type[0]==ENEMY_TYPE_DIVER){wave_mode=WAVE_MODE_INDIAN;if(wave_total<DIVER_WAVE_MIN)wave_total=DIVER_WAVE_MIN;if(wave_total>DIVER_WAVE_MAX)wave_total=DIVER_WAVE_MAX;}if(wave_slot_type[0]==ENEMY_TYPE_HEAVY&&wave_total>heavyWaveCapForLevel())wave_total=heavyWaveCapForLevel();if(wave_slot_type[0]==ENEMY_TYPE_BOMBER&&wave_total>bomberWaveCapForLevel())wave_total=bomberWaveCapForLevel();buildWaveLayoutAfterType(wave_total);pickWaveUnifiedMovement();tryWaveSpawnTopup();spawn_timer=(u8)(SPAWN_BASE+(cpct_rand()&SPAWN_VARIANCE));}
static void spawnWave(void){if(wave_active){if(wave_spawned<wave_total)tryWaveSpawnTopup();return;}if(spawn_timer){--spawn_timer;return;}if(countActiveEnemies())return;startNewWave();}
static void moveEnemies(void){
    u8 i=MAX_ENEMIES;
    while(i--){
        if(!enemies[i].active)continue;
        #if !NAU_TEST_NO_BOSS
        if(enemies[i].type==ENEMY_TYPE_BOSS){
            u8 xmn,xmx,r,hold,ymin,pivot,tier,dual_mode;
            i16 nxx,dxp;
            xmn=BOSS_MIN_X;
            xmx=BOSS_MAX_X;
            pivot=enemies[i].boss_lane_x0;
            dual_mode=(countActiveBosses()>=2u)?1u:0u;
            if(dual_mode){
                i16 lx=(i16)pivot-(i16)BOSS_DUAL_LANE_HALF;
                i16 rx=(i16)pivot+(i16)BOSS_DUAL_LANE_HALF;
                if(lx<(i16)BOSS_MIN_X) lx=(i16)BOSS_MIN_X;
                if(rx>(i16)BOSS_MAX_X) rx=(i16)BOSS_MAX_X;
                xmn=(u8)lx; xmx=(u8)rx;
            }
            tier=bossTierFromLevel(level);
            hold=BOSS_HOLD_Y;
            ymin=(u8)(hold-BOSS_Y_OSC);
            if(ymin<GAME_Y0+4u)ymin=(u8)(GAME_Y0+4u);
            if(enemies[i].boss_vosc==BOSS_VOSC_DESC){
                if(enemies[i].y<hold){enemies[i].y=(u8)(enemies[i].y+enemies[i].vy);if(enemies[i].y>hold)enemies[i].y=hold;}
                if(enemies[i].y>=hold)enemies[i].boss_vosc=BOSS_VOSC_UP;
            }else if(enemies[i].boss_vosc==BOSS_VOSC_UP){
                if(enemies[i].y>ymin)enemies[i].y=(u8)(enemies[i].y-(u8)1u);else enemies[i].boss_vosc=BOSS_VOSC_DOWN;
            }else{
                if(enemies[i].y<hold)enemies[i].y=(u8)(enemies[i].y+(u8)1u);else enemies[i].boss_vosc=BOSS_VOSC_UP;
            }
            if(enemies[i].zig_timer)--enemies[i].zig_timer;
            else{
                /* Més nervi lateral als bosses 10/15/20. */
                if(tier>=3u) enemies[i].zig_timer=(u8)(6u+(cpct_rand()&7u));
                else if(tier==2u) enemies[i].zig_timer=(u8)(7u+(cpct_rand()&7u));
                else if(tier==1u) enemies[i].zig_timer=(u8)(8u+(cpct_rand()&7u));
                else enemies[i].zig_timer=(u8)(8u+(cpct_rand()&11u));
                r=(u8)(cpct_rand()&7u);
                if(tier>=2u){
                    if(r<4u) enemies[i].vx=(enemies[i].vx<0)?(i8)-2:(i8)2;
                    else enemies[i].vx=(cpct_rand()&1u)?(i8)2:(i8)-2;
                }else{
                    if(r<3u) enemies[i].vx=(enemies[i].vx<0)?(i8)-2:(i8)2;
                    else if(r<6u) enemies[i].vx=(cpct_rand()&1u)?(i8)1:(i8)-1;
                    else enemies[i].vx=(cpct_rand()&1u)?(i8)2:(i8)-2;
                }
                if(enemies[i].vx==0) enemies[i].vx=1;
            }
            /* Recentratge suau cap al pivot perquè no es quedi enganxat a un costat. */
            dxp=(i16)pivot-(i16)enemies[i].x;
            if(dxp>10 && enemies[i].vx<2) ++enemies[i].vx;
            else if(dxp<-10 && enemies[i].vx>-2) --enemies[i].vx;
            nxx=(i16)enemies[i].x+(i16)enemies[i].vx;
            if(nxx<(i16)xmn){enemies[i].x=xmn;enemies[i].vx=(enemies[i].vx<0)?(i8)2:(i8)1;}
            else if(nxx>(i16)xmx){enemies[i].x=xmx;enemies[i].vx=(enemies[i].vx>0)?(i8)-2:(i8)-1;}
            else enemies[i].x=(u8)nxx;
            if(dual_mode){
                u8 j=MAX_ENEMIES;
                while(j--){
                    i16 d;
                    if(j==i) continue;
                    if(!enemies[j].active||enemies[j].type!=ENEMY_TYPE_BOSS) continue;
                    d=(i16)enemies[i].x-(i16)enemies[j].x;
                    if(d>=0 && d<(i16)BOSS_DUAL_MIN_GAP){
                        if(enemies[i].x+1u<=xmx) ++enemies[i].x;
                    }else if(d<0 && d>-(i16)BOSS_DUAL_MIN_GAP){
                        if(enemies[i].x>=xmn+1u) --enemies[i].x;
                    }
                }
            }
            continue;
        }
        #endif
        enemies[i].y+=enemies[i].vy;
        if(enemies[i].pattern==PATT_ZIGZAG){
            if(enemies[i].zig_timer)--enemies[i].zig_timer;else{enemies[i].zig_timer=6;enemies[i].vx=-enemies[i].vx;}
            enemies[i].x+=enemies[i].vx;
            if(enemies[i].x<ENEMY_MIN_X)enemies[i].x=ENEMY_MIN_X;
            if(enemies[i].x>ENEMY_MAX_X)enemies[i].x=ENEMY_MAX_X;
        }else if(enemies[i].pattern==PATT_DIAGONAL){
            if(enemies[i].vx<0){if(enemies[i].x<=ENEMY_MIN_X)enemies[i].vx=(i8)(-enemies[i].vx);else enemies[i].x+=enemies[i].vx;}
            else if(enemies[i].vx>0){if(enemies[i].x>=ENEMY_MAX_X)enemies[i].vx=(i8)(-enemies[i].vx);else enemies[i].x+=enemies[i].vx;}
        }
        if(enemies[i].y>=(u8)(SCR_H-ENEMY_H_PIX))enemies[i].active=0;
    }
}
static void collideShotsEnemies(void){u8 i,j;for(i=0;i<MAX_ENEMIES;++i){if(!enemies[i].active)continue;for(j=0;j<MAX_SHOTS;++j){if(!shots[j].active)continue;
#if !NAU_TEST_NO_BOSS
if(enemies[i].type==ENEMY_TYPE_BOSS){if(!rectIntersects(enemies[i].x,enemies[i].y,BOSS_W_BYTES,BOSS_H_PIX,shots[j].x,shots[j].y,SHOT_W_BYTES,SHOT_H_PIX))continue;}else
#endif
{if(!rectIntersects(enemies[i].x,enemies[i].y,ENEMY_W_BYTES,ENEMY_H_PIX,shots[j].x,shots[j].y,SHOT_W_BYTES,SHOT_H_PIX))continue;}
shots[j].active=0;--enemies[i].health;if(enemies[i].health==0){spawnExplosion(enemies[i].x,enemies[i].y,EXP_KIND_ENEMY);enemies[i].active=0;++wave_killed;addScore(enemy_score_tbl[enemies[i].type]);
#if !NAU_TEST_NO_BOSS
if(enemies[i].type==ENEMY_TYPE_BOSS && countActiveBosses()==0u)clearBossBarBothBuffers();
#endif
}else{spawnExplosion(enemies[i].x,enemies[i].y,EXP_KIND_ENEMY);}sfxEnemyExplosion();break;}}}
static void collideShipEnemies(void){u8 i;if(ship_exploding||ship_invul||ship_last_life)return;for(i=0;i<MAX_ENEMIES;++i){if(!enemies[i].active)continue;
#if !NAU_TEST_NO_BOSS
if(enemies[i].type==ENEMY_TYPE_BOSS){if(rectIntersects(enemies[i].x,enemies[i].y,BOSS_W_BYTES,BOSS_H_PIX,shipX,shipY,SHIP_W_BYTES,SHIP_H_PIX)){enemies[i].active=0;triggerShipHit();return;}}else
#endif
{if(rectIntersects(enemies[i].x,enemies[i].y,ENEMY_W_BYTES,ENEMY_H_PIX,shipX,shipY,SHIP_W_BYTES,SHIP_H_PIX)){enemies[i].active=0;triggerShipHit();return;}}}}
static void updateWaveBonus(void){const TLevelConfig* cfg;if(!wave_active)return;if(countActiveEnemies())return;if(wave_spawned<wave_total)return;if(wave_killed==wave_total){u16 wb=0;u8 wk;for(wk=0;wk<wave_total;++wk)wb=(u16)(wb+current_wave_bonus_base);addScore(wb);bonus_mult=wave_total;if(bonus_mult>9u)bonus_mult=9u;if(bonus_mult>=2u){bonus_text[0]='X';bonus_text[1]=(char)('0'+bonus_mult);bonus_text[2]=0;bonus_timer=27;hud_bonus_dirty=1;}}++waves_cleared;cfg=level_config_get(level);if(waves_cleared>=cfg->waves){waves_cleared=0;if(level>=ENDGAME_FINAL_LEVEL)goToMenu(NAU_RESULT_WIN);if(level<25u){++level;palette_dirty=1;sfxLevelUp();}hud_dirty=1;}wave_active=0;}
static void tickBonusHud(void){if(!bonus_timer)return;--bonus_timer;if(!bonus_timer)hud_bonus_dirty=1;}

/* ---- Dirty-rects i drawFrame ---- */
static void initDirtyRects(void){u8 i,b;for(b=0;b<2;++b){dr_buf_ready[b]=0;dr_map_phase[b]=0xFF;dr_ship_drawn[b]=0;for(i=0;i<MAX_SHOTS;++i)dr_shot_drawn[b][i]=0;for(i=0;i<MAX_ENEMIES;++i)dr_emy_drawn[b][i]=0;for(i=0;i<MAX_ENEMY_SHOTS;++i)dr_es_drawn[b][i]=0;for(i=0;i<MAX_EXPLOSIONS;++i)dr_exp_drawn[b][i]=0;}}
static void drawFrame(void){
    u8 i;
    u8 bi=(backBuffer==SCR_PAGE_C0)?0:1;
    if(dr_buf_ready[bi]){
        if(dr_ship_drawn[bi]) drawSolidBoxSafe(backBuffer,dr_ship_px[bi],dr_ship_py[bi],SHIP_W_BYTES,(u8)(SHIP_H_PIX+5),B_BG);
        for(i=0;i<MAX_SHOTS;++i) if(dr_shot_drawn[bi][i]) drawSolidBoxSafe(backBuffer,dr_shot_px[bi][i],dr_shot_py[bi][i],SHOT_W_BYTES,SHOT_H_PIX,B_BG);
        for(i=0;i<MAX_ENEMIES;++i) if(dr_emy_drawn[bi][i]){u8 ew=(dr_emy_type[bi][i]==ENEMY_TYPE_BOSS)?BOSS_W_BYTES:ENEMY_W_BYTES;u8 eh=(dr_emy_type[bi][i]==ENEMY_TYPE_BOSS)?BOSS_H_PIX:ENEMY_H_PIX;drawSolidBoxSafe(backBuffer,dr_emy_px[bi][i],dr_emy_py[bi][i],ew,eh,B_BG);}
        for(i=0;i<MAX_ENEMY_SHOTS;++i) if(dr_es_drawn[bi][i]) drawSolidBoxSafe(backBuffer,dr_es_px[bi][i],dr_es_py[bi][i],ENEMYSHOT_W_BYTES,ENEMYSHOT_H_PIX,B_BG);
        for(i=0;i<MAX_EXPLOSIONS;++i) if(dr_exp_drawn[bi][i]){u8 ex=(dr_exp_px[bi][i]>=3)?(u8)(dr_exp_px[bi][i]-3):0;u8 ey=(dr_exp_py[bi][i]>=3)?(u8)(dr_exp_py[bi][i]-3):GAME_Y0;if(ey<GAME_Y0)ey=GAME_Y0;drawSolidBoxSafe(backBuffer,ex,ey,16,16,B_BG);}
        eraseStars(backBuffer,s1,N1,8);
        eraseStars(backBuffer,s2,N2,18);
        eraseStars(backBuffer,s3,N3,32);
    }
    /* Esborra la franja UI del boss abans de redibuixar-la. */
    clearBossBarStrip(backBuffer);
    if(dr_map_phase[bi] != wallPhase){
        drawMap(backBuffer);
        dr_map_phase[bi] = wallPhase;
    }
    drawStars(backBuffer,s1,N1,B_STAR1);
    drawStars(backBuffer,s2,N2,B_STAR2);
    drawStars(backBuffer,s3,N3,B_STAR3);
    drawBossHealthBars(backBuffer);
    drawShots(backBuffer);
    drawEnemyShots(backBuffer);
    drawEnemies(backBuffer);
    drawExplosions(backBuffer);
    if(!ship_exploding&&!ship_last_life&&(!ship_invul||(ship_invul_timer&1))){
        drawShipThrusters(backBuffer,shipX,shipY);
        drawShipAt(backBuffer,shipX,shipY);
        dr_ship_drawn[bi]=1;
    } else { dr_ship_drawn[bi]=0; }
    dr_ship_px[bi]=shipX; dr_ship_py[bi]=shipY;
    for(i=0;i<MAX_SHOTS;++i){dr_shot_drawn[bi][i]=shots[i].active;dr_shot_px[bi][i]=shots[i].x;dr_shot_py[bi][i]=shots[i].y;}
    for(i=0;i<MAX_ENEMIES;++i){dr_emy_drawn[bi][i]=enemies[i].active;dr_emy_px[bi][i]=enemies[i].x;dr_emy_py[bi][i]=enemies[i].y;dr_emy_type[bi][i]=enemies[i].type;}
    for(i=0;i<MAX_ENEMY_SHOTS;++i){dr_es_drawn[bi][i]=enemyshots[i].active;dr_es_px[bi][i]=enemyshots[i].x;dr_es_py[bi][i]=enemyshots[i].y;}
    for(i=0;i<MAX_EXPLOSIONS;++i){dr_exp_drawn[bi][i]=explosions[i].active;dr_exp_px[bi][i]=explosions[i].x;dr_exp_py[bi][i]=explosions[i].y;}
    dr_buf_ready[bi]=1;
    if(palette_dirty){u8 biome_idx=biome_idx_from_level(level);if(biome_idx>4u)biome_idx=4u;cpct_waitVSYNC();cpct_setPalette((u8*)biome_palettes[biome_idx],16);palette_dirty=0;}
    cpct_waitVSYNC();
    cpct_setVideoMemoryPage(backPage);
    swapVideoPages();
}

/* ---- Transició a MENU.BIN en acabar la partida ---- */
static void goToMenu(u8 result) {
    g_shared->score       = score;
    g_shared->level       = level;
    g_shared->game_result = result;
    g_shared->from_game   = NAU_FROM_GAME;
    soundStopAll();
    nau_transit_load_menu();
}

/* =========================================================================
 * main() — GAME.BIN
 * ========================================================================= */
void main(void) {
    u8 nx,ny,stepx,stepy,thrust_now,fire_now;
    u8 pause_key_was_pressed, pause_arm_frames;
    u8 pause_prev_state=0;
    u16 nau_firmware_jp_save;

    /* La pila no pot viure a &8000..&BFFF: aquesta zona es fa servir com a backbuffer. */
    /* Keep stack above GAME.BIN end and below TRANSIT at 0x7880. */
    cpct_setStackLocation((void*)0x787E);
    dx_memory_default();

    {
        volatile u8* p = (volatile u8*)(void*)0x0038u;
        if(p[0] == 0xC3u)
            nau_firmware_jp_save = cpct_disableFirmware();
        else {
            (void)cpct_disableFirmware();
            if(g_shared->firmware_jp != 0u)
                nau_firmware_jp_save = g_shared->firmware_jp;
            else
                nau_firmware_jp_save = NAU_FIRMWARE_IRQ_ROM_DEFAULT;
        }
    }
    g_shared->firmware_jp = nau_firmware_jp_save;

    cpct_setVideoMode(0);
    cpct_setVideoMemoryPage(cpct_pageC0);
    cpct_clearScreen(0);
    cpct_srand(12345);

    /* Carregar controls des de shared state (escrits per MENU.BIN) */
    if(g_shared->magic == NAU_SHARED_MAGIC && nau_shared_payload_plausible()) {
        control_mode = g_shared->control_mode;
        key_left     = g_shared->key_left;
        key_right    = g_shared->key_right;
        key_up       = g_shared->key_up;
        key_down     = g_shared->key_down;
        key_fire     = g_shared->key_fire;
        key_pause    = g_shared->key_pause;
        key_quit     = g_shared->key_quit;
    }

    ship_buildMaskedSprite();
    boss_buildMaskedSprite();
    enemy_buildMaskedSprite();
    enemy_fast_buildMaskedSprite();
    enemy_heavy_buildMaskedSprite();
    enemy_diver_buildMaskedSprite();
    enemy_bomber_buildMaskedSprite();
    shot_buildMaskedSprite();
    hud_buildFont();
    buildWallTiles();
    resetVideoPages();
    initStars(s1,N1);
    initStars(s2,N2);
    initStars(s3,N3);
    resetGameSession();
    initHUD();
    soundInit();

    { u8 bi=biome_idx_from_level(level); if(bi>4u)bi=4u; cpct_setPalette((u8*)biome_palettes[bi],16); }
    game_paused=0;
    clearGameArea(SCR_PAGE_C0,B_BG);
    clearGameArea(SCR_PAGE_80,B_BG);
    drawMap(SCR_PAGE_C0);
    drawMap(SCR_PAGE_80);
    initDirtyRects();
    flushHUDToBothBuffers();
    drawFrame();
    cpct_scanKeyboard_f();
    pause_key_was_pressed = inputPause();
    pause_arm_frames = 8;
    spawn_timer=SPAWN_START_GRACE;

    /* ---- Bucle principal (GS_PLAYING) ---- */
    while(1) {
        static u8 game_over_delay=0;

        cpct_scanKeyboard_f();

        /* Pausa */
        if(pause_arm_frames) {
            --pause_arm_frames;
            pause_key_was_pressed = inputPause();
        } else {
            if(inputPause()&&!pause_key_was_pressed) {
                pause_key_was_pressed=1;
            } else if(!inputPause()&&pause_key_was_pressed) {
                pause_key_was_pressed=0;
                game_paused^=1;
                if(game_paused) {
                    soundStopAll();
                } else {
                    soundInit();
                }
            }
        }

        /* Sortir al menú */
        if(inputQuit()) {
            goToMenu(NAU_RESULT_GAMEOVER);
        }

        if(game_paused) {
            static u8 pause_blink_timer=0,pause_blink_state=0;
            if(!pause_prev_state){
                /* Entrada a pausa: neteja inicial als dos buffers per evitar residus. */
                clearPauseOverlayOn(SCR_PAGE_C0);
                clearPauseOverlayOn(SCR_PAGE_80);
                pause_prev_state=1;
            }
            pause_blink_timer++;
            if(pause_blink_timer>=8){pause_blink_timer=0;pause_blink_state^=1;}
            cpct_waitVSYNC();
            if(pause_blink_state) {
                /* Dibuixar només al buffer visible evita dibuix parcial tipus "PAUS". */
                drawPauseOverlayOn(frontBuffer);
            } else {
                clearPauseOverlayOn(frontBuffer);
            }
            cpct_setVideoMemoryPage(frontPage);
            cpct_scanKeyboard_f();
            continue;
        }
        if(pause_prev_state){
            clearPauseOverlayOn(SCR_PAGE_C0);
            clearPauseOverlayOn(SCR_PAGE_80);
            pause_prev_state=0;
        }

        if(!ship_exploding&&!ship_last_life) {
            if(shipY<GAME_Y0) shipY=GAME_Y0;
            nx=shipX; ny=shipY;
            stepx=shipStepX(); stepy=shipStepY();
            thrust_now=0;
            if(inputLeft()&&nx>=stepx)     { nx-=stepx; thrust_now=1; }
            if(inputRight()&&nx<=(SCR_W_BYTES-SHIP_W_BYTES-stepx)) { nx+=stepx; thrust_now=1; }
            if(inputUp()&&ny>=(u8)(GAME_Y0+stepy))   { ny-=stepy; thrust_now=1; }
            if(inputDown()&&ny<=(SCR_H-SHIP_H_PIX-stepy)) ny+=stepy;
            if(ny<GAME_Y0) ny=GAME_Y0;
            if(!rectHitsWall(nx,ny,SHIP_W_BYTES,SHIP_H_PIX)) { shipX=nx; shipY=ny; }
            ship_thrust=thrust_now;
            fire_now=inputFire();
            if(fire_now) fireShot(shipX,shipY);
            spawnWave();
            moveEnemies();
            trySpawnEnemyShots();
        } else ship_thrust=0;

        v9_tickStarsShots();
        tickExplosions();
        collideShotsEnemies();
        collideShipEnemies();
        collideShipEnemyShots();
        tickEnemyShots();
        updateWaveBonus();
        tickBonusHud();
        updateShipThrustAnim();
        updateShipInvulnerability();
        updateShipExplosionState();

        /* Game over */
        if(ship_last_life&&!ship_exploding) {
            if(game_over_delay==0) {
                sfxGameOver();
                game_over_delay=1;
            } else if(game_over_delay<30) {
                game_over_delay++;
                cpct_waitVSYNC();
            } else {
                game_over_delay=0;
                goToMenu(NAU_RESULT_GAMEOVER);
            }
        }

        soundTick();
        flushHUDToBothBuffers();
        drawFrame();
    }
}
