# ADR 0001: One portable authoritative session

Status: accepted, 2026-09-20.

## Context and alternatives

Party, combat, quest, world, inventory and time must survive presentation changes and be testable without Unreal. Alternatives were Unreal actors as owners, separate quest/combat save stores, or one portable session.

## Decision

`core/` owns the C++20 session. Hosts send JSON commands and receive canonical JSON snapshots and events through a small C ABI. `game/` owns definitions; `Unreal/` owns input, rendering and file I/O. A rejected command restores the previous state. The headless tool and Unreal use the same library and content. The Unreal-facing build is a shared `libodr_core.dylib` so exception unwinding across the packaged Mac host boundary is contained in the portable library; the `ODR_SHARED=ON` build is staged as an Unreal runtime dependency.

## Consequences

Presentation can be unloaded or replaced without changing outcomes. Core commands are versioned data boundaries; saves need validation and migration as schemas evolve. The current UI is intentionally thin and serializes a full snapshot after each command; profiling must guide later optimization.
