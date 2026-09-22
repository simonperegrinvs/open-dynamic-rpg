# Original diorama stand-ins

The kit contains fifteen static props and one armored humanoid with a two-bone
idle animation. It is original procedural geometry under the repository's
[MIT license](../../LICENSE), with no downloaded art, textures or add-ons.

[build_diorama.py](../../tools/art_probe/build_diorama.py) is the reproducible
source. Each `.blend` remains editable; each `.fbx` is the engine interchange
asset. `manifest.json` records Blender's version, the palette and polygon counts.
Unreal copies live in `Unreal/Content/Art/Diorama` and are built by
[import_diorama.py](../../tools/art_probe/import_diorama.py).

The character is a readable equipment silhouette, not a production body or
wardrobe system. Classes share its body and weapons; cloak colors distinguish
roles. Ancestry shapes, proper locomotion, attack animation and garment fitting
remain future art work. The earlier binding probe remains available separately.

Source dimensions use meters; FBX imports in centimeters. Character feet and
prop bases are at zero height. The hex tile matches the adapter's 145 cm axial
spacing. Meshes have no gameplay collision: footprints, obstacles, movement,
visibility and interactions come from the accepted portable layout.

See [the presentation guide](../../docs/PRESENTATION.md) for rebuilding and the
boundary between this kit and gameplay authority.
