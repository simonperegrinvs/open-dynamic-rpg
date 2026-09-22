# ADR 0006: Content-owned encounters and resolved rewards

Status: accepted, 2026-09-22.

## Context and alternatives

The first mine embedded its enemy roster, terrain recipe and rewards in the
portable implementation. That made a second adventure a code change and allowed
updated content to change the meaning of an accepted run. Alternatives were
separate engine scripts per adventure, executable content plugins, or versioned
data interpreted by the existing session.

## Decision

Content schema 2 owns recruit presets, objective positions, terrain recipes,
encounter rosters and reward values. Mine and quarry definitions use the same
commands, generator and validator. The core validates definitions at creation;
both authored and generated layouts must also pass physical and deployment
validation before acceptance.

Save schema 2 contains the accepted layout, resolved enemy records, bindings,
reward quantities and subsequent changes. Loading an accepted run does not
rebuild it from a changed definition. Main and secret encounters retain separate
occupancy because they never coexist. The owning session still requires the
same template ID. Version 1 prototype content and saves are rejected explicitly;
we do not silently invent missing resolved values.

## Consequences

New variations of this expedition pattern can be tested without editing the
core. The quarry provides a second example with different enemies, terrain,
objectives and rewards. This is not yet a general quest graph: the city upgrade
loop, room roles, encounter slots and floor-transition pattern remain rules.
The next distinct adventure pattern should drive that abstraction, with a
behavioral acceptance scenario. Production save compatibility will require a
migration policy before player releases.
