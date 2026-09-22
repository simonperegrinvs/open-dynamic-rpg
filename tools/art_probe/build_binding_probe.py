"""Build the original two-bone Unreal binding probe with Blender in batch mode."""

import json
import sys
from pathlib import Path

import bpy

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "ArtSource" / "BindingProbe"


def parse_output() -> Path:
    """Return the optional output directory passed after Blender's ``--`` marker."""
    if "--" not in sys.argv:
        return OUTPUT
    arguments = sys.argv[sys.argv.index("--") + 1 :]
    if "--output" not in arguments:
        return OUTPUT
    index = arguments.index("--output") + 1
    if index >= len(arguments):
        raise SystemExit("--output requires a directory")
    return Path(arguments[index]).resolve()


def make_material() -> bpy.types.Material:
    material = bpy.data.materials.new("BindingProbe_OriginalMaterial")
    material.diffuse_color = (0.08, 0.42, 0.85, 1.0)
    return material


def make_mesh() -> bpy.types.Mesh:
    """Create two simple cuboid segments with a visible hinge seam."""
    half_width = 0.16
    lower_bottom = 0.0
    lower_top = 0.58
    upper_bottom = 0.62
    upper_top = 1.2
    vertices = []
    for bottom, top in ((lower_bottom, lower_top), (upper_bottom, upper_top)):
        vertices.extend(
            [
                (-half_width, -half_width, bottom),
                (half_width, -half_width, bottom),
                (half_width, half_width, bottom),
                (-half_width, half_width, bottom),
                (-half_width, -half_width, top),
                (half_width, -half_width, top),
                (half_width, half_width, top),
                (-half_width, half_width, top),
            ]
        )
    faces = []
    for offset in (0, 8):
        faces.extend(
            [
                (offset + 0, offset + 1, offset + 2, offset + 3),
                (offset + 4, offset + 7, offset + 6, offset + 5),
                (offset + 0, offset + 4, offset + 5, offset + 1),
                (offset + 1, offset + 5, offset + 6, offset + 2),
                (offset + 2, offset + 6, offset + 7, offset + 3),
                (offset + 4, offset + 0, offset + 3, offset + 7),
            ]
        )
    mesh = bpy.data.meshes.new("BindingProbe_Mesh")
    mesh.from_pydata(vertices, [], [tuple(reversed(face)) for face in faces])
    mesh.update()
    uv_layer = mesh.uv_layers.new(name="UVMap")
    for polygon in mesh.polygons:
        for loop_index, uv in zip(
            polygon.loop_indices, ((0, 0), (1, 0), (1, 1), (0, 1)), strict=True
        ):
            uv_layer.data[loop_index].uv = uv
    return mesh


def make_armature() -> bpy.types.Object:
    data = bpy.data.armatures.new("BindingProbe_Armature")
    armature = bpy.data.objects.new("BindingProbe_Armature", data)
    bpy.context.collection.objects.link(armature)
    bpy.context.view_layer.objects.active = armature
    armature.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    root = data.edit_bones.new("root")
    root.head = (0.0, 0.0, 0.0)
    root.tail = (0.0, 0.0, 0.6)
    tip = data.edit_bones.new("tip")
    tip.head = root.tail
    tip.tail = (0.0, 0.0, 1.2)
    tip.parent = root
    tip.use_connect = True
    bpy.ops.object.mode_set(mode="OBJECT")
    return armature


def add_animation(armature: bpy.types.Object) -> bpy.types.Action:
    bpy.context.view_layer.objects.active = armature
    armature.select_set(True)
    bpy.ops.object.mode_set(mode="POSE")
    tip = armature.pose.bones["tip"]
    tip.rotation_mode = "XYZ"
    for frame, angle in ((1, 0.0), (20, 0.7), (40, -0.35)):
        tip.rotation_euler[1] = angle
        tip.keyframe_insert(data_path="rotation_euler", frame=frame, group="tip")
    bpy.ops.object.mode_set(mode="OBJECT")
    action = armature.animation_data.action
    if action is None:
        raise RuntimeError("animation action was not created")
    action.name = "BindingProbe_TipSwing"
    return action


def build(output: Path) -> dict[str, object]:
    output.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    scene.frame_start = 1
    scene.frame_end = 40
    scene.render.fps = 30

    armature = make_armature()
    mesh = make_mesh()
    marker = bpy.data.objects.new("BindingProbe_Marker", mesh)
    bpy.context.collection.objects.link(marker)
    marker.data.materials.append(make_material())
    marker.parent = armature
    marker.parent_type = "OBJECT"
    modifier = marker.modifiers.new("BindingProbe_ArmatureDeform", "ARMATURE")
    modifier.object = armature
    root_group = marker.vertex_groups.new(name="root")
    tip_group = marker.vertex_groups.new(name="tip")
    root_group.add(list(range(8)), 1.0, "REPLACE")
    tip_group.add(list(range(8, 16)), 1.0, "REPLACE")
    action = add_animation(armature)
    scene.frame_set(1)

    blend_path = output / "BindingProbe.blend"
    fbx_path = output / "BindingProbe.fbx"
    report_path = output / "binding_probe_report.json"
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))
    bpy.ops.object.select_all(action="SELECT")
    bpy.context.view_layer.objects.active = armature
    bpy.ops.export_scene.fbx(
        filepath=str(fbx_path),
        use_selection=False,
        object_types={"ARMATURE", "MESH"},
        add_leaf_bones=False,
        primary_bone_axis="Y",
        secondary_bone_axis="X",
        armature_nodetype="NULL",
        mesh_smooth_type="FACE",
        bake_anim=True,
        bake_anim_use_all_actions=False,
        bake_anim_use_nla_strips=False,
        bake_anim_simplify_factor=0.0,
    )

    weights = []
    for vertex in mesh.vertices:
        weights.append(
            {
                "index": vertex.index,
                "groups": [
                    {"name": marker.vertex_groups[group.group].name, "weight": group.weight}
                    for group in vertex.groups
                ],
            }
        )
    report = {
        "asset": "BindingProbe",
        "purpose": "original two-bone skeletal binding integration probe",
        "blender_version": bpy.app.version_string,
        "source_of_truth": "tools/art_probe/build_binding_probe.py",
        "blend": str(blend_path.relative_to(ROOT)),
        "fbx": str(fbx_path.relative_to(ROOT)),
        "mesh": {"name": mesh.name, "vertices": len(mesh.vertices), "polygons": len(mesh.polygons)},
        "bones": [bone.name for bone in armature.data.bones],
        "vertex_groups": [group.name for group in marker.vertex_groups],
        "weights": weights,
        "animation": {
            "action": action.name,
            "frame_start": scene.frame_start,
            "frame_end": scene.frame_end,
            "fcurves": len(
                action.layers[0].strips[0].channelbag(armature.animation_data.action_slot).fcurves
            ),
        },
        "limits": [
            "This proves source mesh, weights, skeleton and FBX export only.",
            "Unreal import, cooking, retargeting and runtime animation need separate checks.",
            "The marker is original integration-test geometry, not a production character asset.",
        ],
    }
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report


if __name__ == "__main__":
    result = build(parse_output())
    print(json.dumps(result, indent=2))
    bpy.ops.wm.quit_blender()
