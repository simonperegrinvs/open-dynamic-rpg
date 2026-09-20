# ADR 0005: CI trust boundary

Status: accepted, 2026-09-20.

## Context and alternatives

Linux can cheaply verify portable rules. Unreal needs a local Mac with the installed engine. A self-hosted runner executes repository code on the Mac and must not run untrusted fork code.

## Decision

Linux checks run for every pull request. The Mac job uses a repository-scoped runner and is gated to same-repository `codex/` branches and pushes to `main`. Only trusted maintainers may push those branches. Workflow permissions are read-only; no signing credentials are used. The runner is registered as a `launchd` user service on this Mac, and `main` branch protection requires both Linux and Mac checks from an up-to-date commit.

## Consequences

Fork PRs receive portable checks but no Mac execution. A maintainer must move reviewed code to a trusted branch to obtain Mac verification. CI workflow changes themselves deserve Sol review before running on the Mac. The runner currently uses the owner's account rather than a dedicated account; isolate it before expanding repository write access. The runner was verified online on 2026-09-20, but its first required pull-request run must pass before this boundary is considered operationally proven.
