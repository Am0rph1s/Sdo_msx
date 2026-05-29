# Devlog: Nau DX — From Amstrad CPC to MSX1

Nau DX is a horizontal shoot-em-up originally written for the Amstrad CPC, already ported to ZX Spectrum. This is the MSX1 conversion.

## The joystick that broke on real hardware

Everything worked perfectly in the emulator. On a real Sony HB-75P, the ship fired non-stop and refused to move in any direction. The keyboard was fine. Only the joystick had the problem.

The MSX reads joystick input through the same chip that plays music — the PSG sound generator. One register in that chip, number 7, controls whether the chip's I/O pins listen for input or send output. Set it wrong and the joystick port goes deaf.

The problem was a read-then-write pattern happening inside the sound library. Every time a sound effect enabled or disabled a channel, it would read register 7, flip a couple of bits, and write it back. But two bits in that register always read as zero, no matter what you wrote there. So every sound effect was silently switching the joystick port off.

The fix was to stop reading register 7 entirely. Every place that touched it now computes the full register value from scratch, always keeping the joystick port in input mode. The intro music player needed the same treatment — its original register value was also putting the port in output mode, a configuration that some older MSX models warn against because it can cause electrical conflicts with the keyboard.

## Making sprites on a console with no real palette

The CPC draws sprites in mode 0 — wide pixels with 4 colors per sprite. A ship might have a white body, cyan highlights, red cockpit details, and yellow accents, all in one sprite. The MSX1 video chip handles sprites differently. Each sprite is one bit per pixel, flat, with a single color picked from a fixed 15-color palette. You can't have a white-and-red ship in a single sprite.

The solution was to split every sprite into two layers drawn on top of each other. A white layer carries the main silhouette. A red layer, positioned at the exact same coordinates, carries the details. The video chip composites them automatically. Every enemy type — basic, fast, heavy, diver, bomber — and the boss uses this two-layer approach. It costs twice the sprite slots but the MSX has 32 hardware sprites, so there is room.

## C code that runs at 50 frames per second

The game logic was ported from CPC assembler to C, compiled with SDCC. The first builds ran at the correct speed, but there was headroom to improve. A couple of targeted optimizations made a difference.

The enemy update loop accesses each enemy's position, speed, pattern, health, and timers multiple times per frame. On Z80, accessing a struct field means recalculating the memory address every time. Putting a pointer to the current enemy in a local variable lets the compiler use faster register-based addressing for all subsequent accesses. The boss AI section, with its vertical oscillation, lateral movement, and firing logic, benefited most.

Another win came from the sprite attribute table. The original code wrote all 32 sprite entries to the video chip every frame — all 128 bytes — regardless of how many sprites were actually visible. The new code tracks how many entries were used and only sends that many. On the title screen, where zero sprites are active, zero bytes go to the video chip. Every byte saved matters when the CPU and video chip share the same bus.

## One game, two speeds

European MSX machines run at 50 frames per second. Japanese and American machines run at 60. The game logic is frame-based — one update per frame — so on a 60 Hz machine everything would move twenty percent faster. Enemies shoot sooner, bullets fly quicker, and the whole difficulty curve shifts.

The fix is simple: detect the refresh rate at startup and, on 60 Hz hardware, skip exactly one out of every six frames. Five updates, one skip, repeat. This locks the effective speed to 50 Hz on any machine. The game plays the same in Tokyo or Barcelona.

## Fitting a full game in 32 kilobytes

The MSX1 cartridge format gives you 32KB of ROM space. The compiled game code, all the sprite data, the level layouts, the sound effects, and the menu graphics all had to fit. The final build leaves eight bytes free.

Things that saved space: removing a division call from the score system and replacing it with a simple threshold comparison, hand-optimizing the wall scroll routine in assembler, and stripping unused library code from the engine build.

## The intro screen that needed a memory mapper

The intro shows a full-screen color image with background music before the game starts — a 12 kilobyte screen plus audio. That alone would eat nearly half the cartridge. Together with the 32KB game, it does not fit in a standard ROM.

The solution was a mapper chip, the Konami SCC, which divides the cartridge into eight 8KB segments and lets you choose which ones are visible to the CPU at any moment. The intro sits in the first two segments. The game is split across four more. When the intro ends, a tiny assembly routine switches all four banks simultaneously and jumps directly into the game. The transition is instant — no loading bar, no black screen, no waiting.

## Wall tiles that mysteriously changed

During boss fights, the wall on the left side of the screen would sometimes show weird blocks at the same height as the boss health bar. Every time the bar updated, the wall glitched.

The boss HP bar draws five filled blocks at the right edge of the screen, starting at column twenty-seven. The screen is thirty-two columns wide. Simple math. But the original code started drawing at column twenty-eight. Five blocks from column twenty-eight reaches column thirty-two — which in the video chip's memory is the same address as column zero of the next row, where the wall lives. The fifth block of the health bar was silently overwriting the wall one row below.

Three bugs like this — the HP bar overflow, a text cleanup string wrapping around the screen edge, and the game running uncapped at 60 Hz — all appeared at roughly the same time and caused symptoms that looked exactly like a memory overflow. Corrupt tiles, enemies ignoring boundaries, slowdown. We blamed RAM, rewrote struct layouts, reverted optimizations. It was never RAM. It was three tiny video memory overflows hiding in plain sight, each one completely invisible in emulators that do not warn about out-of-bounds VRAM writes.

## The result

A 50 Hz horizontal shooter on MSX1, with intro screen and music on a 64KB mapper cartridge, five enemy types, multi-tier boss fights, starfield scrolling, 25 levels, and configurable controls. The standalone game ROM fits in 32KB. After a week of debugging a joystick that only failed on real hardware and three VRAM bugs that impersonated a memory leak, it is ready to play.
