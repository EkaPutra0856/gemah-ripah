"""Cuts a 4x4 Gemini icon sheet into rounded-square PNGs with transparent corners.

Usage: python Tools/Python/slice_icon_sheet.py <sheet.jpg> Content/Data/Icons "name1,name2,...(16, '-' = skip)" 128
The source sheets live in Tools/IconSheets (see Tools/IconSheets/README.md for the names of each tile).
"""
import sys
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

sheet, out_dir, names = sys.argv[1], sys.argv[2], sys.argv[3].split(",")
im = Image.open(sheet).convert("RGB")
a = np.asarray(im).astype(int)
ink = (a.max(axis=2) - a.min(axis=2) > 28) | (a.max(axis=2) < 200)  # coloured or dark = tile


def runs(profile, n):
    on = profile > profile.max() * 0.25
    out, start = [], None
    for i, v in enumerate(on):
        if v and start is None:
            start = i
        if not v and start is not None:
            out.append((start, i)); start = None
    if start is not None:
        out.append((start, len(on)))
    out = sorted(out, key=lambda r: r[1] - r[0], reverse=True)[:n]
    return sorted(out)


cols = runs(ink.sum(axis=0), 4)
rows = runs(ink.sum(axis=1), 4)
assert len(cols) == 4 and len(rows) == 4, (cols, rows)
S = int(sys.argv[4]) if len(sys.argv) > 4 else 256
for k, name in enumerate(names):
    if not name or name == "-":
        continue
    (y0, y1), (x0, x1) = rows[k // 4], cols[k % 4]
    side = max(x1 - x0, y1 - y0)
    cx, cy = (x0 + x1) / 2, (y0 + y1) / 2
    inset = side * 0.015
    box = (cx - side / 2 + inset, cy - side / 2 + inset, cx + side / 2 - inset, cy + side / 2 - inset)
    tile = im.crop(tuple(int(round(v)) for v in box)).resize((S, S), Image.LANCZOS).convert("RGBA")
    mask = Image.new("L", (S * 4, S * 4), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, S * 4 - 1, S * 4 - 1), radius=int(S * 4 * 0.22), fill=255)
    tile.putalpha(mask.resize((S, S), Image.LANCZOS))
    tile.save(f"{out_dir}/{name}.png")
print("rows", rows, "cols", cols)
