# Open Dynamic RPG

Working repository for an original single-player fantasy RPG built around
meaningful character growth, equipment-driven combat, persistent companions and
rule-based generated adventures.

The intended game combines the legible rules and equipment importance of a
tabletop-inspired RPG with a persistent world and repeatable expeditions. Enemy
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

- Single-player party of the hero and up to three active companions.
- Tactical real-time combat with pause and direct control of any party member.
- Equipment, character rules and preparation materially determine outcomes.
- No automatic enemy scaling to the hero's level.
- Persistent world consequences and independently identified adventure runs.
- Compact, composable dungeon descriptions whose resolved layouts survive
  travel, saves and generator updates.
- Authored and generated adventures use the same data, rules and validation.
- Development tools should be free to use and suitable for open-source reuse.

## Licensing

Original documentation and future original project code in this repository are
licensed under the [MIT License](LICENSE), unless a file says otherwise.
Third-party engines, plugins, assets and referenced projects retain their own
licenses. Research links and technical discussion do not incorporate those
projects' code or assets into this repository.

