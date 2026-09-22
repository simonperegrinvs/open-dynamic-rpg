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
| Blender and Rigify | Blender 5.2.1 LTS, build `9e2066aef7ef`; production Rigify workflow still to validate | Blender is [GPL](https://www.blender.org/about/license/). The original [two-bone binding probe](../ArtSource/BindingProbe/README.md) uses Blender's built-in armature and FBX tools; it does not depend on Rigify or an add-on. Source art remains separate from gameplay. |
| GitHub Actions | `ubuntu-24.04` for portable checks; `actions/checkout` v7.0.1 pinned to a release commit. Mac ARM64 runner 2.337.0 archive SHA-256 `5a2cd92908a93d7276a194e1de6008099f3e7946f3f8e14aa7a1a7b4a31fdec2` | Hosted Linux CI is active. The Mac runner completed integration runs, then was moved from this public repository to a private companion for manually dispatched, exact-commit builds. It does not report a required public PR status. [Runner license](https://github.com/actions/runner/blob/main/LICENSE). |

The game content, original binding probe, [diorama kit](../ArtSource/Diorama/README.md)
and owned code use the repository [MIT license](../LICENSE). No code or art from
the reference RPGs is imported. The diorama kit was generated with Blender 5.2.1
LTS using built-in geometry, armature and FBX tools; no add-on or downloaded art
dependency was added. Its editable sources and FBX files live in
`ArtSource/Diorama/`, reproduction scripts in `tools/art_probe/`, and imported
assets in `Unreal/Content/Art/Diorama/`. The older two-bone integration probe stays
in the corresponding `BindingProbe/` directories. Record each future asset's
source and license when added.
