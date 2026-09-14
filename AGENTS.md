# Project instructions

Read `docs/GAME_CONCEPT.md` and `docs/TOOLING_REVIEW.md` before changing the
game architecture or selecting dependencies. Use
`docs/RPG_REFERENCE_REVIEW.md` as study evidence, not as proof that a referenced
project is suitable for reuse.

The current working engine is Unreal Engine 5.8. Blender and Rigify are the
portable source foundation for original characters, creatures and animation.
Additional production tools must be free to use. New general-purpose adventure,
dungeon and character-production tools should be designed for open-source reuse.

Preserve these boundaries:

- authored and generated adventures use the same validated data and runtime;
- a story template and each concrete adventure run have separate stable IDs;
- quest requirements and dungeon constraints are planned and validated together;
- save the accepted resolved layout and subsequent changes, not only a seed;
- authoritative party, combat, quest and world state must not depend on whether
  its current 3D presentation is loaded;
- runtime story generation assembles designed rules and patterns and does not
  depend on a live language model;
- game-specific content, portable core tools, engine adapters and third-party
  dependencies retain clear ownership and license boundaries.

Use the blacksmith/mine scenario in `docs/TOOLING_REVIEW.md` as the first
end-to-end acceptance slice. Record implementation decisions and measured
limitations in the nearest focused document. Do not present proposed checks,
upstream claims or inspected tests as locally verified results.

