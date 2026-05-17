# Nau DX — MSX1 Port

## Game Overview

**Nau DX** is a vertical-scrolling shoot-em-up ported from Amstrad CPC to MSX1 (TMS9918, Screen 2).

### Gameplay

- **25 levels** across 5 biomes (Rocky, Ice, Forest, Fire, Tech)
- **5 enemy types**: Basic, Fast, Heavy, Bomber, Diver
- **Boss fights** at levels 5, 10, 15, 20, 25
- **Hi-score system** with 3-letter name input
- **Extra life** every 5000 points

### Controls

| Action | Joystick (default) | Keyboard |
|--------|-------------------|----------|
| Move | D-pad | Arrow keys |
| Fire | Button 1 | Z |
| Pause | Button 2 | P |
| Quit | — | Q |

### Menu

- **1 JOYSTICK** — use joystick port 1 (default)
- **2 KEYBOARD** — use keyboard arrows + Z
- **3 SET KEYS** — redefine keyboard keys
- **FIRE TO START** — start game (only works in selected mode)

**Important**: The menu fire button is mode-specific:
- Joystick mode → only joystick fire works
- Keyboard mode → only Z or SPACE works

### Biome Palettes

Wall tiles change color per biome (CPC palette indices 8,9). Star colors are fixed.

### HUD Layout

```
Row 0: SCORE (label)
Row 1: 00000  (value)
Row 3: BONUS (label, temporary)
Row 4: Xn    (wave bonus multiplier)
Row 6: LEVEL (label)
Row 7: nn    (value)
Row 9: LIVES (label)
Row 10: n    (value)
Row 13: HP + bar (boss only)
```

---

## Architecture

### Cartridge Layout (64KB ASCII16)

```
0x0000-0x3FFF  Segment 0: Intro code + CRT0 header
0x4000-0x7FFF  Bank 0 (fixed at boot): Intro code (~2KB)
0x8000-0xBFFF  Bank 1 (default): intro_sc2_raw.bin (16KB)
               (switched to segment 3 at runtime)
0xC000-0xFFFF  RAM: trampoline code, game stack/vars
```

### Game Binary (32KB ROM_32K)

The game compiles as a flat 32KB ROM, then split into two 16KB parts:
- `game_p1.bin` = bytes 0-16383 (mapped to segment 2, bank 0)
- `game_p2.bin` = bytes 16384-32767 (mapped to segment 3, bank 1)

### Intro → Game Transition

1. Intro displays SC2 image and plays music loop
2. User presses SPACE, Z, or joystick fire
3. Intro copies a **RAM trampoline** to 0xC000:
   ```asm
   LD A, 2         ; segment 2 = game_p1.bin
   LD (0x6000), A  ; switch bank 0
   LD A, 3         ; segment 3 = game_p2.bin
   LD (0x77FF), A  ; switch bank 1
   JP 0x4014       ; jump to game entry (after CRT0)
   ```
4. Trampoline runs from RAM (safe bank switch), jumps to game

### VRAM Tile Layout (Screen 2)

| Range | Purpose |
|-------|---------|
| 0-89 | Empty / available |
| 90-97 | Wall tiles (8 phases) |
| 100-123 | Title logo |
| 124 | Horizontal line tile |
| 125 | Bar fill tile (HP bar) |
| 130-166 | HUD font (normal, green) |
| 167-203 | HUD font (highlight, white) |
| 204-211 | Menu star states (3 colors) |
| 212-219 | Game star layer 1 (slow) |
| 220-227 | Game star layer 2 (medium) |
| 228-235 | Game star layer 3 (fast) |

### Sprite Layout (32 sprites, 16x16)

| Sprite | Purpose |
|--------|---------|
| 0-1 | Player ship (white + red layers) |
| 2-3 | Ship thrusters (when moving) |
| 4-7 | Player shots |
| 8-27 | Enemies (white layers first, red layers last) |
| 28+ | Explosions, enemy shots |

### Bank Switching (ASCII16)

- `LD (0x6000), A` → switch segment `A` into bank 0 (0x4000-0x7FFF)
- `LD (0x77FF), A` → switch segment `A` into bank 1 (0x8000-0xBFFF)
- **Never switch bank 0 while running code from it** — use the RAM trampoline

---

## Build Process

### Prerequisites

- **MSXgl** installed at `C:\MSXgl\`
- **SDCC** (included with MSXgl)
- **PowerShell** (Windows)

### Project Structure

```
C:\msxgl\projects\nau_dx\
├── nau_dx.c                  # Game source (all gameplay)
├── nau_dx_intro.c            # Intro source (standalone, ASCII16)
├── project_config_game.js    # Game build config (ROM_32K, 32KB)
├── project_config_intro.js   # Intro build config (ROM_ASCII16, 64KB)
├── project_config.js         # Active config (copied by build)
├── intro_sc2_raw.bin         # Raw SC2 intro image (16KB)
├── game_p1.bin               # Game binary part 1 (16KB, generated)
├── game_p2.bin               # Game binary part 2 (16KB, generated)
├── build.bat                 # MSXgl build script
├── out\
│   ├── nau_dx.rom            # Game-only ROM (32KB)
│   └── nau_dx_intro.rom      # Full cartridge ROM (64KB)
└── .codex_cpc_ref\           # CPC reference source
```

### Full Build (Intro + Game)

**Run these commands in order from `C:\msxgl\projects\nau_dx\`:**

```powershell
# Step 1: Build the game binary
Copy-Item project_config_game.js project_config.js -Force
.\build.bat

# Step 2: Split the game ROM into two 16KB parts
$rom = [System.IO.File]::ReadAllBytes("out\nau_dx.rom")
[System.IO.File]::WriteAllBytes("game_p1.bin", $rom[0..16383])
[System.IO.File]::WriteAllBytes("game_p2.bin", $rom[16384..($rom.Length-1)])

# Step 3: Build the full cartridge (intro + game)
Copy-Item project_config_intro.js project_config.js -Force
.\build.bat

# Step 4: Copy to test output
Copy-Item "out\nau_dx_intro.rom" "C:\msx-game\output\nau_dx.rom" -Force
```

### One-liner (copy-paste)

```powershell
cd C:\msxgl\projects\nau_dx; Copy-Item project_config_game.js project_config.js -Force; .\build.bat; $rom = [System.IO.File]::ReadAllBytes("out\nau_dx.rom"); [System.IO.File]::WriteAllBytes("game_p1.bin", $rom[0..16383]); [System.IO.File]::WriteAllBytes("game_p2.bin", $rom[16384..($rom.Length-1)]); Copy-Item project_config_intro.js project_config.js -Force; .\build.bat; Copy-Item "out\nau_dx_intro.rom" "C:\msx-game\output\nau_dx.rom" -Force
```

### Build Only Game (for debugging)

```powershell
cd C:\msxgl\projects\nau_dx
Copy-Item project_config_game.js project_config.js -Force
.\build.bat
```

Output: `out\nau_dx.rom` (32KB, game only — no intro)

### Build Only Intro (if game binaries unchanged)

```powershell
cd C:\msxgl\projects\nau_dx
Copy-Item project_config_intro.js project_config.js -Force
.\build.bat
```

Output: `out\nau_dx_intro.rom` (64KB, uses existing `game_p1.bin` / `game_p2.bin`)

### Default Config

`project_config.js` is set to `project_config_intro.js` by default, so running `.\build.bat` without copying configs produces the full cartridge ROM.

---

## Key Files

| File | Purpose |
|------|---------|
| `nau_dx.c` | Game source: gameplay, HUD, input, menus, enemies, collisions, SFX |
| `nau_dx_intro.c` | Intro source: SC2 display, music loop, bank switch trampoline |
| `project_config_game.js` | Game build: ROM_32K target, 32KB output |
| `project_config_intro.js` | Intro build: ROM_ASCII16 target, 64KB output, embeds intro image + game binaries |
| `intro_sc2_raw.bin` | Raw SC2 intro image (16KB, copied directly to VRAM) |
| `game_p1.bin` | Game binary part 1 (first 16KB, generated from `nau_dx.rom`) |
| `game_p2.bin` | Game binary part 2 (second 16KB, generated from `nau_dx.rom`) |

---

## Important Rules

1. **Always rebuild game before intro** when modifying `nau_dx.c` — the intro embeds pre-built `game_p1.bin` / `game_p2.bin`
2. **Never switch bank 0 while running code from it** — the RAM trampoline at 0xC000 handles this safely
3. **Game entry point** must be at 0x4014 (after CRT0 init for ROM_32K)
4. **HUD font tiles** must be reloaded after `VDP_FillScreen_GM2()` (which clears VRAM)
5. **`InitGamePlay()`** must be called before first gameplay frame to load font tiles and reset game state
6. **Menu fire button** is mode-specific — joystick fire only works in joystick mode, keyboard fire only in keyboard mode

---

## Testing

- **Emulator**: openMSX
- **ROM path**: `C:\msx-game\output\nau_dx.rom`
- **Machine**: MSX1 (TMS9918)
- **Input**: Joystick port 1 or keyboard
