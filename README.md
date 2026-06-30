# 🏁 Yellow Racer

Un videogioco di guida **pseudo-3D** (stile *Pole Position*) per la board
**ESP32-2432S028R** — la nota *"Cheap Yellow Display"* (CYD) con display LCD
2.8" 320×240 e touch resistivo.

Realizzato in C++ con il framework Arduino, tramite PlatformIO. Il motore
grafico è un renderer a segmenti proiettati (curve + salite/discese) con
double-buffer su sprite a 8-bit per evitare flicker.

![Menu](docs/menu.png)
![Gara](docs/race.png)
![Game Over](docs/gameover.png)

### In azione

![Race gameplay](docs/race.gif)

## ✨ Caratteristiche

- Renderer pseudo-3D a segmenti proiettati (stile Jake Gordon) con **curve** e
  **salite/discese** (hill).
- Cordoli alternati bianco/rosso, linea tratteggiata centrale, cielo e sagoma
  del Monte Fuji con neve.
- **Double-buffer** su sprite 8-bit (RGB332) in RAM: niente flicker/ghost.
- **Forza centrifuga**: in curva l'auto viene spinta verso l'esterno, devi sterzare.
- **Fuori strada**: esci dai cordoli e l'erba ti frena.
- 3 piste (facile / media / difficile) con curve e hill diversi.
- Time attack a **3 giri** con budget di tempo; punteggio = tempo rimanente × 100.
- FSM Menu → Gara → Game Over, tutto via touch.
- HUD: giro corrente, tempo rimanente, velocità; flash "GIRO n!" al passaggio.
- ~25 FPS stabili su hardware senza PSRAM.

## 🎮 Comandi

- **Menu**: tocca una delle 3 piste per iniziare.
- **Gara**:
  - Tocca la **metà sinistra** dello schermo → sterza a sinistra.
  - Tocca la **metà destra** → sterza a destra.
  - L'auto accelera automaticamente. Non c'è recentraggio: tieni premuto per
    sterzare, rilascia per mantenere la traiettoria (la centrifuga farà il resto).
- **Game Over**: tocca per tornare al menu.

## 🔧 Hardware

- **Board**: ESP32-2432S028R (CYD) — ESP32 + LCD 2.8" ILI9341 320×240 + touch XPT2046.
- Nessuna saldatura necessaria: display e touch sono già onboard.
- Alimentazione e flash via USB (CH340).

Pinout (configurato in [`src/User_Setup.h`](src/User_Setup.h) e
[`src/config.hpp`](src/config.hpp)):

| Funzione | Pin |
|----------|-----|
| TFT_DC / CS / SCK / MOSI / MISO | 2 / 15 / 14 / 13 / 12 |
| TFT_BL (backlight) | 21 |
| Touch XPT2046 CLK / MOSI / MISO / CS / IRQ | 25 / 32 / 39 / 33 / 36 |

> Il touch usa un bus SPI **separato** dal display (VSPI rimappato sui pin 25/32/33/36/39).

## 🛠️ Build & Flash

Requisiti: [PlatformIO Core](https://platformio.org/) (`pip install platformio`).

```bash
# build
pio run

# flash (board collegata su /dev/ttyUSB0)
pio run -t upload

# monitor seriale (115200) — serve un terminale TTY
pio device monitor
```

Se l'upload fallisce con "port busy", chiudi eventuali monitor seriale attivi o
abbassa la velocità di upload in [`platformio.ini`](platformio.ini).

## 📁 Struttura del progetto

```
├── platformio.ini         # config ambiente + librerie
├── PIANO_SVILUPPO.md      # piano di sviluppo (M1-M4) con note tecniche
├── README.md
├── docs/                  # screenshot
│   ├── menu.png
│   ├── race.png
│   └── gameover.png
├── tools/
│   └── capture.py         # script per catturare screenshot via seriale
└── src/
    ├── User_Setup.h       # config TFT_eSPI (driver ILI9341, pin CYD)
    ├── config.hpp         # pin touch, palette, costanti di gioco
    ├── input.hpp/.cpp     # XPT2046 (SPI dedicato) + calibrazione
    ├── track_data.hpp     # struct Segment + 3 TrackDef + buildTrack()
    ├── renderer.hpp/.cpp  # pseudo-3D a segmenti + sprite 8-bit double-buffer
    ├── game.hpp/.cpp      # FSM Menu/Race/GameOver, sterzo, giri, tempo, HUD
    └── main.cpp           # entry point + loop
```

## 🖼️ Catturare screenshot (dev)

Il firmware espone un comando seriale: inviando il carattere **`s`** (a 115200 baud)
composta 3 scene (menu/gara/gameover) e ne scarica il framebuffer 8-bit (RGB332,
320×240, stride 320) marcato `SHOT:<nome>\n`. Lo script
[`tools/capture.py`](tools/capture.py) invia il comando e salva i PNG in `docs/`:

```bash
python3 tools/capture.py   # richiede pyserial + Pillow
```

Per generare una **GIF animata** della gara (60 frame a velocità di gioco):

```bash
python3 tools/capture_gif.py   # usa 921600 baud; salvata in docs/race.gif
```

> Nota: `capture_gif.py` apre la seriale a 921600 baud. Per catturare a quella
> velocità il firmware deve essere temporaneamente ricompilato con
> `Serial.begin(921600)` in `src/main.cpp` (poi revertato). Lo script vale come
> riferimento; le GIF già pronte sono in `docs/`.

## 🧠 Note tecniche

- **Niente PSRAM** sul CYD: nessun framebuffer 16-bit (320×240×2 = 154 KB non
  entra nel blocco contiguo di heap, ~108 KB). Si usa uno **sprite 8-bit**
  (77 KB) come double-buffer; i colori RGB565 vengono quantizzati a RGB332.
- **Tiling esatto** tra segmenti (`lroundf` su edge bit-identical): zero gap
  (niente ghost) e zero overdraw.
- Il **touch resistivo** XPT2046 usa il `SPI` globale (VSPI) rimappato sui pin
  del touch *prima* di `ts.begin()`: questo perché `SPIClass::begin()` è no-op se
  il bus è già avviato (v. [`PIANO_SVILUPPO.md`](PIANO_SVILUPPO.md)).

## 🗺️ Stato & roadmap

- ✅ **M1** Inizializzazione (display, touch, build).
- ✅ **M2** Motore grafico 2.5D (segmenti, curve, hill, double-buffer).
- ✅ **M3** Logica di gioco (FSM, sterzo, centrifuga, giri, tempo, 3 piste).
- ⬜ **M4** (opzionale) Ottimizzazione: push 16-bit con DMA (due sprite 320×120),
  calibrazione touch fine, asset bitmap, audio (speaker su IO26).

Dettagli in [`PIANO_SVILUPPO.md`](PIANO_SVILUPPO.md).

## 🙏 Crediti

- Algoritmo pseudo-3D ispirato a [Jake Gordon — "Making a racing game"](https://github.com/jakesgordon/javascript-racer).
- Community [ESP32-Cheap-Yellow-Display](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display) (pinout e docs CYD).
- [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) di Bodmer per il display.
- [XPT2046_Touchscreen](https://github.com/PaulStoffregen/XPT2046_Touchscreen) di Paul Stoffregen per il touch.

## 📄 Licenza

MIT — vedi [`LICENSE`](LICENSE). Librerie terze mantenute dalle rispettive licenze.
