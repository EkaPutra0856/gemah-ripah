"""Builds /Game/GemahRipah/Maps/L_KotaSentosa: grey-box open world for Bab 1 (about 1 x 1 km, World Partition).

Layout (X = north, Y = east, 1 unit = 1 cm):
  Jl. Raya Sentosa runs west-east at X=0, Jl. Merdeka runs south-north at Y=0, Tugu at the crossing.
  South-west: Kampung Gang (rumah Bayu, pos ronda, musala)   South-east: Pasar Sentosa (+ warnet on Jl. Raya)
  North-west: Pusat Pemerintahan (kelurahan, pos SPN, Gedung Dewan at the north end)
  North-east: Kawasan Sekolah (SDN Sentosa 1, dapur PPR, puskesmas)

Every actor is "always loaded" (not spatially streamed) so the runtime map capture sees the whole city.
Run headless via Tools/BuildKotaSentosa.bat. If the map already exists, the script stops and changes nothing.
"""
import random
import unreal

MAP_PATH = "/Game/GemahRipah/Maps/L_KotaSentosa"
MAT_DIR = "/Game/GemahRipah/Materials/Blockout"
CUBE = "/Engine/BasicShapes/Cube.Cube"
PLANE = "/Engine/BasicShapes/Plane.Plane"
CYLINDER = "/Engine/BasicShapes/Cylinder.Cylinder"
SPHERE = "/Engine/BasicShapes/Sphere.Sphere"
CHAMFER = "/Game/LevelPrototyping/Meshes/SM_ChamferCube.SM_ChamferCube"
GRID_PARENT = "/Game/LevelPrototyping/Materials/M_PrototypeGrid.M_PrototypeGrid"

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
rng = random.Random(7)
_meshes = {}
_mats = {}


def mesh(path):
    if path not in _meshes:
        _meshes[path] = unreal.load_asset(path)
    return _meshes[path]


def mat(name, rgb):
    """Grid material instance in a flat color (created once, reused)."""
    if name in _mats:
        return _mats[name]
    path = f"{MAT_DIR}/MI_{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        mi = unreal.load_asset(path)
    else:
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        mi = tools.create_asset(f"MI_{name}", MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel = unreal.MaterialEditingLibrary
        mel.set_material_instance_parent(mi, unreal.load_asset(GRID_PARENT))
        base = unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0)
        line = unreal.LinearColor(rgb[0] * 0.7, rgb[1] * 0.7, rgb[2] * 0.7, 1.0)
        for p in ("SurfaceColor", "TopSurfaceColor"):
            mel.set_material_instance_vector_parameter_value(mi, p, base)
        for p in ("GridColor", "TopGridColor", "SubGridColor", "TopSubGridGridColor"):
            mel.set_material_instance_vector_parameter_value(mi, p, line)
        unreal.EditorAssetLibrary.save_loaded_asset(mi)
    _mats[name] = mi
    return mi


def finish(actor, label, folder):
    actor.set_actor_label(label)
    actor.set_folder_path(folder)
    actor.set_editor_property("is_spatially_loaded", False)
    return actor


def spawn(cls, loc, rot=(0, 0, 0), label="Actor", folder="Misc"):
    # rot is (pitch, yaw, roll); unreal.Rotator's positional order is roll, pitch, yaw.
    pitch, yaw, roll = rot
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw))
    return finish(actor, label, folder)


def box(label, center, size, material, folder="Blockout", shape=CUBE, yaw=0):
    """Box by center and size in cm (engine shapes are 100 cm)."""
    actor = spawn(unreal.StaticMeshActor, center, (0, yaw, 0), label, folder)
    comp = actor.static_mesh_component
    comp.set_static_mesh(mesh(shape))
    comp.set_material(0, material)
    actor.set_actor_scale3d(unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    return actor


def building(label, x, y, sx, sy, h, material, folder="Bangunan"):
    return box(label, (x, y, h / 2), (sx, sy, h), material, folder)


def room(label, x, y, sx, sy, h, door, material, floor_mat, door_w=260, folder="Bangunan"):
    """Roofless walled room (top-down camera looks inside). door: 'N', 'S', 'E' or 'W'."""
    t = 25
    box(f"{label}_Lantai", (x, y, 3), (sx, sy, 6), floor_mat, folder)
    walls = {
        "N": ((x + sx / 2, y), (t, sy), "Y"),
        "S": ((x - sx / 2, y), (t, sy), "Y"),
        "E": ((x, y + sy / 2), (sx, t), "X"),
        "W": ((x, y - sy / 2), (sx, t), "X"),
    }
    for side, ((cx, cy), (wx, wy), axis) in walls.items():
        if side != door:
            box(f"{label}_Dinding{side}", (cx, cy, h / 2), (wx, wy, h), material, folder)
            continue
        length = wy if axis == "Y" else wx
        piece = (length - door_w) / 2
        for i, sign in enumerate((-1, 1)):
            offset = sign * (door_w / 2 + piece / 2)
            if axis == "Y":
                box(f"{label}_Dinding{side}{i}", (cx, cy + offset, h / 2), (t, piece, h), material, folder)
            else:
                box(f"{label}_Dinding{side}{i}", (cx + offset, cy, h / 2), (piece, t, h), material, folder)


def room_volume(label, x, y, sx, sy, minigame=""):
    vol = spawn(unreal.GRTopDownRoomVolume, (x, y, 150), label=f"Room_{label}", folder="Ruangan")
    vol.trigger.set_box_extent(unreal.Vector(sx / 2 - 40, sy / 2 - 40, 200))
    if minigame:
        vol.set_editor_property("minigame_id", unreal.GRScriptLibrary.request_tag(minigame))
    return vol


def marker(label, loc, glyph, rgb, text):
    m = spawn(unreal.GRMapMarker, loc, label=f"Marker_{label}", folder="Peta")
    comp = m.marker
    comp.set_editor_property("label", unreal.Text(text))
    comp.set_editor_property("glyph", glyph)
    comp.set_editor_property("color", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    return m


def district(ident, name, subtitle, x0, x1, y0, y1, rgb):
    d = spawn(unreal.GRDistrictVolume, ((x0 + x1) / 2, (y0 + y1) / 2, 2000), label=f"Kawasan_{ident}", folder="Kawasan")
    d.area.set_box_extent(unreal.Vector((x1 - x0) / 2, (y1 - y0) / 2, 4000))
    d.set_editor_property("district_id", ident)
    d.set_editor_property("display_name", unreal.Text(name))
    d.set_editor_property("subtitle", unreal.Text(subtitle))
    d.set_editor_property("map_color", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    return d


def interactable(cls, label, loc, scale, prompt, folder="Interaksi", **props):
    a = spawn(cls, loc, label=label, folder=folder)
    a.mesh.set_static_mesh(mesh(CHAMFER))
    a.set_actor_scale3d(unreal.Vector(*scale))
    a.set_editor_property("prompt", unreal.Text(prompt))
    for key, value in props.items():
        if key in ("action_tag", "story_flag_to_add"):
            value = unreal.GRScriptLibrary.request_tag(value)
        a.set_editor_property(key, value)
    return a


def text_label(text, loc, yaw=180, size=120):
    a = spawn(unreal.TextRenderActor, loc, (0, yaw, 0), f"Label_{text.replace(' ', '_')}", "Label")
    c = a.text_render
    c.set_text(text)
    c.set_world_size(size)
    c.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    c.set_text_render_color(unreal.Color(255, 230, 170, 255))
    return a


def lamp(i, x, y):
    box(f"Tiang_{i}", (x, y, 250), (20, 20, 500), M["tiang"], "Lampu", CYLINDER)
    light = spawn(unreal.PointLight, (x, y, 480), label=f"LampuJalan_{i}", folder="Lampu")
    comp = light.point_light_component
    comp.set_intensity(4000.0)
    comp.set_attenuation_radius(1800.0)
    comp.set_light_color(unreal.LinearColor(1.0, 0.7, 0.4, 1.0))
    comp.set_cast_shadows(False)
    light.tags = [unreal.Name("LampuJalan")]


def tree(i, x, y):
    box(f"Pohon_Batang_{i}", (x, y, 200), (40, 40, 400), M["batang"], "Pohon", CYLINDER)
    s = rng.uniform(350, 520)
    box(f"Pohon_Daun_{i}", (x, y, 400 + s / 2), (s, s, s), M["daun"], "Pohon", SPHERE)


M = {}


def build():
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        unreal.log_warning("L_KotaSentosa sudah ada, script dihentikan tanpa perubahan.")
        return False
    if not levels.new_level(MAP_PATH, True):
        unreal.log_error("Gagal membuat level " + MAP_PATH)
        return False

    M.update({
        "tanah": mat("Tanah", (0.32, 0.30, 0.22)),
        "jalan": mat("Jalan", (0.07, 0.07, 0.08)),
        "trotoar": mat("Trotoar", (0.45, 0.43, 0.40)),
        "rumah1": mat("Rumah1", (0.78, 0.70, 0.55)),
        "rumah2": mat("Rumah2", (0.62, 0.72, 0.68)),
        "rumah3": mat("Rumah3", (0.80, 0.62, 0.55)),
        "rumah4": mat("Rumah4", (0.70, 0.70, 0.74)),
        "bayu": mat("RumahBayu", (0.25, 0.65, 0.62)),
        "lantai": mat("Lantai", (0.35, 0.30, 0.26)),
        "ruko1": mat("Ruko1", (0.85, 0.82, 0.70)),
        "ruko2": mat("Ruko2", (0.60, 0.55, 0.62)),
        "pasar": mat("Pasar", (0.82, 0.55, 0.25)),
        "lapak": mat("Lapak", (0.70, 0.30, 0.25)),
        "warnet": mat("Warnet", (0.30, 0.45, 0.80)),
        "sekolah": mat("Sekolah", (0.85, 0.30, 0.30)),
        "lapangan": mat("Lapangan", (0.30, 0.50, 0.25)),
        "dapur": mat("DapurPPR", (0.90, 0.85, 0.80)),
        "puskesmas": mat("Puskesmas", (0.95, 0.95, 0.95)),
        "kelurahan": mat("Kelurahan", (0.85, 0.75, 0.40)),
        "spn": mat("SPN", (0.15, 0.20, 0.40)),
        "dewan": mat("Dewan", (0.95, 0.92, 0.85)),
        "kubah": mat("Kubah", (0.55, 0.75, 0.55)),
        "musala": mat("Musala", (0.90, 0.92, 0.85)),
        "tiang": mat("Tiang", (0.20, 0.20, 0.22)),
        "batang": mat("Batang", (0.35, 0.22, 0.12)),
        "daun": mat("Daun", (0.25, 0.42, 0.20)),
        "pagar": mat("Pagar", (0.30, 0.30, 0.32)),
    })
    houses = [M["rumah1"], M["rumah2"], M["rumah3"], M["rumah4"]]

    # Sky, sun, moon, smoke.
    sun = spawn(unreal.DirectionalLight, (0, 0, 2000), (-50, -60, 0), "Matahari", "Cahaya")
    sun.light_component.set_editor_property("atmosphere_sun_light", True)
    sun.tags = [unreal.Name("Matahari")]
    moon = spawn(unreal.DirectionalLight, (0, 0, 2100), (-45, 120, 0), "Bulan", "Cahaya")
    moon.light_component.set_intensity(0.0)
    moon.light_component.set_cast_shadows(False)
    moon.tags = [unreal.Name("Bulan")]
    spawn(unreal.SkyAtmosphere, (0, 0, 0), label="SkyAtmosphere", folder="Cahaya")
    sky = spawn(unreal.SkyLight, (0, 0, 1500), label="SkyLight", folder="Cahaya")
    sky.light_component.set_editor_property("real_time_capture", True)
    fog = spawn(unreal.ExponentialHeightFog, (0, 0, 0), label="Asap_HeightFog", folder="Cahaya")
    fog.component.set_volumetric_fog(True)
    spawn(unreal.GRDayNightController, (0, 0, 2200), label="DayNight", folder="Cahaya")
    bounds = spawn(unreal.GRMapBounds, (0, 0, 0), label="MapBounds", folder="Peta")
    bounds.area.set_box_extent(unreal.Vector(50000, 50000, 5000))

    # Ground and roads.
    box("Tanah", (0, 0, -5), (120000, 120000, 10), M["tanah"], "Jalan", PLANE)
    box("Jl_Raya", (0, 0, 2), (1600, 100000, 4), M["jalan"], "Jalan")
    box("Jl_Merdeka", (0, 0, 2), (100000, 1600, 4), M["jalan"], "Jalan")
    for i, x in enumerate((-27000, 27000)):
        box(f"Jl_Lingkar_X{i}", (x, 0, 1), (800, 100000, 2), M["jalan"], "Jalan")
    for i, y in enumerate((-27000, 27000)):
        box(f"Jl_Lingkar_Y{i}", (0, y, 1), (100000, 800, 2), M["jalan"], "Jalan")
    for i, x in enumerate((-1000, 1000)):
        box(f"Trotoar_Raya_{i}", (x, 0, 6), (400, 100000, 12), M["trotoar"], "Jalan")
    for i, y in enumerate((-1000, 1000)):
        box(f"Trotoar_Merdeka_{i}", (0, y, 6), (100000, 400, 12), M["trotoar"], "Jalan")

    def free(x, y, margin=0):
        """True when (x, y) is not on a road, the roundabout or a reserved plot."""
        for rx in (0, -27000, 27000):
            if abs(x - rx) < (1300 if rx == 0 else 700) + margin:
                return False
        for ry in (0, -27000, 27000):
            if abs(y - ry) < (1300 if ry == 0 else 700) + margin:
                return False
        for (px0, px1, py0, py1) in reserved:
            if px0 - margin < x < px1 + margin and py0 - margin < y < py1 + margin:
                return False
        return True

    reserved = [
        (-9900, -6800, -10000, -7600),     # rumah Bayu + halaman
        (-3000, -2000, 5000, 7400),        # warnet (on the ruko strip)
        (-18000, -6000, 8000, 18000),      # pasar
        (6000, 24000, 6000, 26000),        # sekolah, dapur, puskesmas
        (5000, 12000, -14000, -3000),      # kelurahan, pos SPN
        (36000, 50000, -9000, 9000),       # gedung dewan
        (-3500, 3500, -3500, 3500),        # tugu
        (-14500, -10500, -22500, -18500),  # musala
    ]

    # Tugu Gemah Ripah at the crossing (a roundabout).
    box("Tugu_Bundaran", (0, 0, 40), (2400, 2400, 80), M["trotoar"], "Landmark", CYLINDER)
    box("Tugu_Tiang", (0, 0, 1300), (300, 300, 2500), M["dewan"], "Landmark")
    box("Tugu_Puncak", (0, 0, 2700), (500, 500, 500), M["kelurahan"], "Landmark", SPHERE)
    text_label("TUGU GEMAH RIPAH", (-1300, 0, 300), 180, 90)

    # Ruko along Jl. Raya (both sides).
    n = 0
    for side in (-1, 1):
        y = -48000
        while y < 48000:
            if free(side * 1900, y, 300):
                h = rng.choice((700, 900, 1100))
                building(f"Ruko_{n}", side * 1900, y, 1200, 2300, h, M["ruko1"] if n % 2 == 0 else M["ruko2"], "Ruko")
                n += 1
            y += 2500

    # Kampung Gang (south-west): dense houses near rumah Bayu, sparser further out.
    n = 0
    for x in range(-24000, -2500, 1500):
        for y in range(-25000, -2500, 1500):
            if free(x, y, 500):
                building(f"Rumah_{n}", x + rng.uniform(-150, 150), y + rng.uniform(-150, 150), 800, 900, rng.choice((350, 400, 450, 600)), rng.choice(houses), "Kampung")
                n += 1
    for x in range(-48000, -26000, 4000):
        for y in range(-48000, -2500, 4000):
            if free(x, y, 800) and rng.random() < 0.7:
                building(f"Rumah_{n}", x, y, 900, 1000, rng.choice((350, 450)), rng.choice(houses), "Kampung")
                n += 1
    for x in range(-24000, -2500, 4000):
        for y in range(-48000, -26000, 4000):
            if free(x, y, 800) and rng.random() < 0.7:
                building(f"Rumah_{n}", x, y, 900, 1000, rng.choice((350, 450)), rng.choice(houses), "Kampung")
                n += 1

    # Rumah Bayu: walled room, door to the north (towards Jl. Raya).
    room("RumahBayu", -9000, -9000, 1000, 1200, 320, "N", M["bayu"], M["lantai"])
    room_volume("RumahBayu", -9000, -9000, 1000, 1200)
    interactable(unreal.GRBed, "Kasur_Bayu", (-9300, -9400, 40), (2.0, 1.0, 0.4), "Tidur")
    # Furniture (dining table, kitchen, Dinda's desk) comes from Content/Data/furniture.json.
    text_label("RUMAH BAYU", (-8400, -9000, 380), 180, 70)
    spawn(unreal.PlayerStart, (-7800, -9000, 120), (0, 0, 0), "PlayerStart", "Pemain")

    # Pos ronda and musala.
    building("PosRonda", -6000, -12500, 300, 300, 260, M["lantai"], "Kampung")
    interactable(unreal.GRInteractableActor, "Int_Warga_PosRonda", (-6000, -12000, 60), (0.5, 0.5, 1.2), "Ngobrol dengan warga", suara_delta=5.0, action_tag="Aksi.Kumpul")
    building("Musala", -12500, -20500, 1200, 1200, 450, M["musala"], "Kampung")
    box("Musala_Kubah", (-12500, -20500, 450), (700, 700, 500), M["kubah"], "Kampung", SPHERE)

    # Pasar Sentosa (south-east): roofed hall with stalls.
    px, py = -12000, 13000
    box("Pasar_Atap", (px, py, 620), (5600, 8200, 40), M["pasar"], "Pasar")
    for ix in range(-2, 3):
        for iy in range(-4, 5, 2):
            box(f"Pasar_Tiang_{ix}_{iy}", (px + ix * 1300, py + iy * 1000, 300), (40, 40, 600), M["tiang"], "Pasar", CYLINDER)
    k = 0
    for ix in range(-2, 2):
        for iy in range(-3, 4):
            box(f"Lapak_{k}", (px + ix * 1300 + 650, py + iy * 1100, 45), (300, 500, 90), M["lapak"], "Pasar")
            k += 1
    text_label("PASAR SENTOSA", (px + 3000, py, 700), 180, 140)
    for x in range(-48000, -2500, 4000):
        for y in range(4000, 48000, 4000):
            if free(x, y, 800) and rng.random() < 0.6:
                building(f"Toko_{x}_{y}", x, y, 1200, 1200, rng.choice((450, 700)), rng.choice((M["ruko1"], M["ruko2"])), "Pasar")

    # Warnet on the south side of Jl. Raya, door facing the road.
    room("Warnet", -2500, 6200, 1000, 1200, 320, "N", M["warnet"], M["lantai"])
    room_volume("Warnet", -2500, 6200, 1000, 1200, "Minigame.Warnet")
    interactable(unreal.GRInteractableActor, "Int_PCWarnet", (-2850, 6200, 60), (0.6, 0.6, 1.0), "Unggah kritik",
                 suara_delta=10.0, diawasi_delta=10.0, single_use=True, action_tag="Aksi.Unggah.Kritik", story_flag_to_add="Flag.Bab1.BuktiDiunggah")
    text_label("WARNET JAYA", (-1900, 6200, 420), 180, 70)

    # Kawasan Sekolah (north-east).
    sx, sy = 12000, 13000
    building("Sekolah_Utara", sx + 2400, sy, 800, 5000, 700, M["sekolah"], "Sekolah")
    building("Sekolah_Barat", sx, sy - 2100, 4000, 800, 700, M["sekolah"], "Sekolah")
    building("Sekolah_Timur", sx, sy + 2100, 4000, 800, 700, M["sekolah"], "Sekolah")
    box("Sekolah_Lapangan", (sx - 200, sy, 3), (3800, 3400, 6), M["lapangan"], "Sekolah")
    box("Sekolah_Pagar", (sx - 2400, sy, 100), (40, 5000, 200), M["pagar"], "Sekolah")
    text_label("SDN SENTOSA 1", (sx - 2600, sy, 400), 180, 100)
    room("DapurPPR", 20000, 20000, 1800, 1400, 350, "S", M["dapur"], M["lantai"])
    room_volume("DapurPPR", 20000, 20000, 1800, 1400, "Minigame.DapurPPR")
    text_label("DAPUR PPR", (19000, 20000, 450), 180, 90)
    building("Puskesmas", 9000, 22000, 1400, 1800, 450, M["puskesmas"], "Sekolah")
    text_label("PUSKESMAS", (8200, 22000, 520), 180, 80)
    for x in range(4000, 48000, 4000):
        for y in range(4000, 48000, 4000):
            if free(x, y, 800) and rng.random() < 0.5:
                building(f"Gedung_NE_{x}_{y}", x, y, 1400, 1400, rng.choice((600, 900, 1200)), rng.choice(houses), "Sekolah")

    # Pusat Pemerintahan (north-west).
    room("Kelurahan", 8500, -9000, 1400, 1800, 380, "S", M["kelurahan"], M["lantai"])
    room_volume("Kelurahan", 8500, -9000, 1400, 1800)
    box("Kelurahan_TiangBendera", (7400, -7700, 400), (20, 20, 800), M["tiang"], "Pemerintahan", CYLINDER)
    text_label("KANTOR KELURAHAN", (7600, -9000, 480), 180, 80)
    building("PosSPN", 6500, -4500, 900, 900, 350, M["spn"], "Pemerintahan")
    text_label("POS SPN", (5900, -4500, 420), 180, 70)
    building("GedungDewan", 44000, 0, 6000, 12000, 3000, M["dewan"], "Pemerintahan")
    box("GedungDewan_Kubah", (44000, 0, 3000), (5000, 5000, 3600), M["kubah"], "Pemerintahan", SPHERE)
    box("GedungDewan_Plaza", (39000, 0, 4), (4000, 9000, 8), M["trotoar"], "Pemerintahan")
    box("GedungDewan_Pagar", (37000, 0, 150), (60, 16000, 300), M["pagar"], "Pemerintahan")
    text_label("GEDUNG DEWAN RAKYAT", (36800, 0, 600), 180, 220)
    for x in range(4000, 36000, 4000):
        for y in range(-48000, -2500, 4000):
            if free(x, y, 800) and rng.random() < 0.5:
                building(f"Gedung_NW_{x}_{y}", x, y, 1400, 1400, rng.choice((600, 900, 1500)), rng.choice((M["ruko1"], M["rumah4"])), "Pemerintahan")

    # Street lamps and trees along the main roads.
    i = 0
    for y in range(-48000, 48001, 4000):
        if abs(y) > 2000:
            lamp(i, 1300, y)
            i += 1
    for x in range(-48000, 36001, 4000):
        if abs(x) > 2000:
            lamp(i, x, -1300)
            i += 1
    t = 0
    for y in range(-46000, 46001, 6000):
        if abs(y) > 3000:
            tree(t, -1300, y + 2000)
            t += 1
    for x in range(-46000, 34001, 6000):
        if abs(x) > 3000:
            tree(t, x + 2000, 1300)
            t += 1

    # Districts (the ruko strip sits inside the quadrants; the most recently entered wins).
    district("KampungGang", "Kampung Gang", "Tempat Bayu tinggal", -50000, -1300, -50000, -1300, (0.35, 0.8, 0.75))
    district("Pasar", "Pasar Sentosa", "Harga naik, timbangan turun", -50000, -1300, 1300, 50000, (0.95, 0.6, 0.25))
    district("Sekolah", "Kawasan Sekolah", "SDN Sentosa 1 dan Dapur PPR", 1300, 50000, 1300, 50000, (0.95, 0.4, 0.4))
    district("Pemerintahan", "Pusat Pemerintahan", "Melayani dengan sepenuh pungli", 1300, 50000, -50000, -1300, (0.95, 0.85, 0.4))
    district("JalanRaya", "Jalan Raya Sentosa", "Ruko, ojol, dan klakson", -3000, 3000, -50000, 50000, (0.75, 0.75, 0.8))

    # Map markers.
    marker("RumahBayu", (-9000, -9000, 0), "R", (0.3, 0.85, 0.8), "Rumah Bayu")
    marker("PosRonda", (-6000, -12500, 0), "PR", (0.8, 0.7, 0.5), "Pos Ronda")
    marker("Musala", (-12500, -20500, 0), "M", (0.7, 0.9, 0.6), "Musala Al-Ikhlas")
    marker("Pasar", (px, py, 0), "P", (0.95, 0.6, 0.25), "Pasar Sentosa")
    marker("Warnet", (-2500, 6200, 0), "W", (0.45, 0.6, 1.0), "Warnet Jaya")
    marker("Sekolah", (sx, sy, 0), "S", (0.95, 0.4, 0.4), "SDN Sentosa 1")
    marker("DapurPPR", (20000, 20000, 0), "D", (0.95, 0.25, 0.2), "Dapur PPR")
    marker("Puskesmas", (9000, 22000, 0), "+", (0.9, 0.95, 0.95), "Puskesmas")
    marker("Kelurahan", (8500, -9000, 0), "K", (0.95, 0.85, 0.4), "Kantor Kelurahan")
    marker("PosSPN", (6500, -4500, 0), "!", (0.4, 0.55, 1.0), "Pos SPN")
    marker("Dewan", (44000, 0, 0), "GD", (0.95, 0.92, 0.85), "Gedung Dewan Rakyat")
    marker("Tugu", (0, 0, 0), "T", (0.9, 0.9, 0.9), "Tugu Gemah Ripah")

    # Save the level and every external actor package (World Partition: one file per actor).
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log_warning(f"L_KotaSentosa dibuat: {len(actors.get_all_level_actors())} actor.")
    return True


build()
