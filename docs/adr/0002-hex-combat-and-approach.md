# ADR 0002: Hex combat and approach deployment

Status: accepted, 2026-09-20.

## Context and alternatives

The party needs individual turns with movement, range, obstacles and meaningful entry/retreat. We considered square grids, free movement with measured distances, and hexes.

## Decision

Axial hex coordinates define collision, movement cost, occupancy and line of sight. Each character receives movement followed by one action in initiative order. Battle deployment is computed around the approach reached in exploration: dungeon movement retains `mine_previous_pos`, and a battle records its selected `approach` plus the encounter `trigger`. A retreat succeeds only while living allies are in the entry zone; enemy damage and position persist, with the party returning to the recorded approach.

## Consequences

Visual meshes cannot alter combat reach or pathing. The first prototype has attack, prepared spell, defense, poison and terrain cost; it does not yet have a full ability/area library. Eight active characters are the default; twelve are supported for measurement, not declared as the final limit.
