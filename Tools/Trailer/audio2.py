"""Soundtrack for the full-song trailer: the song, with game SFX on top of the shots they belong to."""
import os
import subprocess
import sys

import numpy as np

import plan2

SR = 48000
ART = "E:/NewSSDFile/Game Pemerintahan/Art/Audio_src/SFX/"
OUT = os.path.join(plan2.HERE, "trailer2_audio.wav")


def load(path, start=0.0, dur=None):
    cmd = ["ffmpeg", "-v", "error", "-ss", str(start), "-i", path]
    if dur:
        cmd += ["-t", str(dur)]
    cmd += ["-ac", "2", "-ar", str(SR), "-f", "f32le", "-"]
    return np.frombuffer(subprocess.run(cmd, capture_output=True).stdout, np.float32).reshape(-1, 2).copy()


def db(x):
    return 10 ** (x / 20)


song = load(plan2.SONG)
N = len(song)
fx = np.zeros_like(song)


def fade(x, fin, fout):
    x = x.copy()
    a, b = int(fin * SR), int(fout * SR)
    if a:
        x[:a] *= np.linspace(0, 1, a)[:, None]
    if b:
        x[-b:] *= np.linspace(1, 0, b)[:, None]
    return x


def sfx(name, at, gain_db, dur=None, fin=0.0, fout=0.0):
    x = load(ART + name + ".wav")
    if dur:
        n = int(dur * SR)
        x = np.tile(x, (n // len(x) + 1, 1))[:n]
    x = fade(x, fin, fout)
    i = int(at * SR)
    n = min(len(x), N - i)
    fx[i:i + n] += x[:n] * db(gain_db)


T = plan2.timeline()


def at(index, offset=0.0):
    return T[index][0] + offset


def length(index):
    return T[index][1]


# Each shot index refers to plan2.EDIT.
sfx("S_Amb_Jalan", 0.0, -20, dur=7.2, fin=1.5, fout=0.6)          # smog city, then Bayu on the street
sfx("S_UI_Buka", at(3, 0.05), -12)                                 # phone screen on
sfx("S_Mesin_Motor", at(5), -22, dur=1.2 * 2, fin=0.05, fout=0.3)
sfx("S_Klakson", at(6, 0.25), -16)
sfx("S_Order_Masuk", at(7, 0.3), -10)
sfx("S_Mesin_Motor", at(8), -22, dur=1.2, fin=0.05, fout=0.3)
sfx("S_Notif_Info", at(10, 0.35), -12)
sfx("S_Klakson", at(11, 0.4), -17)
sfx("S_Koin_1", at(13, 0.3), -10)
sfx("S_Mesin_Motor", at(14), -22, dur=2.4, fin=0.05, fout=0.3)
sfx("S_Notif_Buruk", at(16, 0.25), -9)                             # "Dinda muntah-muntah"
sfx("S_Notif_Info", at(23, 0.3), -13)                              # the upload
sfx("S_Sirene", at(24), -18, dur=4.8, fin=0.3, fout=1.2)           # SPN at the door, Pos SPN
sfx("S_Amb_Massa", at(26), -17, dur=7.2, fin=0.4, fout=1.0)        # long march, Dewan, arrests
sfx("S_UI_Klik", at(29, 0.05), -12)
sfx("S_Notif_Baik", at(33, 0.3), -12)                              # Kata Warga
sfx("S_Mesin_Mobil", at(34), -20, dur=2.4, fin=0.1, fout=0.4)      # patok, ekskavator
sfx("S_UI_Klik", at(36, 0.05), -12)
sfx("S_Koin_1", at(38, 0.5), -10)                                  # serangan fajar
sfx("S_Amb_Massa", at(40), -20, dur=1.2, fin=0.2, fout=0.3)    # TPS queue
sfx("S_Amb_Massa", at(43), -18, dur=2.4, fin=0.3, fout=0.6)        # kotak dibuka
sfx("S_UI_Buka", at(45, 0.05), -13)
sfx("S_Mesin_Motor", at(52), -22, dur=2.4, fin=0.1, fout=0.8)      # riding home
sfx("S_UI_Klik", at(53, 0.05), -12)

mix = song + fx
peak = np.max(np.abs(mix))
mix *= db(-1) / peak
out = (np.clip(mix, -1, 1) * 32767).astype("<i2")
subprocess.run(["ffmpeg", "-v", "error", "-y", "-f", "s16le", "-ar", str(SR), "-ac", "2", "-i", "-", OUT], input=out.tobytes())
print("audio ok %.2f s, gain %.1f dB" % (N / SR, 20 * np.log10(db(-1) / peak)))
