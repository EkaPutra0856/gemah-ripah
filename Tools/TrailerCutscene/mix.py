"""Soundtrack for the 2 minute in-game trailer (Content/Data/trailer.json): an original synthesized score that hits
on the cuts, the game's ambience and SFX under each shot, and two in-game songs (Radio Gang and the campaign jingle).

Usage: python mix.py out.wav
The synth instruments are the ones Tools/Trailer/audio.py defines for the 60 s trailer; they are loaded from there.
"""
import json
import os
import subprocess
import sys

import numpy as np
from scipy import signal

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
SR = 48000
ART = "E:/NewSSDFile/Game Pemerintahan/Art/Audio_src/"
rng = np.random.default_rng(11)

with open(os.path.join(ROOT, "Content", "Data", "trailer.json"), encoding="utf-8") as f:
    SHOTS = json.load(f)["shots"]
START, at = {}, 0.0
for s in SHOTS:
    START[s["name"]] = (at, s["t"])
    at += s["t"]
DUR = at + 0.5
N = int(SR * DUR)
music = np.zeros((N, 2), np.float32)
fx = np.zeros((N, 2), np.float32)

# The instrument definitions (load, env, filters, saw, boom, braam, riser, whoosh, taiko, snare, tick, drone,
# bass_pulse, fade, sfx, tape_stop) live in the 60 s trailer's audio.py, between its imports and its score.
src = open(os.path.join(ROOT, "Tools", "Trailer", "audio.py"), encoding="utf-8").read()
helpers = src[src.index("def load("):src.index("# ---------- score")]
exec(helpers, globals())


def put_on(bus, sound, at, gain_db=0.0, pan=0.0):
    if sound.ndim == 1:
        sound = np.stack([sound * (1 - max(pan, 0)), sound * (1 + min(pan, 0))], 1)
    i = int(at * SR)
    if i >= N:
        return
    n = min(len(sound), N - i)
    bus[i:i + n] += sound[:n] * db(gain_db)


def shot(name):
    return START[name]


def amb(name, shot_name, gain, pad_in=0.3, pad_out=0.4, offset=0.0):
    """A sound under a whole shot, faded at both ends."""
    t0, length = shot(shot_name)
    x = sfx(name, offset, length)
    put_on(fx, fade(x, min(pad_in, length / 3), min(pad_out, length / 3)), t0, gain)


def radio(x):
    """Small speaker: band limited and a little crushed."""
    return np.tanh(bandpass(x, 350, 3400, 3) * 2.2) * 0.6


cards = [s["name"] for s in SHOTS if s.get("card")]
bab_cards = [c for c in cards if c.startswith("kartu BAB")]
end_of_story = shot("hidup jalan terus")[0]
title = shot("kartu GEMAH RIPAH")[0]

# ---------- score ----------
# Opening: a low drone and a clock tick over the black card and the smog.
t_bab1 = shot(bab_cards[0])[0]
put_on(music, fade(drone(t_bab1 + 0.5), 2.0, 1.0), 0.0, -5)
for k in range(int((t_bab1 - 0.6) / 0.6)):
    put_on(music, tick(), 0.6 + k * 0.6, -18 + 7 * k / 14, pan=0.3 if k % 2 else -0.3)
put_on(music, boom(2.6, 90, 30), shot("kota berasap")[0], -7)

# From Bab 1 on: a pulse that grows louder chapter by chapter, a drone under it, hits on every chapter card.
for i, card in enumerate(bab_cards):
    c0, clen = shot(card)
    c1 = shot(bab_cards[i + 1])[0] if i + 1 < len(bab_cards) else end_of_story
    root = (55.0, 55.0, 58.27, 61.74, 65.41)[i]
    put_on(music, riser(1.3, 300, 6000), c0 - 1.3, -15)
    put_on(music, whoosh(0.7), c0 - 0.35, -10)
    put_on(music, boom(), c0, -3)
    put_on(music, braam(3.4, root), c0, -6)
    body = c1 - (c0 + clen)
    put_on(music, fade(drone(body + 0.6, root), 0.4, 0.6), c0 + clen, -9 + i)
    put_on(music, fade(bass_pulse(body + 0.3, root, 0.3 - 0.015 * i), 0.2, 0.4), c0 + clen, -13 + 1.6 * i)

# A taiko on every cut inside a chapter, a snare on the second beat after.
for s in SHOTS:
    t0, _ = shot(s["name"])
    if not s.get("card") and t_bab1 < t0 < title:
        put_on(music, taiko(), t0, -6)
        put_on(music, snare(), t0 + 0.3, -16)

# Snare roll into Bab 5, the last chapter.
b5 = shot(bab_cards[-1])[0]
for k in range(16):
    put_on(music, snare(0.2), b5 - 2.0 + k * 0.125, -22 + k)

# The story stops: the pulse winds down like a tape, then the title lands on a braam and a long boom.
stop0 = end_of_story - 0.05
seg = music[int((stop0 - 1.2) * SR):int(stop0 * SR)].copy()
music[int((stop0 - 1.2) * SR):] *= 0
music[int((stop0 - 1.2) * SR):int((stop0 - 1.2) * SR) + len(tape_stop(seg, 1.0))] += tape_stop(seg, 1.0)
put_on(music, boom(3.5, 80, 28), title, -2)
put_on(music, braam(5.0, 55.0), title, -5)
put_on(music, fade(drone(5.5, 55.0), 0.2, 3.0), title, -10)

# ---------- songs ----------
# Radio Gang on Pak Harso's radio, the campaign jingle under the candidates. The score ducks under both.
duck = np.ones(N, np.float32)
for shot_name, path, offset, gain, small in (("radio gang", "MS_Radio_PakVramroro.wav", 11.2, -6, True),
                                              ("calon satu", "MS_Kampanye_OkeGas.wav", 16.0, -8, False),
                                              ("bu ratna", "MS_Kampanye_OkeGas.wav", 30.0, -10, False)):
    t0, length = shot(shot_name)
    x = load(ART + path, offset, length + 0.2)
    if small:
        x = radio(x)
    put_on(fx, fade(x, 0.15, 0.5), t0, gain)
    a, b = int(t0 * SR), int((t0 + length) * SR)
    ramp = int(0.25 * SR)
    duck[a:b] = db(-9)
    duck[a - ramp:a] = np.linspace(1, db(-9), ramp)
    duck[b:b + ramp] = np.linspace(db(-9), 1, ramp)
music *= duck[:, None]

# ---------- ambience and SFX ----------
amb("S_Amb_Jalan", "kota berasap", -16, 1.0, 1.0)
amb("S_Mesin_Motor", "bayu ngojol", -12)
amb("S_Amb_Jalan", "bayu ngojol", -15)
put_on(fx, sfx("S_Klakson"), shot("bayu ngojol")[0] + 2.2, -14, pan=-0.4)
amb("S_Amb_Burung", "ibu dan dinda", -16)
amb("S_Amb_Jangkrik", "menyusup dapur", -10)
for k in range(5):
    put_on(fx, sfx("S_Langkah_Rumput_%d" % k), shot("menyusup dapur")[0] + 0.6 + k * 0.55, -16)
amb("S_Amb_Jangkrik", "unggah di warnet", -16)
put_on(fx, sfx("S_Notif_Info"), shot("unggah di warnet")[0] + 1.6, -10)
amb("S_Amb_Jangkrik", "spn datang", -16)
amb("S_Sirene", "spn datang", -12)
amb("S_Mesin_Mobil", "spn datang", -14)
amb("S_Amb_Massa", "pamflet", -18)
amb("S_Amb_Massa", "long march", -6, 0.8)
for k in range(8):
    put_on(fx, sfx("S_Langkah_Beton_%d" % (k % 5)), shot("long march")[0] + 0.4 + k * 0.6, -14, pan=0.2)
amb("S_Amb_Massa", "depan dewan", -5)
amb("S_Sirene", "depan dewan", -16)
amb("S_Amb_Massa", "ditangkap", -11)
put_on(fx, sfx("S_Tabrak_Badan_2"), shot("ditangkap")[0] + 0.5, -6)
put_on(fx, sfx("S_Tabrak_Logam_1"), shot("rudi di sel")[0] + 0.2, -8)
amb("S_Amb_Jangkrik", "radio gang", -20)
amb("S_Amb_Jalan", "pria berkemeja", -18)
amb("S_Amb_Massa", "sidang", -20)
amb("S_Amb_Burung", "patok oranye", -16)
put_on(fx, sfx("S_Tabrak_Barang_3"), shot("patok oranye")[0] + 0.8, -10)
amb("S_Amb_Jalan", "humas lambai", -18)
amb("S_Amb_Massa", "pedagang marah", -8)
t0, length = shot("ekskavator")
diesel = lowpass(sfx("S_Mesin_Angkot", 0, length), 900, 2)
put_on(fx, fade(diesel, 0.8, 0.4), t0, -2)
put_on(fx, sfx("S_Tabrak_Logam_3"), t0 + length - 1.4, -4)
amb("S_Amb_Burung", "calon satu", -20)
amb("S_Amb_Jangkrik", "serangan fajar", -14)
put_on(fx, sfx("S_Koin_0"), shot("serangan fajar")[0] + 1.2, -8)
put_on(fx, sfx("S_Koin_1"), shot("serangan fajar")[0] + 1.5, -10)
amb("S_Amb_Massa", "bu ratna", -16)
amb("S_Amb_Burung", "tps", -14)
amb("S_Amb_Massa", "kotak dibuka", -18)
put_on(fx, sfx("S_Notif_Baik"), shot("kotak dibuka")[0] + 1.0, -12)
amb("S_Mesin_Motor", "hidup jalan terus", -12)
amb("S_Amb_Jalan", "hidup jalan terus", -14)
put_on(fx, sfx("S_UI_Klik"), title + 0.02, -12)

out = music + fx
out = np.tanh(out * 1.1) / np.tanh(1.1)
peak = np.max(np.abs(out))
out = (out / max(peak, 1e-6) * db(-1.0)).astype(np.float32)
out = (np.clip(out, -1, 1) * 32767).astype(np.int16)
subprocess.run(["ffmpeg", "-v", "error", "-y", "-f", "s16le", "-ar", str(SR), "-ac", "2", "-i", "-", sys.argv[1]], input=out.tobytes(), check=True)
print("%s %.1f detik" % (sys.argv[1], DUR))
