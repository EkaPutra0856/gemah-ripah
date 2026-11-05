"""Road and ground materials for L_KotaSentosa_Plus, from the Scene_RoadsideConstruction surface textures.

The pack's own instances (M_Urb_Roa_Layer) blend three layers and a puddle by vertex color. The town's road boxes
have no vertex color, so they come out as one black puddle. These materials sample one surface set, projected from
above in world space, so a 1600 cm wide road box tiles evenly instead of stretching one texture over it.

Run headless (editor closed):
  UnrealEditor-Cmd.exe GemahRipah.uproject -run=pythonscript -script="<absolute path>/Tools/Python/build_plus_materials.py"
"""
import unreal

SURF = "/Game/Scene_RoadsideConstruction/Assets/MS/Surfaces"
DST = "/Game/GemahRipah/Materials/Plus"
MASTER = "M_GRP_Permukaan"

# Instance name, surface set, tile size in cm, tint (linear multiply).
INSTANCES = [
    ("MI_GRP_Aspal", "Ground_Asphalt_Rough_03", 400.0, (1.9, 1.9, 1.9)),
    ("MI_GRP_AspalHalus", "Ground_Asphalt_Fresh_01", 400.0, (2.2, 2.2, 2.2)),
    ("MI_GRP_AspalRusak", "Ground_Asphalt_Destroyed_01", 500.0, (1.0, 1.0, 1.0)),
    ("MI_GRP_Trotoar", "Ground_Tiles_Sidewalk_Dirty_01", 200.0, (1.6, 1.6, 1.6)),
    ("MI_GRP_Paving", "Ground_Cobblestone_Tileable_01", 200.0, (1.0, 1.0, 1.0)),
    ("MI_GRP_Kerikil", "Ground_Road_Gravel_01", 300.0, (1.0, 1.0, 1.0)),
]

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def log(*a):
    unreal.log_warning("GRPLUS " + " ".join(str(x) for x in a))


def surface(set_name, kind):
    return lib.load_asset(f"{SURF}/{set_name}/T_{set_name}_{kind}")


def master():
    path = f"{DST}/{MASTER}"
    if lib.does_asset_exist(path):
        mat = lib.load_asset(path)
        mel.delete_all_material_expressions(mat)
    else:
        mat = tools.create_asset(MASTER, DST, unreal.Material, unreal.MaterialFactoryNew())
    pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1600, 0)
    xy = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -1400, 0)
    xy.set_editor_property("r", True)
    xy.set_editor_property("g", True)
    xy.set_editor_property("b", False)
    xy.set_editor_property("a", False)
    mel.connect_material_expressions(pos, "", xy, "")
    tile = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -1400, 200)
    tile.set_editor_property("parameter_name", "Ubin")
    tile.set_editor_property("default_value", 400.0)
    uv = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, -1200, 0)
    mel.connect_material_expressions(xy, "", uv, "A")
    mel.connect_material_expressions(tile, "", uv, "B")

    first = INSTANCES[0][1]
    samples = {}
    T = unreal.MaterialSamplerType
    # The pack's surface textures stream as virtual textures, which need the virtual sampler types.
    for i, (param, kind, plain, virtual) in enumerate((
            ("Warna", "D", T.SAMPLERTYPE_COLOR, T.SAMPLERTYPE_VIRTUAL_COLOR),
            ("Normal", "N", T.SAMPLERTYPE_NORMAL, T.SAMPLERTYPE_VIRTUAL_NORMAL),
            ("ORD", "ORDp", T.SAMPLERTYPE_MASKS, T.SAMPLERTYPE_VIRTUAL_MASKS))):
        tex = surface(first, kind)
        s = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -900, i * 300)
        s.set_editor_property("parameter_name", param)
        s.set_editor_property("texture", tex)
        s.set_editor_property("sampler_type", virtual if tex.get_editor_property("virtual_texture_streaming") else plain)
        mel.connect_material_expressions(uv, "", s, "UVs")
        samples[kind] = s

    tint = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -900, -250)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    color = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -500, 0)
    mel.connect_material_expressions(samples["D"], "RGB", color, "A")
    mel.connect_material_expressions(tint, "", color, "B")
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(samples["N"], "RGB", unreal.MaterialProperty.MP_NORMAL)
    # ORDp packs occlusion in R, roughness in G and displacement in B.
    mel.connect_material_property(samples["ORDp"], "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    mel.connect_material_property(samples["ORDp"], "G", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -500, 600)
    spec.set_editor_property("r", 0.3)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    # Instanced static meshes (dressing) and the runtime road pieces may use it too.
    mel.set_material_usage(mat, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    mel.recompile_material(mat)
    lib.save_loaded_asset(mat)
    log("master", mat.get_path_name())
    return mat


def instance(mat, name, set_name, tile, tint):
    path = f"{DST}/{name}"
    if lib.does_asset_exist(path):
        mi = lib.load_asset(path)
    else:
        mi = tools.create_asset(name, DST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, mat)
    for param, kind in (("Warna", "D"), ("Normal", "N"), ("ORD", "ORDp")):
        tex = surface(set_name, kind)
        if not tex:
            log("GAGAL tekstur", set_name, kind)
            continue
        if not tex.get_editor_property("virtual_texture_streaming"):
            log("BUKAN virtual texture, sampler tidak cocok:", tex.get_path_name())
        mel.set_material_instance_texture_parameter_value(mi, param, tex)
    mel.set_material_instance_scalar_parameter_value(mi, "Ubin", tile)
    mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(tint[0], tint[1], tint[2], 1))
    mel.update_material_instance(mi)
    lib.save_loaded_asset(mi)
    log("instance", path)


def main():
    mat = master()
    for name, set_name, tile, tint in INSTANCES:
        instance(mat, name, set_name, tile, tint)


main()
