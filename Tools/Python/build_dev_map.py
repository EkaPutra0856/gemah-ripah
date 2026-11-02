"""Builds /Game/GemahRipah/Maps/L_Dev_Sandbox: a grey-box test map for the core systems.

Run from the editor (Tools > Execute Python Script) or headless via Tools/BuildDevMap.bat.
The map is generated: if it already exists, the script stops and changes nothing.
"""
import unreal

MAP_PATH = "/Game/GemahRipah/Maps/L_Dev_Sandbox"
CUBE = "/Engine/BasicShapes/Cube.Cube"
PLANE = "/Engine/BasicShapes/Plane.Plane"
CHAMFER = "/Game/LevelPrototyping/Meshes/SM_ChamferCube.SM_ChamferCube"
GRID_MAT = "/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray.MI_PrototypeGrid_Gray"
DARK_MAT = "/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark.MI_PrototypeGrid_TopDark"

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def spawn(cls, loc, rot=(0, 0, 0), label=None):
    # rot is (pitch, yaw, roll); unreal.Rotator's positional order is roll, pitch, yaw.
    pitch, yaw, roll = rot
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw))
    if label:
        actor.set_actor_label(label)
    return actor


def box(label, loc, scale, mat=GRID_MAT, mesh=CUBE):
    actor = spawn(unreal.StaticMeshActor, loc, label=label)
    comp = actor.static_mesh_component
    comp.set_static_mesh(unreal.load_asset(mesh))
    comp.set_material(0, unreal.load_asset(mat))
    actor.set_actor_scale3d(unreal.Vector(*scale))
    actor.set_folder_path("Blockout")
    return actor


def label_text(text, loc, rot=(0, 180, 0)):
    actor = spawn(unreal.TextRenderActor, loc, rot, label="Label_" + text.replace(" ", "_"))
    comp = actor.text_render
    comp.set_text(text)
    comp.set_world_size(60.0)
    comp.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    comp.set_text_render_color(unreal.Color(255, 220, 120, 255))
    actor.set_folder_path("Labels")
    return actor


def interactable(label, loc, prompt, uang=0, suara=0.0, diawasi=0.0, action="", flag="", single_use=False):
    actor = spawn(unreal.GRInteractableActor, loc, label=label)
    actor.mesh.set_static_mesh(unreal.load_asset(CHAMFER))
    actor.set_actor_scale3d(unreal.Vector(0.6, 0.6, 1.2))
    actor.set_editor_property("prompt", unreal.Text(prompt))
    actor.set_editor_property("uang_delta", uang)
    actor.set_editor_property("suara_delta", suara)
    actor.set_editor_property("diawasi_delta", diawasi)
    actor.set_editor_property("single_use", single_use)
    if action:
        actor.set_editor_property("action_tag", unreal.GRScriptLibrary.request_tag(action))
    if flag:
        actor.set_editor_property("story_flag_to_add", unreal.GRScriptLibrary.request_tag(flag))
    actor.set_folder_path("Interactables")
    return actor


def build():
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        unreal.log_warning("L_Dev_Sandbox sudah ada, script dihentikan tanpa perubahan.")
        return False

    if not levels.new_level(MAP_PATH):
        unreal.log_error("Gagal membuat level " + MAP_PATH)
        return False

    # Lighting: hazy orange sky from karhutla smoke.
    sun = spawn(unreal.DirectionalLight, (0, 0, 1000), (-35, -40, 0), "Sun")
    sun.light_component.set_intensity(6.0)
    sun.light_component.set_light_color(unreal.LinearColor(1.0, 0.78, 0.55, 1.0))
    sun.light_component.set_editor_property("atmosphere_sun_light", True)
    spawn(unreal.SkyAtmosphere, (0, 0, 0), label="SkyAtmosphere")
    sky = spawn(unreal.SkyLight, (0, 0, 500), label="SkyLight")
    sky.light_component.set_editor_property("real_time_capture", True)
    fog = spawn(unreal.ExponentialHeightFog, (0, 0, 0), label="Asap_HeightFog")
    fog_comp = fog.component
    fog_comp.set_fog_density(0.06)
    fog_comp.set_fog_inscattering_color(unreal.LinearColor(0.75, 0.45, 0.18, 1.0))
    fog_comp.set_volumetric_fog(True)
    for a in (sun, sky, fog):
        a.set_folder_path("Lighting")

    # Ground and a "gang" (alley) of house blocks.
    ground = spawn(unreal.StaticMeshActor, (0, 0, 0), label="Ground")
    ground.static_mesh_component.set_static_mesh(unreal.load_asset(PLANE))
    ground.static_mesh_component.set_material(0, unreal.load_asset(GRID_MAT))
    ground.set_actor_scale3d(unreal.Vector(100, 100, 1))
    ground.set_folder_path("Blockout")

    for i in range(5):
        x = -1500 + i * 700
        box(f"Rumah_Kiri_{i}", (x, -650, 200), (5.5, 4, 4))
        box(f"Rumah_Kanan_{i}", (x, 650, 200), (5.5, 4, 4))
    label_text("Gang Kampung", (-1500, 0, 450))

    spawn(unreal.PlayerStart, (-1800, 0, 120), label="PlayerStart")

    interactable("Int_Order", (-900, -350, 60), "Antar order (+Rp15.000)", uang=15000)
    interactable("Int_Warga", (0, 350, 60), "Ngobrol dengan warga", suara=5.0, action="Aksi.Kumpul")
    label_text("Uang / Kumpul warga", (-450, 0, 300))

    # Top-down room (warnet) at x=+2500: 12 x 12 m, door on the west wall.
    rx, half, wall_h = 2500, 600, 300
    box("Warnet_Lantai", (rx, 0, 2), (12, 12, 0.04), DARK_MAT)
    box("Warnet_Dinding_N", (rx, half, wall_h / 2), (12, 0.2, 3))
    box("Warnet_Dinding_S", (rx, -half, wall_h / 2), (12, 0.2, 3))
    box("Warnet_Dinding_E", (rx + half, 0, wall_h / 2), (0.2, 12, 3))
    box("Warnet_Dinding_W1", (rx - half, -400, wall_h / 2), (0.2, 4, 3))
    box("Warnet_Dinding_W2", (rx - half, 400, wall_h / 2), (0.2, 4, 3))

    room = spawn(unreal.GRTopDownRoomVolume, (rx, 0, 150), label="Room_Warnet")
    room.trigger.set_box_extent(unreal.Vector(half - 20, half - 20, 200))
    room.set_editor_property("minigame_id", unreal.GRScriptLibrary.request_tag("Minigame.Warnet"))
    room.set_folder_path("Rooms")

    interactable("Int_PCWarnet", (rx + 300, 0, 60), "Unggah kritik", suara=10.0, diawasi=10.0,
                 action="Aksi.Unggah.Kritik", flag="Flag.Bab1.BuktiDiunggah", single_use=True)
    label_text("Warnet (top-down)", (rx - half - 50, 0, 380), (0, 180, 0))

    if not levels.save_current_level():
        unreal.log_error("Gagal menyimpan level.")
        return False

    unreal.log("L_Dev_Sandbox selesai dibuat.")
    return True


build()
