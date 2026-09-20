# ADR 0004: Save and visual boundaries

Status: accepted, 2026-09-20.

## Context and alternatives

Models may be produced later or in parallel. Animation and mesh loading must not determine rules or block a save. Alternatives were Unreal asset references in saved gameplay state or independent visual IDs.

## Decision

The session stores gameplay hex positions, state and stable `visual_id` strings only. `game/content/visuals.json` maps those IDs to optional static or skeletal model paths, materials, attachment points and animation roles, with primitive fallbacks. A skeletal binding may name an animation class; a missing or unloaded model falls back to a static primitive. Unreal resolves bindings after receiving a snapshot and draws marker labels in the HUD at projected world positions. The host writes save JSON to a temporary file and renames it after successful write.

## Consequences

Model replacements and unloaded presentation preserve canonical state. The prototype shows a short HUD event playback after a command; it never gates another command or a save. Later animation queues may consume events but must not decide an outcome or delay a save. Save schema changes require explicit migrations or rejection with a useful error.
