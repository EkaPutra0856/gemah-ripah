"""Writes Content/Data/trailer.json, the 2 minute in-game trailer played by UGRTrailerScreen (cheat GRTrailer),
and a dev script that plays it: trailer.txt for recording, or trailer_test.txt that also takes a screenshot mid-shot.

Usage: python shots.py [out_dir_for_scripts]
"""
import json
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
DATA = os.path.join(ROOT, "Content", "Data")


def bab(n):
    with open(os.path.join(DATA, "bab%d.json" % n), encoding="utf-8") as f:
        return json.load(f)


def prop(n, spawn_id, **over):
    """A prop exactly as the chapter places it (model, size, colour, label)."""
    for s in bab(n)["spawns"]:
        if s.get("id") == spawn_id:
            out = {k: s[k] for k in ("pos", "yaw", "size", "color", "model", "shape") if k in s}
            out.update(over)
            return out
    raise KeyError(spawn_id)


def person(pid, name, pos, yaw=0, **kw):
    out = {"id": pid, "name": name, "pos": list(pos), "yaw": yaw}
    out.update(kw)
    return out


def card(title, sub="", t=2.4, **kw):
    out = {"name": "kartu " + (title or sub)[:20], "t": t, "card": True, "title": title, "sub": sub, "cast": [], "props": [], "veh": [], "fadein": 0.35, "fadeout": 0.35}
    out.update(kw)
    return out


def row(prefix, name, start, step, count, yaw, **kw):
    """People standing in a line from start, step apart."""
    return [person("%s%d" % (prefix, i), name, (start[0] + step[0] * i, start[1] + step[1] * i), yaw, **kw) for i in range(count)]


EXTRAS = ["N15_Bapak_warga", "N16_Ibu_warga", "N19_Pemuda", "N21_Ojol_bapak", "N24_Buruh", "N36_Mahasiswa", "N37_Mahasiswi",
          "N41_Ibu_PKK", "N47_Ibu_berjilbab", "N40_Pemuda_gondrong", "N20_Pedagang_pasar", "N51_Pemuda_bertopi"]

shots = []
add = shots.append

# --- Opening ---------------------------------------------------------------------------------
add(card("", "Republik Gemah Ripah. Musim asap tahun ketiga.", t=3.2, hour=7.2, ispu=260, fadein=0.9))
add({"name": "kota berasap", "t": 5.0, "hour": 7.2, "ispu": 260, "cast": [], "props": [], "veh": [],
     "cam": [-20000, -25000, 6000, -18, 51], "cam2": [-15500, -19000, 4400, -16, 51], "focal": 28,
     "text": "Kata baliho, udaranya baik.", "fadein": 0.5})

# --- Bab 1: Asap dan Piring ---------------------------------------------------------------------
add(card("BAB 1", "Asap dan Piring", hour=8.0, ispu=210))
add({"name": "bayu ngojol", "t": 4.6, "hour": 8.0, "ispu": 200, "cast": [], "props": [], "veh": [],
     "ride": {"pos": [-14000, 400], "yaw": 0, "throttle": 0.85},
     "follow": "bayu_motor", "offset": [-520, 120, 190], "offset2": [-300, -230, 140], "aimz": 150, "focal": 30,
     "text": "Bayu, 24 tahun. Pengemudi OjekKu."})
add({"name": "ibu dan dinda", "t": 4.0, "hour": 8.5, "roofs": False,
     "cast": [person("ibu", "Bu Sri", (-8950, -9150), -63, anim="Talk"),
              person("dinda", "Dinda", (-8850, -9350), 120, pose="duduk", anim="Sit")],
     "props": [], "veh": [],
     "cam": [-9400, -9500, 230, -10, 15], "cam2": [-9350, -9470, 220, -10, 15], "aim": "dinda", "aimz": 130, "focal": 30,
     "text": "Adiknya muntah setelah makan Piring Rakyat."})
add({"name": "menyusup dapur", "t": 4.6, "hour": 21.0, "ispu": 150,
     "cast": [person("satpam", "Satpam Dapur", (18800, 19300), 90, walk=[18800, 19900], speed=120, light=[900, 28]),
              person("bayu", "Bayu", (18450, 20250), -90, walk=[18450, 19650], speed=170)],
     "props": [], "veh": [],
     "cam": [18100, 20500, 520, -32, -45], "cam2": [18150, 20350, 480, -30, -50], "aim": "bayu", "aimz": 120, "focal": 28,
     "text": "Buktinya disimpan di dapur."})
add({"name": "unggah di warnet", "t": 3.6, "hour": 21.0, "roofs": False,
     "cast": [person("bayu", "Bayu", (-2750, 6150), 180, anim="Ponsel"),
              person("joko", "Mas Joko", (-2250, 6650), 180, pose="duduk", anim="Sit")],
     "props": [], "veh": [],
     "cam": [-3200, 5900, 230, -10, 30], "cam2": [-3150, 5950, 220, -8, 28], "aim": "bayu", "aimz": 160, "focal": 40,
     "text": "Satu unggahan."})
add({"name": "spn datang", "t": 4.2, "hour": 22.0,
     "cast": [person("wiryo", "Komandan Wiryo", (-7650, -8350), -135, anim="Siaga"),
              person("spn1", "Petugas SPN", (-7450, -8150), -135, kind="spn", anim="Siaga"),
              person("spn2", "Petugas SPN", (-7850, -8150), -135, kind="spn", anim="Siaga")],
     "veh": [{"id": "mobil", "mesh": "V03_Mobil_taktis_aparat", "anim": "Roda", "pos": [-7000, -7700], "yaw": -135}],
     "props": [],
     "cam": [-8150, -8900, 180, 2, 45], "cam2": [-8050, -8850, 170, 3, 45], "aim": "wiryo", "aimz": 170, "focal": 35,
     "text": "Malam itu, SPN mengetuk pintu."})

# --- Bab 2: Turun ke Jalan ----------------------------------------------------------------------
add(card("BAB 2", "Turun ke Jalan", hour=10.0, ispu=180))
add({"name": "pamflet", "t": 3.8, "hour": 10.0, "ispu": 170,
     "cast": [person("sari", "Sari", (-8650, 12350), 0, anim="Talk"),
              person("bayu", "Bayu", (-8400, 12350), 180, anim="Lambai"),
              person("w1", "Warga", (-8500, 12650), -90, model="N41_Ibu_PKK", anim="Lirik")],
     "props": [prop(2, "tempel_pasar")], "veh": [],
     "cam": [-8150, 11800, 200, -6, 125], "cam2": [-8180, 11830, 195, -5, 125], "aim": "sari", "aimz": 160, "focal": 32,
     "text": "Warga mulai bergerak."})
march = []
for i in range(18):
    lane = (i % 3 - 1) * 170
    march.append(person("m%d" % i, "Warga Aksi", (7600 - (i // 3) * 230, 1300 + lane), 0, model=EXTRAS[i % len(EXTRAS)],
                        walk=[16000 - (i // 3) * 230, 1300 + lane], speed=135))
march[0].update(name="Bu Yati", model="N16_Ibu_warga")
add({"name": "long march", "t": 5.4, "hour": 9.5, "ispu": 190, "cast": march, "props": [], "veh": [],
     "cam": [8900, 650, 140, 6, 175], "cam2": [9000, 600, 150, 4, 178], "aim": "m0", "aimz": 170, "focal": 30,
     "text": "Piring kosong untuk Dewan."})
spn_line = row("s", "Petugas SPN", (35500, 600), (0, 300), 7, 180, kind="spn", anim="Siaga")
spn_line[3] = person("wiryo", "Komandan Wiryo", (35300, 1500), 180, anim="Marah")
massa = [person("a%d" % i, "Warga Aksi", (34100 - (i % 2) * 220, 700 + i * 200), 0, model=EXTRAS[(i + 3) % len(EXTRAS)], anim="Marah" if i % 2 else "Siaga")
         for i in range(8)]
add({"name": "depan dewan", "t": 4.2, "hour": 11.0, "ispu": 180, "cast": spn_line + massa,
     "props": [prop(2, "spanduk")], "veh": [{"mesh": "V03_Mobil_taktis_aparat", "pos": [36600, 900], "yaw": 90}],
     "cam": [33600, 2100, 230, -4, -12], "cam2": [33900, 1900, 220, -3, -8], "aim": "wiryo", "aimz": 170, "focal": 30,
     "text": "Pagar Dewan sudah dikunci."})
add({"name": "ditangkap", "t": 3.4, "hour": 12.0, "ispu": 180,
     "cast": [person("bayu", "Bayu", (30000, -1200), 180, anim="Menyerah", say="Saya cuma bawa piring, Pak!"),
              person("t1", "Petugas SPN", (30180, -1050), -150, kind="spn", anim="Tangkap"),
              person("t2", "Petugas SPN", (30180, -1380), 150, kind="spn", anim="Tangkap")],
     "props": [], "veh": [],
     "cam": [29550, -1100, 170, 0, -10], "cam2": [29620, -1150, 165, 0, -6], "aim": "bayu", "aimz": 160, "focal": 45})

# --- Bab 3: Pasal Karet -------------------------------------------------------------------------
add(card("BAB 3", "Pasal Karet", hour=15.0, ispu=170))
add({"name": "rudi di sel", "t": 3.8, "hour": 15.0, "roofs": False,
     "cast": [person("rudi", "Rudi", (6500, -3850), 90, pose="duduk", anim="Sit"),
              person("jaga", "Petugas Piket", (6050, -3700), 0, kind="spn", anim="Siaga")],
     "props": [prop(3, "sel_bangku")], "veh": [],
     "cam": [6500, -3250, 190, -8, -90], "cam2": [6480, -3290, 185, -8, -92], "aim": "rudi", "aimz": 120, "focal": 32,
     "text": "Pasal baru: Penghinaan Pejabat. Rudi tersangka pertama."})
add({"name": "radio gang", "t": 3.8, "hour": 20.5, "roofs": False,
     "cast": [person("harso", "Pak Harso", (1450, -9250), 180, anim="Radio")],
     "props": [], "veh": [],
     "cam": [1000, -9080, 190, -6, -20], "cam2": [1060, -9110, 185, -6, -18], "aim": "harso", "aimz": 160, "focal": 35,
     "text": "Radio Gang 107,7. Mengudara dari tukang fotokopi."})
add({"name": "pria berkemeja", "t": 3.2, "hour": 17.0, "ispu": 170,
     "cast": [person("intel", "Pria Berkemeja", (-4950, -11400), 160, model="N09_Intel", anim="Lirik")],
     "props": [], "veh": [],
     "cam": [-5300, -11700, 175, 0, 40], "cam2": [-5260, -11660, 175, 0, 40], "aim": "intel", "aimz": 165, "focal": 55,
     "text": "Di setiap sudut, ada yang mencatat."})
visitors = [person("v%d" % i, "Warga", (5250 + (i // 3) * 170, -4800 + (i % 3) * 160), 0, model=EXTRAS[(i + 5) % len(EXTRAS)], anim="Lirik" if i % 2 else "Siaga")
            for i in range(6)]
add({"name": "sidang", "t": 3.8, "hour": 10.0, "roofs": False,
     "cast": [person("hakim", "Hakim", (5900, -4600), 180, model="N04_Pejabat_pria", anim="Talk")] + visitors,
     "props": [], "veh": [],
     "cam": [4800, -4600, 300, -10, 0], "cam2": [4880, -4600, 290, -9, 0], "aim": "hakim", "aimz": 150, "focal": 30,
     "text": "Laporan warga nomor 47. Pelapornya sudah lama meninggal."})

# --- Bab 4: Proyek Mercusuar --------------------------------------------------------------------
add(card("BAB 4", "Proyek Mercusuar", hour=9.0, ispu=190))
add({"name": "patok oranye", "t": 3.8, "hour": 9.0, "ispu": 190,
     "cast": [person("petugas", "Petugas Proyek", (-9200, 12900), 200, model="N35_Petugas_kebersihan", anim="Siaga"),
              person("ijah", "Mak Ijah", (-9700, 12500), 30, anim="Kaget")],
     "props": [prop(4, "patok1"), prop(4, "patok2"), prop(4, "patok3")], "veh": [],
     "cam": [-8700, 12250, 200, -8, 150], "cam2": [-8730, 12280, 195, -7, 150], "aim": "petugas", "aimz": 150, "focal": 22,
     "text": "Patok oranye tumbuh lebih cepat dari sayur."})
add({"name": "humas lambai", "t": 3.4, "hour": 11.0, "ispu": 190,
     "cast": [person("hendro", "Pak Hendro", (7200, -8100), 180, anim="Lambai")],
     "props": [prop(4, "baliho_menara")], "veh": [],
     "cam": [6300, -8600, 150, 8, 35], "cam2": [6330, -8570, 150, 8, 35], "aim": "hendro", "aimz": 260, "focal": 24,
     "text": "Menara Gemilang. 88 lantai, untuk rakyat."})
add({"name": "pedagang marah", "t": 3.8, "hour": 13.0, "ispu": 190,
     "cast": [person("liong", "Koh Liong", (-9500, 13300), 0, anim="Marah"),
              person("ijah", "Mak Ijah", (-9500, 12950), 0, anim="Marah"),
              person("b1", "Barikade SPN", (-8500, 12900), 180, kind="spn", anim="Siaga"),
              person("b2", "Barikade SPN", (-8500, 13350), 180, kind="spn", anim="Siaga")],
     "props": [], "veh": [],
     "cam": [-8750, 13000, 180, -3, 158], "cam2": [-8790, 13010, 178, -3, 158], "aim": "liong", "aimz": 160, "focal": 30,
     "text": "Tujuh hari untuk pergi. Mereka memilih tinggal."})
traders = row("p", "Pedagang", (-8150, 12500), (0, 230), 6, 0, anim="Siaga")
for i, t in enumerate(traders):
    t["model"] = ["N20_Pedagang_pasar", "N17_Nenek", "N15_Bapak_warga", "N16_Ibu_warga", "N24_Buruh", "N41_Ibu_PKK"][i]
add({"name": "ekskavator", "t": 4.4, "hour": 14.0, "ispu": 200, "cast": traders, "props": [],
     "veh": [{"id": "eks", "mesh": "V06_Ekskavator", "anim": "Gali", "pos": [-5600, 13050], "to": [-6900, 13050], "yaw": 180}],
     "cam": [-8550, 12450, 180, 4, 10], "cam2": [-8600, 12450, 175, 5, 10], "aim": "eks", "aimz": 300, "focal": 28, "f": 0,
     "text": "Lalu mesinnya datang."})

# --- Bab 5: Suara Rakyat ------------------------------------------------------------------------
add(card("BAB 5", "Suara Rakyat", hour=16.0, ispu=170))
add({"name": "calon satu", "t": 3.4, "hour": 16.0, "ispu": 170,
     "cast": [person("wira", "Wiratama Gemilang", (-2050, -2600), 0, anim="Lambai"),
              person("jefri", "Bang Jefri", (-2250, -2850), 30, anim="Siaga")],
     "props": [], "veh": [{"mesh": "V04_Mobil_dinas_mewah", "pos": [-2700, -2200], "yaw": 90}],
     "cam": [-1450, -2350, 200, -3, -160], "cam2": [-1500, -2380, 195, -3, -164], "aim": "wira", "aimz": 165, "focal": 35,
     "text": "Calon nomor 1: Ketua Dewan."})
add({"name": "serangan fajar", "t": 3.8, "hour": 5.1, "ispu": 150,
     "cast": [person("jefri", "Bang Jefri", (-5300, -11500), 200, anim="Talk"),
              person("w", "Warga", (-5520, -11580), 20, model="N47_Ibu_berjilbab", anim="Kaget")],
     "props": [], "veh": [],
     "cam": [-5100, -11800, 180, -2, 150], "cam2": [-5130, -11770, 178, -2, 146], "aim": "w", "aimz": 160, "focal": 30,
     "text": "Amplop datang sebelum fajar."})
fans = [person("r%d" % i, "Relawan", (3150 + (i % 3) * 150, 3150 + (i // 3) * 180), 220, model=EXTRAS[(i + 1) % len(EXTRAS)], anim="Lambai" if i % 2 else "Siaga")
        for i in range(6)]
add({"name": "bu ratna", "t": 3.8, "hour": 9.5, "ispu": 170,
     "cast": [person("ratna", "Bu Ratna", (2300, 2700), 40, anim="Lambai")] + fans,
     "props": [prop(5, "posko_ratna")], "veh": [],
     "cam": [1750, 2350, 200, -3, 40], "cam2": [1800, 2400, 195, -3, 42], "aim": "ratna", "aimz": 160, "focal": 35,
     "text": "Calon nomor 2: seorang guru honorer."})
queue = [person("q%d" % i, "Pemilih", (9200 + i * 10, 11000 - i * 170), 90, model=EXTRAS[(i + 7) % len(EXTRAS)], walk=[9200 + i * 10, 11650 - i * 170], speed=40)
         for i in range(5)]
tps_props = [prop(5, p) for p in ("tps_tenda", "kotak_suara", "tps_meja", "tps_kursi", "tps_kursi2", "tps_bilik1", "tps_bilik2")]
add({"name": "tps", "t": 4.6, "hour": 7.5, "ispu": 140,
     "cast": [person("kpps", "Pak Hartono", (9000, 12500), 180, pose="duduk", anim="SitTalk"),
              person("bayu", "Bayu", (9000, 12340), 180, pose="duduk", anim="Sit"),
              person("spn", "SPN", (9500, 11400), 120, kind="spn", anim="Lirik")] + queue,
     "props": tps_props, "veh": [],
     "cam": [9600, 11900, 220, -6, 144], "cam2": [9580, 11930, 215, -6, 144], "aim": "bayu", "aimz": 120, "focal": 32,
     "text": "Bayu jadi saksi. Mandat di saku, nasi bungkus di tas."})
add({"name": "kotak dibuka", "t": 3.6, "hour": 13.0, "ispu": 140,
     "cast": [person("kpps", "Pak Hartono", (8700, 11950), 180, anim="Talk"),
              person("w1", "Warga", (8600, 11800), 60, model="N16_Ibu_warga", anim="Kaget"),
              person("w2", "Warga", (8600, 12100), -60, model="N19_Pemuda", anim="Kaget")],
     "props": tps_props, "veh": [],
     "cam": [8200, 11900, 180, -6, 6], "cam2": [8260, 11905, 175, -6, 6], "aim": "kpps", "aimz": 150, "focal": 28,
     "text": "Enam jam kemudian, kotak suara dibuka."})

# --- Closing --------------------------------------------------------------------------------------
add({"name": "hidup jalan terus", "t": 4.2, "hour": 17.8, "ispu": 120, "cast": [], "props": [], "veh": [],
     "ride": {"pos": [20000, -400], "yaw": 180, "throttle": 0.7},
     "follow": "bayu_motor", "offset": [650, -160, 150], "offset2": [420, -260, 140], "aimz": 150, "focal": 32,
     "text": "Hidup jalan terus.", "fadeout": 0.6})
add(card("GEMAH RIPAH", "Suara rakyat bukan barang dagangan.", t=4.4, size=88, fadein=0.6, fadeout=0.8))
add(card("", "", t=1.0, fadein=0.0, fadeout=0.0))

total = sum(s["t"] for s in shots)
with open(os.path.join(DATA, "trailer.json"), "w", encoding="utf-8") as f:
    json.dump({"length": round(total, 2), "shots": shots}, f, ensure_ascii=False, indent=1)
print("%d shot, %.1f detik" % (len(shots), total))

# Dev scripts: new game, wait past the opening, play the trailer.
out = sys.argv[1] if len(sys.argv) > 1 else os.path.dirname(os.path.abspath(__file__))
start = 30.0
head = ["2.00 dialogjalan 0.3", "3.00 GRRekam mulai", "6.00 GRNewGame", "21.00 tunggu 300", "24.00 GRTekan Escape 0.1", "26.00 tunggu 60"]
play = ["%.2f GRTrailer" % start]
end = ["%.2f GRRekam vid_trailer" % (start + total + 1.5), "%.2f quit" % (start + total + 9.5)]
test, at = [], start
for i, s in enumerate(shots):
    test.append("%.2f shot trl_%02d" % (at + s["t"] * 0.6, i))
    at += s["t"]
for name, lines in (("trailer.txt", head + play + end), ("trailer_test.txt", head + play + test + end)):
    with open(os.path.join(out, name), "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
