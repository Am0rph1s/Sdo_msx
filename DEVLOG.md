# Devlog: Porting Nau DX from Amstrad CPC to MSX1

## The Challenge

Take a complete space shooter from the Amstrad CPC, written for a 4MHz Z80 with 128KB of RAM and a 16-color palette, and make it run on an MSX1 — a 3.58MHz Z80 with just 16KB of RAM, 15 colors, and a completely different video chip. Oh, and it had to feel exactly like the original.

Here's what broke, what we learned, and how we fixed it.

---

## 1. The Joystick That Wouldn't Work

**The problem:** On real hardware (a Sony HB-75P), the ship shot continuously and refused to move. But it worked fine in emulators.

**The cause:** The MSX reads the joystick through the PSG sound chip's I/O port. Register 7 of the YM2149 controls whether the port reads external input or drives output. Bits 6-7 are **write-only** — they always read back as zero. Every time a sound effect called the mixer update (which does read-modify-write on register 7), the returned zeros overwrote the port direction. Port A switched to output mode, and the joystick went dead.

**The fix:** Never read register 7. Compute the mixer value directly and always set bit 6 to 1 (Port A = input). Every sound effect function was rewritten to call the safe mixer update instead of the library's read-modify-write version. The intro music player got the same treatment — its value `0xF8` was also silently putting the ports in output mode.

**Lesson learned:** On the YM2149, register 7 bits 6-7 are write-only. Any read-modify-write pattern will silently corrupt them.

---

## 2. Sprites: Two Layers Where CPC Had One

**The problem:** CPC sprites use mode 0 (16 colors, 4 per pixel with wide pixels). MSX sprites have one bit per pixel with a single color per sprite. A CPC ship with white hull and red cockpit detail doesn't directly translate.

**The fix:** Split every sprite into two overlapping layers. A "white" layer carries the main shape, a "red" layer carries the detail. Both are placed at the same screen position with different sprite colors. The VDP composites them naturally. The player ship, all five enemy types, and the boss all use this technique. 

Seven sprites × two layers = 14 sprite slots in the pattern table. With 32 hardware sprites available on screen, the two-layer approach fits comfortably — but the red layer is drawn last so it gets discarded first on sprite overflow.

---

## 3. Audio and Input: Sharing Is Hard

**The problem:** The game needs sound effects AND joystick input simultaneously. Both go through the same chip (the PSG). The MSXgl library's `EnableTone` functions modify register 7 — exactly the same register that controls the joystick port.

**The fix:** We wrote our own `updateMixer` function. It computes the entire register 7 value from scratch — which tone channels are active, which noise channels are muted, and critically, bit 6 always set to 1. When the intro transitions to the game, the music silence routine also writes a safe value (`0x3F` with both I/O ports as input) so the joystick works from the very first in-game frame.

---

## 4. Video Memory Overflows From Nowhere

**The problem:** During boss fights, the left wall would occasionally show garbage tiles — random HP bar pixels appearing in the rock pattern. It only happened at specific screen positions.

**The cause:** The boss HP bar draws 5 filled segments starting at column 28. The screen is 32 columns wide (0-31). That 5th segment landed on column 32 — which in the VDP's name table is the same memory as column 0 of the *next* row. The bar was silently writing over the wall tiles one row below.

**The fix:** Shift the entire HP bar one column left (start at column 27 instead of 28). Five segments now fit within columns 27-31. No overflow, no wall corruption.

**Similar issue:** The HUD cleanup strings were 10 characters long at column 25 (wrapping to column 0-2 of the next row). Shortened to 7 characters.

---

## 5. The SDCC Performance Wall

**The problem:** SDCC (the C compiler for Z80) generates decent but not great code. The CPC original ran at a comfortable frame rate. On MSX with SDCC, complex frames with many enemies and bullets would occasionally drop below 50Hz.

**The victories (things we kept):**

- **Pointer caching in hot loops:** The enemy update loop accesses `enemies[i].x`, `.y`, `.type`, `.health`, `.vx`, `.vy`, `.zig_timer` up to 40 times per enemy. Each access recalculates the struct offset on Z80. Caching a pointer (`enemy = &enemies[i]`) lets SDCC use register-based access through `(HL + offset)`. Huge savings in the boss AI section.

- **VDP partial writes:** The original code wrote all 32 sprite attributes to VRAM every frame (128 bytes) regardless of how many sprites were actually visible. The new code tracks the last frame's sprite count and only writes the used range plus any newly-disabled slots. When no sprites are active (menus, title screens), VRAM writes drop to zero bytes.

- **Extra life check without division:** The old code divided the score by 5000 using SDCC's `__divuint` routine (~800 cycles on Z80). The new code tracks a `nextLifeAt` threshold and compares directly. No division, zero function calls.

**The casualty (thing we reverted):** Struct padding to power-of-2 sizes looked promising on paper (faster array indexing via shifts), but the extra 30+ bytes of RAM pushed us too close to the stack and caused unpredictable corruption during gameplay.

---

## 6. PAL vs NTSC: One Game, Two Speeds

**The problem:** MSX machines run at either 50Hz (PAL, Europe) or 60Hz (NTSC, Japan/Americas). At 50Hz, the game updates 50 times per second. At 60Hz, everything moves 20% faster — enemies are harder, bullets fly faster, and the difficulty curve is completely different.

**The fix:** Detect the refresh rate at startup using the VDP status register. On 60Hz machines, skip exactly 1 out of every 6 frames (a 6-frame cycle: 5 game updates, 1 skip). This brings the effective update rate to 50 per second on any hardware. The timing is consistent, and the HP bar, wall scroll, music, and everything else behaves the same everywhere.

---

## 7. Memory Mapper Dance

**The problem:** The intro screen is a full-screen 16-color image (12KB of VRAM data) plus background music. The game itself is 32KB of code and data. Together they don't fit in a standard 32KB ROM cartridge.

**The fix:** Use the Konami SCC mapper to create a 64KB ROM. The intro occupies segments 1-2 (16KB for the screen image and music). The game is split into four 8KB chunks across segments 3-6. When the intro finishes (or the player presses fire), a tiny assembly trampoline switches all four mapper banks simultaneously and jumps to the game entry point. The transition is instant — no loading screen, no delay.

---

## 8. Enemy Waves: Keeping Them Interesting But Fair

**The original CPC behavior:** Enemies spawned and immediately fired a volley — every enemy in the wave shooting at the same time. Visually chaotic, and against fast enemies whose bullets move with their ship movement, almost impossible to dodge.

**Our adjustment:** Each enemy gets a staggered initial cooldown based on its slot index in the array. The first enemy shoots after about 0.13 seconds (8 frames), the last after about 0.4 seconds (25 frames). Different enough that you can dodge them individually, tight enough that waves still feel aggressive.

**Wave spacing:** Some wave layouts at higher levels (5 fast enemies) would place two enemies at the exact same screen position due to how rows were distributed. We gave the 5-enemy fast wave an explicit position layout instead of letting the generic row-based algorithm overlap them.

---

## 9. The Bug That Wasn't RAM

For several frustrating hours we chased a phantom RAM overflow. The game corrupted itself in bizarre ways — wrong tiles on walls, enemies passing through boundaries, slowdowns. The struct padding optimization added ~30 bytes of RAM and we blamed it. Reverted it. Still broken.

The real culprits? **Three tiny VRAM overflows hiding in plain sight:**

1. The HP bar's 5th column writing to screen position 32 (wrap to next row)
2. The HUD clear string (10 chars starting at column 25, wrapping to column 0)
3. Running uncapped at 60Hz making everything too fast and exposing race conditions

Once all three were fixed — without touching a single byte of RAM allocation — the game ran perfectly. Sometimes the simplest bugs hide behind the most intimidating symptoms.

---

## What's Left

The game is functionally complete. Remaining polish items are cosmetic: testing the full intro+game ROM on real hardware, fine-tuning difficulty, and maybe adding a splash of yellow to one enemy type for visual variety.

The ROM fits in 32KB with 8 bytes to spare. The intro version uses a 64KB mapper cartridge. Both run at a locked 50Hz on any MSX1 hardware.

---

*This was a port that started with a non-working joystick on real hardware and ended with a smooth 50Hz shooter. The MSX1 is a quirky but capable machine — once you learn to keep your PSG ports in input mode, your VDP column counts under 32, and your SDCC loops cache their pointers.*
