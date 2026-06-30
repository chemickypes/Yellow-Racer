# Piano di Sviluppo: "Yellow Racer" (ESP32-2432S028R / CYD)

> Versione corretta e finalizzata. Sostituisce la bozza iniziale.
> Riferimento visivo: stile **Pole Position** (rear-view, strada con cordoli bianchi/rossi,
> linea tratteggiata centrale, sagome montagne/Fuji, prato, billboard).

## 1. Obiettivo

Videogioco di guida **pseudo-3D** per la board ESP32-2432S028R ("Cheap Yellow Display").
Time attack a **3 giri** con **budget di tempo** per pista. Niente ostacoli, niente audio nell'MVP.

## 2. Hardware & Toolchain

- **Hardware:** ESP32-2432S028R (CYD). Display 2.8" 320x240, touch **resistivo** XPT2046.
- **OS Host:** Linux Bluefin (Silverblue).
- **Toolchain:** PlatformIO Core 6.1.19.
- **Driver display:** **ILI9341_2_DRIVER** via `TFT_eSPI` (NON ST7789).
- **Driver touch:** `XPT2046_Touchscreen` su **bus SPI dedicato** (pin 25/32/33/36/39).
- **Board ID:** `esp32dev` (il board ID `esp32-2432S028` non esiste in PlatformIO).

### Pinout (da PINS.md ufficiale CYD)
| Funzione | Pin |
|----------|-----|
| TFT_DC   | 2   |
| TFT_MISO | 12  |
| TFT_MOSI | 13  |
| TFT_SCK  | 14  |
| TFT_CS   | 15  |
| TFT_BL   | 21  |
| TFT_RST  | -1 (board RST) |
| XPT2046_CLK  | 25  |
| XPT2046_MOSI | 32  |
| XPT2046_CS   | 33  |
| XPT2046_IRQ  | 36 (input only) |
| XPT2046_MISO | 39 (input only) |
| LED RGB   | 4(R)/16(G)/17(B) (active low) |
| BOOT btn  | 0   |
| Speaker   | 26 (I2S DAC, non usato in MVP) |
| SD (non usata) | VSPI 5/18/19/23 |

## 3. Vincoli hardware decisivi

- **NO PSRAM** sul CYD: niente framebuffer completo (320x240x2 = 153 KB > RAM).
  Rendering **diretto su TFT**, back-to-front, con `fillRect`/`drawFastHLine`.
- Display su HSPI, touch su SPI separato (`SPIClass touchSPI(VSPI); touchSPI.begin(25,39,32,33)`).
- `tft.setRotation(1)` per landscape 320x240; touch va calibrato/rimappato.

## 4. Struttura progetto

```
platformio.ini
PIANO_SVILUPPO.md
src/
├── User_Setup.h      # config TFT_eSPI (iniettata via -include)
├── config.hpp        # pin touch, dimensioni schermo, palette RGB565
├── input.hpp/.cpp    # XPT2046 + calibrazione + zone sterzo
├── track_data.hpp    # struct Segment + piste in PROGMEM  (M3)
├── sprites.hpp       # bitmap PROGMEM: auto, billboard, montagne (M3)
├── renderer.hpp/.cpp # pseudo-3D a segmenti (curve + hill)  (M2)
├── game.hpp/.cpp     # FSM MENU/RACE/GAMEOVER, giri, tempo, punteggio (M3)
└── main.cpp          # game loop + timing FPS
```

## 5. `platformio.ini`

```ini
[platformio]
default_envs = esp32dev

[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
upload_speed = 921600
lib_deps =
    bodmer/TFT_eSPI @ ^2.5.43
    paulstoffregen/XPT2046_Touchscreen @ ^1.4
build_flags =
    -D USER_SETUP_LOADED=1
    -include src/User_Setup.h
```

## 6. Decisioni di design

- **Renderer:** modello a **segmenti proiettati** (stile Jake Gordon). Ogni segmento ha
  `curve` (offset X accumulato) e `y` (altezza hill accumulata). Proiezione:
  `scale = camDepth / z`; `screenX = W/2 + scale*(worldX - camX)*W/2`;
  `screenY = H/2 - scale*(worldY - camHeight)*H/2`; `screenW = scale*roadWidth*W/2`.
  Disegno back-to-front: bande prato/cordolo/strada via `fillRect`.
  Sostituisce l'idea `sin(y)*amp` (valida solo per strada piatta, si rompe con le hill).
- **Controlli:** auto-accelerazione + sterzo touch (zone sx/dx). Touch resistivo single-touch:
  deadzone + rilevazione "premuto".
- **Asset:** `PROGMEM` nell'MVP; codice strutturato per migrare a SD in M4.
- **Win/lose:** 3 giri entro budget di tempo. Tempo=0 -> GAMEOVER; 3 giri -> WIN,
  punteggio = tempo_rimanente * 100. Lap rilevato al wrap della posizione cumulativa.
- **Audio:** assente nell'MVP (speaker su IO26 disponibile per fase successiva).

## 7. Roadmap / Milestones

### M1 - Inizializzazione (COMPLETATO)
- [x] `platformio.ini` con le correzioni (board, lib_deps, build_flags).
- [x] `src/User_Setup.h` CYD (ILI9341_2_DRIVER, pin HSPI, BL 21, RST -1).
- [x] `src/config.hpp` (pin touch, dims, palette).
- [x] `src/input.hpp/.cpp` (XPT2046 su SPI globale VSPI rimappato).
- [x] `src/main.cpp` di test: rettangolo + coordinate touch su serial.
- [x] `pio run` compila; `pio run -t upload` OK; seriale mostra boot "M1 init" senza flood.
- [ ] Verifica utente: rettangolo giallo a video + tocco reale (calibrazione `TS_MIN/MAX` da affinare in M3).

**Gotcha risolta (M1):** `SPIClass::begin()` su arduino-esp32 2.0.x e' no-op se il bus e'
gia' avviato (`if(_spi) return;`). La lib `paulstoffregen/XPT2046_Touchscreen` usa il `SPI`
globale e il suo `ts.begin()` chiama `SPI.begin()` (pin default VSPI 18/19/23/5). Percio'
`SPI.begin(25,39,32,33)` va chiamato **PRIMA** di `ts.begin()`, altrimenti SCK/MOSI restano
sui pin default e non raggiungono il chip (MISO float -> 0xFFFF -> raw=8191 fisso).

### M2 - Motore grafico 2.5D (in verifica visiva)
- [x] Struct `Segment { float curve, y; }` + accumulo curve/hill.
- [x] Proiezione segmenti + bande prato/cordolo/strada.
- [x] Linea tratteggiata centrale + cordoli bianchi/rossi scorrevoli.
- [x] Cielo + sagoma montagne (Fuji + picchi) + prato.
- [x] Test scroll auto-forward (camera fissa lateralmente, auto ferma).
- [x] `pio run` compila; upload OK; ~24 FPS su seriale.
- [ ] Conferma visiva utente: strada che si restringe all'orizzonte, curve sx/dx,
      salite/discese, cordoli alternati, linea tratteggiata, montagne.

**Note M2:** renderer a segmenti proiettati (stile Jake Gordon). drawDistance=150,
roadWidth=2000, cameraHeight=1000, fov=100 (cameraDepth=0.8391). Tiling esatto tra
segmenti (`lroundf` su edge bit-identical): zero gap (niente ghost) e zero overlap.
Cielo fillato dinamicamente fino al bordo strada piu' alto (niente gap orizzonte).

**Double-buffer (soluzione flicker):** sprite **8-bit RGB332** (77KB) come framebuffer
in RAM. Il CYD non ha PSRAM e l'heap e' frammentato (blocco contiguo max ~108KB misurato):
sprite 16-bit (154KB) OOM, sprite 8-bit (77KB) entra. Comporre in RAM elimina ghost e
flicker dell'HUD; un solo `pushSprite` riduce il tearing. I colori `col::*` (RGB565)
vengono auto-quantizzati a RGB332 dallo sprite (lieve, accettabile).
- ~25 FPS: il push 8-bit e' CPU-bound (conversione RGB332->RGB565 + `pushPixels` per-linea);
  bump SPI 55->80MHz non aiuta (non e' SPI-bound).
- **M4 (performance):** per >40fps servirebbe push 16-bit con DMA. Opzioni: due sprite
  16-bit 320x120 (76.8KB cada) pushati con `pushImageDMA` (split segmenti a y=120), oppure
  sprite 16-bit 320x168 (105KB, entra) per sola area strada + cielo/montagne statici.
  Rimandato a M4.

**Fix M2 (iterati dopo feedback visivo):** tiling esatto (ghost), cielo dinamico (gap
orizzonte), asfalto >=1px (prato sull'asfalto), wrap posizione (precisione float),
HUD composto nello sprite (flicker HUD).

### M3 - Logica di gioco (COMPLETATO)
- [x] FSM: `STATE_MENU` (3 piste touch), `STATE_RACE`, `STATE_GAMEOVER`.
- [x] Auto disegnata con primitive (body/alettone/abitacolo/fanali/pneumatici) + sterzo touch (lerp su playerX, senza recentraggio).
- [x] Auto-accelerazione; fuori strada (|playerX|>1) decelera sull'erba.
- [x] Forza centrifuga (car drifta verso l'esterno in curva -> deve sterzare).
- [x] Rilevamento giro (wrap posizione) + budget tempo + punteggio (win=tempo_rim*100, lose=giri*1000).
- [x] Flash "GIRO n!" in alto a sx (sopra l'indicatore LAP) per 1.5s al passaggio giro.
- [x] 3 piste in `track_data.hpp` (TrackDef con curve/hill/frequenze, budget 85/80/75s).
- [x] Zone touch sx/dx per sterzo; tap per menu/gameover.
- [x] `pio run` compila; upload OK; boot OK; verificato dall'utente.

**Note M3:** `Game` orchestra FSM e compone il frame su `Renderer::canvas()` (sprite 8-bit)
poi `push()`. Renderer refactored: `renderRace` (strada, no HUD/push), `drawCar`, `wipe`,
`canvas`, `push`, `loadTrack`, `curveAt`. Bilanciamento: N=800 segmenti (trackLength=160000),
MAX_SPEED=7000 (giro ~23s, 3 giri ~69s), budget 85/80/75s. Sterzo: meta' sx=-1 / dx=+1,
SENZA recentraggio (l'auto tiene la posizione laterale); CENTRIFUGAL=0.4 spinge fuori in
curva. Off-road (|playerX|>1): OFFROAD_DRAG decelera. Calibrazione `TS_MIN/MAX` ancora
grezza ma funzionante (bottoni menu e sterzo OK).

### M4 - Ottimizzazione & asset
- [ ] `tft.startWrite()/endWrite()` + DMA per il rendering.
- [ ] FPS limiter via `millis()`, contatore FPS a schermo.
- [ ] (Opzionale) scaling sprite per distanza.
- [ ] Astrazione asset per futura migrazione a SD.

## 8. Correzioni rispetto alla bozza iniziale

| # | Bozza | Corretto |
|---|-------|----------|
| 1 | Display ST7789 | ILI9341_2_DRIVER |
| 2 | Touch capacitivo | Resistivo XPT2046, bus SPI separato |
| 3 | `board = esp32-2432S028` | `board = esp32dev` |
| 4 | "SD non presente" | SD presente (non usata in MVP) |
| 5 | `bodmer/XPT2046_Touchscreen` | `paulstoffregen/XPT2046_Touchscreen` |
| 6 | `User_Setup.h` in `lib/` | in `src/` + `-include src/User_Setup.h` |
| 7 | Curve via `sin(y)*amp` | Modello a segmenti proiettati (curve + hill) |

## 9. Rischi

- Touch resistivo: serve deadzone + gestione "premuto". Opzionale: BOOT (IO0) come azione secondaria.
- Renderer curve+hill e' la parte complessa; se le perf sono insufficienti ridurre il count
  segmenti (~120) o degradare temporaneamente a strada piatta.
- Calibrazione touch: i valori `TS_MIN/MAX` sono grezzi, da affinare in M3 con sketch dedicato.
