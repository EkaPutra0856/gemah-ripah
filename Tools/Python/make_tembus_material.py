"""Builds the see-through roof material for the top-down rooms (World/GRRoomShells.cpp).

Run with the full script path (the commandlet resolves a relative one from the engine binaries folder):
  UnrealEditor-Cmd.exe GemahRipah.uproject -run=pythonscript -script="<project>/Tools/Python/make_tembus_material.py"

Creates /Game/GemahRipah/Materials/M_GR_Tembus: translucent, with the vector parameter Color and the scalar Opacity.
"""
import unreal

PATH = "/Game/GemahRipah/Materials"
NAME = "M_GR_Tembus"
MEL = unreal.MaterialEditingLibrary


def main():
    full = PATH + "/" + NAME
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        mat = unreal.load_asset(full)
        MEL.delete_all_material_expressions(mat)
    else:
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        mat = tools.create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    color = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 0)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(0.5, 0.17, 0.08, 1.0))
    opacity = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -400, 200)
    opacity.set_editor_property("parameter_name", "Opacity")
    opacity.set_editor_property("default_value", 1.0)
    MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)

    MEL.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(full, only_if_is_dirty=False)
    unreal.log("make_tembus_material: saved " + full)


main()
