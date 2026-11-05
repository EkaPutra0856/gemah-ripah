"""Ground and wall materials for L_KotaSentosa_Plus, from the surface textures of the Epic packs.

The packs' own instances blend several layers and a puddle by vertex color. The town's road and building boxes have no
vertex color, so they come out as one black puddle. These materials sample one surface set in world space instead:
  M_GRP_Permukaan  projected from above, for roads, pavement and plazas;
  M_GRP_Dinding    projected on all three axes (triplanar), for building walls.
So a 1600 cm wide road or a 12 m long ruko tiles evenly instead of stretching one texture over it.

Names keep the blockout prefixes on purpose: AGRDressing skips ground whose material starts with MI_Jalan, and
AGRFacades decorates boxes whose material starts with MI_Rumah or MI_Ruko. AGRPlusDressing swaps each blockout
material MI_X for MI_X_Plus at runtime (see Source/GemahRipah/World/GRPlusDressing.cpp).

The surface textures stream as virtual textures, so the samplers use the virtual sampler types.

Run headless (editor closed):
  UnrealEditor-Cmd.exe GemahRipah.uproject -run=pythonscript -script="<absolute path>/Tools/Python/build_plus_materials.py"
"""
import unreal

ROAD = "/Game/Scene_RoadsideConstruction/Assets/MS/Surfaces"
TRASH = "/Game/Scene_Trash/Assets/MS/Surfaces"
UNF = "/Game/Scene_UnfinishedBuilding/Assets/Megascans/Surfaces"
DST = "/Game/GemahRipah/Materials/Plus"

# Ground: instance name, surface folder, set, tile size in cm, tint (linear multiply).
GROUND = [
    ("MI_Jalan_Plus", ROAD, "Ground_Asphalt_Rough_03", 400.0, (1.9, 1.9, 1.9)),
    ("MI_Jalan_PlusRusak", ROAD, "Ground_Asphalt_Destroyed_01", 500.0, (1.0, 1.0, 1.0)),
    ("MI_Jalan_PlusHalus", ROAD, "Ground_Asphalt_Fresh_01", 400.0, (2.2, 2.2, 2.2)),
    ("MI_Trotoar_Plus", ROAD, "Ground_Tiles_Sidewalk_Dirty_01", 200.0, (1.6, 1.6, 1.6)),
    ("MI_Paving_Plus", ROAD, "Ground_Cobblestone_Tileable_01", 200.0, (1.1, 1.1, 1.1)),
    ("MI_Kerikil_Plus", ROAD, "Ground_Road_Gravel_01", 300.0, (1.0, 1.0, 1.0)),
    ("MI_Lantai_PlusBeton", UNF, "Floor_Concrete_Dusty_01", 300.0, (1.0, 1.0, 1.0)),
]

PAINT = (TRASH, "Wall_Painted_Worn_01")
BRICK = (TRASH, "Wall_Brick_Rough_01")
CONCRETE = (UNF, "Wall_Concrete_Rough_01")
STAINED = (UNF, "Wall_Concrete_Stained_02")

# Walls: blockout material name, surface, tile size in cm, tint. The tints are the blockout colors brightened,
# because the worn paint texture is a dim off-white.
WALLS = [
    ("MI_Rumah1_Plus", PAINT, 300.0, (1.25, 1.12, 0.88)),
    ("MI_Rumah2_Plus", PAINT, 300.0, (0.95, 1.15, 1.08)),
    ("MI_Rumah3_Plus", PAINT, 300.0, (1.3, 1.0, 0.88)),
    ("MI_Rumah4_Plus", PAINT, 300.0, (1.12, 1.12, 1.18)),
    # Unplastered houses: bare concrete and bare brick (picked by AGRPlusDressing for part of the kampung).
    ("MI_Rumah5_Plus", CONCRETE, 300.0, (1.0, 1.0, 1.0)),
    ("MI_Rumah6_Plus", BRICK, 250.0, (1.0, 1.0, 1.0)),
    ("MI_RumahBayu_Plus", PAINT, 300.0, (0.55, 1.15, 1.1)),
    ("MI_Ruko1_Plus", PAINT, 300.0, (1.35, 1.3, 1.1)),
    ("MI_Ruko2_Plus", PAINT, 300.0, (1.0, 0.9, 1.0)),
    ("MI_Ruko3_Plus", BRICK, 250.0, (1.0, 1.0, 1.0)),
    ("MI_Sekolah_Plus", PAINT, 300.0, (1.35, 0.55, 0.5)),
    ("MI_Puskesmas_Plus", PAINT, 300.0, (1.45, 1.45, 1.45)),
    ("MI_Kelurahan_Plus", PAINT, 300.0, (1.35, 1.2, 0.65)),
    ("MI_SPN_Plus", STAINED, 300.0, (0.55, 0.65, 0.9)),
    ("MI_Dewan_Plus", PAINT, 400.0, (1.5, 1.45, 1.35)),
    ("MI_Musala_Plus", PAINT, 300.0, (1.4, 1.45, 1.3)),
    ("MI_Warnet_Plus", PAINT, 300.0, (0.6, 0.8, 1.35)),
    ("MI_DapurPPR_Plus", PAINT, 300.0, (1.4, 1.35, 1.25)),
    ("MI_Pagar_Plus", STAINED, 250.0, (1.0, 1.0, 1.0)),
]

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
T = unreal.MaterialSamplerType


def log(*a):
    unreal.log_warning("GRPLUS " + " ".join(str(x) for x in a))


def surface(folder, set_name, kind):
    return lib.load_asset(f"{folder}/{set_name}/T_{set_name}_{kind}")


def new_material(name):
    path = f"{DST}/{name}"
    if lib.does_asset_exist(path):
        mat = lib.load_asset(path)
        mel.delete_all_material_expressions(mat)
        return mat
    return tools.create_asset(name, DST, unreal.Material, unreal.MaterialFactoryNew())


def param_sample(mat, param, tex, kind, x, y, uv):
    plain, virtual = {
        "D": (T.SAMPLERTYPE_COLOR, T.SAMPLERTYPE_VIRTUAL_COLOR),
        "N": (T.SAMPLERTYPE_NORMAL, T.SAMPLERTYPE_VIRTUAL_NORMAL),
        "ORDp": (T.SAMPLERTYPE_MASKS, T.SAMPLERTYPE_VIRTUAL_MASKS),
    }[kind]
    s = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    s.set_editor_property("parameter_name", param)
    s.set_editor_property("texture", tex)
    s.set_editor_property("sampler_type", virtual if tex.get_editor_property("virtual_texture_streaming") else plain)
    mel.connect_material_expressions(uv, "", s, "UVs")
    return s


def mask(mat, src, channels, x, y, out=""):
    node = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, x, y)
    for c in "rgba":
        node.set_editor_property(c, c in channels)
    mel.connect_material_expressions(src, out, node, "")
    return node


def binary(mat, cls, a, b, x, y, a_out="", b_out=""):
    node = mel.create_material_expression(mat, cls, x, y)
    mel.connect_material_expressions(a, a_out, node, "A")
    mel.connect_material_expressions(b, b_out, node, "B")
    return node


def scalar(mat, name, value, x, y):
    p = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", value)
    return p


def tint_param(mat, x, y):
    p = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, x, y)
    p.set_editor_property("parameter_name", "Tint")
    p.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
    return p


def finish(mat):
    spec = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 900)
    spec.set_editor_property("r", 0.3)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    mel.set_material_usage(mat, unreal.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
    mel.recompile_material(mat)
    lib.save_loaded_asset(mat)
    log("master", mat.get_path_name())
    return mat


def ground_master(folder, set_name):
    mat = new_material("M_GRP_Permukaan")
    pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1600, 0)
    uv = binary(mat, unreal.MaterialExpressionDivide, mask(mat, pos, "rg", -1400, 0), scalar(mat, "Ubin", 400.0, -1400, 200), -1200, 0)
    d = param_sample(mat, "Warna", surface(folder, set_name, "D"), "D", -900, 0, uv)
    n = param_sample(mat, "Normal", surface(folder, set_name, "N"), "N", -900, 300, uv)
    o = param_sample(mat, "ORD", surface(folder, set_name, "ORDp"), "ORDp", -900, 600, uv)
    color = binary(mat, unreal.MaterialExpressionMultiply, d, tint_param(mat, -900, -250), -500, 0, "RGB", "")
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(n, "RGB", unreal.MaterialProperty.MP_NORMAL)
    # ORDp packs occlusion in R, roughness in G and displacement in B.
    mel.connect_material_property(o, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    mel.connect_material_property(o, "G", unreal.MaterialProperty.MP_ROUGHNESS)
    return finish(mat)


def wall_master(folder, set_name):
    """Triplanar: each axis plane samples the texture, blended by how much the surface faces that axis."""
    mat = new_material("M_GRP_Dinding")
    pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -2200, 0)
    nrm = mel.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -2200, 900)
    tile = scalar(mat, "Ubin", 300.0, -2200, 300)
    dtex = surface(folder, set_name, "D")
    otex = surface(folder, set_name, "ORDp")
    planes = []
    for i, ch in enumerate(("gb", "rb", "rg")):
        uv = binary(mat, unreal.MaterialExpressionDivide, mask(mat, pos, ch, -1900, i * 250), tile, -1700, i * 250)
        planes.append((param_sample(mat, "Warna", dtex, "D", -1400, i * 250, uv),
                       param_sample(mat, "ORD", otex, "ORDp", -1400, 900 + i * 250, uv)))
    a = mel.create_material_expression(mat, unreal.MaterialExpressionAbs, -1900, 900)
    mel.connect_material_expressions(nrm, "", a, "")
    p = mel.create_material_expression(mat, unreal.MaterialExpressionPower, -1700, 900)
    p.set_editor_property("const_exponent", 4.0)
    mel.connect_material_expressions(a, "", p, "Base")
    ones = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -1700, 1100)
    ones.set_editor_property("constant", unreal.LinearColor(1, 1, 1, 1))
    w = binary(mat, unreal.MaterialExpressionDivide, p, binary(mat, unreal.MaterialExpressionDotProduct, p, ones, -1500, 1100), -1300, 1000)
    color = rough = ao = None
    for i, ch in enumerate("rgb"):
        wi = mask(mat, w, ch, -1100, 700 + i * 150)
        c = binary(mat, unreal.MaterialExpressionMultiply, planes[i][0], wi, -900, i * 250, "RGB", "")
        r = binary(mat, unreal.MaterialExpressionMultiply, mask(mat, planes[i][1], "g", -1100, 1300 + i * 150), wi, -900, 900 + i * 250)
        o = binary(mat, unreal.MaterialExpressionMultiply, mask(mat, planes[i][1], "r", -1100, 1800 + i * 150), wi, -900, 1600 + i * 250)
        color = c if color is None else binary(mat, unreal.MaterialExpressionAdd, color, c, -700, i * 250)
        rough = r if rough is None else binary(mat, unreal.MaterialExpressionAdd, rough, r, -700, 900 + i * 250)
        ao = o if ao is None else binary(mat, unreal.MaterialExpressionAdd, ao, o, -700, 1600 + i * 250)
    color = binary(mat, unreal.MaterialExpressionMultiply, color, tint_param(mat, -700, -250), -500, 0)
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(ao, "", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    return finish(mat)


def instance(parent, name, folder, set_name, tile, tint, params):
    path = f"{DST}/{name}"
    if lib.does_asset_exist(path):
        mi = lib.load_asset(path)
    else:
        mi = tools.create_asset(name, DST, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent)
    for param, kind in params:
        tex = surface(folder, set_name, kind)
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
    ground = ground_master(GROUND[0][1], GROUND[0][2])
    for name, folder, set_name, tile, tint in GROUND:
        instance(ground, name, folder, set_name, tile, tint, (("Warna", "D"), ("Normal", "N"), ("ORD", "ORDp")))
    wall = wall_master(*PAINT)
    for name, (folder, set_name), tile, tint in WALLS:
        instance(wall, name, folder, set_name, tile, tint, (("Warna", "D"), ("ORD", "ORDp")))
    # The first try's names, before the blockout prefixes were kept.
    for old in ("MI_GRP_Aspal", "MI_GRP_AspalHalus", "MI_GRP_AspalRusak", "MI_GRP_Trotoar", "MI_GRP_Paving", "MI_GRP_Kerikil"):
        if lib.does_asset_exist(f"{DST}/{old}"):
            lib.delete_asset(f"{DST}/{old}")
            log("hapus", old)


main()
