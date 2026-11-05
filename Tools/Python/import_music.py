"""Imports the WAV songs in Art/Audio_src into /Game/GemahRipah/Audio/Musik.

Run: UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script=Tools/Python/import_music.py
"""
import os
import unreal

SRC = os.path.normpath(os.path.join(unreal.Paths.project_dir(), "..", "Art", "Audio_src"))
DEST = "/Game/GemahRipah/Audio/Musik"

tasks = []
for name in sorted(os.listdir(SRC)):
    if not name.lower().endswith(".wav"):
        continue
    task = unreal.AssetImportTask()
    task.filename = os.path.join(SRC, name)
    task.destination_path = DEST
    task.automated = True
    task.replace_existing = True
    task.save = True
    tasks.append(task)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for task in tasks:
    for path in task.imported_object_paths:
        sound = unreal.load_asset(path)
        unreal.log("GRMUSIC imported %s %.1fs" % (path, sound.get_editor_property("duration")))
