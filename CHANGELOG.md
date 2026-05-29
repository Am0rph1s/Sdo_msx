# Nau DX MSX — Changelog

## Fixes aplicats

### Joystick (HB-75P)
- YM2149 PSG R#7 bits 6-7 write-only → root cause de fire perpetuo i direccions trencades
- `updateMixer()`: sempre posa bit 6=1 (Port A input)
- Totes les funcions `sfx*()` criden `updateMixer()` en lloc de `PSG_EnableTone/Noise`
- `BiosDir()/BiosTrig()` `__NAKED` reemplaçades per `Joystick_Read()` directe
- `MusicSilence()` intro: escriu 0xBF (bit 6=1) per no corrompre R#7 al saltar al joc

### Gameplay
- **Shot posició**: `SHIP_W/2-2` → `SHIP_W/2-4` (centrat al morro de la nau)
- **Volum shot**: `sfxShot 9→12`, `sfxEnemyShot 8→11`
- **Fire to start**: `sfxMenuSelect` → `sfxBeep` (feedback audible)
- **Biome reset**: `ResetGameSession()` abans de `InitWallTiles()`
- **Hi-score inicials**: comencen a 'A' (no espai)
- **Redefine keys**: hi-score input i fire del menú respecten tecles redefinides
- **Extra vides sense divisió**: `g_NextLifeAt` threshold tracking (no `__divuint`)
- **Bales boss**: velocitat reduïda (vy 3→2 a tier 0, cap t=1)
- **Cooldown inicial enemics**: 16+i*3 → 8+i*2 (no triguen mitja pantalla a disparar)

### Rendiment (SDCC Z80)
- **Pointer cache** a `UpdateEnemies()`: `TEnemy* e` evita recàlcul repetit de `g_Enemies[i]`
- **VDP partial write**: només envia sprites actius+desactivats (no 128B fixes cada frame)
- **Extra vides**: tracking per llindar, zero divisions

### Visuals
- **Boss sprite**: nou disseny (2 capes: cyan hull + red core)
- **HP bar**: clamp columna 32 (evitava corrompre wall esquerre — pendent de re-aplicar)
- **Bales enemigues**: vermell (`COLOR_LIGHT_RED`)

### Bug arreglat
- **Wave overlap**: `BuildWaveLayoutFastRank` n=5 — 3 enemics a Y=20 no comparteixen X (pendent de re-aplicar)

---

## Estat actual (commit `10aa7d5`)

- ROM: 32689 / 32768 bytes (79 lliures)
- RAM BSEG: ~650 bytes (de ~12KB disponibles)
- Funciona estable. Inclou pointer cache, VDP partial write, score sense divisió.

---

## Pendent

- [ ] **HP bar column clamp** — re-aplicar sense trencar res
- [ ] **Wave overlap fix** (n=5) — re-aplicar sense trencar res  
- [ ] Identificar quin dels dos canvis anteriors va causar la corrupció
- [ ] Provar bales grogues (alternativa al vermell per contrast)
- [ ] Generar ROM amb intro (nau_dx_intro.rom)
- [ ] Test en hardware real (HB-75P)
- [ ] Publicar a itch.io
