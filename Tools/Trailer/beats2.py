"""Per-beat energy in low, mid and high bands, to find drops, breaks and crash hits."""
import sys
import numpy as np
from scipy.io import wavfile
from scipy.signal import butter, sosfilt

sr, x = wavfile.read(sys.argv[1]); x = x.astype(np.float32) / 32768
B, T0 = 0.6, 0.017
bands = {"lo": butter(4, 150, "low", fs=sr, output="sos"), "mid": butter(4, [300, 3000], "band", fs=sr, output="sos"), "hi": butter(4, 6000, "high", fs=sr, output="sos")}
env = {k: sosfilt(s, x) ** 2 for k, s in bands.items()}
n = int((len(x) / sr - T0) / B)
rows = []
for i in range(n):
    a, b = int((T0 + i * B) * sr), int((T0 + (i + 1) * B) * sr)
    rows.append([np.sqrt(env[k][a:b].mean()) for k in ("lo", "mid", "hi")])
rows = np.array(rows); rows /= rows.max(axis=0)
for i in range(0, n, 4):
    line = "  ".join("%d%d%d" % tuple(min(9, int(v * 9.99)) for v in rows[j]) for j in range(i, min(i + 4, n)))
    print("bar %2d %6.2f | %s" % (i // 4, T0 + i * B, line))
