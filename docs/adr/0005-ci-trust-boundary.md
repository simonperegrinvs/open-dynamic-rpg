# ADR 0005: CI trust boundary

Status: accepted, 2026-09-20.

## Context and alternatives

Linux can cheaply verify portable rules. Unreal needs a local Mac with the installed engine. A self-hosted runner executes repository code on the Mac and must not run untrusted fork code.

## Decision

Linux checks run for every pull request. The Mac job uses a repository-scoped, dedicated runner and is gated to same-repository `codex/` branches and pushes to `main`. Only trusted maintainers may push those branches. Workflow permissions are read-only; no signing credentials are used. Runner registration and branch protection are manual prerequisites before the Mac job becomes a required check.

## Consequences

Fork PRs receive portable checks but no Mac execution. A maintainer must move reviewed code to a trusted branch to obtain Mac verification. CI workflow changes themselves deserve Sol review before running on the Mac. The runner is not registered as of 2026-09-20.
