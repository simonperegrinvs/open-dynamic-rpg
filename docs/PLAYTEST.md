# Placeholder expedition acceptance

Use the packaged Development game built from the revision under test. Keep the
party at eight for the first pass and repeat at twelve before choosing a cap.
Record the revision, package path, party size and whether the run was authored,
generated or the lower-floor boss variant. Engineering replay times do not
measure the intended 30–45 minute player expedition.

## Ordinary controls

Create the hero with 1–6 (class), C (ancestry), B (background), then Enter.
In town, N recruits a companion, M accepts an authored mine and G accepts a
generated mine. P activates all recruited characters for the twelve-person
comparison. X enters or leaves the nearby destination. Q/W/E/A/S/D move one hex.
The footer lists the available exploration and battle actions.

In battle, T cycles enemy targets and Y cycles wounded allies for a Cleric.
F attacks, C casts, Z uses a Mage's area spell, Space defends and R retreats
when the core's entry-zone requirements are met. Check the current actor,
remaining movement and selected target before acting. A star marks the acting character; > marks the selected target. Enemy map labels E1/E2 remain stable during the encounter; the target HUD gives the full name and HP. F5 saves and F9 loads.

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
