# Open Dynamic RPG

Working repository for an original single-player fantasy RPG combining
Heroes-inspired city screens and a compact, freely navigable overworld,
mysterious dungeon exploration inspired by Neverwinter Nights, Diablo I and Skyrim, and
Shining Force II-inspired turn-based tactical battles.

Prepare the party through direct city services, discover destinations on the
regional map with alternative places to explore and revisit, explore connected
dungeon spaces for clues and loot, and fight selected major encounters.
Usually each floor has one main battle; additional battles
are optional secret content with special rewards. A possible lowest-floor boss
occupies that floor's main battle slot.

The game retains meaningful character growth, equipment, persistent companions
and a persistent world with repeatable expedition templates. Enemy
strength belongs to places and dungeon depths instead of following the hero's
level. Generated adventures are assembled from designed story patterns, world
facts and validated physical locations.

## Current status

The first blacksmith/mine slice is implemented as a portable C++20 session,
headless scenario runner and Unreal 5.8 placeholder host. Its authored loop
reaches a lasting equipment upgrade; generated variants use the same runtime.
The project remains a prototype, with playtime and performance targets awaiting
measurement. Blender and Rigify remain the portable character-art foundation.

Start with [development setup and checks](docs/DEVELOPMENT.md). The quickest
portable verification is `cmake --preset dev`, `cmake --build --preset dev`,
then `ctest --preset dev`. Unreal requires staged content and the portable
`ue-release` library first; the development guide includes the Mac commands.

## Documents

- [Game concept](docs/GAME_CONCEPT.md) — agreed player experience, combat,
  companions, progression, equipment services, generated adventures and
  dynamic-dungeon requirements.
- [Tooling review](docs/TOOLING_REVIEW.md) — engine decision, free and
  open-source candidates, public-tool boundaries, animation and character
  production.
- [AI-assisted RPG reference review](docs/RPG_REFERENCE_REVIEW.md) — pinned
  source findings from Sanctuary's End, Aetheria, Godotwind and Embermere RPG.
- [Development flow](docs/DEVELOPMENT.md) — exact checks, content and save
  workflow, local Unreal use, runner setup and Luna/Sol review handoff.
- [Tool and license inventory](docs/TOOLS.md) — pinned versions and boundaries.
- [Architecture decisions](docs/adr/0001-authoritative-session.md) — the
  authoritative simulation and related decisions.

## Guiding constraints

- A single-player party of individual characters; evaluate more than four
  simultaneously active members. Active-party and recruitable-roster limits
  remain open.
- Turn-based tactical combat with direct control of each active character.
- A compact open overworld with alternative destinations and discoverable places,
  not a fixed itinerary or a full-scale 3D outdoor world.
- Direct city services; progression does not require
  walking around towns or repeatedly checking NPC conversations.
- Connected dungeon exploration, one main battle per floor as the usual pattern,
  optional secret bonus encounters and a possible lowest-floor boss.
- Normal progression remains viable without discovering secret rewards.
- Equipment, character rules and preparation materially determine outcomes.
- No automatic enemy scaling to the hero's level.
- Persistent world consequences and independently identified adventure runs.
- Compact, composable dungeon descriptions whose resolved layouts survive
  travel, saves and generator updates.
- City/map, party, quest and tactical battle state persist independently of
  loaded 3D presentation, including saves during an expedition or battle.
- Authored and generated adventures use the same data, rules and validation.
- Development tools should be free to use and suitable for open-source reuse.

## Licensing

Original documentation and future original project code in this repository are
licensed under the [MIT License](LICENSE), unless a file says otherwise.
Third-party engines, plugins, assets and referenced projects retain their own
licenses. Research links and technical discussion do not incorporate those
projects' code or assets into this repository.
