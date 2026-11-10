"""Builds the 60 s trailer soundtrack: an original synthesized score, two in-game songs, and the game's SFX."""
import subprocess
import numpy as np
from scipy import signal

import plan

SR = 48000
DUR = 60.0
N = int(SR * DUR)
ART = "E:/NewSSDFile/Game Pemerintahan/Art/Audio_src/"
rng = np.random.default_rng(7)
mix = np.zeros((N, 2), np.float32)
starts, _ = plan.timeline()
CUT = {i: s for i, s in enumerate(starts)}


def load(path, start=0.0, dur=None):
    cmd = ["ffmpeg", "-v", "error", "-ss", str(start), "-i", path]
    if dur:
        cmd += ["-t", str(dur)]
    cmd += ["-ac", "2", "-ar", str(SR), "-f", "f32le", "-"]
    return np.frombuffer(subprocess.run(cmd, capture_output=True).stdout, np.float32).reshape(-1, 2).copy()


def db(x):
    return 10 ** (x / 20)


def put(sound, at, gain_db=0.0, pan=0.0):
    if sound.ndim == 1:
        sound = np.stack([sound * (1 - max(pan, 0)), sound * (1 + min(pan, 0))], 1)
    i = int(at * SR)
    if i >= N:
        return
    n = min(len(sound), N - i)
    mix[i:i + n] += sound[:n] * db(gain_db)


def env(n, attack, release, curve=4.0):
    t = np.arange(n) / SR
    a = np.clip(t / max(attack, 1e-4), 0, 1)
    r = np.exp(-curve * t / max(release, 1e-4))
    return a * r


def lowpass(x, hz, order=4):
    return signal.sosfilt(signal.butter(order, hz, "low", fs=SR, output="sos"), x, axis=0)


def highpass(x, hz, order=4):
    return signal.sosfilt(signal.butter(order, hz, "high", fs=SR, output="sos"), x, axis=0)


def bandpass(x, lo, hi, order=4):
    return signal.sosfilt(signal.butter(order, [lo, hi], "band", fs=SR, output="sos"), x, axis=0)


def saw(freq, n, detune=0.0):
    t = np.arange(n) / SR
    return 2 * ((t * freq * (1 + detune)) % 1.0) - 1


# ---------- instruments ----------
def boom(length=2.2, f0=110, f1=34):
    n = int(SR * length)
    t = np.arange(n) / SR
    f = f1 + (f0 - f1) * np.exp(-t * 9)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t * 2.2)
    click = lowpass(rng.standard_normal(n), 2500) * np.exp(-t * 60) * 0.6
    tail = lowpass(rng.standard_normal(n), 300) * np.exp(-t * 3) * 0.25
    return np.tanh(1.6 * (body + click + tail)) * 0.9


def braam(length=3.2, root=55.0):
    n = int(SR * length)
    t = np.arange(n) / SR
    x = np.zeros(n)
    for mul, g in ((1, 1.0), (1.5, 0.6), (2, 0.7), (3, 0.25)):
        for d in (-0.006, 0.0, 0.007):
            x += saw(root * mul, n, d) * g
    cutoff = 200 + 1600 * np.exp(-t * 1.5)
    y = np.zeros(n)
    # time-varying lowpass in blocks
    blk = 2048
    zi = None
    for s in range(0, n, blk):
        sos = signal.butter(2, float(cutoff[s]), "low", fs=SR, output="sos")
        if zi is None:
            zi = signal.sosfilt_zi(sos) * 0
        y[s:s + blk], zi = signal.sosfilt(sos, x[s:s + blk], zi=zi)
    y *= env(n, 0.04, length * 0.55, 2.0)
    return np.tanh(y * 0.5) * 0.8


def riser(length=2.0, lo=200, hi=5000):
    n = int(SR * length)
    t = np.arange(n) / SR
    noise = rng.standard_normal(n)
    out = np.zeros(n)
    blk = 1024
    for s in range(0, n, blk):
        c = lo * (hi / lo) ** (t[s] / length)
        sos = signal.butter(2, [c * 0.7, min(c * 1.4, SR / 2 - 100)], "band", fs=SR, output="sos")
        out[s:s + blk] = signal.sosfilt(sos, noise[s:s + blk])
    tone = np.sin(2 * np.pi * np.cumsum(80 * (8 ** (t / length))) / SR) * 0.25
    return (out * 3 + tone) * (t / length) ** 2


def whoosh(length=0.6):
    n = int(SR * length)
    t = np.arange(n) / SR
    x = bandpass(rng.standard_normal(n), 300, 4000) * np.sin(np.pi * t / length) ** 2
    return x * 1.5


def taiko(length=0.9):
    n = int(SR * length)
    t = np.arange(n) / SR
    f = 55 + 90 * np.exp(-t * 25)
    x = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t * 6)
    x += lowpass(rng.standard_normal(n), 900) * np.exp(-t * 35) * 0.5
    return np.tanh(x * 1.4)


def snare(length=0.35):
    n = int(SR * length)
    t = np.arange(n) / SR
    x = bandpass(rng.standard_normal(n), 900, 7000) * np.exp(-t * 16)
    x += np.sin(2 * np.pi * 190 * t) * np.exp(-t * 30) * 0.5
    return x * 0.8


def tick(length=0.05):
    n = int(SR * length)
    t = np.arange(n) / SR
    return highpass(rng.standard_normal(n), 3000) * np.exp(-t * 120) * 0.6


def drone(length, root=55.0):
    n = int(SR * length)
    x = sum(saw(root * m, n, d) * g for m, g in ((1, 1), (2, 0.4), (1.5, 0.3)) for d in (-0.004, 0.004))
    x = lowpass(x, 420, 2)
    lfo = 0.75 + 0.25 * np.sin(2 * np.pi * 0.21 * np.arange(n) / SR)
    return x * lfo * 0.25


def bass_pulse(length, root=55.0, step=0.3):
    n = int(SR * length)
    out = np.zeros(n)
    k = int(SR * step)
    note = np.arange(k) / SR
    pat = [1, 1, 1, 1, 1.189, 1.189, 0.891, 0.891]  # A A A A C C G G
    for j, s in enumerate(range(0, n - k, k)):
        f = root * pat[(j // 2) % len(pat)]
        tone = np.sign(np.sin(2 * np.pi * f * note)) * np.exp(-note * 9)
        out[s:s + k] += tone
    return lowpass(out, 380, 2) * 0.5


def fade(x, fin=0.0, fout=0.0):
    x = x.copy()
    a, b = int(fin * SR), int(fout * SR)
    if a:
        x[:a] *= np.linspace(0, 1, a)[:, None] if x.ndim == 2 else np.linspace(0, 1, a)
    if b:
        x[-b:] *= np.linspace(1, 0, b)[:, None] if x.ndim == 2 else np.linspace(1, 0, b)
    return x


def sfx(name, start=0.0, dur=None):
    x = load(ART + "SFX/" + name + ".wav", start, dur)
    if dur and len(x) < int(dur * SR):
        # Loops are short: repeat them to the length asked for.
        x = np.tile(x, (int(dur * SR) // len(x) + 1, 1))[: int(dur * SR)]
    return x


def tape_stop(x, length):
    """Slows the end of x to a halt, like a tape machine switched off."""
    n = int(length * SR)
    head, tail = x[:-n], x[-n:]
    speed = np.linspace(1, 0.05, n * 2)
    pos = np.cumsum(speed)
    pos = pos[pos < len(tail) - 1]
    out = np.stack([np.interp(pos, np.arange(len(tail)), tail[:, c]) for c in range(2)], 1)
    out *= np.linspace(1, 0, len(out))[:, None]
    return np.concatenate([head, out])


# ---------- score ----------
# Intro: drone with a clock tick, hits on the cutscene cuts.
put(fade(drone(12.6), 2.0, 1.5), 0.0, -4)
for k in range(int(11.2 / 0.6)):
    put(tick(), 0.6 + k * 0.6, -16 + 6 * (k / 18), pan=0.3 if k % 2 else -0.3)
put(boom(2.6, 90, 30), CUT[1], -6)
put(boom(), CUT[2], -3)
put(boom(), CUT[3], -3)
put(riser(1.4, 300, 6000), 12.0 - 1.4, -12)

# Ojol montage: Pak Vramroro, first heard through a phone speaker, then full.
song = load(ART + "MS_Radio_PakVramroro.wav", 11.2, 24.63 - 11.2 + 0.3)
song = song[: int((24.63 - 11.2) * SR)]
k = int(0.8 * SR)
song[:k] = bandpass(song[:k], 500, 3200) * 1.8
song = fade(song, 0.05, 0.25)
put(song, 11.2, -3)
put(fade(sfx("S_Mesin_Motor", 0, 13.0), 0.5, 1.0), 12.0, -22)
put(sfx("S_Order_Masuk"), CUT[4] + 0.05, -4)
put(sfx("S_Klakson"), CUT[10] + 0.3, -8, )
put(sfx("S_Koin_1"), CUT[12] + 0.35, -6)
put(sfx("S_Notif_Baik"), CUT[12] + 0.75, -8)
for c in (8, 11):
    put(whoosh(0.5), CUT[c] - 0.25, -16)

# Tension: drone, bass pulse at 100 BPM, drums building.
T0, T1 = CUT[13], 44.0
put(fade(drone(T1 - T0 + 0.5, 55.0), 0.3, 0.6), T0, -3)
put(fade(bass_pulse(T1 - CUT[14]), 0.5, 0.3), CUT[14], -6)
put(boom(2.4, 100, 32), CUT[13], -2)
put(boom(2.4, 100, 32), CUT[17], -2)
put(boom(2.8, 120, 30), CUT[22], 0)
b = 0
t = CUT[18]
while t < T1 - 0.05:
    put(taiko(), t, -6 if b % 2 == 0 else -10, pan=-0.2 if b % 2 else 0.2)
    if t >= CUT[21] and b % 2 == 1:
        put(snare(), t, -12)
    b += 1
    t += 0.6
for k in range(int((T1 - T0) / 0.3)):
    put(tick(), T0 + k * 0.3, -22, pan=0.4 if k % 2 else -0.4)
put(sfx("S_Notif_Buruk"), CUT[18] + 0.1, -5)
for d in (0.25, 0.7, 1.15):
    put(sfx("S_UI_Klik"), CUT[21] + d, -7)
put(fade(sfx("S_Sirene", 0, T1 - CUT[23] + 0.2), 0.4, 0.6), CUT[23], -14)
put(riser(1.6, 200, 7000), T1 - 1.6, -10)

# Campaign song: Oke Gas, cut short by a tape stop.
song2 = load(ART + "MS_Kampanye_OkeGas.wav", 16.0, 6.2)[: int(6.0 * SR)]
song2 = tape_stop(fade(song2, 0.02, 0.0), 0.7)
put(song2, 44.0, -5)
put(sfx("S_Amb_Massa", 0, 6.0), 45.0, -20)

# Climax: braam on the Dewan, quick hits, title.
put(braam(3.4), 50.0, -2)
put(boom(3.0, 80, 28), 50.0, -1)
put(fade(drone(10.0, 41.2), 0.2, 3.0), 50.0, -2)
for k, c in enumerate(range(30, 34)):
    put(taiko(), CUT[c], -2)
    put(whoosh(0.4), CUT[c] - 0.2, -12)
    put(snare(), CUT[c] + 0.25, -10)
put(riser(2.0, 150, 8000), 55.0 - 2.0, -8)
put(boom(4.5, 140, 24), 55.0, 2)
put(braam(4.8, 41.2), 55.0, -1)
put(boom(2.0, 90, 30), 56.4, -8)
put(fade(drone(4.8, 55.0), 0.6, 2.5), 55.2, -8)

# ---------- master ----------
y = mix.astype(np.float64)
y = highpass(y, 25, 2)
rms = np.sqrt(np.mean(y ** 2))
y *= db(-16) / rms
y = np.tanh(y * 1.2) / np.tanh(1.2)
y *= db(-1) / np.max(np.abs(y))
y[-int(0.5 * SR):] *= np.linspace(1, 0, int(0.5 * SR))[:, None]
out = (np.clip(y, -1, 1) * 32767).astype("<i2")
subprocess.run(["ffmpeg", "-v", "error", "-y", "-f", "s16le", "-ar", str(SR), "-ac", "2", "-i", "-", "trailer_audio.wav"], input=out.tobytes())
print("audio ok, rms %.1f dB, peak %.1f dB" % (20 * np.log10(np.sqrt(np.mean(y ** 2))), 20 * np.log10(np.max(np.abs(y)))))
