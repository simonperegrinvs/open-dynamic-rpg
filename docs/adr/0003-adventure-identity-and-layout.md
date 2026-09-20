# ADR 0003: Template and run identity with validated layouts

Status: accepted, 2026-09-20.

## Context and alternatives

One story pattern can be instantiated repeatedly with different claimants, sites and rewards. Saving only a random seed would change a run when the generator changes. Keying progress by template would mix separate expeditions.

## Decision

The authored template has a stable `template_id`; every accepted run receives its own `run-*` ID. Authored and generated runs use the same layout schema and validator. The save contains the accepted resolved layout, object bindings, room links, enemy changes and rewards. Validation checks objective reachability, required room links, enemy-spawn walkability and collision, and connected party deployment from each reachable approach before acceptance and on load. Battle startup rechecks deployment against living enemy positions.

## Consequences

Generator and quest rules can evolve without silently reshaping an accepted run. Rejected candidates keep per-attempt diagnostics and generation tries at most sixteen candidates before a validated authored fallback. The current generator varies a compact fixed geometry; richer floor composition must retain these contracts.
