/* main_menu.c — MENU.BIN: títol, menú, hi-scores, redefinir tecles.
 *
 * No inclou: sprites de nau/enemics, lògica de joc, onades, col·lisions.
 * Transició a GAME.BIN: shared state + nau_transit_load_game() (TRANSIT.BIN resident).
 * En arrencar: llegeix shared state per saber si ve d'una partida o és fresh boot.
 */

#include <cpctelera.h>
#include "hud_font.h"
#include "title_logo.h"
#include "game_sfx.h"
#include "nau_shared.h"
#include "dx_bank.h"

void psgWrite(u8 reg, u8 value);

/* ---- Dimensions de pantalla ---- */
#define SCR_W_BYTES    80
#define SCR_H         200
#define HUD_H          16
#define GAME_Y0       HUD_H
#define SCR_PAGE_C0  ((u8*)0xC000)
#define SCR_PAGE_80  ((u8*)0x8000)

/* ---- Colors (bytes mode 0) ---- */
#define B_BG        cpct_px2byteM0(0,0)
#define B_LINE_CYAN cpct_px2byteM0(5,5)
#define B_LINE_RED  cpct_px2byteM0(2,2)
#define B_DOT_BLUE  cpct_px2byteM0(1,1)
#define B_DOT_BBLUE cpct_px2byteM0(3,3)

/* ---- Estats del joc i del títol ---- */
#define GS_TITLE       0
#define TS_MENU        0
#define TS_REDEFINE    1
#define TS_HISCORE_VIEW 2
#define TS_HISCORE_INPUT 3
#define TS_ATTRACT_SCORE 4
#define TS_WIN         5
#define TS_HELP        6
#define ATTRACT_HOLD_FRAMES 500

/* ---- Controls ---- */
#define CTRL_JOYSTICK  0
#define CTRL_KEYBOARD  1

/* ---- HUD font layout ---- */
#define HUD_SCORE_X    2
#define HUD_LEVEL_X   38
#define HUD_LIVES_X   54
#define TITLE_CREDIT_X 0
#define TITLE_CREDIT_Y ((u8)(SCR_H - HUD_GLYPH_H))
/* Any a la dreta, mateixa alçada que el crèdit (alineat a marge dret). */
#define TITLE_YEAR_X   ((u8)(SCR_W_BYTES - (4u * HUD_GLYPH_ADVANCE)))
#define HISCORE_ROW_CHARS 16
/* Menú principal: equilibri entre ratlla cyan superior (sota logo) i la d’avall (mateix marge ~29 px). */
#define MENU_OPT1_Y          73u
#define MENU_OPT2_Y          89u
#define MENU_OPT3_Y         105u
#define MENU_RULE_BOTTOM_Y 142u
#define MENU_PROMPT_Y     160u
/* HOW TO: desplaçament vertical del bloc sencer (mides relatives iguals). */
#define TITLE_HELP_VERT_SHIFT 14u
/* HOW TO: sense logo ni vermell; cyan separa títol de les instruccions + barra abans de FIRE. */
#define TITLE_HELP_TITLE_Y      ((u8)(10u + TITLE_HELP_VERT_SHIFT))
#define TITLE_HELP_RULE_MID_Y   ((u8)(TITLE_HELP_TITLE_Y + HUD_GLYPH_H + 2u))
#define TITLE_HELP_L1_Y         ((u8)(TITLE_HELP_RULE_MID_Y + 4u))
#define TITLE_HELP_L2_Y         ((u8)(TITLE_HELP_L1_Y + 12u))
#define TITLE_HELP_L3_Y         ((u8)(TITLE_HELP_L2_Y + 12u))
#define TITLE_HELP_L4_Y         ((u8)(TITLE_HELP_L3_Y + 12u))
#define TITLE_HELP_L5_Y         ((u8)(TITLE_HELP_L4_Y + 12u))
#define TITLE_HELP_L6_Y         ((u8)(TITLE_HELP_L5_Y + 12u))
#define TITLE_HELP_RULE_BOT_Y   ((u8)(TITLE_HELP_L6_Y + 12u))
#define TITLE_HELP_FIRE_Y       ((u8)(TITLE_HELP_RULE_BOT_Y + 12u))

/* ---- Forward declarations ---- */
static void drawTitleBoth(void);
static void drawTitleScreen(u8* v);

/* ---- Paletes ---- */
/* Logo SOLO metàl·lic: 14 vora = HW 23 (Sky Blue); evita HW 5 (Purple, massa vermell). */
static const u8 loading_palette_hw[16] = {20,4,28,21,18,23,0,19,13,31,25,31,19,4,23,11};

/* ---- Pàgines de vídeo ---- */
static u8* frontBuffer = SCR_PAGE_C0;
static u8* backBuffer  = SCR_PAGE_80;
static u8  frontPage   = cpct_pageC0;
static u8  backPage    = cpct_page80;

static void resetVideoPages(void) {
    frontBuffer = SCR_PAGE_C0;
    backBuffer  = SCR_PAGE_80;
    frontPage   = cpct_pageC0;
    backPage    = cpct_page80;
    cpct_setVideoMemoryPage(frontPage);
}

/* ---- Controls (locals; sincronitzats amb shared state) ---- */
static u8 control_mode = CTRL_JOYSTICK;
static cpct_keyID key_left  = Key_CursorLeft;
static cpct_keyID key_right = Key_CursorRight;
static cpct_keyID key_up    = Key_CursorUp;
static cpct_keyID key_down  = Key_CursorDown;
static cpct_keyID key_fire  = Key_Z;
static cpct_keyID key_pause = Key_P;
static cpct_keyID key_quit  = Key_Return;

static cpct_keyID firstPressedKey(void) {
    u8 i=10, *keys=cpct_keyboardStatusBuffer+9;
    u16 keypressed;
    do {
        keypressed = ((u16)(*keys)) ^ 0xFF;
        if(keypressed) return (cpct_keyID)((keypressed<<8)+(i-1));
        --keys;
    } while(--i);
    return 0;
}
static u8 inputLeft(void)  { return (control_mode==CTRL_JOYSTICK)?cpct_isKeyPressed(Joy0_Left):cpct_isKeyPressed(key_left); }
static u8 inputRight(void) { return (control_mode==CTRL_JOYSTICK)?cpct_isKeyPressed(Joy0_Right):cpct_isKeyPressed(key_right); }
static u8 inputUp(void)    { return (control_mode==CTRL_JOYSTICK)?cpct_isKeyPressed(Joy0_Up):cpct_isKeyPressed(key_up); }
static u8 inputDown(void)  { return (control_mode==CTRL_JOYSTICK)?cpct_isKeyPressed(Joy0_Down):cpct_isKeyPressed(key_down); }
static u8 inputFire(void)  { return (control_mode==CTRL_JOYSTICK)?(cpct_isKeyPressed(Joy0_Fire1)||cpct_isKeyPressed(Joy0_Fire2)||cpct_isKeyPressed(Joy0_Fire3)):cpct_isKeyPressed(key_fire); }
static u8 menuFire(void)   { return inputFire(); }

/* ---- Hi-scores locals (sincronitzats amb shared state) ---- */
typedef struct { u16 score; u8 level_reached; u8 name[3]; } THiScore;
static THiScore hiscores[3];
static u8 hs_pos = 0;
static u8 hs_input_pos = 0;
static u8 hs_input_char[3];

static void initHiScores(void) {
    hiscores[0].score=300; hiscores[0].level_reached=0;
    hiscores[0].name[0]='X'; hiscores[0].name[1]='M'; hiscores[0].name[2]='A';
    hiscores[1].score=300; hiscores[1].level_reached=0;
    hiscores[1].name[0]='P'; hiscores[1].name[1]='M'; hiscores[1].name[2]='H';
    hiscores[2].score=300; hiscores[2].level_reached=0;
    hiscores[2].name[0]='C'; hiscores[2].name[1]='M'; hiscores[2].name[2]='T';
}

/* Sincronitza hiscores[] locals → shared state (crida abans de carregar GAME.BIN) */
static void syncHiScoresToShared(void) {
    u8 i;
    for(i=0;i<3;i++) {
        g_shared->hs_score[i] = hiscores[i].score;
        g_shared->hs_level[i] = hiscores[i].level_reached;
        g_shared->hs_name[i][0] = hiscores[i].name[0];
        g_shared->hs_name[i][1] = hiscores[i].name[1];
        g_shared->hs_name[i][2] = hiscores[i].name[2];
    }
}

/* Sincronitza shared state → hiscores[] locals (crida en arrencar si ve de partida) */
static void syncHiScoresFromShared(void) {
    u8 i;
    for(i=0;i<3;i++) {
        hiscores[i].score        = g_shared->hs_score[i];
        hiscores[i].level_reached= g_shared->hs_level[i];
        hiscores[i].name[0]      = g_shared->hs_name[i][0];
        hiscores[i].name[1]      = g_shared->hs_name[i][1];
        hiscores[i].name[2]      = g_shared->hs_name[i][2];
    }
}

static u8 hs_newBeatsEntry(u16 ns, u8 nlv, const THiScore* e) {
    if(ns>e->score) return 1;
    if(ns<e->score) return 0;
    return nlv>e->level_reached;
}
static u8 hs_isTop(u16 s, u8 lv) {
    u8 i;
    for(i=0;i<3;i++) if(hs_newBeatsEntry(s,lv,&hiscores[i])) return i;
    return 255;
}
static void hs_insert(u8 pos, u16 s, u8 lv, const char* name) {
    u8 i;
    for(i=2;i>pos;i--) {
        hiscores[i].score=hiscores[i-1].score;
        hiscores[i].level_reached=hiscores[i-1].level_reached;
        hiscores[i].name[0]=hiscores[i-1].name[0];
        hiscores[i].name[1]=hiscores[i-1].name[1];
        hiscores[i].name[2]=hiscores[i-1].name[2];
    }
    hiscores[pos].score=s; hiscores[pos].level_reached=lv;
    hiscores[pos].name[0]=name[0]; hiscores[pos].name[1]=name[1]; hiscores[pos].name[2]=name[2];
}

/* ---- Estat del títol ---- */
static u8 title_dirty=1, title_phase=0, title_screen_mode=TS_MENU, redefine_step=0;
static u8 blink_ctr=0;
static u16 attract_frm=0;
static u16 hs_view_release_wait=0;
/* 0: menú inicial→TOP3; 1: TOP3→menú; 2: menú→HOWTO; 3: HOWTO→menú (cicle attract). */
static u8 attract_cycle=0;
#define TITLE_SIDE_STAR_N    40
/* TOP 3 / attract: una sola capa, repartida al centre i a la franja superior (evita marges i pics agrupats). */
#define TITLE_CENTER_STAR_N  12
#define TITLE_TWINKLE_FRAMES  4
#define HS_VIEW_RELEASE_MAX_FRAMES 600u

static u8 title_twinkle_tick=0, title_twinkle_rng=0x5Au;
static u8 title_star_state[TITLE_SIDE_STAR_N], title_star_center_state[TITLE_CENTER_STAR_N];

/* Estrelles laterals (mes densitat; eviten zona central logo/menú principal) */
static const u8 title_star_x[TITLE_SIDE_STAR_N] = {
    4, 15,  9, 18,  6, 13,  3, 16, 10,  5, 19, 12,  7, 14, 11, 20,  8, 17,  2, 21,
   66, 74, 61, 70, 77, 64, 72, 68, 75, 62, 71, 65, 73, 69, 76, 63, 78, 67, 59, 79
};
static const u8 title_star_y[TITLE_SIDE_STAR_N] = {
    22, 41, 29, 74, 58,113, 95,146,131,176,164,191, 50, 67, 88,104,122,139,156,172,
    18, 37, 55, 48, 83,101,119,136,158,149,181,193, 52, 69, 90,108,125,142,159,185
};
static const u8 title_star_seed[TITLE_SIDE_STAR_N] = {
    0,2,1,3,1,0,3,2,0,1,3,2, 2,0,3,1, 1,2,0,3,
    2,0,3,1,3,2,1,0,2,3,0,1, 3,2,0,1, 2,3,1,0
};
static const u8 title_star_center_x[TITLE_CENTER_STAR_N] = {
    30, 46, 26, 42, 34, 50, 24, 38, 44, 48, 32, 52
};
static const u8 title_star_center_y[TITLE_CENTER_STAR_N] = {
    18, 22, 30, 28, 36, 34, 38, 44, 46, 62, 64, 26
};
/* Fases inicials escalonades (0–3) perquè el pic cyan no coincideixi en punts propers. */
static const u8 title_star_center_seed[TITLE_CENTER_STAR_N] = {
    0, 3, 1, 2, 2, 0, 3, 1, 1, 2, 3, 0
};

/* Puntuació de la partida acabada (llegida de shared state) */
static u16 last_score=0;
static u8  last_level=1;

/* ---- Strings del menú ---- */
static const char menu_credit[]="GAME BY XEVIMET4L";
static const char menu_opt1[]="1 JOYSTICK";
static const char menu_opt2[]="2 KEYBOARD";
static const char menu_opt3[]="3 SET KEYS";
static const char menu_play[]="FIRE TO START";
static const char win_fire_prompt[]="PRESS FIRE TO MENU";
static const char help_title[]="HOW TO PLAY";
static const char help_l1[]="JOY OR ARROWS TO MOVE";
static const char help_l2[]="Z OR FIRE 1 TO SHOOT";
static const char help_l3[]="P TO PAUSE";
static const char help_l4[]="Q OR RETURN TO QUIT";
static const char help_l5[]="EXTRA LIFE EVERY 5000 PTS";
static const char help_l6[]="COMPLETE WAVE FOR BONUS";
static const char help_fire[]="FIRE TO MENU";
static const char menu_loading[]="LOADING";
/* Prompt inferior i "LOADING" centrat com a subtítol del mateix bloc que FIRE TO START. */
#define MENU_PROMPT_X 20
static const u8 menu_loading_x = (u8)(MENU_PROMPT_X + (u8)(((sizeof(menu_play) - 1u) - (sizeof(menu_loading) - 1u)) * HUD_GLYPH_ADVANCE / 2u));
static const char redef_title[]="SET KEYS";
static const char redef_msg[]="PRESS A KEY";
static const char redef_left[]="LEFT";
static const char redef_right[]="RIGHT";
static const char redef_up[]="UP";
static const char redef_down[]="DOWN";
static const char redef_fire[]="FIRE";
static const char redef_pause[]="PAUSE";
static const char redef_quit[]="QUIT";
static const char* const redef_names[]={redef_left,redef_right,redef_up,redef_down,redef_fire,redef_pause,redef_quit};
static const u8 redef_lens[]={4,5,2,4,4,5,4};

/* ---- Funcions de dibuix HUD font ---- */
static u8* drawHudGlyphByte(u8* d, const u8* spr) {
    cpct_drawSprite((u8*)spr, d, HUD_GLYPH_W_BYTES, HUD_GLYPH_H);
    if(HUD_GLYPH_GAP_BYTES) cpct_drawSolidBox(d+HUD_GLYPH_W_BYTES, B_BG, HUD_GLYPH_GAP_BYTES, HUD_GLYPH_H);
    return d+HUD_GLYPH_ADVANCE;
}
static void drawMessageLineColor(u8* v, u8 x, u8 y, const char* s, u8 len, u8 hi) {
    u8 i;
    u8* d = cpct_getScreenPtr(v, x, y);
    for(i=0;i<len;++i) {
        const u8* spr = hi ? hud_getGlyphHi(s[i]) : hud_getGlyph(s[i]);
        d = drawHudGlyphByte(d, spr);
    }
}

/* ---- Hi-score input ---- */
static void initHiScoreInput(void) {
    u8 i;
    hs_input_pos = 0;
    for(i=0;i<3;i++) hs_input_char[i]='.';
}
static void hs_formatRow(char* row, u8 rank, u16 sc, u8 lv, const u8* name) {
    static const u16 hs_pow10[4]={10000u,1000u,100u,10u};
    u8 i,d;
    u16 s=sc;
    u8 tens,ones;
    row[0]=(char)('1'+rank); row[1]=' ';
    for(i=0;i<4u;++i){
        d=0;
        while(s>=hs_pow10[i]){s=(u16)(s-hs_pow10[i]);++d;}
        row[2+i]=(char)('0'+d);
    }
    row[6]=(char)('0'+(u8)s);
    row[7]=' ';
    row[8]='L'; row[9]='V';
    if(lv>=20u){tens=2u;ones=(u8)(lv-20u);}
    else if(lv>=10u){tens=1u;ones=(u8)(lv-10u);}
    else{tens=0u;ones=lv;}
    row[10]=(char)('0'+tens);
    row[11]=(char)('0'+ones);
    row[12]=' '; row[13]=(char)name[0]; row[14]=(char)name[1]; row[15]=(char)name[2];
    row[16]=0;
}
static void drawHiScoreTable(u8* v, u8 y0) {
    u8 i; char row[17];
    for(i=0;i<3;i++) {
        hs_formatRow(row,i,hiscores[i].score,hiscores[i].level_reached,hiscores[i].name);
        drawMessageLineColor(v,16,(u8)(y0+(u8)(i*14)),row,HISCORE_ROW_CHARS,(i==0)?1:0);
    }
}
static void drawHiScoreInput(u8* v) {
    u8 i,ry; char row[17];
    drawMessageLineColor(v,28,48,"GAME OVER",9,1);
    cpct_drawSolidBox(cpct_getScreenPtr(v,10,60),B_LINE_RED,60,1);
    drawMessageLineColor(v,32,74,"TOP 3",5,1);
    cpct_drawSolidBox(cpct_getScreenPtr(v,14,88),B_LINE_CYAN,52,1);
    for(i=0;i<3;i++) {
        ry=(u8)(96+(u8)(i*14));
        if(i==hs_pos) {
            hs_formatRow(row,i,last_score,last_level,(const u8*)hs_input_char);
            drawMessageLineColor(v,16,ry,row,HISCORE_ROW_CHARS,1);
        } else if(i<hs_pos) {
            hs_formatRow(row,i,hiscores[i].score,hiscores[i].level_reached,hiscores[i].name);
            drawMessageLineColor(v,16,ry,row,HISCORE_ROW_CHARS,0);
        } else {
            hs_formatRow(row,i,hiscores[i-1u].score,hiscores[i-1u].level_reached,hiscores[i-1u].name);
            drawMessageLineColor(v,16,ry,row,HISCORE_ROW_CHARS,0);
        }
    }
    cpct_drawSolidBox(cpct_getScreenPtr(v,14,142),B_LINE_CYAN,52,1);
}
static void drawHiScoreInputRow(u8* v) {
    u8 ry=(u8)(96+(u8)(hs_pos*14));
    u8 ix=(u8)(16+(13*HUD_GLYPH_ADVANCE));
    u8* d=cpct_getScreenPtr(v,ix,ry);
    cpct_drawSprite((u8*)hud_getGlyphHi(hs_input_char[0]),d,HUD_GLYPH_W_BYTES,HUD_GLYPH_H);
    if(HUD_GLYPH_GAP_BYTES) cpct_drawSolidBox(d+HUD_GLYPH_W_BYTES,B_BG,HUD_GLYPH_GAP_BYTES,HUD_GLYPH_H);
    cpct_drawSprite((u8*)hud_getGlyphHi(hs_input_char[1]),d+HUD_GLYPH_ADVANCE,HUD_GLYPH_W_BYTES,HUD_GLYPH_H);
    if(HUD_GLYPH_GAP_BYTES) cpct_drawSolidBox(d+HUD_GLYPH_ADVANCE+HUD_GLYPH_W_BYTES,B_BG,HUD_GLYPH_GAP_BYTES,HUD_GLYPH_H);
    cpct_drawSprite((u8*)hud_getGlyphHi(hs_input_char[2]),d+(2*HUD_GLYPH_ADVANCE),HUD_GLYPH_W_BYTES,HUD_GLYPH_H);
    {
        u8 j,c,pix[2];
        for(j=0;j<3;++j) {
            c=(j==hs_input_pos)?15:0;
            pix[0]=pix[1]=c;
            cpct_drawSprite(pix,cpct_getScreenPtr(v,(u8)(ix+j*HUD_GLYPH_ADVANCE),(u8)(ry+HUD_GLYPH_H)),2,1);
        }
    }
}
static void updateHiScoreInput(void) {
    static u8 key_cooldown=0;
    cpct_scanKeyboard_f();
    if(key_cooldown>0) { key_cooldown--; return; }
    if(inputLeft()) {
        if(hs_input_pos>0) { hs_input_pos--; title_phase=0; }
        key_cooldown=8;
    } else if(inputRight()) {
        if(hs_input_pos<2) { hs_input_pos++; title_phase=0; }
        key_cooldown=8;
    }
    if(inputUp()) {
        if(hs_input_char[hs_input_pos]=='.') hs_input_char[hs_input_pos]='A';
        else if(hs_input_char[hs_input_pos]=='Z') hs_input_char[hs_input_pos]='.';
        else if(hs_input_char[hs_input_pos]<'Z') hs_input_char[hs_input_pos]++;
        title_phase=0; key_cooldown=8;
    } else if(inputDown()) {
        if(hs_input_char[hs_input_pos]=='A') hs_input_char[hs_input_pos]='.';
        else if(hs_input_char[hs_input_pos]=='.') hs_input_char[hs_input_pos]='Z';
        else if(hs_input_char[hs_input_pos]>'.') hs_input_char[hs_input_pos]--;
        title_phase=0; key_cooldown=8;
    }
    if(inputFire()) {
        hs_insert(hs_pos,last_score,last_level,hs_input_char);
        title_screen_mode=TS_HISCORE_VIEW;
        title_phase=0; key_cooldown=8;
    }
}

/* ---- Dibuix del títol ---- */
static void drawTitleCredit(u8* v) {
    drawMessageLineColor(v,TITLE_CREDIT_X,TITLE_CREDIT_Y,menu_credit,(u8)(sizeof(menu_credit)-1u),0);
    /* Literal com HELP: mateix camí HUD (pen 4). */
    drawMessageLineColor(v,TITLE_YEAR_X,TITLE_CREDIT_Y,"2026",4,0);
}
/* El twinkle escriu 1 byte/estrella: no tocar la fila HUD del peu (crèdit + any). */
static u8 titleStarOverUiHud(u8 x, u8 y) {
    if((title_screen_mode==TS_MENU || title_screen_mode==TS_REDEFINE)
        && y==TITLE_MENU_RULE_Y && x>=14u && x<66u)
        return 1;
    if(title_screen_mode==TS_MENU && y==MENU_RULE_BOTTOM_Y && x>=14u && x<66u)
        return 1;
    if(title_screen_mode==TS_HELP
        && (y==TITLE_HELP_RULE_MID_Y || y==TITLE_HELP_RULE_BOT_Y) && x>=14u && x<66u)
        return 1;
    /* HOW TO: estrelles centrals poden caure sobre títol / línies / FIRE (veure H6 al log). */
    if(title_screen_mode==TS_HELP && x>=10u && x<70u) {
        if((y>=TITLE_HELP_TITLE_Y && y<(u8)(TITLE_HELP_TITLE_Y+HUD_GLYPH_H))
            || (y>=TITLE_HELP_L1_Y && y<(u8)(TITLE_HELP_L6_Y+HUD_GLYPH_H))
            || (y>=TITLE_HELP_FIRE_Y && y<(u8)(TITLE_HELP_FIRE_Y+HUD_GLYPH_H)))
            return 1;
    }
    /* Menú: twinkle lateral sobre files d’opció o FIRE (opcions pujades; més solapament). */
    if(title_screen_mode==TS_MENU && x>=12u && x<=78u) {
        if((y>=MENU_OPT1_Y && y<(u8)(MENU_OPT1_Y+HUD_GLYPH_H))
            || (y>=MENU_OPT2_Y && y<(u8)(MENU_OPT2_Y+HUD_GLYPH_H))
            || (y>=MENU_OPT3_Y && y<(u8)(MENU_OPT3_Y+HUD_GLYPH_H))
            || (y>=MENU_PROMPT_Y && y<(u8)(MENU_PROMPT_Y+HUD_GLYPH_H)))
            return 1;
    }
    /* Peu HUD: crèdit + any. */
    {
        u8 y0 = TITLE_CREDIT_Y;
        if(y < y0 || y >= (u8)(y0 + HUD_GLYPH_H)) return 0;
        if(x < (u8)(((sizeof(menu_credit) - 1u) * HUD_GLYPH_ADVANCE))) return 1;
        if(x >= TITLE_YEAR_X && x < (u8)(TITLE_YEAR_X + (4u * HUD_GLYPH_ADVANCE))) return 1;
    }
    return 0;
}
static u8 titleStarsUseCenter(void) {
    return title_screen_mode==TS_HISCORE_VIEW || title_screen_mode==TS_ATTRACT_SCORE
        || title_screen_mode==TS_WIN || title_screen_mode==TS_HELP;
}
static u8 titleStarColor(u8 state) {
    state &= 3u;
    if(state==0u) return B_BG;
    if(state==1u) return B_DOT_BLUE;
    if(state==2u) return B_DOT_BBLUE;
    return B_LINE_CYAN;
}
/* Centre (TOP 3 / attract): mateix ramp sense pic blanc — menys “llums”. */
static u8 titleStarColorCenter(u8 state) {
    state &= 3u;
    if(state==0u) return B_BG;
    if(state==1u) return B_DOT_BLUE;
    if(state==2u) return B_DOT_BBLUE;
    return B_LINE_CYAN;
}
static void drawOneTitleStar(u8* v, u8 i) {
    if(titleStarsUseCenter() && (title_star_x[i]<=4u || title_star_x[i]>=76u)) {
        *cpct_getScreenPtr(v,title_star_x[i],title_star_y[i]) = B_BG;
        return;
    }
    if(titleStarOverUiHud(title_star_x[i], title_star_y[i])) return;
    *cpct_getScreenPtr(v,title_star_x[i],title_star_y[i]) = titleStarColor(title_star_state[i]);
}
static void drawOneTitleCenterStar(u8* v, u8 i) {
    /* Victoria: no tapar files del logo. */
    if(title_screen_mode==TS_WIN && title_star_center_y[i]<32u) return;
    if(titleStarOverUiHud(title_star_center_x[i], title_star_center_y[i])) return;
    *cpct_getScreenPtr(v,title_star_center_x[i],title_star_center_y[i]) = titleStarColorCenter(title_star_center_state[i]);
}
static void drawTitleStars(u8* v) {
    u8 i,c;
    for(i=0;i<TITLE_SIDE_STAR_N;++i) {
        /* TOP 3 / victòria: menys pixels als marges extrems (evita competir amb el camp central). */
        if(titleStarsUseCenter() && (title_star_x[i]<=4u || title_star_x[i]>=76u)) continue;
        if(titleStarOverUiHud(title_star_x[i], title_star_y[i])) continue;
        c = titleStarColor(title_star_state[i]);
        *cpct_getScreenPtr(v,title_star_x[i],title_star_y[i]) = c;
    }
    if(titleStarsUseCenter()) {
        for(i=0;i<TITLE_CENTER_STAR_N;++i) {
            if(title_screen_mode==TS_WIN && title_star_center_y[i]<32u) continue;
            if(titleStarOverUiHud(title_star_center_x[i], title_star_center_y[i])) continue;
            c = titleStarColorCenter(title_star_center_state[i]);
            *cpct_getScreenPtr(v,title_star_center_x[i],title_star_center_y[i]) = c;
        }
    }
}
static void drawTitleBg(u8* v) {
    cpct_drawSolidBox(v,B_BG,64,SCR_H);
    cpct_drawSolidBox(cpct_getScreenPtr(v,64,0),B_BG,16,SCR_H);
    drawTitleStars(v);
}
static void drawTitleLogo(u8* v) {
    title_logo_draw_masked_with_rim(v);
}
/* Pantalla victòria (congratulations). */
static void drawTitleWinLayout(u8* v) {
    drawTitleLogo(v);
    cpct_drawSolidBox(cpct_getScreenPtr(v,10,52),B_LINE_RED,60,1);
    drawMessageLineColor(v,(u8)((80-(15*HUD_GLYPH_ADVANCE))>>1),60,"CONGRATULATIONS",15,1);
    drawMessageLineColor(v,(u8)((80-(20*HUD_GLYPH_ADVANCE))>>1),74,"THE INVASION IS OVER",20,1);
    drawMessageLineColor(v,(u8)((80-(21*HUD_GLYPH_ADVANCE))>>1),88,"THANK YOU FOR PLAYING",21,1);
    cpct_drawSolidBox(cpct_getScreenPtr(v,10,100),B_LINE_RED,60,1);
    drawMessageLineColor(v,(u8)((80-(18*HUD_GLYPH_ADVANCE))>>1),160,win_fire_prompt,(u8)(sizeof(win_fire_prompt)-1u),1);
    drawTitleCredit(v);
}
static void drawTitleHelpLayout(u8* v) {
    drawMessageLineColor(v,(u8)((80-((sizeof(help_title)-1u)*HUD_GLYPH_ADVANCE))>>1),TITLE_HELP_TITLE_Y,help_title,(u8)(sizeof(help_title)-1u),1);
    cpct_drawSolidBox(cpct_getScreenPtr(v,14,TITLE_HELP_RULE_MID_Y),B_LINE_CYAN,52,1);
    drawMessageLineColor(v,(u8)((80-((sizeof(help_l1)-1u)*HUD_GLYPH_ADVANCE))>>1),TITLE_HELP_L1_Y,help_l1,(u8)(sizeof(help_l1)-1u),0);
    drawMessageLineColor(v,(u8)((80-((sizeof(help_l2)-1u)*HUD_GLYPH_ADVANCE))>>1),TITLE_HELP_L2_Y,help_l2,(u8)(sizeof(help_l2)-1u),0);
    drawMessageLineColor(v,(u8)((80-((sizeof(help_l3)-1u)*HUD_GLYPH_ADVANCE))>>1),TITLE_HELP_L3_Y,help_l3,(u8)(sizeof(help_l3)-1u),0);
    drawMessageLineColor(v,(u8)((80-((sizeof(help_l4)-1u)*HUD_GLYPH_ADVANCE))>>1),TITLE_HELP_L4_Y,help_l4,(u8)(sizeof(help_l4)-1u),0);
    drawMessageLineColor(v,(u8)((80-((sizeof(help_l5)-1u)*HUD_GLYPH_ADVANCE))>>1),TITLE_HELP_L5_Y,help_l5,(u8)(sizeof(help_l5)-1u),0);
    drawMessageLineColor(v,(u8)((80-((sizeof(help_l6)-1u)*HUD_GLYPH_ADVANCE))>>1),TITLE_HELP_L6_Y,help_l6,(u8)(sizeof(help_l6)-1u),0);
    cpct_drawSolidBox(cpct_getScreenPtr(v,14,TITLE_HELP_RULE_BOT_Y),B_LINE_CYAN,52,1);
    drawMessageLineColor(v,(u8)((80-((sizeof(help_fire)-1u)*HUD_GLYPH_ADVANCE))>>1),TITLE_HELP_FIRE_Y,help_fire,(u8)(sizeof(help_fire)-1u),1);
    drawTitleCredit(v);
}
static void drawTitleScreen(u8* v) {
    drawTitleBg(v);
    if(title_screen_mode==TS_HISCORE_VIEW) {
        cpct_drawSolidBox(cpct_getScreenPtr(v,10,42),B_LINE_RED,60,1);
        drawMessageLineColor(v,28,48,"GAME OVER",9,1);
        cpct_drawSolidBox(cpct_getScreenPtr(v,10,60),B_LINE_RED,60,1);
        drawMessageLineColor(v,32,74,"TOP 3",5,1);
        cpct_drawSolidBox(cpct_getScreenPtr(v,14,88),B_LINE_CYAN,52,1);
        drawHiScoreTable(v,96);
        cpct_drawSolidBox(cpct_getScreenPtr(v,14,142),B_LINE_CYAN,52,1);
        drawMessageLineColor(v,20,164,"PRESS ANY KEY",13,1);
    } else if(title_screen_mode==TS_ATTRACT_SCORE) {
        drawMessageLineColor(v,32,74,"TOP 3",5,1);
        cpct_drawSolidBox(cpct_getScreenPtr(v,14,88),B_LINE_CYAN,52,1);
        drawHiScoreTable(v,96);
        cpct_drawSolidBox(cpct_getScreenPtr(v,14,142),B_LINE_CYAN,52,1);
        drawTitleCredit(v);
    } else if(title_screen_mode==TS_HELP) {
        drawTitleHelpLayout(v);
    } else if(title_screen_mode==TS_HISCORE_INPUT) {
        drawHiScoreInput(v);
    } else if(title_screen_mode==TS_WIN) {
        drawTitleWinLayout(v);
    } else {
        drawTitleLogo(v);
        cpct_drawSolidBox(cpct_getScreenPtr(v,14,TITLE_MENU_RULE_Y),B_LINE_CYAN,52,1);
        if(title_screen_mode==TS_MENU) {
            drawMessageLineColor(v,25,MENU_OPT1_Y,menu_opt1,sizeof(menu_opt1)-1,control_mode==CTRL_JOYSTICK);
            drawMessageLineColor(v,25,MENU_OPT2_Y,menu_opt2,sizeof(menu_opt2)-1,control_mode==CTRL_KEYBOARD);
            drawMessageLineColor(v,25,MENU_OPT3_Y,menu_opt3,sizeof(menu_opt3)-1,0);
            cpct_drawSolidBox(cpct_getScreenPtr(v,14,MENU_RULE_BOTTOM_Y),B_LINE_CYAN,52,1);
            drawMessageLineColor(v,MENU_PROMPT_X,MENU_PROMPT_Y,menu_play,sizeof(menu_play)-1,1);
            drawTitleCredit(v);
        } else {
            u8 rs=(redefine_step<7)?redefine_step:6;
            const char* step=redef_names[rs];
            u8 len=redef_lens[rs];
            drawMessageLineColor(v,28,106,redef_title,sizeof(redef_title)-1,0);
            drawMessageLineColor(v,(u8)((80-len*HUD_GLYPH_ADVANCE)>>1),128,step,len,1);
            drawMessageLineColor(v,23,146,redef_msg,sizeof(redef_msg)-1,0);
        }
    }
}
static void drawTitleBoth(void) {
    drawTitleScreen(SCR_PAGE_C0);
    drawTitleScreen(SCR_PAGE_80);
    cpct_waitVSYNC();
    cpct_setVideoMemoryPage(cpct_pageC0);
}
static void tickTitleTwinkle(void) {
    if(++title_twinkle_tick >= TITLE_TWINKLE_FRAMES) {
        u8 i, j, k, k2;
        title_twinkle_tick = 0;
        title_twinkle_rng = (u8)(title_twinkle_rng * 17u + 29u);
        i=title_twinkle_rng;
        while(i>=TITLE_SIDE_STAR_N)i=(u8)(i-TITLE_SIDE_STAR_N);
        title_star_state[i] = (u8)((title_star_state[i] + 1u) & 3u);
        drawOneTitleStar(SCR_PAGE_C0, i);
        drawOneTitleStar(SCR_PAGE_80, i);
        title_twinkle_rng = (u8)(title_twinkle_rng * 17u + 29u);
        j=title_twinkle_rng;
        while(j>=TITLE_SIDE_STAR_N)j=(u8)(j-TITLE_SIDE_STAR_N);
        if(j==i){j=(u8)(i+17u);while(j>=TITLE_SIDE_STAR_N)j=(u8)(j-TITLE_SIDE_STAR_N);}
        title_star_state[j] = (u8)((title_star_state[j] + 1u) & 3u);
        drawOneTitleStar(SCR_PAGE_C0, j);
        drawOneTitleStar(SCR_PAGE_80, j);
        if(titleStarsUseCenter()) {
            title_twinkle_rng = (u8)(title_twinkle_rng * 17u + 29u);
            k=title_twinkle_rng;
            while(k>=TITLE_CENTER_STAR_N)k=(u8)(k-TITLE_CENTER_STAR_N);
            title_star_center_state[k] = (u8)((title_star_center_state[k] + 1u) & 3u);
            drawOneTitleCenterStar(SCR_PAGE_C0, k);
            drawOneTitleCenterStar(SCR_PAGE_80, k);
            title_twinkle_rng = (u8)(title_twinkle_rng * 17u + 29u);
            k2=title_twinkle_rng;
            while(k2>=TITLE_CENTER_STAR_N)k2=(u8)(k2-TITLE_CENTER_STAR_N);
            if(k2==k && TITLE_CENTER_STAR_N>1u){k2=(u8)(k+7u);while(k2>=TITLE_CENTER_STAR_N)k2=(u8)(k2-TITLE_CENTER_STAR_N);}
            title_star_center_state[k2] = (u8)((title_star_center_state[k2] + 1u) & 3u);
            drawOneTitleCenterStar(SCR_PAGE_C0, k2);
            drawOneTitleCenterStar(SCR_PAGE_80, k2);
        }
    }
}

static void enterTitle(void) {
    u8 i;
    resetVideoPages();
    cpct_setPalette((u8*)loading_palette_hw,16);
    soundStopAll();
    title_twinkle_tick = 0;
    title_twinkle_rng = 0x5Au;
    for(i=0;i<TITLE_SIDE_STAR_N;++i) title_star_state[i] = title_star_seed[i];
    for(i=0;i<TITLE_CENTER_STAR_N;++i) title_star_center_state[i] = title_star_center_seed[i];
    attract_cycle = 0;
}

static void playMenuCoin(void) {
    u8 i;
    static const u8 coin_a[16] = {239,239,239,190,190,190,160,160,160,119,119,119,119,119,119,119};
    static const u8 coin_b[16] = {190,190,190,160,160,160,119,119,119, 95, 95, 95, 95, 95, 95, 95};
    static const u8 coin_c[16] = {160,160,160,119,119,119, 95, 95, 95, 80, 80, 80, 80, 80, 80, 80};
    psgWrite(7, 0x38);
    /* Coin/start arcade curt: arpegi ascendent de tres canals abans de saltar a GAME.BIN. */
    for(i=0; i<16; ++i) {
        u8 vol = (i<12u) ? 15u : 8u;
        psgWrite(0, coin_a[i]); psgWrite(1, 0); psgWrite(8, vol);
        psgWrite(2, coin_b[i]); psgWrite(3, 0); psgWrite(9, (u8)(vol-3u));
        psgWrite(4, coin_c[i]); psgWrite(5, 0); psgWrite(10, (u8)(vol-6u));
        tickTitleTwinkle();
        cpct_waitVSYNC();
    }
}

static void nau_shared_consume_resume(void) {
    /* Un cop sortim del post-partida (o anem a jugar), no tornar a llegir com a “resume” en un nou main(). */
    g_shared->from_game = NAU_FROM_FRESH;
}

static void returnToMenuReset(void) {
    title_screen_mode=TS_MENU;
    attract_frm=0;
    attract_cycle=0;
    title_dirty=1;
    title_phase=0;
    hs_view_release_wait=0;
    nau_shared_consume_resume();
}

static void enterPostGameHiScore(void) {
    hs_pos = hs_isTop(last_score, last_level);
    if(last_score > 0u && hs_pos < 3u) {
        initHiScoreInput();
        title_screen_mode = TS_HISCORE_INPUT;
    } else {
        title_screen_mode = TS_HISCORE_VIEW;
    }
    title_dirty = 1;
    title_phase = 0;
    hs_view_release_wait = 0;
}

/* ---- Transició a GAME.BIN ---- */
static void goToGame(void) {
    /* Escriure configuració de controls i hi-scores al shared state */
    g_shared->control_mode = control_mode;
    g_shared->key_left     = key_left;
    g_shared->key_right    = key_right;
    g_shared->key_up       = key_up;
    g_shared->key_down     = key_down;
    g_shared->key_fire     = key_fire;
    g_shared->key_pause    = key_pause;
    g_shared->key_quit     = key_quit;
    nau_shared_consume_resume();
    syncHiScoresToShared();
    soundStopAll();
    nau_transit_load_game();
}

/* =========================================================================
 * main() — MENU.BIN
 * ========================================================================= */
void main(void) {
    u16 nau_firmware_jp_save;

    cpct_setStackLocation((void*)0xBFFE);
    /* Després d’AMSDOS/MC_START el Gate Array pot deixar mapa 128K no estàndard: &C000 no és VRAM. */
    dx_memory_default();
    /* INTRO overscan deixa el 6845 fora de mode 0 estàndard; fer-ho aquí (DI curt) evita barallar IRQ amb el loader. */
    nau_crtc_apply_mode0();

    {
        volatile u8* p = (volatile u8*)(void*)0x0038u;
        if(p[0] == 0xC3u)
            nau_firmware_jp_save = cpct_disableFirmware();
        else {
            (void)cpct_disableFirmware();
            if(g_shared->magic == NAU_SHARED_MAGIC && g_shared->firmware_jp != 0u)
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
    hud_buildFont();
    resetVideoPages();
    soundInit();

    /* Inicialitzar o restaurar shared state */
    if(g_shared->magic != NAU_SHARED_MAGIC || !nau_shared_payload_plausible()) {
        /* Primera arrencada o RAM amb magic casual però camps incoherents */
        g_shared->magic        = NAU_SHARED_MAGIC;
        g_shared->from_game    = NAU_FROM_FRESH;
        g_shared->score        = 0;
        g_shared->level        = 1;
        g_shared->game_result  = NAU_RESULT_GAMEOVER;
        g_shared->control_mode = CTRL_JOYSTICK;
        g_shared->key_left     = Key_CursorLeft;
        g_shared->key_right    = Key_CursorRight;
        g_shared->key_up       = Key_CursorUp;
        g_shared->key_down     = Key_CursorDown;
        g_shared->key_fire     = Key_Z;
        g_shared->key_pause    = Key_P;
        g_shared->key_quit     = Key_Return;
        initHiScores();
        syncHiScoresToShared();
        title_screen_mode = TS_MENU;
    } else {
        /* Ve d'una partida acabada o d'un reinici: restaurar */
        control_mode = g_shared->control_mode;
        key_left     = g_shared->key_left;
        key_right    = g_shared->key_right;
        key_up       = g_shared->key_up;
        key_down     = g_shared->key_down;
        key_fire     = g_shared->key_fire;
        key_pause    = g_shared->key_pause;
        key_quit     = g_shared->key_quit;
        syncHiScoresFromShared();

        if(g_shared->from_game == NAU_FROM_GAME) {
            last_score = g_shared->score;
            last_level = g_shared->level;
            if(g_shared->game_result == NAU_RESULT_WIN) {
                title_screen_mode = TS_WIN;
                attract_frm = 0;
            } else {
                enterPostGameHiScore();
            }
        } else {
            title_screen_mode = TS_MENU;
        }
    }

    cpct_setPalette((u8*)loading_palette_hw,16);
    enterTitle();

    /* ---- Bucle principal (GS_TITLE) ---- */
    while(1) {
        cpct_scanKeyboard_f();

        if(title_dirty) {
            drawTitleBoth();
            title_dirty=0;
            blink_ctr=0;
        }

        tickTitleTwinkle();

        if((title_screen_mode==TS_ATTRACT_SCORE || title_screen_mode==TS_HELP) && menuFire()) {
            returnToMenuReset();
        } else if(title_screen_mode==TS_MENU || title_screen_mode==TS_ATTRACT_SCORE || title_screen_mode==TS_HELP) {
            attract_frm++;
            if(attract_frm>=ATTRACT_HOLD_FRAMES) {
                attract_frm=0;
                if(title_screen_mode==TS_MENU) {
                    if(attract_cycle==0u) {
                        title_screen_mode=TS_ATTRACT_SCORE;
                        attract_cycle=1u;
                    } else {
                        title_screen_mode=TS_HELP;
                        attract_cycle=3u;
                    }
                } else if(title_screen_mode==TS_ATTRACT_SCORE) {
                    title_screen_mode=TS_MENU;
                    attract_cycle=2u;
                } else {
                    title_screen_mode=TS_MENU;
                    attract_cycle=0u;
                }
                title_dirty=1; title_phase=0;
            }
        }

        if(title_screen_mode==TS_HISCORE_VIEW) {
            if(title_phase==0) {
                if(!cpct_isAnyKeyPressed_f()) {
                    title_phase=1;
                    hs_view_release_wait=0;
                } else if(++hs_view_release_wait >= HS_VIEW_RELEASE_MAX_FRAMES) {
                    /* Failsafe: evita bloqueig etern si alguna tecla/entrada queda clavada. */
                    title_phase=1;
                    hs_view_release_wait=0;
                }
            } else {
                if(cpct_isAnyKeyPressed_f()) {
                    cpct_setPalette((u8*)loading_palette_hw,16);
                    returnToMenuReset();
                }
            }
        } else if(title_screen_mode==TS_WIN) {
            if(attract_frm<35u) {
                ++attract_frm;
            } else if(menuFire()) {
                playMenuCoin();
                soundStopAll();
                enterPostGameHiScore();
            }
            /* Segona línia amb puls suau (mateix ritme que FIRE TO START). */
            if(attract_frm>=35u) {
                u8 pb=(blink_ctr&0x10)?1:0, cb, wx;
                ++blink_ctr;
                cb=(blink_ctr&0x10)?1:0;
                if(pb!=cb) {
                    wx=(u8)((80-(21*HUD_GLYPH_ADVANCE))>>1);
                    if(cb) {
                        cpct_drawSolidBox(cpct_getScreenPtr(SCR_PAGE_C0,wx,88),B_BG,(u8)(21*HUD_GLYPH_ADVANCE),HUD_GLYPH_H);
                        cpct_drawSolidBox(cpct_getScreenPtr(SCR_PAGE_80,wx,88),B_BG,(u8)(21*HUD_GLYPH_ADVANCE),HUD_GLYPH_H);
                    } else {
                        drawMessageLineColor(SCR_PAGE_C0,wx,88,"THANK YOU FOR PLAYING",21,1);
                        drawMessageLineColor(SCR_PAGE_80,wx,88,"THANK YOU FOR PLAYING",21,1);
                    }
                }
            }
        } else if(title_screen_mode==TS_HISCORE_INPUT) {
            updateHiScoreInput();
            drawHiScoreInputRow(SCR_PAGE_C0);
            drawHiScoreInputRow(SCR_PAGE_80);
        } else if(title_screen_mode==TS_ATTRACT_SCORE || title_screen_mode==TS_HELP) {
            /* Toggle per temps i FIRE→MENU (gestionat a dalt). */
        } else if(title_screen_mode==TS_MENU) {
            if(title_phase==0) {
                if(!(cpct_isKeyPressed(Key_1)||cpct_isKeyPressed(Key_2)||cpct_isKeyPressed(Key_3)||menuFire()))
                    title_phase=1;
            } else {
                if(cpct_isKeyPressed(Key_1)) {
                    if(control_mode!=CTRL_JOYSTICK) sfxMenuSelect();
                    control_mode=CTRL_JOYSTICK; title_dirty=1; title_phase=0;
                } else if(cpct_isKeyPressed(Key_2)) {
                    if(control_mode!=CTRL_KEYBOARD) sfxMenuSelect();
                    control_mode=CTRL_KEYBOARD; title_dirty=1; title_phase=0;
                } else if(cpct_isKeyPressed(Key_3)) {
                    sfxMenuSelect();
                    title_screen_mode=TS_REDEFINE; redefine_step=0; title_dirty=1; title_phase=0;
                } else if(menuFire()) {
                    /* Feedback immediat abans de la càrrega llarga MENU->GAME. */
                    cpct_drawSolidBox(cpct_getScreenPtr(SCR_PAGE_C0,MENU_PROMPT_X,MENU_PROMPT_Y),B_BG,39,8);
                    cpct_drawSolidBox(cpct_getScreenPtr(SCR_PAGE_80,MENU_PROMPT_X,MENU_PROMPT_Y),B_BG,39,8);
                    drawMessageLineColor(SCR_PAGE_C0,menu_loading_x,MENU_PROMPT_Y,menu_loading,sizeof(menu_loading)-1,1);
                    drawMessageLineColor(SCR_PAGE_80,menu_loading_x,MENU_PROMPT_Y,menu_loading,sizeof(menu_loading)-1,1);
                    /* Els tres punts finals no existeixen al HUD font: els pintem manualment. */
                    cpct_drawSolidBox(cpct_getScreenPtr(SCR_PAGE_C0,(u8)(menu_loading_x+(sizeof(menu_loading)-1u)*HUD_GLYPH_ADVANCE+1u),(u8)(MENU_PROMPT_Y+6u)),B_LINE_CYAN,1,1);
                    cpct_drawSolidBox(cpct_getScreenPtr(SCR_PAGE_C0,(u8)(menu_loading_x+(sizeof(menu_loading)-1u)*HUD_GLYPH_ADVANCE+4u),(u8)(MENU_PROMPT_Y+6u)),B_LINE_CYAN,1,1);
                    cpct_drawSolidBox(cpct_getScreenPtr(SCR_PAGE_C0,(u8)(menu_loading_x+(sizeof(menu_loading)-1u)*HUD_GLYPH_ADVANCE+7u),(u8)(MENU_PROMPT_Y+6u)),B_LINE_CYAN,1,1);
                    cpct_drawSolidBox(cpct_getScreenPtr(SCR_PAGE_80,(u8)(menu_loading_x+(sizeof(menu_loading)-1u)*HUD_GLYPH_ADVANCE+1u),(u8)(MENU_PROMPT_Y+6u)),B_LINE_CYAN,1,1);
                    cpct_drawSolidBox(cpct_getScreenPtr(SCR_PAGE_80,(u8)(menu_loading_x+(sizeof(menu_loading)-1u)*HUD_GLYPH_ADVANCE+4u),(u8)(MENU_PROMPT_Y+6u)),B_LINE_CYAN,1,1);
                    cpct_drawSolidBox(cpct_getScreenPtr(SCR_PAGE_80,(u8)(menu_loading_x+(sizeof(menu_loading)-1u)*HUD_GLYPH_ADVANCE+7u),(u8)(MENU_PROMPT_Y+6u)),B_LINE_CYAN,1,1);
                    cpct_setVideoMemoryPage(frontPage);
                    playMenuCoin();
                    soundStopAll();
                    goToGame();
                    /* goToGame() no retorna mai (TRANSIT salta a GAME.BIN) */
                }
            }
            /* Efecte parpelleig "FIRE TO START" */
            {
                u8 pb=(blink_ctr&0x10)?1:0, cb;
                ++blink_ctr;
                cb=(blink_ctr&0x10)?1:0;
                if(pb!=cb) {
                    if(cb) {
                        cpct_drawSolidBox(cpct_getScreenPtr(SCR_PAGE_C0,MENU_PROMPT_X,MENU_PROMPT_Y),B_BG,39,8);
                        cpct_drawSolidBox(cpct_getScreenPtr(SCR_PAGE_80,MENU_PROMPT_X,MENU_PROMPT_Y),B_BG,39,8);
                    } else {
                        drawMessageLineColor(SCR_PAGE_C0,MENU_PROMPT_X,MENU_PROMPT_Y,menu_play,sizeof(menu_play)-1,1);
                        drawMessageLineColor(SCR_PAGE_80,MENU_PROMPT_X,MENU_PROMPT_Y,menu_play,sizeof(menu_play)-1,1);
                    }
                }
            }
        } else {
            /* TS_REDEFINE */
            if(title_phase==0) {
                if(!cpct_isAnyKeyPressed_f()) title_phase=1;
            } else {
                cpct_keyID pressed = firstPressedKey();
                if(pressed) {
                    sfxMenuSelect();
                    switch(redefine_step) {
                        case 0: key_left=pressed;  break;
                        case 1: key_right=pressed; break;
                        case 2: key_up=pressed;    break;
                        case 3: key_down=pressed;  break;
                        case 4: key_fire=pressed;  break;
                        case 5: key_pause=pressed; break;
                        default: key_quit=pressed; break;
                    }
                    ++redefine_step;
                    title_phase=0; title_dirty=1;
                    if(redefine_step>=7) {
                        control_mode=CTRL_KEYBOARD;
                        title_screen_mode=TS_MENU;
                        redefine_step=0; attract_frm=0; attract_cycle=0;
                    }
                }
            }
        }

        soundTick();
        cpct_waitVSYNC();
    }
}
