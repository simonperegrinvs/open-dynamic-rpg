# Development flow

The first executable slice is the blacksmith's mine. [Game rules](GAME_CONCEPT.md), [tooling evidence](TOOLING_REVIEW.md) and the [ADRs](adr/0001-authoritative-session.md) explain the boundaries. Original content lives in `game/`, the portable C++20 rules in `core/`, headless hosts and checks in `tools/`, the Unreal adapter in `Unreal/`, and pinned external source in `third_party/`.

## Prepare a machine

Install CMake 3.25 or newer, a C++20 compiler, Python 3.12 or newer, clang-format 21, clang-tidy 21 and Ruff 0.16.8. On this Mac, Unreal Engine 5.8.2 is installed at `/Volumes/UE_5_8_APFS`; Xcode 26.6 supplies the compiler and SDK. The Xcode editor is optional. The Editor target compiled and its automation test passed with 26.6 on 2026-09-20. Epic recommends Xcode 26.1.1 for UE 5.8, so retain 26.1.1 as the fallback if a later engine build fails. Do not assume another Xcode version works without a project build.

On macOS, configure the portable library for Unreal before compiling the Unreal target. The `ue-release` preset sets `ODR_SHARED=ON` and produces `build/ue-release/libodr_core.dylib`; the Unreal external module stages that dylib with the packaged game. The earlier monolithic packaged Mac binary aborted while unwinding a rejected-command exception, so the shared-library boundary is the current Unreal-facing deployment. Build and cook from an APFS checkout. This repository currently sits on an external volume that creates AppleDouble `._*` files in `.app` bundles; those files caused Xcode's local ad-hoc signing to fail. A runner checkout on APFS avoids that issue. No signing identity or secret is required for a local Development package.

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

`check_cpp.py` locates LLVM 21 or Apple's clang-format 21 and supplies Xcode SDK headers to clang-tidy on macOS. The pinned Ruff command used here when no global installation is present is `UV_CACHE_DIR=/private/tmp/odr-uv-cache UV_TOOL_DIR=/private/tmp/odr-uv-tools uvx --from ruff==0.16.8 ruff`. Format commands in the check list inspect files without rewriting them. To intentionally format owned code, run `clang-format-21 -i` on the four portable C++ files and `ruff format tools`, then rerun the checks. Never format vendored code or Unreal engine code in this pass.

The test categories are: portable command and layout scenarios (`ctest`), sanitizer scenarios, Unreal automation (`ODR.BlacksmithMine`), and packaged unattended smoke (`-odrsmoke`). The versioned [eight-character scenario](../game/scenarios/blacksmith_mine.jsonl) is replayed by the headless tool and Unreal; a [twelve-character scenario](../game/scenarios/blacksmith_mine_12.jsonl) measures the larger roster. The Unreal-facing check must use the staged shared dylib; a monolithic packaged Mac core is not a supported deployment. To replay or measure them headlessly:

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
"$UE_ROOT/Engine/Binaries/Mac/UnrealEditor-Cmd" "$PWD/Unreal/OpenDynamicRPG.uproject" -unattended -nullrhi -nosplash -ExecCmds='Automation RunTests ODR.BlacksmithMine;Quit' -ReportExportPath="$PWD/build/reports/unreal"
python3 tools/check_unreal_report.py
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun -project="$PWD/Unreal/OpenDynamicRPG.uproject" -platform=Mac -clientconfig=Development -build -cook -stage -pak -package -archive -archivedirectory="$PWD/build/package" -unattended -nop4
"$PWD/build/package/Mac/OpenDynamicRPG.app/Contents/MacOS/OpenDynamicRPG" -odrsmoke -unattended -nullrhi -nosplash -stdout -FullStdOutLogOutput > build/packaged-smoke.log 2>&1
rg 'ODR_SMOKE_PASS' build/packaged-smoke.log
```

Open `Unreal/OpenDynamicRPG.uproject` in Unreal Editor for visual inspection or run the Development game. The mode creates its own camera and labeled primitives. On the creation screen, select class with 1–6, cycle ancestry with C and background with B, then press Enter. The on-screen footer gives phase-specific controls; F5 saves to Unreal's project Saved directory as `session.json`, and F9 loads. The packaged Mac app stores it under `~/Library/Containers/com.YourCompany.OpenDynamicRPG/Data/Library/Application Support/Epic/OpenDynamicRPG/Saved/`; the status line prints the actual path. The authored route is city recruitment and mine acceptance, hex travel to the mine, main battle, ore pickup, return to city, upgrade. The optional ruins, clue, hidden cache and secret fight can be explored without blocking normal progress. To replace a primitive, add a cooked `model` or `material` path under its `visual_id` in `game/content/visuals.json`; keep `fallback_mesh`. Set `mesh_type` to `skeletal` for rigged characters or creatures and optionally set `animation_class` to a cooked animation Blueprint class path. Static models use `mesh_type: static` or omit it. Attachment points and animation roles are contracts for future art integration, not combat inputs.

## Content, generated adventures and saves

Edit `game/content/mine.json` for template bindings and `game/content/visuals.json` for presentation. Run `python3 tools/stage_content.py` after either changes. Both authored and generated runs receive a resolved layout and pass the same validation. `odr_tool` can play a JSONL command script to review the accepted layout, room connections, objective coordinates and event results in its printed snapshot. For a generated run, use `{"action":"create_run","mode":"generated","seed":42}` after hero creation; compare other seeds and confirm that each run ID has independent `changes` and enemy records. Rejected candidates report objective or deployment diagnostics; after at most sixteen attempts the session accepts a validated authored fallback and records `layout_source: authored_fallback`. A standalone failed `validate_layout` command leaves the prior state untouched. A `boss: true` run has an upper floor and a separate lower main-encounter floor; `descend` and `ascend` use its stairs.

Approach-dependent deployment is part of the accepted gameplay state. Dungeon movement updates `mine_previous_pos`; when the party reaches an encounter trigger, the prior walkable hex is preferred as the battle entry approach, with a validated fallback neighbor only when necessary. The battle snapshot stores both `approach` and `trigger`, so deployment and retreat use the same entry route. Retreat returns the party to `approach` and records the trigger as the next previous position. These fields are saved with the canonical snapshot and must survive load/replay.

Save JSON has `schema_version: 1`, `template_id`, run IDs, the accepted layout and all changes. When changing the schema, add a migration or reject old saves explicitly; add a before/after fixture and a roundtrip test. Never recover a layout only from a seed. An action or animation may be interrupted by a save; the canonical state is the portable snapshot, and visual playback must not write rules.

## Luna implementation and Sol review

Use Luna for a bounded, testable implementation slice: state the affected files, acceptance example, architecture constraints and exact checks, then let it implement and run them. Keep slices small enough that the diff and test evidence can be reviewed together. After local checks pass, give Sol the complete diff once for a focused review of rule ownership, save/reload behavior, content validation, generated/adventure identity, test gaps, CI trust changes and license boundaries. Ask for prioritized actionable findings with file/line references. Send those findings back to Luna for repairs; request a targeted Sol re-review only if a fix changes a risky boundary. A failed check goes straight back to implementation before review. Do not spend Sol turns on routine formatting or repeated status polling. Model billing or Codex usage savings are not guaranteed by this workflow; measure actual usage in the account if cost matters.

Prompts for the two passes can be concise:

```text
Luna: Implement <one slice> within the existing ADRs. Add one behavior-focused scenario, run the exact relevant checks, and report changed files, results and remaining risks.
Sol: Review this completed diff and check output for correctness and omissions. Prioritize authoritative-state, save/layout identity, generated fallback, gameplay acceptance and CI runner trust. Return concrete findings only; do not rewrite the implementation.
```

Do not create a separate Codex task automatically for every pass. An explicitly requested task or manual model selection is the point at which to use Luna or Sol. This implementation received one consolidated Sol review and one targeted re-review. The review found missing generated-layout enemy/deployment validation, incomplete save-state checks and a static-only Unreal model binding. Those findings were repaired, with focused corrupted-save and layout tests; the Unreal adapter now accepts optional skeletal bindings. No extra Sol pass was spent on formatting or routine status checks.

## Mac validation and runner boundary

The repository-scoped macOS ARM64 runner `odr-mac-mini` was registered on 2026-09-20 and verified online with the `odr-mac` label. Two project PR runs passed the portable Linux checks and the Unreal Editor/package/smoke checks on this runner. Sol review then identified a public-fork workflow bypass: a fork can change the `pull_request` workflow and request a repository-scoped self-hosted runner even if the original Mac job has a trust condition. Its public registration was removed; the same installation is now registered only with the private `simonperegrinvs/open-dynamic-rpg-ci` repository. The public workflow uses GitHub-hosted Linux only. [ADR 0005](adr/0005-ci-trust-boundary.md) records the decision; do not re-register this runner with the public repository while fork PR workflow changes can reach it.

Run `bash tools/run_mac_ci.sh` from an APFS checkout for local Mac validation and record the tested commit SHA in the review. Runner 2.337.0 is installed at `/Users/simon/.local/share/odr-actions-runner` on APFS; its downloaded archive matched the SHA-256 recorded in [TOOLS.md](TOOLS.md). It runs under the `simon` account and has no signing secrets. A dedicated account would require macOS administrator credentials. The private workflow can be dispatched manually with PR number and exact head/merge SHAs; it verifies those refs before building and has read-only workflow permissions. Automatic dispatch and a public required Mac status still need a least-privilege credential design. Automatic approval review rejected a proposed persistent watcher with Actions write access and a status token, so that path was not installed. `main` protection currently requires `linux-portable` only; the private run URL and SHA are review evidence, not a PR status check.

When a Mac check fails, inspect Unreal's `Saved/Logs`, `build/reports/unreal/index.json`, the packaged smoke log, Xcode selection (`xcode-select -p`) and free disk space. Verify the portable `ue-release` library exists before Unreal Build Tool runs. In the runner installation, `./svc.sh status` checks the private repository's user service; `gh api repos/simonperegrinvs/open-dynamic-rpg-ci/actions/runners` checks its online status. If `._*` files appear in a package or source, move the checkout to APFS and clean generated directories. If automation appears idle after a test, use the semicolon form `Automation RunTests ODR.BlacksmithMine;Quit`; a separate comma-separated console `Quit` can stop or hang at the wrong time. Restart or update the service only after draining jobs, then manually verify one exact-commit private run.

## Current measured limits

On this M4 Pro Mac (64 GB), the final portable dev CTest completed in 7.51 seconds and the address/undefined-behavior sanitizer CTest in 23.37 seconds on 2026-09-20. The Unreal Editor target built with Xcode 26.6; its blacksmith/mine automation test passed with zero errors. The final staged shared-dylib package and unattended packaged smoke passed after the HUD-label, event-playback and save-validation changes. The first package attempt on `T9` failed at ad-hoc signing because of `._PkgInfo`.

Three Release-build headless replays per party size, made after final packaging, produced these medians; `peak RSS` is the headless process, and the battle figures come from the deterministic test driver. `movement-only` means a party turn spent moving and defending without an attack. It is a navigation-pressure indicator, not a claim about human player mistakes. The raw result is in ignored local `build/measurements.json` and can be regenerated with `tools/measure.py`.

| Active party | Layout generation | Save JSON | Load | Peak RSS | Replay commands to upgrade | Main-battle party turns / movement-only / rounds | Battle-driver time |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 8 | 2.10 ms | 5,855 bytes | 2.15 ms | 2.85 MB | 95 | 19 / 8 / 3 | 103 ms |
| 12 | 2.68 ms | 6,679 bytes | 2.10 ms | 3.01 MB | 108 | 23 / 11 / 2 | 145 ms |

Visual checks for the eight-character city, dungeon and battle slice ran at approximately 60 fps with placeholder geometry rendering. A twelve-character battle also ran at approximately 60 fps and the game process used about 672 MiB RSS. These were desktop observations, not a repeatable frame-time profile. Initial 3D labels became colored blocks after a save/load scene rebuild, so labels now render in the HUD at projected marker positions. The final package displayed readable labels after F5/F9 reload. Keyboard controls were verified through hero creation, eight-character recruitment, overworld travel, mine entry and clue exploration; the complete keyboard-only battle-to-upgrade expedition still needs a human playthrough. The canonical full expedition passed both the Unreal automation and packaged smoke tests.

These are engineering microbenchmarks, not a measured 30–45 minute player expedition. Repeatable rendered frame-time and Unreal memory profiling for both party sizes still need to precede an active-party limit. The lower boss floor currently reuses the same compact hex geometry with a distinct floor identity; more varied geometry and area abilities remain future work.
