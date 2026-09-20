# Pinned tools and license inventory

The portable build and CI pin major compiler tooling and exact Python dependency versions. Record any changed version here with the build/test result. Third-party source stays in `third_party/` and is not covered by the repository's original-code MIT license.

| Tool or component | Version or source | Role and license boundary |
| --- | --- | --- |
| Unreal Engine | Installed 5.8.2; [Epic terms](https://www.unrealengine.com/eula/unreal) | Game host only; Epic's EULA, not vendored. |
| Xcode toolchain | Installed 26.6; [Epic macOS requirements](https://dev.epicgames.com/documentation/en-us/unreal-engine/macos-development-requirements-for-unreal-engine) recommend 26.1.1 for UE 5.8 | Apple compiler/SDK for Mac builds; Xcode editor is optional. Editor build and automation test passed locally with 26.6. |
| CMake / CTest | Minimum 3.25; local 4.2.3 | Portable build and scenarios; `ODR_SHARED=ON` builds the Unreal-facing `libodr_core.dylib`; [BSD-3-Clause](https://gitlab.kitware.com/cmake/cmake/-/blob/master/Copyright.txt). |
| C++ | C++20 | Owned `core/` and tool code; compiler warnings are errors. |
| nlohmann/json | [3.12.0](https://github.com/nlohmann/json/releases/tag/v3.12.0); source archive SHA-256 `42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa` | Vendored in `third_party/nlohmann_json`; [MIT license](../third_party/nlohmann_json/LICENSE.MIT). Unreal links the compiled portable library through an external module. |
| clang-format / clang-tidy | 21; local Apple clang-format 21.0.0 and Homebrew LLVM/clang-tidy 21.1.8 | Format inspection and static analysis of owned portable C++; LLVM [Apache-2.0 with LLVM exceptions](https://llvm.org/LICENSE.txt). Linux CI installs LLVM 21 tools. |
| Python | Minimum 3.12; local 3.14.6 | Development scripts only; [PSF license](https://docs.python.org/3/license.html). |
| Ruff | [0.16.8](https://github.com/astral-sh/ruff/releases/tag/0.16.8) | Python lint and format inspection; [MIT license](https://github.com/astral-sh/ruff/blob/main/LICENSE). |
| Blender and Rigify | Production baseline; version to pin with first art asset | Blender is [GPL](https://www.blender.org/about/license/) and Rigify ships with Blender; source art remains separate from gameplay. No character source is imported yet. |
| GitHub Actions | `ubuntu-24.04` for portable checks; repository-scoped Mac ARM64 runner 2.337.0, archive SHA-256 `5a2cd92908a93d7276a194e1de6008099f3e7946f3f8e14aa7a1a7b4a31fdec2` | Workflow execution service. Runner registered and online on 2026-09-20; required branch checks need the first pull-request run. [Runner license](https://github.com/actions/runner/blob/main/LICENSE). |

The game content and owned code use the repository [MIT license](../LICENSE). No code or art from the reference RPGs is imported. The visual binding file can point to original cooked assets later; record each imported asset's source and license when added.
