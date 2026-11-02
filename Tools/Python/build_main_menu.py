"""Builds /Game/GemahRipah/Maps/L_MainMenu: a smoky dusk street seen through one camera.

The main menu (UGRMainMenuScreen) is drawn on top by AGRMenuPlayerController.
Run headless via Tools/BuildMainMenu.bat. If the map already exists, the script stops and changes nothing.
"""
import unreal

MAP_PATH = "/Game/GemahRipah/Maps/L_MainMenu"
CUBE = "/Engine/BasicShapes/Cube.Cube"
PLANE = "/Engine/BasicShapes/Plane.Plane"
CYLINDER = "/Engine/BasicShapes/Cylinder.Cylinder"
GRID_MAT = "/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray.MI_PrototypeGrid_Gray"
DARK_MAT = "/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark.MI_PrototypeGrid_TopDark"

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def spawn(cls, loc, rot=(0, 0, 0), label=None, folder=None):
    # rot is (pitch, yaw, roll); unreal.Rotator's positional order is roll, pitch, yaw.
    pitch, yaw, roll = rot
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*loc), unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw))
    if label:
        actor.set_actor_label(label)
    if folder:
        actor.set_folder_path(folder)
    return actor


def box(label, loc, scale, mat=GRID_MAT, mesh=CUBE, rot=(0, 0, 0)):
    actor = spawn(unreal.StaticMeshActor, loc, rot, label, "Blockout")
    comp = actor.static_mesh_component
    comp.set_static_mesh(unreal.load_asset(mesh))
    comp.set_material(0, unreal.load_asset(mat))
    actor.set_actor_scale3d(unreal.Vector(*scale))
    return actor


def street_lamp(i, x, y):
    box(f"Tiang_{i}", (x, y, 200), (0.12, 0.12, 4), DARK_MAT, CYLINDER)
    light = spawn(unreal.PointLight, (x, y, 390), label=f"Lampu_{i}", folder="Lighting")
    light.point_light_component.set_intensity(8000.0)
    light.point_light_component.set_light_color(unreal.LinearColor(1.0, 0.62, 0.3, 1.0))
    light.point_light_component.set_attenuation_radius(900.0)


def build():
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        unreal.log_warning("L_MainMenu sudah ada, script dihentikan tanpa perubahan.")
        return False
    if not levels.new_level(MAP_PATH):
        unreal.log_error("Gagal membuat level " + MAP_PATH)
        return False

    # Low sun through thick karhutla smoke.
    sun = spawn(unreal.DirectionalLight, (0, 0, 1000), (-12, 150, 0), "Sun", "Lighting")
    sun.light_component.set_intensity(3.0)
    sun.light_component.set_light_color(unreal.LinearColor(1.0, 0.55, 0.28, 1.0))
    sun.light_component.set_editor_property("atmosphere_sun_light", True)
    spawn(unreal.SkyAtmosphere, (0, 0, 0), label="SkyAtmosphere", folder="Lighting")
    sky = spawn(unreal.SkyLight, (0, 0, 500), label="SkyLight", folder="Lighting")
    sky.light_component.set_editor_property("real_time_capture", True)
    sky.light_component.set_intensity(0.6)
    fog = spawn(unreal.ExponentialHeightFog, (0, 0, 0), label="Asap_HeightFog", folder="Lighting")
    fog_comp = fog.component
    fog_comp.set_fog_density(0.045)
    fog_comp.set_fog_inscattering_color(unreal.LinearColor(0.8, 0.42, 0.15, 1.0))
    fog_comp.set_volumetric_fog(True)

    ground = spawn(unreal.StaticMeshActor, (0, 0, 0), label="Ground", folder="Blockout")
    ground.static_mesh_component.set_static_mesh(unreal.load_asset(PLANE))
    ground.static_mesh_component.set_material(0, unreal.load_asset(GRID_MAT))
    ground.set_actor_scale3d(unreal.Vector(120, 120, 1))
    box("Jalan", (0, 0, 1), (60, 7, 0.02), DARK_MAT)

    # Rows of houses of varied height along the street, receding into the smoke.
    heights = [3.5, 5, 4, 7, 4.5, 3.5, 9, 5, 4, 6]
    for i, h in enumerate(heights):
        x = -400 + i * 650
        box(f"Rumah_Kiri_{i}", (x, -800, h * 50), (5.5, 4, h))
        box(f"Rumah_Kanan_{i}", (x, 800, heights[-1 - i] * 50), (5.5, 4, heights[-1 - i]))
    # Billboard of the government, the satirical landmark.
    box("Baliho_Tiang", (900, 520, 300), (0.2, 0.2, 6), DARK_MAT)
    box("Baliho", (900, 520, 650), (0.2, 6, 3), GRID_MAT)
    for i in range(5):
        street_lamp(i, -200 + i * 1100, 420 if i % 2 == 0 else -420)

    # Camera on the left of the street looking down it; the menu sits on the left of the screen.
    cam = spawn(unreal.CameraActor, (-1400, -250, 260), (-4, 8, 0), "MenuCamera", "Camera")
    cam.camera_component.set_editor_property("field_of_view", 70.0)

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", unreal.GRMenuGameMode.static_class())

    if not levels.save_current_level():
        unreal.log_error("Gagal menyimpan " + MAP_PATH)
        return False
    unreal.log("L_MainMenu dibuat.")
    return True


build()
