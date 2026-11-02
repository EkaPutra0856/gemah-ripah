"""Draws the game's flat white UI icons (phone apps, chapter map) as 128 px PNGs with transparency."""
import math
import sys
from pathlib import Path
from PIL import Image, ImageDraw

OUT = Path(sys.argv[1])
OUT.mkdir(parents=True, exist_ok=True)
S = 4  # supersampling
N = 128 * S
W = (255, 255, 255, 255)
T = (0, 0, 0, 0)


def canvas():
    img = Image.new("RGBA", (N, N), T)
    return img, ImageDraw.Draw(img)


def p(*v):
    return [x * S for x in v]


def save(img, name):
    img.resize((128, 128), Image.LANCZOS).save(OUT / f"{name}.png")


def scooter():
    img, d = canvas()
    d.ellipse(p(14, 78, 46, 110), outline=W, width=8 * S)
    d.ellipse(p(84, 78, 116, 110), outline=W, width=8 * S)
    d.polygon(p(40, 86, 92, 86, 100, 66, 58, 66, 52, 50, 40, 50), fill=W)
    d.line(p(92, 70, 84, 30), fill=W, width=8 * S)
    d.line(p(74, 30, 98, 30), fill=W, width=8 * S)
    d.rounded_rectangle(p(34, 40, 60, 52), radius=5 * S, fill=W)
    save(img, "app_ojekku")


def chat():
    img, d = canvas()
    d.rounded_rectangle(p(14, 20, 114, 88), radius=20 * S, fill=W)
    d.polygon(p(32, 80, 30, 110, 60, 86), fill=W)
    for x in (42, 64, 86):
        d.ellipse(p(x - 7, 47, x + 7, 61), fill=T)
    save(img, "app_pesan")


def bird():
    img, d = canvas()
    d.ellipse(p(26, 44, 98, 98), fill=W)  # body
    d.ellipse(p(70, 24, 106, 60), fill=W)  # head
    d.polygon(p(104, 36, 122, 44, 104, 50), fill=W)  # beak
    d.polygon(p(30, 70, 6, 52, 14, 84), fill=W)  # tail
    d.ellipse(p(86, 34, 94, 42), fill=T)  # eye
    d.polygon(p(40, 58, 76, 66, 50, 86), fill=T)  # wing cut
    save(img, "app_kicau")


def news():
    img, d = canvas()
    d.rounded_rectangle(p(16, 22, 112, 106), radius=8 * S, outline=W, width=8 * S)
    d.rectangle(p(30, 36, 98, 50), fill=W)
    d.rectangle(p(30, 58, 58, 92), fill=W)
    for y in (60, 72, 84):
        d.rectangle(p(66, y, 98, y + 6), fill=W)
    save(img, "app_berita")


def wallet():
    img, d = canvas()
    d.rounded_rectangle(p(14, 32, 114, 104), radius=12 * S, fill=W)
    d.polygon(p(24, 32, 86, 14, 94, 32), fill=W)
    d.rounded_rectangle(p(76, 56, 118, 80), radius=8 * S, fill=T)
    d.ellipse(p(84, 62, 96, 74), fill=W)
    save(img, "app_dompet")


def camera():
    img, d = canvas()
    d.rounded_rectangle(p(12, 36, 116, 104), radius=12 * S, fill=W)
    d.rounded_rectangle(p(40, 22, 80, 40), radius=6 * S, fill=W)
    d.ellipse(p(40, 46, 88, 94), fill=T)
    d.ellipse(p(50, 56, 78, 84), fill=W)
    d.ellipse(p(96, 44, 106, 54), fill=T)
    save(img, "app_galeri")


def pin():
    img, d = canvas()
    d.ellipse(p(30, 12, 98, 80), fill=W)
    d.polygon(p(34, 60, 94, 60, 64, 116), fill=W)
    d.ellipse(p(50, 32, 78, 60), fill=T)
    save(img, "app_peta")


def plate_smoke():
    img, d = canvas()
    d.ellipse(p(12, 70, 116, 112), fill=W)
    d.ellipse(p(30, 78, 98, 102), fill=T)
    d.ellipse(p(40, 82, 88, 98), fill=W)
    for i, x in enumerate((42, 64, 86)):
        pts = []
        for k in range(30):
            t = k / 29
            y = 64 - t * 52
            pts.append(((x + 7 * math.sin(t * 6.28 + i)) * S, y * S))
        d.line(pts, fill=W, width=6 * S, joint="curve")
    save(img, "bab1")


def megaphone():
    img, d = canvas()
    d.polygon(p(24, 50, 92, 18, 92, 102, 24, 74), fill=W)
    d.rounded_rectangle(p(10, 48, 30, 76), radius=4 * S, fill=W)
    d.polygon(p(36, 74, 52, 74, 60, 106, 44, 106), fill=W)
    for r in (14, 26):
        d.arc(p(92 - r, 60 - r - 6, 92 + r + 6, 60 + r), start=-50, end=50, fill=W, width=6 * S)
    save(img, "bab2")


def gag():
    img, d = canvas()
    # Speech bubble crossed by a padlock: "Pasal Karet".
    d.rounded_rectangle(p(10, 14, 92, 72), radius=16 * S, fill=W)
    d.polygon(p(24, 66, 20, 92, 46, 70), fill=W)
    d.arc(p(70, 50, 108, 92), start=180, end=360, fill=W, width=8 * S)
    d.rounded_rectangle(p(62, 70, 118, 116), radius=8 * S, fill=W)
    d.rounded_rectangle(p(62, 70, 118, 116), radius=8 * S, outline=(0, 0, 0, 0), width=1)
    d.ellipse(p(84, 82, 96, 94), fill=T)
    d.rectangle(p(87, 90, 93, 104), fill=T)
    save(img, "bab3")


def crane():
    img, d = canvas()
    d.rectangle(p(30, 20, 42, 112), fill=W)  # mast
    d.rectangle(p(14, 20, 116, 30), fill=W)  # jib
    d.line(p(36, 8, 14, 22), fill=W, width=5 * S)
    d.line(p(36, 8, 100, 22), fill=W, width=5 * S)
    d.line(p(96, 30, 96, 64), fill=W, width=4 * S)
    d.rectangle(p(84, 64, 108, 80), fill=W)  # load
    d.rectangle(p(16, 104, 120, 114), fill=W)  # ground
    d.rectangle(p(58, 70, 76, 104), fill=W)  # small house
    d.polygon(p(54, 72, 67, 58, 80, 72), fill=W)
    save(img, "bab4")


def ballot():
    img, d = canvas()
    d.rounded_rectangle(p(16, 56, 112, 114), radius=8 * S, fill=W)
    d.rectangle(p(40, 64, 88, 72), fill=T)
    d.polygon(p(44, 66, 52, 14, 96, 22, 86, 70), fill=W)
    d.line(p(58, 40, 66, 50, 82, 30), fill=T, width=6 * S)
    save(img, "bab5")


def check():
    img, d = canvas()
    d.line(p(20, 66, 50, 96, 108, 32), fill=W, width=16 * S, joint="curve")
    save(img, "status_selesai")


def lock():
    img, d = canvas()
    d.arc(p(36, 16, 92, 76), start=180, end=360, fill=W, width=12 * S)
    d.rectangle(p(36, 44, 48, 60), fill=W)
    d.rectangle(p(80, 44, 92, 60), fill=W)
    d.rounded_rectangle(p(22, 56, 106, 116), radius=10 * S, fill=W)
    d.ellipse(p(56, 72, 72, 88), fill=T)
    d.rectangle(p(61, 84, 67, 102), fill=T)
    save(img, "status_terkunci")


def star():
    img, d = canvas()
    pts = []
    for k in range(10):
        r = 56 if k % 2 == 0 else 24
        a = -math.pi / 2 + k * math.pi / 5
        pts.append(((64 + r * math.cos(a)) * S, (66 + r * math.sin(a)) * S))
    d.polygon(pts, fill=W)
    save(img, "status_sekarang")


for fn in (scooter, chat, bird, news, wallet, camera, pin, plate_smoke, megaphone, gag, crane, ballot, check, lock, star):
    fn()
print("ok", len(list(OUT.glob("*.png"))))
