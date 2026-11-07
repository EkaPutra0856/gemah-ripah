"""Sets the usage flags the cooked game needs on the base materials of everything it draws instanced or as Nanite.

The editor quietly turns a missing flag on while playing, but a cooked game cannot compile a new shader and shows the grey
default material instead. Run with the pythonscript commandlet before packaging:
UnrealEditor-Cmd.exe GemahRipah.uproject -run=pythonscript -script=Tools/fix_material_usage.py -unattended -nosplash
"""
import os

import unreal

ROOT = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
FOLDERS = ("/Game/GemahRipah/Materials", "/Game/GemahRipah/Props")


def base_of(material):
    base = material.get_base_material() if material else None
    return base if base and base.get_path_name().startswith("/Game/") else None


def main():
    instanced, nanite = {}, {}

    def note(material, on_nanite):
        base = base_of(material)
        if base:
            instanced[base.get_path_name()] = base
            if on_nanite:
                nanite[base.get_path_name()] = base

    for line in open(os.path.join(ROOT, "Config", "CookExtra.txt"), encoding="utf8"):
        path = line.strip()
        if not path.startswith("/Game/"):
            continue
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if isinstance(asset, unreal.StaticMesh):
            on_nanite = asset.get_editor_property("nanite_settings").enabled
            for slot in asset.static_materials:
                note(slot.material_interface, on_nanite)
        elif isinstance(asset, unreal.MaterialInterface):
            note(asset, False)

    for folder in FOLDERS:
        for path in unreal.EditorAssetLibrary.list_assets(folder, recursive=True, include_folder=False):
            asset = unreal.EditorAssetLibrary.load_asset(path)
            if isinstance(asset, unreal.MaterialInterface):
                # The Plus materials replace slots on pack meshes, some of which are Nanite.
                note(asset, "/Materials/Plus/" in path)
            elif isinstance(asset, unreal.StaticMesh):
                on_nanite = asset.get_editor_property("nanite_settings").enabled
                for slot in asset.static_materials:
                    note(slot.material_interface, on_nanite)

    changed = 0
    for name, base in instanced.items():
        dirty = False
        if not base.get_editor_property("used_with_instanced_static_meshes"):
            base.set_editor_property("used_with_instanced_static_meshes", True)
            dirty = True
        if name in nanite and not base.get_editor_property("used_with_nanite"):
            base.set_editor_property("used_with_nanite", True)
            dirty = True
        if dirty:
            unreal.MaterialEditingLibrary.recompile_material(base)
            unreal.EditorAssetLibrary.save_loaded_asset(base, only_if_is_dirty=False)
            unreal.log("GRUSAGE fixed " + name)
            changed += 1
    unreal.log("GRUSAGE done: {} checked, {} changed".format(len(instanced), changed))


main()
