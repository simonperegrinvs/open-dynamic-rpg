# Open Dynamic RPG: concept and design considerations

Updated: 2026-09-14.

Status: concept and gameplay-system definition. Agreed gameplay choices are
recorded below, and the eight pillars remain accepted design considerations.
Unreal Engine 5.8 is the agreed working engine, with Blender and Rigify for
creature authoring and reusable open-source adventure tools around the engine.
Detailed gameplay rules, technical architecture and prototype scope remain
to be settled. Compact dynamic dungeons and composable environment assets are
required; specific quest and dungeon dependencies remain under evaluation.

The current direction combines Heroes-inspired city screens and a compact,
freely navigable overworld with curiosity-driven dungeon exploration and
Shining Force II-inspired tactical battles. See
[Gameplay reference mix](#gameplay-reference-mix) and
[Dungeon exploration and battle pacing](#dungeon-exploration-and-battle-pacing).
The former real-time-with-pause approach and fixed four-character party have
been superseded. The active party limit, total recruitable roster and detailed
turn rules remain open. This document integrates the revised direction; earlier
compatible choices remain the full-game baseline, not a requirement to implement
every system in the first playable slice.

## Working concept

A single-player fantasy RPG about an independent adventurer building a career
from a familiar trading town while uncovering why ancient routes between
forgotten places are reopening. Local commissions, recovered equipment,
relationships and discoveries lead gradually into otherworldly expeditions.
Prepare through direct city services, discover and choose destinations across
a compact open overworld, explore connected dungeon spaces for mystery and loot,
and command the party in
selected turn-based tactical battles.

The world combines wonder with real danger: welcoming places and memorable
friendships coexist with dangerous ruins, disturbing discoveries and difficult
choices. Growing capabilities open stronger challenges while earlier dangers
remain recognizable measures of the party's progress.

The first complete version is designed primarily for the user's own play,
with practical development costs and a possible future public release in mind.

The existing graphics experiments can inform character and equipment production.
This is an original game concept with its own rules and content.

## Gameplay reference mix

Working direction, 2026-09-14. Each reference informs a particular part of the
experience; exact controls, rules and scope remain open.

| Part of the game | Reference | Intended experience |
|---|---|---|
| City visits | Heroes of Might and Magic | An atmospheric city screen with direct access to services, party preparation and opportunities. |
| Regional travel | Shining Force II as an exploration reference; our own compact overworld design | Freely navigate a region, discover alternative destinations and revisit places, with room to detour from the main story. |
| Dungeon exploration | Neverwinter Nights, Diablo I and Skyrim | Physically explore connected spaces, uncover mysteries, discover hidden loot and learn about the place through its environment. These are references for exploration and atmosphere. |
| Battles | Shining Force II | Control individual party members in readable, turn-based tactical encounters, with movement, positioning and complementary roles. |

### City services and the compact overworld

City visits should be quick and purposeful. Present the forge, market, recovery,
recruitment and adventure opportunities through directly accessible services.
Recurring characters can express personality and offer short consequential
choices within those interactions. Progress should not depend on walking around
town or repeatedly checking multiple NPCs for new dialogue.

Use a compact, freely navigable regional overworld rather than a fixed sequence
of destination nodes. Offer several places to explore at once, let the player
change course, and allow discoveries independently of accepted quests. Roads
guide navigation; traversable terrain can support off-road detours. Familiar
places remain revisitable under their persistence rules. This is a selective
exploration reference, not a commitment to reproduce Shining Force II's world
structure or story progression.

Compact describes the scale and presentation, not a linear itinerary. Represent
party travel at regional scale; detailed exploration of actual rooms and passages
begins inside dungeons. A continuous, full-scale 3D outdoor world is not required.
Geography, access conditions and recognizable fixed dangers can limit where the
party can safely go, while story leads provide direction without removing
alternative destinations.

Heroes remains the reference for direct city services, not the specification for
overworld travel. Exact map controls, discovery visibility, travel time and
movement budgets remain undecided. City construction and territorial control
are also separate, undecided systems.

### Mystery, discovery and tactical encounters

Dungeon exploration should reward curiosity, observation and the team's
capabilities. Hidden loot, secret areas and optional encounters make searching
worthwhile. Environmental clues can give a location a history and suggest what
lies deeper. Possible discoveries to evaluate include shortcuts, unusual
mechanisms, evidence of an inhabitant and alternate approaches to a battle.

The chosen rhythm is one main battle per floor, optional secret bonus battles
with special rewards, and a possible lowest-floor boss as that floor's main
battle. See the pacing section for the full decision. Combat occupies selected
major rooms within the connected dungeon. Discovery and atmosphere must carry
the intervals between battles; copying these references' encounter frequency
is not part of the decision.

The loop is: prepare in the city, explore the compact overworld and choose a
destination, explore its dungeon, resolve selected tactical encounters, then return with
discoveries and rewards that develop the party and unlock further opportunities.
The simultaneous party limit, exploration controls, deployment transitions and
precise combat rules remain open. Plan for evaluating parties larger than four;
eight and twelve are comparison candidates, not selected limits.

The separation of town, adventure-map and combat views can be studied in the
[Heroes III manual](https://shared.fastly.steamstatic.com/store_item_assets/steam/apps/297000/manuals/BONUS_Heroes_of_Might_and_Magic_III_HDEdition_OldManual1999_EN.pdf).
Individual tactical movement and actions can be studied in the
[Shining Force II manual](https://www.sega.jp/genesismini2/assets/manual/pdf/US_Shining-Force-II.pdf).
The proposed combination is our design interpretation, not a locally tested
result or a dependency selection.

### Research evidence and its limits

The comparison researched on 2026-09-14 supports the ingredients, not a proven
formula for their combination. The
[Solasta review](https://www.pcgamer.com/solasta-crown-of-the-magister-review/)
describes connected dungeon exploration, alternative approaches and tactical
use of the environment. The
[Battle Chasers publisher description](https://www.thqnordicmobile.com/en/games/battle-chasers-nightwar-mobile/)
describes secrets, puzzles and character-specific dungeon abilities, but its
battles use a separate JRPG format. Its
[PC Gamer review](https://www.pcgamer.com/battle-chasers-nightwar-review/)
also reports difficulty spikes and drawn-out fights: interesting combat alone
does not establish good expedition pacing.

One main battle per floor is our chosen starting pattern. Floor size, useful
exploration time, battle length, party size and equipment-management effort
must be assessed together. No playtest of this combination has been performed.

## Agreed direction

- **Unreal Engine 5.8 as the working engine.** Use Unreal for gameplay,
  rendering, animation playback and world assembly. Its integrated animation
  and graphics workflow fits the original creatures, companions and visible
  equipment progression. Validate an original creature and a generated dungeon
  on the intended Mac before committing to large-scale production.
- **Blender and Rigify for creature authoring.** Retain editable models, rigs,
  materials and authored motion outside Unreal, then export deformation
  skeletons and baked clips for engine integration.
- **Tools must be usable for free.** The production workflow must support the
  required creation, editing and export without paid licenses, subscriptions or
  paid-only add-ons. Free-to-use engines such as UE5 are acceptable, with any
  later revenue-based conditions stated clearly. A trial or an export-locked
  free edition cannot be a required production dependency.
- **Develop reusable open-source tools.** New tooling should be suitable for
  other developers to use and extend. Keep reusable rules, interfaces and
  examples separate from this game's story and production assets. Prefer
  extending suitable open-source foundations; exact licenses, repositories
  and dependencies remain to be selected. Public release is an intended
  outcome, not an instruction to publish the current work immediately.
- **Six classes and twelve specializations.** Warrior, Rogue, Ranger, Mage,
  Cleric and Barbarian each have two specialization choices. Roles remain
  distinct but flexible, supporting several effective party compositions.
- **Permanent classes with optional dual-classing.** A character can invest in
  up to two classes, choosing which receives each new level. Each class earns
  one defining specialization. Invested levels and specializations cannot be
  reassigned; mentors can retrain noncombat skills and elective combat talents.
- **Power plus broader growth.** Both chosen classes can eventually develop
  fully with sufficient investment. Remaining single-class reaches deeper
  mastery sooner. Slower mastery continues offering modest combat-power gains
  without a fixed cap, alongside equipment, knowledge, relationships, reputation
  and access. Advancement rates and numerical rules remain open.
- **Ancestry and background.** Choose Human, Elf, Dwarf or Orc and a grounded
  background that influences early contacts and dialogue while leaving future
  ambitions open.
- **An individual-character party and recruitable roster.** Develop the hero
  and companions as tactical partners with personal stories and relationships.
  Evaluate more than four simultaneously active characters; the active limit
  and total recruitable roster remain open.
  Recruited reserves share party advancement; equipment investment and shared
  experiences remain individual.
- **Direct tactical control.** The player commands each active character's
  movement, targets and abilities during turn-based battles. Exploration group
  controls and any optional combat automation remain to be designed.
- **Measured, readable combat.** Shining Force II-inspired tactical encounters
  make movement, actions and outcomes understandable. Outcomes depend on
  character abilities, equipment, tactics and understandable probabilities.
  Armor reduces the harm of successful hits, with avoidance handled separately.
- **D&D-inspired custom rules.** Capture the value of builds, preparation and
  equipment while designing for this game's continuing progression.
- **Equipment counters matter.** Correct weapons, materials and enchantments
  provide strong advantages. Occasional threats have clearly signposted special
  requirements, with alternative approaches where appropriate.
- **Prepared spells with tiered uses.** Choose a spell selection at a safe place,
  then manage limited uses at each power tier during the expedition.
- **Refuges and field camps.** Basic abilities remain reusable, while healing
  resources and limited powers require recovery. Use established refuges or
  spend limited camping supplies at suitable secured sites; town provides
  dependable recovery.
- **Wounds after surviving a battle.** Fallen companions recover at low health
  after victory with a temporary wound cleared at a safe site. Revival during
  combat requires a limited ability or item.
- **Reload after party defeat.** When the whole party loses, resume from a
  previous save. The failed attempt leaves no lasting changes beyond that save.
- **Save anywhere, including battles.** Preserve city and map progress,
  dungeon exploration and an in-progress tactical battle for convenient stopping
  points and retries. Handling a save during action playback remains to be defined.
- **Fixed world danger.** Locations, depths and world events determine threats
  independently of the hero's level. Earlier threats remain easier as the party
  improves. This is our design rule, not a claim about every encounter in NWN.
- **Direct city services and a compact open overworld.** Use a Heroes-inspired
  city screen for preparation and concise interactions. Freely navigate a
  regional map with alternative destinations, discoveries and revisitable
  places, rather than a fixed itinerary. Adventures can expand into ruins,
  wilderness and fortresses, growing from local problems into other realms.
- **Persistent places with temporary expeditions.** Ordinary locations usually
  retain their geography and history. Portals and similar means can provide
  generated expeditions. An unfinished expedition remains available across
  saves and town visits until completed or explicitly abandoned.
- **Compact, dynamically assembled dungeons.** Represent a dungeon as a small
  description that expands into a playable place using shared, reusable and
  composable environment assets. New expedition locations can be generated
  during play; returning to an existing one preserves its layout and progress.
- **Exploration punctuated by major battles.** Connected dungeon spaces reward
  curiosity with mysteries, hidden loot and discoveries. Usually one main battle
  occupies a major room per floor. Extra battles are optional secret content
  with special rewards, not bosses. A dungeon may have a lowest-floor boss as
  that floor's main battle.
- **An evolving mystery.** Reopening ancient routes connect the main story,
  factions, companions and equipment lore. Chapters have real endings while
  discoveries reveal who sealed the routes and why.
- **Discovery and skills unlock approaches.** Clues, relationships and preparation
  reveal alternatives to combat. Meeting a skill requirement makes an approach
  reliable; optional risky attempts offer visible odds and clear stakes.
  Noncombat skills have separate advancement choices. A present, willing party
  member can provide the required expertise or speak on the party's behalf.
- **Concise written choices and discovery-led storytelling.** City-service
  interactions and relevant expedition events offer responses shaped by skills,
  discoveries and relationships. Environmental evidence carries much of the
  mystery. Generated adventures supply contextual choices without requiring
  repeated town exploration or rounds of NPC conversations.
- **Companions warn, then act.** They communicate disagreements and limits before
  refusing significant actions or eventually leaving after consequential conflicts.
  Reconciliation remains possible through meaningful actions or a personal quest.
- **Faction ties become consequential when interests clash.** Work with several
  groups, then take sides when their goals conflict. Deeper commitments affect
  access, services and relationships.
- **Only clearly urgent quests have deadlines.** Selected quests announce a
  deadline or impending event; most allow exploration and other goals at the
  player's own pace.
- **Signature equipment can grow into the endgame.** Certain weapons and armor
  have long growth paths. Other finds offer alternatives, components and
  occasional substantial replacements.
- **Major upgrades motivate specific adventures.** Pursue a known material,
  lost technique or crafter's problem, supported by ordinary money and supplies.
- **Reforgeable item branches.** Choose meaningful upgrade branches and
  enchantments. The appropriate craftsman can revise those choices at a cost
  while preserving the signature item's identity.
- **Selective gear and useful supplies.** Reward adventures with manageable
  equipment discoveries, materials, money and consumables. Use gold and a few
  broadly useful material families, with exceptional quest components for major
  upgrades and unlocks.
- **Equal equipment depth across the party.** Hero and companions use the same
  equipment rules and meaningful choices, supported by comparisons,
  recommendations and saved loadouts.
- **Visible equipment development.** Major upgrades communicate increased power
  through construction, materials, ornamentation, enchantments and effects.
- **Quests assembled from designed rules and story patterns.** This is the
  selected generation approach. Its flexibility comes from supported situations,
  actors, objectives, solutions and consequences.
- **An elevated 3D dungeon viewpoint.** A rotatable, zoomable camera supports
  exploration, full-party tactics and visible equipment. Close inspection can
  show finer detail. City screens and the compact overworld use their own suitable
  presentation; they do not require a walkable 3D town or full-scale 3D outdoor world.
- **Detailed, stylized fantasy with realistic materials.** This is the selected
  visual target: expressive, readable shapes with convincing metal, leather,
  stone, skin and magical effects. Art quality must hold at the gameplay camera
  and during close equipment inspection.
- **A broad creature catalogue.** Support a large variety of creatures, including
  different body types, with reusable foundations for appearance and animation.
  Exact species, rig families and production libraries remain to be selected.
- **Original creature designs with portable production.** Plan for creatures
  built to the game's visual and RPG requirements. The proposed Quaternius
  monster and KayKit skeleton packs are unsuitable as visual baselines. Prefer
  modeling, rigging and animation tools whose results can move between engines.

The immediate priority is to refine this concept and its gameplay systems.
The earlier prototype outline is a proposal to revisit after that work.

## Character creation and advancement

### Ancestry, background and class roles

The playable ancestries are **Human, Elf, Dwarf and Orc**, with distinct
appearances, cultures and opportunities for character background. Select a
grounded origin that can affect early contacts and dialogue. Former guard,
scholar and outlaw are examples; the final background list and ancestry
mechanics still need design. Future ambitions remain open to the player.

Classes have recognizable styles, and their specializations provide different
ways to contribute. Several party compositions should be able to cover healing,
protection and control. The accepted starting roster for the full game is:

| Class | Specialization | Defining role |
|---|---|---|
| Warrior | Guardian | Protection and interception |
| Warrior | Weapon Master | Precision and weapon techniques |
| Rogue | Assassin | Prepared strikes and debilitating effects |
| Rogue | Saboteur | Traps, devices and disruption |
| Ranger | Marksman | Ranged precision |
| Ranger | Pathfinder | Scouting, terrain and field support |
| Mage | Elementalist | Elemental damage and exploiting vulnerabilities |
| Mage | Arcanist | Wards, control and dispelling |
| Cleric | Restorer | Healing and cleansing |
| Cleric | Templar | Blessed weapons and frontline support |
| Barbarian | Berserker | Fury and offensive pressure |
| Barbarian | Juggernaut | Endurance, charges and staggering enemies |

These roles are agreed; individual abilities and their unlock levels remain
to be designed. A character chooses one defining specialization in each class
they develop, with that class's shared abilities and elective talents alongside it.

Develop these roles for turn-based decisions: movement, protection, range,
control, disruption and resource use. Exact action costs, reactions, initiative
effects and terrain rules are still proposals to evaluate. A larger active
party increases the number of decisions and loadouts; keep useful choices
readable and test management effort before expanding ability lists.

### Dual-classing and long-term mastery

The hero and companions can develop up to two classes. Each new level advances
one chosen class. For example, a Warrior 6/Mage 4 is level 10 overall and can
next become Warrior 7/Mage 4 or Warrior 6/Mage 5. Class abilities and the class's
specialization develop through investment in that particular class.

Beginning a second class requires both suitable aptitude and training or a
meaningful quest for an appropriate mentor. Exact prerequisites and unlock
levels remain open.

Access mentor services through the city interface when available. A training
quest can lead to an expedition or discovery; routine training should not
require searching town for a mentor or repeating dialogue to reach the service.

With enough investment, both chosen classes can reach their advanced abilities.
Staying single-class puts that investment into deeper mastery sooner. This
supports a continuing choice between breadth and specialization while preserving
the agreed slower, uncapped power growth. The transition from class development
to mastery, advancement costs and numerical benefits still need design.

The reference for allocating levels to individual classes is
[D&D's multiclassing structure](https://www.dndbeyond.com/sources/dnd/basic-rules-2014/customization-options).
The two-class limit, long-term development and retraining policy are this game's
design choices. A recommendation to evaluate is that the cost of gaining a level
uses total character advancement, keeping a late second class a substantial
investment.

### Skills, party expertise and mentor retraining

Noncombat skills have their own advancement choices, separate from combat
upgrades. Investing in persuasion, investigation or other utility skills does
not consume a combat upgrade. A present, willing party member can contribute
expertise or speak on the party's behalf, while the player chooses the party's
decisions.

Mentors can reassign noncombat skills and elective combat talents within their
respective advancement budgets. **Class choices, invested class levels and
defining specializations are permanent.** Mentor retraining cannot replace a
class, transfer levels between classes or change a specialization. The exact
cost and process for skill and talent retraining remain to be designed.

## Story, world memory and the adventure loop

The trading town anchors the adventurer's relationships and equipment journey.
Ancient roads and portals are reopening connections to forgotten places. The
central mystery concerns their history, who sealed them and why they are opening
again. The hero becomes involved through work, discoveries and relationships;
their identity remains that of an independent adventurer.

The town is represented by its city screen and recurring services. Surface
consequential news and choices directly in those services. Dungeon evidence,
discovered routes, recovered objects and brief expedition events advance the
mystery alongside character interactions. A discovered destination can be worth
exploring before a commission names it.

Ordinary places should remain recognizable on return, with inhabitants and
problems changing through events and the player's choices. Temporary portal
expeditions provide additional variety while keeping long-term world persistence
manageable. Their rewards, discoveries and story consequences belong to the
ongoing character history even when the expedition location is retired. The
player can leave an unfinished expedition, resupply in town and return to the
same location and progress. Saves preserve the unfinished expedition, including
an in-progress tactical battle. Completion or explicit abandonment ends the
expedition's reuse; its location can retire after the party has left. Detailed
storage and cleanup mechanics remain part of the future technical design.

Selected urgent quests can have deadlines or impending events that are clearly
communicated to the player. Most quests allow other adventures to be pursued
without time-based failure. Deadline triggers and the rules for advancing quest
time still need design.

Define regional travel time, exploration time, tactical turn time and recovery
time together. Time spent thinking at a decision or reading a city screen must
not silently consume an urgent quest's deadline. A daily movement budget or a
Heroes-style calendar has not been selected.

The proposed loop connects the agreed systems:

1. Directly accessible city opportunities, rumors, map exploration and dungeon
   discoveries reveal an expedition, an equipment goal, a companion concern or
   a lead in the main mystery. Repeated town dialogue rounds are not required.
2. Prepare the party, equipment and supplies using knowledge of the destination
   and its dangers, then travel across the compact overworld, with room to
   change course or investigate another destination.
3. Explore connected dungeon spaces, gather clues, discover hidden loot and
   choose supported approaches. Use skills, relationships and selected tactical
   battles to resolve obstacles, managing resources between safe places.
4. Return with consequences that advance relationships, knowledge, equipment
   or the main story. Develop signature items through the recurring craftsmen
   and merchant.
5. Pursue newly available opportunities while completed arcs and familiar
   places retain their history.

## Eight design pillars

### 1. Character identity beyond equipment

Use the six classes and twelve specializations above to create distinct but
flexible builds. Develop their utility skills and interactions with equipment.
Ancestry, selected background and personal ambition should matter in appropriate
conversations and discoveries. Dual-class investment and slower, uncapped mastery
extend the character's career alongside broader forms of advancement.

Mentor retraining supports experimentation with skills and elective talents while
preserving class levels and specializations. Continuing advancement should retain
meaningful choices about what to prepare and equip.

**Design check:** two different builds can approach the same adventure through
distinct, viable tactics or opportunities.

### 2. Several meaningful ways to resolve adventures

Develop a deliberate vocabulary of supported actions, such as investigation,
persuasion, intimidation, infiltration, disabling obstacles, protection and
combat. Generated quests must express those actions through actual game rules.
Successful alternative resolutions should receive comparable advancement.

Discoveries, relationships and preparation unlock approaches expressed through
written dialogue choices and supported world interactions. Meeting the relevant
skill requirement makes an approach reliable. Optional attempts beyond that
reliable capability can offer additional opportunities, with visible odds and
clear stakes. The exact skill list, thresholds and failure consequences remain
to be designed.

Keep negotiation and other social solutions focused on consequential decisions
at a service or encounter. A floor's main encounter may support an alternative
resolution where its adventure allows it; one planned battle per floor is a
layout and pacing default, not a requirement to kill every main opponent.

Use the separate noncombat advancement budget to make expertise worth developing
across the party. A companion can supply the needed skill or lead a conversation
when present and willing. The character contributing the expertise should be
acknowledged in the interaction.

**Design check:** a quest family can vary the player's decisions and outcomes,
as well as its names, locations and enemies.

### 3. Factions, loyalties and relationships with consequences

Give recurring characters interests, obligations, beliefs and relationships.
Faction advancement should follow relevant accomplishments and bring recognition,
access and responsibilities. Record consequences that can influence later
conversations, services and adventures.

The player can work with several groups and take consequential sides when their
interests clash. Deeper commitments affect access, services and relationships.
Companion disagreements and departures should have paths to reconciliation
through meaningful actions; repairing a relationship preserves the consequences
of the original decision.

The blacksmith, enchanter and store owner are central examples of these recurring
characters. Their roles in equipment development are detailed below.

Expose these consequences through changed service options, concise messages,
map access and discoveries. Relationships can remain meaningful without
requiring routine social visits to every character in town.

**Design check:** completing an adventure changes something the player can later
observe in a relationship, faction, service or place.

### 4. Exploration and knowledge as progression

Use rumors, maps, scouting, tracks, journals and accumulated creature knowledge
to help players understand dangers and prepare. Include secrets and distinctive
locations that can be discovered independently of accepted quests. Sound,
lighting and environmental details should make places worth exploring.

On the compact overworld, discovery reveals alternative destinations and routes,
including places away from the main story lead. Inside a
dungeon, curiosity can uncover loot, secret spaces and information about its
inhabitants or history. Not every reward needs a battle. Optional secret battles
offer special rewards and must remain unnecessary for ordinary progression.
Prefer observable clues and useful discoveries to exhaustive container checking;
exact detection and interaction rules still need design.

**Design check:** information or a discovered route can change a sensible plan
before the party gains another level or equipment upgrade.

### 5. Combat decisions and expedition resources

Create tactical priorities through enemy roles, terrain, positioning and
encounter objectives. Each turn should offer useful decisions, such as stopping
a ritual, protecting an ally or reaching a vulnerable opponent. Battles occupy
selected major dungeon rooms, following the one-main-battle-per-floor pattern.
Turn order, movement and action budgets, range, terrain, reaction opportunities
and effect-duration rules remain open. The reference alone does not select a
particular grid, agility formula or move-plus-action rule.

Armor reduces damage from successful hits. Agility and positioning support
avoidance separately. Correct weapons, materials and enchantments should confer
strong advantages against relevant threats. A few clearly signposted threats
can require a special counter, with alternative approaches where appropriate.
Exact hit, avoidance, mitigation and resistance formulas remain open.

Casters prepare a selection of spells at a safe place and have limited uses at
each power tier. Basic abilities remain reusable. Define healing, spell recovery,
consumables, resting and retreat together so expedition decisions remain
meaningful and repeated backtracking stays manageable.

Established refuges provide recovery. Suitable secured locations can also support
field camps by spending limited camping supplies; town remains a dependable
place to recover. Precise spell and supply budgets, wound penalties, campsite
requirements, retreat behavior and action timing remain to be designed.

Budget resources around a small number of substantial encounters, with optional
secret battles adding a voluntary cost for a special reward. The main path must
be viable without secret rewards or grinding extra fights. Whether and how any
resources recover between encounters remains to be designed; a battle ending
does not by itself establish a full recovery rule.

The player directly commands individual characters in turn-based combat.
Action presentation should clearly explain attacks, defenses and effects while
keeping a larger party's turns brisk. Optional automation and animation-speed
controls can be evaluated later. If the party wins, fallen companions
recover at low health with a temporary wound cleared at a safe site. Reviving
someone during combat requires a limited ability or item. Whole-party defeat
returns the game state to a previous save.

Allow saving in the city, on the compact overworld, during dungeon exploration and
within tactical battles. Reloading restores the saved situation, including an
unfinished expedition and its battle progress. Preserve the current acting
character, turn order, spent actions, positions, effects, objectives and random
state as required by the eventual rules. Autosave policy and the handling of
save requests during movement or action playback remain open; mid-battle saving
must not become a restart of the encounter.

Communicate dodges, armor deflections, damage and magical resistance clearly in
animation, sound and accessible explanations of the rules.

**Design check:** an encounter can reward a change of tactics, and the player can
understand why an action succeeded or failed.

### 6. Companions as tactical partners and characters

Support the hero and a selected active team drawn from a recruitable roster.
The simultaneous active limit includes the hero and is separate from total
recruits. Evaluate teams larger than four; eight and twelve are study candidates,
not commitments. Give companions distinct roles, personal stories,
equipment development and contributions to discovery and dialogue. Their
relationships with the hero and each other should develop through shared
experiences.

Recruited reserves share the active party's advancement. Equipment investment
and shared experiences remain individual, allowing party changes without having
to grind advancement for a reserve. Each companion has access to the agreed
dual-class and mentor skill-retraining rules.

The player directly controls active companions during their tactical turns.
Exploration should keep moving the group convenient; leader movement, following
formations and deployment at an encounter are options still to compare. The
rules for entering combat must preserve the approach actually discovered and
taken, the participants and any relevant exploration state. Deployment must not
grant an unexplored flank or remove an established obstacle for convenience.
Companions use equipment systems with the same depth as the hero, with comparisons,
recommendations and saved loadouts to keep party management convenient.

Companions voice disagreements and explain their limits. Significant conflicts
can lead to refusing an action or eventually leaving, with warning before a
serious rupture. The exact relationship thresholds and ways to repair a
relationship still need design, but reconciliation remains possible through
meaningful actions or a personal quest. Earning a companion's return does not
erase the consequences of earlier decisions.

Allow several effective party compositions through overlapping capabilities,
equipment and consumables. The generator should consider the active party's
knowledge, relationships and supported actions. World threats remain independent
of party level; travelling with fewer companions can increase the challenge.

Companion identities and starting builds, recruitment and initial advancement,
roster size, optional automation, personal stories and relationship thresholds
still need detailed design. Fallen companions recover with wounds after a surviving
party's victory, as described in the combat rules above.

**Design check:** changing a companion can affect both tactics and an adventure's
social or investigative possibilities.

### 7. A reward economy for treasured equipment

Make treasure valuable through techniques, enchantments, exceptional materials,
alternative equipment and discoveries about an item's potential. A desired
improvement should have an understandable route through relevant characters,
suppliers and adventures. Random discoveries can add surprises.

Signature items have long development paths that can carry them into the distant
endgame. Other finds provide alternatives, components and occasional substantial
replacements. Most major upgrades motivate a specific adventure to obtain a
known material, recover a technique or solve a crafter's problem. Ordinary money
and supplies support those advances.

Signature items have meaningful upgrade branches and enchantment choices. The
appropriate craftsman can revise them at a cost while preserving the item's
identity. Reforging prices, refunds or material recovery and exact branch
compatibility remain to be designed.

Use gold and a few broadly useful material families, with distinctive quest
components for major upgrades and unlocks. Loot should offer manageable equipment
discoveries alongside useful money, materials and consumables, keeping exceptional
items easy to notice.

Favor predictable ordinary enhancements. Any risky experiment should have clear
stakes appropriate to the player's investment in the item. Exact material
families, recipes, costs, upgrade branches and equipment compatibility remain
to be designed.

Special secret rewards can be distinctive equipment, materials or techniques;
their exact forms remain open. They should feel worth discovering while normal
progression remains viable without them. Exploration loot and rewards from the
main route must also sustain equipment growth. Test the burden of equipping the
full active team, retaining comparisons and saved loadouts as it grows.

**Design check:** an expedition can provide a worthwhile reward while the player
keeps their signature item, and major improvements are visibly recognizable.

### 8. Narrative pacing and stories that can end

Provide satisfaction at several scales: useful progress within an expedition,
substantial equipment or relationship development across several adventures,
and complete story arcs over a longer period.

Use an evolving mystery about the reopening ancient routes to connect the
adventurer's career. Discoveries reveal that mystery in chapters with real
endings; factions and companions provide different ways into it. The tone
combines wonder with real danger, and the scale grows from local troubles to
extraordinary creatures, ancient powers and other realms.

Named antagonists can be defeated, mysteries can be answered and resolved
problems leave lasting consequences. Further adventures emerge from those
consequences and new frontiers. Quest selection should consider recent
experiences and unfinished relationships to vary the rhythm of investigation,
danger, discovery and conversation.

Deliver essential story through discoveries, brief event choices and direct
service interactions. A floor's main encounter gives it a point of tension; a
lowest-floor boss can conclude a dungeon, while secret bonus encounters remain
separate optional content. Foreshadowing and environmental storytelling are
preferred ways to make these places intriguing between battles.

Reserve time-based failure or changes for quests that clearly announce urgency.
Most adventures should remain available while the player pursues other goals.

**Design check:** a completed arc has a lasting resolution while the character
retains meaningful reasons to continue adventuring.

## Equipment development through recurring characters

| Character | Role in development | Possible lasting unlocks |
|---|---|---|
| Blacksmith | Improve craftsmanship, reforge components and work exceptional materials | Techniques, tools, access to materials and new physical improvements |
| Enchanter | Reveal hidden properties, add enchantments and awaken dormant powers | Knowledge, rituals, reagents and magical development options |
| Store owner | Source equipment and materials, arrange commissions and connect suppliers | Trade routes, special orders, collections and dependable supplies |

These are distinct services supported by character relationships and quests.
Their personal stories can also reveal side quests and main-story information.
Open each service directly from the city screen and surface relevant new
opportunities there. Keep equipment work accessible without repeating its
associated story conversation on every visit.

The blacksmith and enchanter support revising the appropriate upgrade branches
at a cost, preserving a signature item's identity. The store owner helps source
the gold-priced supplies, broad material families and special commissions that
support the equipment journey.

Major upgrades should give the player a specific adventure to pursue through
these relationships. A recommendation to evaluate is that discoveries such as
forging techniques unlock options for the whole party, while rare materials
create individual upgrade priorities. The exact unlock and resource-sharing
rules are still open.

Equipment visuals should retain recognizable item identity while conveying
development through several channels:

- Craftsmanship: construction, fittings, engravings and component detail.
- Materials: distinctive metals, crystals, layered surfaces and magical inlays.
- Enchantments: runes, embers, frost, electrical arcs and thematic particle effects.
- Awakened abilities: distinctive trails, impacts and reactions during use.

The actual item upgrades should drive the visual changes. Major improvements
must read from the elevated gameplay camera, with effects that preserve combat
clarity. Close inspection can reveal finer craftsmanship. The same principles
apply to companion equipment.

## Dynamic dungeons and composable environments

The dungeon system is a required part of the adventure foundation. A generated
quest needs a physical place that supports its objectives, alternative routes,
encounters and discoveries. Quest generation and location generation must use
the same requirements and stable references to rooms, characters and objects.

### Dungeon exploration and battle pacing

Agreed direction, 2026-09-14: explore connected passages and rooms, with selected
major rooms accommodating turn-based tactical battles. Curiosity and the team's
exploration abilities can reveal hidden loot and secret content between battles.
The exploration spaces and battle rooms belong to the same dungeon.

- **One main battle per floor is the usual pattern.** Provide enough space in
  the selected major room for the eventual active party to participate meaningfully.
- **Additional battles are optional secret content.** Discovering and completing
  one yields a special reward. These encounters are bonus battles, not bosses;
  a secret encounter is not required on every floor.
- **Secret rewards remain optional.** The ordinary progression route must work
  without finding or winning the bonus encounter. Ordinary hidden loot can also
  reward exploration without requiring a battle.
- **A dungeon may have a boss on its lowest floor.** When present, treat it as
  that floor's main battle, keeping the usual one-main-battle pattern. It is
  distinct from optional secret encounters.

This is a selected design pattern, not a measured pacing result. The earlier
comparison found precedents for dungeon exploration, tactical encounters and
final-floor bosses, but did not establish an optimal battle count for this game.
Battle duration, floor size, secret-encounter frequency, special rewards and the
active party size remain to be determined through focused design and playtests.

Use the blacksmith/mine acceptance slice to evaluate the pattern first. Its
negotiation and infiltration alternatives still apply; a planned main encounter
does not make combat the only valid way to resolve an adventure. Authored and
generated layouts use the same requirements and runtime. Discoveries, loot,
encounter outcomes and active battle state retain the persistence requirements
below.

### Designing the connected spaces

Give exploration passages, optional branches and the main battle room distinct
roles in each floor. Battle rooms need navigable space for the selected active
party, enemy roles, approach and exit routes, and any objective interactions.
Cover, elevation and hazards are possibilities to test, not mandatory features
for every room. A large empty chamber alone is not a tactical design.

Use clues, atmosphere and discoveries between encounters. Optional branches can
contain unguarded hidden loot or a secret bonus battle with a special reward.
Do not require a second battle simply to fill the floor, and do not hide an
ordinary progression requirement behind optional secret content. A possible
lowest-floor boss occupies the main encounter role rather than adding another
mandatory battle. Dungeon depth and the frequency of secrets remain open.

Define how exploration enters and leaves battle before finalizing room kits:
participants, legal starting positions, discovered approaches, battle extent,
retreat and pursuit limits, and what changes after an alternative resolution.
Connected spaces describe the geography; they do not yet settle every transition
or require unrestricted combat across the whole floor. Defeated inhabitants,
opened shortcuts and collected rewards remain changed after the battle.

The following technical design is proposed. Exact file formats, module
dimensions, generation algorithms and performance budgets remain to be validated.

### A compact description backed by a shared asset library

Store the expensive meshes, textures, animations and reusable room definitions
once in the game's content library. Each dungeon refers to that library through
stable identifiers. Its individual description contains the choices and
arrangement needed to reconstruct the place, keeping repeated asset data out
of expedition saves.

Separate three kinds of information:

| Record | Contents | Purpose |
|---|---|---|
| Generation recipe | Generator and content versions, seed, architectural family, region/depth threat profile, quest requirements, floor encounter roles and layout constraints | Request and reproduce a new candidate dungeon |
| Resolved layout | Stable floor, room and object IDs, module references, positions, orientations, connections, selected variants, main/secret encounter bindings, optional final boss role, rewards and quest bindings | Preserve the specific place accepted by the generator |
| Saved changes and simulation state | Opened doors, collected loot, discovered secrets, defeated or moved inhabitants, quest outcomes, active party, positions and tactical battle state | Resume the same expedition and its consequences |

A seed alone is insufficient for the agreed persistence. Changes to the
generator or asset catalogue can change its output. Keep the resolved layout
and gameplay-relevant placements, version content references, and provide a
compatibility or migration policy for saved expeditions. Store meaningful
spawn and reward choices so that returning or loading cannot reroll them.
Cosmetic variation should not change layout, encounters or rewards.

The eventual battle model also needs a stable description of legal positions,
movement connections and gameplay-relevant terrain, consistent with the room
geometry. Validate it after placement and decoration. Exact grid geometry and
encoding remain undecided. Save rules-relevant turn order, spent actions,
effect durations, objectives and random state independently of animation.

The size goal concerns the per-dungeon description. Shared assets and the
expanded scene still consume installation space and memory. As an illustrative
calculation, 50 room records at 128 bytes each occupy about 6.4 KB before adding
connections, object placements and saved changes. This is a representation
example, not a measured file size or a committed room count. Measure the actual
encoding and complete save state before setting a storage budget.

### Reusable pieces at several scales

Use architectural kits to construct reusable room modules, then assemble those
modules into locations. Combine:

- Structural pieces: walls, floors, corners, arches, stairs and compatible
  doorways, with consistent scale, alignment and material conventions.
- Room and passage modules: galleries, chambers, intersections, bridges,
  entrances and distinctive landmark spaces. Include alternate shapes and
  approaches with meaningful tactical differences.
- Gameplay placements: typed attachment points for doors, clues, ore deposits,
  captives, main and secret encounters, traps, treasure, refuges and other
  supported interactions.
- Dressing and atmosphere: compatible furnishings, damage, vegetation,
  materials, lighting, particles and ambient sound appropriate to the site's
  architecture, inhabitants and history.

Each room declares its bounds, connection positions and types, supported
gameplay roles, collision and navigation requirements, and areas that must
remain clear. Validate doorway and stair connections for party movement,
combat spacing and the elevated camera. Decoration must preserve access,
important sightlines and equipment-effect readability.

Reuse should produce recognizable cultures and places with varied layouts,
landmarks and situations. Material swaps alone are insufficient variety.
Handcrafted story locations can use the same kits and interaction definitions;
generation does not require reshuffling persistent places on return.

### Generate the adventure and its geography together

1. Establish the site's identity, fixed danger and adventure requirements from
   the world and story pattern. Threat comes from the location, depth and world
   conditions independently of the hero's current level.
2. Propose connected spaces with a meaningful main route, optional discoveries,
   supported alternate approaches, shortcuts and an appropriate exit. Match
   those requirements against what the available room modules can supply.
   Assign one main encounter per ordinary floor, optional secret encounters
   with special rewards, and a possible boss replacing the lowest floor's main
   encounter. Preserve each encounter's role in the accepted layout.
3. Fit compatible modules and bind actual quest actors, objects and evidence
   to their supported placements. If the layout cannot support the adventure,
   revise or reject the candidate within a bounded generation process.
4. Validate physical navigation and quest dependencies together: keys and clues
   must be reachable when needed, a promised approach must exist in the level,
   and the party must have a legal way to proceed or return. Account for skill,
   discovery and willing-companion requirements without making every route
   automatically available to every party. Check legal deployment, movement,
   objective reachability and room capacity for the tested party size. The main
   route must remain viable without secret rewards or clearing secret battles.
5. Commit the accepted layout, then instantiate its environment and apply saved
   state. New generation must have a valid fallback if its attempt or time
   budget is exhausted.

For example, an upgrade adventure may require star iron from an occupied mine.
Its physical requirements could include a deposit, rival claimants, evidence
supporting negotiation, a discoverable alternative passage and a route back
out. Both the quest and dungeon refer to those same objects and passages.
Negotiation, infiltration and combat must work through the actual map and
inhabitants, with the agreed outcomes preserved on return to town.

### Generate on demand and preserve unfinished places

Solve the compact logical layout and its important dependencies before allowing
entry. Assemble the 3D representation on demand; larger locations may load
nearby sections as needed while preserving their full logical state. Generation
time, asset loading, navigation preparation and rendering are separate costs
that must be measured on the target machine.

Room modules can provide prepared collision and navigation data, with their
connections validated after assembly. Active party members, ongoing battles
and their objectives must remain supported across section boundaries, including
any reinforcements or pursuing actors allowed by the eventual encounter rules.
Unloading presentation must not reset encounters or erase quest progress.

Town visits and saves preserve the resolved layout and its current state,
including tactical battle state. Completion or explicit abandonment permits
retiring the location after the party leaves; discoveries, rewards and world
consequences retain their agreed lifetime. These rules also apply when the
original layout was assembled dynamically.

Keep regional map discoveries, routes and links to existing dungeon instances
stable across city visits and saves. A map marker for an unfinished expedition
must reopen that expedition. Define completion and departure clearly so players
know when a temporary location and any uncollected secret content will retire;
ordinary persistent locations retain their existing lifetime rules.

## Implications for the adventure toolkit

The agreed working foundation is Unreal Engine 5.8 with an RPG, adventure and
dungeon toolkit built from tools usable for free. Unreal handles gameplay,
rendering, animation playback and world assembly. Blender and Rigify provide
portable creature source assets and authored animation. Our new adventure tools
are intended for public open-source reuse, with shared data and core rules
separate from Unreal-specific integration.

The engine direction is settled for implementation planning; specific quest and
dungeon packages, save architecture and integration details remain proposals.
Validate visual quality, runtime animation, persistence and performance on the
intended Mac before large-scale production. A quest-graph plugin alone does not
provide the complete foundation. The Godot and Unity comparisons below remain
reference material for portability and any later reassessment.

The proposed toolkit should provide:

- Shared definitions for authored and generated adventures: situations,
  participants, objectives, encounters, solutions and consequences.
- A compact dungeon description, reusable environment and room libraries,
  constrained layout assembly, and shared bindings between quest requirements
  and physical locations. Preserve the accepted layout independently of its
  loaded 3D representation.
- A persistent record of characters, active party and reserves, relationships,
  knowledge, map discoveries and routes, equipment, services, locations and
  story progress. City and map screens observe the same state as the dungeon.
- Separate lifetimes for persistent places and temporary expedition locations,
  preserving earned rewards and story consequences when temporary areas retire.
  Keep unfinished expeditions across saves and town visits until completion or
  explicit abandonment, and retire their location data after the party has left.
- Save and resume city/map progress, dungeon exploration and tactical battles,
  including active temporary expedition layouts, progress and encounters.
- Reusable story patterns with conditions for when they make sense and effects
  that the game can execute and preserve.
- Assembly that selects compatible participants and places, then establishes
  objectives, encounters, written dialogue choices and outcomes for the
  adventure's lifetime.
- Supported approaches with discovery and relationship prerequisites, reliable
  skill requirements and optional risky attempts with visible odds. Account for
  which present party members have the expertise and are willing to contribute.
- Threat definitions tied to places, depths and expedition conditions, while
  slower uncapped mastery develops the party independently of existing threats.
- Checks for achievable objectives, valid dependencies, supported consequences
  and consistency with established world facts; validate floor encounter roles,
  optional secret access and battle-space capacity alongside quest constraints.
- Main-story truths and turning points that can be reached through suitable
  adventure paths, including alternatives when a particular path becomes unavailable.
- Quest and dialogue conditions that support faction conflicts, companion
  reconciliation and clearly announced urgency on selected adventures.
- Equipment goals tied to recipes, materials, mentor or craftsman tasks and
  meaningful service unlocks.
- Ways to inspect and expand the content vocabulary as the game develops.

An achievable quest is only one quality bar. The eight pillars above also guide
whether its choices, pacing, rewards and consequences make it worth playing.

The [AI-assisted 3D RPG reference review](RPG_REFERENCE_REVIEW.md) records
code-level findings from Sanctuary's End, Aetheria, Godotwind and Embermere RPG.
They offer useful examples of equipment services, compact layouts, appearance
recipes, streaming and validated state changes. Their inspected systems do not
establish our complete combination of story-aware generation, independent
adventure instances, persistent locations, larger-party tactical play and mid-battle
saving. Treat them as study material while retaining those requirements.

### Working stack and remaining dependency choices

Unreal Engine 5.8 and the Blender/Rigify authoring direction are agreed. The
quest and dungeon packages below are researched candidates, not installed or
integration-tested dependencies for this game. Reusable graphics production
knowledge does not establish a complete reusable RPG core.

| Direction | Reusable foundation | Work and uncertainty remaining |
|---|---|---|
| UE5.8 — working engine | Included animation, rendering and PCG tools; MIT SUQS and SimpleQuest as quest candidates; BenPyton's ProceduralDungeon as a CeCILL-C dungeon candidate; included Experimental MCP plugin | Evaluate runtime instances and save compatibility; add the adventure director and shared location constraints; check the dungeon license and public distribution design; validate animation, editor automation and performance on Mac |
| Godot — reference alternative | MIT engine and Dialogue Manager; MIT QuestSystem 2 and Nexus Quest Weaver candidates; room scenes and mesh libraries; CC0 SimpleDungeons as a modular-layout candidate | Retain findings for possible future adapters or reassessment; independent generated-adventure identity, planning, quest/location validation and persistence would need work |
| Unity Personal — reference alternative | Engine available for free to eligible users; standard model/animation import and rendering tools | Eligibility and free production-ready quest/dungeon dependencies would need evaluation if revisited; previously discussed paid packages remain outside this plan |

[Dialogue Manager](https://github.com/nathanhoad/godot_dialogue_manager) provides
branching dialogue connected to game-owned state.
[Nexus Quest Weaver](https://github.com/undomick/godot_nexus_quest_weaver)
provides graph authoring, parameters and saved quest progress. Source inspection
on 2026-09-13 found that its controller and pools identify instances by quest
file. Multiple independent simultaneous runs of one template would require
changes; its template/instance separation must not be mistaken for that feature.
Neither candidate selects a coherent physical dungeon on its own.

The focused [tooling review](TOOLING_REVIEW.md) records inspected
source revisions, maintenance evidence, fork limitations, the proposed public
tool boundaries and the current engine/MCP comparison. The new recommendation
is to evaluate SUQS as a compact Unreal quest foundation, with SimpleQuest as
the alternative when visual authoring warrants its larger scope. Neither is
selected or verified in this game.

Godot's [PackedScene](https://docs.godotengine.org/en/stable/classes/class_packedscene.html)
supports reusable scene instances. Its
[GridMap and MeshLibrary tools](https://docs.godotengine.org/en/stable/tutorials/3d/using_gridmaps.html)
support repeated meshes with collision and navigation data. GridMap is limited
to its supported mesh format; interactive doors, characters and scripted room
behavior need scene instances or other game objects. Neither tool is a complete
quest-aware dungeon generator.

[SimpleDungeons](https://github.com/majikayogames/SimpleDungeons) is a CC0 Godot 4
add-on for assembling procedural 3D levels from user-defined prefab rooms. It
is a candidate for reuse, not a verified complete solution for this game's
quest dependencies, room constraints or save compatibility.

UE5's included
[Procedural Content Generation framework](https://dev.epicgames.com/documentation/unreal-engine/procedural-content-generation-framework-in-unreal-engine)
supports procedural world-building tools, including
[runtime generation](https://dev.epicgames.com/documentation/unreal-engine/using-pcg-generation-modes-in-unreal-engine).
It provides construction machinery; our adventure rules still need to establish
valid routes, objective placement and lasting consequences.

[Godot is MIT-licensed](https://godotengine.org/license/).
[UE5's game-development license](https://www.unrealengine.com/license) provides
free upfront access for game development, with the standard 5% royalty applying
to qualifying lifetime product revenue above $1 million, subject to its terms
and exclusions.
[Unity Personal](https://unity.com/products/unity-personal) is free for eligible
individuals and organizations below its $200,000 revenue/funding threshold over
the preceding 12 months. Eligibility has not been assumed for this project.

Quest Machine, Dungeon Architect and other paid-only RPG packages are excluded
from the production baseline. Their documented approaches may inform our own
design, but they are not proposed dependencies. A free evaluation download does
not establish a free production license. None of the replacement paths has yet
been integration-tested against the full adventure and persistence requirements.

Before choosing dependencies, verify one representative adventure can:

- Generate different valid layouts with a coherent architectural identity and
  actual combat, social or exploration routes bound to the same quest.
- Connect direct city preparation, regional map travel, dungeon exploration and
  tactical battles through the same world and adventure state.
- Support exploration movement, camera, collisions and tactical deployment at
  explicitly recorded candidate party sizes, including a larger-than-four team
  and a battle near a section boundary. No final party cap is implied by a trial.
- Validate one main encounter per floor, optional secret bonus encounters and
  rewards, and a possible lowest-floor boss occupying its main encounter role.
  Support normal progress without clearing secret content.
- Reconstruct from its compact records and shared assets, preserving unfinished
  progress across town trips, mid-battle saves and reloads without rerolling
  threats or rewards.
- Keep two instances of the same adventure or room template independent, grant
  rewards once, and retain world consequences after a temporary area retires.
- Complete generation within a measured budget with bounded retries and a
  valid fallback; report description size, complete save size, loading time,
  active memory and frame performance separately.

These are proposed acceptance checks for later evaluation, not completed tests.
The [blacksmith/mine evaluation](TOOLING_REVIEW.md#evaluation-before-a-fork-or-dependency-commitment)
is the shared first acceptance slice for these requirements.

## Visual refinement and creature production

The selected target is **detailed, stylized fantasy with realistic materials**.
The Unreal production workflow must support that art direction, visible
equipment growth and readable full-party battles. Creature production starts
from the game's own species designs and anatomy requirements. Reuse suitable rigging systems,
animation techniques and compatible motion; assess any future finished asset
against the intended creature instead of letting a catalogue define the bestiary.

Apply that identity across the city screen, regional map, dungeon environment,
party portraits and equipment inspection. City illustration and map styling
remain to be designed; they need not reproduce a fully walkable environment.
Dungeon lighting and sound should support mystery, while the battle view must
make positions, ranges and objectives clear for the larger candidate parties.

### Rendering and the practical quality ceiling

Unreal is the working choice for this target. The comparison below records the
rendering and workflow differences considered during selection. Lighting,
materials, silhouettes, deformation and animation quality still need to be
assessed together in an actual generated dungeon on the intended hardware.

- **Godot reference:** Forward+ supports advanced lighting, volumetric fog and
  screen-space reflections that are
  absent from its Compatibility renderer. The graphics lab's renderer settings
  do not define this game's visual ceiling.
  [Renderer comparison](https://docs.godotengine.org/en/stable/tutorials/rendering/renderers.html)
- **Unity reference:** URP was the initial rendering candidate. Unity's published
  2026 strategy directs new rendering investment toward URP while maintaining
  HDRP for stability and platform support. Planned URP features must be checked
  against what has shipped in the selected engine version.
  [Unity rendering strategy](https://unity.com/topics/render-pipelines-strategy-for-2026)
- **Unreal working choice:** use the integrated rendering and animation toolset
  to support the target and validate the chosen features on Mac. The current
  documentation lists hardware-ray-traced Lumen/MegaLights as experimental and Nanite/Virtual
  Shadow Maps as beta, with hardware requirements. A feature's availability on
  Windows is insufficient evidence for this project's Mac workflow.
  [Mac feature support](https://dev.epicgames.com/documentation/en-us/unreal-engine/macos-development-requirements-for-unreal-engine)

Procedural rooms must light well in new arrangements. Define compatible material
conventions, light placement rules and an approach to indirect lighting; check
doorway seams, moving actors and enchanted equipment. A fixed scene's precomputed
lighting does not establish the quality of newly assembled locations. More
complex rendering and particle effects also need to preserve tactical clarity.

### Build creature families with reusable motion

The proposed production unit is a compatible body and animation family. Possible
families include weapon-using humanoids, quadrupeds, winged creatures, many-legged
creatures and amorphous or floating creatures. These are planning categories,
not a claim that every member can share one skeleton or every animation.

Develop original working rigs and skinning, then vary supported proportions,
meshes, materials, equipment and effects. Each family needs an appropriate action set:
idle, movement and turns, attacks, reactions, defeat and applicable special
abilities. Validate ground contact, weapon grip, hit timing, deformation and
transitions at normal speed. A generated static mesh does not establish these
capabilities; unusual anatomy may need dedicated rigging and motion.

In tactical battles, animation communicates the resolved rules and must not own
turn order, hit results or effect lifetimes. A save, a presentation unload or
any future animation-speed setting must not alter the authoritative outcome.
Measured turn-based playback still needs convincing movement, contact and
reactions; it does not remove those production requirements.

Keep editable creature masters and authored motion outside the game engine.
Engine tools remain useful for importing, playback, blending and runtime
adaptation. Godot provides skeleton profiles and bone mapping for retargeting,
while Unity's Generic animation workflow supports non-humanoids with compatible
skeletons. Unreal's IK Rig system can map animation chains across different
skeletons and help maintain hand and foot contact. Retargeting still needs
appropriate source motion, mapping and inspection for each body family.
[Godot retargeting](https://docs.godotengine.org/en/stable/tutorials/assets_pipeline/retargeting_3d_skeletons.html),
[Unity non-humanoid animation](https://docs.unity3d.com/6000.0/Documentation/Manual/GenericAnimations.html),
[Unreal IK Rig](https://dev.epicgames.com/documentation/unreal-engine/ik-rig-animation-retargeting-in-unreal-engine)

Maintain a creature catalogue connecting each stable species/variant identifier
to its model, compatible rig and action set, material variants, equipment
attachments, sounds, collision/navigation size, abilities, behavior, habitat
and fixed threat profile. Dungeon records reference these definitions and save
individual instances' state. Visual variants should support distinct roles and
ecology; colour changes alone do not supply meaningful encounter variety.

Catalogue breadth and simultaneous on-screen cost are different constraints.
Load the needed assets, share compatible resources and evaluate detail levels,
animation cost, lights, shadows and effects against representative encounters.
No maximum creature count or performance guarantee has been established.

### Adjustable characters and compatible wardrobes

The desired authoring workflow reuses approved character models with controls
for body and face proportions, compatible clothes and equipment, and editable
textures and materials. This is an internal production tool; it does not add a
new player-facing character-creator requirement.

Prepare a continuous body for each supported anatomical family. Body controls
drive authored morphs and matching joint adjustments; wardrobe choices select
garments built to fit that family. Importing an arbitrary finished model does
not automatically provide these controls. Preserve each character's identity
while constraining proportion ranges and outfit combinations to those that
deform and animate correctly.

Separate the body, hair and garments sufficiently to edit and replace them.
Removing a hood or changing a robe must reveal coherent underlying surfaces.
Garments need suitable openings, thickness, layer order, weights and clearance.
Fitting clothing to a body does not provide cloth motion or collision response;
capes, robes, hand grips and foot contact still require movement checks.

Keep stable UVs and editable texture sources for skin, fabric, armor finishes,
wear and emblems. Material and equipment variants should support the agreed
visual progression, including appropriate runes and effects without obscuring
the party or changing a character's identity.

Save a versioned character recipe containing the body/rig family and versions,
body and face parameters, hair and wardrobe asset IDs, equipment/grip profiles,
material variants, animation overrides and export settings. Rebuild variations
from those shared assets rather than regenerating a complete mesh each time.
The recipe and source assets should remain usable outside Unreal; engine
assembly and runtime materials belong in the Unreal integration.

**Proposed evaluation:** keep Blender/Rigify as the foundation, test CharMorph
for adapting an approved custom body into an adjustable family, and test
Ucupaint for layered surface editing. Evaluate Unreal's Mutable for assembling
prepared variations. MPFB remains an alternative when its compatible human
base is suitable; MetaHuman is a separate human-character conversion option.
These additional tools are candidates, not validated production dependencies.
The [character customization review](TOOLING_REVIEW.md#character-customization-and-compatible-assets)
records capabilities, free-use terms, limitations and the proposed first trial.

Begin with one visually approved body, a few useful proportion controls, two
compatible outfits and material variants. Check body-only and dressed views,
walking, shoulder movement and weapon grip, then save/rebuild and export the
same recipe. Preserve the graphics lab's rejected assembly findings when
designing this trial. A reusable recipe editor and compatibility checks are a
possible open-source contribution; extend a suitable foundation before
considering a complete new character generator.

### Portable creature-authoring tools

Research checked on 2026-09-13. The agreed free authoring direction has not been
validated for this game's original creatures. The graphics lab's bounded humanoid result
does not establish non-humanoid production coverage.

**Working foundation:** Blender with Rigify, retaining editable creature
masters and using Blender's own animation, constraints and baking tools. This
supports original production and export without a required paid add-on.

| Tool | Purpose | Relevant capability and limit |
|---|---|---|
| Blender | Free modeling, sculpting, UVs, materials, texture baking and source-asset management | Build each original creature and retain editable geometry and material sources. Deformation-ready topology and skinning still need deliberate work. |
| Rigify | Free rig generation within Blender | Generates animator controls from fitted meta-rigs and includes human, quadruped and several animal presets. More unusual anatomy requires suitable rig components or custom bones and constraints. |
| Blender animation tools | Free posing, keyframes, motion reuse, editing and export preparation | Use actions, the Graph Editor, nonlinear animation, IK constraints and baking to author complete motion sets. These tools do not automatically invent convincing motion for arbitrary anatomy. |

Rigify's [basic workflow and presets](https://docs.blender.org/manual/en/latest/addons/rigging/rigify/basics.html)
describe fitting the meta-rig, generating controls and binding the geometry.
Choose deformation features that can be baked into ordinary bones or supported
shape-key animation for export. Preserve the authoring rig separately from the
engine-ready skeleton.

**Excluded by the free-use requirement:**
[Auto-Rig Pro is paid](https://www.lucky3d.fr/auto-rig-pro/doc/download.html), and
[Cascadeur's current free tier](https://cascadeur.com/plans) exports only its
proprietary CASC format. Their required production capabilities are not free,
so neither is part of the recommended workflow. Free trials do not change this.

### Preserve a portable master and export finished motion

The proposed production flow is:

1. Define the creature's anatomy, silhouette, materials, behavior and attack
   vocabulary from its role in the game.
2. Build the model and a deformation-ready mesh in Blender, with UVs and texture
   sources retained for future changes.
3. Fit a compatible rig family or build the anatomy-specific rig. Reuse proven
   controls and motion only where the new creature's proportions and behavior
   support them.
4. Author and refine its complete action set in Blender, using reusable actions,
   keyframes and procedural constraints where suitable. Additional legs, wings,
   tails and unusual weight distribution require their own motion and contact
   checks.
5. Preserve the editable source, then export a skinned mesh, deformation
   skeleton and baked animation clips through suitable FBX or glTF/GLB presets.
   Keep stable clip names, loop/root-motion conventions, attachment definitions
   and gameplay event timing in versioned companion data.
6. Import into Unreal Engine 5.8 and check playback, scale, orientation,
   materials, transitions and runtime contact adjustments against the master.

The control rig is the animator's working apparatus; the deformation skeleton
and its finished motion are the portable deliverable. Blender's
[Bake Action](https://docs.blender.org/manual/en/4.5/editors/nla/editing/strip.html#bake-action)
can turn evaluated constraints and other motion into keyframes. Keep source
constraints and controllers editable in the master while preparing the export.

Portability does not mean that editor controls, arbitrary shader graphs,
runtime IK or animation state machines transfer unchanged. Keep meshes, texture
sources and motion reusable, with a small engine-specific integration layer for
materials, playback and gameplay events. Export settings and content versions
must be pinned once a working combination is established.

For tool evaluation, compare an original biped and a representative non-humanoid
through movement, turning, attack, reaction, defeat and relevant special actions.
Establish the Unreal baseline against the Blender master first. Retain a later
cross-engine playback comparison of the same exported source to verify the
portability contract. Species selection, exact evaluation scope and production
effort remain open.

## Audio candidates under evaluation

Research checked on 2026-09-13. These are candidates for a later audition;
no audio model has been selected, installed or benchmarked for this game.

- [Qwen3-TTS](https://github.com/QwenLM/Qwen3-TTS): Apache-2.0 models for speech,
  voice design and voice cloning. The documented design-then-clone workflow
  creates an original reference voice and reuses it across subsequent lines.
- [MOSS-TTS family](https://github.com/OpenMOSS/MOSS-TTS): Apache-2.0 models
  include voice design, dialogue synthesis and MOSS-SoundEffect for text-driven
  sound effects. Check each model's runtime separately; support for a speech
  model does not establish support for the sound-effects model.
  [MOSS-SoundEffect-v2.0](https://huggingface.co/OpenMOSS-Team/MOSS-SoundEffect-v2.0)
  documents a CUDA/Triton workflow; a usable Mac path remains to be established.
- [Kokoro-82M](https://huggingface.co/hexgrad/Kokoro-82M): an Apache-2.0 compact
  speech model to compare as a lightweight option with preset voices.
- [Chatterbox](https://github.com/resemble-ai/chatterbox): an MIT-licensed speech
  model family to compare for expressive delivery and reference-based voices.

[MLX-Audio](https://github.com/Blaizzy/mlx-audio) documents Apple Silicon
implementations for Qwen3-TTS, Kokoro and other speech models. Actual voice
consistency, pronunciation, generation speed and memory use on the project Mac
remain unverified. Sound-effects generation needs its own compatibility check.

Keep characterization in the authored character profiles and dialogue rules:
motives, knowledge, vocabulary, speech habits and current emotional context.
The chosen conversation interface uses written dialogue choices, providing
resolved dialogue text for any voice production. Speech synthesis supplies the
voice and delivery. Reuse approved voice references or presets for recurring
NPCs and companions.

Prioritize dungeon ambience, clues conveyed by sound, readable combat effects
and concise character responses. Voice coverage remains open; extensive voiced
town conversations are not a requirement of the city-screen experience.

A proposed production approach is to prepare recurring speech and sound effects
as reusable audio assets, then generate and cache complete resolved lines for
new adventures when needed. Equipment sound can develop through material,
enchantment and ability layers alongside its visual upgrades. Generation timing,
runtime packaging and voice coverage remain design decisions.

## Remaining design work

### Resolve the experience before expanding content

1. Compare simultaneous party sizes, including eight and twelve as candidates.
   Assess useful turns, battle duration, room crowding, equipment effort and
   companion identity. Set the active limit separately from the recruitable roster.
2. Specify tactical movement and action budgets, turn order, targeting, terrain,
   reactions, effect duration and enemy behavior. Decide exploration controls,
   deployment, encounter extent, retreat and mid-battle save behavior together.
3. Define city-service navigation and controls for the agreed compact, freely
   navigable overworld: traversable terrain, discovering and entering locations,
   visibility, travel time and persistent markers. Test whether alternative
   destinations and detours offer meaningful choices at this scale. City
   development, territorial systems and daily movement budgets remain undecided.
4. Test the floor rhythm with a main battle, meaningful exploration, optional
   secret content and a possible final-floor boss. Measure battle and exploration
   time, backtracking, useful discoveries and voluntary secret participation.
   One battle per floor is a chosen pattern, not a verified session-length target.
5. Develop the blacksmith/mine slice across city preparation, map travel,
   connected exploration, alternative resolutions, a tactical encounter and a
   lasting equipment reward. Use the same validated data/runtime for an authored
   first case and generated variants. Follow the tooling review's persistence,
   instance-isolation and measurement checks before adopting dependencies.

### Retained systems for later detailed design

- Six classes and twelve specializations, optional dual-classing, permanent
  class investments, mentor retraining and slower uncapped mastery remain the
  full-game direction. Design their abilities and progression for tactical turns.
- Develop companions' identities, recruitment, concise personal events,
  relationships and reconciliation with the eventual roster size in mind.
- Keep prepared spells, supplies, refuges, camps, wounds and defeat/reload rules;
  tune their budgets around sparse major battles and optional secret risk.
- Define signature equipment, branches, recipes, special secret rewards and
  visible upgrades. Test convenience across the entire active party.
- Write the town, factions and ancient-route mystery; deliver their consequences
  through city services, map changes, discoveries and short meaningful choices.
- Define the skill/action vocabulary, discovery clues, optional risks and
  clearly announced deadlines. Secret content must remain optional for progress.
- Develop dungeon families, compatible rooms, battle geometry and generation
  constraints. Measure saving, loading, memory and rendering on the intended Mac.
- Retain Unreal 5.8, Blender/Rigify and the free-tool/open-source boundaries.
  Validate original creatures, motion and equipment first, then evaluate the
  existing customization and audio candidates within their recorded limits.
- Define reusable public-tool interfaces, licenses and the Unreal adapter after
  the acceptance slice supplies evidence. No inspected reference is a verified
  dependency for the new gameplay combination.

The main experience and the explicitly agreed retained rules are the baseline.
Exact party size, turn rules, floor dimensions, dungeon count, session length,
first-release content breadth and production estimates remain open. The earlier
proposal to postpone broad generation or progression systems is a sequencing
option, not a removal of those full-game goals or an approved implementation plan.
