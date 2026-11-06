"""Builds the landscape of L_KotaSentosa: hills outside Jl. Lingkar, the river and the kampung kali, painted layers.

The shape lives in Source/GemahRipah/World/GRTerrain (C++), shared with the runtime bridges and water
(World/GRTerrainDetails). This script makes the assets and calls UGRScriptLibrary::BuildTerrain, which:
  - removes the old ground plane "Tanah" and any previous landscape and foundations,
  - removes the generic buildings standing in the river, the kali, the field or the project site,
  - puts the buildings on the hills on stone foundations (actors "Pondasi_*", tag GRPondasi),
  - imports a 2017 x 2017 heightmap (60 cm) with six weightmap layers.
It can be run again: it starts over from the building footprints.

Textures: Poly Haven CC0, "Aset Gemini Stylized/14 Tekstur Tanah" (see SUMBER.txt there).

Run headless (editor closed):
  UnrealEditor-Cmd.exe GemahRipah.uproject -run=pythonscript -script="<absolute path>/Tools/Python/build_terrain.py"
Log: Saved/build_terrain.log
Design: landscaping design notes (not included)
"""
import glob
import json
import os
import re
import unreal

SRC = r"E:/NewSSDFile/Game Pemerintahan/Aset Gemini Stylized/14 Tekstur Tanah"
DST = "/Game/GemahRipah/Landscape"
TEX = DST + "/Textures"
# Both cities share the shape; the Plus copy is dressed with the Epic/Fab packs at runtime (AGRPlusDressing).
MAPS = ["/Game/GemahRipah/Maps/L_KotaSentosa", "/Game/GemahRipah/Maps/L_KotaSentosa_Plus"]
LOG = []

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary

# Layer name (as in GRTerrain::LayerName), Poly Haven set, tile size in cm, color tint (linear multiply).
# Both grass sets are dry (straw and dead leaves); the tint turns them into the green of a wet-season Indonesian town.
LAYERS = [
    ("Rumput", "leafy_grass", 400.0, (0.31, 0.62, 0.27)),
    ("RumputJarang", "sparse_grass", 400.0, (1.2, 2.4, 2.4)),
    ("TanahMerah", "red_laterite_soil_stones", 350.0, None),
    ("Lumpur", "muddy_tracks", 450.0, None),
    ("BatuSungai", "river_small_rocks", 250.0, None),
    ("TepiSungai", "brown_mud_02", 350.0, None),
]
LANDSCAPE_SPACING = 60.0
SHARED = unreal.SamplerSourceMode.SSM_WRAP_WORLD_GROUP_SETTINGS


def log(*a):
    LOG.append(" ".join(str(x) for x in a))
    unreal.log_warning("GRTERRAIN " + " ".join(str(x) for x in a))


def texture(name, kind):
    """kind: diff, nor_gl or arm."""
    asset = f"T_{name}_{kind}"
    path = f"{TEX}/{asset}"
    if lib.does_asset_exist(path):
        return lib.load_asset(path)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", f"{SRC}/{name}_{kind}_2k.jpg")
    task.set_editor_property("destination_path", TEX)
    task.set_editor_property("destination_name", asset)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", False)
    task.set_editor_property("replace_existing", True)
    tools.import_asset_tasks([task])
    tex = lib.load_asset(path)
    if not tex:
        log("GAGAL impor", asset)
        return None
    if kind == "nor_gl":
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("srgb", False)
        # Poly Haven's GL normals have green up; Unreal wants it down.
        tex.set_editor_property("flip_green_channel", True)
    elif kind == "arm":
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
        tex.set_editor_property("srgb", False)
    lib.save_loaded_asset(tex)
    return tex


def sample(mat, tex, kind, x, y, uv=None):
    node = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSample, x, y)
    node.set_editor_property("texture", tex)
    node.set_editor_property("sampler_source", SHARED)
    node.set_editor_property("sampler_type", {
        "diff": unreal.MaterialSamplerType.SAMPLERTYPE_COLOR,
        "nor_gl": unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
        "arm": unreal.MaterialSamplerType.SAMPLERTYPE_MASKS,
    }[kind])
    if uv is not None:
        mel.connect_material_expressions(uv, "", node, "UVs")
    return node


def new_material(name):
    path = f"{DST}/{name}"
    if lib.does_asset_exist(path):
        # Rebuilt in place, so the landscape and the bridges keep their reference.
        mat = lib.load_asset(path)
        mel.delete_all_material_expressions(mat)
        return mat
    return tools.create_asset(name, DST, unreal.Material, unreal.MaterialFactoryNew())


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


def landscape_material():
    """Weight-blended layers: each layer's color, normal and ARM times its weight, summed."""
    mat = new_material("M_GR_Tanah")
    sums = {"diff": None, "nor_gl": None, "arm": None}
    coords = {}
    for i, (layer, name, tile, tint) in enumerate(LAYERS):
        y = i * 700
        if tile not in coords:
            c = mel.create_material_expression(mat, unreal.MaterialExpressionLandscapeLayerCoords, -2200, y)
            c.set_editor_property("mapping_scale", tile / LANDSCAPE_SPACING)
            coords[tile] = c
        weight = mel.create_material_expression(mat, unreal.MaterialExpressionLandscapeLayerSample, -1500, y + 300)
        weight.set_editor_property("parameter_name", layer)
        for k, kind in enumerate(("diff", "nor_gl", "arm")):
            tex = texture(name, kind)
            s = sample(mat, tex, kind, -1500, y + k * 200, coords[tile])
            out = "RGB"
            if kind == "diff" and tint:
                c = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -1500, y - 120)
                c.set_editor_property("constant", unreal.LinearColor(tint[0], tint[1], tint[2], 1))
                s = binary(mat, unreal.MaterialExpressionMultiply, s, c, -1300, y, "RGB", "")
                out = ""
            m = binary(mat, unreal.MaterialExpressionMultiply, s, weight, -1100, y + k * 200, out, "")
            sums[kind] = m if sums[kind] is None else binary(mat, unreal.MaterialExpressionAdd, sums[kind], m, -800, y + k * 200)
    mel.connect_material_property(sums["diff"], "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(sums["nor_gl"], "", unreal.MaterialProperty.MP_NORMAL)
    mel.connect_material_property(mask(mat, sums["arm"], "r", -500, 600), "", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)
    mel.connect_material_property(mask(mat, sums["arm"], "g", -500, 800), "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -500, 1000)
    spec.set_editor_property("r", 0.3)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(mat)
    lib.save_loaded_asset(mat)
    log("material", mat.get_path_name())
    return mat


def triplanar_material(name, set_name, tile, tint=1.0):
    """World-aligned on all three axes, so long bridge boxes and tall foundations do not stretch the texture."""
    mat = new_material(name)
    tex = texture(set_name, "diff")
    arm = texture(set_name, "arm")
    pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -2000, 0)
    nrm = mel.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -2000, 600)
    planes = []
    for i, ch in enumerate(("gb", "rb", "rg")):
        m = mask(mat, pos, ch, -1700, i * 250)
        d = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, -1500, i * 250)
        d.set_editor_property("const_b", tile)
        mel.connect_material_expressions(m, "", d, "A")
        planes.append((sample(mat, tex, "diff", -1200, i * 250, d), sample(mat, arm, "arm", -1200, 800 + i * 250, d)))
    a = mel.create_material_expression(mat, unreal.MaterialExpressionAbs, -1700, 700)
    mel.connect_material_expressions(nrm, "", a, "")
    p = mel.create_material_expression(mat, unreal.MaterialExpressionPower, -1500, 700)
    p.set_editor_property("const_exponent", 4.0)
    mel.connect_material_expressions(a, "", p, "Base")
    ones = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -1500, 900)
    ones.set_editor_property("constant", unreal.LinearColor(1, 1, 1, 1))
    total = binary(mat, unreal.MaterialExpressionDotProduct, p, ones, -1300, 900)
    w = binary(mat, unreal.MaterialExpressionDivide, p, total, -1100, 800)
    color = rough = None
    for i, ch in enumerate("rgb"):
        wi = mask(mat, w, ch, -900, 700 + i * 150)
        c = binary(mat, unreal.MaterialExpressionMultiply, planes[i][0], wi, -700, i * 250, "RGB", "")
        r = binary(mat, unreal.MaterialExpressionMultiply, mask(mat, planes[i][1], "g", -900, 1300 + i * 150), wi, -700, 900 + i * 250)
        color = c if color is None else binary(mat, unreal.MaterialExpressionAdd, color, c, -500, i * 250)
        rough = r if rough is None else binary(mat, unreal.MaterialExpressionAdd, rough, r, -500, 900 + i * 250)
    if tint != 1.0:
        t = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 0)
        t.set_editor_property("const_b", tint)
        mel.connect_material_expressions(color, "", t, "A")
        color = t
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    lib.save_loaded_asset(mat)
    log("material", mat.get_path_name())
    return mat


def water_material(name="M_GR_Air", rgb=(0.06, 0.075, 0.045), roughness=0.12):
    """The river: a murky green-brown city river. M_GR_Kali: the shallow kali between its walls, muddier and lighter."""
    mat = new_material(name)
    color = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -600, 0)
    color.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1))
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -600, 200)
    rough.set_editor_property("r", roughness)
    # Slow ripples: the mud normal map, large and flattened, drifting downstream.
    pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1600, 500)
    xy = mask(mat, pos, "rg", -1400, 500)
    scale = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, -1200, 500)
    scale.set_editor_property("const_b", 700.0)
    mel.connect_material_expressions(xy, "", scale, "A")
    pan = mel.create_material_expression(mat, unreal.MaterialExpressionPanner, -1000, 500)
    pan.set_editor_property("speed_x", 0.02)
    pan.set_editor_property("speed_y", 0.012)
    mel.connect_material_expressions(scale, "", pan, "Coordinate")
    ripple = sample(mat, texture("brown_mud_02", "nor_gl"), "nor_gl", -800, 500, pan)
    flat = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -800, 750)
    flat.set_editor_property("constant", unreal.LinearColor(0, 0, 1, 1))
    lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -500, 600)
    lerp.set_editor_property("const_alpha", 0.35)
    mel.connect_material_expressions(flat, "", lerp, "A")
    mel.connect_material_expressions(ripple, "RGB", lerp, "B")
    mel.connect_material_property(lerp, "", unreal.MaterialProperty.MP_NORMAL)
    spec = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -600, 300)
    spec.set_editor_property("r", 0.6)
    mel.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(mat)
    lib.save_loaded_asset(mat)
    log("material", mat.get_path_name())
    return mat


def story_spots():
    """Every 2D position in the chapter data and the furniture: they keep level ground."""
    data = os.path.join(unreal.Paths.project_content_dir(), "Data")
    spots = []
    for f in glob.glob(os.path.join(data, "bab*.json")) + [os.path.join(data, "furniture.json")]:
        text = open(f, encoding="utf-8").read()
        for m in re.finditer(r'"pos"\s*:\s*\[\s*(-?[\d.]+)\s*,\s*(-?[\d.]+)', text):
            spots.append(unreal.Vector2D(float(m.group(1)), float(m.group(2))))
    return spots


def load_all_actors():
    """World Partition: a commandlet loads no cells, so load every actor of the map."""
    wpl = unreal.WorldPartitionBlueprintLibrary
    result = wpl.get_actor_descs()
    descs = result[1] if isinstance(result, tuple) else result
    guids = [d.guid for d in descs]
    wpl.load_actors(guids)
    log("actor dimuat", len(guids))


def instanced_usage():
    """GRTerrainDetails draws bridges, walls, the kali and the trees as instanced meshes; without this flag the game
    swaps their materials for the default grid."""
    paths = [f"{DST}/{n}" for n in ("M_GR_Beton", "M_GR_Pondasi", "M_GR_Air", "M_GR_Kali")]
    paths += [a for a in lib.list_assets("/Game/GemahRipah/Props/_Batch", recursive=True) if "/MI_" in a]
    done = set()
    for path in paths:
        mat = lib.load_asset(path)
        while isinstance(mat, unreal.MaterialInstance):
            mat = mat.get_editor_property("parent")
        if not mat or mat.get_path_name() in done:
            continue
        done.add(mat.get_path_name())
        if not mat.get_editor_property("used_with_instanced_static_meshes"):
            mat.set_editor_property("used_with_instanced_static_meshes", True)
            mel.recompile_material(mat)
            lib.save_loaded_asset(mat)
            log("flag instanced", mat.get_path_name())


def main():
    ground = landscape_material()
    triplanar_material("M_GR_Beton", "concrete_floor_02", 250.0)
    triplanar_material("M_GR_Pondasi", "river_small_rocks", 120.0, 0.8)
    water_material()
    water_material("M_GR_Kali", (0.16, 0.15, 0.085), 0.3)
    instanced_usage()
    for path in MAPS:
        build_map(path, ground)


def build_map(path, ground):
    world = unreal.EditorLoadingAndSavingUtils.load_map(path)
    if not world:
        log("GAGAL memuat", path)
        return
    log("map", path)
    load_all_actors()
    spots = story_spots()
    log("titik cerita datar", len(spots))
    report = unreal.GRScriptLibrary.build_terrain(world, spots, ground.get_path_name(), f"{DST}/M_GR_Pondasi.M_GR_Pondasi")
    report, _, files = report.partition("|FILES|")
    log(report)
    if report.startswith("OK"):
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        log("tersimpan")
        # Removed actors: their World Partition package files are not deleted by the save.
        gone = 0
        for f in filter(None, files.split(";")):
            path = os.path.abspath(unreal.Paths.convert_relative_path_to_full(f))
            if os.path.exists(path):
                os.remove(path)
                gone += 1
        log("file aktor dihapus", gone)


try:
    main()
finally:
    out = os.path.join(unreal.Paths.project_saved_dir(), "build_terrain.log")
    with open(out, "w", encoding="utf-8") as f:
        f.write("\n".join(LOG) + "\n")
