"""Imports the rigged Tripo vehicles (skeletal mesh + animations) and shows them animated in the props gallery.

Input: Art/_ekspor_ue/Kendaraan_rig/<Code>_<Name>/, written by Art/_skrip_kendaraan (Blender 4.0 headless, see
BACA.txt there):
  SK_<Code>_<Name>.fbx           mesh + skeleton, centimeters, leveled, faces +X, pivot bottom center
  A_<Code>_<Name>_<Anim>.fbx     one animation each: Roda (wheels, one turn per second, loop), Gali (excavator)
  rig.json                       bones, pivots, wheel radius (cm)
Material: the batch material MI_<K> made by import_tripo_props.py (same UVs as the static meshes).

Run headless (editor closed), after import_tripo_props.py:
  UnrealEditor-Cmd.exe GemahRipah.uproject -run=pythonscript -script="<absolute path>/Tools/Python/import_tripo_vehicles.py"
Set GR_REIMPORT=1 to replace what already exists.

Creates /Game/GemahRipah/Props/Kendaraan/Rig/<Code>_<Name>/{SK_*, SKEL_*, A_*} and adds a row of animated vehicles
(folder KendaraanRig, in front of the static vehicle row) to /Game/GemahRipah/Maps/L_GaleriProps.
In game, wheel play rate = speed (cm/s) / (2 pi radius_cm), radius from rig.json.
"""
import json
import os
import unreal

ROOT = r"E:/NewSSDFile/Game Pemerintahan"
ART = ROOT + "/Art/_ekspor_ue/Kendaraan_rig"
SPEC = ROOT + "/Art/_skrip_kendaraan/rig_spec.json"
DST = "/Game/GemahRipah/Props/Kendaraan/Rig"
BATCH = "/Game/GemahRipah/Props/_Batch"
GALLERY = "/Game/GemahRipah/Maps/L_GaleriProps"
LOG = []

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary


def log(*a):
    LOG.append(" ".join(str(x) for x in a))


def load(path):
    return lib.load_asset(path) if lib.does_asset_exist(path) else None


def run_import(src, dest, name, ui):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", src)
    task.set_editor_property("destination_path", dest)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    task.set_editor_property("options", ui)
    tools.import_asset_tasks([task])
    return [str(p).split(".")[0] for p in task.get_editor_property("imported_object_paths")]


def import_mesh(full, dest):
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("import_animations", False)
    ui.set_editor_property("create_physics_asset", False)
    data = ui.get_editor_property("skeletal_mesh_import_data")
    data.set_editor_property("import_morph_targets", False)
    data.set_editor_property("convert_scene", True)
    data.set_editor_property("import_uniform_scale", 1.0)
    run_import(f"{ART}/{full}/SK_{full}.fbx", dest, f"SK_{full}", ui)
    mesh = load(f"{dest}/SK_{full}")
    if not mesh:
        return None
    skel = mesh.get_editor_property("skeleton")
    want = f"{dest}/SKEL_{full}"
    if skel.get_path_name().split(".")[0] != want:
        if lib.does_asset_exist(want):
            lib.delete_asset(want)
        lib.rename_asset(skel.get_path_name().split(".")[0], want)
    return mesh


def import_anim(full, anim, dest, skel):
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", False)
    ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
    ui.set_editor_property("import_animations", True)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("skeleton", skel)
    ad = ui.get_editor_property("anim_sequence_import_data")
    ad.set_editor_property("animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    ad.set_editor_property("import_bone_tracks", True)
    ad.set_editor_property("convert_scene", True)
    ad.set_editor_property("import_uniform_scale", 1.0)
    name = f"A_{full}_{anim}"
    paths = run_import(f"{ART}/{full}/A_{full}_{anim}.fbx", dest, name, ui)
    seq = load(f"{dest}/{name}")
    if not isinstance(seq, unreal.AnimSequence):
        # The legacy importer may name the sequence after the FBX take: rename it.
        made = next((p for p in paths if isinstance(load(p), unreal.AnimSequence)), None)
        if made:
            lib.rename_asset(made, f"{dest}/{name}")
            seq = load(f"{dest}/{name}")
    if seq:
        lib.save_loaded_asset(seq)
    return seq


def set_material(mesh, mi):
    mats = mesh.get_editor_property("materials")
    for i in range(len(mats)):
        m = mats[i]
        m.set_editor_property("material_interface", mi)
        mats[i] = m   # array elements are copies: write back by index
    mesh.modify()
    mesh.set_editor_property("materials", mats)
    lib.save_asset(mesh.get_path_name().split(".")[0], only_if_is_dirty=False)


def main():
    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX False")
    spec = {k: v for k, v in json.load(open(SPEC)).items() if not k.startswith("_")}
    if os.environ.get("GR_REIMPORT") and lib.does_directory_exist(DST):
        lib.delete_directory(DST)   # start clean: skeletons are per vehicle and must match the new export
    made = []
    for full in sorted(spec):
        folder = f"{ART}/{full}"
        if not os.path.exists(f"{folder}/rig.json"):
            log("RIG hilang", full)
            continue
        info = json.load(open(f"{folder}/rig.json"))
        dest = f"{DST}/{full}"
        mesh = load(f"{dest}/SK_{full}")
        if not mesh or os.environ.get("GR_REIMPORT"):
            mesh = import_mesh(full, dest)
        if not mesh:
            log("GAGAL mesh", full)
            continue
        set_material(mesh, load(f'{BATCH}/{spec[full]["batch"]}/MI_{spec[full]["batch"]}'))
        skel = load(f"{dest}/SKEL_{full}")
        anims = {}
        for anim in info["anims"]:
            seq = load(f"{dest}/A_{full}_{anim}")
            if not isinstance(seq, unreal.AnimSequence) or os.environ.get("GR_REIMPORT"):
                seq = import_anim(full, anim, dest, skel)
            if seq:
                anims[anim] = seq
                log("ANIM", full, anim, "len_s", round(seq.get_editor_property("sequence_length"), 2),
                    "frames", seq.get_editor_property("number_of_sampled_frames"))
            else:
                log("GAGAL anim", full, anim)
        b = mesh.get_bounds()
        log("SK", full, "ukuran_cm", [round(b.box_extent.x * 2), round(b.box_extent.y * 2), round(b.box_extent.z * 2)],
            "zmin", round(b.origin.z - b.box_extent.z, 1), "bones", len(info["bones"]) + 2,
            "radius_cm", [x.get("radius_cm") for x in info["bones"] if "radius_cm" in x])
        made.append((full, mesh, anims))
    gallery_row(made)


def gallery_row(made):
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not levels.load_level(GALLERY):
        log("GALERI tidak bisa dibuka")
        return
    for a in actors.get_all_level_actors():
        if str(a.get_folder_path()) == "KendaraanRig":
            actors.destroy_actor(a)
    x = 0.0
    y = -900.0   # the static vehicle row starts at y 0
    for full, mesh, anims in made:
        b = mesh.get_bounds()
        a = actors.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(x + b.box_extent.x, y - b.box_extent.y, 0))
        comp = a.skeletal_mesh_component
        comp.set_skeletal_mesh_asset(mesh)
        anim = anims.get("Gali") or anims.get("Roda")
        if anim:
            comp.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_SINGLE_NODE)
            play = unreal.SingleAnimationPlayData()
            play.set_editor_property("anim_to_play", anim)
            play.set_editor_property("saved_looping", True)
            play.set_editor_property("saved_playing", True)
            comp.set_editor_property("animation_data", play)
        a.set_actor_label(f"Rig_{full}")
        a.set_folder_path("KendaraanRig")
        x += b.box_extent.x * 2 + 150
    levels.save_current_level()
    log("GALERI", GALLERY, "rig", len(made))


try:
    main()
except Exception as e:  # noqa: BLE001
    import traceback
    log("ERROR", e, traceback.format_exc())
for line in LOG:
    unreal.log_warning("GRVEH " + line)
