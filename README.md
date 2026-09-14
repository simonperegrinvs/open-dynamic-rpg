# Open Dynamic RPG

Working repository for an original single-player fantasy RPG combining
Heroes-inspired city screens and adventure-map travel, mysterious dungeon
exploration inspired by Neverwinter Nights, Diablo I and Skyrim, and
Shining Force II-inspired turn-based tactical battles.

Prepare the party through direct city services, discover destinations on the
map, explore connected dungeon spaces for clues and loot, and fight selected
major encounters. Usually each floor has one main battle; additional battles
are optional secret content with special rewards. A possible lowest-floor boss
occupies that floor's main battle slot.

The game retains meaningful character growth, equipment, persistent companions
and a persistent world with repeatable expedition templates. Enemy
strength belongs to places and dungeon depths instead of following the hero's
level. Generated adventures are assembled from designed story patterns, world
facts and validated physical locations.

## Current status

This project is in concept and technical research. No playable game or reusable
toolkit has been implemented yet. Unreal Engine 5.8 is the working game-engine
choice, with Blender and Rigify for portable character and creature authoring.

## Documents

- [Game concept](docs/GAME_CONCEPT.md) — agreed player experience, combat,
  companions, progression, equipment services, generated adventures and
  dynamic-dungeon requirements.
- [Tooling review](docs/TOOLING_REVIEW.md) — engine decision, free and
  open-source candidates, public-tool boundaries, animation and character
  production.
- [AI-assisted RPG reference review](docs/RPG_REFERENCE_REVIEW.md) — pinned
  source findings from Sanctuary's End, Aetheria, Godotwind and Embermere RPG.

## Guiding constraints

- A single-player party of individual characters; evaluate more than four
  simultaneously active members. Active-party and recruitable-roster limits
  remain open.
- Turn-based tactical combat with direct control of each active character.
- Direct city services and map-based travel; progression does not require
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
