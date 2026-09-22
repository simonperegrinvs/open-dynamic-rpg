"""Require both the canonical scenario and player adapter Unreal checks to pass."""

from __future__ import annotations

import json
import sys
from pathlib import Path

REQUIRED_TESTS = {"ODR.BlacksmithMine", "ODR.PlayerAdapter"}


def main() -> None:
    path = Path(sys.argv[1] if len(sys.argv) > 1 else "build/reports/unreal/index.json")
    report = json.loads(path.read_text(encoding="utf-8-sig"))
    tests = report.get("tests", [])
    for name in sorted(REQUIRED_TESTS):
        matches = [test for test in tests if test.get("fullTestPath") == name]
        if len(matches) != 1 or matches[0].get("state") != "Success":
            raise SystemExit(f"Unreal automation did not pass: {name}")
    if report.get("failed") != 0:
        raise SystemExit("Unreal automation report includes failed tests")
    print("Unreal canonical scenario and player adapter automation passed")


if __name__ == "__main__":
    main()
