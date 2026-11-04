"""Builds NPC conversation animations from MM_Idle by layering procedural bone rotations.

Run inside the editor (needs the Python plugin):
  UnrealEditor-Cmd.exe GemahRipah.uproject -run=pythonscript -script="Tools/Python/make_talk_anims.py"

Creates in /Game/GemahRipah/Anims:
  MM_GR_Talk     standing, hand gestures and nods while speaking
  MM_GR_Sit      seated idle (for NPCs behind desks / on benches)
  MM_GR_SitTalk  seated with gestures
  MM_GR_Ride     Bayu on the "motor" prop model (World/GRPropModels.cpp), solved with two-bone IK
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



# --- Rider pose -------------------------------------------------------------------------------------------------
# Spots on the "motor" model, in mesh space (the mesh origin is the scooter origin: mesh +Y = scooter +X,
# mesh +X = scooter -Y). Keep in sync with GRPropModels.cpp: seat top 79, floorboard top 42, grips (37.5, +-31, 102).
RIDE_PELVIS = (0.0, -22.0, 89.2)       # hips about 9 cm above the seat top
RIDE_ANKLE_SIDE, RIDE_ANKLE_Y, RIDE_ANKLE_Z = 11.0, 0.0, 50.5  # ankle bone 8.5 cm above the floorboard
RIDE_GRIP = (31.0, 37.5, 102.0)
RIDE_FINGERS = ("index", "middle", "ring", "pinky")


def ride_parents():
    p = dict(PARENT)
    for s in ("l", "r"):
        for f in RIDE_FINGERS:
            p["%s_metacarpal_%s" % (f, s)] = "hand_" + s
            p["%s_01_%s" % (f, s)] = "%s_metacarpal_%s" % (f, s)
            p["%s_02_%s" % (f, s)] = "%s_01_%s" % (f, s)
            p["%s_03_%s" % (f, s)] = "%s_02_%s" % (f, s)
        p["ball_" + s] = "foot_" + s
    # ABP_Unarmed pulls the feet to these IK bones; left at MM_Idle's standing spots, they dragged Bayu's feet
    # off the floorboard down to the road.
    p.update({"ik_foot_root": "root", "ik_foot_l": "ik_foot_root", "ik_foot_r": "ik_foot_root",
              "ik_hand_root": "root", "ik_hand_gun": "ik_hand_root", "ik_hand_l": "ik_hand_gun", "ik_hand_r": "ik_hand_gun"})
    return p


def vadd(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def vsub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def vmul(a, k):
    return (a[0] * k, a[1] * k, a[2] * k)


def vdot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def vcross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def vlen(a):
    return math.sqrt(vdot(a, a))


def vnorm(a):
    n = vlen(a)
    return vmul(a, 1.0 / n) if n > 1e-6 else (0.0, 0.0, 0.0)


def qnorm(q):
    n = math.sqrt(sum(c * c for c in q))
    return tuple(c / n for c in q)


def qrot(q, v):
    p = qmul(qmul(q, (v[0], v[1], v[2], 0.0)), qinv(q))
    return (p[0], p[1], p[2])


def qarc(u, v):
    """Shortest rotation taking direction u to direction v."""
    u, v = vnorm(u), vnorm(v)
    d = vdot(u, v)
    if d < -0.9999:
        axis = vnorm(vcross(u, X if abs(u[0]) < 0.9 else Y))
        return axis_angle(axis, 180.0)
    c = vcross(u, v)
    return qnorm((c[0], c[1], c[2], 1.0 + d))


def qbasis(old_f, old_a, new_f, new_a):
    """Rotation taking the frame (forward f, across a) to the new one."""
    def frame(f, a):
        e1 = vnorm(f)
        e2 = vnorm(vsub(a, vmul(e1, vdot(a, e1))))
        return e1, e2, vcross(e1, e2)
    o, n = frame(old_f, old_a), frame(new_f, new_a)
    m = [[sum(n[k][i] * o[k][j] for k in range(3)) for j in range(3)] for i in range(3)]
    tr = m[0][0] + m[1][1] + m[2][2]
    if tr > 0:
        s = math.sqrt(tr + 1.0) * 2
        q = ((m[2][1] - m[1][2]) / s, (m[0][2] - m[2][0]) / s, (m[1][0] - m[0][1]) / s, 0.25 * s)
    elif m[0][0] > m[1][1] and m[0][0] > m[2][2]:
        s = math.sqrt(1.0 + m[0][0] - m[1][1] - m[2][2]) * 2
        q = (0.25 * s, (m[0][1] + m[1][0]) / s, (m[0][2] + m[2][0]) / s, (m[2][1] - m[1][2]) / s)
    elif m[1][1] > m[2][2]:
        s = math.sqrt(1.0 + m[1][1] - m[0][0] - m[2][2]) * 2
        q = ((m[0][1] + m[1][0]) / s, 0.25 * s, (m[1][2] + m[2][1]) / s, (m[0][2] - m[2][0]) / s)
    else:
        s = math.sqrt(1.0 + m[2][2] - m[0][0] - m[1][1]) * 2
        q = ((m[0][2] + m[2][0]) / s, (m[1][2] + m[2][1]) / s, 0.25 * s, (m[1][0] - m[0][1]) / s)
    return qnorm(q)


class Pose:
    """Local bone transforms with component-space queries and edits."""

    def __init__(self, local, parents):
        self.local = local
        self.parents = parents

    def cs(self):
        out = {}
        for n in self.local:
            q, t = self.local[n][0], self.local[n][1]
            par = self.parents.get(n)
            if par:
                pq, pp = out[par]
                out[n] = (qmul(pq, q), vadd(pp, qrot(pq, t)))
            else:
                out[n] = (q, t)
        return out

    def rotate(self, bone, d):
        """Turns a bone (and everything under it) by d in component space, about the bone itself."""
        cs = self.cs()
        par = self.parents.get(bone)
        pq = cs[par][0] if par else (0.0, 0.0, 0.0, 1.0)
        self.local[bone][0] = qnorm(qmul(qinv(pq), qmul(d, cs[bone][0])))

    def set_rotation(self, bone, q):
        cs = self.cs()
        par = self.parents.get(bone)
        pq = cs[par][0] if par else (0.0, 0.0, 0.0, 1.0)
        self.local[bone][0] = qnorm(qmul(qinv(pq), q))

    def match(self, bone, other):
        """Puts a bone exactly where another bone is (component space), e.g. an IK target on its limb."""
        cs = self.cs()
        par = self.parents.get(bone)
        pq, pp = cs[par] if par else ((0.0, 0.0, 0.0, 1.0), (0.0, 0.0, 0.0))
        q, p = cs[other]
        self.local[bone][0] = qnorm(qmul(qinv(pq), q))
        self.local[bone][1] = qrot(qinv(pq), vsub(p, pp))

    def two_bone(self, a, b, c, target, pole):
        cs = self.cs()
        pa, pb, pc = cs[a][1], cs[b][1], cs[c][1]
        lab, lbc = vlen(vsub(pb, pa)), vlen(vsub(pc, pb))
        to_target = vsub(target, pa)
        d = min(max(vlen(to_target), abs(lab - lbc) + 0.5), (lab + lbc) * 0.999)
        u = vnorm(to_target)
        v = vsub(pole, pa)
        v = vnorm(vsub(v, vmul(u, vdot(v, u))))
        cos_a = (lab * lab + d * d - lbc * lbc) / (2 * lab * d)
        sin_a = math.sqrt(max(0.0, 1.0 - cos_a * cos_a))
        joint = vadd(pa, vmul(vadd(vmul(u, cos_a), vmul(v, sin_a)), lab))
        self.rotate(a, qarc(vsub(pb, pa), vsub(joint, pa)))
        cs = self.cs()
        self.rotate(b, qarc(vsub(cs[c][1], cs[b][1]), vsub(vadd(pa, vmul(u, d)), cs[b][1])))


def ride_frame(pose, yaw_fix, orig_cs, t):
    w = 2 * math.pi * t
    pose.local["pelvis"][1] = RIDE_PELVIS
    pose.rotate("pelvis", yaw_fix)
    # Slight forward lean from the lower back; the neck takes most of it back so Bayu looks down the road.
    for bone, deg in (("spine_01", -4.0), ("spine_02", -3.0 + 0.6 * math.sin(w)), ("spine_03", -1.0), ("neck_01", 3.0), ("head", 4.0)):
        pose.rotate(bone, axis_angle(X, deg))
    for side, sx in (("l", 1.0), ("r", -1.0)):
        cs = pose.cs()
        hip = cs["thigh_" + side][1]
        ankle = (sx * RIDE_ANKLE_SIDE, RIDE_ANKLE_Y, RIDE_ANKLE_Z)
        pose.two_bone("thigh_" + side, "calf_" + side, "foot_" + side, ankle, vadd(hip, (sx * 8.0, 60.0, 30.0)))
        # Feet flat on the floorboard, pointing where the body points.
        pose.set_rotation("foot_" + side, qmul(yaw_fix, orig_cs["foot_" + side][0]))

        cs = pose.cs()
        shoulder = cs["upperarm_" + side][1]
        fwd = (0.0, math.cos(math.radians(25)), -math.sin(math.radians(25)))  # knuckles forward, a little down
        palm = (0.0, -math.sin(math.radians(25)), -math.cos(math.radians(25)))
        grip = (sx * RIDE_GRIP[0], RIDE_GRIP[1], RIDE_GRIP[2])
        wrist = vsub(vsub(grip, vmul(fwd, 5.0)), vmul(palm, 4.5))
        pose.two_bone("upperarm_" + side, "lowerarm_" + side, "hand_" + side, wrist, vadd(shoulder, (sx * 30.0, -5.0, -25.0)))

        # Palm down on the grip, pinky to the outside; the forearm takes half of the roll so the wrist does not twist.
        cs = pose.cs()
        old_f = vsub(cs["middle_01_" + side][1], cs["hand_" + side][1])
        old_a = vsub(cs["pinky_01_" + side][1], cs["index_01_" + side][1])
        turn = qbasis(old_f, old_a, fwd, (sx, 0.0, 0.0))
        hand_q = qmul(turn, cs["hand_" + side][0])
        axis = vnorm(vsub(cs["hand_" + side][1], cs["lowerarm_" + side][1]))
        twist = 2 * math.degrees(math.atan2(vdot(turn[:3], axis), turn[3]))
        twist = (twist + 180.0) % 360.0 - 180.0
        pose.rotate("lowerarm_" + side, axis_angle(axis, twist * 0.5))
        pose.set_rotation("hand_" + side, hand_q)

        # Fingers wrap round the grip: pick the curl direction that moves a fingertip toward the palm side.
        cs = pose.cs()
        across = (sx, 0.0, 0.0)
        p1, tip = cs["index_01_" + side][1], cs["index_03_" + side][1]
        moved = vsub(vadd(p1, qrot(axis_angle(across, 20.0), vsub(tip, p1))), tip)
        sign = 1.0 if vdot(moved, palm) > 0 else -1.0
        for finger in RIDE_FINGERS:
            for joint, deg in (("01", 45.0), ("02", 45.0), ("03", 25.0)):
                pose.rotate("%s_%s_%s" % (finger, joint, side), axis_angle(across, sign * deg))
    for bone, other in (("ik_foot_l", "foot_l"), ("ik_foot_r", "foot_r"), ("ik_hand_gun", "hand_r"), ("ik_hand_l", "hand_l"), ("ik_hand_r", "hand_r")):
        pose.match(bone, other)


def build_ride(name):
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    path = DST + "/" + name
    src = unreal.load_asset(SRC)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        seq = unreal.load_asset(path)
    else:
        seq = tools.duplicate_asset(name, DST, src)
    parents = ride_parents()
    names = ["root"] + list(parents.keys())
    frames = unreal.AnimationLibrary.get_num_frames(src)
    keys = frames + 1
    tracks = {n: ([], [], []) for n in names}
    yaw_fix = None
    report = ""
    for f in range(keys):
        poses = unreal.AnimationLibrary.get_bone_poses_for_frame(src, names, f, False)
        local = {}
        for n, p in zip(names, poses):
            r, tr = p.rotation, p.translation
            local[n] = [(r.x, r.y, r.z, r.w), (tr.x, tr.y, tr.z), p.scale3d]
        pose = Pose(local, parents)
        orig_cs = pose.cs()
        if yaw_fix is None:
            # MM_Idle stands turned about 22 degrees; square the hips to the scooter.
            hips = vsub(orig_cs["thigh_l"][1], orig_cs["thigh_r"][1])
            yaw_fix = axis_angle(Z, -math.degrees(math.atan2(hips[1], hips[0])))
        ride_frame(pose, yaw_fix, orig_cs, f / max(1, frames))
        if f == 0:
            cs = pose.cs()
            report = " ".join("%s=(%.0f,%.0f,%.0f)" % ((n,) + tuple(cs[n][1])) for n in ("pelvis", "calf_l", "foot_l", "lowerarm_l", "hand_l", "head"))
        for n in names:
            q, tr, sc = local[n]
            tracks[n][0].append(unreal.Vector(tr[0], tr[1], tr[2]))
            tracks[n][1].append(unreal.Quat(q[0], q[1], q[2], q[3]))
            tracks[n][2].append(sc)
    ctrl = seq.get_editor_property("controller")
    ctrl.open_bracket(unreal.Text("GR ride anim"), True)
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
    LOG.append("%s: %d keys %s" % (name, keys, report))


try:
    build("MM_GR_Talk", lambda t: talk_deltas(t, False), False)
    build("MM_GR_Sit", sit_deltas, True)
    build("MM_GR_SitTalk", sit_talk_deltas, True)
    build_ride("MM_GR_Ride")
except Exception as e:  # noqa: BLE001
    import traceback
    LOG.append("ERROR " + traceback.format_exc())
unreal.log_warning("GRANIM " + " | ".join(LOG))
