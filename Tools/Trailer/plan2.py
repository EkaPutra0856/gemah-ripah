"""Edit list for the full-song trailer, cut on the beats of "Energetic Action Sport" (AlexGrohl, Pixabay).

The song runs at 100 BPM: one beat is 0.6 s, one bar 2.4 s, the first beat lands at 0.017 s.
Sections: intro 0-7.2, break 7.2-9.6, verse 9.6-28.8, chorus 28.8-48.0, bridge 48.0-67.2 (quiet bars at 50.4-55.2),
chorus 67.2-86.4, outro 86.4-96.0, final hit at 96.0 ringing out to 106.24.
"""
import os

REK = "E:/NewSSDFile/Game Pemerintahan/GemahRipah/Saved/Rekaman/"
HERE = os.path.dirname(os.path.abspath(__file__))
SONG = os.path.join(HERE, "song.wav")
SRC = {
    "cs": REK + "Bab1_Cutscene.mp4",
    "gp": REK + "Bab1_Gameplay.mp4",
    "oj": REK + "Bab1_Ngojol.mp4",
    "gl": REK + "gameplay_lengkap_2menit.mp4",
    "ct": REK + "Trailer_GemahRipah_2menit.mp4",   # in-game story cutscenes, Bab 1-5
    "hp": os.path.join(HERE, "hp_clean.mp4"),      # raw/hp3.mkv from 33.7 s: Bab1_HP_Bayu_v2 without its labels
}
BEAT, FIRST = 0.6, 0.017
SONG_LEN = 106.24

# (kind, source or text, source start, length in beats, options). The first entry also holds the 0.017 s lead-in,
# the title runs to the end of the song.
EDIT = [
    # intro
    ("clip", "ct", 3.4, 6, {"fadein": 0.6}),        # kota berasap, "Kata baliho, udaranya baik"
    ("clip", "cs", 6.4, 2, {}),                     # Gedung Dewan, the baliho
    ("clip", "cs", 12.2, 4, {}),                    # Bayu, 24 tahun, pengemudi OjekKu
    ("clip", "hp", 0.5, 1, {}),                     # phone home screen
    ("card", "Di Kota Sentosa, hidup itu sederhana.", 0, 3, {}),
    # verse: a day of ojol, phone in hand
    ("clip", "oj", 22.0, 2, {}),
    ("clip", "oj", 33.5, 2, {}),
    ("clip", "hp", 7.5, 4, {}),                     # OjekKu order list
    ("clip", "oj", 44.6, 2, {}),
    ("clip", "gl", 16.0, 2, {}),
    ("clip", "hp", 30.0, 4, {}),                    # Kicau, crossing at the zebra
    ("clip", "oj", 59.6, 2, {}),
    ("clip", "oj", 62.8, 2, {}),
    ("clip", "hp", 46.5, 4, {}),                    # Dompet, Rp 105.000
    ("clip", "gl", 43.0, 2, {}),
    ("clip", "oj", 78.4, 2, {}),
    ("clip", "hp", 22.5, 4, {}),                    # Pesan: "BAYU!! Dinda muntah-muntah"
    # chorus: the smog, the kitchen, the upload
    ("clip", "ct", 15.6, 4, {}),                    # ibu dan Dinda
    ("clip", "gp", 66.0, 2, {}),                    # dr. Lestari
    ("clip", "gp", 126.0, 2, {}),                   # SDN Sentosa 1
    ("clip", "ct", 19.8, 4, {}),                    # menyusup dapur, flashlights
    ("clip", "gp", 198.0, 2, {}),                   # pengumuman jam malam
    ("clip", "gl", 72.6, 2, {}),                    # sneaking
    ("clip", "ct", 24.2, 4, {}),                    # "Satu unggahan"
    ("clip", "ct", 27.8, 4, {}),                    # SPN at the door
    ("clip", "hp", 62.0, 4, {}),                    # Galeri, past Pos SPN
    ("clip", "ct", 38.4, 4, {}),                    # long march, "Piring kosong untuk Dewan"
    # bridge: the street, the arrests
    ("clip", "ct", 43.6, 4, {}),                    # depan Dewan
    ("clip", "ct", 47.8, 4, {}),                    # ditangkap
    ("card", "Yang bersuara, dipasalkan.", 0, 4, {}),
    ("clip", "ct", 53.6, 4, {}),                    # Rudi di sel
    ("clip", "ct", 57.4, 2, {}),                    # Radio Gang
    ("clip", "ct", 64.4, 2, {}),                    # sidang
    ("clip", "hp", 14.5, 4, {}),                    # Kata Warga
    ("clip", "ct", 70.6, 2, {}),                    # patok oranye
    ("clip", "ct", 81.6, 2, {}),                    # ekskavator
    ("card", "Tenang saja. Pemilu sebentar lagi.", 0, 4, {}),
    # chorus: the election
    ("clip", "ct", 88.4, 4, {}),                    # calon nomor 1
    ("clip", "ct", 91.8, 4, {}),                    # serangan fajar
    ("clip", "gl", 99.4, 2, {}),                    # Proyek Mercusuar
    ("clip", "gl", 108.4, 2, {}),                   # TPS queue
    ("clip", "ct", 95.6, 4, {}),                    # Bu Ratna
    ("clip", "ct", 99.4, 4, {}),                    # TPS
    ("clip", "ct", 104.0, 4, {}),                   # kotak suara dibuka
    ("clip", "gp", 312.4, 2, {}),                   # Pak RT Darto
    ("clip", "hp", 53.0, 2, {}),                    # Peta, to Gedung Dewan
    ("clip", "oj", 36.0, 1, {}),
    ("clip", "gl", 75.0, 1, {}),
    ("clip", "cs", 18.6, 1, {}),
    ("clip", "gp", 6.0, 1, {}),
    # outro
    ("clip", "cs", 32.9, 4, {}),                    # Gedung Dewan
    ("clip", "ct", 107.6, 4, {}),                   # hidup jalan terus
    ("clip", "hp", 39.0, 4, {}),                    # Berita, riding home
    ("card", "Kamu cuma ojol. Tapi suaramu dihitung.", 0, 4, {}),
    ("title", "", 0, None, {}),
]


def timeline():
    """Start and length of every edit in song time."""
    out, beats = [], 0
    for i, e in enumerate(EDIT):
        t0 = 0.0 if i == 0 else FIRST + beats * BEAT
        if e[3] is None:
            t1 = SONG_LEN
        else:
            beats += e[3]
            t1 = FIRST + beats * BEAT
        out.append((t0, t1 - t0))
    return out


if __name__ == "__main__":
    for (t0, d), e in zip(timeline(), EDIT):
        print("%7.3f %6.3f  %-5s %-3s %6.1f  %s" % (t0, d, e[0], e[1][:3], e[2], e[1] if e[0] == "card" else ""))
