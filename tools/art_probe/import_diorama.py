"""Import the original diorama kit and construct its shared Unreal materials."""

import json
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve().parent
SOURCE = ROOT / "ArtSource" / "Diorama"
DESTINATION = "/Game/Art/Diorama"


def materials(palette):
    assets = unreal.AssetToolsHelpers.get_asset_tools()
    edit = unreal.MaterialEditingLibrary
    parent = unreal.load_asset(f"{DESTINATION}/M_Diorama")
    if parent is None:
        parent = assets.create_asset(
            "M_Diorama", DESTINATION, unreal.Material, unreal.MaterialFactoryNew()
        )
    edit.delete_all_material_expressions(parent)
    parent.set_editor_property("used_with_instanced_static_meshes", True)
    parent.set_editor_property("used_with_skeletal_mesh", True)
    tint = edit.create_material_expression(
        parent, unreal.MaterialExpressionVectorParameter, -450, 0
    )
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(0.4, 0.4, 0.4, 1))
    edit.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
    for name, default, prop in (
        ("Roughness", 0.8, unreal.MaterialProperty.MP_ROUGHNESS),
        ("Metallic", 0.0, unreal.MaterialProperty.MP_METALLIC),
    ):
        expression = edit.create_material_expression(
            parent, unreal.MaterialExpressionScalarParameter
        )
        expression.set_editor_property("parameter_name", name)
        expression.set_editor_property("default_value", default)
        edit.connect_material_property(expression, "", prop)
    glow = edit.create_material_expression(parent, unreal.MaterialExpressionScalarParameter)
    glow.set_editor_property("parameter_name", "Glow")
    glow.set_editor_property("default_value", 0)
    multiply = edit.create_material_expression(parent, unreal.MaterialExpressionMultiply)
    edit.connect_material_expressions(tint, "", multiply, "A")
    edit.connect_material_expressions(glow, "", multiply, "B")
    edit.connect_material_property(multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    edit.recompile_material(parent)
    unreal.EditorAssetLibrary.save_loaded_asset(parent, only_if_is_dirty=False)
    result = {}
    for name, (color, roughness, metallic) in palette.items():
        instance = unreal.load_asset(f"{DESTINATION}/MI_{name}")
        if instance is None:
            instance = assets.create_asset(
                f"MI_{name}",
                DESTINATION,
                unreal.MaterialInstanceConstant,
                unreal.MaterialInstanceConstantFactoryNew(),
            )
        edit.set_material_instance_parent(instance, parent)
        edit.set_material_instance_vector_parameter_value(
            instance, "Tint", unreal.LinearColor(*color, 1)
        )
        edit.set_material_instance_scalar_parameter_value(instance, "Roughness", roughness)
        edit.set_material_instance_scalar_parameter_value(instance, "Metallic", metallic)
        edit.set_material_instance_scalar_parameter_value(
            instance, "Glow", 3 if name == "Glow" else 0.15 if name == "Crystal" else 0
        )
        unreal.EditorAssetLibrary.save_loaded_asset(instance, only_if_is_dirty=False)
        result[name] = instance
    return result


def main():
    manifest = json.loads((SOURCE / "manifest.json").read_text())
    palette = materials(manifest["palette"])
    imported = []
    for asset in manifest["assets"]:
        name, skeletal = asset["name"], asset["skeletal"]
        options = unreal.FbxImportUI()
        options.import_mesh = True
        options.import_as_skeletal = skeletal
        options.import_animations = skeletal
        options.mesh_type_to_import = (
            unreal.FBXImportType.FBXIT_SKELETAL_MESH
            if skeletal
            else unreal.FBXImportType.FBXIT_STATIC_MESH
        )
        options.automated_import_should_detect_type = False
        options.create_physics_asset = False
        options.import_materials = False
        options.import_textures = False
        if not skeletal:
            options.static_mesh_import_data.combine_meshes = True
            options.static_mesh_import_data.auto_generate_collision = False
        task = unreal.AssetImportTask()
        task.filename = str(SOURCE / f"{name}.fbx")
        task.destination_path = DESTINATION
        task.destination_name = name
        task.automated = True
        task.replace_existing = True
        task.save = True
        task.options = options
        task.factory = unreal.FbxFactory()
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        mesh = unreal.load_asset(f"{DESTINATION}/{name}")
        if mesh is None:
            raise RuntimeError(f"Missing imported mesh: {name}")
        slots = mesh.get_editor_property("materials" if skeletal else "static_materials")
        assigned = []
        for slot in slots:
            slot_name = str(slot.get_editor_property("imported_material_slot_name"))
            if slot_name not in palette:
                raise RuntimeError(f"Unknown material {slot_name} on {name}")
            slot.set_editor_property("material_interface", palette[slot_name])
            assigned.append(slot)
        mesh.set_editor_property("materials" if skeletal else "static_materials", assigned)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
        if skeletal:
            for path in (f"{name}_Skeleton", f"{name}_Anim"):
                dependency = unreal.load_asset(f"{DESTINATION}/{path}")
                if dependency is None:
                    raise RuntimeError(f"Missing {path}")
                unreal.EditorAssetLibrary.save_loaded_asset(dependency, only_if_is_dirty=False)
        imported.append(mesh.get_path_name())
    report = ROOT / "build/reports/diorama-import.json"
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(
        json.dumps(
            {"assets": imported, "engine": unreal.SystemLibrary.get_engine_version()}, indent=2
        )
        + "\n"
    )
    unreal.log("ODR_DIORAMA_IMPORT_PASS")


if __name__ == "__main__":
    main()
