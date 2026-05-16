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
#include "title_logo.h"
#include "shot_sprite.h"
#include "enemy_shot_sprite.h"
#include "dx_bank.h"
#include "dx_assets.h"

#ifndef DX_DEBUG_OVERLAY
#define DX_DEBUG_OVERLAY 0
#endif

#ifndef DX_TEST_FULL_INVUL
#define DX_TEST_FULL_INVUL 1
#endif

#define SCR_W_BYTES 80
#define SCR_H 200
#define TILE_W_BYTES 2
#define TILE_H 8
#define HUD_H 16
#define GAME_Y0 HUD_H
#define GAME_H (SCR_H - HUD_H)
#define SCR_PAGE_C0 ((u8*)0xC000)
#define SCR_PAGE_80 ((u8*)0x8000)
#define MAX_SHOTS 4
#define FIRE_COOLDOWN 1
#define MAX_ENEMIES 5
#define WAVE_PLAN_MAX 8
#define FORM_ROW_DY 10
#define WAVE_MAX_HEAVY_PER_WAVE 3
#define WAVE_MAX_FAST_LOW 2
#define WAVE_MAX_FAST_HIGH 3
#define WAVE_FAST_3_FROM_LEVEL 16
#define WAVE_MODE_RANK 0
#define WAVE_MODE_INDIAN 1
#define ENEMY_SPEED_Y 3
#define ENEMY_SPEED_Y_FAST 5
#define ENEMY_SPEED_Y_HEAVY 2
#define ENEMY_SPEED_Y_DIVER (ENEMY_SPEED_Y_FAST+1u)
#define ENEMY_MIN_X 4
#define ENEMY_MAX_X (76 - ENEMY_W_BYTES)
#define ENEMY_SPACING 2
#define ENEMY_SPAWN_Y (GAME_Y0 + 2)
#define SPAWN_BASE 2
#define SPAWN_VARIANCE 5
#define SPAWN_FIRST_DELAY 4
#define SERIAL_DELAY 8
#define WALLS_PER_ROW 4
#define MAP_W 40
#define MAP_H 25
#define PATT_STRAIGHT 0
#define PATT_ZIGZAG 1
#define PATT_DIAGONAL 2
#define GS_TITLE 0
#define GS_PLAYING 3
#define TS_MENU 0
#define TS_REDEFINE 1
#define TS_HISCORE_VIEW 2
#define TS_HISCORE_INPUT 3
/* Attract: TOP 3 + HOW TO; cicle TS_MENU↔TOP3↔MENU↔HOWTO cada ATTRACT_HOLD_FRAMES. */
#define TS_ATTRACT_SCORE 4
#define TS_WIN 5
#define TS_HELP 6
#define ATTRACT_HOLD_FRAMES 500
#define CTRL_JOYSTICK 0
#define CTRL_KEYBOARD 1
#define RESPAWN_INVUL_TICKS 40
#define MAX_ENEMY_SHOTS 11
#define ENEMYSHOT_SPEED_Y 5
/* Vida extra: cada 5000 punts (sense variable global: es compara quants “blocs” de 5k s’han creuat en aquest addScore). */
#define EXTRA_LIFE_EVERY 5000U
#define BOSS_HP_BASE 12
#define BOSS_HP_PER_TIER 5
#define BOSS_MAX_VY 3
#define BOSS_LANE_W ((u8)(SCR_W_BYTES/3u))
#define BOSS_HOLD_Y ((u8)(GAME_Y0+58))
#define ENEMYSHOT_VX_FAST 2
#define ENEMYSHOT_VX_SLOW 1
#define ENEMYSHOT_COOLDOWN 24
#define ENEMYSHOT_STAGGER 5
#define ENDGAME_FINAL_LEVEL 25
static void enterTitle(void);
static void drawMessageLineColor(u8* v,u8 x,u8 y,const char* s,u8 len,u8 hi);
static cpct_keyID firstPressedKey(void);
static u8 inputLeft(void);
static u8 inputRight(void);
static u8 inputUp(void);
static u8 inputDown(void);
static u8 inputFire(void);
static u8 menuFire(void);
static void drawSolidBoxSafe(u8* v,u8 x,u8 y,u8 w,u8 h,u8 c);
static void triggerShipHit(void);
static void trySpawnEnemyShots(void);
void v9_tickStarsShots(void);
static void drawPxSafe(u8* v,u8 x,u8 y,u8 c);
static const u8 palette_hw[16] = {20,6,30,19,0,25,12,21,28,14,20,20,20,20,20,20};
static const u8 loading_palette_hw[16] = {20,4,28,21,18,23,0,19,13,31,25,31,19,4,23,11};

// Biome palettes - only indices 8 and 9 change (wall tile colors)
// 5 acts × 5 levels: (level-1)/5 → 0..4
static const u8 biome_palettes[5][16] = {
    {20,6,30,19,0,25,12,21,28,14,21,20,20,20,20,20},
    {20,6,30,19,0,25,12,21,12,25,21,20,20,20,20,20},
    {20,6,30,19,0,25,12,21,24,18,21,20,20,20,20,20},
    {20,6,30,19,0,25,12,21,14,4,21,20,20,20,20,20},
    {20,6,30,19,0,25,12,21,5,11,21,20,20,20,20,20}
};
#define B_BG cpct_px2byteM0(0,0)
#define B_WALL_A cpct_px2byteM0(8,8)
#define B_WALL_B cpct_px2byteM0(9,9)
#define B_WALL_C cpct_px2byteM0(8,9)
#define B_WALL_D cpct_px2byteM0(9,8)
#define B_WALL_E cpct_px2byteM0(0,8)
#define B_WALL_F cpct_px2byteM0(8,0)
#define B_STAR1 cpct_px2byteM0(1,1)
#define B_STAR2 cpct_px2byteM0(2,2)
#define B_STAR3 cpct_px2byteM0(4,4)
#define B_SHOT cpct_px2byteM0(6,6)
#define B_HUD_LINE cpct_px2byteM0(10,10) /* ratlles HUD: pen 10 → biome_palettes[][10]=21 */
#define B_LINE_CYAN cpct_px2byteM0(5,5)
#define B_LINE_RED cpct_px2byteM0(2,2)
#define B_DOT_BLUE cpct_px2byteM0(1,1)
#define B_DOT_BBLUE cpct_px2byteM0(3,3)
#define B_EXP_W cpct_px2byteM0(4,4)
#define B_EXP_Y cpct_px2byteM0(2,2)
#define B_EXP_R cpct_px2byteM0(6,6)
#define B_EXP_O cpct_px2byteM0(9,9)
typedef struct { u8 x; u8 y; } TStar; typedef struct { u8 x; u8 y; u8 active; } TShot; typedef struct { u8 x; u8 y; i8 vx; u8 active; u8 cd; u8 vy; } TEnemyShot; typedef struct { u8 x; u8 y; u8 active; i8 vx; u8 vy; u8 pattern; u8 zig_timer; u8 type; u8 health; u8 boss_fire_cd; u8 boss_lane_x0; } TEnemy; typedef struct { u8 x; u8 y; u8 frame; u8 active; u8 kind; } TExplosion;
#define N1 4
#define N2 7
#define N3 11
#define MAX_EXPLOSIONS 6
#define EXP_KIND_ENEMY 0
#define EXP_KIND_SHIP 1

// Enemy types
#define ENEMY_TYPE_BASIC 0
#define ENEMY_TYPE_FAST  1
#define ENEMY_TYPE_HEAVY 2
#define ENEMY_TYPE_DIVER 3
#define ENEMY_TYPE_BOMBER 4
#define ENEMY_TYPE_BOSS   5

// Lookup tables for enemy properties (saves code vs switch/case)
static const u16 enemy_score_tbl[] = {10, 20, 50, 40, 80, 1500};
static const u8 enemy_speed_tbl[] = {ENEMY_SPEED_Y, ENEMY_SPEED_Y_FAST, ENEMY_SPEED_Y_HEAVY, ENEMY_SPEED_Y_DIVER, ENEMY_SPEED_Y, 1};

static u8 dr_buf_ready[2];
static u8 dr_ship_drawn[2],dr_ship_px[2],dr_ship_py[2];
static u8 dr_shot_drawn[2][MAX_SHOTS],dr_shot_px[2][MAX_SHOTS],dr_shot_py[2][MAX_SHOTS];
static u8 dr_emy_drawn[2][MAX_ENEMIES],dr_emy_px[2][MAX_ENEMIES],dr_emy_py[2][MAX_ENEMIES],dr_emy_type[2][MAX_ENEMIES];
static u8 dr_es_drawn[2][MAX_ENEMY_SHOTS],dr_es_px[2][MAX_ENEMY_SHOTS],dr_es_py[2][MAX_ENEMY_SHOTS];
static u8 dr_exp_drawn[2][MAX_EXPLOSIONS],dr_exp_px[2][MAX_EXPLOSIONS],dr_exp_py[2][MAX_EXPLOSIONS];
TStar s1[N1], s2[N2], s3[N3]; TShot shots[MAX_SHOTS]; static TEnemyShot enemyshots[MAX_ENEMY_SHOTS]; TEnemy enemies[MAX_ENEMIES]; static TExplosion explosions[MAX_EXPLOSIONS];
u8 fire_cool=0; static u8 shipX,shipY; u8 wallPhase=0; static u8 wallTiles[8][16];
static const u8 wallX[WALLS_PER_ROW]={0,2,76,78};
static u8* frontBuffer=SCR_PAGE_C0; static u8* backBuffer=SCR_PAGE_80; static u8 frontPage=cpct_pageC0; static u8 backPage=cpct_page80;
static u8 spawn_timer=SPAWN_FIRST_DELAY,wave_total=0,wave_spawned=0,current_wave_mask=0; static u16 current_wave_bonus_base=10; u8 wave_killed=0; static u8 wave_active=0;
static u8 wave_slot_x[WAVE_PLAN_MAX],wave_slot_y[WAVE_PLAN_MAX],wave_slot_type[WAVE_PLAN_MAX];
static u8 wave_unified_patt,wave_mode,wave_indian_delay;
static i8 wave_unified_vx;
static u8 mask_pick_buf[5];
static u8 g_boss_lane_idx;
static u16 score=0; static u8 lives=2; static u8 level=1; static u8 waves_cleared=0; static u8 palette_dirty=0; static char score_text[12]="SCORE 00000"; static char lives_text[8]="LIVES 2"; static char level_text[6]="LV01"; static u8 hud_dirty=1;
/* Sep0 després del score (últim byte score ~34 amb SCORE_X=2); Sep1 després de LVxx (4×3 bytes). */
#define HUD_SEP_X0 36
#define HUD_SEP_X1 52
#define HUD_SCORE_X 2
#define HUD_LEVEL_X 38
#define HUD_LIVES_X 54
static u8 game_state=GS_TITLE,title_dirty=1,title_phase=0,h_acc=0,v_acc=0; static u8 ship_exploding=0,ship_expl_timer=0,ship_thrust=0,ship_thrust_frame=0,ship_thrust_level=0,ship_invul=0,ship_invul_timer=0,game_paused=0,ship_last_life=0;
static u8 blink_ctr=0;
static u16 attract_frm=0;
static u8 attract_cycle=0;

// Hi-Score — Top 3: ordenació per score desc.; empat per nivell assolit (desc.).
// No hi ha persistència entre arrencades (només RAM; es reinicia amb initHiScores).
#define HISCORE_ROW_CHARS 16
typedef struct {
    u16 score;
    u8 level_reached; /* 1..25 en partida; 0 = entrada buida / placeholder */
    u8 name[3];
} THiScore;
static THiScore hiscores[3];  // Restaurat a Top 3
static u8 hs_pos=0;
static u8 hs_input_pos=0;    // Posición del cursor (0,1,2)
static u8 hs_input_char[3];  // Caracteres actuales (A-Z)
#define H_NUM 7
#define H_DEN 4
#define V_NUM 9
#define V_DEN 2
#define TITLE_CREDIT_X 0
#define TITLE_CREDIT_Y ((u8)(SCR_H-HUD_GLYPH_H))
#define TITLE_YEAR_X ((u8)(SCR_W_BYTES-(4u*HUD_GLYPH_ADVANCE)))
static const char menu_credit[]="GAME BY XEVIMET4L";
static const char win_fire_prompt[]="PRESS FIRE TO MENU";
static const char help_title[]="HOW TO PLAY";
static const char help_l1[]="JOY OR ARROWS TO MOVE";
static const char help_l2[]="Z OR FIRE 1 TO SHOOT";
static const char help_l3[]="P TO PAUSE";
static const char help_l4[]="Q OR RETURN TO QUIT";
static const char help_l5[]="EXTRA LIFE EVERY 5000 PTS";
static const char help_l6[]="COMPLETE WAVE FOR BONUS";
static const char help_fire[]="FIRE TO MENU";
static const char menu_opt1[]="1 JOYSTICK"; static const char menu_opt2[]="2 KEYBOARD"; static const char menu_opt3[]="3 SET KEYS"; static const char menu_play[]="FIRE TO START"; static const char redef_title[]="SET KEYS"; static const char redef_msg[]="PRESS A KEY"; static const char redef_left[]="LEFT"; static const char redef_right[]="RIGHT"; static const char redef_up[]="UP"; static const char redef_down[]="DOWN"; static const char redef_fire[]="FIRE"; static const char redef_pause[]="PAUSE"; static const char redef_quit[]="QUIT"; u8 control_mode=CTRL_JOYSTICK,title_screen_mode=TS_MENU,redefine_step=0; static cpct_keyID key_left=Key_CursorLeft,key_right=Key_CursorRight,key_up=Key_CursorUp,key_down=Key_CursorDown,key_fire=Key_Z,key_pause=Key_P,key_quit=Key_Return;
static cpct_keyID firstPressedKey(void){u8 i=10,*keys=cpct_keyboardStatusBuffer+9;u16 keypressed;do{keypressed=((u16)(*keys))^0xFF;if(keypressed)return (cpct_keyID)((keypressed<<8)+(i-1));--keys;}while(--i);return 0;} static u8 inputLeft(void){return (control_mode==CTRL_JOYSTICK)?cpct_isKeyPressed(Joy0_Left):cpct_isKeyPressed(key_left);} static u8 inputRight(void){return (control_mode==CTRL_JOYSTICK)?cpct_isKeyPressed(Joy0_Right):cpct_isKeyPressed(key_right);} static u8 inputUp(void){return (control_mode==CTRL_JOYSTICK)?cpct_isKeyPressed(Joy0_Up):cpct_isKeyPressed(key_up);} static u8 inputDown(void){return (control_mode==CTRL_JOYSTICK)?cpct_isKeyPressed(Joy0_Down):cpct_isKeyPressed(key_down);} static u8 inputFire(void){return (control_mode==CTRL_JOYSTICK)?(cpct_isKeyPressed(Joy0_Fire1)||cpct_isKeyPressed(Joy0_Fire2)||cpct_isKeyPressed(Joy0_Fire3)):cpct_isKeyPressed(key_fire);} static u8 inputPause(void){return (control_mode==CTRL_JOYSTICK)?cpct_isKeyPressed(Key_P):cpct_isKeyPressed(key_pause);} static u8 inputQuit(void){return (control_mode==CTRL_JOYSTICK)?cpct_isKeyPressed(Key_Q):cpct_isKeyPressed(key_quit);} static u8 menuFire(void){return inputFire();} static void resetVideoPages(void){frontBuffer=SCR_PAGE_C0;backBuffer=SCR_PAGE_80;frontPage=cpct_pageC0;backPage=cpct_page80;cpct_setVideoMemoryPage(frontPage);} static void swapVideoPages(void){u8* tp=frontBuffer;u8 tg=frontPage;frontBuffer=backBuffer;frontPage=backPage;backBuffer=tp;backPage=tg;}
static u8 shipStepX(void){u8 s=0;h_acc+=H_NUM;while(h_acc>=H_DEN){h_acc-=H_DEN;++s;}return s;} static u8 shipStepY(void){u8 s=0;v_acc+=V_NUM;while(v_acc>=V_DEN){v_acc-=V_DEN;++s;}return s;}

// Hi-Score - Funcions mínimes optimitzades
static void initHiScores(void){
    /* Tres entrades iguals (300 pts, LV00) amb inicials; només RAM. */
    hiscores[0].score=300;
    hiscores[0].level_reached=0;
    hiscores[0].name[0]='X';hiscores[0].name[1]='M';hiscores[0].name[2]='A';
    hiscores[1].score=300;
    hiscores[1].level_reached=0;
    hiscores[1].name[0]='P';hiscores[1].name[1]='M';hiscores[1].name[2]='H';
    hiscores[2].score=300;
    hiscores[2].level_reached=0;
    hiscores[2].name[0]='C';hiscores[2].name[1]='M';hiscores[2].name[2]='T';
}

static u8 hs_newBeatsEntry(u16 ns,u8 nlv,const THiScore* e){
    if(ns>e->score)return 1;
    if(ns<e->score)return 0;
    return nlv>e->level_reached;
}

static u8 hs_isTop(u16 s,u8 lv){
    u8 i;
    for(i=0;i<3;i++){
        if(hs_newBeatsEntry(s,lv,&hiscores[i]))return i;
    }
    return 255;
}

static void hs_insert(u8 pos,u16 s,u8 lv,const char* name){
    u8 i;
    for(i=2;i>pos;i--){
        hiscores[i].score=hiscores[i-1].score;
        hiscores[i].level_reached=hiscores[i-1].level_reached;
        hiscores[i].name[0]=hiscores[i-1].name[0];
        hiscores[i].name[1]=hiscores[i-1].name[1];
        hiscores[i].name[2]=hiscores[i-1].name[2];
    }
    hiscores[pos].score=s;
    hiscores[pos].level_reached=lv;
    hiscores[pos].name[0]=name[0];
    hiscores[pos].name[1]=name[1];
    hiscores[pos].name[2]=name[2];
}

static void hs_formatRow(char* row,u8 rank,u16 sc,u8 lv,const u8* name){
    row[0]=(char)('1'+rank);
    row[1]=' ';
    row[2]=(char)('0'+(sc/10000u)%10u);
    row[3]=(char)('0'+(sc/1000u)%10u);
    row[4]=(char)('0'+(sc/100u)%10u);
    row[5]=(char)('0'+(sc/10u)%10u);
    row[6]=(char)('0'+sc%10u);
    row[7]=' ';
    row[8]='L';
    row[9]='V';
    row[10]=(char)('0'+(lv/10u)%10u);
    row[11]=(char)('0'+lv%10u);
    row[12]=' ';
    row[13]=(char)name[0];
    row[14]=(char)name[1];
    row[15]=(char)name[2];
    row[16]=0;
}

static void drawHiScoreTable(u8* v, u8 y0){
    u8 i;
    char row[17];
    for(i=0;i<3;i++){
        hs_formatRow(row,i,hiscores[i].score,hiscores[i].level_reached,hiscores[i].name);
        drawMessageLineColor(v, 16, (u8)(y0 + (u8)(i * 14)), row, HISCORE_ROW_CHARS, (i==0)?1:0);
    }
}

// Hi-Score Input - Funciones para entrada de iniciales
static void initHiScoreInput(void){
    u8 i;
    hs_input_pos = 0;
    // Inicializar con '.' como placeholder
    for(i=0;i<3;i++){
        hs_input_char[i] = '.';
    }
}

static void drawHiScoreInput(u8* v){
    u8 i,ry;
    char row[17];
    drawMessageLineColor(v, 28, 48, "GAME OVER", 9, 1);
    cpct_drawSolidBox(cpct_getScreenPtr(v,10,60),B_LINE_RED,60,1);
    drawMessageLineColor(v, 32, 74, "TOP 3", 5, 1);
    cpct_drawSolidBox(cpct_getScreenPtr(v,14,88),B_LINE_CYAN,52,1);
    for(i=0;i<3;i++){
        ry=(u8)(96+(u8)(i*14));
        if(i==hs_pos){
            hs_formatRow(row,i,score,level,(const u8*)hs_input_char);
            drawMessageLineColor(v,16,ry,row,HISCORE_ROW_CHARS,1);
        }else if(i<hs_pos){
            hs_formatRow(row,i,hiscores[i].score,hiscores[i].level_reached,hiscores[i].name);
            drawMessageLineColor(v,16,ry,row,HISCORE_ROW_CHARS,0);
        }else{
            hs_formatRow(row,i,hiscores[i-1u].score,hiscores[i-1u].level_reached,hiscores[i-1u].name);
            drawMessageLineColor(v,16,ry,row,HISCORE_ROW_CHARS,0);
        }
    }
    cpct_drawSolidBox(cpct_getScreenPtr(v,14,142),B_LINE_CYAN,52,1);
}
static void drawHiScoreInputRow(u8* v){
    u8 ry=(u8)(96+(u8)(hs_pos*14));
    u8 ix=(u8)(16+(13*HUD_GLYPH_ADVANCE));
    u8* d=cpct_getScreenPtr(v,ix,ry);
    cpct_drawSprite((u8*)hud_getGlyphHi(hs_input_char[0]),d,HUD_GLYPH_W_BYTES,HUD_GLYPH_H);
    if(HUD_GLYPH_GAP_BYTES)cpct_drawSolidBox(d+HUD_GLYPH_W_BYTES,B_BG,HUD_GLYPH_GAP_BYTES,HUD_GLYPH_H);
    cpct_drawSprite((u8*)hud_getGlyphHi(hs_input_char[1]),d+HUD_GLYPH_ADVANCE,HUD_GLYPH_W_BYTES,HUD_GLYPH_H);
    if(HUD_GLYPH_GAP_BYTES)cpct_drawSolidBox(d+HUD_GLYPH_ADVANCE+HUD_GLYPH_W_BYTES,B_BG,HUD_GLYPH_GAP_BYTES,HUD_GLYPH_H);
    cpct_drawSprite((u8*)hud_getGlyphHi(hs_input_char[2]),d+(2*HUD_GLYPH_ADVANCE),HUD_GLYPH_W_BYTES,HUD_GLYPH_H);
    {
        u8 j,c,pix[2];
        for(j=0;j<3;++j){
            c=(j==hs_input_pos)?15:0;
            pix[0]=pix[1]=c;
            cpct_drawSprite(pix,cpct_getScreenPtr(v,(u8)(ix+j*HUD_GLYPH_ADVANCE),(u8)(ry+HUD_GLYPH_H)),2,1);
        }
    }
}

static void updateHiScoreInput(void){
    static u8 key_cooldown=0;
    cpct_scanKeyboard_f();
    
    if(key_cooldown > 0){
        key_cooldown--;
    } else {
        // Mover cursor izquierda/derecha
        if(inputLeft()){
            if(hs_input_pos > 0){ hs_input_pos--; title_phase = 0; }
            key_cooldown = 8;
        } else if(inputRight()){
            if(hs_input_pos < 2){ hs_input_pos++; title_phase = 0; }
            key_cooldown = 8;
        }
        
        // Cambiar letra arriba/abajo
        if(inputUp()){
            if(hs_input_char[hs_input_pos] == '.'){ hs_input_char[hs_input_pos] = 'A'; }
            else if(hs_input_char[hs_input_pos] == 'Z'){ hs_input_char[hs_input_pos] = '.'; }  // Z -> .
            else if(hs_input_char[hs_input_pos] < 'Z'){ hs_input_char[hs_input_pos]++; }
            title_phase = 0;
            key_cooldown = 8;
        } else if(inputDown()){
            if(hs_input_char[hs_input_pos] == 'A'){ hs_input_char[hs_input_pos] = '.'; }
            else if(hs_input_char[hs_input_pos] == '.'){ hs_input_char[hs_input_pos] = 'Z'; }  // . -> Z
            else if(hs_input_char[hs_input_pos] > '.'){ hs_input_char[hs_input_pos]--; }
            title_phase = 0;
            key_cooldown = 8;
        }
        
        // Aceptar nombre
        if(inputFire()){
            hs_insert(hs_pos,score,level,hs_input_char);
            title_screen_mode = TS_HISCORE_VIEW;
            title_phase = 0;
            key_cooldown = 8;
        }
    }
}
static const u8 rock_pairs[32]={8,8, 8,9, 9,9, 9,8, 0,8, 9,9, 8,0, 8,9, 9,9, 0,8, 8,9, 8,8, 9,8, 8,0, 9,9, 8,9};
static u8 rockByte(u8 idx){u8 i=(u8)((idx&15)<<1);return cpct_px2byteM0(rock_pairs[i],rock_pairs[i+1]);}
static void buildWallTiles(void){u8 p,r,src;for(p=0;p<8;++p)for(r=0;r<8;++r){src=(u8)(((r+p)&7)<<1);wallTiles[p][r*2]=rockByte(src);wallTiles[p][r*2+1]=rockByte(src+1);}}
static u8 levelMapGet(u8 tx,u8 ty){if(tx>=MAP_W||ty>=MAP_H)return 0;return(tx<2||tx>=38)?1:0;}
static u8 inScreen(u8 xb,u8 y){return (xb<SCR_W_BYTES && y<SCR_H);} static u8 tileIsWall(u8 tx,u8 ty){return levelMapGet(tx,ty);}
static void clearGameArea(u8* v,u8 c){u8* p=cpct_getScreenPtr(v,0,GAME_Y0);cpct_drawSolidBox(p,c,64,(u8)GAME_H);cpct_drawSolidBox(cpct_getScreenPtr(p,64,0),c,16,(u8)GAME_H);} 
static u8 rectIntersects(u8 x1,u8 y1,u8 w1,u8 h1,u8 x2,u8 y2,u8 w2,u8 h2){if(x1+w1<=x2)return 0;if(x2+w2<=x1)return 0;if(y1+h1<=y2)return 0;if(y2+h2<=y1)return 0;return 1;}
static u8 rectHitsWall(u8 xb,u8 y,u8 wb,u8 h){u8 tx0=xb/TILE_W_BYTES,ty0=y/TILE_H,tx1=(xb+wb-1)/TILE_W_BYTES,ty1=(y+h-1)/TILE_H,tx,ty;if(tx1>=MAP_W)tx1=MAP_W-1;if(ty1>=MAP_H)ty1=MAP_H-1;for(ty=ty0;ty<=ty1;++ty)for(tx=tx0;tx<=tx1;++tx)if(tileIsWall(tx,ty))return 1;return 0;}
static u8 screenX2TileX(u8 xb){return xb/TILE_W_BYTES;} static u8 screenY2TileY(u8 y){return y/TILE_H;} static u8 isCorridorPos(u8 xb,u8 y){u8 tx=screenX2TileX(xb),ty=screenY2TileY(y);if(tx>=MAP_W||ty>=MAP_H)return 0;if(y<GAME_Y0)return 0;return tileIsWall(tx,ty)?0:1;} static void randomCorridorPos(TStar* s){u8 t=0;while(t<64){u8 x=cpct_rand(),y=(u8)(GAME_Y0+(cpct_rand()%GAME_H));if(x<SCR_W_BYTES&&y<SCR_H&&isCorridorPos(x,y)){s->x=x;s->y=y;return;}++t;}s->x=(SCR_W_BYTES>>1);s->y=(GAME_Y0+(GAME_H>>1));}
static void initStars(TStar* s,u8 n){u8 i;for(i=0;i<n;++i)randomCorridorPos(&s[i]);} 
static void drawStars(u8* v,TStar* s,u8 n,u8 b){
    u8 i;
    for(i=0;i<n;++i){
        if(s[i].x<SCR_W_BYTES && s[i].y<SCR_H && s[i].y>=GAME_Y0){
            *cpct_getScreenPtr(v,s[i].x,s[i].y)=b;
        }
    }
}
static void drawMap(u8* v){u8 ty,c;u8 phase;u8* tile;for(ty=GAME_Y0/TILE_H;ty<MAP_H;++ty){phase=(u8)((wallPhase+ty)&7);tile=wallTiles[phase];for(c=0;c<WALLS_PER_ROW;++c)cpct_drawTileAligned2x8_f(tile,cpct_getScreenPtr(v,wallX[c],(u8)(ty*TILE_H)));}}
static void syncHUDText(void){if(!hud_dirty)return;hud_updateScoreText(score,score_text);hud_updateLevelText(level,level_text);hud_updateLivesText(lives,lives_text);hud_dirty=0;}
static void drawHUDSeparators(u8* v){cpct_drawSolidBox(cpct_getScreenPtr(v,HUD_SEP_X0,0),B_HUD_LINE,1,HUD_H);cpct_drawSolidBox(cpct_getScreenPtr(v,HUD_SEP_X1,0),B_HUD_LINE,1,HUD_H);}
static u8* drawHudGlyphByte(u8* d,const u8* spr){cpct_drawSprite((u8*)spr,d,HUD_GLYPH_W_BYTES,HUD_GLYPH_H);if(HUD_GLYPH_GAP_BYTES)cpct_drawSolidBox(d+HUD_GLYPH_W_BYTES,B_BG,HUD_GLYPH_GAP_BYTES,HUD_GLYPH_H);return d+HUD_GLYPH_ADVANCE;}
static void drawHUDScore(u8* v){u8 i;u8* d=cpct_getScreenPtr(v,HUD_SCORE_X,4);for(i=0;i<11;++i)d=drawHudGlyphByte(d,hud_getGlyph(score_text[i]));}
static void drawHUDLives(u8* v){u8 i;u8* d=cpct_getScreenPtr(v,HUD_LIVES_X,4);cpct_drawSolidBox(cpct_getScreenPtr(v,HUD_LIVES_X,4),B_BG,24,8);for(i=0;i<7;++i)d=drawHudGlyphByte(d,hud_getGlyph(lives_text[i]));}
static void drawHUDLevel(u8* v){u8 i;u8* d=cpct_getScreenPtr(v,HUD_LEVEL_X,4);for(i=0;i<4;++i)d=drawHudGlyphByte(d,hud_getGlyph(level_text[i]));}
static void drawHUD(u8* v){syncHUDText();cpct_drawSolidBox(v,B_BG,64,HUD_H);cpct_drawSolidBox(cpct_getScreenPtr(v,64,0),B_BG,16,HUD_H);drawHUDSeparators(v);cpct_drawSolidBox(cpct_getScreenPtr(v,0,HUD_H-1),B_HUD_LINE,64,1);cpct_drawSolidBox(cpct_getScreenPtr(v,64,HUD_H-1),B_HUD_LINE,16,1);drawHUDScore(v);drawHUDLevel(v);drawHUDLives(v);}
#define initHUD() do{hud_dirty=1;}while(0)
#define resetGameplayState() do{respawnShip();initShots();initEnemies();initExplosions();initEnemyShots();}while(0)
static void flushHUDToBothBuffers(void){if(!hud_dirty)return;drawHUD(SCR_PAGE_C0);drawHUD(SCR_PAGE_80);}
void addScore(u16 p){u16 os=score;u16 ns=(u16)(score+p);u8 hi,lo;if(ns<score)score=65535;else score=ns;hi=(u8)(score/EXTRA_LIFE_EVERY);lo=(u8)(os/EXTRA_LIFE_EVERY);for(;hi>lo;++lo){++lives;hud_dirty=1;sfxBeep();}hud_dirty=1;} static void drawShipAt(u8* v,u8 x,u8 y){if(!inScreen(x,y)||y<GAME_Y0)return;cpct_drawSpriteMasked(ship_masked,cpct_getScreenPtr(v,x,y),SHIP_W_BYTES,SHIP_H_PIX);} static void drawShipThrusters(u8* v,u8 x,u8 y){u8 lx,rx,fy,c0,c1,c2;if(!ship_thrust_level)return;lx=(u8)(x+1);rx=(u8)(x+4);fy=(u8)(y+(SHIP_H_PIX-1));if(ship_thrust_frame&1){c0=B_EXP_W;c1=B_EXP_Y;c2=B_EXP_O;}else{c0=B_EXP_Y;c1=B_EXP_W;c2=B_EXP_O;}if(ship_thrust_level>=1){drawPxSafe(v,lx,fy,c0);drawPxSafe(v,rx,fy,c0);}if(ship_thrust_level>=2){drawPxSafe(v,lx,(u8)(fy+1),c1);drawPxSafe(v,rx,(u8)(fy+1),c1);}if(ship_thrust_level>=3){drawPxSafe(v,lx,(u8)(fy+2),c2);drawPxSafe(v,rx,(u8)(fy+2),c2);}if(ship_thrust_level>=4){drawPxSafe(v,lx,(u8)(fy+3),B_EXP_O);drawPxSafe(v,rx,(u8)(fy+3),B_EXP_O);}if(ship_thrust_level>=5){drawPxSafe(v,lx,(u8)(fy+4),B_EXP_R);drawPxSafe(v,rx,(u8)(fy+4),B_EXP_R);}} static void drawEnemyShots(u8* v){u8 i;for(i=0;i<MAX_ENEMY_SHOTS;++i)if(enemyshots[i].active&&inScreen(enemyshots[i].x,enemyshots[i].y)&&enemyshots[i].y>=GAME_Y0)cpct_drawSpriteMasked(enemyshot_getSprite(ENEMYSHOT_TYPE_BULLET),cpct_getScreenPtr(v,enemyshots[i].x,enemyshots[i].y),ENEMYSHOT_W_BYTES,ENEMYSHOT_H_PIX);} static void drawEnemies(u8* v){u8 i;for(i=0;i<MAX_ENEMIES;++i)if(enemies[i].active&&enemies[i].y>=GAME_Y0){if(enemies[i].type==ENEMY_TYPE_BOSS){cpct_drawSpriteMasked(boss_masked,cpct_getScreenPtr(v,enemies[i].x,enemies[i].y),BOSS_W_BYTES,BOSS_H_PIX);}else{static u8* const emspr[]={enemy_masked,enemy_fast_masked,enemy_heavy_masked,enemy_diver_masked,enemy_bomber_masked};cpct_drawSpriteMasked((enemies[i].type<5)?emspr[enemies[i].type]:enemy_masked,cpct_getScreenPtr(v,enemies[i].x,enemies[i].y),ENEMY_W_BYTES,ENEMY_H_PIX);}}} static void initShots(void){u8 i;for(i=0;i<MAX_SHOTS;++i)shots[i].active=0;fire_cool=0;} static void initEnemyShots(void){u8 i;for(i=0;i<MAX_ENEMY_SHOTS;++i){enemyshots[i].active=0;enemyshots[i].x=0;enemyshots[i].y=0;enemyshots[i].vx=0;enemyshots[i].cd=(u8)(i*ENEMYSHOT_STAGGER);}} static void fireShot(u8 sx,u8 sy){u8 i;if(fire_cool||ship_exploding||ship_last_life)return;for(i=0;i<MAX_SHOTS;++i)if(!shots[i].active){shots[i].active=1;shots[i].x=(u8)(sx+(SHIP_W_BYTES/2)-(SHOT_W_BYTES/2));shots[i].y=sy;fire_cool=FIRE_COOLDOWN;sfxShot();return;}} static void drawShots(u8* v){u8 i;for(i=0;i<MAX_SHOTS;++i)if(shots[i].active&&inScreen(shots[i].x,shots[i].y)&&shots[i].y>=GAME_Y0)cpct_drawSpriteMasked(shot_getSprite(SHOT_TYPE_SINGLE),cpct_getScreenPtr(v,shots[i].x,shots[i].y),SHOT_W_BYTES,SHOT_H_PIX);} static u8 bossTierFromLevel(u8 lv){if(lv>=25u)return 4u;return(u8)(lv/5u-1u);} static u8 bossBulletVyFromTier(u8 t){u16 v=(u16)ENEMYSHOT_SPEED_Y+(u16)t;if(t>=3u)v++;if(v>11u)return 11u;return(u8)v;} static u8 allocEnemyShotSlot(void){u8 k;for(k=0;k<MAX_ENEMY_SHOTS;++k)if(!enemyshots[k].active)return k;return 255;} static i8 enemyShotAimVX(u8 sx){i16 dx=(i16)shipX-(i16)sx;if(dx<=-12)return -ENEMYSHOT_VX_FAST;if(dx<=-4)return -ENEMYSHOT_VX_SLOW;if(dx>=12)return ENEMYSHOT_VX_FAST;if(dx>=4)return ENEMYSHOT_VX_SLOW;return 0;} static void trySpawnEnemyShots(void){u8 i,k,s,bx,bvy,tier,cd;i16 xx;tier=bossTierFromLevel(level);for(i=0;i<MAX_ENEMIES;++i){if(!enemies[i].active||enemies[i].y<GAME_Y0)continue;if(enemies[i].type==ENEMY_TYPE_BOSS){if(enemies[i].boss_fire_cd){--enemies[i].boss_fire_cd;continue;}bvy=bossBulletVyFromTier(tier);if(tier>=3u){for(s=0;s<3u;++s){k=allocEnemyShotSlot();if(k==255)break;bx=(u8)(enemies[i].x+4u);if(s==0u)xx=(i16)bx-3;else if(s==1u)xx=(i16)bx;else xx=(i16)bx+3;if(xx<(i16)ENEMY_MIN_X)xx=(i16)ENEMY_MIN_X;if(xx>(i16)(SCR_W_BYTES-ENEMYSHOT_W_BYTES))xx=(i16)(SCR_W_BYTES-ENEMYSHOT_W_BYTES);enemyshots[k].active=1;enemyshots[k].x=(u8)xx;enemyshots[k].y=(u8)(enemies[i].y+BOSS_H_PIX);enemyshots[k].vx=enemyShotAimVX((u8)xx);enemyshots[k].vy=bvy;enemyshots[k].cd=0;}}else{for(s=0;s<2u;++s){k=allocEnemyShotSlot();if(k==255)break;bx=(u8)(enemies[i].x+4u);xx=(i16)bx+(s?(i16)3:(i16)-3);if(xx<(i16)ENEMY_MIN_X)xx=(i16)ENEMY_MIN_X;if(xx>(i16)(SCR_W_BYTES-ENEMYSHOT_W_BYTES))xx=(i16)(SCR_W_BYTES-ENEMYSHOT_W_BYTES);enemyshots[k].active=1;enemyshots[k].x=(u8)xx;enemyshots[k].y=(u8)(enemies[i].y+BOSS_H_PIX);enemyshots[k].vx=enemyShotAimVX((u8)xx);enemyshots[k].vy=bvy;enemyshots[k].cd=0;}}cd=(u8)(4u+(4u-tier)+(cpct_rand()&3u));if(cd<4u)cd=4u;enemies[i].boss_fire_cd=cd;continue;}if(enemyshots[i].active||enemyshots[i].cd)continue;enemyshots[i].active=1;enemyshots[i].x=(u8)(enemies[i].x+2);enemyshots[i].y=(u8)(enemies[i].y+ENEMY_H_PIX);enemyshots[i].vx=enemyShotAimVX(enemies[i].x);enemyshots[i].vy=ENEMYSHOT_SPEED_Y;enemyshots[i].cd=(u8)(ENEMYSHOT_COOLDOWN+(i<<1));}} static void tickEnemyShots(void){u8 i,sp;for(i=0;i<MAX_ENEMY_SHOTS;++i){i16 nx;if(enemyshots[i].cd)--enemyshots[i].cd;if(!enemyshots[i].active)continue;sp=enemyshots[i].vy;if(!sp)sp=ENEMYSHOT_SPEED_Y;if(enemyshots[i].y>=(u8)(SCR_H-sp)){enemyshots[i].active=0;continue;}enemyshots[i].y=(u8)(enemyshots[i].y+sp);nx=(i16)enemyshots[i].x+(i16)enemyshots[i].vx;if(nx<0||nx>=SCR_W_BYTES){enemyshots[i].active=0;continue;}enemyshots[i].x=(u8)nx;}} static void collideShipEnemyShots(void){u8 i;if(ship_exploding||ship_invul||ship_last_life)return;for(i=0;i<MAX_ENEMY_SHOTS;++i){if(!enemyshots[i].active)continue;if(rectIntersects(enemyshots[i].x,enemyshots[i].y,ENEMYSHOT_W_BYTES,ENEMYSHOT_H_PIX,shipX,shipY,SHIP_W_BYTES,SHIP_H_PIX)){enemyshots[i].active=0;triggerShipHit();return;}}}
static void clearAllEnemies(void){u8 i;for(i=0;i<MAX_ENEMIES;++i)enemies[i].active=0;wave_active=0;} static void clearAllShots(void){u8 i;for(i=0;i<MAX_SHOTS;++i)shots[i].active=0;fire_cool=0;} static void clearAllExplosions(void){u8 i;for(i=0;i<MAX_EXPLOSIONS;++i)explosions[i].active=0;} static void placeShipAtSpawn(void){shipX=(SCR_W_BYTES-SHIP_W_BYTES)>>1;shipY=(u8)(SCR_H-SHIP_H_PIX-2);h_acc=0;v_acc=0;} static void resetShipRuntimeState(void){ship_exploding=0;ship_expl_timer=0;ship_thrust=0;ship_thrust_frame=0;ship_thrust_level=0;ship_invul=0;ship_invul_timer=0;ship_last_life=0;} static void setShipRespawnInvul(void){ship_invul=1;ship_invul_timer=RESPAWN_INVUL_TICKS;} static void respawnShip(void){resetShipRuntimeState();placeShipAtSpawn();setShipRespawnInvul();} static void clearActiveCombatState(void){clearAllEnemies();clearAllShots();initEnemyShots();} static void initEnemies(void){u8 i;for(i=0;i<MAX_ENEMIES;++i){enemies[i].active=0;enemies[i].vy=ENEMY_SPEED_Y;enemies[i].vx=0;enemies[i].pattern=PATT_STRAIGHT;enemies[i].zig_timer=6;enemies[i].boss_fire_cd=0;enemies[i].boss_lane_x0=0;}spawn_timer=SPAWN_FIRST_DELAY;wave_total=0;wave_spawned=0;wave_killed=0;wave_active=0;current_wave_mask=0;current_wave_bonus_base=10;wave_mode=WAVE_MODE_RANK;wave_indian_delay=0;} static void initExplosions(void){clearAllExplosions();}
static void resetGameSession(void){score=0;lives=2;level=1;waves_cleared=0;hud_dirty=1;resetGameplayState();} void spawnExplosion(u8 x,u8 y,u8 kind){u8 i=MAX_EXPLOSIONS;while(i--)if(!explosions[i].active){explosions[i].active=1;explosions[i].x=x;explosions[i].y=y;explosions[i].frame=0;explosions[i].kind=kind;return;}} static void tickExplosions(void){u8 i=MAX_EXPLOSIONS;while(i--){u8 maxf;if(!explosions[i].active)continue;maxf=(explosions[i].kind==EXP_KIND_SHIP)?6:4;++explosions[i].frame;if(explosions[i].frame>=maxf)explosions[i].active=0;}} static void drawSolidBoxSafe(u8* v,u8 x,u8 y,u8 w,u8 h,u8 c){if(y>=SCR_H||x>=SCR_W_BYTES||y<GAME_Y0)return;if(x+w>SCR_W_BYTES)w=(u8)(SCR_W_BYTES-x);if(y+h>SCR_H)h=(u8)(SCR_H-y);if(!w||!h)return;cpct_drawSolidBox(cpct_getScreenPtr(v,x,y),c,w,h);} static void drawPxSafe(u8* v,u8 x,u8 y,u8 c){if(y<GAME_Y0||y>=SCR_H||x>=SCR_W_BYTES)return;*cpct_getScreenPtr(v,x,y)=c;} static void drawRadialBurst(u8* v,u8 cx,u8 cy,u8 r,u8 core,u8 mid,u8 outer){u8 l=(cx>r)?(u8)(cx-r):0,t=(cy>r)?(u8)(cy-r):GAME_Y0;drawSolidBoxSafe(v,cx,cy,2,2,core);if(r>=1){drawSolidBoxSafe(v,(cx>0)?(u8)(cx-1):0,cy,4,2,mid);drawSolidBoxSafe(v,cx,(cy>0)?(u8)(cy-1):GAME_Y0,2,4,mid);}if(r>=2){drawSolidBoxSafe(v,(cx>1)?(u8)(cx-2):0,cy,6,2,outer);drawSolidBoxSafe(v,cx,(cy>1)?(u8)(cy-2):GAME_Y0,2,6,outer);drawSolidBoxSafe(v,(cx>1)?(u8)(cx-1):0,(cy>1)?(u8)(cy-1):GAME_Y0,4,4,mid);}if(r>=3){drawSolidBoxSafe(v,l,cy,8,2,outer);drawSolidBoxSafe(v,cx,t,2,8,outer);drawSolidBoxSafe(v,(cx>2)?(u8)(cx-2):0,(cy>2)?(u8)(cy-2):GAME_Y0,6,6,mid);} } static void drawEnemyExplosion(u8* v,TExplosion* e){u8 cx=e->x+1,cy=e->y+2;switch(e->frame){case 0:drawRadialBurst(v,cx,cy,1,B_EXP_W,B_EXP_Y,B_EXP_R);break;case 1:drawRadialBurst(v,cx,cy,2,B_EXP_W,B_EXP_Y,B_EXP_O);break;case 2:drawRadialBurst(v,cx,cy,3,B_EXP_Y,B_EXP_O,B_EXP_O);break;default:drawRadialBurst(v,cx,cy,2,B_EXP_W,B_EXP_O,B_EXP_O);break;}} static void drawShipExplosion(u8* v,TExplosion* e){u8 cx=e->x+2,cy=e->y+3;switch(e->frame){case 0:drawRadialBurst(v,cx,cy,1,B_EXP_W,B_EXP_Y,B_EXP_R);break;case 1:drawRadialBurst(v,cx,cy,2,B_EXP_W,B_EXP_Y,B_EXP_O);break;case 2:drawRadialBurst(v,cx,cy,3,B_EXP_W,B_EXP_R,B_EXP_O);break;case 3:drawRadialBurst(v,cx,cy,4,B_EXP_Y,B_EXP_O,B_EXP_O);break;case 4:drawRadialBurst(v,cx,cy,3,B_EXP_W,B_EXP_O,B_EXP_O);break;default:drawRadialBurst(v,cx,cy,2,B_EXP_W,B_EXP_Y,B_EXP_O);break;}} static void drawExplosions(u8* v){u8 i=MAX_EXPLOSIONS;while(i--){if(!explosions[i].active)continue;if(explosions[i].kind==EXP_KIND_SHIP)drawShipExplosion(v,&explosions[i]);else drawEnemyExplosion(v,&explosions[i]);}} static void updateShipThrustAnim(void){if(ship_thrust){if(ship_thrust_level<5)++ship_thrust_level;++ship_thrust_frame;}else{if(ship_thrust_level)--ship_thrust_level;if(ship_thrust_level)++ship_thrust_frame;else ship_thrust_frame=0;}} static void updateShipInvulnerability(void){
#if DX_TEST_FULL_INVUL
ship_invul=1;ship_invul_timer=1;return;
#endif
if(!ship_invul)return;if(ship_invul_timer)--ship_invul_timer;if(!ship_invul_timer)ship_invul=0;} static void triggerShipHit(void){spawnExplosion(shipX,shipY,EXP_KIND_SHIP);if(lives==0)ship_last_life=1;else{--lives;hud_dirty=1;}ship_exploding=1;ship_expl_timer=12;ship_thrust=0;ship_thrust_level=0;clearActiveCombatState();sfxShipExplosion();} static void updateShipExplosionState(void){if(!ship_exploding)return;if(ship_expl_timer)--ship_expl_timer;if(ship_expl_timer)return;if(ship_last_life){ship_exploding=0;return;}respawnShip();}
static u8 enemiesAlive(void){u8 i=MAX_ENEMIES;while(i--)if(enemies[i].active)return 1;return 0;} static i8 patternInitialVX(u8 patt){if(patt==PATT_ZIGZAG)return 1;if(patt==PATT_DIAGONAL)return (cpct_rand()&1)?1:-1;return 0;} static void spawnSingleEnemy(u8 x,u8 y,u8 patt,i8 vx,u8 enemy_type){u8 i=MAX_ENEMIES;while(i--)if(!enemies[i].active){u8 tier;enemies[i].active=1;enemies[i].x=x;enemies[i].y=y;enemies[i].vx=vx;enemies[i].pattern=patt;enemies[i].zig_timer=6;enemies[i].type=enemy_type;if(enemy_type==ENEMY_TYPE_BOSS){u8 tw,laneL;tier=bossTierFromLevel(level);enemies[i].health=(u8)(BOSS_HP_BASE+tier*BOSS_HP_PER_TIER);enemies[i].vy=(u8)(1u+tier/2u);if(enemies[i].vy>BOSS_MAX_VY)enemies[i].vy=BOSS_MAX_VY;enemies[i].boss_fire_cd=(u8)(8u+(cpct_rand()&15u));tw=BOSS_LANE_W;laneL=(u8)(g_boss_lane_idx*tw);if(laneL<ENEMY_MIN_X)laneL=ENEMY_MIN_X;if((u16)laneL+tw>(u16)SCR_W_BYTES)laneL=(u8)((u16)SCR_W_BYTES-tw);enemies[i].boss_lane_x0=laneL;enemies[i].x=(u8)(laneL+(tw-BOSS_W_BYTES)/2u);enemies[i].zig_timer=(u8)(8u+(cpct_rand()&7u));}else{enemies[i].vy=(enemy_type<6)?enemy_speed_tbl[enemy_type]:ENEMY_SPEED_Y;enemies[i].health=(enemy_type==ENEMY_TYPE_HEAVY||enemy_type==ENEMY_TYPE_BOMBER)?3:1;enemies[i].boss_fire_cd=0;enemies[i].boss_lane_x0=0;}enemyshots[i].cd=(u8)((i*3)+(cpct_rand()&7));return;}}
static u8 countActiveEnemies(void){u8 i,c=0;for(i=0;i<MAX_ENEMIES;++i)if(enemies[i].active)++c;return c;}
static u8 formationRowTw(u8 cnt){return (u8)((u16)cnt*ENEMY_W_BYTES+(u16)(cnt-1u)*ENEMY_SPACING);}
static void buildWaveSlotLayoutRank(u8 n){
  u8 r0,r1,twb,sxb,sxf,twf,avail,col,idx,yb,maxsx;
  if(n<=3u){r0=n;r1=0;}
  else if(n==4u){r0=2;r1=2;}
  else if(n==5u){r0=3;r1=2;}
  else if(n==6u){r0=3;r1=3;}
  else if(n==7u){r0=4;r1=3;}
  else{r0=5;r1=3;}
  twb=formationRowTw(r0);avail=(u8)((ENEMY_MAX_X-ENEMY_MIN_X+1u)-twb+1u);sxb=ENEMY_MIN_X;if(avail>1u)sxb=(u8)(ENEMY_MIN_X+(cpct_rand()%avail));
  idx=0;yb=ENEMY_SPAWN_Y;
  for(col=0;col<r0;++col){wave_slot_x[idx]=(u8)(sxb+col*(ENEMY_W_BYTES+ENEMY_SPACING));wave_slot_y[idx]=yb;++idx;}
  if(!r1)return;
  twf=formationRowTw(r1);sxf=(u8)(sxb+(u8)((twb-twf)>>1));avail=(u8)((ENEMY_MAX_X-ENEMY_MIN_X+1u)-twf+1u);
  maxsx=(u8)(ENEMY_MIN_X+((avail>0u)?(u8)(avail-1u):0u));if(sxf<ENEMY_MIN_X)sxf=ENEMY_MIN_X;if(sxf>maxsx)sxf=maxsx;
  yb=(u8)(yb+FORM_ROW_DY);
  for(col=0;col<r1;++col){wave_slot_x[idx]=(u8)(sxf+col*(ENEMY_W_BYTES+ENEMY_SPACING));wave_slot_y[idx]=yb;++idx;}
}
static void buildWaveSlotLayoutIndian(u8 n){u8 i,tw,avail,sx;tw=ENEMY_W_BYTES;avail=(u8)((ENEMY_MAX_X-ENEMY_MIN_X+1u)-tw+1u);sx=ENEMY_MIN_X;if(avail>1u)sx=(u8)(ENEMY_MIN_X+(cpct_rand()%avail));for(i=0;i<n;++i){wave_slot_x[i]=sx;wave_slot_y[i]=ENEMY_SPAWN_Y;}}
static void buildWaveSlotLayoutFastV3(void){u8 sxf,twf,avail,step,yb;step=(u8)(ENEMY_W_BYTES+ENEMY_SPACING);twf=formationRowTw(2);avail=(u8)((ENEMY_MAX_X-ENEMY_MIN_X+1u)-twf+1u);sxf=ENEMY_MIN_X;if(avail>1u)sxf=(u8)(ENEMY_MIN_X+(cpct_rand()%avail));yb=(u8)(ENEMY_SPAWN_Y+FORM_ROW_DY);wave_slot_x[1]=sxf;wave_slot_y[1]=yb;wave_slot_x[2]=(u8)(sxf+step);wave_slot_y[2]=yb;wave_slot_x[0]=(u8)(sxf+(u8)((twf-ENEMY_W_BYTES)>>1));wave_slot_y[0]=ENEMY_SPAWN_Y;if(wave_slot_x[0]<ENEMY_MIN_X)wave_slot_x[0]=ENEMY_MIN_X;if(wave_slot_x[0]>ENEMY_MAX_X)wave_slot_x[0]=ENEMY_MAX_X;}
/* Layout especial para FAST en modo RANK, pero lo más "barato" posible:
 * reutilizamos el layout genérico y hacemos ajustes mínimos solo donde importa.
 */
static void buildWaveSlotLayoutFastRank(u8 n){
  if(n==3u){
    buildWaveSlotLayoutFastV3();
    return;
  }

  buildWaveSlotLayoutRank(n);

  if(n==4u){
    /* Rombo: evitar 2+2 plano desplazando la fila inferior */
    u8 shift=(u8)((ENEMY_W_BYTES>>1)+(ENEMY_SPACING>>1)); /* 3+1=4 */
    /* rank(4): [0..1] arriba, [2..3] abajo */
    if(wave_slot_x[3]+shift<=ENEMY_MAX_X){
      wave_slot_x[2]+=shift;
      wave_slot_x[3]+=shift;
    }else if(wave_slot_x[2]>=ENEMY_MIN_X+shift){
      wave_slot_x[2]-=shift;
      wave_slot_x[3]-=shift;
    }
    return;
  }

  if(n==5u){
    /* 2+3: swap de filas (top=2 desde la fila inferior original) */
    wave_slot_y[0]=ENEMY_SPAWN_Y+FORM_ROW_DY;
    wave_slot_y[1]=ENEMY_SPAWN_Y+FORM_ROW_DY;
    wave_slot_y[2]=ENEMY_SPAWN_Y+FORM_ROW_DY;
    wave_slot_y[3]=ENEMY_SPAWN_Y;
    wave_slot_y[4]=ENEMY_SPAWN_Y;
    return;
  }
}
/* Per sota del nivell 4 no hi ha FAST al pool. Del 4 al 15 el límit el marca `per_wave`
 * (no capar a 2: sinó onades de 3–7 enemics mai poden ser totes fast encara que el mask ho permeti).
 * A partir del 16 es torna a limitar per la formació FastV3 i el ritme. */
static u8 waveMaxFastAllowed(void){
  if(level>=WAVE_FAST_3_FROM_LEVEL)return WAVE_MAX_FAST_HIGH;
  if(level>=4u)return WAVE_PLAN_MAX;
  return WAVE_MAX_FAST_LOW;
}
static u8 pickHomogeneousTypeForWave(u8 mask,u8 total){u8 nc=0,b,tt,mf;if(!mask)return ENEMY_TYPE_BASIC;mf=waveMaxFastAllowed();for(b=0;b<5;++b){if(!(mask&(1u<<b)))continue;tt=(u8)b;if(tt==ENEMY_TYPE_FAST&&total>mf)continue;if(tt==ENEMY_TYPE_HEAVY&&total>WAVE_MAX_HEAVY_PER_WAVE)continue;mask_pick_buf[nc++]=tt;}if(!nc)return ENEMY_TYPE_BASIC;return mask_pick_buf[cpct_rand()%nc];}
/* Una sola LMASK_*: onada homogènia sense aplicar límits fast/heavy (intro pre-boss). */
static u8 waveTypeFromSingleLmask(u8 m){u8 b;if(!m)return ENEMY_TYPE_BASIC;if(m&(m-1u))return (u8)255;for(b=0;b<5;++b)if(m==(u8)(1u<<b))return b;return (u8)255;}
static void buildWaveTypePlan(u8 total,u8 mask){u8 slot,t,st;st=waveTypeFromSingleLmask(mask);if(st!=(u8)255){t=st;for(slot=0;slot<total;++slot)wave_slot_type[slot]=t;current_wave_bonus_base=(t<5u)?enemy_score_tbl[t]:10u;return;}t=pickHomogeneousTypeForWave(mask,total);for(slot=0;slot<total;++slot)wave_slot_type[slot]=t;current_wave_bonus_base=(t<5u)?enemy_score_tbl[t]:10u;}
static void buildWaveLayoutAfterType(u8 n){u8 t=wave_slot_type[0];if(wave_mode==WAVE_MODE_INDIAN)buildWaveSlotLayoutIndian(n);else if(t==ENEMY_TYPE_FAST)buildWaveSlotLayoutFastRank(n);else buildWaveSlotLayoutRank(n);}
static void getPatternForType(u8 enemy_type,u8* patt,i8* vx){
  if(enemy_type==ENEMY_TYPE_BASIC){*patt=(u8)(cpct_rand()%3);*vx=patternInitialVX(*patt);}
  else if(enemy_type==ENEMY_TYPE_FAST){*patt=(cpct_rand()&1u)?PATT_DIAGONAL:PATT_ZIGZAG;*vx=patternInitialVX(*patt);}
  else if(enemy_type==ENEMY_TYPE_HEAVY||enemy_type==ENEMY_TYPE_BOMBER){*patt=PATT_STRAIGHT;*vx=(cpct_rand()&1u)?(i8)1:(i8)-1;}
  else if(enemy_type==ENEMY_TYPE_DIVER){*patt=PATT_DIAGONAL;*vx=patternInitialVX(*patt);}
  else{*patt=PATT_STRAIGHT;*vx=0;}
}
static void pickWaveUnifiedMovement(void){getPatternForType(wave_slot_type[0],&wave_unified_patt,&wave_unified_vx);}
static void spawnOneFromQueue(void){u8 slot,t,x,y;if(wave_spawned>=wave_total)return;slot=wave_spawned;t=wave_slot_type[slot];x=wave_slot_x[slot];y=wave_slot_y[slot];spawnSingleEnemy(x,y,wave_unified_patt,wave_unified_vx,t);++wave_spawned;}
static void tryWaveSpawnTopup(void){
  if(wave_mode==WAVE_MODE_INDIAN){
    if(wave_indian_delay){--wave_indian_delay;return;}
    if(wave_spawned<wave_total&&countActiveEnemies()<MAX_ENEMIES){
      spawnOneFromQueue();
      if(wave_spawned<wave_total)wave_indian_delay=SERIAL_DELAY;
    }
    return;
  }
  while(wave_spawned<wave_total&&countActiveEnemies()<MAX_ENEMIES)spawnOneFromQueue();
}
static void startNewWave(void){const TLevelConfig* cfg;u8 wave_mask;cfg=level_config_get(level);wave_killed=0;wave_active=1;wave_spawned=0;wave_indian_delay=0;if(cfg->flags&LCFG_F_BOSS2){wave_total=2;current_wave_mask=0;current_wave_bonus_base=0;g_boss_lane_idx=(u8)(cpct_rand()%3u);spawnSingleEnemy(0,ENEMY_SPAWN_Y,PATT_STRAIGHT,(i8)1,ENEMY_TYPE_BOSS);g_boss_lane_idx=(u8)((g_boss_lane_idx+1u)%3u);spawnSingleEnemy(0,ENEMY_SPAWN_Y,PATT_STRAIGHT,(i8)1,ENEMY_TYPE_BOSS);wave_spawned=2;spawn_timer=(u8)(SPAWN_BASE+(cpct_rand()&SPAWN_VARIANCE));return;}if(cfg->flags&LCFG_F_BOSS1){wave_total=1;current_wave_mask=0;current_wave_bonus_base=0;g_boss_lane_idx=(u8)(cpct_rand()%3u);spawnSingleEnemy(0,ENEMY_SPAWN_Y,PATT_STRAIGHT,(i8)1,ENEMY_TYPE_BOSS);wave_spawned=1;spawn_timer=(u8)(SPAWN_BASE+(cpct_rand()&SPAWN_VARIANCE));return;}wave_total=cfg->per_wave;if(wave_total>WAVE_PLAN_MAX)wave_total=WAVE_PLAN_MAX;wave_mask=cfg->mask;if(cfg->intro_mask&&waves_cleared==0u)wave_mask=cfg->intro_mask;current_wave_mask=wave_mask;wave_mode=((cpct_rand()&3u)==0u)?WAVE_MODE_INDIAN:WAVE_MODE_RANK;buildWaveTypePlan(wave_total,wave_mask);if(wave_slot_type[0]==ENEMY_TYPE_DIVER)wave_mode=WAVE_MODE_INDIAN;if(wave_slot_type[0]==ENEMY_TYPE_HEAVY&&wave_total>3u)wave_total=3u;buildWaveLayoutAfterType(wave_total);pickWaveUnifiedMovement();tryWaveSpawnTopup();spawn_timer=(u8)(SPAWN_BASE+(cpct_rand()&SPAWN_VARIANCE));}
static void spawnWave(void){if(wave_active){if(wave_spawned<wave_total)tryWaveSpawnTopup();return;}if(spawn_timer){--spawn_timer;return;}if(enemiesAlive())return;startNewWave();}
static void moveEnemies(void){u8 i=MAX_ENEMIES;while(i--){if(!enemies[i].active)continue;if(enemies[i].type==ENEMY_TYPE_BOSS){u8 tw,xmn,xmx,r;i16 nxx;tw=BOSS_LANE_W;xmn=enemies[i].boss_lane_x0;xmx=(u8)(xmn+tw-BOSS_W_BYTES);if(xmx>SCR_W_BYTES-BOSS_W_BYTES)xmx=(u8)(SCR_W_BYTES-BOSS_W_BYTES);if(xmn<ENEMY_MIN_X)xmn=ENEMY_MIN_X;if(enemies[i].y<BOSS_HOLD_Y){enemies[i].y=(u8)(enemies[i].y+enemies[i].vy);if(enemies[i].y>BOSS_HOLD_Y)enemies[i].y=BOSS_HOLD_Y;}if(enemies[i].zig_timer)--enemies[i].zig_timer;else{enemies[i].zig_timer=(u8)(10u+(cpct_rand()&15u));r=(u8)(cpct_rand()&7u);if(r<2u)enemies[i].vx=(i8)-enemies[i].vx;else if(r<4u){if(enemies[i].vx)enemies[i].vx=0;else enemies[i].vx=(cpct_rand()&1u)?(i8)1:(i8)-1;}else if(r<6u){if(enemies[i].vx<(i8)0)enemies[i].vx=(i8)-2;else if(enemies[i].vx>(i8)0)enemies[i].vx=(i8)2;else enemies[i].vx=(cpct_rand()&1u)?(i8)1:(i8)-1;}else enemies[i].vx=(cpct_rand()&1u)?(i8)1:(i8)-1;}nxx=(i16)enemies[i].x+(i16)enemies[i].vx;if(nxx<(i16)xmn){enemies[i].x=xmn;enemies[i].vx=(i8)-enemies[i].vx;if(!enemies[i].vx)enemies[i].vx=(i8)1;}else if(nxx>(i16)xmx){enemies[i].x=xmx;enemies[i].vx=(i8)-enemies[i].vx;if(!enemies[i].vx)enemies[i].vx=(i8)-1;}else enemies[i].x=(u8)nxx;continue;}enemies[i].y+=enemies[i].vy;if(enemies[i].pattern==PATT_ZIGZAG){if(enemies[i].zig_timer)--enemies[i].zig_timer;else{enemies[i].zig_timer=6;enemies[i].vx=-enemies[i].vx;}enemies[i].x+=enemies[i].vx;if(enemies[i].x<ENEMY_MIN_X)enemies[i].x=ENEMY_MIN_X;if(enemies[i].x>ENEMY_MAX_X)enemies[i].x=ENEMY_MAX_X;}else if(enemies[i].pattern==PATT_DIAGONAL){if(enemies[i].vx<0){if(enemies[i].x<=ENEMY_MIN_X)enemies[i].vx=1;else enemies[i].x+=enemies[i].vx;}else if(enemies[i].vx>0){if(enemies[i].x>=ENEMY_MAX_X)enemies[i].vx=-1;else enemies[i].x+=enemies[i].vx;}}if(enemies[i].y>=(u8)(SCR_H-ENEMY_H_PIX))enemies[i].active=0;}}static void updateWaveBonus(void){const TLevelConfig* cfg;if(!wave_active)return;if(enemiesAlive())return;if(wave_spawned<wave_total)return;if(wave_killed==wave_total)addScore((u16)wave_total*(u16)current_wave_bonus_base);++waves_cleared;cfg=level_config_get(level);if(waves_cleared>=cfg->waves){waves_cleared=0;if(level>=ENDGAME_FINAL_LEVEL){dx_enter_safepoint(DX_SP_ENDGAME);game_state=GS_TITLE;title_screen_mode=TS_WIN;title_dirty=1;title_phase=0;attract_frm=0;clearActiveCombatState();clearAllExplosions();cpct_setVideoMemoryPage(cpct_pageC0);cpct_setPalette((u8*)loading_palette_hw,16);soundStopAll();wave_active=0;return;}if(level<25u){++level;palette_dirty=1;sfxLevelUp();if((level==6u)||(level==11u)||(level==16u)||(level==21u)){dx_assets_prepare_for_level_at_safepoint(DX_SP_BIOME, level);}}hud_dirty=1;}wave_active=0;}
static void collideShotsEnemies(void){u8 i,j;for(i=0;i<MAX_ENEMIES;++i){if(!enemies[i].active)continue;for(j=0;j<MAX_SHOTS;++j){if(!shots[j].active)continue;if(enemies[i].type==ENEMY_TYPE_BOSS){if(!rectIntersects(enemies[i].x,enemies[i].y,BOSS_W_BYTES,BOSS_H_PIX,shots[j].x,shots[j].y,SHOT_W_BYTES,SHOT_H_PIX))continue;}else{if(!rectIntersects(enemies[i].x,enemies[i].y,ENEMY_W_BYTES,ENEMY_H_PIX,shots[j].x,shots[j].y,SHOT_W_BYTES,SHOT_H_PIX))continue;}shots[j].active=0;--enemies[i].health;if(enemies[i].health==0){spawnExplosion(enemies[i].x,enemies[i].y,EXP_KIND_ENEMY);enemies[i].active=0;++wave_killed;addScore(enemy_score_tbl[enemies[i].type]);}else{spawnExplosion(enemies[i].x,enemies[i].y,EXP_KIND_ENEMY);}sfxEnemyExplosion();break;}}}
static void drawMessageLineColor(u8* v,u8 x,u8 y,const char* s,u8 len,u8 hi){u8 i;u8* d=cpct_getScreenPtr(v,x,y);for(i=0;i<len;++i){const u8* spr=hi?hud_getGlyphHi(s[i]):hud_getGlyph(s[i]);d=drawHudGlyphByte(d,spr);}} 
#if DX_DEBUG_OVERLAY
static void drawDxDebugOverlay(u8* v){
    static char dbg_text[7] = "SP0 B0";
    dbg_text[2]=(char)('0'+(dx_last_safepoint%10u));
    dbg_text[5]=(dx_assets_loaded_biome==0xFFu)?'-':(char)('0'+(dx_assets_loaded_biome%10u));
    cpct_drawSolidBox(cpct_getScreenPtr(v,2,2),B_BG,18,8);
    drawMessageLineColor(v,2,2,dbg_text,6,0);
}
#endif
static const char* const redef_names[]={redef_left,redef_right,redef_up,redef_down,redef_fire,redef_pause,redef_quit};
static const u8 redef_lens[]={4,5,2,4,4,5,4}; 
static void drawTitleCredit(u8* v){
    drawMessageLineColor(v,TITLE_CREDIT_X,TITLE_CREDIT_Y,menu_credit,(u8)(sizeof(menu_credit)-1u),0);
    drawMessageLineColor(v,TITLE_YEAR_X,TITLE_CREDIT_Y,"2026",4,0);
}
static void drawTitleBg(u8* v){
    u8 i;
    cpct_drawSolidBox(v,B_BG,64,SCR_H);
    cpct_drawSolidBox(cpct_getScreenPtr(v,64,0),B_BG,16,SCR_H);
    for(i=0;i<24;++i){
        u8 x=(u8)(((u16)i*37+7)%SCR_W_BYTES);
        u8 y=(u8)(((u16)i*43+11)%SCR_H);
        *cpct_getScreenPtr(v,x,y)=(i&1)?B_DOT_BLUE:B_DOT_BBLUE;
    }
}
static void drawTitleLogo(u8* v){
    title_logo_draw_masked_with_rim(v);
}
static void drawTitleWinLayout(u8* v){
    drawTitleLogo(v);
    cpct_drawSolidBox(cpct_getScreenPtr(v,10,52),B_LINE_RED,60,1);
    drawMessageLineColor(v,(u8)((80-(15*HUD_GLYPH_ADVANCE))>>1),60,"CONGRATULATIONS",15,1);
    drawMessageLineColor(v,(u8)((80-(20*HUD_GLYPH_ADVANCE))>>1),74,"THE INVASION IS OVER",20,1);
    drawMessageLineColor(v,(u8)((80-(21*HUD_GLYPH_ADVANCE))>>1),88,"THANK YOU FOR PLAYING",21,1);
    cpct_drawSolidBox(cpct_getScreenPtr(v,10,100),B_LINE_RED,60,1);
    drawMessageLineColor(v,(u8)((80-(18*HUD_GLYPH_ADVANCE))>>1),160,win_fire_prompt,(u8)(sizeof(win_fire_prompt)-1u),1);
    drawTitleCredit(v);
}
static void drawTitleHelpLayout(u8* v){
    cpct_drawSolidBox(cpct_getScreenPtr(v,10,48),B_LINE_RED,60,1);
    drawMessageLineColor(v,(u8)((80-((sizeof(help_title)-1u)*HUD_GLYPH_ADVANCE))>>1),56,help_title,(u8)(sizeof(help_title)-1u),1);
    cpct_drawSolidBox(cpct_getScreenPtr(v,10,68),B_LINE_RED,60,1);
    drawMessageLineColor(v,(u8)((80-((sizeof(help_l1)-1u)*HUD_GLYPH_ADVANCE))>>1),76,help_l1,(u8)(sizeof(help_l1)-1u),0);
    drawMessageLineColor(v,(u8)((80-((sizeof(help_l2)-1u)*HUD_GLYPH_ADVANCE))>>1),92,help_l2,(u8)(sizeof(help_l2)-1u),0);
    drawMessageLineColor(v,(u8)((80-((sizeof(help_l3)-1u)*HUD_GLYPH_ADVANCE))>>1),104,help_l3,(u8)(sizeof(help_l3)-1u),0);
    drawMessageLineColor(v,(u8)((80-((sizeof(help_l4)-1u)*HUD_GLYPH_ADVANCE))>>1),116,help_l4,(u8)(sizeof(help_l4)-1u),0);
    drawMessageLineColor(v,(u8)((80-((sizeof(help_l5)-1u)*HUD_GLYPH_ADVANCE))>>1),128,help_l5,(u8)(sizeof(help_l5)-1u),0);
    drawMessageLineColor(v,(u8)((80-((sizeof(help_l6)-1u)*HUD_GLYPH_ADVANCE))>>1),140,help_l6,(u8)(sizeof(help_l6)-1u),0);
    cpct_drawSolidBox(cpct_getScreenPtr(v,14,152),B_LINE_CYAN,52,1);
    drawMessageLineColor(v,(u8)((80-((sizeof(help_fire)-1u)*HUD_GLYPH_ADVANCE))>>1),164,help_fire,(u8)(sizeof(help_fire)-1u),1);
    drawTitleCredit(v);
}
static void drawTitleScreen(u8* v){
    drawTitleBg(v);
    if(title_screen_mode==TS_HISCORE_VIEW){
        cpct_drawSolidBox(cpct_getScreenPtr(v,10,42),B_LINE_RED,60,1);
        drawMessageLineColor(v,28,48,"GAME OVER",9,1);
        cpct_drawSolidBox(cpct_getScreenPtr(v,10,60),B_LINE_RED,60,1);
        drawMessageLineColor(v,32,74,"TOP 3",5,1);
        cpct_drawSolidBox(cpct_getScreenPtr(v,14,88),B_LINE_CYAN,52,1);
        drawHiScoreTable(v,96);
        cpct_drawSolidBox(cpct_getScreenPtr(v,14,142),B_LINE_CYAN,52,1);
        drawMessageLineColor(v,20,164,"PRESS ANY KEY",13,1);
    }else if(title_screen_mode==TS_ATTRACT_SCORE){
        drawMessageLineColor(v,32,74,"TOP 3",5,1);
        cpct_drawSolidBox(cpct_getScreenPtr(v,14,88),B_LINE_CYAN,52,1);
        drawHiScoreTable(v,96);
        cpct_drawSolidBox(cpct_getScreenPtr(v,14,142),B_LINE_CYAN,52,1);
        drawTitleCredit(v);
    }else if(title_screen_mode==TS_HISCORE_INPUT){
        drawHiScoreInput(v);
    }else if(title_screen_mode==TS_WIN){
        drawTitleWinLayout(v);
    }else if(title_screen_mode==TS_MENU||title_screen_mode==TS_REDEFINE){
        drawTitleLogo(v);
        cpct_drawSolidBox(cpct_getScreenPtr(v,14,TITLE_MENU_RULE_Y),B_LINE_CYAN,52,1);
        if(title_screen_mode==TS_MENU){
            drawMessageLineColor(v,25,64,menu_opt1,sizeof(menu_opt1)-1,control_mode==CTRL_JOYSTICK);
            drawMessageLineColor(v,25,80,menu_opt2,sizeof(menu_opt2)-1,control_mode==CTRL_KEYBOARD);
            drawMessageLineColor(v,25,96,menu_opt3,sizeof(menu_opt3)-1,0);
            cpct_drawSolidBox(cpct_getScreenPtr(v,14,142),B_LINE_CYAN,52,1);
            drawMessageLineColor(v,20,160,menu_play,sizeof(menu_play)-1,1);
            drawTitleCredit(v);
        }else{
            u8 rs=(redefine_step<7)?redefine_step:6;
            const char* step=redef_names[rs];
            u8 len=redef_lens[rs];
            drawMessageLineColor(v,28,106,redef_title,sizeof(redef_title)-1,0);
            drawMessageLineColor(v,(u8)((80-len*HUD_GLYPH_ADVANCE)>>1),128,step,len,1);
            drawMessageLineColor(v,23,146,redef_msg,sizeof(redef_msg)-1,0);
        }
    }
} 
static void drawTitleBoth(void){drawTitleScreen(SCR_PAGE_C0);drawTitleScreen(SCR_PAGE_80);cpct_waitVSYNC();cpct_setVideoMemoryPage(cpct_pageC0);} 
static void enterTitle(void){dx_enter_safepoint(DX_SP_TITLE);game_state=GS_TITLE;title_dirty=1;title_phase=0;title_screen_mode=TS_MENU;redefine_step=0;attract_frm=0;attract_cycle=0;cpct_setPalette((u8*)loading_palette_hw,16);frontBuffer=SCR_PAGE_C0;backBuffer=SCR_PAGE_80;frontPage=cpct_pageC0;backPage=cpct_page80;cpct_setVideoMemoryPage(frontPage);resetShipRuntimeState();clearActiveCombatState();clearAllExplosions();soundStopAll();}
static void initDirtyRects(void){u8 i,b;for(b=0;b<2;++b){dr_buf_ready[b]=0;dr_ship_drawn[b]=0;for(i=0;i<MAX_SHOTS;++i)dr_shot_drawn[b][i]=0;for(i=0;i<MAX_ENEMIES;++i)dr_emy_drawn[b][i]=0;for(i=0;i<MAX_ENEMY_SHOTS;++i)dr_es_drawn[b][i]=0;for(i=0;i<MAX_EXPLOSIONS;++i)dr_exp_drawn[b][i]=0;}}
static void eraseStars(u8* v,TStar* s,u8 n,u8 delta2){u8 i,oy;for(i=0;i<n;++i){if(s[i].x>=SCR_W_BYTES||s[i].y>=SCR_H)continue;oy=s[i].y;if(oy<delta2){oy=(u8)(oy+(u8)(200u-delta2));if(oy>=SCR_H)continue;}else{oy=(u8)(oy-delta2);}if(oy<GAME_Y0||oy>=SCR_H)continue;*cpct_getScreenPtr(v,s[i].x,oy)=B_BG;}}
static void drawFrame(void){
    u8 i;
    u8 bi=(backBuffer==SCR_PAGE_C0)?0:1;
    if(dr_buf_ready[bi]){
        if(dr_ship_drawn[bi])
            drawSolidBoxSafe(backBuffer,dr_ship_px[bi],dr_ship_py[bi],SHIP_W_BYTES,(u8)(SHIP_H_PIX+5),B_BG);
        for(i=0;i<MAX_SHOTS;++i)
            if(dr_shot_drawn[bi][i])
                drawSolidBoxSafe(backBuffer,dr_shot_px[bi][i],dr_shot_py[bi][i],SHOT_W_BYTES,SHOT_H_PIX,B_BG);
        for(i=0;i<MAX_ENEMIES;++i)
            if(dr_emy_drawn[bi][i]){
                u8 ew=(dr_emy_type[bi][i]==ENEMY_TYPE_BOSS)?BOSS_W_BYTES:ENEMY_W_BYTES;
                u8 eh=(dr_emy_type[bi][i]==ENEMY_TYPE_BOSS)?BOSS_H_PIX:ENEMY_H_PIX;
                drawSolidBoxSafe(backBuffer,dr_emy_px[bi][i],dr_emy_py[bi][i],ew,eh,B_BG);
            }
        for(i=0;i<MAX_ENEMY_SHOTS;++i)
            if(dr_es_drawn[bi][i])
                drawSolidBoxSafe(backBuffer,dr_es_px[bi][i],dr_es_py[bi][i],ENEMYSHOT_W_BYTES,ENEMYSHOT_H_PIX,B_BG);
        for(i=0;i<MAX_EXPLOSIONS;++i)
            if(dr_exp_drawn[bi][i]){
                u8 ex=(dr_exp_px[bi][i]>=3)?(u8)(dr_exp_px[bi][i]-3):0;
                u8 ey=(dr_exp_py[bi][i]>=3)?(u8)(dr_exp_py[bi][i]-3):GAME_Y0;
                if(ey<GAME_Y0)ey=GAME_Y0;
                drawSolidBoxSafe(backBuffer,ex,ey,16,16,B_BG);
            }
        eraseStars(backBuffer,s1,N1,8);
        eraseStars(backBuffer,s2,N2,18);
        eraseStars(backBuffer,s3,N3,32);
    }
    drawMap(backBuffer);
    drawStars(backBuffer,s1,N1,B_STAR1);
    drawStars(backBuffer,s2,N2,(g_dx_active_biome_meta.star_theme!=0u)?B_STAR3:B_STAR2);
    drawStars(backBuffer,s3,N3,B_STAR3);
    drawShots(backBuffer);
    drawEnemyShots(backBuffer);
    drawEnemies(backBuffer);
    drawExplosions(backBuffer);
#if DX_DEBUG_OVERLAY
    drawDxDebugOverlay(backBuffer);
#endif
    if(!ship_exploding && !ship_last_life && (!ship_invul||(ship_invul_timer&1))){
        drawShipThrusters(backBuffer,shipX,shipY);
        drawShipAt(backBuffer,shipX,shipY);
        dr_ship_drawn[bi]=1;
    }else{
        dr_ship_drawn[bi]=0;
    }
    dr_ship_px[bi]=shipX; dr_ship_py[bi]=shipY;
    for(i=0;i<MAX_SHOTS;++i){dr_shot_drawn[bi][i]=shots[i].active;dr_shot_px[bi][i]=shots[i].x;dr_shot_py[bi][i]=shots[i].y;}
    for(i=0;i<MAX_ENEMIES;++i){dr_emy_drawn[bi][i]=enemies[i].active;dr_emy_px[bi][i]=enemies[i].x;dr_emy_py[bi][i]=enemies[i].y;dr_emy_type[bi][i]=enemies[i].type;}
    for(i=0;i<MAX_ENEMY_SHOTS;++i){dr_es_drawn[bi][i]=enemyshots[i].active;dr_es_px[bi][i]=enemyshots[i].x;dr_es_py[bi][i]=enemyshots[i].y;}
    for(i=0;i<MAX_EXPLOSIONS;++i){dr_exp_drawn[bi][i]=explosions[i].active;dr_exp_px[bi][i]=explosions[i].x;dr_exp_py[bi][i]=explosions[i].y;}
    dr_buf_ready[bi]=1;
    if(palette_dirty){
        u8 biome_idx=(u8)((level-1u)/5u);
        if(biome_idx>4u)biome_idx=4u;
        cpct_waitVSYNC();
        cpct_setPalette((u8*)biome_palettes[biome_idx],16);
        palette_dirty=0;
    }
    cpct_waitVSYNC();
    cpct_setVideoMemoryPage(backPage);
    swapVideoPages();
}
static void startGame(void){
    // Wait for beep to finish (15 frames = ~300ms)
    u8 i;
    for(i=0; i<15; i++){
        soundTick();
        cpct_waitVSYNC();
    }
    
    resetVideoPages();
    resetGameSession();
    dx_assets_prepare_for_level_at_safepoint(DX_SP_STARTGAME, level);
    { u8 bi=(u8)((level-1u)/5u); if(bi>4u)bi=4u; cpct_setPalette((u8*)biome_palettes[bi],16); }
    game_paused=0;
    initHUD();
    soundStopAll();
    clearGameArea(SCR_PAGE_C0,B_BG);
    clearGameArea(SCR_PAGE_80,B_BG);
    drawMap(SCR_PAGE_C0);
    drawMap(SCR_PAGE_80);
    initDirtyRects();
    flushHUDToBothBuffers();
    drawFrame();
    game_state=GS_PLAYING;
}
static void collideShipEnemies(void){u8 i;if(ship_exploding||ship_invul||ship_last_life)return;for(i=0;i<MAX_ENEMIES;++i){if(!enemies[i].active)continue;if(enemies[i].type==ENEMY_TYPE_BOSS){if(rectIntersects(enemies[i].x,enemies[i].y,BOSS_W_BYTES,BOSS_H_PIX,shipX,shipY,SHIP_W_BYTES,SHIP_H_PIX)){enemies[i].active=0;triggerShipHit();return;}}else{if(rectIntersects(enemies[i].x,enemies[i].y,ENEMY_W_BYTES,ENEMY_H_PIX,shipX,shipY,SHIP_W_BYTES,SHIP_H_PIX)){enemies[i].active=0;triggerShipHit();return;}}}}
void main(void){
    u8 nx,ny,stepx,stepy,thrust_now,fire_now;
    /* Pila just sota 0x8000 (2n buffer vídeo). Codi gran: s__DATA puja i el marge SP↔BSS es redueix; cal picar codi/BSS o baixar l__CODE. */
    cpct_setStackLocation((void*)0x7FFF);
    
    cpct_disableFirmware();
    dx_boot();
    cpct_setVideoMode(0);
    cpct_clearScreen(0);
    cpct_srand(12345);
    ship_buildMaskedSprite();
    boss_buildMaskedSprite();
    enemy_buildMaskedSprite();
    enemy_fast_buildMaskedSprite();
    enemy_heavy_buildMaskedSprite();
    enemy_diver_buildMaskedSprite();
    enemy_bomber_buildMaskedSprite();
    shot_buildMaskedSprite();
    enemyshot_buildMaskedSprites();
    hud_buildFont();
    buildWallTiles();
    resetVideoPages();
    initStars(s1,N1);
    initStars(s2,N2);
    initStars(s3,N3);
    resetGameSession();
    initHiScores();
    initHUD();
    soundInit();
    dx_assets_init();
    enterTitle();
    
    while(1){
        cpct_scanKeyboard_f();
        
        if(game_state==GS_TITLE){
            if(title_dirty){
                drawTitleBoth();
                title_dirty=0;
                blink_ctr=0;
            }
            
            soundTick();

            /* Attract: cicle menú→TOP3→menú→HOWTO. Foc a TOP3/HOWTO → menú (no arrenca partida). */
            if((title_screen_mode==TS_ATTRACT_SCORE||title_screen_mode==TS_HELP)&&menuFire()){
                title_screen_mode=TS_MENU;
                attract_frm=0;
                attract_cycle=0;
                title_dirty=1;
                title_phase=0;
            }else if(title_screen_mode==TS_MENU||title_screen_mode==TS_ATTRACT_SCORE||title_screen_mode==TS_HELP){
                attract_frm++;
                if(attract_frm>=ATTRACT_HOLD_FRAMES){
                    attract_frm=0;
                    if(title_screen_mode==TS_MENU){
                        if(attract_cycle==0u){
                            title_screen_mode=TS_ATTRACT_SCORE;
                            attract_cycle=1u;
                        }else{
                            title_screen_mode=TS_HELP;
                            attract_cycle=3u;
                        }
                    }else if(title_screen_mode==TS_ATTRACT_SCORE){
                        title_screen_mode=TS_MENU;
                        attract_cycle=2u;
                    }else{
                        title_screen_mode=TS_MENU;
                        attract_cycle=0u;
                    }
                    title_dirty=1;
                    title_phase=0;
                }
            }
            
            if(title_screen_mode==TS_HISCORE_VIEW){
                if(title_phase==0){
                    if(!cpct_isAnyKeyPressed_f())
                        title_phase=1;
                }else{
                    if(cpct_isAnyKeyPressed_f()){
                        cpct_setPalette((u8*)loading_palette_hw,16);
                        title_screen_mode=TS_MENU;
                        attract_frm=0;
                        attract_cycle=0;
                        title_dirty=1;
                        title_phase=0;
                    }
                }
            }else if(title_screen_mode==TS_WIN){
                if(attract_frm<35u){
                    ++attract_frm;
                }else if(menuFire()){
                    hs_pos=hs_isTop(score,level);
                    if(score>0u&&hs_pos<3u){
                        initHiScoreInput();
                        title_screen_mode=TS_HISCORE_INPUT;
                    }else{
                        title_screen_mode=TS_HISCORE_VIEW;
                    }
                    title_dirty=1;
                    title_phase=0;
                }
                if(attract_frm>=35u){
                    u8 pb=(blink_ctr&0x10)?1:0,cb,wx;
                    ++blink_ctr;
                    cb=(blink_ctr&0x10)?1:0;
                    if(pb!=cb){
                        wx=(u8)((80-(21*HUD_GLYPH_ADVANCE))>>1);
                        if(cb){
                            cpct_drawSolidBox(cpct_getScreenPtr(SCR_PAGE_C0,wx,88),B_BG,(u8)(21*HUD_GLYPH_ADVANCE),HUD_GLYPH_H);
                            cpct_drawSolidBox(cpct_getScreenPtr(SCR_PAGE_80,wx,88),B_BG,(u8)(21*HUD_GLYPH_ADVANCE),HUD_GLYPH_H);
                        }else{
                            drawMessageLineColor(SCR_PAGE_C0,wx,88,"THANK YOU FOR PLAYING",21,1);
                            drawMessageLineColor(SCR_PAGE_80,wx,88,"THANK YOU FOR PLAYING",21,1);
                        }
                    }
                }
            }else if(title_screen_mode==TS_HISCORE_INPUT){
                title_phase++;
                updateHiScoreInput();
                drawHiScoreInputRow(SCR_PAGE_C0);
                drawHiScoreInputRow(SCR_PAGE_80);
            }else if(title_screen_mode==TS_ATTRACT_SCORE||title_screen_mode==TS_HELP){
                /* Toggle per temps i FIRE→MENU (gestionat a dalt). */
            }else if(title_screen_mode==TS_MENU){
                if(title_phase==0){
                    if(!(cpct_isKeyPressed(Key_1)||cpct_isKeyPressed(Key_2)||cpct_isKeyPressed(Key_3)||menuFire()))
                        title_phase=1;
                }else{
                    if(cpct_isKeyPressed(Key_1)){
                        control_mode=CTRL_JOYSTICK;
                        title_dirty=1;
                        title_phase=0;
                    }else if(cpct_isKeyPressed(Key_2)){
                        control_mode=CTRL_KEYBOARD;
                        title_dirty=1;
                        title_phase=0;
                    }else if(cpct_isKeyPressed(Key_3)){
                        title_screen_mode=TS_REDEFINE;
                        redefine_step=0;
                        title_dirty=1;
                        title_phase=0;
                    }else if(menuFire()){
                        sfxBeep();
                        startGame();
                        title_phase=0;
                    }
                }
                {
                    u8 pb=(blink_ctr&0x10)?1:0;
                    u8 cb;
                    ++blink_ctr;
                    cb=(blink_ctr&0x10)?1:0;
                    if(pb!=cb){
                        if(cb){
                            cpct_drawSolidBox(cpct_getScreenPtr(SCR_PAGE_C0,20,160),B_BG,39,8);
                            cpct_drawSolidBox(cpct_getScreenPtr(SCR_PAGE_80,20,160),B_BG,39,8);
                        }else{
                            drawMessageLineColor(SCR_PAGE_C0,20,160,menu_play,sizeof(menu_play)-1,1);
                            drawMessageLineColor(SCR_PAGE_80,20,160,menu_play,sizeof(menu_play)-1,1);
                        }
                    }
                }
            }else{
                if(title_phase==0){
                    if(!cpct_isAnyKeyPressed_f())
                        title_phase=1;
                }else{
                    cpct_keyID pressed;
                    pressed=firstPressedKey();
                    if(pressed){
                        switch(redefine_step){
                            case 0:key_left=pressed;break;
                            case 1:key_right=pressed;break;
                            case 2:key_up=pressed;break;
                            case 3:key_down=pressed;break;
                            case 4:key_fire=pressed;break;
                            case 5:key_pause=pressed;break;
                            default:key_quit=pressed;break;
                        }
                        ++redefine_step;
                        title_phase=0;
                        title_dirty=1;
                        if(redefine_step>=7){
                            control_mode=CTRL_KEYBOARD;
                            title_screen_mode=TS_MENU;
                            redefine_step=0;
                            attract_frm=0;
                            attract_cycle=0;
                        }
                    }
                }
            }
            soundTick();
            cpct_waitVSYNC();
            continue;
        }
        
        if(game_state==GS_PLAYING){
            static u8 pause_key_was_pressed=0;
            
            // Check for pause key press (not release)
            if(inputPause() && !pause_key_was_pressed){
                pause_key_was_pressed=1;
            }
            // Check for pause key release (activate on release)
            else if(!inputPause() && pause_key_was_pressed){
                pause_key_was_pressed=0;
                game_paused^=1;  // Toggle pause state
                if(game_paused){
                    soundStopAll();
                }else{
                    soundInit();
                }
            }
            
            if(inputQuit()){
                game_paused=0;  // Reset pause state
                soundInit();    // Restore sound
                game_state=GS_TITLE;
                enterTitle();
                continue;
            }
        }
        if(game_paused && game_state==GS_PLAYING){
            static u8 pause_blink_timer=0;
            static u8 pause_blink_state=0;
            
            // Blink logic
            pause_blink_timer++;
            if(pause_blink_timer>=8){
                pause_blink_timer=0;
                pause_blink_state^=1;
            }
            
            // Clear game area center and draw PAUSA
            cpct_drawSolidBox(cpct_getScreenPtr(frontBuffer,32,96),B_BG,24,16);
            if(pause_blink_state){
                drawMessageLineColor(frontBuffer,32,96,"PAUSA",5,0);
            }
            
            cpct_setVideoMemoryPage(frontPage);
            cpct_waitVSYNC();
            cpct_scanKeyboard_f();
            continue;
        }
        
        if(!ship_exploding && !ship_last_life && game_state==GS_PLAYING){
            if(shipY<GAME_Y0) shipY=GAME_Y0;
            nx=shipX;
            ny=shipY;
            stepx=shipStepX();
            stepy=shipStepY();
            thrust_now=0;
            
            if(inputLeft()&&nx>=stepx){
                nx-=stepx;
                thrust_now=1;
            }
            if(inputRight()&&nx<=(SCR_W_BYTES-SHIP_W_BYTES-stepx)){
                nx+=stepx;
                thrust_now=1;
            }
            if(inputUp()&&ny>=(u8)(GAME_Y0+stepy)){
                ny-=stepy;
                thrust_now=1;
            }
            if(inputDown()&&ny<=(SCR_H-SHIP_H_PIX-stepy))
                ny+=stepy;
            if(ny<GAME_Y0) ny=GAME_Y0;
            if(!rectHitsWall(nx,ny,SHIP_W_BYTES,SHIP_H_PIX)){
                shipX=nx;
                shipY=ny;
            }
            ship_thrust=thrust_now;
            fire_now=inputFire();
            if(fire_now)fireShot(shipX,shipY);
            spawnWave();
            moveEnemies();
            trySpawnEnemyShots();
        }else ship_thrust=0;
        
        if(!game_paused && game_state==GS_PLAYING){
            v9_tickStarsShots();
            tickExplosions();
            collideShotsEnemies();
            collideShipEnemies();
            collideShipEnemyShots();
            tickEnemyShots();
            updateWaveBonus();
            updateShipThrustAnim();
            updateShipInvulnerability();
            updateShipExplosionState();
            
            // Check for game over when ship_last_life is set and explosion finished
            if(ship_last_life && !ship_exploding){
                static u8 game_over_delay = 0;
                if(game_over_delay == 0){
                    sfxGameOver();
                    game_over_delay = 1;
                }else if(game_over_delay < 30){  // Wait ~600ms for sound to complete
                    game_over_delay++;
                    cpct_waitVSYNC();
                }else{
                    game_state=GS_TITLE;
                    game_over_delay = 0;
                    
                    // Check hi-score - només si té puntuació vàlida
                    hs_pos=hs_isTop(score,level);
                    if(score>0u&&hs_pos<3u){
                        // Iniciar modo input de iniciales
                        initHiScoreInput();
                        title_screen_mode = TS_HISCORE_INPUT;
                    } else {
                        title_screen_mode = TS_HISCORE_VIEW;
                    }
                    
                    title_dirty=1;
                    title_phase=0;
                    cpct_setVideoMemoryPage(cpct_pageC0);
                    cpct_setPalette((u8*)loading_palette_hw,16);  // Establecer paleta correcta para título
                    soundStopAll();
                    continue;
                }
            }
        }
        flushHUDToBothBuffers();
        drawFrame();
        title_phase++;  // Incrementar para efecto de parpadeo
        soundTick();
    }
}
