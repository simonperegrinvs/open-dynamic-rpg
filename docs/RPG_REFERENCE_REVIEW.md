# AI-assisted 3D RPG reference review

Research date: 2026-09-13. Supports the [game concept](GAME_CONCEPT.md)
and [tooling review](TOOLING_REVIEW.md).

## Assessment

These four projects provide useful examples of AI-assisted game development,
but none establishes a complete foundation for our proposed RPG. Keep Unreal
Engine 5.8 and Blender/Rigify as the working foundation. Study individual
systems before considering an extraction or fork.

| Reference | Most useful to us | Important limit | Recommended use |
|---|---|---|---|
| [Sanctuary's End][s-repo] | Connected dungeon, loot, blacksmith and enchanting loop; compact room graphs and generation tests | Returning to a dungeon rebuilds its layout and encounters; character saving is not expedition saving | First gameplay-system study; possible small algorithm examples |
| [Aetheria][a-repo] | Shared content tables, compact appearance choices and procedural character presentation | Quest vocabulary is kill/collect; the current tower stops at ten floors and its floor recipe does not describe room geometry | Content-schema and character-recipe study |
| [Godotwind][g-repo] | Streaming budgets, world-source adapters and assembling compatible body parts | World/rendering laboratory; incomplete RPG gameplay and Morrowind-specific asset conventions | Architecture and performance study; selective ideas for our Unreal adapter |
| [Embermere RPG][e-repo] | Closest Unreal/Blender/Codex workflow; gameplay/presentation separation; save validation and original creature production | Early prototype, small authored quest ledger, no combat-state saving; no explicit reuse license found | First production-workflow study; reference only until reuse terms are clear |

AI authorship is useful evidence about a development process. It does not by
itself establish maintainability, visual quality, complete mechanics or reliable
saves. Our recommendations below distinguish inspected implementation from
author-reported results.

## Evidence and revisions

Public repository trees and selected text files were retrieved at these exact
commits. All four tree responses were complete. The downloaded files' Git blob
hashes matched those trees. Commit dates are activity observations, not release
dates or promises of maintenance.

| Project | Inspected commit | Commit date, UTC | Source files retrieved |
|---|---|---|---:|
| Sanctuary's End | [9a4265f3e31a][s-revision] | 2026-08-22 | 48 |
| Aetheria | [300c1dc91feb][a-revision] | 2026-07-15 | 42 |
| Godotwind | [e074931e7f22][g-revision] | 2026-07-08 | 15 |
| Embermere RPG | [6ded3fd754c0][e-revision] | 2026-09-13 | 21 |

This is a focused static source review: quest definitions and progression,
dungeon construction, persistence, character assembly, selected tests and asset
validation. Retrieval is not a claim that every line of every file was audited.
No downloaded project code, build or test suite was executed, and no dependency
was installed. Source snapshots remain outside the game repository.

During the preceding shortlist review, Sanctuary's End's public browser demo
loaded its character creator and rendered town. That confirms a working entry
screen and scene on this browser, not a completed dungeon run, tested save
reconstruction or correspondence to the pinned source revision. The other
projects were not run. Upstream test definitions and reported successful runs
are recorded as such, not as our own test results.

### AI provenance and reuse terms

| Project | Evidence of AI-written source | License evidence and boundary |
|---|---|---|
| Sanctuary's End | The [README][s-readme] calls the project vibe-coded; the inspected head is a merge from a Claude-named branch. This does not establish a percentage of AI authorship. | A root [MIT license][s-license] covers the game's own code. [Asset credits][s-credits] and the separate [asset catalogue credits][s-asset-credits] retain third-party terms. |
| Aetheria | The [README][a-readme] explicitly identifies Claude Code and says the owner authored the code with AI assistance. | The README declares MIT, including its procedural assets, but no separately named LICENSE/COPYING file was found in the complete tree. Obtain a conventional license notice before packaging an extracted component for public redistribution. |
| Godotwind | The [README][g-readme] says the author used Opus, Codex and Fable and wrote no code manually. This is the author's statement, not independently verified provenance. | Root [MIT license][g-license]. Morrowind game data and third-party add-ons/assets have separate terms; the sample world is not an original freely redistributable asset library. |
| Embermere RPG | The [README][e-readme] describes Codex as a development partner and documents Unreal and Blender MCP workflows. | Public source, but no LICENSE/COPYING file or explicit general reuse grant was found in the inspected tree and documentation. Do not classify it as a permissively licensed dependency. Raw Fab/Epic packs are excluded and must be obtained separately. |

The inspected dungeon and quest paths use ordinary program logic. Their use of
AI during development does not demonstrate, or require us to adopt, live LLM
generation during play. The agreed rule-based adventure generation remains the
target.

## Sanctuary's End

### Implementation findings

**A real compact layout precedes rendering.** `genDungeonLayout` partitions a
5-by-5 grid into six to nine rooms, connects them with a randomized spanning
tree, adds optional extra doors, and emits rooms, walls, connections and a
descent-portal position. It places that portal in a room farthest from the
entrance by graph distance. The renderer then instantiates reusable architecture
and props. This is a useful small example of separating a place description
from its meshes. It is planar room partitioning, not a general library of
multistorey authored rooms with typed sockets. [Layout code][s-layout],
[dungeon assembly][s-dungeon].

**The equipment services perform distinct operations.** Item definitions and
rules cover affixes, upgrades, sockets, gems and reforging. The smith and
enchanter interfaces operate on selected items and recalculate equipped stats.
These are useful references for making equipment improvement an ongoing part
of the adventure loop. The implementation does not establish our craftspeople's
relationships, personal quests or story-dependent service unlocks.
[Item rules][s-items], [service interfaces][s-panels].

**Generated objectives are bounded event counters.** Floor goals select among
champion kills, ordinary kills, elite kills and shrine use. Progress reports a
completion transition once. These are optional floor challenges, not a director
that binds NPC motives, factions, information and alternative story outcomes.
This distinction matters when discussing "dynamic quests." [Objective code][s-items].

**Encounter difficulty partly matches our direction.** The inspected zone
scaling derives enemy health and damage from dungeon depth or regional level,
plus the chosen difficulty. However, loot item level explicitly includes the
player's level. Do not copy that formula into our location-based reward/threat
policy without a separate design decision. [Scaling code][s-zones].

**Visible weapon replacement is narrower than our equipment-art requirement.**
`attachHeroWeapon` chooses a model from the item's base type, attaches it to a
hand bone and uses a request token to discard an obsolete asynchronous load.
That last detail is useful when equipment changes rapidly. This path does not
establish a complete quality/enchantment-to-material-and-VFX system or a fitted
body wardrobe. [Weapon assembly][s-assets].

### Persistence and quality limits

The save object records the character, inventory, equipment and progression.
`saveProgress` copies current character progress into that object. `enterGame`
starts in town; `enterTown` clears active enemies and loot; `enterDungeon`
generates a new layout, clears the field and places fresh encounters and
interactables. A cached biome's meshes are reused, but that is a rendering cache,
not preservation of the expedition. This directly fails our requirement that
unfinished locations survive town visits and reloads unchanged.
[Save schema][s-save], [save entry point][s-update],
[game entry][s-screens], [zone transitions][s-zones].

The [layout tests][s-layout-tests] check connectivity, portal placement and
bounds across 500 generated floors, then exercise spawn and room-placement
rules. Other inspected tests cover objective completion, character migration,
item identity and reforging. This is a useful validation style. It does not
prove physical navigation after decoration, party movement, narrative
reachability or full world-state saving. Tests were inspected, not run.
[Objective tests][s-objective-tests], [save tests][s-save-tests],
[reforge tests][s-reforge-tests].

The scripts share a global scope and depend on load order, as the
[test harness][s-harness] explains. Extracting a layout function or test idea
is considerably more plausible than using the whole browser game as our core.

**Study outcome:** retain the layout/renderer separation, connectivity checks,
equipment-service loop and protection against stale asset loads. Our adventure
identity, story planning, saved expedition state and Unreal integration still
need their own design.

## Aetheria

### Implementation findings

**Content is centralized and cross-referenced.** Shared TypeScript tables define
quests, items, NPCs, recipes, maps and appearance. Client and server consume the
same vocabulary. The [quest catalogue tests][a-quest-tests] check that chained
quests, monsters and reward/collection items exist. This is a useful starting
pattern for our content catalogue and diagnostics, although valid references
alone do not prove a quest is achievable. [Shared content][a-shared].

**The quest system is intentionally small.** Each definition has one `kill` or
`collect` objective, a giver, turn-in NPC, level requirement, reward and optional
next quest. Active state maps a definition ID to one progress count; completed
state stores definition IDs. That supports authored chains but not two
independent runs of the same template, several simultaneous objectives, or
negotiation/infiltration branches. [Quest definitions][a-quests],
[quest service][a-quest-service].

**The current tower is finite, despite its name.** `DUNGEONS` authors two floors
and generates definitions for floors three through ten. `enterDungeon` also
rejects floor numbers outside that range. A floor definition supplies spawn
kinds/counts and rewards; entry scatters those monsters at random coordinates.
It does not supply a room/door/module graph. The map file separately has seeded
world-obstacle construction, which should not be mistaken for quest-aware
dungeon assembly. [Map and floor definitions][a-maps],
[tower runtime][a-world].

**Appearance is compact and reconstructable at a simple visual level.** Its
appearance object stores body shape, hair/accessory choices and colors.
`HeroModel` uses those parameters to size and position procedural parts and
animate them. Clothing here is principally colored body parts and accessories;
the inspected GLTF hero file is a stub. This demonstrates the recipe idea, not
fitting arbitrary garments to detailed imported bodies or producing portable
skeletal animation. [Appearance schema][a-appearance], [hero model][a-hero],
[GLTF stub][a-gltf].

**Its companions solve a different problem.** The inspected summon handler
removes an owner's existing companion before creating another, while shared
definitions provide attacker, defender and support roles. It does not establish
our hero plus three directly controllable companions, their equipment, party
pathfinding or relationship arcs. [Companion definitions][a-companions],
[summon and companion runtime][a-world].

### Persistence and quality limits

Quest state is saved as part of the character's data, but the inspected tower
state is a session map of floor/clear/reward flags, not a durable resolved
location. Entering a tower floor changes the room's shared map ID and clears its
monster collection. Those ownership boundaries need scrutiny before borrowing
this as an independent adventure-instance design. [World runtime][a-world].

Reward ordering also deserves attention: the quest service applies currency
and XP, awaits item delivery, then removes the active quest and records
completion. There is no in-progress guard in this method. This is a static
concern about overlapping turn-ins or interrupted reward delivery, not a
reproduced runtime failure. The inspected service unit test covers acceptance;
it does not establish the reward transaction's safety.
[Quest service][a-quest-service], [service test][a-service-test].

**Study outcome:** retain shared content tables, cross-reference validation and
small appearance recipes. Do not adopt this quest runtime or tower as our
adventure foundation. Its multiplayer server architecture would also add work
unrelated to our single-player target.

## Godotwind

### Implementation findings

**Streaming has explicit categories and a shared budget.**
`StreamingPublicationBudget` allocates work among nearby gameplay, static
visuals, distant representations, lights and unloading. It tracks claimed,
spent and refunded time and reports overruns. The streaming manager uses this
helper. This is a useful pattern for making generation/loading costs visible
and scheduling scene publication over several frames; a cooperative time budget
does not guarantee a frame rate. [Budget helper][g-budget],
[streaming manager][g-streaming], [budget tests][g-budget-tests].

**There is substantive work on keeping source data behind adapters.**
The architecture describes world-source and object-provider contracts, and
inspected tests exercise delegation and guard several streaming modules against
direct calls to the Morrowind record manager. This is relevant to keeping our
portable adventure description separate from an engine and from game-specific
data. The project still contains Morrowind-specific import, coordinate, asset
and skeleton handling; this is not a ready engine-agnostic package.
[Architecture][g-architecture], [boundary tests][g-boundary-tests].

**Body-part assembly assumes compatible content.** The Morrowind assembler
loads that game's skeleton and binds its body parts, including skin-index
mapping and mirrored limbs. A separate humanoid factory loads GLB/FBX scenes
and merges supplied animation libraries. Neither demonstrates that arbitrary
generated meshes acquire a compatible skeleton, body sliders or fitting clothes
automatically. Its value is the explicit compatibility boundary.
[Morrowind assembler][g-assembler], [humanoid factory][g-humanoid].

### Gameplay and quality limits

The README describes incomplete combat, quests, inventory and saving. The
source gives a more precise picture: a generic quest manager already tracks
journal stages and completion, and an inventory-service interface already
exists. That is useful infrastructure, but it does not establish an integrated,
complete RPG. Correct the earlier shorthand that these systems have no code at
all. [README][g-readme], [quest manager][g-quests],
[inventory interface][g-inventory].

The quest manager keys state by a quest ID; it is not a demonstrated runtime
for multiple instances of the same generated story pattern. Nothing in the
inspected streaming budget proves that quest actors, pursuing enemies or
companions retain authoritative state when their visual cells unload. Our
simulation lifetime must remain a separate requirement.

Morrowind world data is an input to the sample, so this also does not furnish
an original monster or dungeon-art library for our public tools. Upstream
performance figures and screenshots have not been reproduced on our Mac.

**Study outcome:** retain the budgeting, diagnostics and adapter ideas. Use
Unreal's existing rendering/streaming capabilities where suitable rather than
porting this rendering framework wholesale. A small original test world would
be needed before claiming any extracted subsystem works independently.

## Embermere RPG

### Implementation findings

**Presentation is replaceable without moving gameplay ownership into the art.**
The NPC presentation actor configures static/skeletal visuals and contextual
greetings, disables their gameplay collision/navigation effects, and observes
quest state. Separate components own quests, inventory, equipment and services.
This is directly relevant to refining our bodies, outfits and creatures while
their identities and interactions remain stable.
[NPC presentation][e-npc], [presentation contract][e-npc-contract].

**Quest completion preflights its rewards.** The quest component checks whether
required owners, assets, currency/XP capacity and inventory space are valid,
then guards mutation and marks completion before reward callbacks can reenter.
Progress routes through an explicit quest/objective pair. These are concrete
patterns for avoiding duplicate rewards and misrouted progress.
[Quest component][e-quests].

**Save loading validates a candidate before applying it.** The persistence
layer resolves saved asset references, checks matching stable identities and
valid states, then replaces live progression, inventory, equipment, quest and
vendor state. Versions one and two have explicit compatibility paths into the
current version-three ledger. Equipment bonuses are reconstructed rather than
stacked on each load. Tests include actual serialized archives and missing
version handling, not only object-to-object copies.
[Persistence code][e-persistence], [save types][e-save],
[save-version tests][e-save-tests].

**Original creatures have repeatable source and diagnostics.** The Marsh Prowler
builder creates geometry, a quadruped rig and six action clips, then exports
editable Blender source, FBX and metrics. Its checks include bounds, triangle
budget, UV presence, manifold edges, transforms, expected materials, required
bones and action names. That is a useful asset-production workflow to study.
[Creature builder][e-creature], [Blender pipeline][e-pipeline].

### Scope and reuse limits

The current quest data has one objective ID/count per definition, and the
ledger allows up to eight records with unique quest IDs. Independent authored
quests can coexist; two generated instances sharing a template ID still need a
separate identity model. Greetings respond to available/active/ready/completed
state, but this is not our planned branching conversation and world-fact
director. [Quest data][e-quest-data], [quest component][e-quests],
[multi-quest contract][e-quest-contract].

The save contract deliberately excludes position, current vitals, targets,
aggro, cooldowns and temporary effects. Loading resets combat and restores
vitals. It is a progression save, not a paused-combat or persistent-expedition
snapshot. The equipment save records authored item identities and slots, not
unique item instances with individually rolled affixes, upgrade histories and
visual recipes. Both distinctions matter for our design.
[Save contract][e-save-contract], [save types][e-save],
[equipment component][e-equipment].

The creature builder is specific to one authored animal, with explicit shapes,
bone assignments and animation keys. A joined mesh and zero non-manifold edges
do not prove a continuous body, satisfactory deformation, foot contact or good
motion at varied proportions. Its checks do not replace our character-family,
garment-fit and animation tests. We have not run it or judged its output in
Unreal. [Creature construction and checks][e-creature].

The missing general reuse license prevents a dependency recommendation. The
saved level also refers to locally installed Fab/Epic content excluded from
the repository. A public checkout is consequently not evidence of a complete,
freely redistributable game or asset kit. [Repository explanation][e-readme].

**Study outcome:** highest priority for understanding the Unreal/Blender AI
workflow and explicit ownership/validation patterns. Use as reference material;
do not copy or fork it into our public toolkit until reuse terms are established.

## Implications for our tools

These are research recommendations, not newly selected dependencies or changes
to the agreed game concept.

| Tool area | Useful reference pattern | Work our game still requires |
|---|---|---|
| Content catalogue and diagnostics | Aetheria's shared definitions and cross-reference tests | Versioned IDs, world facts, asset/rig compatibility and actionable authoring errors |
| Adventure director and quest adapter | Small event progression in Sanctuary; explicit routing/commit in Embermere | Template ID separate from instance ID, bound participants/places, alternative solutions, NPC knowledge, faction and companion consequences |
| Dungeon recipes and provider interface | Sanctuary's logical room graph preceding mesh placement | Typed composable modules, multiple floors, constraints shared with the quest, navigation after decoration, bounded retries/fallback and stable resolved layout |
| Equipment and services | Sanctuary's separate smith/enchanter operations; Embermere's inventory/equipment ownership | Persistent individual items, treasured-item development, craftspeople's story unlocks, materials/runes/VFX tied to actual item properties |
| Character and creature production | Aetheria's appearance data; Godotwind's compatible-part assembly; Embermere's source/export metrics | Original master bodies, supported morph ranges, fitted wardrobes, rig/animation families, grips, materials and export validation |
| Persistence inspector | Embermere's preflight and archive-version tests; Sanctuary's migration tests | Complete adventure/world changes, unique item state, four characters, paused combat and explicit content/generator migrations |
| Streaming and encounter diagnostics | Godotwind's budgets and source adapters | Keep authoritative party/quest/combat state alive independently of visible rooms; measure on our intended Mac |
| AI authoring interface | Embermere's repeatable build/inspect/export process | Human and AI tools invoke the same validated domain operations and receive saved-state and visual feedback |

The central missing component remains **a director that plans the story and
the place together and preserves the resulting adventure**. Random enemy
placement, a connected map and a kill counter are useful parts; their presence
does not establish coherent generated adventures.

## Recommended follow-up order

1. Use Sanctuary's loop and room-graph tests to refine our small blacksmith/mine
   example: preparation, several routes, meaningful reward and visible equipment
   improvement. Treat its reset-on-entry behavior as a comparison case.
2. Use Embermere's ownership, save-validation and asset-production patterns to
   specify our own interfaces and acceptance evidence. Resolve its license only
   if direct reuse becomes desirable; no maintainer has been contacted.
3. Use Aetheria to make our shared content and appearance schemas concrete,
   adding template/instance separation and a richer action vocabulary from the
   start.
4. Consult Godotwind when defining generation/loading measurements and the
   boundary between game state and visible scene objects. Benchmark our actual
   Unreal scenario before adopting additional streaming infrastructure.

The existing [blacksmith/mine acceptance scenario](TOOLING_REVIEW.md#evaluation-before-a-fork-or-dependency-commitment)
remains the decision test: concurrent generated adventures, several solutions,
stable expedition state, exactly-once rewards, four-character navigation,
paused-combat reload and explicit migration after content changes. No reviewed
project has been shown here to pass that scenario.

## Reproducing or extending this review

Open the pinned source links below, or retrieve the repository tree and raw text
at the recorded commit. Read the code path, its callers and its tests together.
Record newer revisions separately if behavior has changed; do not silently
replace these findings with a moving README or demo. Source snapshots, downloaded
art and generated captures should remain outside this game repository.

Any later hands-on evaluation should record the exact source/engine/tool
versions, required separately obtained assets, setup, player actions, observable
result and limitations. Automated checks should distinguish logical graph
reachability from actual navigation and distinguish a saved character from a
saved world. For assets, inspect the exported moving character in-engine as well
as the Blender source and a still render.

[s-repo]: https://github.com/J3vb/Sanctuarys_End
[s-revision]: https://github.com/J3vb/Sanctuarys_End/commit/9a4265f3e31a8324f3e30da09dec0be3219c9197
[s-readme]: https://github.com/J3vb/Sanctuarys_End/blob/9a4265f3e31a8324f3e30da09dec0be3219c9197/README.md
[s-license]: https://github.com/J3vb/Sanctuarys_End/blob/9a4265f3e31a8324f3e30da09dec0be3219c9197/LICENSE
[s-credits]: https://github.com/J3vb/Sanctuarys_End/blob/9a4265f3e31a8324f3e30da09dec0be3219c9197/CREDITS.md
[s-asset-credits]: https://github.com/J3vb/Sanctuarys_End/blob/9a4265f3e31a8324f3e30da09dec0be3219c9197/assets/CREDITS.md
[s-layout]: https://github.com/J3vb/Sanctuarys_End/blob/9a4265f3e31a8324f3e30da09dec0be3219c9197/js/00-core.js#L67
[s-dungeon]: https://github.com/J3vb/Sanctuarys_End/blob/9a4265f3e31a8324f3e30da09dec0be3219c9197/js/09-dungeon.js#L133
[s-items]: https://github.com/J3vb/Sanctuarys_End/blob/9a4265f3e31a8324f3e30da09dec0be3219c9197/js/01-items.js
[s-panels]: https://github.com/J3vb/Sanctuarys_End/blob/9a4265f3e31a8324f3e30da09dec0be3219c9197/js/19-panels.js
[s-zones]: https://github.com/J3vb/Sanctuarys_End/blob/9a4265f3e31a8324f3e30da09dec0be3219c9197/js/16-zones.js
[s-assets]: https://github.com/J3vb/Sanctuarys_End/blob/9a4265f3e31a8324f3e30da09dec0be3219c9197/js/10-assets.js#L69
[s-save]: https://github.com/J3vb/Sanctuarys_End/blob/9a4265f3e31a8324f3e30da09dec0be3219c9197/js/03-save.js
[s-update]: https://github.com/J3vb/Sanctuarys_End/blob/9a4265f3e31a8324f3e30da09dec0be3219c9197/js/17-update.js#L157
[s-screens]: https://github.com/J3vb/Sanctuarys_End/blob/9a4265f3e31a8324f3e30da09dec0be3219c9197/js/22-screens.js#L3
[s-layout-tests]: https://github.com/J3vb/Sanctuarys_End/blob/9a4265f3e31a8324f3e30da09dec0be3219c9197/tests/layout.test.js
[s-objective-tests]: https://github.com/J3vb/Sanctuarys_End/blob/9a4265f3e31a8324f3e30da09dec0be3219c9197/tests/objectives.test.js
[s-save-tests]: https://github.com/J3vb/Sanctuarys_End/blob/9a4265f3e31a8324f3e30da09dec0be3219c9197/tests/save.test.js
[s-reforge-tests]: https://github.com/J3vb/Sanctuarys_End/blob/9a4265f3e31a8324f3e30da09dec0be3219c9197/tests/reforge.test.js
[s-harness]: https://github.com/J3vb/Sanctuarys_End/blob/9a4265f3e31a8324f3e30da09dec0be3219c9197/tests/harness.js
[a-repo]: https://github.com/mositron/Aetheria
[a-revision]: https://github.com/mositron/Aetheria/commit/300c1dc91feb0f412e42d05886086224aa685b7b
[a-readme]: https://github.com/mositron/Aetheria/blob/300c1dc91feb0f412e42d05886086224aa685b7b/README.md
[a-shared]: https://github.com/mositron/Aetheria/tree/300c1dc91feb0f412e42d05886086224aa685b7b/packages/shared/src
[a-quests]: https://github.com/mositron/Aetheria/blob/300c1dc91feb0f412e42d05886086224aa685b7b/packages/shared/src/quests.ts
[a-quest-tests]: https://github.com/mositron/Aetheria/blob/300c1dc91feb0f412e42d05886086224aa685b7b/packages/shared/src/__tests__/quests.test.ts
[a-quest-service]: https://github.com/mositron/Aetheria/blob/300c1dc91feb0f412e42d05886086224aa685b7b/packages/server/src/services/Quest.ts
[a-service-test]: https://github.com/mositron/Aetheria/blob/300c1dc91feb0f412e42d05886086224aa685b7b/packages/server/src/services/Quest.test.ts
[a-maps]: https://github.com/mositron/Aetheria/blob/300c1dc91feb0f412e42d05886086224aa685b7b/packages/shared/src/maps.ts#L251
[a-world]: https://github.com/mositron/Aetheria/blob/300c1dc91feb0f412e42d05886086224aa685b7b/packages/server/src/rooms/WorldRoom.ts
[a-appearance]: https://github.com/mositron/Aetheria/blob/300c1dc91feb0f412e42d05886086224aa685b7b/packages/shared/src/appearance.ts
[a-hero]: https://github.com/mositron/Aetheria/blob/300c1dc91feb0f412e42d05886086224aa685b7b/packages/client/src/scene/models/HeroModel.tsx
[a-gltf]: https://github.com/mositron/Aetheria/blob/300c1dc91feb0f412e42d05886086224aa685b7b/packages/client/src/scene/models/GLTFHero.tsx
[a-companions]: https://github.com/mositron/Aetheria/blob/300c1dc91feb0f412e42d05886086224aa685b7b/packages/shared/src/companions.ts
[g-repo]: https://github.com/lihogloglo/godotwind
[g-revision]: https://github.com/lihogloglo/godotwind/commit/e074931e7f227d6bdcfa2d02889d1209dc025a33
[g-readme]: https://github.com/lihogloglo/godotwind/blob/e074931e7f227d6bdcfa2d02889d1209dc025a33/README.md
[g-license]: https://github.com/lihogloglo/godotwind/blob/e074931e7f227d6bdcfa2d02889d1209dc025a33/LICENSE
[g-budget]: https://github.com/lihogloglo/godotwind/blob/e074931e7f227d6bdcfa2d02889d1209dc025a33/src/core/world/streaming_publication_budget.gd
[g-streaming]: https://github.com/lihogloglo/godotwind/blob/e074931e7f227d6bdcfa2d02889d1209dc025a33/src/core/world/native_streaming_manager.gd
[g-budget-tests]: https://github.com/lihogloglo/godotwind/blob/e074931e7f227d6bdcfa2d02889d1209dc025a33/tests/unit/test_streaming_publication_budget.gd
[g-architecture]: https://github.com/lihogloglo/godotwind/blob/e074931e7f227d6bdcfa2d02889d1209dc025a33/docs/ARCHITECTURE.md
[g-boundary-tests]: https://github.com/lihogloglo/godotwind/blob/e074931e7f227d6bdcfa2d02889d1209dc025a33/tests/unit/test_streaming_modular_boundaries.gd
[g-assembler]: https://github.com/lihogloglo/godotwind/blob/e074931e7f227d6bdcfa2d02889d1209dc025a33/src/core/character/morrowind/morrowind_npc_assembler.gd
[g-humanoid]: https://github.com/lihogloglo/godotwind/blob/e074931e7f227d6bdcfa2d02889d1209dc025a33/src/core/character/humanoid/humanoid_character_factory.gd
[g-quests]: https://github.com/lihogloglo/godotwind/blob/e074931e7f227d6bdcfa2d02889d1209dc025a33/src/core/dialogue/quest_manager.gd
[g-inventory]: https://github.com/lihogloglo/godotwind/blob/e074931e7f227d6bdcfa2d02889d1209dc025a33/src/core/interaction/inventory_service.gd
[e-repo]: https://github.com/disbitski/embermere-rpg
[e-revision]: https://github.com/disbitski/embermere-rpg/commit/6ded3fd754c0c7b6c3bf2c74b1b94fa0b145066d
[e-readme]: https://github.com/disbitski/embermere-rpg/blob/6ded3fd754c0c7b6c3bf2c74b1b94fa0b145066d/README.md
[e-npc]: https://github.com/disbitski/embermere-rpg/blob/6ded3fd754c0c7b6c3bf2c74b1b94fa0b145066d/Source/Embermere/Private/Characters/EmbermereNpcPresentationActor.cpp
[e-npc-contract]: https://github.com/disbitski/embermere-rpg/blob/6ded3fd754c0c7b6c3bf2c74b1b94fa0b145066d/Docs/NPC_PRESENTATION_CONTRACT.md
[e-quests]: https://github.com/disbitski/embermere-rpg/blob/6ded3fd754c0c7b6c3bf2c74b1b94fa0b145066d/Source/Embermere/Private/Components/EmbermereQuestLogComponent.cpp
[e-quest-data]: https://github.com/disbitski/embermere-rpg/blob/6ded3fd754c0c7b6c3bf2c74b1b94fa0b145066d/Source/Embermere/Public/Data/EmbermereQuestData.h
[e-quest-contract]: https://github.com/disbitski/embermere-rpg/blob/6ded3fd754c0c7b6c3bf2c74b1b94fa0b145066d/Docs/MULTI_QUEST_CONTRACT.md
[e-persistence]: https://github.com/disbitski/embermere-rpg/blob/6ded3fd754c0c7b6c3bf2c74b1b94fa0b145066d/Source/Embermere/Private/Save/EmbermerePersistenceLibrary.cpp
[e-save]: https://github.com/disbitski/embermere-rpg/blob/6ded3fd754c0c7b6c3bf2c74b1b94fa0b145066d/Source/Embermere/Public/Save/EmbermereSaveGame.h
[e-save-contract]: https://github.com/disbitski/embermere-rpg/blob/6ded3fd754c0c7b6c3bf2c74b1b94fa0b145066d/Docs/SAVE_GAME_CONTRACT.md
[e-save-tests]: https://github.com/disbitski/embermere-rpg/blob/6ded3fd754c0c7b6c3bf2c74b1b94fa0b145066d/Source/Embermere/Private/Tests/EmbermereSaveVersionTests.cpp
[e-equipment]: https://github.com/disbitski/embermere-rpg/blob/6ded3fd754c0c7b6c3bf2c74b1b94fa0b145066d/Source/Embermere/Private/Components/EmbermereEquipmentComponent.cpp
[e-creature]: https://github.com/disbitski/embermere-rpg/blob/6ded3fd754c0c7b6c3bf2c74b1b94fa0b145066d/Scripts/blender/build_embermere_marsh_prowler.py
[e-pipeline]: https://github.com/disbitski/embermere-rpg/blob/6ded3fd754c0c7b6c3bf2c74b1b94fa0b145066d/Docs/BLENDER_ASSET_PIPELINE.md
