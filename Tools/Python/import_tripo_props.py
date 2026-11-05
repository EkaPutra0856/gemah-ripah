"""Imports the Tripo props (vehicles, buildings, outdoor and room props) as static meshes, and builds a gallery map.

Input: Art/_ekspor_ue/Props, written by Art/_skrip_props (Blender 4.0 headless, see BACA.txt there):
  <Kategori>/<Code>_<Name>/SM_<Code>_<Name>.fbx   centimeters, pivot at bottom center, long side along X,
                                                  vehicles face +X
  _Batch/<K>/T_<K>_{basecolor,normal,rm}.*        texture atlas shared by every prop cut from one Tripo sheet
Art/_skrip_props/props_spec.json says which batch (K1..K9) each prop comes from.

Run headless (editor closed):
  UnrealEditor-Cmd.exe GemahRipah.uproject -run=pythonscript -script="<absolute path>/Tools/Python/import_tripo_props.py"
Set GR_REIMPORT=1 to replace meshes that already exist.

Creates:
  /Game/GemahRipah/Props/_Batch/<K>/T_<K>_*, MI_<K>   (MI parent: /Game/GemahRipah/Characters/M_GR_Tripo)
  /Game/GemahRipah/Props/<Kategori>/SM_<Code>_<Name>   Kendaraan, Bangunan, PropertiLuar, PropertiRuangan
  /Game/GemahRipah/Maps/L_GaleriProps                  every prop in rows per category, for checking
"""
import json
import os
import unreal

ROOT = r"E:/NewSSDFile/Game Pemerintahan"
ART = ROOT + "/Art/_ekspor_ue/Props"
SPEC = ROOT + "/Art/_skrip_props/props_spec.json"
DST = "/Game/GemahRipah/Props"
MASTER = "/Game/GemahRipah/Characters/M_GR_Tripo"
GALLERY = "/Game/GemahRipah/Maps/L_GaleriProps"
CATS = ["Kendaraan", "Bangunan", "PropertiLuar", "PropertiRuangan"]
LOG = []

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def log(*a):
    LOG.append(" ".join(str(x) for x in a))


def load(path):
    return lib.load_asset(path) if lib.does_asset_exist(path) else None


def import_file(src, dest, name, options=None):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", src)
    task.set_editor_property("destination_path", dest)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    if options:
        task.set_editor_property("options", options)
    tools.import_asset_tasks([task])
    return list(task.get_editor_property("imported_object_paths"))


def batch_material(k, master):
    dest = f"{DST}/_Batch/{k}"
    mi = load(f"{dest}/MI_{k}")
    if mi and not os.environ.get("GR_REIMPORT"):
        return mi
    folder = f"{ART}/_Batch/{k}"
    tex = {}
    for role in ("basecolor", "normal", "rm"):
        src = next((f"{folder}/{f}" for f in os.listdir(folder) if f.startswith(f"T_{k}_{role}.")), None)
        if not src:
            log("TEX hilang", k, role)
            continue
        import_file(src, dest, f"T_{k}_{role}")
        t = load(f"{dest}/T_{k}_{role}")
        if role == "normal":
            t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            t.set_editor_property("srgb", False)
        elif role == "rm":
            t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
            t.set_editor_property("srgb", False)
            # 2K is plenty for roughness/metal; base color and normal keep the 4K atlas (5-10 props share it).
            t.set_editor_property("max_texture_size", 2048)
        lib.save_loaded_asset(t)
        tex[role] = t
    mi = mi or tools.create_asset(f"MI_{k}", dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, master)
    for role, param in (("basecolor", "Base"), ("normal", "Normal"), ("rm", "RM")):
        if role in tex:
            mel.set_material_instance_texture_parameter_value(mi, param, tex[role])
    lib.save_loaded_asset(mi)
    return mi


def import_mesh(fbx, dest, name, complex_collision):
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", False)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_STATIC_MESH)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("import_animations", False)
    data = ui.get_editor_property("static_mesh_import_data")
    data.set_editor_property("combine_meshes", True)
    data.set_editor_property("auto_generate_collision", not complex_collision)
    data.set_editor_property("generate_lightmap_u_vs", True)
    data.set_editor_property("convert_scene", True)
    data.set_editor_property("import_uniform_scale", 1.0)
    import_file(fbx, dest, name, ui)
    mesh = load(f"{dest}/{name}")
    if mesh and complex_collision:
        # Buildings and big walk-around props: collide with the real shape, not a convex blob.
        body = mesh.get_editor_property("body_setup")
        if body:
            body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    return mesh


def set_material(mesh, mi):
    mats = mesh.get_editor_property("static_materials")
    for i in range(len(mats)):
        m = mats[i]
        m.set_editor_property("material_interface", mi)
        mats[i] = m   # array elements are copies: write back by index
    mesh.modify()
    mesh.set_editor_property("static_materials", mats)
    lib.save_asset(mesh.get_path_name().split(".")[0], only_if_is_dirty=False)


def main():
    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX False")
    master = load(MASTER)
    spec = json.load(open(SPEC))
    made = []
    for k, items in spec.items():
        K = k.upper()
        mi = batch_material(K, master)
        for it in items:
            full = f'{it["code"]}_{it["name"]}'
            fbx = f'{ART}/{it["cat"]}/{full}/SM_{full}.fbx'
            dest = f'{DST}/{it["cat"]}'
            path = f"{dest}/SM_{full}"
            if not os.path.exists(fbx):
                log("FBX hilang", fbx)
                continue
            mesh = load(path)
            if not mesh or os.environ.get("GR_REIMPORT"):
                big = it["cat"] == "Bangunan" or it["code"] in ("P13", "P16", "P21", "P24", "R05", "R12")
                mesh = import_mesh(fbx, dest, f"SM_{full}", big)
            if not mesh:
                log("GAGAL", full)
                continue
            set_material(mesh, mi)
            b = mesh.get_bounds()
            log("PROP", full, "ukuran_cm", [round(b.box_extent.x * 2), round(b.box_extent.y * 2), round(b.box_extent.z * 2)],
                "zmin", round(b.origin.z - b.box_extent.z, 1), "tris", mesh.get_num_triangles(0), "mat", mi.get_name())
            made.append((it["cat"], full, mesh))
    build_gallery(made)


def build_gallery(made):
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if lib.does_asset_exist(GALLERY):
        lib.delete_asset(GALLERY)
    levels.new_level(GALLERY)
    floor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(4000, 4000, -5))
    floor.static_mesh_component.set_static_mesh(load("/Engine/BasicShapes/Plane"))
    floor.set_actor_scale3d(unreal.Vector(120, 120, 1))
    floor.set_actor_label("Lantai")
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 2000), unreal.Rotator(roll=0, pitch=-45, yaw=-30))
    sun.set_actor_label("Matahari")
    actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1500)).set_actor_label("Langit")
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0)).set_actor_label("Atmosfer")
    actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-600, 600, 100)).set_actor_label("Mulai")
    y = 0.0
    for cat in CATS:
        row = [m for m in made if m[0] == cat]
        x = 0.0; depth = 0.0
        for _, full, mesh in row:
            b = mesh.get_bounds()
            a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x + b.box_extent.x, y + b.box_extent.y, 0))
            a.static_mesh_component.set_static_mesh(mesh)
            a.set_actor_label(full)
            a.set_folder_path(cat)
            x += b.box_extent.x * 2 + 150
            depth = max(depth, b.box_extent.y * 2)
        y += depth + 400
    levels.save_current_level()
    log("GALERI", GALLERY, len(made))


try:
    main()
except Exception as e:  # noqa: BLE001
    import traceback
    log("ERROR", e, traceback.format_exc())
for line in LOG:
    unreal.log_warning("GRPROP " + line)
