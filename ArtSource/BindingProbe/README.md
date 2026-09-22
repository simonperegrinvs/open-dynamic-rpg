# BindingProbe

This original two-bone marker proves the Blender-to-Unreal skeletal asset
handoff. It is integration-test geometry, not a production character. The
source, script and exported assets use the repository [MIT license](../../LICENSE).
No art from another game or external asset collection is included.

[build_binding_probe.py](../../tools/art_probe/build_binding_probe.py) creates
two weighted cuboid segments, root and tip bones, UVs and a 40-frame action
at 30 fps. The source uses meters; the upright mesh is 1.2 m tall. Every vertex
is weighted to one bone. The .blend remains editable; .fbx is the interchange
file. The script is the reproducible source of truth.

## Reproduce

Use Blender **5.2.1 LTS**, build **9e2066aef7ef**. Set BLENDER_BIN to that
installation's executable, then run from the repository root:

    BLENDER_USER_CONFIG=/private/tmp/odr-binding-probe-config \
    BLENDER_USER_SCRIPTS=/private/tmp/odr-binding-probe-scripts \
    "$BLENDER_BIN" --background --factory-startup --python-exit-code 1 \
      --python tools/art_probe/build_binding_probe.py -- \
      --output ArtSource/BindingProbe

The outputs are BindingProbe.blend, BindingProbe.fbx and
binding_probe_report.json. The report records the source mesh, bones, weights
and animation. Blender's Mac startup needs access to the graphics runtime even
in background mode; a restricted run on this Mac crashed before executing Python.
An APFS copy of the installed Blender completed the export with that access.

Import from an APFS project checkout using Unreal 5.8.2. The recipe selects the
FBX importer explicitly and saves all three assets, including the skeleton and
animation packages. Python errors must fail the process:

    UE_ROOT=/Volumes/UE_5_8_APFS
    "$UE_ROOT/Engine/Binaries/Mac/UnrealEditor-Cmd" \
      "$PWD/Unreal/OpenDynamicRPG.uproject" \
      -unattended -nullrhi -nosplash -nosound -EnablePlugins=PythonScriptPlugin \
      -DPCVars=Interchange.FeatureFlags.Import.FBX=0 -ScriptErrorsAreFatal \
      -ExecutePythonScript="$PWD/tools/art_probe/import_binding_probe.py" \
      -stdout -FullStdOutLogOutput

The script checks the mesh path, skeleton, animation and 120 cm bounds, then
writes build/reports/binding-probe-import.json. The imported assets are
SK_BindingProbe, SK_BindingProbe_Skeleton and SK_BindingProbe_Anim under
Unreal/Content/Art/BindingProbe/. No physics asset is generated: gameplay
geometry belongs to the portable hex rules.

## Validation boundary

Export and import passed locally on 2026-09-22. Imported height measured
120.00001 cm. The hero's visual binding selects the skeletal mesh;
ODR.PlayerAdapter requires it to load, substitutes primitives, and compares
canonical state before and after rebuilding the presentation. Packaged smoke
also requires the skeletal marker to load, preventing an uncooked model from
silently passing through its primitive fallback.

The exported animation is imported as a sequence, but is not bound to runtime
playback. This probe does not validate production Rigify rigs, retargeting,
attachments or animation quality. Blender/Rigify remains the direction for
production character authoring; those contracts need representative characters.
