"""Imports the Tripo characters (rigged in Blender, see Art/_skrip_rig) and sets up runtime retargeting.

Input: Art/_ekspor_ue/<Name>/SK_<Name>.fbx and T_<Name>_basecolor/normal/rm.png, written by
Art/_skrip_rig/exp_ue.py (Blender 4.0 headless; strips face shape keys, decimates, downsizes textures):
  blender -b <Art/Name/Name_rig_detail.blend> --python exp_ue.py -- <Art/_ekspor_ue/Name> <Name> 40000 2048

Run inside the editor (needs the Python plugin):
  UnrealEditor-Cmd.exe GemahRipah.uproject -run=pythonscript -script="Tools/Python/import_tripo_chars.py"

Creates in /Game/GemahRipah/Characters:
  M_GR_Tripo            master material: base color, tangent normal, glTF roughness (G) / metal (B)
  M_GR_MulutDalam       dark inside of the mouth
  SKEL_GR_Tripo         one skeleton shared by every character (25 body bones, plus hair bones where present)
  <Name>/SK_<Name>      skeletal mesh, MI_<Name>, T_<Name>_*
  IK_GR_Manny           IK rig of the UE mannequin (auto generated)
  IK_GR_Tripo           IK rig of the Tripo skeleton (hand-made chains)
  RTG_GR_MannyKeTripo   retargeter used at runtime by UGRRetargetAnimInstance (Character/GRBody.cpp):
                        the mannequin keeps playing every animation hidden, the Tripo body copies its pose.
"""
import os
import unreal

ART = r"E:/NewSSDFile/Game Pemerintahan/Art/_ekspor_ue"
DST = "/Game/GemahRipah/Characters"
MANNY = "/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple"
SKEL = f"{DST}/SKEL_GR_Tripo"
LOG = []

tools = unreal.AssetToolsHelpers.get_asset_tools()
lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary


def log(*a):
    LOG.append(" ".join(str(x) for x in a))


def load(path):
    return lib.load_asset(path) if lib.does_asset_exist(path) else None


def master_material():
    path = f"{DST}/M_GR_Tripo"
    if lib.does_asset_exist(path):
        return load(path)
    mat = tools.create_asset("M_GR_Tripo", DST, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("used_with_skeletal_mesh", True)
    base = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -500, -200)
    base.set_editor_property("parameter_name", "Base")
    # Every texture parameter needs a default, or the material fails to compile and renders as the default material.
    base.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/DefaultTexture"))
    nrm = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -500, 100)
    nrm.set_editor_property("parameter_name", "Normal")
    nrm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    nrm.set_editor_property("texture", unreal.load_asset("/Engine/EngineMaterials/DefaultNormal"))
    rm = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, -500, 400)
    rm.set_editor_property("parameter_name", "RM")
    rm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    rm.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
    mel.connect_material_property(base, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(nrm, "RGB", unreal.MaterialProperty.MP_NORMAL)
    mel.connect_material_property(rm, "G", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(rm, "B", unreal.MaterialProperty.MP_METALLIC)
    mel.recompile_material(mat)
    lib.save_loaded_asset(mat)
    log("MASTER", [str(x) for x in mel.get_texture_parameter_names(mat)], "compiled", mel.get_num_material_expressions(mat))
    return mat


def mouth_material():
    path = f"{DST}/M_GR_MulutDalam"
    if lib.does_asset_exist(path):
        return load(path)
    mat = tools.create_asset("M_GR_MulutDalam", DST, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("used_with_skeletal_mesh", True)
    col = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -300, 0)
    col.set_editor_property("constant", unreal.LinearColor(0.08, 0.02, 0.02, 1.0))
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    rough = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 200)
    rough.set_editor_property("r", 0.6)
    mel.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(mat)
    lib.save_loaded_asset(mat)
    return mat


def import_file(src, dest, name, options=None):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", src)
    task.set_editor_property("destination_path", dest)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    if options:
        task.set_editor_property("options", options)
    tools.import_asset_tasks([task])
    paths = list(task.get_editor_property("imported_object_paths"))
    return paths


def import_textures(name, folder, dest):
    out = {}
    for role in ("basecolor", "normal", "rm"):
        src = os.path.join(folder, f"T_{name}_{role}.png")
        if not os.path.exists(src):
            continue
        import_file(src, dest, f"T_{name}_{role}")
        tex = load(f"{dest}/T_{name}_{role}")
        if role == "normal":
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property("srgb", False)
        elif role == "rm":
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
            tex.set_editor_property("srgb", False)
        lib.save_loaded_asset(tex)
        out[role] = tex
    return out


def import_mesh(name, folder, dest, skeleton):
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_mesh", True)
    ui.set_editor_property("import_as_skeletal", True)
    ui.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    ui.set_editor_property("import_materials", False)
    ui.set_editor_property("import_textures", False)
    ui.set_editor_property("import_animations", False)
    ui.set_editor_property("create_physics_asset", False)
    if skeleton:
        ui.set_editor_property("skeleton", skeleton)
    data = ui.get_editor_property("skeletal_mesh_import_data")
    data.set_editor_property("import_morph_targets", False)
    data.set_editor_property("convert_scene", True)
    # exp_ue.py writes centimeters (root bone scale 1, which the retargeter needs).
    data.set_editor_property("import_uniform_scale", 1.0)
    data.set_editor_property("update_skeleton_reference_pose", False)
    paths = import_file(os.path.join(folder, f"SK_{name}.fbx"), dest, f"SK_{name}", ui)
    return load(f"{dest}/SK_{name}"), paths


def assign_materials(mesh, mi, mouth):
    # Array elements come back as copies: write each changed struct back by index.
    mats = mesh.get_editor_property("materials")
    for i in range(len(mats)):
        m = mats[i]
        slot = str(m.get_editor_property("material_slot_name"))
        m.set_editor_property("material_interface", mouth if "Mouth" in slot else mi)
        mats[i] = m
    mesh.modify()
    mesh.set_editor_property("materials", mats)
    # Not marked dirty by the property write: save regardless.
    lib.save_asset(mesh.get_path_name().split(".")[0], only_if_is_dirty=False)


def report(name, mesh):
    b = mesh.get_bounds()
    log("MESH", name, "z", round(b.origin.z - b.box_extent.z, 1), round(b.origin.z + b.box_extent.z, 1),
        "x", round(b.origin.x, 1), round(b.box_extent.x, 1), "y", round(b.origin.y, 1), round(b.box_extent.y, 1),
        "slots", [(str(m.get_editor_property("material_slot_name")), m.get_editor_property("material_interface").get_name() if m.get_editor_property("material_interface") else None)
                  for m in mesh.get_editor_property("materials")])


def main():
    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX False")
    master = master_material()
    mouth = mouth_material()
    skeleton = load(SKEL)
    meshes = {}
    names = sorted(n for n in os.listdir(ART) if os.path.exists(os.path.join(ART, n, f"SK_{n}.fbx")))
    # A character with hair bones first, so the shared skeleton starts with the longest bone list.
    names.sort(key=lambda n: 0 if n == "N19_Pemuda" else 1)
    for name in names:
        folder = os.path.join(ART, name)
        dest = f"{DST}/{name}"
        if lib.does_asset_exist(f"{dest}/SK_{name}") and not os.environ.get("GR_REIMPORT"):
            # Already imported: set GR_REIMPORT=1 to bring in a new export.
            meshes[name] = load(f"{dest}/SK_{name}")
            assign_materials(meshes[name], load(f"{dest}/MI_{name}"), mouth)
            report(name, meshes[name])
            skeleton = skeleton or load(SKEL)
            continue
        tex = import_textures(name, folder, dest)
        mi_path = f"{dest}/MI_{name}"
        mi = load(mi_path) or tools.create_asset(f"MI_{name}", dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, master)
        for role, param in (("basecolor", "Base"), ("normal", "Normal"), ("rm", "RM")):
            if role in tex:
                mel.set_material_instance_texture_parameter_value(mi, param, tex[role])
        lib.save_loaded_asset(mi)

        mesh, paths = import_mesh(name, folder, dest, skeleton)
        if not mesh:
            log("GAGAL", name, paths)
            continue
        if not skeleton:
            # The first import made its own skeleton: rename it to the shared one.
            made = mesh.get_editor_property("skeleton")
            lib.rename_asset(made.get_path_name().split(".")[0], SKEL)
            skeleton = load(SKEL)
        assign_materials(mesh, mi, mouth)
        report(name, mesh)
        meshes[name] = mesh
    lib.save_loaded_asset(skeleton)
    make_retargeter(meshes.get("N07_Aparat_patroli") or next(iter(meshes.values())))


def chain_str(ch):
    bone = lambda key: str(ch.get_editor_property(key).get_editor_property("bone_name"))
    return (str(ch.get_editor_property("chain_name")), bone("start_bone"), bone("end_bone"))


def make_retargeter(preview):
    manny = load(MANNY)
    src = load(f"{DST}/IK_GR_Manny") or tools.create_asset("IK_GR_Manny", DST, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    c = unreal.IKRigController.get_controller(src)
    c.set_skeletal_mesh(manny)
    c.apply_auto_generated_retarget_definition()
    log("MANNY chains", [chain_str(ch) for ch in c.get_retarget_chains()],
        "root", c.get_retarget_root())
    lib.save_loaded_asset(src)

    tgt = load(f"{DST}/IK_GR_Tripo") or tools.create_asset("IK_GR_Tripo", DST, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    t = unreal.IKRigController.get_controller(tgt)
    t.set_skeletal_mesh(preview)
    for ch in list(t.get_retarget_chains()):
        t.remove_retarget_chain(ch.get_editor_property("chain_name"))
    t.set_retarget_root("Hips")
    chains = [
        ("Spine", "Spine", "Spine2"), ("Neck", "Neck", "Neck"), ("Head", "Head", "Head"),
        ("LeftClavicle", "LeftShoulder", "LeftShoulder"), ("LeftArm", "LeftArm", "LeftHand"),
        ("RightClavicle", "RightShoulder", "RightShoulder"), ("RightArm", "RightArm", "RightHand"),
        ("LeftLeg", "LeftUpLeg", "LeftFoot"), ("LeftFoot", "LeftToeBase", "LeftToeBase"),
        ("RightLeg", "RightUpLeg", "RightFoot"), ("RightFoot", "RightToeBase", "RightToeBase"),
    ]
    for name, a, b in chains:
        t.add_retarget_chain(name, a, b, "")
    log("TRIPO chains", [chain_str(ch) for ch in t.get_retarget_chains()])
    lib.save_loaded_asset(tgt)

    rtg = load(f"{DST}/RTG_GR_MannyKeTripo") or tools.create_asset("RTG_GR_MannyKeTripo", DST, unreal.IKRetargeter, unreal.IKRetargetFactory())
    r = unreal.IKRetargeterController.get_controller(rtg)
    r.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, src)
    r.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, tgt)
    r.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE, manny)
    r.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET, preview)
    r.remove_all_ops()
    r.add_default_ops()
    r.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.SOURCE, src)
    r.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.TARGET, tgt)
    r.auto_map_chains(unreal.AutoMapChainType.EXACT, True)
    ops = []
    for i in range(r.get_num_retarget_ops()):
        name = str(r.get_op_name(i))
        # No IK: the Tripo rig has no IK goals, and plain FK keeps crowds cheap. No root motion either: the body is
        # attached to the mannequin, so copying the root's movement would add the actor's position a second time.
        if ("IK" in name and "FK" not in name) or name == "Root Motion":
            r.set_retarget_op_enabled(i, False)
        ops.append((name, r.get_retarget_op_enabled(i)))
    log("OPS", ops)
    try:
        r.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)
    except Exception as e:  # noqa: BLE001
        log("ALIGN gagal", e)
    lib.save_loaded_asset(rtg)


try:
    main()
except Exception as e:  # noqa: BLE001
    import traceback
    log("ERROR", e, traceback.format_exc())
for line in LOG:
    unreal.log_warning("GRCHAR " + line)
