# SDO MSX - NAU DX Port CPC -> MSX1

Port del joc **NAU DX** (Amstrad CPC) a **MSX1** usant la llibreria MSXgl.

## Resum del Projecte

Joc d'acció vertical per MSX1 (TMS9918, Screen 2) amb:
- **25 nivells** amb 5 tipus d'enemics i 5 biomes visuals
- **5 bosses** (nivells 5, 10, 15, 20, 25) amb patrons de foc variats
- **Sistema de puntuació** amb TOP 3 i entrada de nom
- **Bonus** per completar onades sense perdre vides
- **Vida extra** cada 5000 punts

## Controls

| Accio | Joystick | Teclat |
|-------|----------|--------|
| Moure | D-pad | Fletxes / WASD |
| Disparar | Botó A | Espai / Z |
| Pausa | Botó B | P |
| Sortir | - | Q / Retorn |

## Arquitectura ROM

### Cartutx complet (64KB ASCII-16)
```
Segment 0 (0x8000-0xBFFF): Intro (Screen 2, musica PSG)
Segment 1 (0x4000-0x7FFF): Joc part 1 (game_p1.bin)
Segment 2 (0x8000-0xBFFF): Joc part 2 (game_p2.bin)
```

### Joc sol (32KB plain ROM)
- Entry point: `0x4014`
- Header: `AB 14 40 00 00 00 00 00`
- Es pot carregar directament en emuladors com plain ROM a `0x4000`

## Com Compilar

### Requits
- **MSXgl** a `C:\msxgl`
- **SDCC** 4.2.0 (inclou MSXgl)
- **Node.js** 18.x (MSXgl build tool)

### Compilar joc (32KB plain ROM)
```bat
cd C:\msxgl\projects\nau_dx
copy project_config_game.js project_config.js
build.bat
```
Output: `out\nau_dx.rom` (32KB)

### Compilar cartutx complet (64KB amb intro)
```bat
cd C:\msxgl\projects\nau_dx

:: 1. Compilar joc
copy project_config_game.js project_config.js
build.bat

:: 2. Split joc en dues parts
:: (manual: copiar bytes 0-16383 a game_p1.bin, 16384-32767 a game_p2.bin)

:: 3. Compilar intro amb joc incrustat
copy project_config_intro.js project_config.js
build.bat
```
Output: `out\nau_dx_intro.rom` (64KB ASCII-16 cartridge)

### Fitxers de configuracio
| Fitxer | Descripcio |
|--------|-----------|
| `project_config_game.js` | Config per joc sol (ROM_32K) |
| `project_config_intro.js` | Config per cartutx complet (ROM_ASCII16, 64KB) |

## Layout de Pantalla (Screen 2)

```
[0-1]  [2-19]         [20-21] [22-31]
Paret  Area de joc    Paret   HUD
16px   160px          16px    64px
```

### HUD (columnes 25-31)
| Element | Row | Descripcio |
|---------|-----|-----------|
| SCORE | 1-2 | Label + valor (5 digits) |
| BONUS | 5-6 | Label + XN (visible temporalment) |
| LEVEL | 9-10 | Label + valor (2 digits) |
| LIVES | 12-13 | Label + valor |
| HP Boss 1 | 17 | Label + barra 5 segments |
| HP Boss 2 | 19 | Label + barra 5 segments (dual boss) |

## VRAM Layout

| Range | Us |
|-------|-----|
| 0x0000-0x17FF | Pattern table (tiles) |
| 0x2000-0x37FF | Color table |
| 0x1800-0x1AFF | Name table (layout) |
| 0x1B00-0x1BFF | Sprite attribute table |

### Tile Assignments
| Tile | Us |
|------|-----|
| 1-80 | Logo (16x5 tiles, 128x40px) |
| 90-97 | Wall scroll (8 tiles animats) |
| 98-99 | UI elements (barra HP) |
| 100-173 | HUD font (verd/blanc) |
| 174-181 | Menu star tiles (twinkle states) |
| 182-189 | Game star layer 1 (blau fosc) |
| 190-197 | Game star layer 2 (blau clar) |
| 198-205 | Game star layer 3 (blanc) |

## Features Implementades

### Joc
- [x] Nau jugador (2 sprites: blanc + vermell)
- [x] Starfield de 3 capes amb parallax
- [x] Columnes laterals amb scroll de roca animat
- [x] 5 tipus d'enemics amb colors diferenciats:
  - Basic: cyan
  - Fast: cyan
  - Heavy/Tank: gris metallic
  - Diver: verd
  - Bomber: blanc
- [x] 5 bosses amb forma custom i patrons de foc variats
- [x] Sistema d'onades 1:1 CPC (25 nivells, 5 biomes)
- [x] Dispars enemics amb punteria
- [x] Col·lisions completes
- [x] Explosions multi-frame
- [x] HUD complet amb boss HP bars (single + dual)
- [x] Bonus per onada
- [x] Vida extra cada 5000 punts
- [x] Game over + hi-score input
- [x] Pantalla CONGRATULATIONS (victoria)

### Menu
- [x] Logo amb colors per tile
- [x] Starfield per-screen (MENU, TOP3, GAME OVER, HELP)
- [x] Opcions: JOYSTICK / KEYBOARD / SET KEYS
- [x] Redefinicio de tecles (7 accions)
- [x] HOW TO PLAY
- [x] TOP 3 hi-scores
- [x] Mode attract ciclic
- [x] Pausa

### Intro
- [x] Screen 2 amb art custom
- [x] Musica PSG sincronitzada
- [x] Salt al joc amb trampoline ASCII-16

## Dificultat Balancejada

| Nivell | Boss Tier | HP Boss | Velocitat Bales |
|--------|-----------|---------|-----------------|
| 1-9 | 0 | 15 | 3 |
| 10-14 | 1 | 20 | 4 |
| 15-19 | 2 | 25 | 5 |
| 20-24 | 3 | 30 | **5** (capada) |
| 25 | 4 | 35 | **5** (capada) |

A partir del nivell 15, la dificultat ve dels **HP extra** del boss, no de la velocitat de les bales.

## ROM Size

| Segment | Size |
|---------|------|
| CODE | ~31,900 bytes |
| DATA | ~715 bytes |
| **Total** | **~32,615 de 32,768** (~150 bytes lliures) |

## Credits

- Joc original CPC: **Xevimet4l** (2026)
- Port MSX1: **ND**
- Engine: **MSXgl**
- Compiler: **SDCC**

## Referencia

- Codi original CPC a `C:\msx-game\cpc\nau_dx\nau_dx-main`
- Output: `C:\msx-game\output\nau_dx.rom`
