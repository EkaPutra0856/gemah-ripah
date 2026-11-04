"""Builds the clothing material for the townspeople (World/GRCrowd.cpp).

Run inside the editor (needs the Python plugin; the commandlet resolves a relative script path from the engine
binaries folder, so pass the full path):
  UnrealEditor-Cmd.exe GemahRipah.uproject -run=pythonscript -script="<project>/Tools/Python/make_warga_material.py"

Creates /Game/GemahRipah/Materials/M_GR_Warga: the mannequin colored by height above its feet, so one material
reads as shoes, trousers, shirt and skin. AGRWarga sets per person:
  Shirt, Pants, Skin, Shoes   vector parameters
  ShoeZ, HipZ, NeckZ          heights above the feet in the mesh's unscaled space, cm (lower while seated)
"""
import unreal

PATH = "/Game/GemahRipah/Materials"
NAME = "M_GR_Warga"
MEL = unreal.MaterialEditingLibrary


def vector_param(mat, name, color, x, y):
    node = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", unreal.LinearColor(*color))
    return node


def scalar_param(mat, name, value, x, y):
    node = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("default_value", value)
    return node


def pick(mat, height, limit, above, below, x, y):
    """above where height >= limit, below otherwise."""
    node = MEL.create_material_expression(mat, unreal.MaterialExpressionIf, x, y)
    MEL.connect_material_expressions(height, "", node, "A")
    MEL.connect_material_expressions(limit, "", node, "B")
    # Input names as the editor shows them (not the C++ property names).
    MEL.connect_material_expressions(above, "", node, "A > B")
    MEL.connect_material_expressions(above, "", node, "A == B")
    MEL.connect_material_expressions(below, "", node, "A < B")
    return node


def main():
    full = PATH + "/" + NAME
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        # AGRWarga's constructor loads it at startup, and clearing a loaded material's graph asserts.
        unreal.log_error("make_warga_material: delete Content/GemahRipah/Materials/M_GR_Warga.uasset first, then run again")
        return
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("used_with_skeletal_mesh", True)

    # Height of the pixel above the feet, in the mesh's own (unscaled) space: its origin is between the feet.
    world = MEL.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1400, 0)
    local = MEL.create_material_expression(mat, unreal.MaterialExpressionTransformPosition, -1200, 0)
    local.set_editor_property("transform_source_type", unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD)
    local.set_editor_property("transform_type", unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_LOCAL)
    MEL.connect_material_expressions(world, "", local, "")
    height = MEL.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -1000, 60)
    height.set_editor_property("r", False)
    height.set_editor_property("g", False)
    height.set_editor_property("b", True)
    height.set_editor_property("a", False)
    MEL.connect_material_expressions(local, "", height, "")

    shirt = vector_param(mat, "Shirt", (0.8, 0.8, 0.8, 1.0), -1000, 300)
    pants = vector_param(mat, "Pants", (0.05, 0.05, 0.06, 1.0), -1000, 500)
    skin = vector_param(mat, "Skin", (0.45, 0.25, 0.15, 1.0), -1000, 700)
    shoes = vector_param(mat, "Shoes", (0.03, 0.03, 0.03, 1.0), -1000, 900)
    shoe_z = scalar_param(mat, "ShoeZ", 9.0, -1000, -300)
    hip_z = scalar_param(mat, "HipZ", 95.0, -1000, -200)
    neck_z = scalar_param(mat, "NeckZ", 150.0, -1000, -100)

    body = pick(mat, height, hip_z, shirt, pants, -700, 200)
    head = pick(mat, height, neck_z, skin, body, -450, 200)
    color = pick(mat, height, shoe_z, head, shoes, -200, 200)
    MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)

    rough = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, 450)
    rough.set_editor_property("r", 0.8)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)

    MEL.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    unreal.log_warning("make_warga_material: saved " + full)


main()
