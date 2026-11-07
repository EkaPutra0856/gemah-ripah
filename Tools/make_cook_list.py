"""Writes Config/CookExtra.txt, the assets the game only reaches through path strings in the C++.

A cook follows references from the maps, so assets loaded by name would be left out of the package, and cooking the whole
pack folders they come from would add tens of gigabytes. Run this after adding such a path, before packaging. Paths under
/Game/GemahRipah are covered by DirectoriesToAlwaysCook in DefaultGame.ini, including the ones built at runtime.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ENGINE_CONTENT = r"E:\UE_5.8\Engine\Content"
LITERAL = re.compile(r'"(/(?:Game|Engine)/[A-Za-z0-9_\-/.]+)"')
DECALS = {
    "RoadDecal": "/Game/Scene_RoadsideConstruction/Assets/MS/Decals/{0}/MI_{0}",
    "UnfinishedDecal": "/Game/Scene_UnfinishedBuilding/Assets/Megascans/Decals/{0}/MI_{0}",
}
TRASH = "/Game/Scene_Trash/Assets/MS/Decals/{0}/MI_{0}{1}"
# The old city stays locked for players, only the settings point at it. A leading minus keeps it out of the cook.
NEVER = ["-/Game/GemahRipah/Maps/L_KotaSentosa"]


def on_disk(package):
    root, rest = (ROOT + "/Content", package[6:]) if package.startswith("/Game/") else (ENGINE_CONTENT, package[8:])
    return any(os.path.isfile(os.path.join(root, rest + ext)) for ext in (".uasset", ".umap"))


def main():
    found = set()
    for folder, _, names in os.walk(os.path.join(ROOT, "Source", "GemahRipah")):
        for name in names:
            if not name.endswith((".cpp", ".h")):
                continue
            text = open(os.path.join(folder, name), encoding="utf8").read()
            found.update(path.split(".")[0] for path in LITERAL.findall(text) if not path.endswith("/"))
            for helper, pattern in DECALS.items():
                found.update(pattern.format(n) for n in re.findall(helper + r'\(TEXT\("([^"]+)"\)', text))
            found.update(TRASH.format(a, b) for a, b in re.findall(r'TrashDecal\(TEXT\("([^"]+)"\)(?:, TEXT\("([^"]*)"\))?', text))
            if name == "GRPlusDressing.cpp":
                # The road damage table feeds RoadDecal through a variable.
                found.update(DECALS["RoadDecal"].format(n) for n in re.findall(r'\{ TEXT\("(Decal_[^"]+)"\), FVector2D', text))

    extra = sorted(p for p in found if not p.startswith("/Game/GemahRipah/"))
    missing = [p for p in extra if not on_disk(p)]
    keep = [p for p in extra if p not in missing]
    with open(os.path.join(ROOT, "Config", "CookExtra.txt"), "w", encoding="utf8", newline="\n") as out:
        out.write("\n".join(keep + NEVER) + "\n")
    print(len(keep), "assets listed")
    for path in missing:
        print("not found:", path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
