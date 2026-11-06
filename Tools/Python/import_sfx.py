"""Imports the sound effects and VFX sprites made by prepare_sfx.py.

Input:  Art/Audio_src/SFX/S_*.wav, Art/VFX_src/T_Vfx_*.png
Output: /Game/GemahRipah/Audio/SFX/S_*          sounds; S_Amb_* and S_Mesin_* loop
        /Game/GemahRipah/VFX/T_Vfx_*            sprites
        /Game/GemahRipah/VFX/M_GRVfx_Sprite     translucent unlit; tint and opacity come per instance (custom data 0-3)
        /Game/GemahRipah/VFX/MI_Vfx_*           one per sprite, used by UGRVfxSubsystem (World/GRVfx.cpp)

Run headless (editor closed):
  UnrealEditor-Cmd.exe GemahRipah.uproject -run=pythonscript -script="<absolute path>/Tools/Python/import_sfx.py"
"""
import glob
import os
import unreal

ROOT = os.path.normpath(os.path.join(unreal.Paths.project_dir(), ".."))
AUDIO_SRC = os.path.join(ROOT, "Art", "Audio_src", "SFX")
VFX_SRC = os.path.join(ROOT, "Art", "VFX_src")
AUDIO = "/Game/GemahRipah/Audio/SFX"
VFX = "/Game/GemahRipah/VFX"
MASTER = VFX + "/M_GRVfx_Sprite"
WHITE = "/Engine/EngineResources/WhiteSquareTexture"

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def log(*a):
    unreal.log_warning("GRSFX " + " ".join(str(x) for x in a))


def import_files(files, dest):
    tasks = []
    for path in files:
        task = unreal.AssetImportTask()
        task.filename = path
        task.destination_path = dest
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)
    tools.import_asset_tasks(tasks)
    return [p for t in tasks for p in t.imported_object_paths]


def sounds():
    for path in import_files(sorted(glob.glob(os.path.join(AUDIO_SRC, "S_*.wav"))), AUDIO):
        wave = unreal.load_asset(path)
        name = wave.get_name()
        wave.set_editor_property("looping", name.startswith(("S_Amb_", "S_Mesin_")))
        lib.save_loaded_asset(wave)
        log(name, round(wave.get_editor_property("duration"), 2))


def sprites():
    for path in import_files(sorted(glob.glob(os.path.join(VFX_SRC, "T_Vfx_*.png"))), VFX):
        tex = unreal.load_asset(path)
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_EFFECTS)
        tex.set_editor_property("virtual_texture_streaming", False)
        lib.save_loaded_asset(tex)
        log(tex.get_name())


def expr(mat, cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)


def master():
    if lib.does_asset_exist(MASTER):
        lib.delete_asset(MASTER)
    mat = tools.create_asset("M_GRVfx_Sprite", VFX, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    mat.set_editor_property("used_with_instanced_static_meshes", True)

    tex = expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, -700, 0)
    tex.set_editor_property("parameter_name", "Tex")
    tex.set_editor_property("texture", unreal.load_asset(WHITE))
    data = []
    for i in range(4):
        d = expr(mat, unreal.MaterialExpressionPerInstanceCustomData, -700, 260 + i * 110)
        d.set_editor_property("data_index", i)
        data.append(d)
    rg = expr(mat, unreal.MaterialExpressionAppendVector, -480, 260)
    mel.connect_material_expressions(data[0], "", rg, "A")
    mel.connect_material_expressions(data[1], "", rg, "B")
    rgb = expr(mat, unreal.MaterialExpressionAppendVector, -340, 300)
    mel.connect_material_expressions(rg, "", rgb, "A")
    mel.connect_material_expressions(data[2], "", rgb, "B")
    color = expr(mat, unreal.MaterialExpressionMultiply, -180, 0)
    mel.connect_material_expressions(tex, "RGB", color, "A")
    mel.connect_material_expressions(rgb, "", color, "B")
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    alpha = expr(mat, unreal.MaterialExpressionMultiply, -340, 120)
    mel.connect_material_expressions(tex, "A", alpha, "A")
    mel.connect_material_expressions(data[3], "", alpha, "B")
    # Soft where a puff meets the ground or a wall, instead of a hard cut line.
    fade = expr(mat, unreal.MaterialExpressionDepthFade, -180, 160)
    fade.set_editor_property("fade_distance_default", 25.0)
    mel.connect_material_expressions(alpha, "", fade, "Opacity")
    mel.connect_material_property(fade, "", unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(mat)
    lib.save_loaded_asset(mat)
    log("MASTER", mat.get_path_name())
    return mat


def instance(name, tex_path, master_mat):
    path = f"{VFX}/{name}"
    if lib.does_asset_exist(path):
        lib.delete_asset(path)
    mi = tools.create_asset(name, VFX, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, master_mat)
    mel.set_material_instance_texture_parameter_value(mi, "Tex", unreal.load_asset(tex_path))
    lib.save_loaded_asset(mi)
    log("MI", name)


def main():
    sounds()
    sprites()
    mat = master()
    instance("MI_Vfx_Asap", f"{VFX}/T_Vfx_Asap", mat)
    instance("MI_Vfx_Kerikil", f"{VFX}/T_Vfx_Kerikil", mat)
    instance("MI_Vfx_Percikan", f"{VFX}/T_Vfx_Percikan", mat)
    instance("MI_Vfx_Kertas", WHITE, mat)


try:
    main()
except Exception as e:  # noqa: BLE001
    import traceback
    log("ERROR", e, traceback.format_exc())
