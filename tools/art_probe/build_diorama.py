"""Rebuild our original, deliberately replaceable diorama kit in Blender.

Units are meters in the source and centimeters after FBX import. No downloaded
art, textures or add-ons are used. The assembled adventurer is a presentation
stand-in, not the production character/wardrobe foundation.
"""

import json
import math
import random
from pathlib import Path

import bpy

ROOT = Path(__file__).resolve().parents[2]
OUTPUT = ROOT / "ArtSource" / "Diorama"
PALETTE = {
    "Stone": ((0.27, 0.32, 0.36), 0.92, 0),
    "StoneLight": ((0.46, 0.49, 0.47), 0.88, 0),
    "Rock": ((0.17, 0.23, 0.28), 0.95, 0),
    "Road": ((0.36, 0.32, 0.25), 0.96, 0),
    "Dirt": ((0.19, 0.15, 0.11), 1, 0),
    "Grass": ((0.19, 0.29, 0.21), 0.96, 0),
    "Leaf": ((0.12, 0.25, 0.20), 0.9, 0),
    "Wood": ((0.22, 0.11, 0.055), 0.85, 0),
    "WoodLight": ((0.45, 0.28, 0.12), 0.83, 0),
    "Iron": ((0.23, 0.30, 0.34), 0.34, 0.75),
    "Copper": ((0.72, 0.43, 0.14), 0.33, 0.65),
    "Cloth": ((0.06, 0.36, 0.42), 0.9, 0),
    "Skin": ((0.66, 0.43, 0.28), 0.85, 0),
    "Hair": ((0.085, 0.045, 0.026), 0.95, 0),
    "Glow": ((1.0, 0.30, 0.025), 0.6, 0),
    "Crystal": ((0.15, 0.75, 0.85), 0.23, 0.35),
    "Plaster": ((0.70, 0.63, 0.44), 0.98, 0),
    "Roof": ((0.22, 0.30, 0.34), 0.8, 0.15),
    "Coal": ((0.018, 0.026, 0.033), 1, 0),
}
PARTS = []
MATERIALS = {}


def finish(obj, material, bone="root"):
    obj.data.materials.append(MATERIALS[material])
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    group = obj.vertex_groups.new(name=bone)
    group.add(list(range(len(obj.data.vertices))), 1, "REPLACE")
    PARTS.append(obj)
    return obj


def box(location, scale, material, rotation=(0, 0, 0), bevel=0.025, bone="root"):
    bpy.ops.mesh.primitive_cube_add(size=1, location=location, rotation=rotation)
    obj = bpy.context.object
    obj.scale = scale
    finish(obj, material, bone)
    if bevel:
        modifier = obj.modifiers.new("Soft worn edges", "BEVEL")
        modifier.width = bevel
        modifier.segments = 1
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    return obj


def cone(location, radius, depth, material, top=0, vertices=8, rotation=(0, 0, 0), bone="root"):
    bpy.ops.mesh.primitive_cone_add(
        vertices=vertices,
        radius1=radius,
        radius2=top,
        depth=depth,
        location=location,
        rotation=rotation,
    )
    return finish(bpy.context.object, material, bone)


def rock(location, scale, material="Rock", seed=0):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, radius=1, location=location)
    obj = bpy.context.object
    rng = random.Random(seed)
    for vertex in obj.data.vertices:
        vertex.co *= rng.uniform(0.8, 1.15)
    obj.scale = scale
    return finish(obj, material)


def join(name):
    bpy.ops.object.select_all(action="DESELECT")
    for part in PARTS:
        part.select_set(True)
    bpy.context.view_layer.objects.active = PARTS[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    bpy.context.scene.cursor.location = (0, 0, 0)
    bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    PARTS.clear()
    return obj


def export(name, builder, skeletal=False):
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    builder()
    obj = join(name)
    armature = None
    if skeletal:
        data = bpy.data.armatures.new("AdventurerRig")
        armature = bpy.data.objects.new("AdventurerRig", data)
        bpy.context.collection.objects.link(armature)
        bpy.context.view_layer.objects.active = armature
        armature.select_set(True)
        bpy.ops.object.mode_set(mode="EDIT")
        root = data.edit_bones.new("root")
        root.head, root.tail = (0, 0, 0), (0, 0, 0.62)
        body = data.edit_bones.new("body")
        body.head, body.tail, body.parent = (0, 0, 0.62), (0, 0, 1.2), root
        bpy.ops.object.mode_set(mode="OBJECT")
        obj.parent = armature
        modifier = obj.modifiers.new("Adventurer deform", "ARMATURE")
        modifier.object = armature
        body = armature.pose.bones["body"]
        body.rotation_mode = "XYZ"
        for frame, angle in ((1, 0), (31, 0.018), (61, 0)):
            body.rotation_euler[0] = angle
            body.keyframe_insert(data_path="rotation_euler", frame=frame)
        armature.animation_data.action.name = "Adventurer_Breathe"
    bpy.context.scene.frame_set(1)
    bpy.ops.wm.save_as_mainfile(filepath=str(OUTPUT / f"{name}.blend"), compress=True)
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.export_scene.fbx(
        filepath=str(OUTPUT / f"{name}.fbx"),
        use_selection=True,
        object_types={"ARMATURE", "MESH"},
        add_leaf_bones=False,
        armature_nodetype="NULL",
        mesh_smooth_type="FACE",
        bake_anim=skeletal,
        bake_anim_use_all_actions=False,
        bake_anim_use_nla_strips=False,
    )
    return {"name": name, "skeletal": skeletal, "polygons": len(obj.data.polygons)}


def hex_tile():
    cone((0, 0, -0.08), 0.837, 0.16, "Stone", top=0.837, vertices=6)
    cone((0, 0, 0.005), 0.816, 0.012, "StoneLight", top=0.816, vertices=6)


def rock_cluster():
    for index, (loc, scale) in enumerate(
        [
            ((0, 0, 0.5), (0.6, 0.5, 0.75)),
            ((0.4, 0.1, 0.2), (0.4, 0.36, 0.42)),
            ((-0.4, -0.12, 0.18), (0.4, 0.38, 0.3)),
        ]
    ):
        rock(loc, scale, seed=index)


def crystal():
    rock((0, 0, 0.13), (0.55, 0.43, 0.26))
    for i in range(5):
        angle = i * 2.4
        x, y = math.cos(angle) * 0.25, math.sin(angle) * 0.21
        cone(
            (x, y, 0.4),
            0.10,
            0.65 + i * 0.045,
            "Crystal",
            vertices=5,
            rotation=(math.sin(angle) * 0.3, math.cos(angle) * 0.3, angle),
        )


def timber():
    for x in (-0.62, 0.62):
        box((x, 0, 0.85), (0.18, 0.23, 1.7), "Wood")
        for z in (0.25, 1.4):
            box((x, -0.01, z), (0.205, 0.255, 0.075), "Iron", bevel=0.008)
    box((0, 0, 1.7), (1.58, 0.3, 0.22), "WoodLight")
    for x, tilt in ((-0.46, -0.7), (0.46, 0.7)):
        box((x, 0, 1.45), (0.14, 0.16, 0.55), "Wood", rotation=(0, tilt, 0))


def torch():
    box((0, 0, 0.55), (0.12, 0.12, 1.1), "Wood")
    cone((0, 0, 1.08), 0.18, 0.2, "Iron", top=0.25)
    cone((0, 0, 1.29), 0.17, 0.36, "Glow", vertices=7)
    rock((0, 0, 1.22), (0.13, 0.13, 0.21), "Glow")


def crate():
    box((0, 0, 0.3), (0.65, 0.6, 0.6), "Wood")
    for z in (0.07, 0.52):
        for y in (-0.315, 0.315):
            box((0, y, z), (0.7, 0.055, 0.1), "WoodLight", bevel=0.005)
    for x in (-0.28, 0.28):
        box((x, -0.32, 0.3), (0.075, 0.05, 0.6), "WoodLight", bevel=0.005)
    box((0, -0.345, 0.3), (0.08, 0.05, 0.68), "WoodLight", rotation=(0, 0.8, 0))


def barrel():
    cone((0, 0, 0.34), 0.28, 0.64, "WoodLight", top=0.28, vertices=12)
    for z in (0.08, 0.32, 0.6):
        cone((0, 0, z), 0.295, 0.055, "Iron", top=0.295, vertices=12)
    for a in range(8):
        angle = a * math.pi / 4
        box(
            (math.cos(angle) * 0.273, math.sin(angle) * 0.273, 0.34),
            (0.015, 0.018, 0.57),
            "Wood",
            bevel=0,
        )


def house():
    box((0, 0, 0.12), (3.3, 2.6, 0.24), "Stone")
    box((0, 0, 1.08), (3, 2.3, 1.9), "Plaster")
    for x in (-1.42, 0, 1.42):
        box((x, -1.18, 1.1), (0.15, 0.13, 2), "Wood")
    for z in (0.38, 1.65):
        box((0, -1.19, z), (3, 0.15, 0.14), "Wood")
    for side in (-1, 1):
        box((0, side * 0.69, 2.1), (3.6, 1.75, 0.16), "Roof", rotation=(side * -0.48, 0, 0))
        for index in range(10):
            box(
                (-1.65 + index * 0.365, side * 0.69, 2.18),
                (0.025, 1.74, 0.025),
                "Iron",
                rotation=(side * -0.48, 0, 0),
                bevel=0,
            )
    box((0.95, 0.3, 2.45), (0.45, 0.48, 1.2), "Stone")
    box((0, -1.27, 0.76), (0.65, 0.13, 1.35), "WoodLight")
    for x in (-0.9, 0.9):
        box((x, -1.265, 1.13), (0.48, 0.055, 0.55), "Glow")
        box((x, -1.3, 1.13), (0.04, 0.06, 0.58), "Wood")
        box((x, -1.3, 1.13), (0.52, 0.06, 0.04), "Wood")


def forge():
    for x in (-0.58, 0.58):
        box((x, 0, 0.62), (0.34, 1.25, 1.24), "Stone")
    box((0, 0.25, 1.16), (1.5, 1, 0.3), "StoneLight")
    box((0, 0.28, 1.9), (0.66, 0.65, 1.3), "Stone")
    box((0, 0, 0.35), (0.85, 0.85, 0.28), "Coal")
    for i in range(7):
        rock(((i % 3 - 1) * 0.22, (i // 3 - 1) * 0.23, 0.5), (0.13, 0.14, 0.14), "Glow", i)
    cone((0.7, -1, 0.28), 0.35, 0.56, "Wood", top=0.31)
    box((0.7, -1, 0.68), (0.3, 0.35, 0.35), "Iron")
    box((0.7, -1, 0.87), (0.8, 0.42, 0.13), "Iron")
    cone((1.17, -1, 0.87), 0.21, 0.38, "Iron", top=0.035, rotation=(0, math.pi / 2, 0))
    box((0.78, -1.05, 1), (0.12, 0.34, 0.12), "Iron")
    box((0.65, -1.05, 1), (0.38, 0.065, 0.065), "WoodLight")


def gate():
    for x in (-1.3, 1.3):
        box((x, 0, 1.45), (0.9, 1.2, 2.9), "Stone")
        for offset in (-0.3, 0.3):
            box((x + offset, 0, 3.05), (0.25, 1.25, 0.35), "StoneLight")
    box((0, 0, 2.52), (2.3, 0.9, 0.42), "StoneLight")
    for x in (-0.88, 0.88):
        box((x, -0.63, 1.15), (0.13, 0.08, 2.3), "Wood")


def tree():
    cone((0, 0, 0.85), 0.16, 1.7, "Wood", top=0.09)
    for z, radius in ((1.1, 0.8), (1.6, 0.66), (2.1, 0.44)):
        cone((0, 0, z), radius, 1.15, "Leaf", vertices=9)


def ruins():
    for x in (-0.55, 0.55):
        box((x, 0, 0.8), (0.35, 0.6, 1.6), "StoneLight")
        box((x, 0, 1.65), (0.5, 0.72, 0.18), "Stone")
    box((-0.2, 0, 1.82), (1.05, 0.67, 0.25), "StoneLight", rotation=(0, 0.12, 0))
    rock((0.48, -0.4, 0.15), (0.4, 0.35, 0.25), "StoneLight")


def chest():
    box((0, 0, 0.25), (0.78, 0.49, 0.48), "Wood")
    box((0, 0, 0.53), (0.82, 0.53, 0.16), "WoodLight")
    for x in (-0.3, 0.3):
        box((x, 0, 0.56), (0.065, 0.56, 0.09), "Copper")
        box((x, -0.255, 0.28), (0.065, 0.045, 0.51), "Copper")
    box((0, -0.285, 0.42), (0.13, 0.07, 0.16), "Copper")


def stairs():
    for step in range(5):
        box((0, step * 0.22, step * 0.10 + 0.05), (0.9, 0.24, (step + 1) * 0.1), "StoneLight")


def banner():
    box((0, 0, 0.82), (0.06, 0.06, 1.64), "WoodLight")
    box((0.24, 0, 1.35), (0.48, 0.035, 0.58), "Cloth", bevel=0)
    box((0.24, -0.025, 1.35), (0.09, 0.035, 0.35), "Copper", bevel=0)


def adventurer():
    for side in (-1, 1):
        box((side * 0.12, -0.065, 0.09), (0.19, 0.34, 0.18), "Wood", bevel=0.04)
        box((side * 0.12, 0, 0.33), (0.17, 0.19, 0.40), "Coal", bevel=0.04)
        box((side * 0.12, -0.1, 0.43), (0.19, 0.09, 0.18), "Iron", bevel=0.04)
        box((side * 0.29, 0, 0.74), (0.16, 0.2, 0.39), "Cloth", bone="body")
        box((side * 0.31, -0.04, 0.59), (0.18, 0.22, 0.16), "Iron", bone="body")
        box((side * 0.31, -0.04, 0.51), (0.12, 0.14, 0.13), "Skin", bone="body")
        box((side * 0.26, 0, 0.9), (0.25, 0.28, 0.15), "Iron", bevel=0.045, bone="body")
    box((0, 0, 0.74), (0.4, 0.28, 0.43), "Cloth", bevel=0.05, bone="body")
    box((0, -0.12, 0.80), (0.34, 0.09, 0.28), "Iron", bevel=0.04, bone="body")
    box((0, 0, 0.56), (0.44, 0.31, 0.065), "WoodLight", bone="body")
    box((0, -0.17, 0.56), (0.09, 0.04, 0.09), "Copper", bone="body")
    box((0, 0.19, 0.68), (0.46, 0.04, 0.55), "Cloth", rotation=(0.15, 0, 0), bone="body")
    box((0, 0, 1.04), (0.255, 0.245, 0.275), "Skin", bevel=0.045, bone="body")
    box((0, 0.015, 1.165), (0.28, 0.27, 0.1), "Hair", bevel=0.035, bone="body")
    for side in (-1, 1):
        box((side * 0.07, -0.125, 1.055), (0.032, 0.015, 0.022), "Coal", bevel=0.004, bone="body")
    box((0, -0.139, 1.025), (0.05, 0.045, 0.06), "Skin", bevel=0.01, bone="body")
    box((-0.36, -0.14, 0.68), (0.065, 0.40, 0.45), "Iron", bevel=0.05, bone="body")
    box((-0.40, -0.14, 0.68), (0.04, 0.30, 0.33), "Cloth", bevel=0.04, bone="body")
    box((0.32, -0.06, 0.91), (0.05, 0.055, 0.6), "Iron", bone="body")
    box((0.32, -0.06, 0.62), (0.25, 0.07, 0.05), "Copper", bone="body")


def main():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.frame_start, scene.frame_end = 1, 61
    scene.render.fps = 30
    for name, (color, roughness, metallic) in PALETTE.items():
        material = bpy.data.materials.new(name)
        material.diffuse_color = (*color, 1)
        material.roughness = roughness
        material.metallic = metallic
        MATERIALS[name] = material
    builders = {
        "SM_HexTile": hex_tile,
        "SM_RockCluster": rock_cluster,
        "SM_Crystal": crystal,
        "SM_Timber": timber,
        "SM_Torch": torch,
        "SM_Crate": crate,
        "SM_Barrel": barrel,
        "SM_House": house,
        "SM_Forge": forge,
        "SM_Gate": gate,
        "SM_Tree": tree,
        "SM_Ruins": ruins,
        "SM_Chest": chest,
        "SM_Stairs": stairs,
        "SM_Banner": banner,
    }
    assets = [export(name, builder) for name, builder in builders.items()]
    assets.append(export("SK_Adventurer", adventurer, skeletal=True))
    (OUTPUT / "manifest.json").write_text(
        json.dumps(
            {
                "blender": bpy.app.version_string,
                "license": "MIT",
                "palette": PALETTE,
                "assets": assets,
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )
    print("ODR_DIORAMA_SOURCE_PASS")


if __name__ == "__main__":
    main()
