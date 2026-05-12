# SDO MSX - NAU DX Port CPC -> MSX1

Port del joc NAU DX (Amstrad CPC) a MSX1 usant la llibreria MSXgl.

## Estat actual

- Sprite de la nau (blanc + vermell, 2 sprites compostos)
- Starfield de 3 capes (lent/mig/ràpid) amb tiles
- Columnes laterals amb scroll de roca (tiles animats)
- Area de joc: 144px (columna 16px + joc 144px + columna 16px + HUD 80px)
- 5 tipus d'enemics (basic, fast, heavy, diver, bomber) amb sprites 1:1 CPC
- Sistema d'onades (wave system) 1:1 CPC: 25 nivells, 5 biomes
- Dispars del jugador i dispars enemics amb punteria
- Col·lisions: bala-enemic, nau-enemic, nau-dispars
- Explosions (4 frames enemic, 6 frames nau) 1:1 CPC
- HUD: score, level, lives amb font HUD custom (verd/blanc)
- Respawn + invulnerabilitat temporal
- Game over + pantalla de resultat
- Menú complet 1:1 CPC:
  - Logo amb colors (blau fosc/blau clar/verd)
  - Starfield amb twinkle 1:1 CPC
  - Opcions: 1 JOYSTICK / 2 KEYBOARD / 3 SET KEYS
  - SET KEYS: redefinició de 5 tecles
  - HOW TO PLAY
  - TOP 3 hi-scores amb entrada de nom
  - Mode attract: MENU -> TOP3 -> HOW TO PLAY -> MENU
  - CONGRATULATIONS (victòria nivell 25)

## Bugs coneguts

- Parpelleig d'invulnerabilitat post-respawn no visible

## Pendent

- Boss (nivells 5, 10, 15, 20, 25)
- Sons / música
- Biomes (canvi de colors per paleta segons nivell)
- Pantalla de títol animada (intro)

## Entorn de compilació

- MSXgl a `C:\msxgl`
- SDCC 4.2.0
- Node.js 18.x

### Compilar

```bat
cd C:\msxgl\projects\nau_dx
build.bat
```

ROM generada a `out\nau_dx.rom`

### Testar

Carregar `nau_dx.rom` manualment a openMSX (Media > Cartridge Slot A).

## Referència

- Codi original CPC a `C:\msx-game\cpc\nau_dx\nau_dx-main`
- Sprites: `ship_sprite.c`, `enemy_sprite.c`, etc.
- Lògica de joc: `src_split\main_game.c`
- Menú: `src_split\main_menu.c`
