# ADR 0005: CI trust boundary

Status: accepted, revised 2026-09-20.

## Context and alternatives

Linux can cheaply verify portable rules. Unreal needs a Mac with the installed engine. This repository is public and belongs to a personal GitHub account. A repository-scoped runner successfully ran this project's Unreal build, Editor automation and packaged smoke test. Sol review then found that a fork pull request can change the `pull_request` workflow itself and request that runner, bypassing a job-level trust condition. [GitHub documents](https://docs.github.com/en/actions/reference/security/securely-using-pull_request_target) that `pull_request` workflows run from the proposed merge commit; [its runner guidance](https://docs.github.com/en/actions/reference/security/secure-use) warns against self-hosted runners in public repositories. A hosted gate job cannot restrict access to a repository-scoped runner.

Alternatives considered: keep the runner with a job-level PR condition; use a `pull_request_target` Mac job from the default branch; move the runner to a private companion CI repository; move the project to an organization with a workflow-restricted runner group; or keep Mac validation local until an isolated CI design is installed. The first option is vulnerable because the condition is in untrusted workflow code. The second still leaves the repository-scoped runner available to other workflows proposed by forks. A private companion or restricted group can separate runner access from public PR workflow changes, but needs its own setup and validation.

## Decision

Public pull requests run portable Linux formatting, linting, builds, CTest scenarios and sanitizer checks on GitHub-hosted compute. The public repository's Mac runner registration was removed and its service stopped after successful project integration runs; no workflow in this repository requests a self-hosted runner. The same Mac build, Editor automation, package and smoke sequence is captured in `tools/run_mac_ci.sh` and remains a local review step for now. Restore an automated required Mac PR check only after runner access is restricted outside the control of public PR workflow changes. The workflow uses read-only permissions and no signing credentials.

## Consequences

The `main` protection rule should require the Linux check while this interim state is active; it must not claim an automated Mac result. A PR cannot honestly be marked Mac-verified merely because an earlier revision passed. Local Mac results and their commit SHA must be recorded during review. The former runner 2.337.0 installation remains on APFS for reuse, but is no longer registered with the public repository. It ran under the owner's account; a dedicated account is preferable before future always-on operation. The private-companion and organization-group options remain open until a safe boundary is chosen and tested.
