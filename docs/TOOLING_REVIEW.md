# Open-source tooling and engine review

Research and working engine decision recorded: 2026-09-13.

Gameplay alignment updated: 2026-09-14. Current requirements below follow the
city-screen, adventure-map, connected-dungeon and turn-based battle direction in
[the game concept](GAME_CONCEPT.md#gameplay-reference-mix). Source inspections,
dependency findings and their dates remain historical research evidence;
this alignment is not a new integration test or dependency selection.

This review supports the [game concept](GAME_CONCEPT.md). The project intends
new tools to be reusable open-source projects. The requirement that production
tools be usable for free remains in force. Unreal Engine 5.8 is the agreed
working engine. This review has not made a purchase, public fork or installation.

The adventure-tool assessment combines official documentation with inspection
of six public source snapshots, including runtime identity, persistence,
extension points, test definitions and recent commits. Those projects were not
compiled or run. Character-customization candidates below were reviewed through
their documentation; they were not tested here. The separate graphics lab has
its own trials and installed tools. Existing tests and stated compatibility are
evidence to investigate, not tests passed by our game. Source snapshots were
kept outside this repository.

## Agreed working foundation

**Use Unreal Engine 5.8 for the game.** Its integrated animation and graphics
workflow fits the selected detailed, stylized fantasy presentation, original
creatures, companions and visibly more powerful equipment. Reproducing
those systems would divert substantial effort from our adventure tools.

| Responsibility | Working foundation |
|---|---|
| Gameplay, rendering, animation playback and world assembly | Unreal Engine 5.8 |
| Editable creature models, rigs, materials and authored animation | Blender with Rigify and Blender's animation/baking tools |
| Adventure generation, quest/dungeon coordination, validation and persistence | Reusable open-source tools extending suitable existing projects, with core rules and shared data separate from Unreal integration |

This is the working engine decision, with practical validation still required.
Before large-scale production, check an original creature and a generated
dungeon on the intended Mac, including animation quality, exploration movement,
tactical deployment and battle readability at recorded candidate party sizes,
mid-battle saving and performance. Include a larger-than-four party trial;
the final active limit is open. Keep the established biped/nonhumanoid
coverage checks as the creature workflow develops.

Unreal is free upfront for game development, with the standard 5% royalty on
qualifying product revenue above the first $1 million, subject to its terms and
exclusions. See [Epic's licensing terms](https://www.unrealengine.com/license).
The cost requirement for additional tools remains unchanged.

## Remaining dependency choices

For the quest foundation, evaluate **SUQS** first as a compact, maintained
runtime with a test suite and readable data. Compare **SimpleQuest** when a
ready-made visual editor, expressive prerequisites and live inspection would
justify a larger dependency with a changing API. These are alternatives, not
two systems proposed for simultaneous use.

For the revised game, their value is stateful objectives, concise contextual
choices and consequences shared across city services, map travel and dungeons.
A dialogue editor's breadth alone is not evidence of fit. Neither quest runtime
supplies the tactical combat rules or the adventure-map experience.

For dungeons, **BenPyton/ProceduralDungeon** is a credible open-source candidate
for the narrower task of assembling authored 3D rooms. It is substantially more
than a random-room demonstration, but does not establish equivalence with
Dungeon Architect's full toolset. Its experimental saving, quest constraints
and licensing need explicit evaluation.

Room assembly must now be assessed against exploration passages, a main battle
space per floor, optional secret encounter/reward spaces, and an optional
lowest-floor boss occupying the main encounter role. The inspected provider
does not establish legal tactical deployment, turn-based movement, encounter
roles or larger-party capacity for our game. Treat these as requirements to
validate or implement after the rules are defined.

Retain Godot as a reference alternative. QuestSystem 2 offers a small foundation;
Nexus Quest Weaver supplies more authoring features but needs changes to runtime
identity for repeated concurrent adventures. SimpleDungeons supplies useful
room assembly with more integration work remaining.

## Inspected foundations

The commit dates below are repository activity observations, not compatibility
certification or a promise of continued maintenance.

| Project | License inspected | Revision and latest commit date | Assessment |
|---|---|---|---|
| [SUQS](https://github.com/sinbad/SUQS) | MIT | `76b9d603e2b0`, 2026-09-07 | Strong candidate for a compact Unreal runtime. Branching, task progression, JSON/data-table definitions, save data, migration hooks and focused tests. No visual graph editor or world-aware procedural adventure planner. |
| [SimpleQuest](https://github.com/TheGeebus/SimpleQuest) | MIT | `354d8a200ef0`, 2026-08-28 | Stronger ready-made Unreal authoring workflow: graph editor, named outcomes, prerequisite composition, event/fact infrastructure, save snapshots and inspection. Pre-1.0 API; independent generated instances require investigation and likely extension. |
| [ProceduralDungeon](https://github.com/BenPyton/ProceduralDungeon) | CeCILL-C for GitHub source | `14e4f45c4d43`, 2026-08-18; release 3.9.2 | Authored room assembly, typed doors, graph queries, loops, visibility control and extension points. Mac appears in the platform allowlist. Save/load is still documented as Experimental. |
| [QuestSystem 2](https://github.com/shomykohai/quest-system) | MIT | `d1d933c7b182`, 2026-08-05; version 2.0.2 | Small Godot resource/pool/API foundation. Dedicated unit tests and a CI matrix including Godot 4.7.1. More quest authoring and generation would be our work. |
| [Nexus Quest Weaver](https://github.com/undomick/godot_nexus_quest_weaver) | MIT | `21936d4f043d`, 2026-03-19 | Richer Godot graph tooling and separated definition/state objects. Current pools identify state by template file, limiting concurrent reuse. Inspected CI launches headless Godot; comparable behavioral coverage was not established. |
| [SimpleDungeons](https://github.com/majikayogames/SimpleDungeons) | CC0 | `15b32406e2f1`, 2025-04-02 | Smaller Godot room-assembly foundation, with seeds and bounded generation stages. Latest inspected change fixes room-count enforcement. Quest-aware constraints and durable expedition state remain substantial additions. |

### Quest runtime identity is a required extension

Generated adventures need both a stable template identity and a separate
identity for each run. Two commissions based on the same pattern must keep
different participants, locations, timers, progress and rewards. Changing or
ending one must not affect the other.

Nexus currently looks up an existing instance by file ID before creating one,
and a pool entry with that same ID replaces the previous entry. This is a
specific limitation observed in the
[controller](https://github.com/undomick/godot_nexus_quest_weaver/blob/21936d4f043d5bb626c68dffdf75b7b5a514ee31/addons/quest_weaver/core/quest_controller.gd#L1314)
and [pool](https://github.com/undomick/godot_nexus_quest_weaver/blob/21936d4f043d5bb626c68dffdf75b7b5a514ee31/addons/quest_weaver/core/pools/base_quest_pool.gd#L8).
It would require changes across routing and saving, not just a second constructor
call. Its broad claims about save stability also do not establish compatibility
with arbitrary changes to active quest logic.

SUQS stores active quests by quest ID. Its parameter facility primarily formats
text; it does not establish independently instantiated procedural adventures.
It exposes JSON-to-data-table construction, but a generated definition's
registration, identity and lifetime must be designed alongside its saved state.
Its [migration documentation](https://github.com/sinbad/SUQS/blob/76b9d603e2b05c2a4ffca55b98d151a698043e03/docs/ChangingQuestDefinitions.md)
explicitly describes changes that lose data and a preload migration hook.

SimpleQuest's manager indexes loaded nodes by tags, and its snapshot references
compiled graphs, tags and per-placement GUIDs. Those mechanisms support authored
content and reuse within graphs, but do not prove arbitrary simultaneous runtime
instances of the same adventure. Check isolation before adopting it. Its
[save snapshot](https://github.com/TheGeebus/SimpleQuest/blob/354d8a200ef03cb841d0d50dd827226d18898a57/Plugins/SimpleQuest/Source/SimpleQuest/Public/Quests/Types/SimpleQuestSaveSnapshot.h)
is versioned and includes objective state; that is useful groundwork.

### Scope and maintenance considerations

SUQS has focused tests for branching, dependencies, events, serialization,
parameters and timing. Its smaller runtime is attractive if our main work is a
new adventure-generation layer rather than a large visual authoring suite.

SimpleQuest supplies more editor functionality and test definitions, but its
[contribution policy](https://github.com/TheGeebus/SimpleQuest/blob/354d8a200ef03cb841d0d50dd827226d18898a57/CONTRIBUTING.md)
states that the API is not frozen. Multiplayer and Gameplay Ability System
integration are reserved for a planned paid module. The existing core remains
MIT; the author explicitly permits independent forks or companion plugins for
those features. Our free workflow must not depend on the planned paid module.
General fixes can be candidates for upstream contribution; no maintainer has
been contacted during this review.

ProceduralDungeon includes graph/room tests and a serialization-interface test.
Those do not constitute our full expedition save/reload acceptance test. Its
changelog records save compatibility changes. Our format must preserve resolved
layout and gameplay changes under versioned content references, even if a
provider's native save format changes. The paid Fab minimap is separate from the
open-source room-assembly functionality and is not a proposed dependency.

The alternative [shun126/DungeonGenerator](https://github.com/shun126/DungeonGenerator)
has a GPL-licensed public plugin and additional paid features. It is not a
recommended Unreal dependency: Epic's EULA expressly lists GPL as incompatible.
A permissively licensed demo does not change the plugin's license.

## Public tool boundaries

The following packaging is proposed, not implemented:

1. **Adventure definitions and generation.** Describe world facts, participants,
   supported actions, prerequisites, outcomes, rewards and physical requirements.
   Assemble designed story patterns into concrete adventures. The game supplies
   its factions, characters, equipment rules and canon through data and adapters.
   Express city-service opportunities, map destinations, dungeon discoveries and
   main/secret/boss encounter roles through the same state and stable references.
2. **Quest runtime and authoring adapter.** Translate the shared adventure into
   the selected engine's quest runtime and expose its state. Extend an existing
   foundation where practical, keeping upstream fixes distinct from new features.
3. **Dungeon provider interface.** Pass required room roles, connections,
   interaction placements and route constraints to a provider; receive a compact
   resolved layout with stable identifiers. Begin with one provider. A later
   paid-provider adapter should not become mandatory for the public toolkit.
   Validate battle-space capacity and movement geometry for the tested party
   sizes, plus optional secret access and normal progress without secret rewards.
4. **Validation and persistence.** Check physical and logical reachability,
   supported alternative solutions and world consistency. Persist each adventure
   and its consequences; prevent duplicated rewards and accidental rerolls.
   Version templates, instances, layouts and save migrations explicitly.
   Include map discovery/travel state and tactical turn state. Current actor,
   order, spent actions, positions, effects, objectives and random state must
   survive independently of animation or whether a room is currently rendered.
5. **Authoring and automation.** Provide documented operations for editors,
   command-line use and MCP: inspect a definition, generate a candidate, explain
   a failed constraint, preview changes, run a validation batch and save accepted
   content. Human and AI authoring should call the same validated operations.

Keep the shared data contract independent of Unreal types where practical.
SUQS, SimpleQuest and ProceduralDungeon themselves are Unreal plugins; calling
our data portable does not make their code portable. Support Unreal first
and add another adapter only after the contract works in a complete example.

Use a small, original example project with redistributable assets to demonstrate
the tools independently of this RPG. Stable interfaces, actionable diagnostics,
tests, versioned releases and migration examples are part of the public tool
scope. Exact repository names and programming-language choices remain open.

MIT is a reasonable proposed license for wholly original general-purpose code.
Forks retain their upstream obligations. ProceduralDungeon's GitHub license
allows redistribution and requires preserving its terms and notices and making
modified plugin source available; see the
[publisher's license explanation](https://benpyton.github.io/ProceduralDungeon/3.8/guides/Copyrights-and-Licenses)
and [license text](https://github.com/BenPyton/ProceduralDungeon/blob/14e4f45c4d43d07e9309580daef37c178e84bb40/LICENSE-CeCILL-C).
It cannot simply be relabeled MIT.

Unreal is source-available under Epic's license, not a permissively licensed
open-source engine. Keep Epic code and marketplace content out of a permissive
public core. Distribution of any package that includes Epic-defined Engine
Tools needs to follow the applicable channels and conditions in
[Epic's EULA, sections 5 and 6](https://www.unrealengine.com/eula/unreal).
Finalize that packaging before public release; a public repository elsewhere is
not by itself proof of compliant redistribution.

## Animation and current Unreal version

The working engine version is UE5.8. Epic currently
targets UE6 Early Access for the end of 2027; the visible development stream is
not presented as an alpha ready for adoption. Build the evaluation around
released functionality, as described in
[Epic's UE6 roadmap](https://www.unrealengine.com/news/the-road-to-ue-6).

Unreal's advantage is the integrated workflow:

- [Control Rig](https://dev.epicgames.com/documentation/en-us/unreal-engine/control-rig-in-unreal-engine)
  for rig controls and procedural adjustments.
- [IK Rig and retargeting](https://dev.epicgames.com/documentation/en-us/unreal-engine/ik-rig-animation-retargeting-in-unreal-engine)
  for mapping supported movement between suitable skeletons and proportions.
- Animation Blueprints, blending and
  [Motion Matching](https://dev.epicgames.com/documentation/en-us/unreal-engine/motion-matching-in-unreal-engine)
  for selecting and combining authored motion during gameplay.

These capabilities support the working engine decision; no measured
cross-engine comparison has been completed. Motion Matching still needs a
suitable animation database.
Retargeting does not create a convincing motion vocabulary for arbitrary monster
anatomy. Keep the agreed Blender/Rigify production path and assess complete
biped and nonhumanoid action sets, equipment contacts and party readability.
New Beta or Experimental animation features should be evaluated separately from
the established tools.

## Character customization and compatible assets

Recorded on 2026-09-13 following the user's character-model experiments in
[Evaluate improved graphics direction](codex://threads/01a09426-7423-7141-85bf-f3d3da6f7d42).
The requirement is to adjust pre-existing bodies, clothes and textures while
preserving character identity and reliable animation. The
[concept's authoring requirements](GAME_CONCEPT.md#adjustable-characters-and-compatible-wardrobes)
describe compatible families and versioned character recipes. No additional
plugin has been selected or installed by this review.

### Free tools to evaluate

| Tool | Capability | Compatibility, cost and evaluation limit |
|---|---|---|
| [Blender shape keys](https://docs.blender.org/manual/en/3.2/animation/shape_keys/introduction.html), modeling and rigging tools | Edit an existing mesh and author reusable proportion changes, then fit garments and joints to those changes. | Free foundation. Shape keys store vertex positions; the desired changes must be authored against stable topology. A finished imported model does not acquire meaningful body sliders automatically. |
| [CharMorph](https://github.com/Upliner/CharMorph) | Body morphs, real-time asset fitting, skin/eye controls and Rigify support. Custom character definitions and alternative-topology mapping can adapt prepared existing bodies. | Free open-source Blender add-on; promising candidate to extend for our own bases. Body morphs, joint placement and garment compatibility need setup. Compatibility with our Blender version and models has not been tested. Asset licenses vary. |
| [MPFB 2 / MakeHuman](https://extensions.blender.org/add-ons/mpfb/) | Parametric human bodies, rig choices, materials and compatible clothing assets. | Free open-source Blender add-on with CC0 core assets. Its modeling system depends on a compatible MakeHuman base; arbitrary imported meshes do not inherit its parameters. Check additional community assets individually. |
| [Ucupaint](https://github.com/ucupumar/ucupaint) | Layered texture editing and painting inside Blender, with baking for export. | Free GPL-licensed add-on. Useful for existing models with suitable UVs and materials; it does not supply anatomy or garment construction. Export/bake the textures and recreate suitable Unreal materials rather than assuming Blender shader graphs transfer. |
| [Unreal Mutable](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/PluginIndex/Mutable) | Customizable meshes, materials and textures in the editor or during play, assembled from supplied content. | Included with Unreal under its terms, with no separate plugin purchase. Epic marks it Beta. Evaluate it as the Unreal assembly layer for prepared bodies and wardrobes; it does not replace their authoring or guarantee fit and motion. |
| [MetaHuman 5.8](https://dev.epicgames.com/documentation/metahuman/metahuman-5-8-release-notes-in-unreal-engine) | Conform an existing human head and body into a rigged MetaHuman; customize texture/material overrides. | Included under Unreal licensing, subject to its conditions. The result adopts MetaHuman topology and rig. Treat preservation of our stylized identity as a visual test; this is a human-character option, not a general creature foundation. |

CharMorph's
[custom-character workflow](https://blendercharacterproject.org/docs/html/Custom%20Characters.html)
explicitly starts from an existing body and describes authoring morphs,
configuring the character, and defining how joints follow changed proportions.
Its [alternative-topology workflow](https://blendercharacterproject.org/docs/html/Creating%20A%20Character.html)
maps character changes onto a different mesh structure. These are useful
extension points, not evidence that arbitrary dressed/generated characters
convert without preparation. The custom-character tutorial references Blender
4.0.2; verify compatibility with the working Blender version before adoption.

Distinguish the tool's license from the assets it supplies. CharMorph's
[asset-license chart](https://blendercharacterproject.org/docs/html/Introduction.html)
lists CC-BY, CC0 and AGPL content depending on the base. Prefer our own assets
or an appropriate CC0 base for the portable example; do not assume all bundled
characters share permissive terms. MPFB's
[license](https://github.com/makehumancommunity/mpfb2/blob/master/LICENSE.md)
distinguishes GPL program code from CC0 core assets and exported artwork.
An exported asset and a fork of its authoring add-on have different obligations.

Mutable's [official example](https://www.unrealengine.com/news/the-mutable-sample-project-is-now-available)
demonstrates appearance customization using clothing, hair, makeup and other
variants. Keep editable masters and shared recipe data in the portable
authoring layer, with Mutable-specific assets and operations in the Unreal
adapter. Check output quality, character rebuild cost and animation/cloth
behavior on the intended Mac before committing to it.

MetaHuman is a separate comparison route. Its
[current licensing page](https://www.metahuman.com/en-US/license)
permits use with other engines/software under the applicable Unreal terms; this
does not make its assets a permissively licensed public-tool dependency. Current
[hardware requirements](https://dev.epicgames.com/documentation/metahuman/metahuman-hardware-requirements-in-unreal-engine?lang=en-US)
include Mac Creator support with feature-specific limits. UE 5.8's facial
animation tools support Mac, while markerless body capture remains Windows-only.

### Proposed trial and reusable tool scope

**Recommendation to evaluate:** Blender/Rigify plus a small CharMorph trial and
Ucupaint for surface layers; then assess Mutable for the Unreal assembly layer.
MPFB is an alternative when its base suits the character, and MetaHuman is an
optional human-character comparison. These recommendations do not replace the
agreed authoring foundation or establish an accepted production result.

The graphics lab's MPFB/assembled-body attempt was rejected for fit and motion
problems. This is evidence about that construction, not a blanket verdict on
MPFB. Carry forward those findings: preserve a coherent approved body and
build compatible garments, rather than repeatedly fitting unrelated parts.
The lab's source is its `CHARACTER_PRODUCTION_PIPELINE.md`, under
`build/character-lab-worktree/docs/remaster/`, particularly its adjustable-family
section. That local lab location is context, not a distributable dependency.

The proposed first proof is deliberately small:

1. Choose one visually approved body. Confirm complete underlying body/head
   surfaces, stable UVs and a suitable rig before adding customization.
2. Add a few useful proportion controls and two compatible outfits. Adjust
   joints and clothing with the body; preserve openings, thickness and layering.
3. Make material variants without replacing the mesh or losing character
   identity. Inspect body-only and dressed front, side and quarter views.
4. Check walking, shoulder movement, weapon grip and cloth contact at supported
   proportion extremes. A correct still pose or automatic rig is insufficient.
5. Save and rebuild the same character recipe, then export it into Unreal and
   check appearance and motion against the Blender master.

Record the base/rig versions, body/face parameters, hair and wardrobe IDs,
equipment/grip profiles, materials, animation overrides and export settings.
A recipe editor, supported-combination rules and fit/export diagnostics are a
plausible reusable open-source contribution. Extend a suitable foundation first;
do not assume a complete new character generator is necessary. Keep original
general-purpose code, upstream add-on modifications, game-specific art and the
Unreal adapter distinct, retaining each dependency's license obligations.

## MCP and AI-assisted development

All three engines have viable MCP routes. MCP availability alone does not select
the engine, and a connection is not proof that an agent can correctly author
every asset type. Costs below concern the bridge, not a model provider or hosted
service. None of these bridges was installed or exercised during this review.

| Engine | Current MCP route | Implication |
|---|---|---|
| Unreal 5.8 | Official included MCP plugin, labeled Experimental; Epic documents exposure to Blueprints, assets, levels, materials and meshes, with extensions supported | Strong integration opportunity alongside animation tooling. Verify the actual operations needed for creature, dungeon and quest authoring and their feedback on Mac. |
| Unity 6+ | Official MCP server in the AI package, currently beta. Unity's FAQ explicitly says it needs no AI-tools subscription and consumes no Unity credits. The community CoplayDev bridge is also free and MIT | Strong editor automation options. The separate paid in-editor assistant must not be confused with the free MCP bridge. Unity Personal eligibility still applies. |
| Godot | Community MCP servers, including Coding-Solo/godot-mcp (MIT); IvanMurzak's Godot-MCP is another community option listed by Godot, using Apache-2.0 and the .NET editor | Direct project/scene operations and execution feedback are available. Capabilities differ by bridge; an Asset Library listing does not make a plugin first-party. |

Sources: [UE5.8 announcement](https://www.unrealengine.com/news/unreal-engine-5-8-is-now-available),
[Unity MCP introduction](https://unity.com/blog/unity-ai-mcp-how-to-get-started),
[Unity access and pricing FAQ](https://unity.com/features/ai),
[CoplayDev bridge](https://github.com/CoplayDev/unity-mcp),
[Coding-Solo bridge](https://github.com/Coding-Solo/godot-mcp),
[Godot community listing](https://godotengine.org/asset-library/asset/5245).

For AI-assisted development, prioritize readable structured data, explicit
operations, automation tests, useful errors and access to actual editor/game
results. Use engine APIs to edit engine assets and verify the saved result.
Expose our domain operations through MCP so an assistant can validate a quest
and its dungeon, instead of merely creating objects in a scene. MCP is a
development interface; the agreed rule-based runtime generation does not depend
on a live language model.

## Evaluation before a fork or dependency commitment

The separate [AI-assisted 3D RPG reference review](RPG_REFERENCE_REVIEW.md)
records pinned source findings for Sanctuary's End, Aetheria, Godotwind and
Embermere RPG. Use Sanctuary for the equipment/service loop and compact room
graphs, Aetheria for shared content and appearance data, Godotwind for streaming
budgets and adapters, and Embermere for Unreal/Blender production and validated
state changes. These are study references, not newly selected dependencies.

None establishes the complete adventure foundation. Sanctuary reconstructs
dungeons on entry; Aetheria's current tower is limited to ten floors and its
quests use kill/collect counters; Godotwind's RPG gameplay is incomplete;
Embermere saves progression but explicitly excludes combat state and has no
clear general reuse license. Preserve our independent adventure identities,
joint quest/location constraints and complete expedition-save requirements.

Use the previously proposed blacksmith/mine adventure as a bounded comparison:

- Follow one complete loop: access the blacksmith directly through the city
  screen, prepare, select the mine on the adventure map, explore connected
  spaces, resolve its main encounter and return for a persistent equipment
  upgrade. Use concise choices and discoveries without town dialogue rounds.
- Begin with an authored case if useful, then generated variants using the same
  validated data and runtime. Record the actual party size for every trial and
  compare larger-than-four candidates before selecting a cap.
- Check one main battle per floor as the default, hidden loot without combat,
  and an optional secret battle with a special reward. Demonstrate that normal
  progression does not depend on that secret. A separate lowest-floor variant
  can test a boss occupying the main encounter slot; secret encounters are not
  bosses, and neither secret encounters nor a boss are mandatory in every dungeon.
- Two simultaneous instances of one template with different participants,
  locations and rewards; no cross-instance progress or event leakage.
- Real negotiation, infiltration and combat routes in the generated location;
  explain and reject an impossible candidate with a bounded fallback.
- Stable layout, loot, doors, participants and quest progress across town trips,
  map re-entry, saves and reloads, including discoveries and in-progress tactical
  battle state; no rerolled rewards or restarted encounters.
- Exploration movement through room connections, legal tactical deployment,
  reachable objectives and useful participation at candidate party sizes.
  Test a battle near a section boundary; no loss of authoritative actors, turns,
  effects or objectives when presentation unloads.
- Preservation or explicit migration of an active expedition after a content
  or provider update; reward delivery remains exactly once.
- A measured generation budget and separate measurements for description size,
  complete save size, loading, memory and frame performance.
- Separate pacing observations: exploration and battle duration, idle or blocked
  character turns, backtracking, discovery usefulness and equipment-management
  effort. One main battle per floor is a design choice, not a measured optimum.
- An AI-assisted edit that can be inspected, validated, opened in the engine and
  tested through the same public operations available to a human tool user.

These checks are proposed work. No claim of passing them is made here.
