#!/usr/bin/env python3
"""Cattura screenshot dal CYD: invia 's' al firmware, legge i frame RGB332
(320x240, stride 320) marcati 'SHOT:<name>\n' e li salva come PNG (palette)."""
import serial, time, sys, os
from PIL import Image

PORT = "/dev/ttyUSB0"
W, H = 320, 240
N = W * H
OUTDIR = "/var/home/angelo/Dev/esp32/esp32-car-racing/docs"

# palette RGB332 -> RGB888
PAL = []
for b in range(256):
    r = (b >> 5) & 0x7
    g = (b >> 2) & 0x7
    bl = b & 0x3
    PAL += [(r * 255) // 7, (g * 255) // 7, (bl * 255) // 3]

s = serial.Serial(PORT, 115200, timeout=0.5)
time.sleep(1.6)  # boot
s.reset_input_buffer()
print("invio 's' ...")
s.write(b"s")
s.flush()

frames = {}
buf = bytearray()
deadline = time.time() + 60
while time.time() < deadline and len(frames) < 3:
    chunk = s.read(8192)
    if chunk:
        buf.extend(chunk)
    while True:
        idx = buf.find(b"SHOT:")
        if idx < 0:
            break
        nl = buf.find(b"\n", idx)
        if nl < 0:
            break
        name = buf[idx + 5:nl].decode("ascii", "replace").strip()
        start = nl + 1
        if len(buf) - start < N + 1:
            break
        frame = bytes(buf[start:start + N])
        frames[name] = frame
        del buf[:start + N + 1]
        print("  ricevuto:", name)
    if b"CAPTURE DONE" in buf:
        break
    time.sleep(0.02)

s.close()
print("catturati:", list(frames.keys()))
os.makedirs(OUTDIR, exist_ok=True)
for name, frame in frames.items():
    img = Image.frombytes("P", (W, H), frame)
    img.putpalette(PAL)
    img = img.convert("RGB")
    fn = os.path.join(OUTDIR, name + ".png")
    img.save(fn)
    print("salvato", fn)
if not frames:
    sys.exit("nessun frame catturato")
