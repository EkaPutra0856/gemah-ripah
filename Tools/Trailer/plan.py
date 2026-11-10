"""Shared edit list for the 60 second trailer. Times in seconds."""
REK = "E:/NewSSDFile/Game Pemerintahan/GemahRipah/Saved/Rekaman/"
SRC = {
    "cs": REK + "Bab1_Cutscene.mp4",
    "gp": REK + "Bab1_Gameplay.mp4",
    "oj": REK + "Bab1_Ngojol.mp4",
    "gl": REK + "gameplay_lengkap_2menit.mp4",
}

# Pak Vramroro runs at 152 BPM: four beats per cut.
RB = 4 * 60 / 152.0

# (kind, source or text, source start, duration, options)
EDIT = [
    ("card", "", 0, 0.8, {}),
    ("clip", "cs", 0.4, 3.6, {"fadein": 0.8}),             # aerial Kota Sentosa, "Musim asap tahun ketiga"
    ("clip", "cs", 6.4, 3.6, {}),                           # Gedung Dewan, baliho says the air is fine
    ("clip", "cs", 12.2, 3.2, {}),                          # Bayu, 24, OjekKu driver
    ("clip", "oj", 20.6, 12.0 - 11.2, {}),                  # Bayu at his motor, order comes in
    # radio song: ojol montage
    ("clip", "oj", 22.0, RB, {}),
    ("clip", "oj", 33.5, RB, {}),
    ("clip", "oj", 44.6, RB, {}),
    ("clip", "gl", 16.0, RB, {}),
    ("clip", "gl", 43.0, RB, {}),
    ("clip", "oj", 59.6, RB, {}),
    ("clip", "oj", 62.8, RB, {}),
    ("clip", "oj", 78.4, RB, {}),
    # tension
    ("card", "Tapi asapnya makin tebal.", 0, 1.4, {}),
    ("clip", "gp", 66.0, 2.0, {}),                          # dr. Lestari: Dinda harus dirawat
    ("clip", "gp", 126.0, 1.8, {}),                         # SDN Sentosa 1
    ("clip", "gp", 150.0, 1.7, {}),                         # Bu Ratna, dapur PPR
    ("card", "Dan piring rakyat tetap kosong.", 0, 1.4, {}),
    ("clip", "gp", 198.0, 1.8, {}),                         # PENGUMUMAN PEMERINTAH, jam malam
    ("clip", "gl", 72.6, 1.8, {}),                         # sneaking past flashlights
    ("clip", "gp", 204.0, 1.4, {}),                         # gudang dapur at night
    ("clip", "gp", 256.0, 1.6, {}),                         # Warnet Jaya: Unggah ke Kicau
    ("card", "Satu unggahan cukup.", 0, 1.0, {}),
    ("clip", "gp", 264.0, 1.8, {}),                         # Komandan Wiryo, SPN
    ("clip", "gp", 270.0, 1.67, {}),                         # intel at the door
    # campaign song
    ("card", "Tenang saja. Pemilu sebentar lagi.", 0, 1.2, {}),
    ("clip", "gl", 99.4, 1.6, {}),                         # Proyek Mercusuar, Pak Hendro
    ("clip", "gl", 108.4, 1.6, {}),                         # TPS queue
    ("clip", "gp", 312.4, 1.6, {}),                         # Pak RT Darto
    # climax
    ("clip", "cs", 32.9, 3.0, {}),                          # Besok pagi, piring kosong ke Gedung Dewan
    ("clip", "oj", 36.0, 0.5, {}),
    ("clip", "gl", 75.0, 0.5, {}),
    ("clip", "cs", 18.6, 0.5, {}),
    ("clip", "gp", 6.0, 0.5, {}),
    ("title", "", 0, 5.0, {}),
]


def timeline():
    """Start time of every edit, and the total length."""
    t, out = 0.0, []
    for e in EDIT:
        out.append(t)
        t += e[3]
    return out, t


if __name__ == "__main__":
    starts, total = timeline()
    for s, e in zip(starts, EDIT):
        print("%6.2f  %-5s %-4s %6.2f %5.2f  %s" % (s, e[0], e[1][:4], e[2], e[3], e[1] if e[0] == "card" else ""))
    print("total %.2f" % total)
