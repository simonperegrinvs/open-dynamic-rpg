# Placeholder expedition acceptance

Use the packaged Development game built from the revision under test. Keep the
party at eight for the first pass and repeat at twelve before choosing a cap.
Record the revision, package path, party size and whether the run was authored,
generated or the lower-floor boss variant. Engineering replay times do not
measure the intended 30–45 minute player expedition.

## Ordinary controls

Use the mouse for the primary pass. Choose a class, ancestry and background,
then **Begin as Ari**. In town, recruit seven companions, select **Party of 8**,
accept the mine expedition, then **Take the road**. The destination buttons
walk to the mine, town or optional ruins; **Enter** enables on arrival.

Inside the mine, click empty hexes to walk; hover previews the route. Approach
landmarks until their contextual buttons enable. In battle, the teal outline
and initiative row identify the acting character. Click a unit to select it,
click terrain to move, then choose an action. Gold outlines identify the selected
target. Health bars and the bottom party strip show damage. Save and Load work
through the upper-right buttons, including between movement and action.

Wheel zooms, middle-drag pans, right-drag rotates, Home resets the camera and
Escape cancels pending travel/selection. Verify panel clicks never move the party.
Repeat at 1280-by-720 if readability or pointer alignment looks wrong.

Keyboard shortcuts remain available for comparison: 1–6 selects class, C cycles
ancestry, B cycles background and Enter creates the hero. N recruits, M accepts
authored content, G accepts generated content, P activates all recruits, X
enters/leaves, and Q/W/E/A/S/D move one adjacent hex.

In battle, T cycles enemy targets and Y cycles wounded allies for a Cleric.
F attacks, C casts, Z uses a Mage's area spell, Space defends and R retreats
when the core's entry-zone requirements are met. Check the current actor,
remaining movement and selected target before acting. Incompatible explicit
targets must not redirect to another character. F5 saves and F9 loads.

## Required pass

1. Recruit eight characters, accept the mine and leave town. Detour to the ruins
   and return to the same mine run. Verify blocked and costly terrain is visible.
2. Explore the clue and cache. A hidden cache must not be labeled across the map;
   collected loot disappears. Discover the secret branch and verify its marker
   appears, without needing its rewards for the main objective.
3. Begin the main encounter from the route actually taken. Identify the acting
   character and the retreat entry. Select a different enemy and attack it. Use
   a Cleric to heal a wounded ally; a full-health party must not spend a spell.
4. Save after movement and before the action, then reload. Compare positions,
   HP, spell uses, acting character, enemies and terrain. Save again during the
   brief action presentation and confirm that its resolved outcome is retained.
5. Exercise retreat, a town visit and re-entry using a separate save. Damage and
   defeated enemies must remain changed. Reloading must retain the accepted map.
6. Win the required fight, recover ore, return and upgrade. Repeat the upgrade
   input and reload; neither should grant another reward.
7. Repeat with a generated run and test the optional secret battle and lower
   boss floor. Repeat the main loop with twelve active characters.

## Record separately

Measure exploration time, battle time, movement-only turns, turns blocked by
other actors, backtracking and equipment decisions. Note where a player cannot
tell what to do or why an action failed. Record rendered frame times and process
memory independently from headless generation/load/save measurements. Preserve
raw reports under ignored `build/` and summarize measured limits in
[DEVELOPMENT.md](DEVELOPMENT.md#current-measured-limits).

The automated tests establish repeatable rule and adapter behavior. A person
must still judge readability, discovery, enjoyment and the intended expedition
length; a rapid scripted pass does not establish those outcomes.
