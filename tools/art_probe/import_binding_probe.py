"""Import the original BindingProbe FBX into Unreal's skeletal mesh pipeline."""

import json
from pathlib import Path

import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve().parent
FBX = ROOT / "ArtSource" / "BindingProbe" / "BindingProbe.fbx"
DESTINATION = "/Game/Art/BindingProbe"


def main() -> None:
    report = ROOT / "build" / "reports" / "binding-probe-import.json"
    report.unlink(missing_ok=True)
    if not FBX.is_file():
        raise RuntimeError(f"Missing source FBX: {FBX}")
    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = True
    options.import_animations = True
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    options.create_physics_asset = False
    options.automated_import_should_detect_type = False
    options.import_materials = False
    options.import_textures = False
    options.skeletal_mesh_import_data.convert_scene_unit = True

    task = unreal.AssetImportTask()
    task.filename = str(FBX)
    task.destination_path = DESTINATION
    task.destination_name = "SK_BindingProbe"
    task.automated = True
    task.replace_existing = True
    task.save = True
    task.options = options
    task.factory = unreal.FbxFactory()
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    imported = task.get_objects()
    meshes = [asset for asset in imported if isinstance(asset, unreal.SkeletalMesh)]
    if len(meshes) != 1:
        raise RuntimeError("Unreal must import exactly one BindingProbe skeletal mesh")
    expected_path = f"{DESTINATION}/SK_BindingProbe.SK_BindingProbe"
    if meshes[0].get_path_name() != expected_path:
        raise RuntimeError(f"Unexpected imported asset: {meshes[0].get_path_name()}")
    bounds = meshes[0].get_bounds()
    height = bounds.box_extent.z * 2
    if not 119 <= height <= 121:
        raise RuntimeError(f"Expected a 120 cm skeletal marker, got {height} cm")
    skeleton = meshes[0].get_editor_property("skeleton")
    animation = unreal.load_asset(f"{DESTINATION}/SK_BindingProbe_Anim")
    if skeleton is None or not isinstance(animation, unreal.AnimSequence):
        raise RuntimeError("Imported probe needs both a skeleton and an animation sequence")
    for asset in (skeleton, animation, meshes[0]):
        if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
            raise RuntimeError(f"Could not save imported asset: {asset.get_path_name()}")
    report.parent.mkdir(parents=True, exist_ok=True)
    report.write_text(
        json.dumps(
            {
                "mesh": expected_path,
                "height_cm": height,
                "skeleton": skeleton.get_path_name(),
                "animation": animation.get_path_name(),
                "physics_asset": None,
                "engine": unreal.SystemLibrary.get_engine_version(),
            },
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )
    unreal.log(f"ODR_BINDING_IMPORT_PASS {expected_path}")
    print(f"Imported {len(imported)} asset(s) under {DESTINATION}")


if __name__ == "__main__":
    main()
