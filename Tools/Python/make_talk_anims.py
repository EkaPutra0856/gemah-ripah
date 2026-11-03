"""Builds NPC conversation animations from MM_Idle by layering procedural bone rotations.

Run inside the editor (needs the Python plugin):
  UnrealEditor-Cmd.exe GemahRipah.uproject -run=pythonscript -script="Tools/Python/make_talk_anims.py"

Creates in /Game/GemahRipah/Anims:
  MM_GR_Talk     standing, hand gestures and nods while speaking
  MM_GR_Sit      seated idle (for NPCs behind desks / on benches)
  MM_GR_SitTalk  seated with gestures
Rotations are applied in mesh (component) space: the mannequin faces +Y, up is +Z, its left side is +X.
"""
import math
import unreal

SRC = "/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle"
DST = "/Game/GemahRipah/Anims"
LOG = []

PARENT = {
    "pelvis": "root", "spine_01": "pelvis", "spine_02": "spine_01", "spine_03": "spine_02", "spine_04": "spine_03", "spine_05": "spine_04",
    "neck_01": "spine_05", "neck_02": "neck_01", "head": "neck_02",
    "clavicle_l": "spine_05", "upperarm_l": "clavicle_l", "lowerarm_l": "upperarm_l", "hand_l": "lowerarm_l",
    "clavicle_r": "spine_05", "upperarm_r": "clavicle_r", "lowerarm_r": "upperarm_r", "hand_r": "lowerarm_r",
    "thigh_l": "pelvis", "calf_l": "thigh_l", "foot_l": "calf_l",
    "thigh_r": "pelvis", "calf_r": "thigh_r", "foot_r": "calf_r",
}
ORDER = ["root"] + list(PARENT.keys())


def qmul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz)


def qinv(q):
    return (-q[0], -q[1], -q[2], q[3])


def axis_angle(axis, deg):
    r = math.radians(deg) * 0.5
    s = math.sin(r)
    return (axis[0] * s, axis[1] * s, axis[2] * s, math.cos(r))


X, Y, Z = (1, 0, 0), (0, 1, 0), (0, 0, 1)


def talk_deltas(t, seated):
    """Component-space rotations per bone at loop phase t (0..1). Two gesture cycles per loop, nods twice as fast."""
    w = 2 * math.pi * 2 * t
    d = {
        "spine_03": [(X, -3 + 1.5 * math.sin(w))],
        "head": [(X, 5 * math.sin(2 * w)), (Z, 6 * math.sin(w * 0.5 + 1))],
        # Right hand does most of the talking, left joins in on the beat.
        # Forearms stay roughly level (open-palm explaining), never raised like a wave.
        "upperarm_r": [(X, 20 + 9 * math.sin(w)), (Y, 10)],
        "lowerarm_r": [(X, 42 + 16 * math.sin(w + 0.9))],
        "hand_r": [(Z, 16 * math.sin(w + 1.6))],
        "upperarm_l": [(X, 14 + 7 * math.sin(w + 2.4)), (Y, -8)],
        "lowerarm_l": [(X, 34 + 12 * math.sin(w + 3.1))],
        "hand_l": [(Z, -12 * math.sin(w + 3.6))],
    }
    if seated:
        d["spine_03"] = [(X, -6 + 2 * math.sin(w))]
    return d


def sit_deltas(t):
    w = 2 * math.pi * t
    return {
        "thigh_l": [(X, 88), (Y, 4)],
        "thigh_r": [(X, 88), (Y, -4)],
        "calf_l": [(X, -92)],
        "calf_r": [(X, -92)],
        "spine_03": [(X, -4 + 1.2 * math.sin(w))],
        "upperarm_l": [(X, 30), (Y, -6)],
        "upperarm_r": [(X, 30), (Y, 6)],
        "lowerarm_l": [(X, 42)],
        "lowerarm_r": [(X, 42)],
        "head": [(X, 2 * math.sin(w * 2))],
    }


def build(name, deltas_fn, sit):
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    path = DST + "/" + name
    src = unreal.load_asset(SRC)
    # Rewrite an existing asset in place: NPCs load these at startup, so deleting them is not possible.
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        seq = unreal.load_asset(path)
    else:
        seq = tools.duplicate_asset(name, DST, src)
    frames = unreal.AnimationLibrary.get_num_frames(src)
    keys = frames + 1
    names = [n for n in ORDER]
    tracks = {n: ([], [], []) for n in names}
    for f in range(keys):
        poses = unreal.AnimationLibrary.get_bone_poses_for_frame(src, names, f, False)
        local = {}
        for n, p in zip(names, poses):
            r = p.rotation
            local[n] = [(r.x, r.y, r.z, r.w), p.translation, p.scale3d]
        t = f / max(1, frames)
        deltas = deltas_fn(t)
        cs_new = {}
        for n in names:
            parent = PARENT.get(n)
            q_parent = cs_new[parent] if parent else (0.0, 0.0, 0.0, 1.0)
            q_local = local[n][0]
            for axis, deg in deltas.get(n, []):
                d = axis_angle(axis, deg)
                q_local = qmul(qmul(qinv(q_parent), qmul(d, q_parent)), q_local)
            local[n][0] = q_local
            cs_new[n] = qmul(q_parent, q_local)
        if sit:
            tr = local["pelvis"][1]
            local["pelvis"][1] = unreal.Vector(tr.x, tr.y - 12.0, tr.z - 44.0)
        for n in names:
            q, tr, sc = local[n]
            tracks[n][0].append(tr)
            tracks[n][1].append(unreal.Quat(q[0], q[1], q[2], q[3]))
            tracks[n][2].append(sc)
    ctrl = seq.get_editor_property("controller")
    ctrl.open_bracket(unreal.Text("GR talk anim"), True)
    for n in names:
        if n == "root":
            continue
        pos, rot, scl = tracks[n]
        try:
            ctrl.set_bone_track_keys(n, pos, rot, scl, True)
        except TypeError:
            ctrl.set_bone_track_keys(n, [unreal.Vector3f(v.x, v.y, v.z) for v in pos], [unreal.Quat4f(q.x, q.y, q.z, q.w) for q in rot],
                                     [unreal.Vector3f(v.x, v.y, v.z) for v in scl], True)
    ctrl.close_bracket(True)
    unreal.EditorAssetLibrary.save_loaded_asset(seq)
    LOG.append("%s: %d keys" % (name, keys))


def sit_talk_deltas(t):
    d = {k: v for k, v in sit_deltas(t).items() if k.startswith(("thigh", "calf"))}
    d.update(talk_deltas(t, True))
    return d


try:
    build("MM_GR_Talk", lambda t: talk_deltas(t, False), False)
    build("MM_GR_Sit", sit_deltas, True)
    build("MM_GR_SitTalk", sit_talk_deltas, True)
except Exception as e:  # noqa: BLE001
    import traceback
    LOG.append("ERROR " + traceback.format_exc())
unreal.log_warning("GRANIM " + " | ".join(LOG))
