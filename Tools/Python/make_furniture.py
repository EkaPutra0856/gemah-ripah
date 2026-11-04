"""Writes Content/Data/furniture.json: tables, chairs and room dressing shared by every chapter.

Run with plain Python (no editor needed):  python Tools/Python/make_furniture.py
The story subsystem appends these spawns to the chapter's own. Models are built in C++ (World/GRPropModels.cpp).

Conventions: yaw 0 faces +X (north). A seat's origin is where the sitter's feet are and the sitter faces the yaw;
a desk's user sits on its -X side. NPCs that sit here have "pose": "duduk" in the chapter files.
Furniture inside rooms that host a stealth level is hidden while "menyusup" is set; "bab" is the chapter number.
"""
import json
import math
from pathlib import Path

OUT = Path(__file__).resolve().parents[2] / "Content" / "Data" / "furniture.json"
spawns = []


def put(ident, model, x, y, yaw=0, size=None, cond="", color=None):
    s = {"id": "f_" + ident, "kind": "prop", "model": model, "pos": [round(x, 1), round(y, 1)], "yaw": yaw}
    if size:
        s["size"] = size
    if color:
        s["color"] = color
    if cond:
        s["if"] = cond
    spawns.append(s)


def ahead(x, y, yaw, dist):
    r = math.radians(yaw)
    return x + math.cos(r) * dist, y + math.sin(r) * dist


def desk_seat(ident, x, y, yaw, desk="meja_kantor", chair="kursi_kantor", dist=95, cond="", desk_size=None):
    """Chair under a sitter at (x, y) facing yaw, desk in front of them."""
    put(ident + "_kursi", chair, x, y, yaw, cond=cond)
    dx, dy = ahead(x, y, yaw, dist)
    put(ident + "_meja", desk, dx, dy, yaw, size=desk_size, cond=cond)


def pc_row(ident, x, y, yaw):
    """Warnet PC desk with its chair (the desk's user side faces yaw + 180)."""
    put(ident + "_meja", "meja_pc", x, y, yaw)
    cx, cy = ahead(x, y, yaw, -55)
    put(ident + "_kursi", "kursi_kantor", cx, cy, yaw)


# Kantor Kelurahan (x 7800..9200, y -9900..-8100, door in the -X wall). Pak Darmo sits facing the door.
KEL = "!menyusup"
desk_seat("kel_darmo", 8950, -9000, 180, cond=KEL)
desk_seat("kel_loket", 8650, -8400, 180, cond=KEL)
put("kel_arsip1", "lemari_arsip", 9160, -9350, 180, cond=KEL)
put("kel_arsip2", "lemari_arsip", 9160, -9290, 180, cond=KEL)
put("kel_rak", "rak", 9165, -8700, 180, cond=KEL)
put("kel_bangku", "bangku", 8300, -9780, 90, cond=KEL)
put("kel_dispenser", "dispenser", 9160, -8250, 180, cond=KEL)
put("kel_pot1", "pot", 7870, -8200, cond=KEL)
put("kel_pot2", "pot", 7870, -9820, cond=KEL)

# Warnet Jaya (x -3000..-2000, y 5600..6800, door in the +X wall). Mas Joko at the cashier desk, PCs along the -X wall.
desk_seat("warnet_kasir", -2250, 6650, 180)
for i, y in enumerate((5720, 5840, 5960, 6080, 6320, 6440)):
    pc_row(f"warnet_pc{i}", -2850, y, 180)
put("warnet_kursi7", "kursi_kantor", -2795, 6200, 180)  # PC Nomor 7 itself is a chapter prop.
put("warnet_dispenser", "dispenser", -2080, 5660, 180)
spawns.append({"id": "f_warnet_gamer1", "kind": "npc", "name": "Pelanggan Warnet", "pos": [-2795, 5840], "yaw": 180, "pose": "duduk",
               "color": "#6C757D", "barks": ["Sstt, lagi push rank.", "Paket data mahal, mending warnet.", "Jangan buka situs aneh, Mas Joko diawasi."]})
spawns.append({"id": "f_warnet_gamer2", "kind": "npc", "name": "Mahasiswa", "pos": [-2795, 6440], "yaw": 180, "pose": "duduk",
               "color": "#2A9D8F", "barks": ["Skripsi bab dua, sudah tiga semester.", "Wifi kampus diblokir, katanya demi keamanan."]})

# Rumah Bayu (x -9500..-8500, y -9600..-8400, door in the +X wall).
put("rumah_meja", "meja_makan", -8800, -8680, 0, size=[70, 120, 72])
for side, yaw in ((-1, 0), (1, 180)):
    for y in (-8710, -8650):
        put(f"rumah_kursi{side}{y}", "kursi", -8800 + side * 80, y, yaw)
put("rumah_dapur", "dapur", -9455, -8650, 0)
put("rumah_lemari", "lemari", -9300, -8460, -90)
desk_seat("rumah_dinda", -8850, -9350, 0, desk="meja", chair="kursi", dist=85, desk_size=[50, 90, 68])
# Living corner: rug, TV on the west wall with a photo above it, a photo by the dining table, a plant by the door.
put("rumah_karpet", "karpet", -9050, -9050, 0)
put("rumah_tv", "tv", -9050, -9560, 90)
put("rumah_foto", "bingkai", -9050, -9586, 90)
put("rumah_foto2", "bingkai", -8900, -8414, -90, size=[3, 45, 55], color="#D98E73")
put("rumah_pot", "pot", -8570, -9540)

# Dapur PPR (x 19100..20900, y 19300..20700, door in the -X wall): stoves on the north wall, prep tables in the
# middle, store shelves on the east and west walls. Hidden while the stealth level runs in this room.
DAP = "!menyusup"
for i, y in enumerate((19560, 20000, 20440)):
    put(f"dapur_kompor{i}", "kompor_besar", 20840, y, 180, cond=DAP)
for i, y in enumerate((19750, 20250)):
    put(f"dapur_meja{i}", "meja_dapur", 20300, y, 0, cond=DAP)
for i, x in enumerate((19450, 19650, 19850)):
    put(f"dapur_rak{i}", "rak_dapur", x, 20660, -90, cond=DAP)
for i, x in enumerate((19500, 19700)):
    put(f"dapur_rakb{i}", "rak_dapur", x, 19340, 90, cond=DAP)
put("dapur_dispenser", "dispenser", 19180, 20620, 0, cond=DAP)
for i, x in enumerate((19250, 19330)):
    put(f"dapur_kursi{i}", "kursi_plastik", x, 19420, 90, cond=DAP)

# Pos Ronda: Pak RT on the bench with kopi, a second bench facing the warung.
put("ronda_bangku", "bangku", -5650, -12300, 90)
put("ronda_meja", "meja_kecil", -5650, -12200, 90)
put("ronda_bangku2", "bangku", -6000, -11880, -90)

# Puskesmas: dr. Lestari's service desk under a tent in front of the building.
put("pkm_tenda", "tenda", 7860, 22300, 0, size=[300, 320, 240], color="#2A9D8F")
desk_seat("pkm_dokter", 7900, 22300, 180, desk="meja", chair="kursi", dist=95)
for i, y in enumerate((21560, 21620, 21680)):
    put(f"pkm_antre{i}", "kursi_plastik", 7620, y, 0)

# Pos SPN: Komandan Wiryo's duty desk outside the post.
desk_seat("spn_wiryo", 5850, -4500, 180)

# Fotokopi Pak Harso (from Bab 3): he sits at the mouth of the 2 m alley between two ruko, facing Jl. Raya;
# the stensil machine (a chapter prop) stands beside the alley on the ruko forecourt.
desk_seat("harso", 1450, -9250, 180, cond="bab>=3", desk_size=[60, 110, 75])

OUT.write_text(json.dumps({"spawns": spawns}, ensure_ascii=False, indent=1), encoding="utf-8")
print(f"{OUT}: {len(spawns)} spawns")
