"""Turns the downloaded CC0 sounds and sprites into the files the game imports. Plain Python (numpy, Pillow, ffmpeg),
no Unreal.

Input:  Art/Audio_src/SFX/_sumber (Kenney + OpenGameArt, see Docs/CREDITS.md), Art/VFX_src/_sumber
Output: Art/Audio_src/SFX/S_*.wav (16 bit, 44.1 kHz), Art/VFX_src/T_Vfx_*.png
Next:   Tools/Python/import_sfx.py

Loops get a crossfade across the seam, and every sound a set level, so game code can use volume 1 as a starting point.
The car horn is made here (two detuned tones through a soft clip), no CC0 recording of one was found.
"""
import glob
import os
import subprocess
import wave

import numpy as np
from PIL import Image

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
SRC = os.path.join(ROOT, "Art", "Audio_src", "SFX", "_sumber")
OUT = os.path.join(ROOT, "Art", "Audio_src", "SFX")
VFX = os.path.join(ROOT, "Art", "VFX_src")
RATE = 44100
KEN = os.path.join(SRC, "kenney")
OGA = os.path.join(SRC, "opengameart")


def decode(path, channels=1):
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", path, "-f", "f32le", "-ac", str(channels), "-ar", str(RATE), "-"],
                         capture_output=True, check=True).stdout
    return np.frombuffer(raw, np.float32).reshape(-1, channels).astype(np.float64)


def loop(x, fade):
    """Makes x loop: its tail is crossfaded (equal power) over its head."""
    n = int(fade * RATE)
    t = np.linspace(0, np.pi / 2, n)[:, None]
    y = x[:-n].copy()
    y[:n] = x[:n] * np.sin(t) + x[-n:] * np.cos(t)
    return y


def rms_to(x, db):
    y = x * (10 ** (db / 20) / max(np.sqrt((x ** 2).mean()), 1e-9))
    return y * min(1.0, 0.95 / max(np.abs(y).max(), 1e-9))


def peak_to(x, db):
    return x * (10 ** (db / 20) / max(np.abs(x).max(), 1e-9))


def trim(x, floor=0.004):
    """Cuts silence off both ends, keeping 5 ms before the sound starts."""
    on = np.where(np.abs(x).max(1) > floor * max(np.abs(x).max(), 1e-9))[0]
    return x[max(on[0] - int(0.005 * RATE), 0):on[-1] + int(0.02 * RATE)] if len(on) else x


def save(name, x):
    pcm = (np.clip(x, -1, 1) * 32767).astype("<i2")
    with wave.open(os.path.join(OUT, name + ".wav"), "wb") as w:
        w.setnchannels(pcm.shape[1])
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(pcm.tobytes())
    print(name, "%.2fs %dch rms %.3f peak %.2f" % (len(x) / RATE, pcm.shape[1], np.sqrt((x ** 2).mean()), np.abs(x).max()))


def horn():
    def beep(sec):
        t = np.arange(int(sec * RATE)) / RATE
        s = sum(np.sin(2 * np.pi * f * t) + 0.5 * np.sin(2 * np.pi * 2 * f * t) + 0.3 * np.sin(2 * np.pi * 3 * f * t) for f in (415.0, 512.0))
        s = np.tanh(s * 0.9)
        edge = np.minimum(1, np.minimum(t / 0.012, (sec - t) / 0.03))
        return s * edge
    x = np.r_[beep(0.17), np.zeros(int(0.09 * RATE)), beep(0.38)]
    return peak_to(x[:, None], -5)


def sprites():
    def fit(src, name, size):
        im = Image.open(os.path.join(VFX, "_sumber", src + ".png")).convert("RGBA").resize((size, size), Image.LANCZOS)
        im.save(os.path.join(VFX, name + ".png"))
        print(name, size)
    fit("smoke_07", "T_Vfx_Asap", 256)
    fit("dirt_02", "T_Vfx_Kerikil", 256)
    fit("star_01", "T_Vfx_Percikan", 128)


def main():
    ui = os.path.join(KEN, "interface-sounds", "Audio")
    for name, src, db in (("S_UI_Hover", "tick_002", -16), ("S_UI_Klik", "click_001", -11), ("S_UI_Buka", "maximize_003", -12),
                          ("S_UI_Tutup", "minimize_003", -12), ("S_Notif_Info", "pluck_001", -12), ("S_Notif_Baik", "confirmation_002", -12),
                          ("S_Notif_Buruk", "error_006", -12), ("S_Order_Masuk", "confirmation_004", -8)):
        save(name, peak_to(trim(decode(os.path.join(ui, src + ".ogg"))), db))

    hit = os.path.join(KEN, "impact-sounds", "Audio")
    for name, src, db in (("S_Langkah_Beton", "footstep_concrete", -6), ("S_Langkah_Rumput", "footstep_grass", -6),
                          ("S_Tabrak_Logam", "impactMetal_heavy", -5), ("S_Tabrak_Badan", "impactPunch_heavy", -6),
                          ("S_Tabrak_Barang", "impactPlank_medium", -8)):
        for i in range(5):
            save("%s_%d" % (name, i), peak_to(trim(decode(os.path.join(hit, "%s_%03d.ogg" % (src, i)))), db))

    rpg = os.path.join(KEN, "rpg-audio", "Audio")
    for i, src in enumerate(sorted(glob.glob(os.path.join(rpg, "handleCoins*.ogg")))):
        save("S_Koin_%d" % i, peak_to(trim(decode(src)), -10))

    save("S_Mesin_Motor", rms_to(loop(decode(os.path.join(OGA, "motor_looping_0.mp3")), 0.08), -20))
    save("S_Mesin_Mobil", rms_to(loop(decode(os.path.join(OGA, "loop_1_0.wav")), 0.02), -18))
    save("S_Mesin_Angkot", rms_to(loop(decode(os.path.join(OGA, "loop_0.wav")), 0.02), -18))
    save("S_Amb_Jalan", rms_to(loop(decode(os.path.join(OGA, "gatve_Varniu_2.ogg")), 3.0), -17))
    save("S_Amb_Burung", rms_to(loop(decode(os.path.join(OGA, "birds-isaiah658_0.ogg"), 2), 2.0), -22))
    save("S_Amb_Jangkrik", rms_to(loop(decode(os.path.join(OGA, "crickets_1.mp3"), 2), 1.0), -24))
    save("S_Amb_Massa", rms_to(loop(decode(os.path.join(OGA, "crowd_shouting_0.ogg")), 2.0), -20))

    siren = decode(os.path.join(OGA, "siren_0.mp3"))[: 9 * RATE]
    siren[-2 * RATE:] *= np.linspace(1, 0, 2 * RATE)[:, None]
    save("S_Sirene", rms_to(siren, -18))
    save("S_Klakson", horn())
    sprites()


main()
