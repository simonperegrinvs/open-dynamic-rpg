# ADR 0007: Interactive presentation over the portable session

Status: accepted, 2026-09-22.

## Context and alternatives

The original primitive grid and keyboard command list proved integration but
were too crude for a useful player pacing test. We considered waiting for
production models, importing a third-party fantasy pack, and building an original
small diorama kit with direct pointer controls.

## Decision

Build original, reproducible Blender stand-ins and import them as separately
owned Unreal presentation assets. Use a small Canvas HUD for the current C++
prototype with a transparent Slate pointer-event surface, clickable choices,
target inspection, health, initiative and
contextual actions. Keep it outside the portable library. Revisit widget tooling
when screen count, accessibility, localization or content authoring outgrow it.

Exploration queues adjacent commands; battle sends a single movement command.
Route preview, camera controls, selection, idle animation and HUD state are
transient presentation. Save cancels pending travel and serializes the reached
authoritative state. Pointer actions never silently substitute another target.
Meshes do not supply collision or line of sight. Batch static scenery separately
from actors so each command does not reconstruct the whole floor.

## Consequences

The game can be inspected and played without waiting for final art. The original
kit has no external asset license dependency and can be replaced incrementally.
The current humanoid is not a production character foundation. Advisory path
previews and action availability must remain aligned with portable rules through
focused tests; the core always validates actual commands. Headless adapter tests
cannot prove that pointer coordinates, colors or text work in a rendered window,
so visible packaged playtests remain a separate acceptance step.
