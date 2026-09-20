"""Inspect formatting and clang-tidy for owned portable C++ without rewriting files."""

from __future__ import annotations

import platform
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FILES = (
    "core/src/session.cpp",
    "core/include/odr/session.h",
    "tools/odr_tool.cpp",
    "tests/session_tests.cpp",
)


def resolve_tool(name: str) -> str:
    found = shutil.which(f"{name}-21") or shutil.which(name)
    if found:
        return found
    homebrew = Path(f"/opt/homebrew/opt/llvm@21/bin/{name}")
    if homebrew.exists():
        return str(homebrew)
    if name == "clang-format" and platform.system() == "Darwin":
        return subprocess.check_output(["xcrun", "--find", name], text=True).strip()
    raise SystemExit(f"{name} 21 is required")


def main() -> None:
    formatter = resolve_tool("clang-format")
    tidy = resolve_tool("clang-tidy")
    for tool in (formatter, tidy):
        version = subprocess.check_output([tool, "--version"], text=True)
        if "21." not in version:
            raise SystemExit(f"expected version 21: {version.strip()}")
    subprocess.run([formatter, "--dry-run", "--Werror", *FILES], cwd=ROOT, check=True)
    arguments = [
        tidy,
        "-p",
        "build/dev",
        "core/src/session.cpp",
        "tools/odr_tool.cpp",
        "tests/session_tests.cpp",
        "--quiet",
    ]
    if platform.system() == "Darwin":
        sdk = subprocess.check_output(["xcrun", "--show-sdk-path"], text=True).strip()
        arguments.extend(
            (f"--extra-arg=-isystem{sdk}/usr/include/c++/v1", f"--extra-arg=-isysroot{sdk}")
        )
    subprocess.run(arguments, cwd=ROOT, check=True)
    print("portable C++ formatting and clang-tidy passed")


if __name__ == "__main__":
    main()
