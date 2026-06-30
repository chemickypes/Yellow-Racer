#!/usr/bin/env python3
"""Cattura N frame di gara dal CYD (921600 baud) e li assembla in una GIF.
Invia 'g', legge i frame marcati 'FRAME\\n' + 76800 byte RGB332, salva docs/race.gif."""
import serial, time, sys, os
from PIL import Image

PORT = "/dev/ttyUSB0"
BAUD = 921600
W, H = 320, 240
N = W * H
MAXFRAMES = 60
OUT = "/var/home/angelo/Dev/esp32/esp32-car-racing/docs/race.gif"

PAL = []
for b in range(256):
    r = (b >> 5) & 0x7
    g = (b >> 2) & 0x7
    bl = b & 0x3
    PAL += [r * 255 // 7, g * 255 // 7, bl * 255 // 3]

s = serial.Serial(PORT, BAUD, timeout=0.5)
time.sleep(1.6)  # boot
s.reset_input_buffer()
print("invio 'g' ...")
s.write(b"g")
s.flush()

frames = []
buf = bytearray()
deadline = time.time() + 120
while time.time() < deadline and len(frames) < MAXFRAMES:
    chunk = s.read(16384)
    if chunk:
        buf.extend(chunk)
    while True:
        idx = buf.find(b"FRAME\n")
        if idx < 0:
            break
        start = idx + 6
        if len(buf) - start < N:
            break
        frames.append(bytes(buf[start:start + N]))
        del buf[:start + N]
        if len(frames) % 10 == 0:
            print("  frame", len(frames))
    if b"GIF DONE" in buf:
        break
    time.sleep(0.005)

s.close()
print("frame catturati:", len(frames))
if not frames:
    sys.exit("nessun frame")

imgs = []
for fr in frames:
    im = Image.frombytes("P", (W, H), fr)
    im.putpalette(PAL)
    im = im.convert("RGB")
    imgs.append(im)

imgs[0].save(OUT, save_all=True, append_images=imgs[1:],
             duration=40, loop=0, optimize=True, disposal=2)
print("salvato", OUT)
