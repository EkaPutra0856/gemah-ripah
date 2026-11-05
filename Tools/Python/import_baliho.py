"""Imports the billboard, banner and poster pictures (Gemini) as textures with one material instance each.

Input: "Aset Gemini Stylized/13 Baliho dan Poster/<Code>.jpg"
  BL01..BL09  billboards, landscape (on SM_P07_Baliho, both faces)
  SP01..SP06  painted cloth banners (on SM_P08_Spanduk_demo, both faces)
  PS01..PS04  paper posters, portrait (on house and building side walls)

Run headless (editor closed):
  UnrealEditor-Cmd.exe GemahRipah.uproject -run=pythonscript -script="<absolute path>/Tools/Python/import_baliho.py"

Creates:
  /Game/GemahRipah/Props/Baliho/M_GR_Poster          two-sided, texture parameter "Tex", a little self-lit
  /Game/GemahRipah/Props/Baliho/T_<Code>, MI_Poster_<Code>
World/GRPosters.cpp loads the MI_Poster_<Code> assets by name.
"""
import os
import unreal

SRC = r"E:/NewSSDFile/Game Pemerintahan/Aset Gemini Stylized/13 Baliho dan Poster"
DST = "/Game/GemahRipah/Props/Baliho"
LOG = []

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def log(*a):
    LOG.append(" ".join(str(x) for x in a))


def master():
    path = f"{DST}/M_GR_Poster"
    if lib.does_asset_exist(path):
        return lib.load_asset(path)
    mat = tools.create_asset("M_GR_Poster", DST, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("two_sided", True)
    tex = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -600, 0)
    tex.set_editor_property("parameter_name", "Tex")
    # Every texture parameter needs a default, or the material fails to compile.
    tex.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/DefaultTexture"))
    glow = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, 200)
    glow.set_editor_property("const_b", 0.12)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -250, 350)
    rough.set_editor_property("r", 0.75)
    mel.connect_material_expressions(tex, "RGB", glow, "A")
    mel.connect_material_property(tex, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    # Printed vinyl in the shade of a building still reads from across the road.
    mel.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    lib.save_loaded_asset(mat)
    return mat


def import_texture(src, name):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", src)
    task.set_editor_property("destination_path", DST)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    tools.import_asset_tasks([task])
    tex = lib.load_asset(f"{DST}/{name}")
    # 2K keeps the text sharp from the road; the sources are 2.7K.
    tex.set_editor_property("max_texture_size", 2048)
    tex.set_editor_property("srgb", True)
    lib.save_loaded_asset(tex)
    return tex


def main():
    mat = master()
    for f in sorted(os.listdir(SRC)):
        code, ext = os.path.splitext(f)
        if ext.lower() not in (".jpg", ".png") or len(code) != 4:
            continue
        tex = import_texture(f"{SRC}/{f}", f"T_{code}")
        mi_path = f"{DST}/MI_Poster_{code}"
        mi = lib.load_asset(mi_path) if lib.does_asset_exist(mi_path) else tools.create_asset(
            f"MI_Poster_{code}", DST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, mat)
        mel.set_material_instance_texture_parameter_value(mi, "Tex", tex)
        lib.save_loaded_asset(mi)
        log("OK", code, tex.blueprint_get_size_x(), tex.blueprint_get_size_y())


try:
    main()
finally:
    with open(os.path.join(unreal.Paths.project_saved_dir(), "import_baliho.log"), "w", encoding="utf-8") as out:
        out.write("\n".join(LOG) + "\n")
