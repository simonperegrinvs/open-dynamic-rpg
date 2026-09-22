# Adventure content

`game/content/mine.json` and `quarry.json` are version 2 definitions for the same
expedition pattern. The quarry is exercised headlessly; the current Unreal
entry point selects the mine. Each session selects one template, and each
accepted expedition receives its own run ID.

## Add or change a definition

Start from either definition and keep a stable, unique `template_id`.

- `recruits` contains stable IDs, display names and supported class names.
- `mine` binds entry, exit, clue, hidden loot, main and secret triggers and ore
  to axial hex coordinates. `terrain` supplies inclusive q/r bounds, walls,
  rough hexes and nonempty generated wall/rough candidate lists.
- `encounters.main` and `encounters.secret` contain unique enemy suffixes,
  names, classes, visual IDs, starting positions and combat statistics.
  An optional `boss: true` main record appears only in the lower-floor variant.
- `defaults` supplies participant, location and reward descriptions plus
  `cache_gold`, `ore_quantity`, `main_xp`, `secret_xp` and `secret_gold`.

Validate both bundled definitions with the actual core, then stage them for
Unreal:

```sh
build/dev/odr_tool --definition game/content/mine.json --validate
build/dev/odr_tool --definition game/content/quarry.json --validate
python3 tools/stage_content.py
python3 tools/stage_content.py --check
ctest --preset dev
```

`--definition` also accepts a new file. Add new bundled files to the staging
list and CI validation commands. A syntax-only check is insufficient: add a
scenario that reaches its changed objectives and rewards through ordinary
commands, including a save/reload. Keep tests independent of production art.

## Review a generated run

Use the headless tool interactively with the chosen definition. Create a hero,
then issue `{"action":"create_run","mode":"generated","seed":42}`. Inspect
the printed run's layout, object bindings, enemy records, rewards and generation
diagnostics. Try multiple seeds and both party sizes. Supply an invalid
`candidate_layouts` list to exercise bounded rejection and the authored fallback;
check that the fallback itself passes validation. Never accept a candidate just
because its room graph is connected: objectives, movement and battle deployment
must be physically legal too.

Save the accepted snapshot and load it after changing the source definition's
enemy stats or rewards. Existing run values must remain unchanged; only a newly
accepted run should receive the new values. Runs of one template must retain
independent progress and rewards. The CTest scenarios cover these boundaries.

## Visual replacement and current limits

`visuals.json` uses stable visual IDs, primitive fallbacks and optional cooked
model/material paths. The [original skeletal probe](../ArtSource/BindingProbe/README.md)
demonstrates source-to-engine replacement. Mesh bounds, collisions and animation
never determine a hex, attack outcome or saved state. JSON-referenced art belongs
under `/Game/Art`, which the project explicitly cooks.

The current pattern still fixes the city-to-mine upgrade loop, room roles,
main/secret encounter slots and compact floor structure. Generated variants
choose among supplied obstacle options; they do not synthesize arbitrary quest
graphs or rooms. Character class mechanics remain portable rules. Richer
adventures should extend the data contract with a concrete scenario and an ADR,
not fork the runtime per template.
