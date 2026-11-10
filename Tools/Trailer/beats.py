"""Finds tempo, beat phase and loudness sections of the trailer song."""
import sys
import numpy as np
from scipy.io import wavfile
from scipy.signal import stft

sr, x = wavfile.read(sys.argv[1])
x = x.astype(np.float32) / 32768
hop = 256
f, t, Z = stft(x, sr, nperseg=1024, noverlap=1024 - hop)
mag = np.log1p(np.abs(Z) * 100)
flux = np.maximum(np.diff(mag, axis=1), 0).sum(axis=0)
flux = (flux - flux.mean()) / flux.std()
fps = sr / hop
# Tempo from autocorrelation between 80 and 180 BPM.
ac = np.correlate(flux, flux, "full")[len(flux) - 1:]
lags = np.arange(len(ac))
bpm = 60 * fps / np.maximum(lags, 1)
ok = (bpm > 80) & (bpm < 180)
lag = lags[ok][np.argmax(ac[ok])]
# refine
best = None
for L in np.arange(lag - 2, lag + 2, 0.01):
    period = L
    for ph in np.arange(0, period, 0.5):
        idx = (ph + period * np.arange(int((len(flux) - ph) / period))).astype(int)
        s = flux[idx].sum()
        if best is None or s > best[0]:
            best = (s, period, ph)
_, period, ph = best
print("BPM %.3f  beat %.4f s  first beat %.3f s" % (60 * fps / period, period / fps, ph / fps))
# Loudness per bar (4 beats)
rms_hop = int(sr * 0.05)
rms = np.sqrt(np.convolve(x ** 2, np.ones(rms_hop) / rms_hop, "same"))
beat = period / fps
t0 = ph / fps
bar = 4 * beat
n = int((len(x) / sr - t0) / bar)
for i in range(n + 1):
    a, b = t0 + i * bar, t0 + (i + 1) * bar
    seg = rms[int(a * sr):int(min(b, len(x) / sr) * sr)]
    lo = flux[int(a * fps):int(b * fps)]
    print("bar %2d %6.2f  rms %.3f  flux %5.2f %s" % (i, a, seg.mean() if len(seg) else 0, lo.mean() if len(lo) else 0, "#" * int((seg.mean() if len(seg) else 0) * 120)))
