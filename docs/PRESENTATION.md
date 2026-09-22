# Playable diorama presentation

The Unreal adapter displays the blacksmith expedition as a stylized diorama:
town buildings and a lit forge, a regional road with forest and ruins, and a mine
with rock boundaries, timber supports, lamps, crates and visible ore. This is
still replaceable prototype art. It establishes readable places and mouse
interactions before production models and animation are available.

## Ownership and controls

`OdrPresentation.cpp` batches scenery into instanced mesh components and rebuilds
it when the environment or accepted layout changes. Dynamic actors and objective
markers follow snapshots. `OdrHUD.cpp` owns responsive panels, selection,
tooltips and projected health bars; `OdrController.cpp` forwards pointer and
keyboard input. A transparent Slate surface supplies pointer-event coordinates
to the Canvas HUD and consumes handled clicks; the viewport does not capture
the cursor on launch. Losing pointer capture ends camera drags. These components
do not own quest or combat outcomes.

Left-click selects a character or moves to an empty hex. Exploration follows a
queued route through adjacent core commands; a new destination, another command,
Escape, save or load cancels that queue. Battle movement submits one destination
to the core. A teal hover path accounts for walls, occupants, rough terrain and
movement allowance. It is advisory; the portable session validates every move.

Mouse actions use the explicitly selected target. The UI disables actions with
known unmet phase, class, resource, discovery or proximity requirements. Combat
range and line of sight remain authoritative core checks with rejection feedback.
Every living companion must be in the entry zone before retreat is enabled.

Use the wheel to zoom, middle-drag to pan, right-drag to rotate and Home to reset
the view. The UI uses a 1440-by-900 logical canvas scaled to fit the viewport;
camera letterboxing is disabled so pointer and HUD coordinates share the same
viewport. Panel backgrounds consume clicks. Save and Load are always visible.
Only the acting, selected or hovered unit shows a map name plate; compact health
bars leave clustered models visible. Full party names remain in the bottom strip.

For a quick art inspection, launch the Development game with
`-odrpreviewbattle`. It replays the ordinary authored scenario commands until
the first eight-character battle, without loading or writing a save. Omit this
flag for the creation-to-upgrade playtest. Use a separate `-UserDir` if saving
from the preview, as described in [development setup](DEVELOPMENT.md).

## Rebuild the art

Run from the repository root, with `BLENDER` and `UE_ROOT` pointing to installed
tools. Build and import on an APFS checkout on this Mac; see
[development setup](DEVELOPMENT.md) for the external-volume restriction.

```sh
"$BLENDER" --background --factory-startup --python tools/art_probe/build_diorama.py
"$UE_ROOT/Engine/Binaries/Mac/UnrealEditor-Cmd" "$PWD/Unreal/OpenDynamicRPG.uproject" \
  -unattended -nullrhi -nosplash -nosound -EnablePlugins=PythonScriptPlugin \
  -DPCVars=Interchange.FeatureFlags.Import.FBX=0 -ScriptErrorsAreFatal \
  -ExecutePythonScript="$PWD/tools/art_probe/import_diorama.py"
```

The importer creates a parameterized material with explicit skeletal and
instanced-mesh usage, assigns each material slot, and saves skeleton, animation
and meshes. Inspect a rendered packaged build after import: a successful FBX
import alone does not establish correct material assignment or cooking.
`build/reports/diorama-import.json` records the engine and imported paths.
The [source kit](../ArtSource/Diorama/README.md) has its own limitations and license.

## Validation boundary

`ODR.PlayerAdapter` covers pointer destination queuing, cancellation at save,
hex coordinate conversion, battle movement and incompatible explicit targets,
alongside existing save and model-substitution checks. These are adapter tests;
they do not operate the rendered HUD or the macOS mouse. The packaged unattended
scenario checks cooked assets and canonical gameplay state. A separate visible
mouse pass must check button hit areas, deprojection, hover, camera behavior,
readability and the full expedition. See [PLAYTEST.md](PLAYTEST.md).

On this Mac, automated native clicks reported the unchanged system cursor
position instead of the requested synthetic position. Unreal's Mac input path
reads the system cursor, so that automation did not establish a successful
physical mouse pass. Real mouse interaction remains an explicit manual check;
keyboard input and adapter calls are not substitutes for that evidence.

This pass does not establish the 30–45 minute pacing target, production character
quality or a new performance budget. Previous frame measurements describe the
earlier primitive presentation and must not be reused for this art kit.
