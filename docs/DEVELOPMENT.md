# Development flow

The first executable slice is the blacksmith's mine. [Game rules](GAME_CONCEPT.md), [tooling evidence](TOOLING_REVIEW.md) and the [ADRs](adr/0001-authoritative-session.md) explain the boundaries. Original content lives in `game/`, the portable C++20 rules in `core/`, headless hosts and checks in `tools/`, the Unreal adapter in `Unreal/`, and pinned external source in `third_party/`.

## Prepare a machine

Install CMake 3.25 or newer, a C++20 compiler, Python 3.12 or newer, clang-format 21, clang-tidy 21 and Ruff 0.16.8. On this Mac, Unreal Engine 5.8.2 is installed at `/Volumes/UE_5_8_APFS`; Xcode 26.6 supplies the compiler and SDK. The Xcode editor is optional. The Editor target compiled and its automation test passed with 26.6 on 2026-09-20. Epic recommends Xcode 26.1.1 for UE 5.8, so retain 26.1.1 as the fallback if a later engine build fails. Do not assume another Xcode version works without a project build.

On macOS, configure the portable library for Unreal before compiling the Unreal target. The `ue-release` preset sets `ODR_SHARED=ON` and produces `build/ue-release/libodr_core.dylib`; the Unreal external module stages that dylib with the packaged game. The earlier monolithic packaged Mac binary aborted while unwinding a rejected-command exception, so the shared-library boundary is the current Unreal-facing deployment. Build and cook from an APFS checkout. This repository currently sits on an external volume that creates AppleDouble `._*` files in `.app` bundles; those files caused Xcode's local ad-hoc signing to fail. A runner checkout on APFS avoids that issue. No signing identity or secret is required for a local Development package.

The external module copies its dylib beside the binary that uses it with `$(BinaryOutputDir)`, following [Epic's runtime dependency rules](https://dev.epicgames.com/documentation/en-us/unreal-engine/integrating-third-party-libraries-into-unreal-engine). For Editor builds this is the project module directory. `$(TargetOutputDir)` points at the engine executable and previously left a stale project dylib loading against newer content. The Mac script compares the built and Editor-staged library bytes before running automation.

## Exact local checks

Run from the repository root:

```sh
python3 tools/stage_content.py
python3 tools/stage_content.py --check
python3 tools/check_links.py
ruff check tools
ruff format --check tools
cmake --preset dev
cmake --build --preset dev -j4
python3 tools/check_cpp.py
ctest --preset dev
cmake --preset sanitizers
cmake --build --preset sanitizers -j4
ctest --preset sanitizers
cmake --preset ue-release
cmake --build --preset ue-release -j4
git diff --check
```

`check_cpp.py` locates LLVM 21 or Apple's clang-format 21 and supplies Xcode SDK headers to clang-tidy on macOS. The pinned Ruff command used here when no global installation is present is `UV_CACHE_DIR=/private/tmp/odr-uv-cache UV_TOOL_DIR=/private/tmp/odr-uv-tools uvx --from ruff==0.16.8 ruff`. Format commands in the check list inspect files without rewriting them. To intentionally format owned code, run `clang-format-21 -i` on changed owned portable C++ files and `ruff format tools`, then rerun the checks. Never format vendored code or Unreal engine code in this pass. The checker discovers owned portable headers and sources under `core/`, `tools/` and `tests/`, excluding macOS AppleDouble files. Ruff also checks nested asset-development scripts.

The test categories are: portable command and layout scenarios (`ctest`), sanitizer scenarios, Unreal automation (`ODR.BlacksmithMine` and `ODR.PlayerAdapter`), and packaged unattended smoke (`-odrsmoke`). The versioned [eight-character scenario](../game/scenarios/blacksmith_mine.jsonl) is replayed by the headless tool and Unreal; a [twelve-character scenario](../game/scenarios/blacksmith_mine_12.jsonl) measures the larger roster. The Unreal-facing check must use the staged shared dylib; a monolithic packaged Mac core is not a supported deployment. To replay or measure them headlessly:

```sh
build/dev/odr_tool --script game/scenarios/blacksmith_mine.jsonl --save build/mine-save.json
build/dev/odr_tool --load build/mine-save.json < /dev/null
cmake --preset release
cmake --build --preset release -j4
python3 tools/measure.py --repeats 3
```

For Unreal on this Mac from an APFS checkout, set `UE_ROOT=/Volumes/UE_5_8_APFS` in your shell, then run `bash tools/run_mac_ci.sh`. The script executes these exact commands:

```sh
cmake --preset ue-release
cmake --build --preset ue-release -j4
python3 tools/stage_content.py
"$UE_ROOT/Engine/Build/BatchFiles/Mac/Build.sh" OpenDynamicRPGEditor Mac Development -project="$PWD/Unreal/OpenDynamicRPG.uproject" -waitmutex -NoHotReload
"$UE_ROOT/Engine/Binaries/Mac/UnrealEditor-Cmd" "$PWD/Unreal/OpenDynamicRPG.uproject" -unattended -nullrhi -nosplash -ExecCmds='Automation RunTests ODR.;Quit' -ReportExportPath="$PWD/build/reports/unreal"
python3 tools/check_unreal_report.py
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun -project="$PWD/Unreal/OpenDynamicRPG.uproject" -platform=Mac -clientconfig=Development -build -cook -stage -pak -package -archive -archivedirectory="$PWD/build/package" -unattended -nop4
"$PWD/build/package/Mac/OpenDynamicRPG.app/Contents/MacOS/OpenDynamicRPG" -odrsmoke -unattended -nullrhi -nosplash -stdout -FullStdOutLogOutput > build/packaged-smoke.log 2>&1
rg 'ODR_SMOKE_PASS' build/packaged-smoke.log
```

Open `Unreal/OpenDynamicRPG.uproject` in Unreal Editor or run the packaged
Development game. The [presentation guide](PRESENTATION.md) describes the original
diorama assets, import recipe, mouse interface, camera and authority boundary.
Follow the [mouse-first playtest](PLAYTEST.md) through creation, recruitment,
travel, mine combat, ore recovery and the forge upgrade. Save/Load buttons and
F5/F9 persist `session.json` under the project's Saved directory. The packaged
Mac default is under
`~/Library/Containers/com.YourCompany.OpenDynamicRPG/Data/Library/Application Support/Epic/OpenDynamicRPG/Saved/`.
For an isolated manual pass, give the binary a unique `-UserDir` inside that app
container; the packaged sandbox cannot write an arbitrary `/private/tmp` UserDir.
Do not overwrite a player's existing save for automation.

Character bindings in `game/content/visuals.json` support cooked `model`,
`material`, optional `animation_class` or `animation` sequence paths and a
`fallback_mesh`. `mesh_type: skeletal` selects the skeletal path; static bindings
can omit it. Visual substitution does not change saves or combat geometry.
The Mac report gate requires both Unreal integration tests and fails if either
is missing; rendered pointer usability remains a separate check.

Original imported models live under `Unreal/Content/Art/`, which is explicitly cooked because JSON paths are not Unreal asset dependencies. Keep the editable source and reproduction recipe under `ArtSource/`.

## Content, generated adventures and saves

Edit `game/content/mine.json` for the first template, `game/content/quarry.json` for the second data-defined example and `game/content/visuals.json` for presentation. The [content guide](CONTENT.md) describes the fields, validation and current limits. Run `python3 tools/stage_content.py` after changes. Both authored and generated runs receive a resolved layout and pass the same validation. `odr_tool` can play a JSONL command script to review the accepted layout, room connections, objective coordinates and event results in its printed snapshot. For a generated run, use `{"action":"create_run","mode":"generated","seed":42}` after hero creation; compare other seeds and confirm that each run ID has independent `changes` and enemy records. Rejected candidates report objective or deployment diagnostics; after at most sixteen attempts the session accepts a validated authored fallback and records `layout_source: authored_fallback`. A standalone failed `validate_layout` command leaves the prior state untouched. A `boss: true` run has an upper floor and a separate lower main-encounter floor; `descend` and `ascend` use its stairs.

Approach-dependent deployment is part of the accepted gameplay state. Dungeon movement updates `mine_previous_pos`; when the party reaches an encounter trigger, the prior walkable hex is preferred as the battle entry approach, with a validated fallback neighbor only when necessary. The battle snapshot stores both `approach` and `trigger`, so deployment and retreat use the same entry route. Retreat returns the party to `approach` and records the trigger as the next previous position. These fields are saved with the canonical snapshot and must survive load/replay.

Content and save JSON now use `schema_version: 2`. Saves preserve `template_id`, run IDs, resolved enemies and rewards, the accepted layout and all changes. Version 1 prototype saves are explicitly rejected; there is no migration, so start a new expedition. [ADR 0006](adr/0006-content-owned-encounters.md) records the format change. Future schema changes require a migration or explicit rejection, a before/after fixture and a roundtrip test. Never recover a layout only from a seed. An action or animation may be interrupted by a save; the canonical state is the portable snapshot, and visual playback must not write rules.

## Luna implementation and Sol review

Use Luna for a bounded, testable implementation slice: state the affected files, acceptance example, architecture constraints and exact checks, then let it implement and run them. Keep slices small enough that the diff and test evidence can be reviewed together. After local checks pass, give Sol the complete diff once for a focused review of rule ownership, save/reload behavior, content validation, generated/adventure identity, test gaps, CI trust changes and license boundaries. Ask for prioritized actionable findings with file/line references. Send those findings back to Luna for repairs; request a targeted Sol re-review only if a fix changes a risky boundary. A failed check goes straight back to implementation before review. Do not spend Sol turns on routine formatting or repeated status polling. Model billing or Codex usage savings are not guaranteed by this workflow; measure actual usage in the account if cost matters.

Prompts for the two passes can be concise:

```text
Luna: Implement <one slice> within the existing ADRs. Add one behavior-focused scenario, run the exact relevant checks, and report changed files, results and remaining risks.
Sol: Review this completed diff and check output for correctness and omissions. Prioritize authoritative-state, save/layout identity, generated fallback, gameplay acceptance and CI runner trust. Return concrete findings only; do not rewrite the implementation.
```

Do not create a separate Codex task automatically for every pass. When the user authorizes parallel agent work, bounded Luna implementation and Sol review can stay within the current task. The initial foundation received a consolidated Sol review and a targeted re-review. The 2026-09-22 hardening pass again used Luna implementation and consolidated Sol review, followed by focused review of save and generated-layout repairs. Routine formatting and build failures stayed with implementation. Record findings and actual check output rather than treating a model's approval as validation.

## Mac validation and runner boundary

The repository-scoped macOS ARM64 runner `odr-mac-mini` was registered on 2026-09-20 and verified online with the `odr-mac` label. Two project PR runs passed the portable Linux checks and the Unreal Editor/package/smoke checks on this runner. Sol review then identified a public-fork workflow bypass: a fork can change the `pull_request` workflow and request a repository-scoped self-hosted runner even if the original Mac job has a trust condition. Its public registration was removed; the same installation is now registered only with the private `simonperegrinvs/open-dynamic-rpg-ci` repository. The public workflow uses GitHub-hosted Linux only. [ADR 0005](adr/0005-ci-trust-boundary.md) records the decision; do not re-register this runner with the public repository while fork PR workflow changes can reach it.

Run `bash tools/run_mac_ci.sh` from an APFS checkout for local Mac validation and record the tested commit SHA in the review. Runner 2.337.0 is installed at `/Users/simon/.local/share/odr-actions-runner` on APFS; its downloaded archive matched the SHA-256 recorded in [TOOLS.md](TOOLS.md). It runs under the `simon` account and has no signing secrets. A dedicated account would require macOS administrator credentials. The private workflow can be dispatched manually with PR number and exact head/merge SHAs; it verifies those refs before building and has read-only workflow permissions. Automatic dispatch and a public required Mac status still need a least-privilege credential design. Automatic approval review rejected a proposed persistent watcher with Actions write access and a status token, so that path was not installed. `main` protection currently requires `linux-portable` only; the private run URL and SHA are review evidence, not a PR status check.

When a Mac check fails, inspect Unreal's `Saved/Logs`, `build/reports/unreal/index.json`, the packaged smoke log, Xcode selection (`xcode-select -p`) and free disk space. Verify the portable `ue-release` library exists before Unreal Build Tool runs. In the runner installation, `./svc.sh status` checks the private repository's user service; `gh api repos/simonperegrinvs/open-dynamic-rpg-ci/actions/runners` checks its online status. If `._*` files appear in a package or source, move the checkout to APFS and clean generated directories. If automation appears idle after a test, use the semicolon form `Automation RunTests ODR.;Quit`; a separate comma-separated console `Quit` can stop or hang at the wrong time. Restart or update the service only after draining jobs, then manually verify one exact-commit private run.

## Current measured limits

On this M4 Pro Mac (64 GB), the 2026-09-22 portable dev scenarios, formatting/static analysis, Python checks, content checks and documentation links passed. Address/undefined-behavior sanitizer scenarios passed in 26.89 seconds. Unreal 5.8.2 built with Xcode 26.6; both `ODR.BlacksmithMine` and `ODR.PlayerAdapter` passed with zero errors. The final Development cook/package and unattended `ODR_SMOKE_PASS` passed. These were local checks against this implementation; CI results must identify their own tested commit.

The later 2026-09-22 presentation pass exported and imported an original kit of fifteen static props and one skeletal adventurer with an idle sequence. The material importer explicitly enables instanced and skeletal usage and writes material slots back to the imported assets. Runtime bindings now play the idle sequence. Unreal automation loads the bound character, replaces it with primitives and rebuilds the presentation without changing canonical state; the packaged smoke requires the cooked skeletal model to load. Production Rigify/retargeting remains unvalidated. See [PRESENTATION.md](PRESENTATION.md) for ownership and reproducible art commands.

The diorama pass also added passing adapter assertions for axial coordinates, weighted path previews, queued travel cancellation when saving, legal battle movement and refusal to redirect an incompatible explicit target. Rendered town and battle views were inspected locally. The Mac synthetic click tool supplied an unchanged system cursor position, so a real mouse pass is still pending; no mouse-only expedition completion is claimed. Updated frame and memory measurements, twelve-character HUD readability and a human pacing pass also remain to be recorded for this presentation.

The following measurements were refreshed on 2026-09-22 for schema 2, after the save and content repairs. Three Release-build headless replays per party size produced these medians; `peak RSS` is the headless process, and the battle figures come from the deterministic test driver. `movement-only` means a party turn spent moving and defending without an attack. It is a navigation-pressure indicator, not a claim about human player mistakes. The raw result is in ignored local `build/measurements.json` and can be regenerated with `tools/measure.py`.

| Active party | Layout generation | Save JSON | Load | Peak RSS | Replay commands to upgrade | Main-battle party turns / movement-only / rounds | Battle-driver time |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 8 | 1.95 ms | 6,388 bytes | 1.93 ms | 2.95 MiB | 95 | 19 / 8 / 3 | 70 ms |
| 12 | 1.82 ms | 7,212 bytes | 2.04 ms | 2.98 MiB | 108 | 23 / 11 / 2 | 114 ms |

Before the diorama presentation change, both eight- and twelve-character authored expeditions completed through ordinary keyboard controls in the rendered primitive package, from creation to equipment upgrade. The saved battle and completed states matched a portable replay of the corresponding input commands. The eight-character pass saved/reloaded between movement and action, saved immediately during action presentation, and rejected a duplicate upgrade without changing state. This was agent-driven functional testing, not a human pacing or enjoyment assessment, and does not establish the later mouse HUD's behavior.

One 300-frame idle main-battle capture per party size used Unreal's console command `CsvProfile FRAMES=300` at 1440×900 windowed resolution in the earlier primitive presentation. These frame and memory figures do not describe the new diorama. Raw CSVs and the summary remain in ignored local `build/ui-eight-frames.csv`, `build/ui-twelve-frames.csv` and `build/render-measurements.json`. RSS is a separate process snapshot, not peak memory.

| Active party | Median frame | 95th percentile | Maximum frame | Process RSS |
| --- | ---: | ---: | ---: | ---: |
| 8 | 16.72 ms | 40.54 ms | 88.00 ms | 358.6 MiB |
| 12 | 16.66 ms | 17.27 ms | 49.49 ms | 374.7 MiB |

These short captures include the console transition and uncontrolled desktop load. Their variance does not establish that twelve characters is faster, nor a supported party cap. Repeat warm captures and measure scene-rebuild/action spikes before choosing that limit. The 30–45 minute human expedition, readability preferences and difficulty still need a playtest. Generated, secret and boss variants pass portable scenarios; they have not received the same complete rendered keyboard pass. The lower boss floor reuses compact geometry with a distinct floor identity; richer floor composition and production animation remain future work.

For isolated packaged Mac playtests, put `-UserDir` under the game's existing sandbox container, for example `~/Library/Containers/com.YourCompany.OpenDynamicRPG/Data/ODRPlaytest/<unique-run>/` (expand `~` before passing the argument). The Mac app sandbox blocks an arbitrary `/private/tmp` user directory; that produces a save failure rather than a portable-state problem. Keep these test saves separate from the default user save.
