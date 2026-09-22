# ADR 0005: CI trust boundary

Status: accepted, revised 2026-09-20.

## Context and alternatives

Linux can cheaply verify portable rules. Unreal needs a Mac with the installed engine. This repository is public and belongs to a personal GitHub account. A repository-scoped runner successfully ran this project's Unreal build, Editor automation and packaged smoke test. Sol review then found that a fork pull request can change the `pull_request` workflow itself and request that runner, bypassing a job-level trust condition. [GitHub documents](https://docs.github.com/en/actions/reference/security/securely-using-pull_request_target) that `pull_request` workflows run from the proposed merge commit; [its runner guidance](https://docs.github.com/en/actions/reference/security/secure-use) warns against self-hosted runners in public repositories. A hosted gate job cannot restrict access to a repository-scoped runner.

Alternatives considered: keep the runner with a job-level PR condition; use a `pull_request_target` Mac job from the default branch; move the runner to a private companion CI repository; move the project to an organization with a workflow-restricted runner group; or keep Mac validation local. The first option is vulnerable because the condition is in untrusted workflow code. The second still leaves the repository-scoped runner available to other workflows proposed by forks. A private companion separates runner access from public PR workflow changes, but automated dispatch and public status reporting need separate credentials and review.

## Decision

Public pull requests run portable Linux formatting, linting, builds, CTest scenarios and sanitizer checks on GitHub-hosted compute. The public repository's Mac runner registration was removed after successful project integration runs; no workflow in this repository requests a self-hosted runner. That runner is now registered only with the private `simonperegrinvs/open-dynamic-rpg-ci` repository. Its manually dispatched read-only workflow verifies an exact same-repository PR head and merge SHA before running `tools/run_mac_ci.sh`; no cross-repository status credential or signing credential is installed. A proposed persistent watcher and status-token workflow was rejected by automatic approval review and was not installed.

## Consequences

The `main` protection rule requires Linux only while this interim state is active; it must not claim an automated Mac PR result. Record the private run URL and exact tested commit during review. A PR cannot honestly be marked Mac-verified merely because an earlier revision passed. The runner 2.337.0 installation remains on APFS, registered only to the private repository, and still runs under the owner's account. A dedicated account is preferable before future always-on operation. A required automated Mac status can be added only after a separately approved least-privilege credential and dispatch design has been implemented and tested.
